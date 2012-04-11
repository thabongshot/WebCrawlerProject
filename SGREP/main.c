/*
 *    SGREP
 *
 *    Copyright 1993, Sean Barrett
 *
 *    File: main.c
 *          simple commandline user interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define USE_FD


#ifdef USE_FD
#ifdef __WATCOMC__
#include <io.h>
#endif
#include <fcntl.h>
#endif

#include "nfa.h"

// from r2nfa.c:
extern nfa_state compile_regexp(char *exp, int error);

// from sgrep.c:
extern int use_quickshift, use_dfa3_cache;
extern unsigned char *run_reverse_scanner(unsigned char *, int);
extern void init_reverse(void);

#define        LINE_LIMIT        256 

// if the buffer size is too large, we end up
// with more cache hosage--if we instead keep
// loading up from disk into the same memory
// buffer, we cache better... so don't let this
// get to big... 16K seems decent
int BUFFER = (4096 * 4 + LINE_LIMIT);

nfa_state start_state;

int match = 0;

void printLine(char *p)
{
   match = 1;
   while (*--p != '\n');
   ++p;
   do
      putchar(*p);
   while (*p++ != '\n');
}

int len;
int cc;

#ifdef USE_FD
// We use non-buffering IO for speed, to avoid
// redundant copies--because this thing is fast
// enough that extra memory usage shows up
   #define file int
#else
   #define file FILE *
#endif

void searchFile(file f, unsigned char *p)
{
   int num, max, offset;

   p[LINE_LIMIT  ] = '\n';
   p[LINE_LIMIT-1] = '\n';
   p[LINE_LIMIT-2] = '\n';
   p[LINE_LIMIT-3] = '\n';

   // we start with a \n in the file so that
   // ^ will match at start

   offset = 1;

   for(;;) {
      // number of unread chars starting at LINE_LIMIT is offset

#ifdef USE_FD
      num = read(f, p + LINE_LIMIT + offset, BUFFER-LINE_LIMIT-offset);
#else
      num = fread(p + LINE_LIMIT + offset, 1, BUFFER-LINE_LIMIT-offset, f);
#endif
      num += LINE_LIMIT + offset;
      if (num == LINE_LIMIT)
         break;

      // number of unread chars starting at LINE_LIMIT is num-LINE_LIMIT

      if (num < BUFFER && p[num-1] != '\n') { p[num++] = '\n'; }
      max = num < BUFFER ? num : BUFFER - LINE_LIMIT;

      // if last buffer, read it all; otherwise, don't read
      // the last LINE_LIMIT of chars... max is index of last such char

      p[0] = p[1] = p[2] = '\n';
      p[num] = p[num+1] = p[num+2] = p[num+3] = '\n';

      max = run_reverse_scanner(p + LINE_LIMIT, max - LINE_LIMIT) - p;
      // now max has lower LINE_LIMIT counted in again

      if (num == LINE_LIMIT + offset)
         break;

      // copy the characters we haven't looked at yet,
      // plus the previous LINE_LIMIT characters (not for searching,
      //   just for display)

      memcpy(p, p + max - LINE_LIMIT, LINE_LIMIT);

      offset = num-max;    // amount of valid data still to go */
      memcpy(p + LINE_LIMIT, p + max, offset);
   }
}


void searchFilename(char *s, char *p)
{
   file fd;
#ifdef USE_FD
   fd = open(s, O_RDONLY );
	// | O_BINARY);
   if (fd >= 0) {
      searchFile(fd, p);
      close(fd);
   } else {
      perror(s);
   }
#else
   fd = fopen(s, "rb");
   if (fd != NULL) {
      searchFile(fd, p);
      fclose(fd);
   } else {
      perror(s);
   }
#endif
}

int main(int argc, char **argv)
{
   char *p;
   int i,done=0;
   int error=0;

   while (argc > 1 && argv[1][0] == '-' && !done) {
      switch (argv[1][1]) {
         case 'e': done = 1; break;   // this allows leading - in patterns
         case 'q': use_quickshift = 0; break;
         case 'c': use_dfa3_cache = 0; break;
         case 'B': BUFFER = atol(argv[1]+2);
                   if (BUFFER < 4 * LINE_LIMIT)
                      BUFFER = 4*LINE_LIMIT;
                   break;
         case 'X': error = atoi(argv[1]+2); break;
         case 'h': case '?':
            goto usage;
         default:
            fprintf(stderr, "sgrep: unrecognized flag '%c'\n", argv[1][1]);
            exit(2);
      }
      ++argv;
      --argc;
   }

   if (argc <= 1) {
     usage:
      fprintf(stderr, "usage: sgrep [options] regexp [filename]*\n"
             "Options:\n"
             "   -e          stop parsing options\n"
             "   -B #        set internal file buffer size to #\n"
             "   -X #        accept up to # errors (e.g. 2, not 1000)\n");
      return 0;
   }

   start_state = compile_regexp(argv[1],error);

   len = strlen(argv[1]);
   init_reverse();

   i = 2;
   p = malloc(BUFFER+16);

   if (!p) { perror("sgrep malloc"); exit(1); }
   if (argc > i) {
      for (; i < argc; ++i)
         searchFilename(argv[i],p);
   } else {
#ifdef USE_FD
      searchFile(0,p);
#else
      searchFile(stdin,p);
#endif
   }
   free(p);

   return !match;
}
