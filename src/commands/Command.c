#include "Command.h"
#include "InitCommand.h"
#include "HelpCommand.h"

struct command command_table[] = {
	{"init", initCmd_help, initCmd_parse, initCmd_execute},
	{"help", helpCmd_help, helpCmd_parse, helpCmd_execute},
};
const size_t command_table_size = sizeof(command_table) / sizeof(command_table[0]);

