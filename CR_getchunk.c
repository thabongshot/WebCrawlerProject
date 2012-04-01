/*
 * Web Crawler
 *
 * Functions of Working Set of  Getting Chunk
 * Based on chunkbody.c ( W3 Library Example )
 * >> Get chunkbody
 * >> Convert into UTF-8
 * >> Save to memory
 *
 * Returns result string pointer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>

#include <curl/curl.h>

#include "CR_getchunk.h"


extern char* CR_getChunkBodyMain(char* uri)
{
    char* chunkRaw=NULL;
    char* chunkCnvd=NULL;
    
    chunkRaw = CR_getRawChunkBody(uri);
    chunkCnvd = CR_charsetToUTF8(chunkRaw);
    
    free(chunkRaw);
    chunkRaw = NULL;
    
    return chunkCnvd;
}



extern char* CR_getRawChunkBody(char* uri);
{
    
    return string;
}



/* charset converting */
#define ICONV_BYTES(a) ((a)*6+1)

/***** gotta solve string limit problem.  *****/
extern char* CR_charsetToUTF8(char* string)
{
	iconv_t to_utf;
	
	if( (to_utf = iconv_open("UTF-8","EUC-KR") ) == (iconv_t)-1 ){
		utf_error:
		fprintf(stderr, "%s :: Converting to UTF-8 failed. \n", __FUNC__);
		iconv_close(to_utf);
		return NULL;
	} else {
		size_t in_bytes = strlen(string), last_bytes;
		size_t out_bytes = ICONV_BYTES(in_bytes);

		char* out = (char*)malloc(ICONV_BYTES(in_BYTES);
		char* outp = out;

		// loop
		do{
			int n;
			
			last_bytes = in_bytes;
			n = iconv(to_utf, char(**)&str, &in_bytes, &outp, &out_bytes);
			if( n < 0 ){
				if(errno == EISLEQ || errno == EINVAL){
					str++;	//skip broken byte
					in_bytes--;
				} else {
					goto utf8_error;
				}
			}
		} while(in_bytes > 0 && (in_bytes < last_bytes) );

		iconv_close(to_utf);
		*outp = '\0';
		return out;
	}
}
