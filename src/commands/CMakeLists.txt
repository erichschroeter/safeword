set(SRCS
Command.c
InitCommand.c
HelpCommand.c
AddCommand.c
ListCommand.c
)
add_library(commands ${SRCS})
