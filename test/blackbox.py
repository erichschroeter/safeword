#!/usr/bin/python
#
# Much of this file is thanks to dropbox.py

import locale
import sys
import os
import json

enc = locale.getpreferredencoding()

def console_print(st=u"", f=sys.stdout, linebreak=True):
	global enc
	assert type(st) is unicode
	f.write(st.encode(enc))
	if linebreak: f.write(os.linesep)

def console_flush(f=sys.stdout):
	f.flush()

def query_yes_no(question, default="no"):
	u"""Ask a yes/no question via raw_input() and return their answer.

"question" is a string that is presented to the user.
"default" is the presumed anser if the user just hits <Enter>.
	It must be "yes" (the default), "no" or None (meaning an answer is required of the user).

The "answer" return value is one of "yes" or "no".
"""
	valid = {"yes":True, "y":True, "ye":True, "no":False, "n":False}
	if default == None:
		prompt = u" [y/n] "
	elif default == "yes":
		prompt = u" [Y/n] "
	elif default == "no":
		prompt = u" [y/N] "
	else:
		raise ValueError("invalid default answer: '%s'" % default)
	while True:
		console_print(question + prompt)
		choice = raw_input().lower()
		if default is not None and choice == '':
			return valid[default]
		elif choice in valid:
			return valid[choice]
		else:
			console_print(u"Please respond with 'yes' or 'no' (or 'y' or 'n').")

commands = {}
aliases = {}

def command(method):
	global commands, aliases
	assert method.__doc__, "All commands need properly formatted docstrings (even %r!!)" % method
	if hasattr(method, 'im_func'): # bound method, if we ever have one
		method = method.im_func
	commands[method.func_name] = method
	method_aliases = [unicode(alias) for alias in aliases.iterkeys() if aliases[alias].func_name == method.func_name]
	if method_aliases:
		method.__doc__ += u"\nAliases: %s" % ",".join(method_aliases)
	return method

@command
def populate(args):
	u"""populate safeword database with test data
blackbox populate [FILE]...

Populates the safeword database with test data contained within each FILE.
"""
	from pprint import pprint

	creds = 0

	for file in args:
		print "populating with " + os.path.abspath(file)
		json_data = open(file)
		data = json.load(json_data)
		for cred in data:
			creds += 1
			print "safeword add -m \"" + cred["message"] + "\" " + cred["username"] + " " + cred["password"]
		json_data.close()
		print "populated safeword database with " + str(creds) + " credentials"

def usage(argv):
	console_print(u"blackbox command-line interface\n")
	console_print(u"commands:\n")
	console_print(u"Note: use blackbox help <command> to view usage for a specific command.\n")
	out = []
	for command in commands:
		out.append((command, commands[command].__doc__.splitlines()[0]))
	spacing = max(len(o[0])+3 for o in out)
	for o in out:
		console_print(" %-*s%s" % (spacing, o[0], o[1]))
	console_print()

def main(argv):
	global commands

	# find out if one of the commands are in the
	# argv list, and if so split the list at that
	# point to separate the argv list
	cut = None
	for i in range(len(argv)):
		if argv[i] in commands:
			cut = i
			break

	if cut == None:
		usage(argv)
		os._exit(0)
		return

	# dispatch and run the command
	result = None
	if argv[i] in commands:
		result = commands[argv[i]](argv[i+1:])
	elif argv[i] in aliases:
		result = aliases[argv[i]](argv[i+1:])

	console_flush()

	return result

if __name__ == "__main__":
	ret = main(sys.argv)
	if ret is not None:
		sys.exit(ret)
