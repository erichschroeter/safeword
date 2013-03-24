#ifndef COMMAND_INFO_H
#define COMMAND_INFO_H

#include "Command.h"

char* infoCmd_help(void);
int infoCmd_parse(int arc, char** argv);
int infoCmd_execute(void);

#endif
