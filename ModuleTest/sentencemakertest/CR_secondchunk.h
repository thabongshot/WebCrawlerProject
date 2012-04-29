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




/**********************************************************
 *      Data Structure                                    *
 *      for Second Chunk                                  *
 *********************************************************/

typedef struct TagSet4 {
	char* rooturl;		// url seed where we get chunk from
        char* url;              // secondly filtered hyperlink
        char* title;            // title of the hyperlink
        char* keyword;          // what the title contains
        char* contents;         // contents of the hyperlink
	struct TagSet4* next;
} Set4Node;



/**********************************************************
 *      Functions for 		                          *
 *      Second Chunk                                      *
 *********************************************************/
extern void CR_SecondChunkBody( Set4Node* inNode );


/**********************************************************
 *      Sentence make function                            *
 *********************************************************/
extern char* CR_sentencemaker(char* str, char* keyword);
static int CR_strlen(const char* str);


/**********************************************************
 *      HTML Tag Remover Module                           *
 *	Set of 3 functions				  *
 *      Second Chunk                                      *
 *********************************************************/

static void CR_TagRemover( char* chunk );

static void CR_makeblank(char* buf, char* str);

static unsigned long int CR_stringhash(const unsigned char* str);


#endif
