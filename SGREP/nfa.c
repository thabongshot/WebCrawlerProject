
/*
 *    SGREP
 *
 *    Copyright 1993,1998 Sean Barrett
 *
 *    File: nfa.c
 *          
 *          A low-level nondeterministic finite automaton toolkit.
 *
 *          Also contains code to modify search patterns to match errors.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nfa.h"

#define MAX_EDGES     8192   // sufficient for 2K regexp
#define MAX_CHARSETS    64   // its silly to need more!

// edges in the scanning NFA

static struct edge {
    int       ch;          // transition character or charset
    nfa_state src;
    nfa_state dest;
    TRtype    type;
} edges[MAX_EDGES];

static struct st_node {
    int  number_edges;       // number of outgoing edges
    struct edge *edges;      // array of outgoing edges

    int  rev_number_edges;   // number of incoming edges
    struct edge *rev_edges;  // array of incoming edges

    boolean accept;          // is this an accepting/goal state?

    int  edist;              // Shortest forward distance to an accepting state
    int  sdist;              // Longest backward distance to start state
} node[MAX_NODES+1];

// reserve node # 0 as unused

int num_nodes = 1;                        /* Number of allocated nodes + 1 */

/*
 *  Compute maxLookahead (edist)
 *
 *  This is a specialized single-source shortest paths along
 *  the backwards graph, starting from all accepting states.
 *  The weight on each edge is either 0 (epsilon), 1 (normal edge),
 *  or infinity (no edge).  This means we can use a modified greedy
 *  breadth-first search algorithm.  The details of this are a
 *  bit complex.  Basically, we have to make two passes over our
 *  data, first doing all edges of length 0, then all 1s,
 *  alternating until done.
 *
 *  To save on storage, we use the node array to store our queue.
 *  (The queue can never get longer than there are nodes).  We use
 *  the 'sdist' field of the node array to be our "node number of
 *  current entry in the queue", since that field is unused at the
 *  time this is computed.
 */

#define queue(x)        (node[x].sdist)
#define INFINITY        32767      // NOT infinity!  What a hack

#define IS_EPSILON(x)   ((x) == EPSILON || (x) == ALL_AND_EPSILON)
#define UNREACHED(x)    (node[x].edist == INFINITY)

static void compute_edist(void)
{
   int i, j, k, n;
   int qhead,qtail;
   struct edge *z;

   qhead = qtail = 0;
   for (i=1; i < num_nodes; ++i) {
      node[i].edist = node[i].accept ? 0 : INFINITY;
      if (node[i].accept)
         queue(qtail++) = i;
   }

   while (qhead != qtail) {
      // process all the guys at this distance away, adding their
      // epsilon transitions
      for (j=qhead; j != qtail; j == num_nodes-1 ? j = 0 : ++j) {    
         n = queue(j);
         k = node[n].rev_number_edges;
         z = node[n].rev_edges;
         for (i=0; i < k; ++i) {
            if (IS_EPSILON(z[i].type) && UNREACHED(z[i].dest)) {
               node[z[i].dest].edist = node[n].edist;
               queue(qtail) = z[i].dest;
               if (++qtail >= num_nodes-1) qtail = 0;
            }
         }
      }

      // now process all the guys again, this time the non-epsilons

      // we want to freeze qtail where it is right now to
      // know who to visit, but we want to add new elements
      // as well.  When we're done, we'll advance qhead to
      // the frozen value, so we do so now, and use IT as
      // the frozen qtail

      j = qhead;
      qhead = qtail;

      for (; j != qhead; j == num_nodes-1 ? j = 0 : ++j) {
         n = queue(j);
         k = node[n].rev_number_edges;
         z = node[n].rev_edges;
         for (i=0; i < k; ++i) {
            if (!IS_EPSILON(z[i].type) && UNREACHED(z[i].dest)) {
               node[z[i].dest].edist = node[n].edist + 1;
               queue(qtail) = z[i].dest;
               if (++qtail >= num_nodes-1) qtail = 0;
            }
         }
      }

   // repeat 'til queue is empty
   }
}

/*
 *
 *  Compute quickShift (sdist)
 *
 *  We apply a Bellman-Ford single-source shortest distance
 *  algorithm to the graph.  This runs in time O(V * E), which
 *  is O(n^2) in the regexp.
 *
 */

// relax this node.  vaguely works even if either one is "Infinity"
static void relax(int x, int y, int z)
{
   // ignore the initial starting loop transition
   if (x == start_state && y == start_state) return;

   if (node[y].sdist > node[x].sdist + z)
      node[y].sdist = node[x].sdist + z;
}

// oops, we must be on a negative cycle
static void fixcycle(int x, int y, int z)
{
   // ignore the initial starting loop transition
   if (x == start_state && y == start_state) return;

   if (node[y].sdist > node[x].sdist + z)
      node[y].sdist = -INFINITY;
}

static void compute_sdist(void)
{
   int i,j,k;
   struct edge *e;

   for (i=1; i < num_nodes; ++i)
      node[i].sdist = INFINITY;

   node[start_state].sdist = 0;

   // progressively relax
   for (k=1; k < num_nodes; ++k) {
      // visit all edges and nodes
      for (i=1; i < num_nodes; ++i) {
         e = node[i].edges;
         for (j=0; j < node[i].number_edges; ++j)
            relax(i, e[j].dest, e[j].type == EPSILON ? 0 : -1);
      }
   }

   // progressively relax
   for (k=1; k < num_nodes; ++k) {
      // visit all edges (by going to each node, then all its edges)
      for (i=1; i < num_nodes; ++i) {
         e = node[i].edges;
         for (j=0; j < node[i].number_edges; ++j)
            fixcycle(i, e[j].dest, e[j].type == EPSILON ? 0 : -1);
      }
   }

   for (i=1; i < num_nodes; ++i)
      node[i].sdist = -node[i].sdist;
}

///////////////////////////////////////////////////////////////////

// character sets

static char charset_match[MAX_CHARSETS][NUM_CHARS];

static int fc = 0;     /* Number of allocated charsets */
static int ncs;

void charset_start(void) { ncs = 0; }

void charset_add_range(int a, int b)
{
   ncs += (b-a+1);
   while (a <= b)
      charset_match[fc][a++] = 1;
}

int charset_end(void)
{
   int acc;
   int i;
   if (ncs > 3)
     return fc++;

/* this is a very small charset.  Build a 32-bit descriptor
 * for it.  We just concatenate the three characters and then
 * set the hi bit.
 * This code assumes NUM_CHARS is at most 256.  Oops
 */

   acc = 1;
   for (i=0; i < NUM_CHARS; ++i) {
      if (charset_match[fc][i]) {
         charset_match[fc][i] = 0;
         acc = (acc << 8) | i;
      }
   }

   // check if we only had one or two characters, in which case
   // just duplicate 'em... trying to support 0 as a valid char, you know!
   while (!(acc & (1 << 24)))
      acc = (acc << 8) | (acc & 255);

   // set the high bit to flag it appropriately
   return acc | 0x80000000;
}

static int edge_count = 0;
int mapsize = 1;     // Number of 'ints' required to store full bitmap

void reset_nfa(void) { num_nodes = 1; edge_count = 0; mapsize = 1;}

nfa_state create_state(void)
{
    node[num_nodes].accept = 0;
    mapsize = (num_nodes+sizeof(int)*8-1) / (sizeof(int) * 8);
    return num_nodes++;
}

void     make_accept(nfa_state x)   { node[x].accept = 1; }
boolean  does_accept(nfa_state x)   { return node[x].accept; }
int  edist(nfa_state x)             { return node[x].edist; }
int  sdist(nfa_state x)             { return node[x].sdist; }

void create_transition(nfa_state s, TRtype t, int c, nfa_state e)
{
   edges[edge_count].src  = s;
   edges[edge_count].dest = e;

   edges[edge_count].type = t;
   edges[edge_count].ch   = c;

   if (++edge_count == MAX_EDGES) {
      fprintf(stderr, "sgrep: regexp too complex (edges).\n");
      exit(1);
   }
}

// "Compile" the computed graph.

// This just lets us use a memory efficient data
// structure without doing lots of reallocs on the fly

int edge_compare(const void *a, const void *b)
{
    return ((const struct edge *) a)->src
         - ((const struct edge *) b)->src;
}


void build_edge_tables(void)
{
    int i;
    struct edge *e = edges;

    // sort the edges so all the edges for one source are together
    qsort(edges, edge_count, sizeof(edges[0]), edge_compare);

    // scan the sorted list of edges and associate them
    // with each node in the graph
    for (i=1; i < num_nodes; ++i) {
        node[i].number_edges = 0;
        node[i].edges = e;
        while (e < edges + edge_count && e->src == i) {
            ++node[i].number_edges;
            ++e;
        }
    }

    // compute the reverse transition table; clone of the above,
    // but we swap start and end

    memcpy(edges + edge_count, edges, edge_count * sizeof(edges[0]));
    for (i=edge_count; i < 2*edge_count; ++i) {
       nfa_state temp = edges[i].src;
       edges[i].src  = edges[i].dest;
       edges[i].dest = temp;
    }

    qsort(edges+edge_count, edge_count, sizeof(edges[0]), edge_compare);
    e = edges + edge_count;

    for (i=1; i < num_nodes; ++i) {
        node[i].rev_number_edges = 0;
        node[i].rev_edges  = e;
        while (e < edges + edge_count*2 && e->src == i) {
            ++node[i].rev_number_edges;
            ++e;
        }
    }
}

void compile_nfa(void)
{
    build_edge_tables();
    compute_edist();
    compute_sdist();
}

///////////////////////////////////////////////////////////////////

/*
 *  Modify the finite automaton to allow k errors.
 *
 *  We duplicate the graph 'k' times.  Then we add a
 *  transition from state J to state J' which matches
 *  anything (insertion), and we add a transition from
 *  state J to state K' which matches everything and
 *  matches epsilon, if there is a non-epsilon transition
 *  from J to K.  (substition and deletion)
 */
void error_augment(int k)
{
    int i,j;
    nfa_state z;
    int n = num_nodes-1;
    int ne = edge_count;

    if (!k) return;

    // if there are too many, the creates will error out for us
    for (i=0; i < k; ++i) {
       // clone the graph
       for (j = 1; j <= n; ++j) {
          z = create_state();
          assert(z == j + n*(i+1));
          create_transition(j + n*i, ALL, 0, z);
       }
       for (j = 0; j < ne; ++j) {
          // insertion
          create_transition(edges[j].src+(i+1)*n, edges[j].type,
               edges[j].ch, edges[j].dest + (i+1)*n);
          // substitution/deletion
          if (edges[j].type != EPSILON)
               create_transition(edges[j].src+i*n, ALL_AND_EPSILON, 0,
                            edges[j].dest + (i+1)*n);
       }
       // epsilon them back to the final state so backwards searching works
       create_transition(final_state + (i+1)*n, EPSILON, 0, final_state);
   }
}

////////////////////////////////////////////////////////////////////////

// Compute a transition from an NFA state.  We also compute the range
// of characters a series of transitions is valid for; this allows
// that entire range to be cached in a DFA state.  That means we have
// to intersect the range from each of the transitions.

int min_match_char, max_match_char;

void reset_transition(void)      { min_match_char = 0; max_match_char = 255; }
int num_transitions(nfa_state f) { return node[f].number_edges; }

#define compare3(three, one) ((three & 0xff) == one ||      \
     ((three>>8) &0xff) == one || ((three>>16)&0xff)==one)

nfa_state transition_to(nfa_state f, int trans, int c)
{
   struct edge *e = node[f].edges + trans;
   int d;

   // IS_EPSILON matches both EPSILON and ALL_AND_EPSILON
   if (c == ECHAR) return IS_EPSILON(e->type) ? e->dest : 0;
   if (c == OMEGA) return e->type == EPSILON ? 0 : e->dest;

   d = e->ch;

   switch (e->type) {
      case EPSILON:
         return 0;    // we already special case epsilon

      case ALL:
      case ALL_AND_EPSILON:
         return e->dest;   // note that min_char & max_char are already set

      case ONE:
         if (c == d) {
            min_match_char = max_match_char = c;
            return e->dest;
         } else {
            if (c > d) {
               if (min_match_char <= d) min_match_char = d+1;
            } else {
               if (max_match_char >= d) max_match_char = d-1;
            }
            return 0;
         }

      case BUT:
         if (c == d) {
            min_match_char = max_match_char = c;
            return 0;
         } else {
            // determine whichever range includes c
            if (c > d) {
               if (min_match_char <= d) min_match_char = d+1;
            } else {
               if (max_match_char >= d) max_match_char = d-1;
            }
            return e->dest;
         }

      case CHARSET:
         min_match_char = max_match_char = c;
         // we could scan around the table
         if (d < 0)
            return compare3(d, c) ? e->dest : 0;
         return charset_match[d][c] ? e->dest : 0;

      case INV_CHARSET:
         min_match_char = max_match_char = c;
         // we could scan around the table
         if (d < 0)
            return compare3(d, c) ? 0 : e->dest;
         return charset_match[d][c] ? 0 : e->dest;

      default:
         fprintf(stderr, "sgrep: Error examining transitions.\n");
         exit(1);
    }
}

int rev_num_transitions(nfa_state f) { return node[f].rev_number_edges; }

nfa_state rev_transition_to(nfa_state f, int trans, int c)
{
    struct edge *e = node[f].rev_edges + trans;
    int d;

    if (c == ECHAR)
       return IS_EPSILON(e->type) ? e->dest : 0;

    d = e->ch;

    switch (e->type) {
        case EPSILON:
            return 0;

        case ALL_AND_EPSILON:
        case ALL:
            return e->dest;

        case ONE:
            if (c == d) {
                min_match_char = max_match_char = c;
                return e->dest;
            } else {
                if (c > d) {
                    if (min_match_char <= d) min_match_char = d+1;
                } else {
                    if (max_match_char >= d) max_match_char = d-1;
                }
                return 0;
            }

        case BUT:  
            if (c == d) {
                min_match_char = max_match_char = c;
                return 0;
            } else {
                if (c > d) {
                    if (min_match_char <= d) min_match_char = d+1;
                } else {
                    if (max_match_char >= d) max_match_char = d-1;
                }
                return e->dest;
            }

        case CHARSET:
            min_match_char = max_match_char = c;
            if (d < 0) return compare3(d,c) ? e->dest : 0;
            return charset_match[d][c] ? e->dest : 0;

        case INV_CHARSET:
            min_match_char = max_match_char = c;
            if (d < 0) return compare3(d,c) ? 0 : e->dest;
            return charset_match[d][c] ? 0 : e->dest;

        default:
            fprintf(stderr, "Error examining transitions.\n");
            exit(1);
    }
}
