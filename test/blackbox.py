#!/usr/bin/python
#
# Much of this file is thanks to dropbox.py

import locale
import sys
import os
import json

enc = locale.getpreferredencoding()
safeword_db = os.path.join(os.getcwd(), "blackbox.safeword")

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

def safeword_db_exists():
	if not os.path.exists(safeword_db):
		if query_yes_no(u"create '" + safeword_db + "'?"):
			console_print(u"creating '" + safeword_db + "'")
			os.system("safeword init " + safeword_db)

	return os.path.exists(safeword_db)

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

def requires_safeword_db(method):
	def newmethod(*n, **kw):
		if safeword_db_exists():
			os.environ['SAFEWORD_DB'] = safeword_db
			return method(*n, **kw)
		else:
			console_print(u"Safeword database does not exist!")
	newmethod.func_name = method.func_name
	newmethod.__doc__ = method.__doc__
	return newmethod

@command
@requires_safeword_db
def populate(args):
	u"""populate safeword database with test data
blackbox populate [FILE]...

Populates the safeword database with test data contained within each FILE.
"""
	def tag(cred_id, tags_str):
		if tags_str != "":
			cmd = "safeword tag %s %s" % (cred_id, tags_str)
			os.system(cmd)

	creds = 0

	for file in args:
		print "populating with " + os.path.abspath(file)
		json_data = open(file)
		data = json.load(json_data)
		for cred in data:
			cmd = ""
			if "message" in cred and "username" in cred and "password" in cred:
				cmd = "safeword add -m \"%s\" %s %s" % (cred["message"], cred["username"], cred["password"])
			elif "username" in cred and "password" in cred:
				cmd = "safeword add %s %s" % (cred["username"], cred["password"])
			if cmd != "":
				os.system(cmd)
				creds += 1
			if "tags" in cred:
				import commands
				cmd = "safeword ls | grep \"%s\"" % cred["message"]
				status, output = commands.getstatusoutput(cmd)
				cred_id = output.partition(" ")
				tags_str = " ".join(cred["tags"])
				tag(cred_id[0], tags_str)
		json_data.close()
	print "populated '%s' with %d credentials" % (safeword_db, creds)

tests = {}

def test(method):
	global tests
	assert method.__doc__, "All tests need properly formatted docstrings (even %r!!)" % method
	if hasattr(method, 'im_func'): # bound method, if we ever have one
		method = method.im_func
	tests[method.func_name] = method
	return method

@command
@requires_safeword_db
def run(args):
	u"""run black box tests from specified files
blackbox test [FILE]...

Runs black box tests on the test database. Files specified should contain @test functions.
"""
	global tests

	tests_passed = 0
	tests_failed = 0

	for test in tests:
		result = tests[test]()
		if result == True:
			tests_passed += 1
		else:
			tests_failed += 1
	
	console_print(u"Test Results\n------------------")
	console_print(u"passed: %d\nfailed: %d" % (tests_passed, tests_failed))

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

@command
def help(argv):
	u"""provide help
blackbox help [COMMAND]

With no arguments, print a list of commands and a short description of each. With a comand, print descriptive help on how to use the command.
"""
	if not argv:
		return usage(argv)
	for command in commands:
		if command == argv[0]:
			console_print(commands[command].__doc__.split('\n', 1)[1].decode('ascii'))
			return
	for alias in aliases:
		if alias == argv[0]:
			console_print(aliases[alias].__doc__.split('\n', 1)[1].decode('ascii'))
			return
	console_print(u"unknown command '%s'" % argv[0], f=sys.stderr)

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
