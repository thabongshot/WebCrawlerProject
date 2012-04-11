SGREP is Copyright 1998 Sean Barrett

ADMIN:

 Redistribution and commercial use limited.
 For more info contact buzzard@world.std.com.

 The current description of SPI, SGREP's
 search algorithm, is at
   http://world.std.com/~buzzard/sgrep.html

USAGE:

 The regular expression syntax for SGREP
 is described at the top of r2nfa.c

SOURCE INFO:

 nfa.c:     implementation of the core searching nfa
            (and the reverse one); also implements
            searching-with-mismatch augmentation

 r2nfa.c:   convert regular expressions to automata

 main.c:    UI and file-I/O driver

 sgrep.c:   implementation of SPI
