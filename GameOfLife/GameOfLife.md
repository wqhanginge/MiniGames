# Game of Life

**AppType: Win32**

A simple simulator for Conway's *Game of Life*.

## Features

- Support a custom game map size up to **65535x65535**.
- Support initializing the game map randomly or from a file.
- Support **2** types of map edges: **bounded** and **unbounded**.
- Support multiple running speeds.
- Support several different display sizes.
- Support **insert mode**, which is allowed to modify the game map contents after initializing.

## Run

Directly run the program with default settings or drag an **init file** onto the executable to run with custom settings. Additionally, this program supports following command lines for custom initializations.
```sh
gameoflife -r <MapWidth> <MapHeight> <AliveRatio> [<PixelWidth>]    //random initialization
gameoflife <FileName>   //file initialization
```

### Init File Format

```txt
<FileType> <MapWidth> <MapHeight> <PixelWidth> <Data>
```

There are **3** file types for a valid init file: **Bitmap**, **Coord** and **Rect**. Each type is mapped to a unique integer. See below,
```c++
#define GLFT_BITMAP 0
#define GLFT_COORD  1
#define GLFT_RECT   2
```

#### Bitmap

The Data part should indicate the state of each cell in game map, 1 for alive and 0 for dead.
```txt
0 <MapWidth> <MapHeight> <PixelWidth> 0 0 1 0 ... 1 0
```

An example is shown below,
```txt
0 10 20 8
0 1 1 1 0 0 0 1 0 1
...
1 0 0 1 1 0 1 0 1 0
```

#### Coord

The Data part is a list of coordinates that indicating which cell is alive.
```txt
1 <MapWidth> <MapHeight> <PixelWidth> [<x y> <x y> ...]
```

An example is shown below,
```txt
1 45 40 5
0 0  0 2
23 23
14 14  14 15  14 16
```

#### Rect

The Data part is a list of rectangles that indicating a region of cells are alive.
```txt
2 <MapWidth> <MapHeight> <PixelWidth> [<left top right bottom> ...]
```

An example is shown below,
```txt
2 300 300 4
10 10 20 20  134 136 224 226
25 156 46 178
```

## Operations

Following operations are supported by pressing the corresponding keys on keyboard or mouse.
```txt
ESC     close window and exit
w       zoom in
s       zoom out
d       speed up
a       slow down
n       perform the next step when in 'Manual' speed
h       show or hide the help message, default is showing
p       pause the game
e       toggle the map edge type when game is paused, default is bounded
i       enter or exit 'insert' mode when game is paused
BS      clear the whole map when in 'insert' mode
MLB     add an alive cell at where the mouse is at when in 'insert' mode
MRB     remove an alive cell at where the mouse is at when in 'insert' mode
```

### Edge Type

There are **2** types of game map edge: **bounded** and **unbounded**. It is possible to switch between those two types at any time after the simulation is started, although the switch can only be done when game is paused.

A **bounded** edge is a solid edge that no cell can go through it. Any logical cells that are located on and outside the **bounded** edge are regarded as dead.

An **unbounded** edge is a transparent edge that connects to the opposite side of the game map like a portal. Any logical cells can walk through the edge on one side on map and show up on the opposite side on map.

### Insert Mode

When entering the 'insert' mode, you can press the **left mouse button** and drag to **add** new alive cells to map, or press the **right mouse button** and drag to **remove** existing alive cells on map.

To remove all existing alive cells, press **BackSpace** on keyboard when in 'insert' mode. It is **NOT** able to undo this operation.

The 'insert' mode works only when game is paused. The simulator automatically exit the 'insert' mode if the game resumes from pause.

## Notes

- C++17 is required for compiling.
