#ifndef COMMAND_INIT_H
#define COMMAND_INIT_H

#include "Command.h"

char* initCmd_help(void);
int initCmd_parse(int arc, char** argv);
int initCmd_execute(void);

#endif
