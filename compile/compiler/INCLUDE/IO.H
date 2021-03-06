/*@A (C) 1992 Allen I. Holub                                                */
/* This file lets us use an ANSI #include <io.h> in a UNIX program */
#ifdef MSDOS

This message deliberately generates an error under MS-DOS. You should use the
io.h that comes with your compiler, not the current file. You will get
the current version of the file only if you compiler searches the /include
directory for the compiler sources BEFORE it searches the default include
directory.  If this is the case, either change the search order in your
INCLUDE environment so that the compiler will get the real io.h or delete
the current file.

#else
#include <sys/file.h>
#endif
