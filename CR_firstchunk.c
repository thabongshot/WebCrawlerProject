#include "CR_firstchunk.h"



/************************************************
 *	functions for data structure		*
 ************************************************/

extern void* CR_AllocSetNode(int setNum)
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
		newNode->keyword = NULL;
		newNode->url = NULL;
		newNode->title = NULL;
		newNode->next = NULL;
		return (void*)newNode;	
	} else {
		fprintf(stderr, "%s :: Variable setNum is not in scope.", __FUNCTION__);
		exit(1);
	}
}

extern void CR_DestroySetNode(int setNum, void* node)
{
	if( setNum == 1){

		Set1Node* workNode = (Set1Node*)node;
		FREE(workNode->rooturl);
		//FREE(workNode->next);
		FREE(workNode);

	} else if( setNum == 2 ){

		Set2Node* workNode = (Set2Node*)node;
		FREE(workNode->rooturl);
		FREE(workNode->chunk);
		//FREE(workNode->next);
		FREE(workNode);

	}else if (setNum == 3 ){

		Set3Node* workNode = (Set3Node*)node;
		FREE(workNode->rooturl);
		FREE(workNode->url);
		FREE(workNode->title);
		FREE(workNode->keyword);
		//FREE(workNode->next);
		FREE(workNode);
	
	}else {	
		fprintf(stderr, "%s :: Variable setNum is not in scope.", __FUNCTION__);
		exit(1);
	}
}

/************************************************
 *	functions for hyperlink filtering	*
 ************************************************/
extern void CR_FirstChunkBody(LinkedQueue* set2Queue, LinkedQueue* set3Queue)
{
	// data

	Set2Node* tmpSet2 = NULL;
	Set3Node* tmpSet3 = NULL;	
	QNode* tmpQNode = NULL;

	// code
	if( !CR_IsEmpty(set2Queue) ){
		printf("%s::",__FUNCTION__);
		puts("set 2 Queue is not empty");
	} else {
		printf("%s::",__FUNCTION__);
		puts("set 2 Queue is empty");
	}

	while( !CR_IsEmpty(set2Queue) ){
	// set2Queue's dataNode must be Set2Node's pointer
		tmpQNode = CR_Dequeue(set2Queue);
		tmpSet2 = (Set2Node*)(tmpQNode->dataNode);
		printf("%s::rooturl::%s\n",__FUNCTION__,tmpSet2->rooturl);
		printf("%s::strlen::%d\n", __FUNCTION__, strlen(tmpSet2->chunk) );

		CR_HyperlinkFilter( tmpSet2, set3Queue );
		if( !CR_IsEmpty(set3Queue) ){
			printf("%s:: set 3 Queue is not empty\n",__FUNCTION__);
		} else {
			printf("%s:: set 3 Queue is empty\n", __FUNCTION__);
		}
	}
}

static void CR_HyperlinkFilter( Set2Node* inputSet2, LinkedQueue* set3Queue)
{
	// data
	const int BUF_SIZE = 8192;
	char tagbuffer[BUF_SIZE];
	char strbuffer[BUF_SIZE];

	char* tag_start = NULL;
	char* tag_end = NULL;
	
	Set3Node* new3Node = NULL;
	
	// code
	CR_TagRemover(inputSet2->chunk);
	tag_start = inputSet2->chunk;
	
	while( (tag_start = (char*)strcasestr(tag_start, "<a ")) != NULL ){

		/* get one tag's addresses */
		if( (tag_end = (char*)strcasestr(tag_start, "/a>")) == NULL ) break;

		/* init memory */
                memset(tagbuffer, 0, BUF_SIZE);
                memset(strbuffer, 0, BUF_SIZE);

                new3Node = (Set3Node*)(CR_AllocSetNode(SET3) );
                new3Node->rooturl = CR_genstr(inputSet2->rooturl);		

		/* put tag into buffer */
		memcpy(tagbuffer, tag_start, (tag_end - tag_start + 4) );
		
		/* get URL string */
		CR_gethref(strbuffer, tagbuffer);
		if( strbuffer[0] != '\0' )
			new3Node->url = CR_genstr(strbuffer);
		memset(strbuffer, 0, BUF_SIZE);
		
		/* get title string */
		CR_gettitle(strbuffer, tagbuffer);
		if( strbuffer[0] != '\0' )
			new3Node->title = CR_genstr(strbuffer);
		memset(strbuffer, 0, BUF_SIZE);

		/* now new3Node has rooturl, url, title */
		if( new3Node->url != NULL && new3Node->title != NULL )
			CR_Enqueue(set3Queue, CR_CreateQNode((void*)new3Node) );
		else
			CR_DestroySetNode(SET3, new3Node);

		new3Node = NULL;

		/* for next tag */
		tag_start = tag_end;

		printf("");
		printf("");
	}
}

static void CR_gethref(char* dest, char* src)
{
	int i = 0;

	src = (char*)strcasestr( src, "href=" );

	if( src ){
		
		src = &src[5];
		while( *src != ' ' && *src != '>'){
                	if( *src != '\'' && *src != '\"' ){
                        	dest[i++] = *src;
                	}
                	src++;
        	}
	}

        dest[i] = '\0';
}

static void CR_gettitle(char* dest, char* src)
{
	while( *src != '>' && *src != '\0' ){
		src++;
	}

	while( *src != '<' && *src != '\0' ){
		*dest++ = *src++ ;
	}
	*dest = '\0';
}

static char* CR_genstr(char* buf)
{
	char* dest = NULL;
	dest = (char*)malloc(sizeof(char)*(strlen(buf)+1));
	strcpy(dest, buf);
	return dest;
}





/* because of static, it doensn't matter implement twice */

static void CR_TagRemover( char* chunk )
{
        const int BUF_SIZE = 4096 * 4;
        int i;
        char* buf;
        char* tag_start;
        char* tmp;


        buf = (char*)malloc(sizeof(char)*BUF_SIZE);

        do{
                if( *chunk == '<' ){

                        i=0;
                        memset(buf, 0, BUF_SIZE);

                        tag_start = chunk;
                        tmp = tag_start;
                        while( *tmp != ' ' && *tmp != '>' && *tmp != '\0' ){
                                buf[i++] = *tmp++;
                        }
                        buf[i] = '\0';

                        CR_makeblank(buf, tag_start);
                        tag_start = NULL;
                        tmp = NULL;
                }
        } while ( *chunk++ != '\0' );

        free(buf);
}

/* firstchunk's tagremover removes tag except href */
static void CR_makeblank(char* buf, char* str)
{
	char* tag_end = NULL;
        unsigned long int hash;

        hash = CR_stringhash(buf);
	switch(hash){

                // hash value of tag "<!--"
                case 441:
                        tag_end = (char*)strcasestr(str, "-->");
                        tag_end += 2;
                        while( str <= tag_end ) *str++ = ' ';

                        break;

                // hash value of tag "<script"
                case 3052:
                        tag_end = (char*)strcasestr(str, "/script>");
                        tag_end += 7;
                        while( str <= tag_end ) *str++ = ' ';

                        break;

                // hash value of tag "<style"
                case 2268:
                        tag_end = (char*)strcasestr(str, "/style>");
                        tag_end += 6;
                        while( str <= tag_end ) *str++ = ' ';

                        break;
		
		// has value of tag "<a " & "</a"
		// do nothing
		case 254: break;
		case 445: break;
                // hash value of tag "<div" & "/div>"
                // do nothing
                //case 1047: break;
                //case 1344: break;

                default:
                        while( *str != '>' ) *str++ = ' ';

                        *str = ' ' ;
                        break;
        }
}

static unsigned long int CR_stringhash(const unsigned char* str)
{
        int i;
        int len;
        unsigned long int hash = 0;

        len = strlen(str);

        for(i=0; i<len; i++){
                hash += ( (i+1) * str[i] );
        }

        return hash;
}
