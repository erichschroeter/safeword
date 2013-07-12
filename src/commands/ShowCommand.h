#ifndef COMMAND_SHOW_H
#define COMMAND_SHOW_H

#include "Command.h"

char* showCmd_help(void);
int showCmd_parse(int arc, char** argv);
int showCmd_execute(void);

#endif
