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





/*
 * Main Body
 * Returns finished chunkbody string
 */

extern char* CR_getChunkBodyMain(char* uri);


/*
 * Getting Chunk Body Function
 * Based from chunkbody.c (from W3 Library Example)
 * Returns raw chunkbody string
 */

extern char* CR_getRawChunkBody(char* uri);


/*
 * Charset Converting Function
 * Converting EUC-KR to UTF-8 s.t can read Korean
 * Have to take stress test about this
 * Returns UTF-8 converted string
 */

extern char* CR_charsetToUTF8(char* string);
