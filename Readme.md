# Sudoku Solver Based on [Z3Prover](https://github.com/Z3Prover/z3)

This is a Sudoku system checking program using Z3Prover [C++ APIs](http://z3prover.github.io/api/html/group__cppapi.html).
All the rules and constraints are **only** represented in propositional logic. 

## Formalization

To formalize the problem in propositional logic, we should not make the
programming language to do any preprocessing which results in simplication of
the logic system. Thus the program should construct the expressions in Z3
without any prior knowledge of the initial input, and assign the value of some
Boolean constants according to the input.

### Number Encoding 

Since we are using propositional logic, the only acceptable operators are NOT ¬,
OR ∨, AND ∧ , IMPLIES → and their combinations, the only values are true ⊤
and false ⊥ and no qualifiers allowed. To represent a digit in the cell denoted
by `cell(r,c)`, where `r` is the row and `c` is the column, we can encode them
using either binary or enumeration. The advantage of using binary is, we only
need 4 bits to represent a digit from `0` to `9`, otherwise we need 9 bits with
extra unique constraints.

We define Boolean variables `x(r,c,b)` to represent the b-th bit of `cell(r,c)` is
equal to 1. `x` is only a name, and `r`, `c` are indices running from `0` to `8`
and `b` running from `0` to `3`. Thus, the value of `cell(r,c)` is

``` cpp
(x(r,c,3) << 3) || (x(r,c,2) << 2) || (x(r,c,1) << 1) || (x(r,c,0) << 0)
```

, where `<<` is the left shift operator and `||` is the or operator in C++ . In
all, we need 324 Boolean constants to represent the digits of the entire board. (We
may need 810 if we apply enumeration method.)

We encapsulate the encoding process into a C++ function `Eq(r,c,d)` which returns a Z3
expression representing `cell(r,c) == d`. For example, `Eq(1,2,3)` will return:

``` lisp
(let (Eq(1,2,3)
      (and x(1,2,0)
           (not x(1,2,1))
           x(1,2,2)
           (not x(1,2,3))
      )
     )
)
```

### Consistency Constraints

To check the consistency of the board, we partition the board into 9
rows, 9 columns and 9 blocks. Every digit but `0` should appear at most once in
a partition. Therefore, unless a cell is `0`, its value will be unique. Let's
denote a function `Uni(r,c)`to express the uniqueness for `cell(r,c)`. For
example, let the partition be the second row of the board, then the uniqueness
constraints for `cell(2,0)` will be:

``` lisp
(let (Uni(2,0)
      (and (implies Eq(2,0,0)
                    true
           )
           (implies Eq(2,0,1)
                    (not Eq(2,1,1))
                    (not Eq(2,2,1))
                    ...
                    (not Eq(2,8,1))
           )
           (implies Eq(2,0,2)
                    (not Eq(2,1,2))
                    ...
           )
           ...
           (implies Eq(2,0,9)
                    (not Eq(2,1,9))
                    ...
           )
      )
     )
)
```

Notice that if `cell(r,c) == 0` the expression is always true, which means there
is no need to make `cell(r,c)` unique.

Given the uniqueness of every cell we can interpret the constraints for all
cells that appear in the same partition, simply making a conjunction of their uniqueness:

``` lisp
(let (RowCons(2)
      (and
       Uni(2,0)
       Uni(2,1)
       Uni(2,2)
       ...
       Uni(2,8)
      )
     )
)
```

Thus, the consistency of the entire board can be represented as:

``` lisp
(and RowCons(0)
     RowCons(1)
     ...
     RowCons(8)
     ColCons(0)
     ColCons(1)
     ...
     ColCons(8)
     BlkCons(0)
     BlkCons(1)
     ...
     BlkCons(8)
)
```

### Initial Assumptions

A board constrained only by the consistency is an unbounded Sudoku game, we still need
an initial layout to start with. Since we want to check if the initial layout is
valid itself, we can make it as an assumption, and let Z3 to
check the satisfiability. If the assumption is satisfiable, then the initial
layout is valid, otherwise it has inconsistent cells. 

In order to encode the initial layout as assumption, we assume that every cell
endowed with an initial value is consistent with each other. Therefore, 
our assumptions are in the form of `x(r,c)`, meaning that cell(r,c) is
consistent with other cells. From the initial layout, we know that
`Eq(r,c,cell(r,c))`, thus `x(r,c) -> Eq(r,c,cell(r,c))`:

``` lisp
(let (assumptions
      (and (implies i(0,0)
                    Eq(0,0,cell(0,0))
           )
           (implies i(0,1)
                    Eq(0,1,cell(0,1))
           )
           ...
           (implies i(8,8)
                    Eq(8,8,cell(8,8))
           )
      )
     )
)
```

We change the constant name from `x` to `i` to represent initial layout. We use
Z3 solver to check all the assumptions and the unsatisfiable cores are the inconsistent cells.

### Sudoku Solving
If the initial layout is consistent, we may now start to solve for a solution, i.e.
a possible model for the given system. Remember the initial layout is only an
assumption in previous step, so we add transform them into assertion, thus the
initial assumptions become the initial constraints.

``` lisp
(and i(0,0)
     i(0,1)
     i(0,2)
     ...
     i(8,8)
)
```

The final (solution) layout should obey the following constraints:

1. if `cell(r,c)` is not `0`, `f(r,c,b)` should be equal to `i(r,c,b)`;

2. if `cell(r,c)` is `0`, `Eq_f(r,c,d)` should be true for some digit `d` other than `0`.

These can be represented as:

``` lisp
(and (implies Eq_i(r,c,0)
              (or
               Eq_f(r,c,1)
               Eq_f(r,c,2)
               ...
               Eq_f(r,c,9)
              )
     )
     (implies Eq_i(r,c,1)
              Eq_f(r,c,1)
     )
     (implies Eq_i(r,c,2)
              Eq_f(r,c,2)
     )
     ...
     (implies Eq_i(r,c,9)
              Eq_f(r,c,9)
     )
)
```

Now we can call the Z3 solver to check the satisfiability and produce a model if
it finds a solution.

## Usage

1. Build  
Requires [z3 library](https://github.com/Z3Prover/z3) to be installed.  
Requires compiler with C++17 support.  
Use [CMake](https://cmake.org/) to build.

2. Run  
It takes a file name as input, the file should consists of 9 lines, each line with nine digits separated by a comma. If a cell is set to `0`, it is considered to be empty.
`./Z3-Sudoku sudoku1.csv`

3. Results  
    * If the input is inconsistent, it will return pairs of conflicting cells;
    * If the input is solvable, it will return a solution;
    * If the input is not solvable, it will return not solvable.
