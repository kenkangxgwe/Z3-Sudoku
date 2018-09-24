#include <utility>

#include <utility>
#include <cstdlib>

//
// Created by kenkangxgwe on 2018.9.23.
//

#include "SymName.h"

namespace Z3_Sudoku
{

SymName::SymName(std::string sym_string)
{
    size_t name_end_pos = sym_string.find('_');
    if(name_end_pos == std::string::npos) {
        throw (exception(exception::TYPE::NONAME));
    }
    name = sym_string.substr(0, name_end_pos - 0);
    size_t sub_start_pos = name_end_pos + 1;
    while(true) {
        size_t sub_end_pos = sym_string.find('_', sub_start_pos);
        if(sub_end_pos == std::string::npos) {
            if(sub_start_pos <= sym_string.size() - 1) {
                subscripts.push_back(std::stoi(sym_string.substr(sub_start_pos)));
            }
            return;
        }
        subscripts.push_back(std::stoi(sym_string.substr(sub_start_pos, sub_end_pos - sub_start_pos)));
        sub_start_pos = sub_end_pos + 1;
    }
}

SymName::SymName(std::string name, std::initializer_list<int> list)
        : name(std::move(name)), subscripts(list)
{
    updateSubString();
}

std::string Z3_Sudoku::SymName::toString() const
{
    return name + "_" + sub_string;
}

std::string SymName::getName() const
{
    return name;
}

std::vector<int> SymName::getSubs() const
{
    return subscripts;
}

void SymName::push_back(int new_sub)
{
    subscripts.push_back(new_sub);
    updateSubString();
}

void SymName::updateSubString()
{
    sub_string.clear();
    for(auto const &sub : subscripts) {
        sub_string += std::to_string(sub) + "_";
    }
    sub_string.pop_back();
}

int SymName::getSub(size_t pos) const
{
    return subscripts[pos];
}

void SymName::setName(std::string new_name)
{
    name = std::move(new_name);
}

SymName::exception::exception(SymName::exception::TYPE type)
        :exception_type(type)
{
}

char const *SymName::exception::msg()
{
    switch(exception_type) {
        case NONAME: {
            return "The input parameter has no name.";
        }
        case NOCLOSE:{
            return "The input parameter has no closed bracket ')'.";
        }
    }
}

}
