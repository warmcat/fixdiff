#!/bin/bash

rm -f .concat.tmp

while [ ! -z "$1" ] ; do

	if [ ! -e "$1" ] ; then
		echo "ABORTING: unable to find $1 - is cwd correct?"
		exit 1
	fi

	echo >> .concat.tmp
	echo >> .concat.tmp
	echo "$1:" >> .concat.tmp
	echo >> .concat.tmp
	echo '```' >> .concat.tmp
	cat $1 >> .concat.tmp
	echo '```' >> .concat.tmp

	shift 1
done

which xsel
if [ $? -ne 0 ] ; then
	echo "xsel not available, dumping to stdout =============================================="
	cat .concat.tmp
	exit 0
fi
cat .concat.tmp | xsel

echo "Concatenated file contents in clipboard"

exit 0
