#ifndef COMMAND_ADD_H
#define COMMAND_ADD_H

#include "Command.h"

char* addCmd_help(void);
int addCmd_parse(int arc, char** argv);
int addCmd_execute(void);

#endif
