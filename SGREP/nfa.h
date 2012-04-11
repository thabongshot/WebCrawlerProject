/*
 *  nfa.h
 *
 *  definitions for interface to nfa.c
 */

#define MAX_NODES  4096 // max nodes that can occur in our basic graph
#define NUM_CHARS   257 // number of possible input chars (including OMEGA)
#define ECHAR        -1 // number representing epsilon transition
#define OMEGA       256 // number representing char that matches everything

typedef short nfa_state;
typedef int   boolean;
#define TRUE  1
#define FALSE 0

// types of NFA transitions
typedef enum
{
  EPSILON, ONE, BUT, ALL, CHARSET, INV_CHARSET, ALL_AND_EPSILON
} TRtype;

extern void reset_nfa(void);            // start building the nfa */
extern void compile_nfa(void);          // finish building the nfa */

extern nfa_state create_state(void);    // add a new state to the nfa
extern void create_transition(nfa_state, TRtype, int, nfa_state);
extern void make_accept(nfa_state);     // mark this as a goal state

extern void reset_transition(void);     // begin computing an NFA transition

// the follow is our extremely inefficient abstraction
// for operating on the NFA--sequential test each outgoing state

extern boolean does_accept(nfa_state);
extern int num_transitions(nfa_state f); // how many outgoing transitions?
extern nfa_state transition_to(nfa_state f, int trans, int c);
 // what state does this lead to on input c; return 0 if no state */

extern int min_match_char, max_match_char;
 // range of characters last NFA transition was valid for

extern int rev_num_transitions(nfa_state f); // how many transitions to this 
extern nfa_state rev_transition_to(nfa_state f, int trans, int c);

// abstraction for building efficient charset tables
// [ideally, this would detect duplicate charsets, but at
//  least this way we can internally reuse them, e.g. in
//  the cloned automatons for matching-with-errors]

extern void charset_start(void);         // begin building a charset
extern void charset_add_range(int, int); // add a range of characters
extern int  charset_end(void);           // return an id for this charset

extern int mapsize;      // number of ints required to store full bit array
extern int num_nodes;    // number of actual nodes in the graph, plus one

extern nfa_state start_state, final_state;

extern int edist(nfa_state f);
extern int sdist(nfa_state f);

extern void error_augment(int error_count);
