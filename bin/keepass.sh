#!/bin/bash
#
#

# exit immediately after fail
set -e

# Check for dependencies so we can parse the exported Keepass 1.x CSV
hash csvtool 2>/dev/null ||
hash dos2unix 2>/dev/null ||
{ echo "
This script depends on the following commands:

  dos2unix
  csvtool

Please install via your package manager.
"; exit 1;
}

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
	local CSV i count username password desc note

	CSV=$1
	[ -z "$CSV" ] && { echo "no CSV file specified"; exit 1; }

	dos2unix $CSV

	i=1
	count=`csvtool height $CSV`
	echo "importing $count credentials"
	while [ $i -lt $count ]; do
		username=`csvtool drop $i $CSV | csvtool take 1 - | csvtool col 2 -`
		password=`csvtool drop $i $CSV | csvtool take 1 - | csvtool col 3 -`
		desc=`csvtool drop $i $CSV | csvtool take 1 - | csvtool col 1 -`
		# Uncomment below to replace all newlines with '\n'.
		# NOTE: You need to comment the line after so you don't add duplicates.
		#note=`csvtool drop $i $CSV | csvtool take 1 - | csvtool col 5 - | sed ':a;N;$!ba;s/\n/\\\\n/g'`
		note=`csvtool drop $i $CSV | csvtool take 1 - | csvtool col 5 -`
		#echo -e "safeword -m '$desc' -n '$note' '$username' '$password'"
		safeword add -m "$desc" -n "$note" "$username" "$password"

		let i=i+1
	done
}

case $1 in
csv)
	shift
	convert_csv $1
	;;
*)
	usage
	;;
esac
