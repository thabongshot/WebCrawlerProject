/*
 * Web Crawler
 * 
 * Functions of second chunk working set
 * 
 * >> get chunk string <-> save to the set
 * 	>> filtering contents text of chunkbody set
 * 	>> reallocate & substitute
 *
 */

typedef struct TagSet2 {
        char* url;              // secondly filtered hyperlink
        char* title;            // title of the hyperlink
        char* keyword;          // what the title contains
        char* contents;         // contents of the hyperlink
} Set3Node;


