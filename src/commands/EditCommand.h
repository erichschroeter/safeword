#ifndef COMMAND_EDIT_H
#define COMMAND_EDIT_H

#include "Command.h"

char* editCmd_help(void);
int editCmd_parse(int arc, char** argv);
int editCmd_execute(void);

#endif // COMMAND_EDIT_H
