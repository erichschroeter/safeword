#ifndef COMMAND_HELP_H
#define COMMAND_HELP_H

#include "Command.h"

char* helpCmd_help(void);
int helpCmd_parse(int arc, char** argv);
int helpCmd_execute(void);

#endif
