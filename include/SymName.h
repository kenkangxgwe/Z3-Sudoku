#pragma once

#include <string>
#include <vector>

namespace Z3_Sudoku{

class SymName
{
public:

    class exception{
    public:
        enum TYPE {
            NONAME
        };
        constexpr exception(TYPE);
        char const *msg();
    private:
        TYPE exception_type;
    };

    SymName(std::string);
    SymName(std::string, std::initializer_list<int>);
    std::string toString() const;
    std::string getName() const;
    std::vector<int> getSubs() const;
    int getSub(size_t) const;
    void push_back(int);

    void setName(std::string new_name);
private:
    void updateSubString();
    std::string name;
    std::string sub_string;
    std::vector<int> subscripts;
};

}
