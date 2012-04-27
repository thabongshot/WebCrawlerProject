#ifndef CR_FIRSTCHUNK_H
#define CR_FIRSTCHUNK_H

/*
 * Web Crawler.
 * Working functions for first chunk.
 * 
 * 
 * >> get chunk string ( use getchunkbody module )
 * 	>> hyperlink filtering
 * 	>> 
 * 	>> 
 * >>return pointer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CR_Qmaker.h"


#define FREE(A)	{free(A); A=NULL;}

/**********************************************************
 *	Data Structure					  *
 *	for First Chunk					  *
 *********************************************************/

typedef enum
{
	SET1 = 1,
	SET2,
	SET3
}SET_TYPE;


// for URL queue 
typedef struct TagSet1Node
{
	char* rooturl;
	struct TagSet1Node* next;
} Set1Node;

// after first getchunk()
typedef struct TagSet2Node
{
	char* rooturl;
	char* chunk;
	struct TagSet2Node* next;
} Set2Node;

// after hyperlink filtering ()
typedef struct TagSet3Node
{
	char* rooturl;
	char* keyword;
	char* url;
	char* title;
	struct TagSet3Node* next;
} Set3Node;

/********************************************************
 *	Functions					* 
 *	for Data Structure				*
 ********************************************************/

//Set1Node* CR_CreateSet1Node(const char* rooturl);
extern void* CR_AllocSetNode(int setNum);
extern void CR_DestroySetNode(int setNum, void* node);


/********************************************************
 *	>> hyperlink filtering				*
 *	>> module body					*
 ********************************************************/
extern void CR_FirstChunkBody(LinkedQueue* set2Queue, LinkedQueue* set3Queue);

static void CR_HyperlinkFilter( Set2Node* inputSet2, LinkedQueue* set3Queue);

static void CR_gethref(char* dest, char* src);

static void CR_gettitle(char* dest, char* src);

static char* CR_genstr(char* buf);


static void CR_TagRemover( char* chunk );

static void CR_makeblank(char* buf, char* str);

static unsigned long int CR_stringhash(const unsigned char* str);


#endif
