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


#ifndef FREE(a)
#define FREE(a) {free(a);a=NULL;}
#endif

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
	char* url;
	char* title;
	char* keyword;
	struct TagSet3Node* next;
} Set3Node;

/********************************************************
 *	Functions					* 
 *	for Data Structure				*
 ********************************************************/

//Set1Node* CR_CreateSet1Node(const char* rooturl);
void* CR_AllocSetNode(int setNum);
void CR_DestroySetNode(int setNum, void* node);


/**********************************************************
 *	>> hyperlink filtering				  *
 *	>> module body					  *
 *********************************************************/



#endif CR_FIRSTCHUNK_H

