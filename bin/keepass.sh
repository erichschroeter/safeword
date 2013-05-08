#!/bin/bash
#
#

usage()
{
	echo "USAGE: $0 [INPUT]

DESCRIPTION:
	Converts from Keepass formats to a format compatible to be imported to safeword.

OPTIONS:
	INPUT
	    csv - Keepass 1.x CSV
"
}

convert_csv()
{
	IFS=","
	count=`cat $1 | cut -d, -f1-4 | sed '/^$/d' | sed '/^"/!d' | sed '/^["]\s$/d' | wc -l`
	count=$((count-1))
	i=0

	# 1. cut the comments field
	# 2. filter out empty lines
	# 3. filter out lines that don't start with quote
	# 4. filter out lines with single quote and any remaining whitespace
	cat $1 | cut -d, -f1-4 | sed '/^$/d' | sed '/^"/!d' | sed '/^["]\s$/d' | while read title username password url
	do
		echo "{"
		echo -e "\t\"message\": $title,"
		echo -e "\t\"username\": $username,"
		echo -e "\t\"password\": $password,"
		echo -e "\t\"tags\": ["
		echo -e "\t\t\"\""
		echo -e "\t]"
		# don't print the comma on the last one
		[ $i -eq $count ] && echo "}" || echo "},"
		i=$((i+1))
	done
}

case $1 in
csv)
	[ $# -gt 1 ] && [ -f $2 ] || exit 1
	convert_csv $2
	;;
*)
	usage
	;;
esac
