# 24 Point Solver

**AppType: Console**

Solve a specified 24 point problem.

## Features

- Support **any integer** as the target number.
- Support **non-negative integers that less than 127** as the input numbers.
- Support **[+, -, \*, /]** as operators.
- Support an **exhaustion mode** for unfixed input numbers.
- Using multi-threading by default.

## Usage

### Syntax

```sh
24point [-d] [-j <n>] [-o <file>] [-p | -P] [-r <min> <max>] <target> <num>[:...] [--op=<op>[...]]

Positional arguments:
  target                expected result value of expressions
  num                   non-negative integers as input numbers

Optional arguments:
  -d, --display-blanks  display number lists that have no solutions
  -j, --jobs <n>        number of working threads, default to max available threads
  -o, --out <file>      output solutions into a file
  -p, --prune           search only one solution for each different list of numbers
  -P, --no-prune        search all possible solutions
  -r, --range <min> <max>
                        enable exhaustion mode to search solutions from ranged input
                        numbers, this interpret the first input number as the size of
                        each number list and ignore excess input numbers
  --op=<op>[...]        extra operators to be used in expressions, excess operators
                        are ignored
```

### Examples

Solve a 24 point problem for fixed input numbers.
```sh
24point 24 5:6:7:6
```

Solve a 32 point problem for fixed input numbers with specified operators.
```sh
24point 32 4:3:6:8:4 --op=++*
```

Solve a 48 point problem for all combinations of 3 input numbers from range [1, 9], and search for only one suitable expression for each combination.
```sh
24point -p 48 3 -r 1 9
```

## Notes

- C++14 is required for compiling.
