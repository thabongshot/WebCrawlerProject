#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "CR_HTMLmaker.h"
#include "CR_Qmaker.h"
#include "CR_getchunk.h"
#include "CR_firstchunk.h"
#include "CR_secondchunk.h"
#include "CR_HTMLmaker.h"

/* these are for visitlist and wordlist **
 *
 */
	const int LIST_SIZE = 1024;
	const int URL_SIZE  = 4096;
	const int WORD_SIZE = 512;

	int rear_visit = 0;
	int rear_word = 0;

	char** wordlist;
	char** visitlist;

	void CR_AddVisitlist(char* url);
	int CR_IsVisited(char* url);
	void CR_DestroyVisitlist();

	void CR_AddWordlist(char* word);
	int CR_TitleHasWord(char* title);
/*
 *
 ****************************************/

/* make url absolute path */
char* CR_ValidURL(char* url, char* rooturl);

/* alloc set 4 node */
Set4Node* CR_AllocSet4Node(Set3Node* in);


int main(int argc, char** argv)
{
	int len;

	FILE* keyword;
	FILE* url;

	char buf_url[URL_SIZE];
	char buf_word[WORD_SIZE];

	char* chunk;

	LinkedQueue* set1q;
	LinkedQueue* set2q;
	LinkedQueue* set3q;

	QNode* newNode;
	QNode* deqd;

	Set1Node* s1node;
	Set2Node* s2node;
	Set3Node* s3node;
	Set4Node* s4node;
	Set4Node* s4list;
	Set4Node* s4tmp;


	/* 1. file read and add to urlist, wordlist */
	keyword = fopen("./keyword.text","r");
	url  = fopen("./url.text","r");
	if( !keyword || !url ){
		perror("fopen ");
		exit(1);
	}	
	
	wordlist = (char**)malloc(sizeof(char*)*LIST_SIZE);
	visitlist= (char**)malloc(sizeof(char*)*LIST_SIZE);

	memset(buf_url, 0, URL_SIZE);
	memset(buf_word, 0, WORD_SIZE);

	/* 1-1. read urlist, enqueue */
puts("Queueing URLs...");
	CR_CreateQueue(&set1q);
	while( fgets(buf_url, URL_SIZE, url) != NULL ){
		len = strlen(buf_url);
		buf_url[ len-1 ] = '\0';
		printf("[Queue] : %s ...", buf_url);
		s1node = CR_AllocSetNode(SET1);
		s1node->rooturl = (char*)malloc(sizeof(char)*len +1);
		strcpy(s1node->rooturl, buf_url);
		CR_CreateQNode( (void*)s1node );
		CR_Enqueue(set1q, CR_CreateQNode((void*)s1node) );
		memset(buf_url, 0, URL_SIZE);
		printf("enqueued\n");
	}
puts("Queueing completed.");	
puts("");
	
	/* 1-2. read wordlist, add list */
puts("Making keyword list...");
	while( fgets(buf_word, WORD_SIZE, keyword) != NULL ){
		len = strlen(buf_word);
		buf_word[ len-1 ] = '\0';
		printf("[Wlist] : %s ...", buf_word);
		CR_AddWordlist( buf_word );
		memset(buf_word, 0, WORD_SIZE);
		printf("added\n");
	}
puts("Keyword list making completed.");
puts("");

	
	/* 2. getchunk from set 1 queue */
printf("Dequeue set1 node and get chunk from url...");
	CR_CreateQueue(&set2q);
	while( !CR_IsEmpty(set1q) ){
		/* A. Dequeue */
		deqd = CR_Dequeue(set1q);
		s1node = (Set1Node*)deqd->dataNode;

		/* B. Make Set 2 Node */
		s2node = CR_AllocSetNode(SET2);
		s2node->rooturl = (char*)malloc(sizeof(char)*strlen(s1node->rooturl)+1);
		strcpy(s2node->rooturl, s1node->rooturl);
		
		/* C. Get chunk and put Set 2 Node */
		s2node->chunk = CR_getChunkBodyMain(s2node->rooturl);
		
		/* D. add to visitlist */
		CR_AddVisitlist(s2node->rooturl);

		/* E. Enqueue Set 2 Node */
		CR_Enqueue(set2q, CR_CreateQNode((void*)s2node) );
		
		CR_DestroySetNode( SET1, (void*)s1node );
		free(deqd);
		deqd = NULL;
		s1node = NULL;
	}
puts("done\n");


	/* 3. hyperlink parser, title and url filtering */
printf("Hyperlink Parsing ...");
	CR_CreateQueue(&set3q);
	CR_FirstChunkBody(set2q, set3q);
printf(" done\n");

	/* 4. title and url filtering, secondchunk */
printf("Choose 2nd getchunk candidates ... ");
	s4list = NULL;
	while( !CR_IsEmpty(set3q) ){
		/* A. Dequeue Set 3 Node */
		deqd = CR_Dequeue(set3q);
		s3node = (Set3Node*)deqd->dataNode;

		/* B. Title and URL filtering */
		if( !CR_IsVisited(s3node->url) && 
			CR_TitleHasWord(s3node->title) ){
				// do getchunk
				s4node = CR_AllocSet4Node(s3node);

				if(s4list == NULL){
					s4list = s4node;
				} else {
					s4tmp = s4list;
					while(s4tmp->next != NULL){
						s4tmp = s4tmp->next;
						printf("");printf("");
					}
					s4tmp->next = s4node;
				}	
				
				s4node->contents = CR_getChunkBodyMain(s4node->url);
				CR_AddVisitlist(s4node->url);
		}
		CR_DestroySetNode(SET3, s3node);
	}

printf("done \n");

	/* 5. sentence making */
puts("making sentences ...");
	s4tmp = s4list;
	while( s4tmp->next != NULL ){
		CR_SecondChunkBody(s4tmp);
		s4tmp = s4tmp->next;
		printf("");
		printf("");
	}
printf(" ... sentence has made.\n");
		
puts(":::::::::::::::::::::::::::::::::::::::::::::::::::");
puts(":::::::::::::::::::::::::::::::::::::::::::::::::::");
puts("Now we have to make HTML file out.");

	return 0;
}



/* functions implementing */
Set4Node* CR_AllocSet4Node(Set3Node* in)
{
	Set4Node* newNode = (Set4Node*)malloc(sizeof(Set4Node));
	newNode->rooturl = (char*)malloc(sizeof(char)*strlen(in->rooturl)+1);
	newNode->url	 = (char*)malloc(sizeof(char)*strlen(in->url)+1);
	newNode->title	 = (char*)malloc(sizeof(char)*strlen(in->title)+1);

	strcpy(newNode->rooturl, in->rooturl);
	strcpy(newNode->url, in->url);
	strcpy(newNode->title, in->title);
	
	newNode->next = NULL;
	return newNode;
}

void CR_AddVisitlist(char* url)
{
	
	int len;
	
	if( rear_visit > LIST_SIZE ) {
		puts("it is trying to put too many urls");
		exit(1);
	}
	
	len = strlen(url)+5;
	visitlist[rear_visit] = (char*)malloc(sizeof(char)*len);
	memset(visitlist[rear_visit],0,len);
	strcpy(visitlist[rear_visit], url);
	
	rear_visit++;
}

int CR_IsVisited(char* url)
{
	int i;
	for(i=0; i<rear_visit; i++){
		if( strcmp(visitlist[i], url) == 0){
			return 1;
		}
	}
	return 0;
}

void CR_DestroyVisitlist()
{
	int i;
	for(i=0; i<rear_visit; i++){
		free(visitlist[i]);
	}
}


char* CR_ValidURL(char* url, char* rooturl)
{
	int newlen;
	int rtlen;

	char* newurl = NULL;

	rtlen = strlen(rooturl);
	newlen= strlen(url)+30;
	
	newurl = (char*)malloc(sizeof(char)*newlen);

	strcpy(newurl, rooturl);
	strcpy( &(newurl[rtlen]), url );

	free(url);
	return newurl;
}

void CR_AddWordlist(char* word)
{
	int len;

	len = strlen(word)+1;
	wordlist[rear_word] = (char*)malloc(sizeof(char)*len);
	memset(wordlist[rear_word], 0, len);
	strcpy(wordlist[rear_word], word);
	rear_word++;
}

int CR_TitleHasWord(char* title)
{
	int i;

	for(i=0; i<rear_word; i++){
		if( strcasestr(title, wordlist[i]) != NULL ){
			return 1;
		}
	}

	return 0;
}
