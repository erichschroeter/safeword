#ifndef COMMAND_REMOVE_H
#define COMMAND_REMOVE_H

#include "Command.h"

char* removeCmd_help(void);
int removeCmd_parse(int arc, char** argv);
int removeCmd_execute(void);

#endif
