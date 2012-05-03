#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
	void CR_TitleKeyword(Set3Node* node);
/*
 *
 ****************************************/

/* make url absolute path */
char* CR_ValidURL(char* url, char* rooturl);

/* alloc set 4 node */
Set4Node* CR_AllocSet4Node(Set3Node* in);

/* put header into file */
void CR_putheader(FILE* out);
void CR_putcontents(FILE* out, Set4Node* s4list);
int mystricmp(char* s1, char* s2);
char UpChar(char ch);


int main(int argc, char** argv)
{
	int len;
	int cnt;
	int num;

	FILE* keyword;
	FILE* url;
	FILE* htmlout;

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
puts("");
fprintf(stderr, "Queueing URLs...\n");
	CR_CreateQueue(&set1q);
	while( fgets(buf_url, URL_SIZE, url) != NULL ){
		len = strlen(buf_url);
		buf_url[ len-1 ] = '\0';
		fprintf(stderr, "[Queue] : %s ...", buf_url);
		s1node = CR_AllocSetNode(SET1);
		s1node->rooturl = (char*)malloc(sizeof(char)*len +1);
		strcpy(s1node->rooturl, buf_url);
		CR_CreateQNode( (void*)s1node );
		CR_Enqueue(set1q, CR_CreateQNode((void*)s1node) );
		memset(buf_url, 0, URL_SIZE);
		fprintf(stderr, "enqueued\n");
	}
fprintf(stderr, "Queueing completed.\n");	
fprintf(stderr, "\n");
	
	/* 1-2. read wordlist, add list */
fprintf(stderr, "Making keyword list...\n");
	while( fgets(buf_word, WORD_SIZE, keyword) != NULL ){
		len = strlen(buf_word);
		buf_word[ len-1 ] = '\0';
		fprintf(stderr, "[Wlist] : %s ...", buf_word);
		CR_AddWordlist( buf_word );
		memset(buf_word, 0, WORD_SIZE);
		fprintf(stderr, "added\n");
	}
fprintf(stderr, "Keyword list making completed.\n");
fprintf(stderr, "\n");




	
	/* 2. getchunk from set 1 queue */
	cnt = 0;
	num = set1q->count;
	CR_CreateQueue(&set2q);
	while( !CR_IsEmpty(set1q) ){
		cnt++;
		fprintf(stderr, "\rGet seed data [%3d] of [%3d]    ", cnt, num);
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
fprintf(stderr, "done\n");





	/* 3. hyperlink parser, title and url filtering */
fprintf(stderr, "Hyperlink Parsing ...");
	CR_CreateQueue(&set3q);
	CR_FirstChunkBody(set2q, set3q);
fprintf(stderr, " done\n");




	/* 4. title and url filtering, secondchunk */
	cnt = 0;
	num = set3q->count;
	s4list = NULL;
fprintf(stderr, "\n");
	while( !CR_IsEmpty(set3q) ){
		
		/* A. Dequeue Set 3 Node */
		deqd = CR_Dequeue(set3q);
		s3node = (Set3Node*)deqd->dataNode;
		CR_TitleKeyword(s3node);
		
		/* B. Title and URL filtering */
		if( !CR_IsVisited(s3node->url) && s3node->keyword != NULL ){

				/* C. Add to visit list */
				CR_AddVisitlist(s3node->url);
				
				/* D. Make s4node from s3node */
				s4node = CR_AllocSet4Node(s3node);

				/* E. Add s4node to set 4 node list (result) */
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

				/* F. Make sure that URL is valid */
				if(s4node->url[0] == '/'){
					 s4node->url = CR_ValidURL(s4node->url, s4node->rooturl);
				}
				
				cnt++;
				fprintf(stderr, "\rGet article data [%3d] of [%3d]    ", cnt, num);
				
				/* G. Get chunk from url */
				s4node->contents = CR_getChunkBodyMain(s4node->url);
		}
		CR_DestroySetNode(SET3, s3node);
	}
fprintf(stderr, "done \n");




	/* 5. sentence making */
fprintf(stderr, "making sentences ... ");
	s4tmp = s4list;
	while( s4tmp->next != NULL ){
		CR_SecondChunkBody(s4tmp);
		s4tmp = s4tmp->next;
		printf("");
		printf("");
	}
fprintf(stderr, "done\n");
		
	
	/* 6. Write result into HTML file */
	htmlout = fopen("./result.html","w");
	if( htmlout == NULL){
		perror("[ fopen ]");
		exit(1);
	}

	CR_putheader(htmlout);
	CR_putcontents(htmlout, s4list);

	return 0;
}



/* functions implementing */
Set4Node* CR_AllocSet4Node(Set3Node* in)
{
	Set4Node* newNode = (Set4Node*)malloc(sizeof(Set4Node));
	newNode->rooturl = (char*)malloc(sizeof(char)*strlen(in->rooturl)+1);
	newNode->url	 = (char*)malloc(sizeof(char)*strlen(in->url)+1);
	newNode->title	 = (char*)malloc(sizeof(char)*strlen(in->title)+1);
	newNode->keyword = (char*)malloc(sizeof(char)*strlen(in->keyword)+1);

	strcpy(newNode->rooturl, in->rooturl);
	strcpy(newNode->url, in->url);
	strcpy(newNode->title, in->title);
	strcpy(newNode->keyword, in->keyword);
	
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

void CR_TitleKeyword(Set3Node* node)
{
	int i;
	int len;

	node->keyword = NULL;
	for(i=0; i<rear_word; i++){
		if( strcasestr(node->title, wordlist[i]) != NULL ){

			/* found keyword matches, alloc memory*/
			len = strlen(wordlist[i])+1;
			node->keyword = (char*)malloc(sizeof(char)*len);
			memset(node->keyword, 0, len);
			
			/* copy , end loop */
			strcpy(node->keyword, wordlist[i]);

			break;
		}
	}
}


void CR_putheader(FILE* out)
{
	fprintf(out, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
	fprintf(out, "<head>\n");
	fprintf(out, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	fprintf(out, "<title>Crawler :: Result File</title>\n");
	fprintf(out, "</head>\n\n");
}

void CR_putcontents(FILE* out, Set4Node* s4list)
{
	int i;
	int j;
	int flag;
	struct tm *t;
	time_t now;
	char day[7][5] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
	char* chtmp = NULL;
	Set4Node* s4tmp = NULL;

	now=time(NULL);
	t=localtime(&now);

	fprintf(out, "<body>\n\n");
	// time
	fprintf(out, "Time : %d.%d.%d [%s]  %d:%d \n\n <br><br>\n", 
		t->tm_year+1990, t->tm_mon+1, t->tm_mday, day[t->tm_wday],
		t->tm_hour, t->tm_min);
	
	for(i=0; i<rear_word; i++){
		
		// wordlist[i]
		flag = 0;
		s4tmp = s4list;
		while( s4tmp->next != NULL){
			if( mystricmp(wordlist[i], s4tmp->keyword) == 0){
				flag=1;
			}
			s4tmp = s4tmp->next;
		}
	
		if(flag){
			fprintf(out, "<h1>[ %s ]</h1><hr>\n", wordlist[i]);
			s4tmp = s4list;
			while( s4tmp->next != NULL){
				if( mystricmp(s4tmp->keyword, wordlist[i]) == 0 ){
		
					fprintf(out, "<b>%s :: [ <a href=\"%s\">Link</a> ]</b><br>\n",
							s4tmp->title, s4tmp->url);

					// shortening content text
					if( strlen(s4tmp->contents) > 100 ){
						fwrite(s4tmp->contents, sizeof(char), 100, out);
					} else {
						fprintf(out, "%s", (s4tmp->contents)?s4tmp->contents:"No Text");
					}
					fprintf(out, "<br><br>\n");
				}
				s4tmp = s4tmp->next;
			}
		}
	}
	fprintf(out, "\n\n</body>\n");
}


/* mystricmp */
int mystricmp(char* s1, char* s2)
{
	while( !(*s1==0 || *s2==0) && 
		( (*s1==*s2) || UpChar(*s1) == UpChar(*s2) )  ){
			s1++;
			s2++;
	}
	return UpChar(*s1) - UpChar(*s2);
}
char UpChar(char ch)
{
	if( (ch>='a') && (ch<='z') ){
		return ch-32;
	} else {
		return ch;
	}
}
