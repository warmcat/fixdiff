# fixdiff

Andy Green <andy@warmcat.com> 2025
See MIT license in LICENSE

```
$ cat llm-patch.diff | fixdiff | patch -p1
```

or with redirect

```
$ cat llm-patch.diff | fixdiff > llm-patch-fixed.diff
```

or optionally if run from elsewhere, you can tell it a path to change CWD to
on the commandline so it can find the sources mentioned in the patch.

```
$ cat llm-patch.diff | fixdiff /path/to/sources | patch -p1
```

`fixdiff` is designed to clean up diffs generated by LLMs (eg, Gemini 2.5).

LLM find it hard to generate diff headers with correct line counts or even
line offsets, although some LLMs are smart enough to produce otherwise
legible diffs.

This utility adjusts the diff stanzas sent to it on stdin and produces new stanza
headers with accurate line counts on stdout.

It covers:

 - wrong "before" line in original stanza header
 - wrong "before" line count in original stanza header
 - wrong "after" line in original stanza header
 - wrong "after" line count in original stanza header
 - removes extra lead-in context lines in stanza
 - for diffs adding to end of file, corrects mismatching context caused by
   LLM losing blank lines at the original EOF (by checking the original
   source file for extra lines and adding them to the stanza as context)

It finds and scans the sources the patches apply to and uses the diff stanza to
find the original line it applied to by itself, along with the original line
count and, considering earlier stanzas, the line in the modified file it appears
at and the new line count for the stanza.  Thus, it does not use the incoming
broken stanza header information at all and replaces all the @@ lines.

## Building

 - There are no dependencies other than libc.
 - It's pure C99.
 - It's valgrind-clean.
 - It just produces a small executable with no data files.
 - There are no switches.
 - It runs as part of a pipe into patch or standalone with redirects.

You can build it like:

```
$ mkdir build
$ cd build
$ cmake ..
$ make && sudo make install
```

