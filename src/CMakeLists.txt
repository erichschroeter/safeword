set(SAFEWORD_SRCS
safeword.c
)
add_library(safeword ${SAFEWORD_SRCS})

set(COMMAND_SRCS
commands/Command.c
commands/InitCommand.c
commands/HelpCommand.c
commands/AddCommand.c
commands/RemoveCommand.c
commands/ListCommand.c
commands/TagCommand.c
commands/CopyCommand.c
commands/ShowCommand.c
commands/EditCommand.c
commands/ExportCommand.c
)
add_library(commands ${COMMAND_SRCS})
target_link_libraries(commands safeword)

set(SAFEWORD_CLI_SRCS
main.c
)

# put the executable in the project root directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
add_executable(safewordcli ${SAFEWORD_CLI_SRCS})
set_target_properties(safewordcli PROPERTIES OUTPUT_NAME safeword)
if(WIN32)
	set(LIBS safeword commands ${SQLITE3_LIBRARIES})
else()
	set(LIBS safeword commands ${SQLITE3_LIBRARIES} ${X11_LIBRARIES} ${X11_Xmu_LIB} rt)
endif()
target_link_libraries(safewordcli ${LIBS})

install(TARGETS safewordcli RUNTIME DESTINATION bin COMPONENT applications)
