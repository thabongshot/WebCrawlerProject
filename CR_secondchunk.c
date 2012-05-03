#include "CR_secondchunk.h"




/**********************************************************
 *      Functions for                                     *
 *      Second Chunk                                      *
 *********************************************************/

extern void CR_SecondChunkBody( Set4Node* inNode )
{
	char* keyword = NULL;
	char* str_chunk = NULL;
	char* tag_removed = NULL;
	char* result = NULL;

	keyword = inNode->keyword;
	str_chunk = inNode->contents;

	CR_TagRemover(str_chunk);
	result = CR_sentencemaker(str_chunk, keyword);

	free(str_chunk);
	str_chunk = NULL;
	inNode->contents = result;
}



/**********************************************************
 *      Sentence make function                            *
 *********************************************************/
extern char* CR_sentencemaker( char* str, char* keyword)
{
	int i;
	int j;
	
	int len_max;	
	int len_cand;
	int len_tmp;

	char* buf_cand = NULL;
	char* buf_tmp  = NULL;

	len_max = strlen(str);
	buf_cand = (char*)malloc(sizeof(char)*len_max);
	buf_tmp  = (char*)malloc(sizeof(char)*len_max);
	memset(buf_cand, 0, len_max);
	memset(buf_tmp, 0, len_max);


	len_cand = 0;
	len_tmp  = 0;
	i=0;
	j=0;
	while( *str != '\0' ){

		if( *str == ' ' || *str == '\n' ){
			i++;
		} else {
			i=0;
		}

		if( i > 1 ){
			buf_tmp[j++] = '\0';
			buf_tmp[j++] = '\0';
			len_tmp = CR_strlen(buf_tmp);
			//printf("%d::len\n",len_tmp);
			if( len_cand < len_tmp && strcasestr(buf_tmp, keyword) != NULL ){
				buf_tmp[j++] = '\0';
				memset(buf_cand, 0, len_max);
				strcpy(buf_cand, buf_tmp);
				len_cand = len_tmp;
			}
			memset(buf_tmp, 0, len_max);
			i=0;
			j=0;
		}

		buf_tmp[j++] = *str++;
	}

	free(buf_tmp);
	return buf_cand;
}

static int CR_strlen(const char* str)
{
	int i=0;
	int cnt = 0;
	while( str[i] != '\0' ){
		if( str[i] != '\n' && str[i] != ' ' ){
			cnt++;
		}
		i++;
	}
	
	return cnt;
}


/**********************************************************
 *      HTML Tag Remover Module                           *
 *      Set of 3 functions                                *
 *      Second Chunk                                      *
 *********************************************************/

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
