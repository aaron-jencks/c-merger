# c-merger
Takes a C file and merges all of the local included files into a single compilable file. It is useful if you do competitive programming and have to be able to submit a single file.

The given source file was generated using the project itself.

# Installation
Build the file `merger.c` and then run it using the syntax:
```
merger in.c out.c
```
Where `in.c` is the input file to merge the includes of and `out.c` is the location of the output merged file.

# Limitations
## Linux exclusive
I don't use windows, so this script is linux exclusive. It makes use of `unistd.h` for the filesystem, if you know how to fix that, then it should be pretty simple to convert.

## File location limitations
This only works for merging C files, and only for projects where the sources are stored with the headers and where the sources have the same name as the headers.
For example:

This works
```
main.c
something/
  arr.h
  arr.c
```

This does not
```
main.c
headers/
  arr.h
source/
  arr.c
```

## Careful with File Structure
This file does not actually do any parsing of the files it finds, aside from finding `#include "` so your files need to be able to be pasted verbatim into the output file.

Some things that should be avoided:
1. `#pragma once`

Okay, that's it, happy merging!
