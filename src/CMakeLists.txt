add_subdirectory(commands)

set(SRCS
main.c
safeword.c
)

# put the executable in the root directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
add_executable(safeword ${SRCS})
target_link_libraries(safeword
commands
${SQLITE3_LIBRARIES}
)

install(TARGETS safeword RUNTIME DESTINATION bin)
