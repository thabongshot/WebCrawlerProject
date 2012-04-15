#ifndef BOYER_MOORE_H
#define BOYER_MOORE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define ALPHABET_SIZE ( 1 << CHAR_BIT )

static void compute_prefix(
					const char* str, 
					size_t size, 
                           		int result[size]
								);

static void prepare_badcharacter_heuristic(
					const char *str, 
					size_t size, 
                                       	int result[ALPHABET_SIZE]
								);

static void prepare_goodsuffix_heuristic(
					const char *normal,
					size_t size, 
                                  	int result[size + 1]
								); 

/*
* Boyer-Moore search algorithm
*/
extern char* CR_BoyerMoore(
				const char *haystack, 
                              	const char *needle
							);


#endif
