#include "CR_Qmaker.h"




void CR_CreateQueue( LinkedQueue** queue)
{
	(*queue)	= (LinkedQueue*)malloc(sizeof(LinkedQueue));
	(*queue)->front	= NULL;
	(*queue)->rear	= NULL;
	(*queue)->count	= 0;
}



QNode* CR_CreateQNode( void* dataNode )
{
	QNode* newNode = NULL;
	newNode = (QNode*)malloc( sizeof(QNode) );
	newNode->dataNode = dataNode;
	newNode->next = NULL;
	
	return newNode;
}



void CR_Enqueue( LinkedQueue* queue, QNode* newNode )
{
	if( queue->front == NULL ) {
		queue->front	= newNode;
		queue->rear	= newNode;
		queue->count++;
	} else {
		queue->rear->next = newNode;
		queue->rear = newNode;
		queue->count++;
	}
}


QNode* CR_Dequeue( LinkedQueue* queue )
{
	QNode* front = queue->front;
	
	if( queue->front->next == NULL ){
		queue->front	= NULL;
		queue->rear	= NULL;
	} else {
		queue->front = queue->front->next;
	}
	
	queue->count--;
	
	return front;
}


int CR_IsEmpty( LinkedQueue* queue )
{
	return ( queue->front == NULL ) ;
}


