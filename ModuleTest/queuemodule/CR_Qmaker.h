#ifndef CR_QMAKER_H
#define CR_QMAKER_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Web Crawler
 * 
 * Queue
 * Contains non-specific data's pointer (void*)
 * Is used for first & second chunk module
 */


/**********************************************************
 *      Data Structure                                    *
 *      for Queue	                                  *
 **********************************************************/
typedef struct TagQNode
{
	void* dataNode;
	struct TagQNode* next;
}QNode;

typedef struct TagLinkedQueue
{
	QNode* front;
	QNode* rear;
	int count;
}LinkedQueue;



/**********************************************************
 *	Functions					  *
 *	for Queue					  *
 **********************************************************/

/* malloc and initialize */
void 	CR_CreateQueue( LinkedQueue** queue );

QNode* 	CR_CreateQNode( void* dataNode );

void 	CR_Enqueue( LinkedQueue* queue, QNode* newNode );

/* you must destroy dataNode after use it */
QNode*	CR_Dequeue( LinkedQueue* queue );

int	CR_IsEmpty( LinkedQueue* queue );


#endif
