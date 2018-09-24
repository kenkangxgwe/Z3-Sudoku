#pragma once

#include <string>
#include <ostream>

namespace Z3_Sudoku{

template<int L, int U>
class FiniteDomain
{

    class exception {
    public:
        explicit exception(int num) {
            msg_string = "Invalide num " + std::to_string(num)
                    + ", the number should be in the range of ["
                    + std::to_string(L) + ", " +std::to_string(U) + "].";
        };
        char const *msg() {
            return msg_string.c_str();
        };

    private:
        std::string msg_string;
    };

public:
    static int const LB = L;
    static int const UB = U;
    static int const size = U - L + 1;

    FiniteDomain() {
        value = L;
    };

    FiniteDomain(int num) {
        if(num < L || num > U) {
            throw(exception(num));
        }
        value = num;
    };

    FiniteDomain(std::string const &num_string)
            :FiniteDomain(std::stoi(num_string)) {};

    inline int operator()() const {
        return value;
    };

private:
    int value;

};

template<int L, int U>
inline std::ostream &operator<<(std::ostream& out, typename FiniteDomain<L, U>::exception const &e)
{
    return out << e.msg();
}

template<int L, int U>
inline bool operator==(FiniteDomain<L, U> d1, FiniteDomain<L, U> d2)
{
    return d1() == d2();
}

template<int L, int U>
inline std::ostream &operator<<(std::ostream &out, FiniteDomain<L, U> const &num)
{
    return out << num();
}

}
