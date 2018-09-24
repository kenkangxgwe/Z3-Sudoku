#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <array>
#include "z3++.h"
#include "FiniteDomain.hpp"
#include "SymName.h"

namespace Z3_Sudoku
{

typedef FiniteDomain<0, 9> Digit;
typedef FiniteDomain<0, 8> RowNum, ColNum, BlkNum;
typedef std::tuple<RowNum, ColNum, Digit> Cell;
typedef std::array<Cell, 9> Partition, Row, Col, Blk;
typedef std::array<std::array<Digit, 9>, 9> Layout;

class Board
{
public:
    Board(z3::context &, std::string);
    ~Board();
    Row getRow(RowNum rownum) const;
    Col getCol(ColNum colnum) const;
    Blk getBlk(BlkNum blknum) const;
    std::vector<Cell> checkInitial(z3::context &, z3::solver &);
    z3::check_result findSolution(z3::context&, z3::solver&);
    Layout retrieveBoard(z3::context &, z3::model const &);

private:
    Layout cells;
    Cell findCell(SymName) const;
};

}
