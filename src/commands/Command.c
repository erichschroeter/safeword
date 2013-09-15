#include "Command.h"
#include "InitCommand.h"
#include "HelpCommand.h"
#include "AddCommand.h"
#include "RemoveCommand.h"
#include "ListCommand.h"
#include "TagCommand.h"
#include "CopyCommand.h"
#include "ShowCommand.h"
#include "EditCommand.h"

struct command command_table[] = {
	{"help", helpCmd_run},
	{"init", initCmd_run},
	{"add", addCmd_run},
	{"rm", removeCmd_run},
	{"ls", listCmd_run},
	{"tag", tagCmd_run},
	{"cp", copyCmd_run},
	{"show", showCmd_run},
	{"edit", editCmd_run},
};
const size_t command_table_size = sizeof(command_table) / sizeof(command_table[0]);

