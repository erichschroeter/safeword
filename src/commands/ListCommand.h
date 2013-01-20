#ifndef COMMAND_LIST_H
#define COMMAND_LIST_H

#include "Command.h"

char* listCmd_help(void);
int listCmd_parse(int arc, char** argv);
int listCmd_execute(void);

#endif
