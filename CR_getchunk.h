/*
 * Web Crawler
 *
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
 * Uses libcurl here.
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
