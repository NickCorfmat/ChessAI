 --------------------------------------------------------------
 Nicolas Corfmat
 CruzID: ncorfmat
 --------------------------------------------------------------

```
Chess/Tic-Tac-Toe vs AI
```

```
Running Console Application
```
+ Enter the source code directory through terminal (named `chessEmpty` by default)
+ Run `make` (If error arises, run `cmake` /path/to/CMakeLists.txt)
+ Run ./gameboard on command line (Opening .exe from folder will likely cause issues)

```
Running AI On CoreChess
```
+ define `UCI_INTERFACE` in `Application.h`
+ Run CoreChess
+ Tools > Engines > Add New Engine
+ Select UCI Engine type
+ Under "Command": path to `gameboard` executable
+ Under "Working dir": path to `build` folder found inside the AI source code
+ Add the new engine and start a new game

Note: If there's no `gameboard` executable in source code folder, run make to build it