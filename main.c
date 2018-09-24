#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<memory.h>
#include<assert.h>
#include<z3.h>

#define LOG_Z3_CALLS

#ifdef LOG_Z3_CALLS
#define LOG_MSG(msg) Z3_append_log(msg)
#else
#define LOG_MSG(msg) ((void)0)
#endif

#define BLKSIZE 3
#define LOWERB 0
#define UPPERB 9

typedef int Board[9][9];

Z3_sort getCoordSort(Z3_context ctx)
{
    return Z3_mk_finite_domain_sort(
            ctx,
            Z3_mk_string_symbol(ctx, "coord_sort"),
            UPPERB);
}

Z3_sort getDigitSort(Z3_context ctx)
{
    return Z3_mk_finite_domain_sort(
            ctx,
            Z3_mk_string_symbol(ctx, "digit_sort"),
            UPPERB + 1);
}

Z3_ast generateCoordinate(Z3_context ctx, Z3_solver solver, char *name)
{
    Z3_ast coord = Z3_mk_const(
            ctx,
            Z3_mk_string_symbol(ctx, name),
            getCoordSort(ctx)
    );
    return coord;
}

Z3_ast constructBoardArray(Z3_context ctx /*, Z3_solver solver */, Board const *board)
{
    Z3_sort coord_sort = getCoordSort(ctx);
    Z3_sort digit_sort = getDigitSort(ctx);
    Z3_sort board_coords[2] = {coord_sort, coord_sort};
    Z3_sort board_sort = Z3_mk_array_sort_n(ctx, 2, board_coords, digit_sort);
    Z3_ast board_array = Z3_mk_const(
            ctx,
            Z3_mk_string_symbol(ctx, "board"),
            board_sort
    );

    /**
     * (9 * 9) is the largest number of constraints in a sudoku board,
     * 2 for the upper and lower bound, and 9 * 9 for all the cells.
     */
    //Z3_ast initialBoard[UPPERB * UPPERB];
    //unsigned int const_count = 0;
    for(int i = LOWERB; i < UPPERB; ++i) {
        for(int j = LOWERB; j < UPPERB; ++j) {
            if((*board)[i][j]) {
                Z3_ast coords[2] = {
                        Z3_mk_int(ctx, i, coord_sort),
                        Z3_mk_int(ctx, j, coord_sort)
                };
                //Z3_ast cell = Z3_mk_select_n(ctx, board_array, 2, coords);
                Z3_ast val = Z3_mk_int(ctx, (*board)[i][j], digit_sort);
                //initialBoard[const_count++] = Z3_mk_eq(ctx, cell, val);
                board_array = Z3_mk_store_n(ctx, board_array, 2, coords, val);
            }
        }
    }
    //Z3_solver_assert(
    //        ctx, solver,
            //Z3_mk_and(ctx, const_count, initialBoard)
    //);
    return board_array;
}

Z3_ast inSameRow(Z3_context ctx, Z3_ast *coord1, Z3_ast *coord2)
{
    return Z3_mk_eq(ctx, coord1[0], coord2[0]);
}

Z3_ast inSameCol(Z3_context ctx, Z3_ast *coord1, Z3_ast *coord2)
{
    return Z3_mk_eq(ctx, coord1[1], coord2[1]);
}

Z3_ast getBlkCoord(Z3_context ctx, Z3_ast coord, Z3_ast blk_coord_num)
{
    Z3_sort coord_sort = getCoordSort(ctx);
    Z3_ast blk_list[3];
    for(int i = 0; i < 3; ++i) {
        Z3_ast blk_coord_list[3];
        for(int j = 0; j < 3; ++j) {
            blk_coord_list[j] = Z3_mk_eq(
                    ctx, coord,
                    Z3_mk_int(ctx, i * 3 + j, coord_sort)
            );
        }
        blk_list[i] = Z3_mk_implies(
                ctx,
                Z3_mk_or(ctx, 3, blk_coord_list),
                Z3_mk_eq(ctx,
                         blk_coord_num,
                         Z3_mk_int(ctx, i, coord_sort
                         )
                )
        );
    }
    return Z3_mk_and(ctx, 3, blk_list);
}

Z3_ast inSameBlk(Z3_context ctx, Z3_ast *coord1, Z3_ast *coord2)
{
    Z3_sort coord_sort = getCoordSort(ctx);
    Z3_ast blk_row1 = Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "blk_row1"), coord_sort);
    Z3_ast blk_col1 = Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "blk_col1"), coord_sort);
    Z3_ast blk_row2 = Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "blk_row2"), coord_sort);
    Z3_ast blk_col2 = Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "blk_col2"), coord_sort);
    Z3_ast same_blk[6] = {
            getBlkCoord(ctx, coord1[0], blk_row1),
            getBlkCoord(ctx, coord1[1], blk_col1),
            getBlkCoord(ctx, coord2[0], blk_row2),
            getBlkCoord(ctx, coord2[1], blk_col2),
            Z3_mk_eq(ctx, blk_row1, blk_row2),
            Z3_mk_eq(ctx, blk_col1, blk_col2)
    };
    return Z3_mk_and(ctx, 6, same_blk);
}

Z3_ast notSameCell(Z3_context ctx, Z3_ast coord1[2], Z3_ast coord2[2])
{
    Z3_ast same_cell[2] = {
            inSameRow(ctx, coord1, coord2),
            inSameCol(ctx, coord1, coord2)
    };
    return Z3_mk_not(ctx, Z3_mk_and(ctx, 2, same_cell));
}

Z3_ast notEmpty(Z3_context ctx, Z3_ast cell1, Z3_ast cell2)
{
    Z3_sort digit_sort = getDigitSort(ctx);
    Z3_ast zero = Z3_mk_int(ctx, 0, digit_sort);
    Z3_ast not_empty[2] = {
            Z3_mk_not(ctx,
                      Z3_mk_eq(ctx,
                               cell1,
                               zero)),
            Z3_mk_not(ctx,
                      Z3_mk_eq(ctx,
                               cell2,
                               zero)),
    };
    return Z3_mk_and(ctx, 2, not_empty);
}

Z3_ast areDistinct(Z3_context ctx, Z3_ast cell1, Z3_ast cell2)
{
    return Z3_mk_not(ctx, Z3_mk_eq(ctx, cell1, cell2));
}

bool checkInitialState(Z3_context ctx, Z3_solver solver, Z3_ast board_array)
{

    Z3_sort coord_sort = getCoordSort(ctx);
    Z3_sort two_coords_sort[4] = {coord_sort, coord_sort, coord_sort, coord_sort};
    Z3_symbol two_coords_name[4] = {
            Z3_mk_string_symbol(ctx, "row1"),
            Z3_mk_string_symbol(ctx, "col1"),
            Z3_mk_string_symbol(ctx, "row2"),
            Z3_mk_string_symbol(ctx, "col2")
    };
    Z3_ast row1 = Z3_mk_const(ctx, two_coords_name[0], getCoordSort(ctx));
    Z3_ast col1 = Z3_mk_const(ctx, two_coords_name[1], getCoordSort(ctx));
    Z3_ast row2 = Z3_mk_const(ctx, two_coords_name[2], getCoordSort(ctx));
    Z3_ast col2 = Z3_mk_const(ctx, two_coords_name[3], getCoordSort(ctx));
    Z3_ast coord1[2] = {row1, col1};
    Z3_ast coord2[2] = {row2, col2};
    Z3_ast cell1 = Z3_mk_select_n(ctx, board_array, 2, coord1);
    Z3_ast cell2 = Z3_mk_select_n(ctx, board_array, 2, coord2);
    Z3_ast distinct_and_non_empty[2] = {
            notSameCell(ctx, coord1, coord2),
            notEmpty(ctx, cell1, cell2)
    };
    Z3_ast inSameArea[3] = {
            inSameRow(ctx, coord1, coord2),
            inSameCol(ctx, coord1, coord2),
            inSameBlk(ctx, coord1, coord2)
    };

    Z3_pattern patterns[2] = {
            Z3_mk_pattern(ctx, 1, &cell1),
            Z3_mk_pattern(ctx, 1, &cell2)
    };
    Z3_ast consistency_assert = Z3_mk_forall(
            ctx, 0, 0,
            patterns,
            4,
            two_coords_sort,
            two_coords_name,
            Z3_mk_implies(
                    ctx,
                    Z3_mk_and(ctx, 2, distinct_and_non_empty),
                    Z3_mk_implies(
                            ctx,
                            Z3_mk_or(ctx, 3, inSameArea),
                            areDistinct(ctx, cell1, cell2)
                    )
            )
    );
    printf("assert axiom:\n%s\n", Z3_ast_to_string(ctx, consistency_assert));

    Z3_solver_assert(ctx, solver, consistency_assert);
    Z3_ast_vector asserts = Z3_solver_get_assertions(ctx, solver);
    for(unsigned i = 0; i < Z3_ast_vector_size(ctx, asserts); ++i) {
        printf("%s\n", Z3_ast_to_string(ctx, Z3_ast_vector_get(ctx, asserts, i)));
    }
    Z3_lbool result = Z3_solver_check(ctx, solver);
    switch(result) {
        case Z3_L_FALSE: {
            Z3_ast_vector core = Z3_solver_get_unsat_core(ctx, solver);
            Z3_ast proof = Z3_solver_get_proof(ctx, solver);
            printf("proof: %s\n", Z3_ast_to_string(ctx, proof));
            printf("\ncore:\n");
            for(unsigned i = 0; i < Z3_ast_vector_size(ctx, core); ++i) {
                printf("%s\n", Z3_ast_to_string(ctx, Z3_ast_vector_get(ctx, core, i)));
            }
            printf("\n");
        }
            break;
        case Z3_L_UNDEF: {
            printf("unknown\n");
            printf("potential model:\n");
            Z3_model model = Z3_solver_get_model(ctx, solver);
            if(model) Z3_model_inc_ref(ctx, model);
            //display_model(ctx, stdout, model);
        }
            break;
        case Z3_L_TRUE: {
            printf("The initial board is inconsistent:\n");
            Z3_model model = Z3_solver_get_model(ctx, solver);
            if(model) {
                Z3_model_inc_ref(ctx, model);
                printf("counterexample:\n%s\n", Z3_model_to_string(ctx, model));
            }
            //display_model(ctx, stdout, model);
        }
            break;
    }

    return 0;
}

/**
 * the entry of the program.
 * @param argc number of input arguments
 * @param argv the input arguments. The first argument (zero-based) should be the input board.
 * @return the exit status.
 */
int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Please speicfy an initial input.\n");
        return EXIT_FAILURE;
    }

    Board board;
    FILE *fin = fopen(argv[1], "r");
    if(!fin) {
        printf("File not acceesible.\n");
        return EXIT_FAILURE;
    }
    size_t i = 0, j = 0;
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            int cur;
            do {
                cur = fgetc(fin);
            } while(cur < '0' || cur > '9');
            board[i][j] = (unsigned) (cur - '0');
        }
    }

    Z3_config conf = Z3_mk_config();
    Z3_set_param_value(conf, "model", "true");
    Z3_set_param_value(conf, "proof", "true");
    Z3_set_param_value(conf, "unsat_core", "true");
    Z3_context ctx = Z3_mk_context(conf);
    Z3_solver solver = Z3_mk_solver(ctx);
    Z3_solver_inc_ref(ctx, solver);

    Z3_ast board_array = constructBoardArray(ctx/*, solver*/, &board);
    bool isInitialValid = checkInitialState(ctx, solver, board_array);

    Z3_solver_dec_ref(ctx, solver);
    Z3_del_context(ctx);
    return EXIT_SUCCESS;
}
