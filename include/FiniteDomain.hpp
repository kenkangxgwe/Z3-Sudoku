#pragma once

#include <string>
#include <ostream>

namespace Z3_Sudoku{

template<int L, int U>
class FiniteDomain
{

    class exception {
    public:
        constexpr explicit exception(int const num) {
            msg_string = "Invalide num " + std::to_string(num)
                    + ", the number should be in the range of ["
                    + std::to_string(L) + ", " +std::to_string(U) + "].";
        };
        constexpr char const *msg(){
            return msg_string.c_str();
        };

    private:
        std::string msg_string;
    };

public:
    static int const LB = L;
    static int const UB = U;
    static int const size = U - L + 1;

    constexpr FiniteDomain() {
        value = L;
    };

    constexpr FiniteDomain(int const num) {
        if(num < L || num > U) {
            throw(exception(num));
        }
        value = num;
    };

    constexpr FiniteDomain(std::string const &num_string)
            :FiniteDomain(std::stoi(num_string)) {};

    constexpr FiniteDomain(FiniteDomain<L,U> const &newfd)
            :value(newfd.value)
    {
    }

    constexpr FiniteDomain(FiniteDomain<L,U> &&newfd) noexcept
            :value(std::move(newfd.value))
    {
    }

    constexpr FiniteDomain<L,U> &operator=(FiniteDomain<L,U> const &newfd) {
        value = newfd.value;
        return (*this);
    };

    constexpr FiniteDomain<L,U> &operator=(FiniteDomain<L,U> &&newfd) noexcept{
        value = std::move(newfd.value);
        return (*this);
    };

    constexpr inline int operator()() const {
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
inline constexpr bool operator==(FiniteDomain<L, U> d1, FiniteDomain<L, U> d2)
{
    return d1() == d2();
}

template<int L, int U>
inline std::ostream &operator<<(std::ostream &out, FiniteDomain<L, U> const &num)
{
    return out << num();
}

}
