cmake_minimum_required(VERSION 2.8.5)
if(WIN32)
set(CMAKE_C_COMPILER "mingw32-gcc-4.6.2.exe")
endif()
set(CMAKE_C_FLAGS "-Wall")

project(safeword)

# uncomment line below to enable debug messages
#add_definitions(-DDEBUG)

if(WIN32)
	add_definitions(-DWIN32)
	message("compiling for Windows")
else()
	message("compiling for Unix")
endif()

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 11)
set(VERSION_META "-beta")

# configure a header file to pass CMake settings to source code
configure_file(
	"${PROJECT_SOURCE_DIR}/src/main.h.in"
	"${PROJECT_BINARY_DIR}/src/main.h"
)

# add the binary tree to the search path for include files for safeword.h
include_directories("${PROJECT_BINARY_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(PROJECT_LIB_PATH "${CMAKE_SOURCE_DIR}/lib")

find_package(Sqlite3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIR})
set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})

if(NOT WIN32)
	find_package(X11 REQUIRED)
	include_directories(${X11_INCLUDE_DIR})
	set(LIBS ${LIBS} ${X11_LIBRARIES} ${X11_Xmu_LIB})
endif()

find_package(CUnit)
if(CUNIT_FOUND)
	message("CUnit found")
	include_directories(${CUNIT_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${CUNIT_LIBRARIES})
else()
	message("CUnit not found")
endif()

add_subdirectory(src)
add_subdirectory(test)

install(FILES "${PROJECT_SOURCE_DIR}/doc/bash/safeword.sh"
DESTINATION "/etc/bash_completion.d"
PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
RENAME safeword
)

# CPack Settings
set(CPACK_GENERATOR "DEB")

set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "Erich Schroeter erichschroeter@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION "cli password manager")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A cli password manager that puts usability as a priority.")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${VERSION_MAJOR}.${VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

# Debian specific settings
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Erich Schroeter")

include(CPack)
