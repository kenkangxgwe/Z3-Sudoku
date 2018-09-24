#include <iostream>
#include <fstream>
#include "z3++.h"
#include "Board.h"

using namespace z3;

int main(int argc, char *argv[])
{
    if(argc != 2) {
        std::cout << "Please speicfy an initial input." << std::endl;
        return 1;
    }

    z3::context ctx;
    z3::solver sol(ctx);
    Z3_Sudoku::Board board{ctx, argv[1]};
    auto incon_cells = board.checkInitial(ctx, sol);
    if(!incon_cells.empty()) {
        std::cout << "The initial board is inconsistent:" << std::endl;
        for(auto const &[row, col, digit] : incon_cells) {
            std::cout << "Row: " << row() + 1 << ", Col: " << col() + 1 << ", Digit: " << digit << std::endl;
        }
        return 0;
    }
    switch(board.findSolution(ctx, sol)) {
        case unsat: {
            std::cout << "The board is unsolvable." << std::endl;
        } break;
        /**
         * if the systme is satisfiable, we find one of its models,
         * and evaluate every final symbols to retrieve the solution,
         */
        case sat: {
            auto solution = board.retrieveBoard(ctx, sol.get_model());
            for(auto const &row:solution) {
                for(auto const &cell: row) {
                    std::cout << cell << " ";
                }
                std::cout << std::endl;
            }
        } break;
        case unknown:break;
    };
    return 0;
}
