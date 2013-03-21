#ifndef COMMAND_COPY_H
#define COMMAND_COPY_H

#include "Command.h"

char* copyCmd_help(void);
int copyCmd_parse(int arc, char** argv);
int copyCmd_execute(void);

#endif
