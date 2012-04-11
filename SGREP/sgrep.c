/*
 *    SGREP
 *
 *    Copyright 1993, 1998 Sean Barrett
 *
 *    File: sgrep.c
 *
 *   The main loop is at the end of the file; everything else is
 *   just there to support the main loop (well, duh).
 *
 *   My code is simplified by assuming there's only one accepting state.
 *   If there's more than one, add a new node, add epsilon transitions
 *   from all the accepting states, and declare that one the final one.
 *
 *   Probably, if there are less than (sizeof(int)*8) [i.e. 32 or 64] nodes
 *   in the main automaton graph, many of these things should be allocated
 *   differently.  A mapping could be represented, guaranteed, by 32 or 64
 *   integers; processing would occur on those that are non-zero, etc.
 *   All the bitarrays would be single integers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nfa.h"

int use_dfa3_cache=1;
int use_quickshift=1;

#define COLLECT_COUNTS          0
#define COLLECT_STATISTICS      0
#define COLLECT_PERFORMANCE     0

/*
 *  There are three kinds of statistic collections:
 *
 *     COLLECT_COUNTS
 * Compute how many of various kinds of unique objects/states were encountered
 *
 *     COLLECT_STATISTICS
 * Compute statistics about internal data structures
 * (i.e. hash table performance)
 *
 *     COLLECT_PERFORMANCE
 * Compute information about how effective the main algorithms
 * were (characters examined, etc.)
 */

#if COLLECT_COUNTS
   #define COUNT(x)        (x)
   int num_dfa=0, num_starts=0, num_mappings=0, num_dfa3=0;
   int num_map_source=0, num_map_total=0;
#else
   #define COUNT(x)
#endif

#if COLLECT_STATISTICS
   #define STAT(x)        (x)
   int dfa_search_count=0, dfa_search_len=0, map_search_count=0;
   int map_search_len=0, bar_search_count=0, bar_search_len=0;
#else
   #define STAT(x)
#endif

#if COLLECT_PERFORMANCE
   #define PERF(x)        (x)
   int shift_by[256], char_seen=0, char_total=0, qshift=0, nshift=0;
#else
   #define PERF(x)
#endif

///////////////////////////////////////////////////////////////////////


// data structures

typedef unsigned int Bitarray;

typedef struct st_mapdata Mapdata;
typedef struct st_dfastate Dnfa_state;
typedef struct st_dfatrans3 DFAtrans3;

typedef struct st_startdata
{
  Bitarray *maxMinusMin;  // in maxEstimate(s,i) but not minEstimate(s,i)
  Bitarray *min;          // bitmap of elements in minEstimate(s,i)
  int       minShift;     // minimum amount we should allow a shift by
} Startdata;

// a Dnfa_state is a single state representing
// a set of states from the main nfa.  We
// compute some composite information as well
struct st_dfastate
{
  Dnfa_state * next; //  link to next Dnfa_state with the same hash key 
  int sdist;       //  furthest distance from start of any of my subelements

  Startdata *startdata;    //  minimum, maximum reachability estimates
  short      maxLookahead;
  short      accept;

  short nodecount;
  nfa_state nodes[1];   // variably alloc'd
};

// This is biggy monstosity of our automaton machinery:
// an array of mappings, pairs of which NFA state we'd
// have to be in now for our observed prefix to lead to
// a given state.
struct st_mapdata
{
   Mapdata  *next;   // link to the next map with the same hash key
   Dnfa_state *dfa;    // pointer to the DFA I am the map for
   Dnfa_state *range;  // pointer to the DFA that is my projection; unused

   short     mapcount;   // count of number of elements in map array
   nfa_state mapping[2]; // array of pairs of which nodes mapped to which state
};

// The central monstrous DFA, which represents a full regexp parse
// of the text up to the anchor point as a single DFA, then the
// mappings with a mapping, and then an integer which is our current
// offset in our readahead.  These are all 3 combined into unique
// nodes in the "DFA-3" graph.  (3 because its state is a triplet).
typedef struct st_dfastate3
{
   Dnfa_state * start;      // the anchor point
   Mapdata  * map;          // mappign during lookahead
   int        lookahead;    // how far we're looking ahead to read
   DFAtrans3 *trans;        // pointer to the DFA-3 transition table
} Dnfa_state3;

// a single element of a transition table
struct st_dfatrans3
{
   int          shift;      // how far to move the anchor point
   Dnfa_state3 *state;      // the resultant state we are in
};


/*
 *
 * General
 *    int disjoiintListSet(Dnfa_state *a, Bitarray *)
 *      Return non-zero if the intersection of the two sets is empty
 *
 * Start state manipulations
 *
 *    void computeStartData(Dnfa_state *)
 *      "pre"-compute maxMinusMin, min, maxLookahead
 *
 * Mapping manipulations
 *    Mapdata *advance_mapping(Mapdata *map, int ch)
 *      process this character through the mapping; could be made
 *      table driven if desired, but we're only going to put tables
 *      in the DFA-3 cache, not here
 *
 *    Dnfa_state *selectAndProject(Mapdata *map, Bitarray *selection)
 *      compute the projection of the intersection of map with selection
 *
 *    void computeSdist(Dnfa_state *state)
 *      compute the sdist value for the encoded NFA
 *
 * DFA-3 manipulations
 *
 *    Dnfa_state3 *getDfa3(Dnfa_state *start, int lookahead, Mapdata *map)
 *      get a Dnfa_state3 that represents all this.
 *
 *    void add_dfa3_transition(Dnfa_state3*,Dnfa_state3*, int, int c1, int c2)
 *        add a transition to the DFA-3 cache, ranging from character c1 to c2
 *
 * Variables
 *
 *    Dnfa_state  *dfa_start;    start state for the general DFA searcher
 *    Mapdata     *allmap;       mapping from every node to itself
 *    Dnfa_state3 *dfa3_start;   DFA3 representing start state:
 *                                <dfa_start, maxLookahead(dfa_start), allmap>
 *
 *   nfa_state   start_start;     start state for the NFA searcher
 *   nfa_state   final_state;     accepting state
 *
 * Here are the subfunctions required to support the above:
 *
 *    Dynamically build an NFA state.  Convert it to a DFA state.
 *    Dynamically build an NFA mapping.  Convert it to a shared mapping.
 *    Keep shared tables of DFA3 states.
 *    Manage tables of DFA3 transitions, within given storage requirements.
 *
 * We need graph operations to support the various precomputations.
 *   maxLookahead is computed from the underyling edist fields in the
 *   automaton.  min is computed statically.  maxMinusMin is iteratively
 *   computed by walking the Dnfa_state through OMEGA transitions.
 *
 * Finally, when we're computing a new mapping, we will dynamically
 * build the mapping table, and we will keep the domain of the mapping
 * computed too.  The domain can be computed as a "dynamically build
 * an NFA state".  We will need to keep a bitarray for each node,
 * indicating which pairs have already been inserted, and we will keep
 * one indicating our projection.  This allows us to use both elements
 * for hashing.
 */

void *permalloc(unsigned int size)
{
   void *p = malloc(size);
   if (!p) { fprintf(stderr, "sgrep: Out of memory.\n"); exit(3); }
   return p;
}

typedef struct
{
   char *nextalloc;
   int   remain;
   int   maximum;
   int   alloc;
} Segment;

void initializeSegment(Segment *s, int max)
{
   int alloc;
   max = (max + 3) & ~3;
   alloc = max * 64;
   s->maximum = max;
   while (alloc > (1 << 18))
      alloc >>= 1;
   if (alloc < max) alloc = max;
   if (alloc < 8192) alloc = 8192;

   s->alloc = alloc;
   s->nextalloc = permalloc(alloc);
   s->remain = alloc;
}

#define getNextBlock(s)        ((void *) (s)->nextalloc)

void consumeBlock(Segment *s, int size)
{
   size = (size + 3) & ~3;
   s->remain -= size;
   s->nextalloc += size;
   if (s->remain < s->maximum) {
      s->nextalloc = permalloc(s->alloc);
      s->remain    = s->alloc;
   }
}

/*
 *  Bitarray data structure
 *
 *    A bitarray is simply a representation of a set of nodes.
 *    If bit #x is one, then node #x is in the set.  Generally,
 *    we will kep a list of nodes in the set as well.  Use of
 *    both of them allows all set operations to be O(1), except
 *    clearing the set and iterating over the set, which are O(n)
 *    where n is the number of nodes in the set.  (With just the
 *    array, this would take O(N) where N is the number of nodes
 *    in the graph.  When the set is used to indicate reachability
 *    on a sparse graph, this representation is more efficient.)
 *    Finally, the bitarray can be used to compute a hash value for
 *    the set in O(N) time, with a very small constant; we treat the
 *    bitarray as an array of integers and add them. 
 *
 *    We provide operations to test if a node is present, add a node,
 *    delete a node, clear the structure given a list, and to compute
 *    the hash value.
 *
 *    The maximum size of a bitarray is dependent on the sizeof
 *    the graph being processed; it's stored in mapsize, once we're
 *    done messing with the graph.
 */

#define BITS                (sizeof(int) * 8)

static void clearBitarray(Bitarray *z)
{
   int i;
   for (i=0; i < mapsize; ++i)
      z[i] = 0;
}

#define field(x,y)       ((x)[(unsigned) (y) / BITS])
#define value(y)         (1 << ((y) & (BITS-1)))

#define addNode(x,y)     (field(x,y) |=  value(y))
#define removeNode(x,y)  (field(x,y) &= ~value(y))
#define existsNode(x,y)  (field(x,y) &   value(y))

static void deleteList(Bitarray *z, nfa_state *list, int length)
{
   while (length--)
      removeNode(z, *list), ++list;
}

static void makeBitarray(Bitarray *z, nfa_state *list, int length)
{
   while (length--)
      addNode(z, *list), ++list;
}

#define HASH_BITS        11
#define HASH_SIZE        (1 << HASH_BITS)

static int get_hash_key(Bitarray *z)
{
   unsigned int i,acc=0;
   for (i=0; i < mapsize; ++i)
     acc += z[i];
   #if HASH_BITS < 32
     i = acc ^ (acc >> HASH_BITS);
     #if HASH_BITS*2 < 32
        acc = i ^ (i >> HASH_BITS);
     #else
        acc = i;
     #endif
   #endif
   return acc & (HASH_SIZE - 1);
}
 
/*
 *  Bitarray allocator
 *
 *  We use a variable number of bitarrays during processing, and sometimes
 *  we keep them forever, sometimes we're done with them somewhat quickly.
 *  Thus we want to allocate and deallocate them intelligently.
 */

typedef union duck Bitalloc;
union duck {
   Bitalloc *next;
   Bitarray value;
};

static Bitalloc *free_array=0;

static Bitarray *allocate_bitarray(void)
{
   Bitarray *z;
   if (free_array) {
      z = &(free_array->value);
      free_array = free_array->next;
      *z = 0;        /* clear out the first field of the union */
   } else {
      z = permalloc(mapsize);
      clearBitarray(z);
   }
   return z;
}

static void free_bitarray(Bitarray *z)
{
  // bitarray must be clear when you free it
#if 1
   clearBitarray(z);
#endif
   ((Bitalloc *) z)->next = free_array;
   free_array = (Bitalloc *) z;
}

//////////////////////////////////////////////////////////////////////////

/*
 *  DFA system
 *
 *  The DFA system simply maps sets of nodes to a unique ID, Dnfa_state *.
 *  This allows us to build precomputed information associated with sets
 *  of nodes, since we can associate this information with the DFA.
 *  Additionally, all things which deal with sets of nodes generally
 *  manipulate DFAs instead, if they can. 
 *
 *  We assume we never flush any DFA nodes; thus, we could require
 *  exponential space in the worst case (well, min(input-size, 2^node-count)).
 *  The only "transition table" associated with a DFA is the transition
 *  on OMEGA, which is stored in state->startdata[1].maxMinusMin, sort of.
 *  Because it subtracts min, it's not very likely to be shared.
 *  state->startdata[0].min is a bitarray version of the nodes.  This
 *  means we can even map a bitarray to a DFA by searching for 
 *  a DFA which matches this.  This means we can save a little storage
 *  for that bitarray.
 *
 */

static Dnfa_state *dfa_hash[HASH_SIZE];
static Segment dfa_seg;

#define get_next_dfa()           ((Dnfa_state *) getNextBlock(&dfa_seg))
#define get_nfa_storage()        (get_next_dfa()->nodes)

static void init_dfa_storage(void)
{
   initializeSegment(&dfa_seg,
          sizeof(Dnfa_state) + sizeof(nfa_state) * num_nodes);
}

static Dnfa_state *convert_to_dfa(int count, Bitarray *z)
{
   // count is number of elements in list, z is an appropriate bitarray.
   // the list itself is always stored in get_nfa_storage()

   int hash = get_hash_key(z);
   Dnfa_state *d = dfa_hash[hash];
   nfa_state *a;
   int i;

   STAT(++dfa_search_count);
   while (d) {
      STAT(++dfa_search_len);
      // to match, they must have the same number of nfa states
      if (d->nodecount == count) {
         // now check that ever nfa state in the existing d
         // are in the current
         a = d->nodes;
         for (i=0; i < count; ++i)
            if (!existsNode(z, a[i]))
               break;  // nope, missing this nfa state

         // were they all there?
         if (i == count)
            return d;
      }
      d = d->next;
   }

   assert(count != 0);

   COUNT(++num_dfa);
   d = get_next_dfa();
   consumeBlock(&dfa_seg, sizeof(Dnfa_state) + count * sizeof(nfa_state));

   d->startdata = 0;
   d->sdist     = -32767;      // indicate we haven't computed it yet
   d->accept    = !!(existsNode(z, final_state));
   d->nodecount = count;

   d->next      = dfa_hash[hash];
   dfa_hash[hash] = d;
   return d;
}

static Bitarray *shareBitarray(Bitarray *z)
{
   // we're about to make this bitarray readonly and permanent.
   // Swap in a different one if possible
   int hash = get_hash_key(z);
   Dnfa_state *d = dfa_hash[hash];
   STAT(++bar_search_count);
   while (d) {
      STAT(++bar_search_len);
      if (d->startdata && !memcmp(d->startdata[0].min,z,mapsize*sizeof(int))) {
         clearBitarray(z);
         free_bitarray(z);
         return d->startdata[0].min;
      }
      d = d->next;
   }
   return z;
}

/*
 * Mapping finite automaton system
 *
 * The mapping system is quite a bit more complex than the DFA system.
 * First of all, we can't put a good upper bound on the number of elements
 * in our list; it could be as many as N*N, although we're quite hosed
 * performance wise if it is.  Thus, rather than using a memory segment
 * thing for these, we have to actually potentially reallocate them.  To
 * do this we explicitly allocate a separate block for them if their
 * storage requirements become excessive.
 *
 * Secondly, we actually compute transitions on the mapping system.
 *
 * A mapping is a set of ordered pairs <cur, dest>.  We store them as a
 * flat, unordered array of elements.  We also build an NFA/DFA with all
 * the <cur> elements.
 *
 * To compare a just-built mapping to an existing mapping, we compare
 * their sizes.  If the sizes are the same, we iterate over the elements
 * of the existing mapping, doing bit tests to see if they are present
 * in the just-built one.  This works because the just-built one keps
 * a bitarray indicating which values are valid as <cur>, and then for
 * each valid <cur> it keeps a bitarray indicating which values of <dest>
 * exist.  Thus, we can do the comparison in O(size) time, without
 * requiring us to store the bitarrays except for the current one.
 *
 * At worst, the bitarrays require N*N/8 bytes of storage; thus it
 * requires at worst 500K to handle an automaton with 2000 nodes.
 */

static Segment map_seg;
static int max_storage, max_nodes;

#define map_storage()        ((Mapdata *) getNextBlock(&map_seg))

static Bitarray **current_dest;
static Bitarray  *current_map;
static Bitarray  *all_dest;


static void init_map_storage(void)
{
   int i;
   max_nodes   = num_nodes * 2;
   max_storage = sizeof(Mapdata) + max_nodes * sizeof(nfa_state);
   initializeSegment(&map_seg, max_storage);
   current_dest = permalloc(sizeof(Bitarray *) * num_nodes);
   current_map  = allocate_bitarray();
   all_dest     = allocate_bitarray();
   for (i=0; i < num_nodes; ++i)
      current_dest[i] = NULL;
}

static Mapdata   *thismap;
static Mapdata   *map_hash[HASH_SIZE];

// convert a map in current_map, all_dest, current_dest, and thismap
static Mapdata *convert_to_map(int count, int sourcecount)
{
   int hash = get_hash_key(current_map) + 2*get_hash_key(all_dest);
   int i;
   Mapdata *m;
   Dnfa_state *dom;
   nfa_state *z;

   hash = hash & (HASH_SIZE-1);
   m = map_hash[hash];

   dom = convert_to_dfa(sourcecount, current_map);
   deleteList(current_map, dom->nodes, dom->nodecount);

   STAT(++map_search_count);
   while (m) {
      STAT(++map_search_len);
      if (m->mapcount == count && m->dfa == dom) {
         // they have same number of elements, and same domain.
         // since they share the domain, then
         // all of the current_dest[] tested below are non-NULL, so
         // we don't need to check
         z = m->mapping;
         for (i=0; i < count; i += 2)
            if (!existsNode(current_dest[z[i]], z[i+1]))
               break;
         if (i == count) {                        /* we have a winner */
            if (thismap != map_storage())
               free(thismap);
            return m;
         }
      }
      m = m->next;
   }

   assert(sourcecount != 0);

   COUNT(++num_mappings);
   COUNT(num_map_source += sourcecount);
   COUNT(num_map_total  += count);

   // well, we've never seen THIS one before

   thismap->dfa = dom;
   thismap->mapcount = count;

   if (thismap == map_storage())
      consumeBlock(&map_seg, sizeof(Mapdata) + sizeof(nfa_state) * count);    

   thismap->next = map_hash[hash];
   map_hash[hash] = thismap;

   return thismap;
}

/*
 *  build a mapping.  This allows us to keep all the variables required
 *  in one area, and makes the mapping-transition routine read much more
 *  clearly then if we passed the state around.  Undoubtedly it's slower
 *  than it has to be, though.
 *
 *     current_map
 *     all_dest
 *     current_dest
 */

static nfa_state *source;
static int sourcecount,totalcount,max_match_charount;

static void start_new_mapping(void)
{
   thismap     = map_storage();
   sourcecount = totalcount = 0;
   source      = get_nfa_storage();
   max_match_charount    = max_nodes;
}

static void add_mapping(nfa_state x, nfa_state mapto)
{
   // first check if x is already in our state
   if (!existsNode(current_map, x)) {
      addNode(current_map, x);
      current_dest[x] = allocate_bitarray();
      source[sourcecount++] = x;
   } else {
      if (existsNode(current_dest[x], mapto))
         return;
   }

   addNode(current_dest[x], mapto);
   addNode(all_dest,        mapto);

   if (totalcount == max_match_charount) {
      // allocate more space for the mapping

      int new_max = max_match_charount + 16;
      int size  = sizeof(Mapdata) + new_max * sizeof(nfa_state);

      // check if this is the first time we grow;
      // if is, we need to malloc, 'cause we're currently
      // using the scratch space

      if (max_match_charount > max_nodes) {
         thismap = realloc(thismap, size);
         if (!thismap) { fprintf(stderr, "sgrep: Out of memory\n"); exit(1); }
      } else {
         thismap = permalloc(size);
         memcpy(thismap, map_storage(), size);
      }
      max_match_charount = new_max;
   }
   thismap->mapping[totalcount  ] = x;
   thismap->mapping[totalcount+1] = mapto;
   totalcount += 2;
}

static Mapdata *end_new_mapping(void)
{
   // convert_to_map does all the hard stuff; this is just cleanup
   int i;
   Dnfa_state *d;
   nfa_state  *f;
   Mapdata  *m;
   int c;

   m = convert_to_map(totalcount, sourcecount);
   d = m->dfa;
   c = m->mapcount;
   f = m->mapping;

        /* clear the bitarrays */
   for (i=0; i < c; i += 2)
      removeNode(current_dest[f[i]], f[i+1]);

        /* free all the ones we allocated */

   c = d->nodecount;
   f = d->nodes;
   for (i=0; i < c; ++i) {
      assert(current_dest[f[i]] != NULL);
      free_bitarray(current_dest[f[i]]);
      current_dest[f[i]] = 0;
   }
   clearBitarray(all_dest);

   return m;
}

// generate the eclosure of the current (unshared) mapping.
static void mapping_eclosure(void)
{
   int i,j,k, n;
   nfa_state p, *f = thismap->mapping;

   // iterate over all our nodes, looking for epsilon transitions
   // our total count may increase as we go, or get relocated */

   // notice how superinefficient it is to go iterate
   // unintelligently like this
   
   for (i=0; i < totalcount; i += 2) {
      n = f[i];
      k = rev_num_transitions(n);
      for (j=0; j < k; ++j) {
         if (p = rev_transition_to(n, j, ECHAR)) {
            add_mapping(p, f[i+1]);
            f = thismap->mapping;      // reload it in case it got moved
         }
      }
   }
}

/*
 *  DFA3  state management
 *
 *  This section manages DFA3s, except for the transition tables.
 *  Thus, it basically just implements the mapping of
 *    Dnfa_state+Mapping+int --> DFA3.
 *
 *  DFA3s do not embed in the structure any next pointer.  This is
 *  because we use different sorts of approaches to searching for DFA3s.
 *
 *  At the moment, this code is broken--we keep _all_ DFA3 states
 *  we encounter, and break if we hit DFA3_HASH unique ones.
 *  Given the size of the state encoded in a DFA3, this could
 *  certainly happen.
 *
 */

static DFAtrans3 *bogus_table;

static Segment dfa3_seg;
static void init_dfa3_storage(void)
{
   initializeSegment(&dfa3_seg, sizeof(Dnfa_state3));
}

    // require 128K of storage for our hash table
#define DFA3_HASH                32768

static Dnfa_state3 *dfa3_hash[DFA3_HASH + 1];

Dnfa_state3 *getDfa3(Dnfa_state *x, int lookahead, Mapdata *m)
{
   Dnfa_state3 *d;
   // use the state and mapdata _pointers_ as hash keys--they never relocate
   int key = (((int) x) + ((int) m) + lookahead * 371) >> 2;
   key = key & (DFA3_HASH-1);

   // now linear probe
   while ((d = dfa3_hash[key]) != NULL) {
      if (d->start == x && d->lookahead == lookahead && d->map == m)
         return d;
      if (++key == DFA3_HASH) key = 0;
   }

   COUNT(++num_dfa3);
   d = getNextBlock(&dfa3_seg);
   consumeBlock(&dfa3_seg, sizeof(Dnfa_state3));

   d->start     = x;
   d->lookahead = lookahead;
   d->map       = m;
   d->trans     = bogus_table;

   dfa3_hash[key] = d;
   return d;
}

//////////////////////////////////////////////////////////////////////////

//  DFA3 transition management

#define BOGUS_SHIFT    (1 << 24)
    // must be bigger than largest buffer size; I use 16M
    // if you make it too big, it may break the pointer math  :(

int alphasize = 256;     // number of things in our input alphabet
                         // may be smaller if we use mappings

#define NUM_TABLES                512

static void init_dfa3_trans_storage(void)
{
   int i;
   bogus_table = permalloc(sizeof(DFAtrans3) * alphasize);
   for (i=0; i < alphasize; ++i)
      bogus_table[i].shift = BOGUS_SHIFT;
}

static void allocate_trans_table(Dnfa_state3 *from)
{
   int i;
   from->trans = permalloc(sizeof(DFAtrans3) * alphasize);
   for (i=0; i < alphasize; ++i)
      from->trans[i].shift = BOGUS_SHIFT;
}

static void add_dfa3_transition(Dnfa_state3 *from, Dnfa_state3 *to,
      int shiftby, int min_match_char, int max_match_char)
{
   if (use_dfa3_cache) {
      if (from->trans == bogus_table)
         allocate_trans_table(from);

      while (min_match_char <= max_match_char) {
         from->trans[min_match_char].shift = shiftby;
         from->trans[min_match_char].state = to;
         ++min_match_char;
      }
   }
}

///////////////////////////////////////////////////////////////////////

// Main support functions

// This routine does all the work of moving around on the graph
static Mapdata *advance_mapping(Mapdata *z, int ch)
{
   int i, j, k, n;
   nfa_state p, *f = z->mapping;
   int c = z->mapcount;

   // record what range of chars ALL the following transitions are valid for
   reset_transition();

   start_new_mapping();

   for (i=0; i < c; i += 2) {
      n = f[i];
      k = rev_num_transitions(n);
      for (j=0; j < k; ++j)
         if (p = rev_transition_to(n, j, ch))
         add_mapping(p, f[i+1]);
   }

   mapping_eclosure();
   return end_new_mapping();
}

// Check that there's no overlap between a list and a set of nfa states
static int disjointListSet(Dnfa_state *d, Bitarray *z)
{
   int c = d->nodecount;
   nfa_state *f = d->nodes;

   while (c--)
      if (existsNode(z, f[c]))
         return 0;
   return 1;
}

// intersect mapping with selection, and project down to domain
static Dnfa_state *selectAndProject(Mapdata *m, Bitarray *z)
{
   nfa_state *k = get_nfa_storage(), *f = m->mapping;
   int count=0, c = m->mapcount;
   int i;
   Bitarray *set = allocate_bitarray();
   Dnfa_state *res;

   for (i=0; i < c; i += 2) {
      if (existsNode(z, f[i]) && !existsNode(set, f[i+1])) {
         addNode(set, f[i+1]);
         k[count++] = f[i+1];
      }
   }

   res = convert_to_dfa(count, set);
   deleteList(set, res->nodes, res->nodecount);
   free_bitarray(set);
   return res;
}


static void computeSdist(Dnfa_state *s)
{
   int i,q;
   int max = use_quickshift ? 0 : 32767;
   for (i=0; i < s->nodecount; ++i)
      if ((q = sdist(s->nodes[i])) > max)
         max = q;
   s->sdist = max;
}

static int eclosure(nfa_state *f, int count, Bitarray *z)
{
   int j,n,k,q;
   nfa_state p;

   for (j=0; j < count; ++j) {
      n = f[j];
      k = num_transitions(n);
      for (q=0; q < k; ++q) {
         if ((p = transition_to(n,q,ECHAR)) != 0 && !existsNode(z,p)) {
            addNode(z, p);
            f[count++] = p;
         }
      }
   }
   return count;
}

// temporary storage for use while computing maxEstimate
static nfa_state *store1, *store2;
static Dnfa_state *dfa_start;

// compute the info that's unique to being in a particular
// anchor point (note this is not a dfa3 unique thing, only
// unique sets of the original NFA, so we don't have to
// compute this TOO often)--which is good because the
// maxEstimate computation is slow
static void computeStartData(Dnfa_state *s)
{
   Bitarray *z;
   nfa_state *f, *g, *h, p;
   int c, i, min, t, count, fc, q, n, k, j;

   // compute maxLookahead, the furthest distance away that we can look

   c = s->nodecount;
   f = s->nodes;
   min = 256;  // limited by the size of the buffer
   for(i=0; i < c; ++i) {
      t = edist(f[i]);
      if (t < min) min = t;
   }
   s->maxLookahead = min;

   // Allocate space for startdata; we should use a separate segment for these.

   if (min)
      s->startdata = permalloc(sizeof(Startdata) * min);
   else {
      s->accept    = 1;
      s->startdata = (Startdata *) 1;
      return;
   }

   // compute our values for the min field; note that this code sneakily
   // works, even if s is dfa_start
   
   // This is a pretty minimal lower bound estimate, but we expect that in
   // practice it's exact 99% of the time.

   s->startdata[0].min = allocate_bitarray();
   makeBitarray(s->startdata[0].min, s->nodes, s->nodecount);

   for (i = 1; i < min; ++i)
      s->startdata[i].min = dfa_start->startdata[0].min;

   // compute our values for the (max - min) field

   s->startdata[0].maxMinusMin = 0;

   /*
    *  now, we need to compute successive values of maxEstimate, get
    *  the bitmap for it, delete the elements from minEstimate, and
    *  save the bitmap.  That means we'll need to build a new bitmap
    *  each time.  That's ok, though, because the old bitmap doesn't
    *  help for the next iteration of the state.
    */
   g = store1;
   h = store2;

   f = s->nodes;
   fc = s->nodecount;

   for (i=1; i < min; ++i) {
      // compute trans(f, fc, OMEGA) into g, count, and z

      // start with an empty state
      z = allocate_bitarray();
      count = 0;

      for (j=0; j < fc; ++j) {
         n = f[j];
         k = num_transitions(n);
         for (q = 0; q < k; ++q)
            if ((p = transition_to(n,q,OMEGA)) != 0 && !existsNode(z, p)) {
             addNode(z, p);
             g[count++] = p;      
         }
      }

      // swap it into f; note that after one iteration,
      // f&g only use "store1" and "store2"
      f = g;
      g = h;
      h = f;
      fc = count;

      // now generate the epsilon closure of that state
      fc = eclosure(f, fc, z);

      // now f,fc holds the current state, and z is a correct bitmap.
      // now delete min from it

      deleteList(z, dfa_start->nodes, dfa_start->nodecount);
      s->startdata[i].maxMinusMin = shareBitarray(z);
   }
}

///////////////////////////////////////////////////////////////////

//  Ok, we're almost ready to run!

static Mapdata  *allmap;
static Dnfa_state3 *dfa3_start;

static void print_statistics(void);

// This initialization routine is called AFTER the graph has been computed
void init_reverse(void)
{
  nfa_state *z;
  Bitarray *q;
  int i;

  init_dfa_storage();
  init_map_storage();
  init_dfa3_storage();
  init_dfa3_trans_storage();

  store1 = permalloc(num_nodes * sizeof(nfa_state));
  store2 = permalloc(num_nodes * sizeof(nfa_state));

  z = get_nfa_storage();
  z[0] = start_state;

  q = allocate_bitarray();
  addNode(q, start_state);
  i = eclosure(z, 1, q);
  dfa_start = convert_to_dfa(i, q);

  deleteList(q, dfa_start->nodes, dfa_start->nodecount);
  free_bitarray(q);

  start_new_mapping();
  for (i=1; i < num_nodes; ++i)
    add_mapping(i,i);
  mapping_eclosure();
  allmap = end_new_mapping();

  computeStartData(dfa_start);
  dfa3_start = getDfa3(dfa_start, dfa_start->maxLookahead-1, allmap);

#if defined(COLLECT_COUNTS) || defined(COLLECT_STATISTICS) || defined(COLLECT_PERFORMANCE)
  atexit(print_statistics);
#endif
}



/*
 *   reverse regexp scanner
 *
 *   We have two chunks of state:   <cur,i> and <mapping>
 *   They're rolled into one big DFA
 *
 */

int fast_reverse_scan(Dnfa_state *cur, unsigned char *input,
                int len, Dnfa_state **res, unsigned char **output)
{
#ifndef NATURAL_REGISTERS
   // don't trust the compiler to optimize it right;
   // declare some _extra_ variables that are ONLY
   // used in the inner loop--this was necessary in
   // 1993 GCC under linux
   register Dnfa_state3 *st;
   register unsigned char *in, *e;
#endif
   register DFAtrans3 *tr;

   unsigned char *end = input + len;
   Mapdata *map;
   Dnfa_state3 *state, *dest;
   int skip, i, extra, ch;

   extra = 2;   // minimum number of characters quickShift must improve by

   // get our state variables into the right state
   if (!cur->startdata)
      computeStartData(cur);
   skip = cur->maxLookahead;

   state = getDfa3(cur, skip-1, allmap);
   input += skip-1;

   // test our pointer arithmetic assumption
   assert(end + BOGUS_SHIFT > end);


/*
 *   Fast inner DFA3  loop
 *
 *   For each DFA3 state, we have internally coded the maxlookahead
 *   and current lookahead.  Thus, we actually let our input pointer
 *   wander over to the current lookahead location; the pointer variable
 *   always stores the "scanning" location, not the anchor location.
 *   We can recover the anchor if we need to.
 *
 *   For each possible input character, we have coded where to move
 *   the input pointer (back one if we're continuing the same lookahead,
 *   forward a bunch if we've completed a full shift, and forward some
 *   if we've completed a quickshift.
 *
 *   We encode not-currently-cached transitions with huge forward jumps,
 *   and similarly if we reach an accepting state.  Then we can
 *   combine the test for off-the-end with the uncached test.
 */

   while (input < end) {
      // the fast version of the inner loop
#ifdef NATURAL_REGISTERS
      for(;;) {
         tr = &state->trans[*input];
         input += tr->shift;
         if (input >= end) break;
         PERF(++char_seen);
         PERF(++shift_by[tr->shift + 2]);  // bring -1 into range
         state = tr->state;
      }
#else
      // copy into register variables

      st = state;
      in = input;
      e  = end;

      for(;;) {
         tr = &st->trans[*in];
         in += tr->shift;
         if (in >= e) break;
         PERF(++char_seen);
         PERF(++shift_by[tr->shift + 2]);
         st = tr->state;
      }
      state = st;
      input = in;
#endif

      input -= tr->shift;   // undo enormous shift

      // determine why we went off the end

      cur = state->start;
      ch   = *input;

      if (state->trans[ch].shift < BOGUS_SHIFT) {
         *res = cur;  // we got here legitimately.  Return to last state
         *output = input - state->lookahead;
         return 0;
      }

      // we have a bogus shift in our transition table.  determine why
      if (cur->accept) {
         *res = dfa_start;   // force us to restart next time--greplike
         *output = input;
         return 1;
      }

      // Ok, it's a transition that's not cached
      // Unroll our DFA3 state into its components

      cur  = state->start;
      skip = cur->maxLookahead;
      map  = state->map;
      i    = state->lookahead;

      // we can compute our next state and convert it back to a DFA
      // but instead we'll actually advance at least one transition
      // completely, and maybe more if we keep uncovering unseen states

      for(;;) {

         map = advance_mapping(map, *input);
         PERF(++char_seen);

         // check if our estimate for the state cur[skip] is exact;
         // which means roughly maxEstimate(cur,i) == minEstimate(cur,i);
         // specifically:
         // let dom = domain(map), and test
         //   (maxEstimate(cur, i) intersect dom) subset (minEstimate(cur, i))
  
         // If true, then 
         //   (maxEstimate(cur, i) intersect dom) project (map)
         // is the exact NFA state for cur[skip], because map is the mapping
         // of what we'll reach at cur[skip] from cur[i], and we
         // know from the prevous test that we know that all the
         // states we _could_ reach by cur[i] (according to maxEstimate)
         // which can have any effect on our states at cur[skip]
         // (that is, they're in the domain(map)) we are GUARANTEED
         // to reach (because they're in minEstimate).  So we can
         // shift forward to cur[skip] and use the above projection

         if (i == 0 ||
            disjointListSet(map->dfa, cur->startdata[i].maxMinusMin)) {
            cur = selectAndProject(map, cur->startdata[i].min);
            if (!cur->startdata)
               computeStartData(cur);
            dest = getDfa3(cur, cur->maxLookahead-1, allmap);
            add_dfa3_transition(state, dest, (skip-i) + (cur->maxLookahead-1),
                min_match_char, max_match_char);
            input += (skip - i) + (cur->maxLookahead-1);
            PERF(++shift_by[2 + (skip-i) + (cur->maxLookahead-1)]);
            PERF(++nshift);
            state = dest;
            break;  // return to fast DFA code
         }

         /* now try applying the other heuristic, "quick shift"
          * quickshift only uses the mapping, not the anchor state.
          * basically, we analyze the mapping to determine what
          * "infixes" we've seen; then we can quickshift forward
          * to align with the "earliest" infix.  This may cause
          * us to sample some characters more than once (because
          * we don't advance the anchor past the characters already
          * seen), so we only do it if we guarantee we skip more
          * characters than we've scanned--this guarantees linearity.
          * we also want to avoid preventing this from preventing
          * the prefix strategy from working, so we only quickshift
          * if it's at least as good as a regular anchor move on the NEXT
          * character would be.  Adaptive based on current success
          * of the other heuristic might be smarter.
          */
    
         if (map->dfa->sdist < i*2 - skip) {
            // if sdist is uninitialized, compute it
            if (map->dfa->sdist < 0)
               computeSdist(map->dfa);
            // check ratio
            if ((i - map->dfa->sdist) * (skip - i + 1) >= skip * (skip - i)) {
               // do a quick shift: jump to start state
               add_dfa3_transition(state, dfa3_start,
                  (-map->dfa->sdist) + (dfa_start->maxLookahead - 1),
                  min_match_char, max_match_char);
               input += - map->dfa->sdist + (dfa_start->maxLookahead - 1);
               PERF(++shift_by[2 + -map->dfa->sdist + dfa_start->maxLookahead]);
               PERF(++qshift);
               state = dfa3_start;
               break;   // return to the fast DFA3 inner loop
            }
         } 
   
         // just scan back another character
         i--;
         input--;
         dest = getDfa3(cur, i, map);
         add_dfa3_transition(state, dest, -1, min_match_char, max_match_char);
         PERF(++shift_by[2-1]);
         state = dest;
         ch = *input;
         if (state->trans[ch].shift != BOGUS_SHIFT)
            break;  // go back to fast dfa3 loop
      }
   }
   *res = state->start;
   *output = input - state->lookahead;
   return 0;
}

// driver for the above routine which
// actually implements grep-like behavior,
// i.e. outputting the text...
// we need to modify this to prefix it with
// the filename if > 1 file...

extern void printLine(char *);
unsigned char *run_reverse_scanner(unsigned char *input, int len)
{
   static Dnfa_state *x;
   unsigned char *end = input + len, *temp;
   if (!x) x = dfa_start;

   if (dfa_start->accept) {
      while (input < end-1) {
         printLine(input+1);
         input = strchr(input+1, '\n');
      }
      PERF(char_total += input - (end - len));
      return input;
   }

   temp = input;
   while (fast_reverse_scan(x, input, end-input, &x, &input)) {
      printLine(input);
      input = (unsigned char * ) strchr((char *) input, '\n');
      if (input <= temp) input = strchr(input+1, '\n');
      temp = input;
   }
   PERF(char_total += input - (end - len));
   return input;
}

static void print_statistics(void)
{
#if COLLECT_PERFORMANCE
   int i;
   printf("Scanned %d characters; considered %d.\n", char_total, char_seen);
   if (use_dfa3_cache)
      printf("For uncached advances: ");
   printf("Quickshifts: %d; full computations: %d.\n", qshift, nshift);
#endif

#if COLLECT_COUNTS
   printf("%d dfa states, %d mappings, %d dfa start states, %d dfa3 states.\n",
               num_dfa, num_mappings, num_starts, num_dfa3);
   printf("Average states/mapping: %8.3g\n",
                (float) num_map_source / num_mappings);
   printf("Average ranges/mapping: %8.3g\n",
                (float) num_map_total / num_mappings);
#endif

#if COLLECT_STATISTICS
   printf(  "Hash table     # searches   average length\n");
   if (dfa_search_count)
      printf("   dfa          %8d        %8.3g\n",
          dfa_search_count, (float) dfa_search_len / dfa_search_count);
   if (map_search_count)
      printf("   map          %8d        %8.3g\n",
          map_search_count, (float) map_search_len / map_search_count);
   if (bar_search_count)
      printf("bit array       %8d        %8.3g\n",
          bar_search_count, (float) bar_search_len / bar_search_count);
#endif

#if COLLECT_PERFORMANCE
   printf("Skips of length: ");
   for (i=0; i < 256; ++i)
      if (shift_by[i])
         printf("%d:%d  ", i-2, shift_by[i]);
   printf("\n");
#endif
}
