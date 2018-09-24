#include <utility>

#include "NumPred.h"
#include <string>
#include <NumPred.h>

using namespace z3;

namespace Z3_Sudoku
{

NumPred::NumPred(context &context, SymName sym_name, Digit digit)
        : ctx(&context),
          sym_name(std::move(sym_name)),
          digit(digit),
          condition(ctx->bool_val(true))
{
    condition = isDigit(digit);
}

NumPred::NumPred(NumPred &&numPred) noexcept
        : ctx(numPred.ctx),
          sym_name(numPred.sym_name),
          digit(numPred.digit),
          condition(numPred.condition)
{
    *this = std::move(numPred);
}

NumPred &NumPred::operator=(NumPred &&numPred) noexcept
{
    ctx = numPred.ctx;
    sym_name = numPred.sym_name;
    digit = numPred.digit;
    condition = numPred.condition;
    return *this;
}

z3::expr NumPred::isDigit(Digit digit)
{
    expr_vector conditions{*ctx};
    for(int i = Digit::LB + 1; i <= Digit::UB; i++) {
        SymName new_sym_name(sym_name);
        new_sym_name.push_back(i);
        if(i == digit()) {
            conditions.push_back(ctx->bool_const((new_sym_name.toString()).c_str()));
        } else {
            conditions.push_back(!ctx->bool_const((new_sym_name.toString()).c_str()));
        }
    }
    return mk_and(conditions);
}

Digit NumPred::getDigit() const
{
    return digit;
}

SymName NumPred::getSymName() const
{
    return sym_name;
}

std::string NumPred::toString() const
{
    return condition.to_string();
}

z3::expr NumPred::getCondition() const
{
    return condition;
}

}
