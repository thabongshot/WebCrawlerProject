/*
 * Web Crawler
 *
 * Functions of Working Set of  Getting Chunk
 * uses libcurl
 * >> Get chunkbody
 * >> Convert into UTF-8
 * >> Save to memory
 *
 * Returns result string pointer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>

#include <curl/curl.h>

#include "CR_getchunk.h"


/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

struct MemoryStruct {
	char* memory;
	size_t size;
};

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;
	
	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if( mem->memory == NULL ){
		/* out of memory */
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy( &(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	
	return realsize;
}

extern char* CR_getRawChunkBody(const char* url)
{
	CURL* curl_handle;
	
	struct MemoryStruct chunk; 
	
	chunk.memory = malloc(1);
	chunk.size = 0;


	curl_global_init(CURL_GLOBAL_ALL);
	
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)(&chunk) );
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	curl_easy_perform(curl_handle);
	
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	
	return chunk.memory;
}


/***************************************************************************
 * 									   *
 ***************************************************************************/

extern char* CR_getChunkBodyMain(const char* url)
{
	int i;
	char* buf;
	char* tmp;
	char* chunkRaw=NULL;
	char* result=NULL;
	
	/* main URL get part */
	chunkRaw = CR_getRawChunkBody(url);
	
	/* header HTTP/1.1 repond check */
	CR_okCheck( &chunkRaw );

	/* header charset check */
	result = CR_charsetCheck( chunkRaw );

	return result;

}

static void CR_okCheck( char** str )
{
	int i;
	char buf_resp[30];
	char buf_url[100];
	char* tmp = NULL;
	
	memset(buf_resp,0,30);
	memset(buf_url,0,100);	

	/* find HTTP/1.1 response result */
	if( (tmp=strstr(*str, "HTTP/1.1")) != NULL){
		i=0;
		while( *tmp++ != ' ' ){}
		while( *tmp != ' ' ){
			buf_resp[i++] = *tmp++;
		}
		buf_resp[i] = '\0';
	}

printf("%s::LINE%d::buf[ %s ]\n",__FUNCTION__,__LINE__,buf_resp);

	/* reponse condition */
	if( strcmp(buf_resp, "200") == 0 ){
		// if HTTP/1.1 200 OK , then do nothing

	} else if( strcmp(buf_resp, "301") == 0 ){
		// if HTTP/1.1 301 Moved Permanently then
		// find => Location: "url"
		// return CR_getChunkBodyMain(url)
		if( (tmp=strstr(*str, "Location:")) != NULL ){
			i=0;
			while( *tmp++ != ' ' ){}
			while( *tmp != ' ' && *tmp != '\0' ){
				buf_url[i++] = *tmp++;
			}
			buf_url[i] = '\0';
		}
printf("%s::LINE%d::buf[ %s ]\n",__FUNCTION__,__LINE__,buf_url);
		free(*str); 
		*str=NULL;
		*str = CR_getChunkBodyMain(buf_url);

	} else {
		// if HTTP/1.1 'else' then nulify string
		memset(*str, 0, strlen(*str) );
	}
}

static char* CR_charsetCheck( char* str )
{
	int i;
	char* tmp;
	char* convd;
	char buf_chset[100];

	if( (tmp=strstr(str,"charset=")) != NULL ){
		i=0;
		tmp += 8;
		while( *tmp != '\n' && *tmp != ' '){
			buf_chset[i++] = *tmp++;
		}
		buf_chset[i] = '\0';
	}
printf("%s::LINE%d::buf[ %s ]\n",__FUNCTION__,__LINE__,buf_chset);

	if( strcasestr(buf_chset, "euc") != NULL &&
		strcasestr(buf_chset, "kr") != NULL ){
			convd = NULL;
			convd = CR_charsetToUTF8(str);
			return convd;
	} 

printf("%s::LINE%d::buf[ %s ]\n",__FUNCTION__,__LINE__,buf_chset);
	return str;
}


/***************************************************************************
 *       Charset converting function. iconv()                              *
 ***************************************************************************/

#define ICONV_BYTES(a) ((a)*6+1)
/***** gotta solve string limit problem.  *****/

static char* CR_charsetToUTF8(char* string)
{
	iconv_t to_utf;
	
	if( (to_utf = iconv_open("UTF-8","EUC-KR") ) == (iconv_t)-1 ){
		utf8_error:
		fprintf(stderr, "%s :: Converting to UTF-8 failed. \n", __FUNCTION__);
		iconv_close(to_utf);
		return NULL;
	} else {
		size_t in_bytes = strlen(string), last_bytes;
		size_t out_bytes = ICONV_BYTES(in_bytes);

		char* out = (char*)malloc(ICONV_BYTES(in_bytes) );
		char* outp = out;

		// loop
		do{
			int n;
			
			last_bytes = in_bytes;
			n = iconv(to_utf, (char**)&string, &in_bytes, &outp, &out_bytes);
			if( n < 0 ){
				if(errno == EILSEQ || errno == EINVAL){
					string++;	//skip broken byte
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

