#include "CR_firstchunk.h"



/************************************************
 *	functions for data structure		*
 ************************************************/

void* CR_AllocSetNode(int setNum)
{

	if( setNum == 1 ){

		Set1Node* newNode = NULL;
		newNode = (Set1Node*)malloc(sizeof(Set1Node));
		newNode->rooturl = NULL;
		newNode->next = NULL;
		return (void*)newNode;

	} else if( setNum == 2 ){

		Set2Node* newNode = NULL;
		newNode = (Set2Node*)malloc(sizeof(Set2Node));
		newNode->rooturl = NULL;
		newNode->chunk = NULL;
		newNode->next = NULL;
		return (void*)newNode;

	} else if ( setNum == 3 ){		

		Set3Node* newNode = NULL;
		newNode = (Set3Node*)malloc(sizeof(Set3Node));
		newNode->rooturl = NULL;
		newNode->url = NULL;
		newNode->title = NULL;
		newNode->keyword = NULL;
		newNode->next = NULL;
		return (void*)newNode;	
	} else {
		fprintf(stderr, "%s :: Variable setNum is not in scope.", __FUNCTION__);
		exit(1);
	}
}

void CR_DestroySetNode(int setNum, void* node)
{
	if( setNum == 1){

		Set1Node* workNode = (Set1Node*)node;
		FREE(workNode->rooturl);
		FREE(workNode->next);
		FREE(workNode);

	} else if( setNum == 2 ){

		Set2Node* workNode = (Set2Node*)node;
		FREE(workNode->rooturl);
		FREE(workNode->chunk);
		FREE(workNode->next);
		FREE(workNode);

	}else if (setNum == 3 ){

		Set3Node* workNode = (Set3Node*)node;
		FREE(workNode->rooturl);
		FREE(workNode->url);
		FREE(workNode->title);
		FREE(workNode->keyword);
		FREE(workNode->next);
		FREE(workNode);
	
	}else {	
		fprintf(stderr, "%s :: Variable setNum is not in scope.", __FUNCTION__);
		exit(1);
	}
}
