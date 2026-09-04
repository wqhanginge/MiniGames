# Bulls and Cows

**AppType: Console**

A simple implementation of classic *Bulls and Cows* game.

## Features

- Adopt the form of **4** digits with **10** numbers from **[0~9]**.
- Up to **10** chances in one game.
- Both **Standard** and **Mastermind** modes have been fully implemented.

## Build Notes

- **C++17** standard is required for compiling.

## Run

Run the binary directly without any arguments. The default mode is **Standard**.

Pressing `Ctrl-Z + Enter` (Windows) or `Ctrl-D + Enter` (Linux/macOS) at any position will exit the program immediately. If the program behaves abnormally, use `Ctrl-C` to abort it.

## Samples

A game in **Standard** mode:
```sh
============================================================
                  Bulls and Cows: Standard
============================================================
Guess> 1234
                                        1.      1234    0A0B
Guess> 5678
                                        2.      5678    0A3B
Guess> 6789
                                        3.      6789    0A2B
Guess> 7805
                                        4.      7805    4A0B

Congratulations! You got the Answer: 7805
A total of 4 chances were consumed.
============================================================
```

A game in **Mastermind** mode:
```sh
============================================================
                 Bulls and Cows: Mastermind
============================================================
Guess> 1234
                                        1.      1234    0A0B
Guess> 5678
                                        2.      5678    1A1B
Guess> 5690
                                        3.      5690    2A0B
Guess> 5799
                                        4.      5799    2A2B
Guess> 5997
                                        5.      5997    4A0B

Congratulations! You got the Answer: 5997
A total of 5 chances were consumed.
============================================================
```

## License

Copyright (C) 2024 Gee Wang\
Licensed under the [GPLv3](../LICENSE).
