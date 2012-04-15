#include "CR_Qmaker.h"
#include "CR_firstchunk.h"







int main(int argc, char** argv)
{
	int i=0;
	Set3Node* arr[3];
	Set3Node* tmp = NULL;
	LinkedQueue* queue = NULL;
	QNode* deqd = NULL;

	CR_CreateQueue( &queue );

	puts("");
	puts("Set3 Make & Destroy Test");
	puts("");


	puts("Make 3 Nodes ...");
	for(i=0; i<3; i++){
		arr[i] = (Set3Node*)CR_AllocSetNode(SET3);
	}
	puts("Success.");puts("");
	
	puts("Enqueue ...");
	for(i=0; i<3; i++){
		CR_Enqueue(queue, CR_CreateQNode( (void*)arr[i] ) );
	}
	puts("Success.");puts(""); 

	puts("Dequeue and Destroy ...");
	for(i=0; i<3; i++){
		deqd = CR_Dequeue(queue);
		tmp = (Set3Node*)(deqd->dataNode);
		printf("%x", tmp->rooturl );
		CR_DestroySetNode(SET3, tmp);
		FREE(deqd);
	}
	puts("");
	puts("Success");puts("");
	puts("Queue module test success");
	return 0;
}
