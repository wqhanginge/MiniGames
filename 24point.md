# 24 Point Solver

**AppType: Console**

Solve a specified 24 point problem.

## Features

- Support **any integer** as the aim number.
- Support **non-negative integers that less than 127** as the input numbers.
- Support **[+, -, \*, /]** as operators.
- Support **4** different tasks.
- Using multi-threading by default.

## Usage

Solve a known 24 point problem.
```sh
24point --aim 24 --solve 5 6 7 8
```

Test if a list of numbers and operators can give a possible answer.
```sh
24point --aim 48 --test 4 3 6 8 + * -
```

Search for numbers within [2, 4] that can give a possible answer with a specified list of operators. Using 8 threads.
```sh
24point --aim 24 --search 2 4 + + * --job 8
```

Exhaust all possible answers under 4 digits of numbers that within [2, 6]. Show only one answer for each set of numbers.
```sh
24point --aim 24 --exhaust 2 6 4 --snum
```

Print the help message.
```sh
24point --help
```

## Notes

- C++14 is required.
