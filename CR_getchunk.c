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

#include "WWWLib.h"
#include "WWWHTTP.h"
#include "WWWInit.h"

#include "CR_getchunk.h"


PRIVATE int printer (const char * fmt, va_list pArgs)
{
    return (vfprintf(stdout, fmt, pArgs));
}

PRIVATE int tracer (const char * fmt, va_list pArgs)
{
    return (vfprintf(stderr, fmt, pArgs));
}

PRIVATE int terminate_handler (HTRequest * request, HTResponse * response,
                               void * param, int status) 
{
    /* Check for status */
    /* HTPrint("Load resulted in status %d\n", status); */
	
	/* we're not handling other requests */
	HTEventList_stopLoop ();
    
	/* stop here */
    return HT_ERROR;
}


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
    HTRequest * request = HTRequest_new();
    HTList * converters = HTList_new();		/* List of converters */
    HTList * encodings = HTList_new();		/* List of encoders */
    HTChunk * chunk = NULL;
    // char * url = argc==2 ? argv[1] : NULL;
    
    /* Initialize libwww core */
    HTLibInit("Get Chunk Module", "1.0");
    
    /* Gotta set up our own traces */
    HTPrint_setCallback(printer);
    HTTrace_setCallback(tracer);
    
    /* Turn on TRACE so we can see what is going on */
#if 0
    HTSetTraceMessageMask("sop");
#endif
    
    /* On windows we must always set up the eventloop */
#ifdef WWW_WIN_ASYNC
    HTEventInit();
#endif
    
    /* Register the default set of transport protocols */
    HTTransportInit();
    
    /* Register the default set of protocol modules */
    HTProtocolInit();
    
    /* Register the default set of BEFORE and AFTER callback functions */
    HTNetInit();
    
    /* Register the default set of converters */
    HTConverterInit(converters);
    HTFormat_setConversion(converters);
    
    /* Register the default set of transfer encoders and decoders */
    HTTransferEncoderInit(encodings);
    HTFormat_setTransferCoding(encodings);
    
    /* Register the default set of MIME header parsers */
    HTMIMEInit();
    
    /* Add our own filter to handle termination */
    HTNet_addAfter(terminate_handler, NULL, NULL, HT_ALL, HT_FILTER_LAST);
    
    /* Set up the request and pass it to the Library */
    HTRequest_setOutputFormat(request, WWW_SOURCE);
    HTRequest_setPreemptive(request, YES);
    
    char* string = NULL;
    if (url) {
        char * cwd = HTGetCurrentDirectoryURL();
        char * absolute_url = HTParse(url, cwd, PARSE_ALL);
        HTAnchor * anchor = HTAnchor_findAddress(absolute_url);
        chunk = HTLoadAnchorToChunk(anchor, request);
        HT_FREE(absolute_url);
        HT_FREE(cwd);
        
        /* If chunk != NULL then we have the data */
        if (chunk) {
            //char * string;
            /* wait until the request is over */
            HTEventList_loop (request);
            string = HTChunk_toCString(chunk);
            //HTPrint("%s", string ? string : "no text");
            //HT_FREE(string);
            
        }
    } else {
        HTPrint("No uri string in.\n");
    }
    
    /* Clean up the request */
    HTRequest_delete(request);
    HTFormat_deleteAll();
    
    /* On windows, shut down eventloop as well */
#ifdef WWW_WIN_ASYNC
    HTEventTerminate();
#endif
    
    /* Terminate the Library */
    HTLibTerminate();
    
    return string;
}


/* charset converting */
#define ICONV_BYTES(a) ((a)*6+1)

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
