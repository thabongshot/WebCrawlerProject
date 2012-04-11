/*
 *    SGREP
 *
 *    Copyright 1993, 1998 Sean Barrett
 *
 *    File: r2nfa.c
 *          Regular Expression -> Nondeterministic Finite Automaton compiler
 *
 *  regular expresion language we handle:
 *      ab       concatenation
 *      a|b      alternation
 *      a?       omission       (a|)
 *      (a)      grouping
 *      a+       1 or more repititions
 *      a*       0 or more repititions   (a+?)
 *      .        match anything but newline
 *      ^        beginning of line  (actually matches \n anywhere in pattern)
 *      $        end of line (actually matches \n anywhere in pattern)
 *   [charset]   arbitrary set of characters, e.g.
 *                  [aeiou]   any vowel
 *                  [a-z]     any character from a-z
 *                  [^a-z]    any character other than a-z
 *                  [-z]      '-' or 'z'
 *                  []z]      ']' or 'z'
 *                  [z^]      '^' or 'z'
 *                  [0-]      '0' or '-'
 *                  [0-\]]    any from '0' to '-'  (or write  []-0])
 *      \BLAH    BLAH where BLAH is any character (e.g. +, *, ?, $, \)
 *               This works inside [] as well.
 *               I don't support \008 types of things
 *
 *  This code uses a totally abstract interface to the NFA.
 *  I believe this is a relatively efficient construction, i.e.
 *  it avoids unnecessary nodes and epsilon transitions pretty
 *  effectively.
 */

#include <stdio.h>
#include <stdlib.h>
#include "nfa.h"

#define START_HAS_IN        1
#define STOP_HAS_OUT        2

nfa_state final_state;
static char *parse_expr(char *exp, nfa_state start, nfa_state *end, int *flag);

// external entry point
nfa_state compile_regexp(char *exp, int k)
{
   int temp;
   nfa_state start, end;
   reset_nfa();
   start = create_state();

   parse_expr(exp, start, &end, &temp);

   // make it a scanning automaton
   create_transition(start, ALL, 0, start);
   make_accept(end);

   final_state = end;
   start_state = start; 

   // accept up to k errors
   error_augment(k);

   // pack it up for further use
   compile_nfa();

   return start;
}

void r2nfaError(char *s)
{
    fprintf(stderr, "sgrep: %s.\n", s);
    exit(2);
}

/*  Grammar:
    leaf   : whatever
           : ( alt )
    rept   : leaf +
           : leaf *
           : leaf ?
           : leaf
    concat : rept concat
           : rept
    alt    : concat
           : concat | alt
    expr   : alt

*/

static char *parse_alt(char *exp, nfa_state start, nfa_state *end, int *flag);

static char *parse_ranges(char *exp, nfa_state start, nfa_state *end)
{
   int inv, c1, c2;

   charset_start();

   if (*++exp == '^') { inv = 1; ++exp; } else inv = 0;
   if (*exp == ']') { charset_add_range(']', ']'); ++exp; }

   while (*exp != ']') {
      if (*exp == 0) r2nfaError("Missing ']'");
      if (*exp == '\\' && !*++exp) r2nfaError("Bad '\\'.");
      c1 = *exp++;
      if (exp[0] == '-' && exp[1] != ']') {
         ++exp;
         if (*exp == '\\' && !*++exp) r2nfaError("Bad '\\'.");
         c2 = *exp++;
         if (c2 >= c1)
            charset_add_range(c1, c2);
         else
            charset_add_range(c2, c1); // z-a means a-z, not wraparound?
      } else
         charset_add_range(c1, c1);
   }

   c1 = charset_end();                /* get the handle to this charset */
   
   *end = create_state();

   create_transition(start, inv ? INV_CHARSET : CHARSET, c1, *end);
   return exp + 1;
}

static char *parse_leaf(char *exp, nfa_state start, nfa_state *end, int *flag)
{
   *flag = 0;

   switch (exp[0]) {
      case 0:
      case ')':
         *end = start;
         *flag = START_HAS_IN | STOP_HAS_OUT;
              // it's as if we epsilon back from stop to start
         return exp;

      case '(':
         exp = parse_alt(exp+1, start, end, flag);
         if (exp[0] != ')') r2nfaError("Missing ')'");
         break;

      case '[':
         return parse_ranges(exp, start, end);

      case '.':
         *end   = create_state();
         create_transition(start, BUT, '\n', *end);
         break;

      case '\\':
         if (!*++exp) r2nfaError("Bad '\\'.");
         goto onechar;

      case '^': 
      case '$':
         exp[0] = '\n'; 
         // Fall through with \n as our character
   
      default:
      onechar:
         *end   = create_state();
         create_transition(start, ONE, *exp, *end);
         break;
   }
   return exp+1;
}

/*
 *  Our unary operators:
 *    ? * +
 *
 *  ? and ?  --> ?
 *  + and +  --> +
 *  + and ?  --> *
 *  ? and +  --> *
 *  * and anything --> *
 */

static char *parse_rept(char *exp, nfa_state start, nfa_state *end, int *flag)
{
   int rptplus=0, rptstar=0, query = 0;
   exp = parse_leaf(exp, start, end, flag);
  
doit:
   switch(exp[0]) {
      case '+': rptplus = 1; ++exp; goto doit;
      case '*': rptstar = 1; ++exp; goto doit;
      case '?': query   = 1; ++exp; goto doit;
   }

   if (rptplus || rptstar || query) {
      if (rptstar) { query = 1; rptplus = 1; }        /* reencode * as +? */
      if (rptplus) {
         create_transition(*end, EPSILON, 0, start);
         *flag |= START_HAS_IN | STOP_HAS_OUT;
      }
      if (query)
         create_transition(start, EPSILON, 0, *end);
   }
   return exp;
}

static char *parse_concat(char *exp, nfa_state start, nfa_state *end, int *flag)
{
   nfa_state s1,e1, s2,e2;
   int f1,f2;
   s1 = start;
   exp = parse_rept(exp, s1, &e1, &f1);

   // make sure the last item concatenated has
   // a clean start point that we can muck with
   // to if we later get a * or + or ?
   while (exp[0] && exp[0] != '|' && exp[0] != ')') {
      if (f1 & STOP_HAS_OUT) {
         s2 = create_state();
         create_transition(e1, EPSILON, 0, s2);
         e1 = s2;
         f1 &= ~STOP_HAS_OUT;
      }
      exp = parse_rept(exp, e1, &e2, &f2);
      f1 |= f2 & STOP_HAS_OUT;
      e1 = e2;
   }
   *end   = e1;
   *flag  = f1;
   return exp;
}
    
static char *parse_alt(char *exp, nfa_state start, nfa_state *end, int *flags)
{
   nfa_state s1=start,e1, s2,e2, temp;
   int f1,f2;
   s1 = create_state();
   create_transition(start, EPSILON, 0, s1);
   exp = parse_concat(exp, s1, &e1, &f1);
   while (exp[0] == '|') {
      s2 = create_state();
      exp = parse_concat(exp+1, s2, &e2, &f2);

      create_transition(start, EPSILON, 0, s2);

      // use one of the existing ends as our end if we can...
      // this may waste one of them

      if (!(f1 & STOP_HAS_OUT)) {
         create_transition(e2, EPSILON, 0, e1);
      } else if (!(f2 & STOP_HAS_OUT)) {
         create_transition(e1, EPSILON, 0, e2);
         e1 = e2;
      } else {
         temp = create_state();
         create_transition(e1, EPSILON, 0, temp);
         create_transition(e2, EPSILON, 0, temp);
         e1 = temp;
      }
      f1 = 0;
   }
   *end   = e1;
   *flags = f1;
   return exp;
}
    
static char *parse_expr(char *exp, nfa_state start, nfa_state *end, int *flags)
{
   exp = parse_alt(exp, start, end, flags);
   if (*exp == ')') r2nfaError("Too many ')'s.");
   return exp;
}  
