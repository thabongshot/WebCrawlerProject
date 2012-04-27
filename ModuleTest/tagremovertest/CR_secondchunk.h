#ifndef CR_SECONDCHUNK_H
#define CR_SECONDCHUNK_H


/*
 * Web Crawler
 * 
 * Functions of second chunk working set
 * 
 * >> get chunk string <-> save to the set
 * 	>> filtering contents text of chunkbody set
 * 	>> reallocate & substitute
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CR_Qmaker.h"



/**********************************************************
 *      Data Structure                                    *
 *      for Second Chunk                                  *
 *********************************************************/

typedef struct TagSet2 {
	char* rooturl;		// url seed where we get chunk from
        char* url;              // secondly filtered hyperlink
        char* title;            // title of the hyperlink
        char* keyword;          // what the title contains
        char* contents;         // contents of the hyperlink
} Set3Node;



/**********************************************************
 *      Functions for 		                          *
 *      Second Chunk                                      *
 *********************************************************/
extern void CR_SecondChunkBody( /* put args here*/ );


/**********************************************************
 *      Sentence make function                            *
 *********************************************************/
static void CR_sentencemaker(char* str);


/**********************************************************
 *      HTML Tag Remover Module                           *
 *	Set of 3 functions				  *
 *      Second Chunk                                      *
 *********************************************************/

extern void CR_TagRemover( char* chunk );

static void CR_makeblank(char* buf, char* str);

static unsigned long int CR_stringhash(const unsigned char* str);


#endif
