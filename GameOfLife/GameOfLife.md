# Game of Life

**AppType: Win32**

A simple simulator for Conway's *Game of Life*.

## Features

- Customizable map size.
- Load map content directly from a file.
- Additional map mode: **Limited Infinity**.
- Adjustable game speed.
- Modify map content instantly using **insert mode**.

## Build Notes

- **C++14** standard is required for compiling.
- Ensure the `SUBSYSTEM` is set to `WINDOWS`.
- **Unicode** version is available by defining the `UNICODE` and `_UNICODE` macros.

## Run

Run the binary directly to start with default settings, or drag an **init file** onto the executable to load custom settings.

### Init File

A text file that defines the custom settings and initial map contents, structured as,
```
<MapWidth> <MapHeight> <MapScale> <DataType> [<Data>]
```

A valid init file can have four distinct data types: **Ratio**, **Bitmap**, **Coord**, and **Rect**.
The following outlines the specific format and explaination for each type.
```
 Ratio: 0 [<ratio>]                     # Proportion of alive cells between 0.0 and 1.0.
Bitmap: 1 [0 1 1 0 ...]                 # 1 for alive and 0 for dead
 Coord: 2 [<x y> <x y> ...]             # Positions of alive cells
  Rect: 3 [<left top right bottom> ...] # Regions of alive cells
```

### Command-Line Option

Additionally, the program supports a command-line option for quick random initialization.
```sh
gameoflife -r <MapWidth> <MapHeight> <MapScale> <Ratio>
```

## Runtime Operations

The simulation process can be controlled using the following keyboard and mouse operations.
Below is a list of available actions along with their explanations.

| Key | Scope | Operation |
| :-: | :---: | :-------- |
| ESC | global | exit |
| w | global | zoom in |
| s | global | zoom out |
| a | global | slow down |
| d | global | speed up |
| h | global | show the help message |
| p | global | pause |
| i | global | insert mode |
| n | manual speed | perform the next step |
| e | paused | toggle map mode |
| BS | insert mode | clear the entire map |
| LMB | insert mode | spawn the specified cell |
| RMB | insert mode | kill the specified cell |

### Insert Mode

**Insert mode** allows you to freely modify the map content.
You can toggle the alive state of **any cell** using the two mouse buttons, or erase the **entire map** by pressing the `Backspace` key.

Take care of your map and enjoy!

## License

Copyright (C) 2023 Gee Wang\
Licensed under the [GNU GPL v3](../LICENSE).
