# Building

## Unix

1. `sudo apt-get install build-essential make cmake libqlite3-dev`
1. `git clone git://github.com/erichschroeter/safeword.git`
1. `cd safeword && mkdir build && cd build && cmake ..`
1. `make`

## Windows

1. this method assumes you have MinGW installed (see the [HOWTO][0])
1. download sqlite3 [headers][1] and [library][2]
1. put `sqlite3.h` and `sqlite3ext.h` in `C:\MinGW\include`
1. put `sqlite3.dll` and `sqlite3.def` in one of the locations below depending on your system
 1. *32-bit* -- `C:\Windows\System32`
 1. *64-bit* -- `C:\Windows\SysWOW64`
1. `git clone git://github.com/erichschroeter/safeword.git`
1. `cd safeword && mkdir build && cd build && cmake -G "MinGW Makefiles" ..`
1. `mingw32-make`

[0]: http://www.mingw.org/wiki/InstallationHOWTOforMinGW
[1]: http://www.sqlite.org/2013/sqlite-amalgamation-3071601.zip
[2]: http://www.sqlite.org/2013/sqlite-dll-win32-x86-3071601.zip
