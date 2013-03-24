set(SRCS
Command.c
InitCommand.c
HelpCommand.c
AddCommand.c
RemoveCommand.c
ListCommand.c
TagCommand.c
CopyCommand.c
)
add_library(commands ${SRCS})
