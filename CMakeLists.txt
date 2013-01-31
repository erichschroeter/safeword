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

install(FILES "${PROJECT_SOURCE_DIR}/doc/bash/safeword.bash_completion"
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
