# 24 Point Solver

**AppType: Console**

Solve a specified *24 Point* problem.

## Features

- Accept a custom **integer** as the target number.
- All input numbers must be **non-negative integers** smaller than **128**.
- Only the four **basic arithmetic operations** (+, -, \*, /) are valid.
- Finding solutions for varying input numbers is achievable through **exhaustion mode**.
- Basic multi-threading support.

## Build Notes

- C++11 standard is required for compiling.

## Usage

### Syntax

```sh
24point [-v] [-j <n>] [-o <file>] [-p <level>] [-r <min>:<max>] <target> <num>[:...] [--op=<op>[...]]

Positional arguments:
  target                expected result value of expressions
  num                   non-negative integers as input numbers

Optional arguments:
  -v, --verbose         display all results including those have no solutions
  -j, --jobs <n>        specify the max available working threads
  -o, --out <file>      output solutions into a file
  -p, --prune <level>   set the prune level for the solving process as <std|max|off>,
                        <std> prune expressions with the same operators (default),
                        <max> prune expressions with the same numbers and operators,
                        <off> do not prune expressions
  -r, --range <min>:<max>
                        enable exhaustion mode to search solutions from ranged input
                        numbers, this interpret the first input number as the size of
                        each number list and ignore excess input numbers
  --op=<op>[...]        extra operators to be used in expressions, excess operators
                        are ignored
```

### Examples

Solve a 24-point problem using fixed input numbers.
```sh
24point 24 5:6:7:6
```

Solve a 32-point problem using fixed input numbers with specified operators.
```sh
24point 32 4:3:6:8:4 --op=++*
```

Solve a 48-point problem by evaluating every possible combination of three input numbers, where each number is drawn from the range [1, 9], ensuring that only one valid expression is selected per combination.
```sh
24point -p max 48 3 -r 1:9
```

## License

Copyright (C) 2020 Gee Wang\
Licensed under the [GNU GPL v3](../LICENSE).
