#include "Board.h"
#include <fstream>

using namespace z3;

namespace Z3_Sudoku
{

expr mk_eq(context &ctx, SymName const &sym_name, Digit digit)
{
    expr_vector conditions{ctx};
    for(int i  = 0; i < 4; i ++) {
        SymName new_sym_name(sym_name);
        new_sym_name.push_back(i);
        if((digit() >> i) & 1) {
            conditions.push_back(ctx.bool_const(new_sym_name.toString().c_str()));
        } else {
            conditions.push_back(!ctx.bool_const(new_sym_name.toString().c_str()));
        }
    }
    return mk_and(conditions);
}

expr partitionConsistency(context &ctx, Partition const &part, std::string const &sym_name)
{
    expr_vector part_cons(ctx);
    for(int i = 0; i < part.size(); i++) {
        expr_vector cell_cons(ctx);
        RowNum row = std::get<0>(part[i]);
        ColNum col = std::get<1>(part[i]);
        for(int d = Digit::LB; d <= Digit::UB; d++) {
            expr antecedent = mk_eq(ctx, {sym_name, {row(), col()}}, d);
            if(d == 0) {
                expr consequence = ctx.bool_val(true);
                cell_cons.push_back(implies(antecedent, consequence));
                continue;
            }
            expr_vector consequences(ctx);
            for(int j = 0; j < part.size(); j++) {
                if(i != j) {
                    consequences.push_back(!mk_eq(ctx, {sym_name,
                                                        {std::get<0>(part[j])(), std::get<1>(part[j])()}}, d));
                }
            }
            cell_cons.push_back(implies(antecedent, mk_and(consequences)));
        }
        part_cons.push_back(mk_and(cell_cons));
    }
    return mk_and(part_cons);
}

z3::expr getConstraints(z3::context &ctx, Board const &board, std::string const &sym_name)
{
    expr_vector conditions(ctx);
    for(int r = 0; r < RowNum::size; r++) {
        conditions.push_back(partitionConsistency(ctx, board.getRow(r), sym_name));
    }

    for(int c = 0; c < ColNum::size; c++) {
        conditions.push_back(partitionConsistency(ctx, board.getCol(c), sym_name));
    }

    for(int b = 0; b < RowNum::size; b++) {
        conditions.push_back(partitionConsistency(ctx, board.getBlk(b), sym_name));
    }
    return mk_and(conditions);
}

expr_vector removeSyms(context &ctx, expr_vector const &syms, expr_vector const &rems)
{
    expr_vector new_syms(ctx);
    for(auto const &sym: syms) {
        bool in_rems = false;
        for(auto const &rem : rems) {
            if(eq(sym, rem)) {
                in_rems = true;
                break;
            }
        }
        if(!in_rems) {
            new_syms.push_back(sym);
        }
    }
    return new_syms;
}

Board::Board(context &context, std::string inputfile)
{
    std::ifstream sudoku_file{inputfile};
    if(!sudoku_file) {
        throw ("File not acceesible.");
    }

    for(int i = 0; i < RowNum::size; i++) {
        for(int j = 0; j < ColNum::size; j++) {
            std::string cur;
            if(j == ColNum::size - 1) {
                getline(sudoku_file, cur);
            } else {
                getline(sudoku_file, cur, ',');
            }
            cells[i][j] = Digit(cur);
        }
    }
}

Board::~Board()
{
}

std::vector<Cell> Board::checkInitial(context &ctx, solver &sol)
{
    expr_vector constraints(ctx);
    // consistency constraints
    constraints.push_back(getConstraints(ctx, *this, "i"));

    // initial assumptions
    for(int r = 0; r < RowNum::size; r++) {
        for(int c = 0; c < RowNum::size; c++) {
            constraints.push_back(implies(
                    ctx.bool_const(SymName("i", {r, c}).toString().c_str()),
                    mk_eq(ctx, {"i", {r, c}}, cells[r][c])
            ));
        }
    }

    // assumption symbols
    expr_vector cell_syms(ctx);
    for(int r = 0; r < RowNum::size; r++) {
        for(int c = 0; c < ColNum::size; c++) {
            cell_syms.push_back(ctx.bool_const(SymName("i", {r, c}).toString().c_str()));
        }
    }

    /**
     * if the system is satisfiable, then the board is consistent initially,
     * otherwise is inconsistent.
     */
    std::vector<Cell> incon_cells;
    try {
        sol.add(mk_and(constraints));
        /**
         * if the board is inconsistent, negate the unsat cores
         * to find all inconsistent cells until it is consistent.
         */
        while(sol.check(cell_syms) == check_result::unsat) {
            auto unsat_cores = sol.unsat_core();
            for(auto const &core : unsat_cores) {
                incon_cells.push_back(findCell(core.to_string()));
            }
            cell_syms = removeSyms(ctx, cell_syms, unsat_cores);
        }
    } catch(exception &e) {
        std::cerr << e << std::endl;
    }
    if(incon_cells.empty()) {
        /**
         * if the initial board is consistent,
         * add all the initial cells into assertions and prepare for solving.
         */
        sol.add(mk_and(cell_syms));
    }
    return incon_cells;
}

check_result Board::findSolution(context &ctx, solver &sol)
{
    expr_vector constraints(ctx);

    // consistency constraints
    constraints.push_back(getConstraints(ctx, *this, "f"));

    // initial constraints
    for(int r = 0; r < RowNum::size; r++) {
        for(int c = 0; c < ColNum::size; c++) {
            SymName initial_sym_name("i", {r, c});
            SymName final_sym_name("f", {r, c});
            for(int d = Digit::LB; d <= Digit::UB; d++) {
                expr antecedent = mk_eq(ctx, initial_sym_name, d);
                /**
                 * if initial cell is 0, then its final value cannot be 0,
                 * otherwise it should be equal to its final value.
                 */
                if(d == 0) {
                    expr_vector consequences(ctx);
                    for(int nd = Digit::LB + 1; nd <= Digit::UB; nd++) {
                        consequences.push_back(mk_eq(ctx, final_sym_name, nd));
                    }
                    constraints.push_back(implies(antecedent, mk_or(consequences)));
                } else {
                    expr consequence = mk_eq(ctx, final_sym_name, d);
                    constraints.push_back(implies(antecedent, consequence));
                }
            }
        }
    }

    expr_vector cell_syms(ctx);
    for(int r = 0; r < RowNum::size; r++) {
        for(int c = 0; c < ColNum::size; c++) {
            cell_syms.push_back(ctx.bool_const(SymName("f", {r, c}).toString().c_str()));
        }
    }

    sol.add(mk_and(constraints));

    /**
     * if the system is satisfiable, otherwise the board is unsolvable.
     */
    return sol.check();
}

Row Board::getRow(RowNum rownum) const
{
    Row row = {};
    for(int c = 0; c < ColNum::size; c++) {
        row[c] = std::make_tuple(rownum(), c, cells[rownum()][c]);
    }
    return row;
}

Col Board::getCol(ColNum colnum) const
{
    Col col = {};
    for(int r = 0; r < RowNum::size; r++) {
        col[r] = std::make_tuple(r, colnum(), cells[r][colnum()]);
    }
    return col;
}

Blk Board::getBlk(BlkNum blknum) const
{
    Blk blk = {};
    for(int i = 0; i < BlkNum::size / 3; i++) {
        for(int j = 0; j < BlkNum::size / 3; j++) {
            blk[i * 3 + j] = std::make_tuple(
                    blknum() - blknum() % 3 + i,
                    (blknum() % 3) * 3 + j, cells[blknum() - blknum() % 3 + i][(blknum() % 3) * 3 + j]);
        }
    }
    return blk;
}

Cell Board::findCell(SymName sym_name) const
{
    RowNum incon_row(sym_name.getSub(0));
    ColNum incon_col(sym_name.getSub(1));
    Digit incon_digit(cells[incon_row()][incon_col()]);
    return std::make_tuple(incon_row, incon_col, incon_digit);
}

/**
 * we find one of its models,
 * and evaluate every final symbols to retrieve the solution,
 */
Layout Board::retrieveBoard(context &ctx, model const &model)
{
    Layout solution = {};

    for(int r = 0; r < RowNum::size; r++) {
        for(int c = 0; c < ColNum::size; c++) {
            int num = 0;
            for(int d = 0; d < 4; d++) {
                if(model.eval(ctx.bool_const(SymName("f", {r, c, d}).toString().c_str())).bool_value() == Z3_L_TRUE) {
                    num |= 1 << d;
                }
            }
            solution[r][c] = Digit(num);
        }
    }
    return solution;
}

}
