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


/**********************************************************
 *	Data Structure					  *
 *	for First Chunk					  *
 *********************************************************/
typedef struct TagSet1Node
{
	char* url;
	Set1Node* next;
}Set1Node;
//CR_CreateSet1List();
//CR_AppendSet1List();
//CR_RmHeadSet1List();

typedef struct TagSet2Node
{
	char* url;		
	char* chunk;
	Set2Node* next;
} Set2Node;

typedef struct TagSet3Node
{
	char* url;
	char* title;
	Set3Node* next;
} Set3Node;

typedef struct TagSet4Node
{
	char* url;
	char* title;
	char* word;
	Set4Node* next;
} Set4Node;

/**********************************************************
 *	>> hyperlink filtering				  *
 *	>> module body					  *
 *********************************************************/


#endif CR_FIRSTCHUNK_H

