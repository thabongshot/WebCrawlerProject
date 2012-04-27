#include "CR_secondchunk.h"






/**********************************************************
 *      Functions for                                     *
 *      Second Chunk                                      *
 *********************************************************/

extern void CR_SecondChunkBody( /* put args here */ )
{

}



/**********************************************************
 *      Sentence make function                            *
 *********************************************************/
static void CR_sentencemaker(char* str)
{
	
}


/**********************************************************
 *      HTML Tag Remover Module                           *
 *      Set of 3 functions                                *
 *      Second Chunk                                      *
 *********************************************************/

extern void CR_TagRemover( char* chunk )
{
	const int BUF_SIZE = 4096 * 4;
	int i;
	int j;
	char* buf;
	char* tag_start;
	char* tmp;
	
int len = strlen(chunk);
j=0;
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
