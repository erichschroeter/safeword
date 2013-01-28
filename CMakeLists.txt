cmake_minimum_required(VERSION 2.8.5)
project(safeword)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# configure a header file to pass CMake settings to source code
configure_file(
	"${PROJECT_SOURCE_DIR}/src/safeword.h.in"
	"${PROJECT_BINARY_DIR}/src/safeword.h"
)

# add the binary tree to the search path for include files for safeword.h
include_directories("${PROJECT_BINARY_DIR}/src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(PROJECT_LIB_PATH "${CMAKE_SOURCE_DIR}/lib")

find_package(Sqlite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})

add_subdirectory(src)
