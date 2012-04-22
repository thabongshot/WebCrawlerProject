#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CR_firstchunk.h"
#include "CR_Qmaker.h"
#include "CR_getchunk.h"


int main( int argc, char** argv)
{
	if(argc<2) exit(1);

	char* string = NULL;
	Set2Node* tmp2node;
	Set3Node* tmp3node;
	
	QNode* tmpqnode;
	LinkedQueue* set2Queue;
	LinkedQueue* set3Queue;

	puts("");
        puts("*********************************************");
	printf("*\n*\n");
	puts("* 1. getchunk");
	puts("* 2. hyperlink filter");
	puts("* ");
	puts("*********************************************");
        puts("");
	
	CR_CreateQueue( &set2Queue );
	CR_CreateQueue( &set3Queue );
	
	tmp2node = (Set2Node*)CR_AllocSetNode(SET2);

	string = CR_getChunkBodyMain(argv[1]);
	
	tmp2node->rooturl = (char*)malloc(sizeof(char)*(strlen(argv[1])+1) );
	strcpy(tmp2node->rooturl, argv[1]);
	tmp2node->chunk = string;

	puts("enqueue");
	CR_Enqueue(set2Queue, CR_CreateQNode( (void*)tmp2node ));
	puts("done");
	

	puts("firstchunkbody");
	CR_FirstChunkBody(set2Queue, set3Queue);
	
	puts("Now we've got chunk.");
	
	while( !CR_IsEmpty(set3Queue) ){

		tmpqnode = CR_Dequeue(set3Queue);
		tmp3node = (Set3Node*)(tmpqnode->dataNode);

		puts("====================================");
		puts(tmp3node->rooturl);
		puts(tmp3node->url);
		puts("::::::::::::::::::::::::::::::::::::");
		puts(tmp3node->title);
		puts("");

		CR_DestroySetNode(SET3, tmp3node);
		FREE(tmpqnode);
	}

	return 0;
}
