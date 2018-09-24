#pragma once

#include <vector>
#include <array>
#include "z3++.h"
#include "FiniteDomain.hpp"
#include "SymName.h"

namespace Z3_Sudoku{

typedef FiniteDomain<0, 9> Digit;
class NumPred
{
public:
    NumPred(z3::context &, SymName, Digit = Digit(0));
    NumPred(NumPred &&) noexcept;
    NumPred& operator=(NumPred&&) noexcept;
    z3::expr isDigit(Digit);
    SymName getSymName() const;
    Digit getDigit() const;
    z3::expr getCondition() const;
    std::string toString() const;

private:
    Digit digit;
    SymName sym_name;
    z3::context *ctx;
    z3::expr condition;
};


}
