#include "Command.h"
#include "InitCommand.h"
#include "HelpCommand.h"
#include "AddCommand.h"
#include "RemoveCommand.h"
#include "ListCommand.h"
#include "TagCommand.h"
#include "CopyCommand.h"
#include "InfoCommand.h"

struct command command_table[] = {
	{"init", initCmd_help, initCmd_parse, initCmd_execute},
	{"help", helpCmd_help, helpCmd_parse, helpCmd_execute},
	{"add", addCmd_help, addCmd_parse, addCmd_execute},
	{"rm", removeCmd_help, removeCmd_parse, removeCmd_execute},
	{"ls", listCmd_help, listCmd_parse, listCmd_execute},
	{"tag", tagCmd_help, tagCmd_parse, tagCmd_execute},
	{"cp", copyCmd_help, copyCmd_parse, copyCmd_execute},
	{"info", infoCmd_help, infoCmd_parse, infoCmd_execute},
};
const size_t command_table_size = sizeof(command_table) / sizeof(command_table[0]);

