#ifndef COMMAND_TAG_H
#define COMMAND_TAG_H

#include "Command.h"

char* tagCmd_help(void);
int tagCmd_parse(int arc, char** argv);
int tagCmd_execute(void);

#endif
