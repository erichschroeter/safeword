cmake_minimum_required(VERSION 2.8.5)
project(safeword)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(PROJECT_LIB_PATH "${CMAKE_SOURCE_DIR}/lib")

add_subdirectory(src)
