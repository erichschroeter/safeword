#include "Command.h"
#include "InitCommand.h"

struct command command_table[] = {
	{"init", initCmd_help, initCmd_parse, initCmd_execute},
};
const size_t command_table_size = sizeof(command_table) / sizeof(command_table[0]);

