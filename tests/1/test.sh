#!/bin/bash

cp client-parser-ws.c-orig client-parser-ws.c
cat gemini.patch | fixdiff | patch -p1
R=`sha256sum client-parser-ws.c`
if [ $R != "fec27b802dc46c2e26f5ccc9316683a780c0785dc46c95f7a9fe73314bb81f5d" ] ; then
	exit 1
fi
exit 0
