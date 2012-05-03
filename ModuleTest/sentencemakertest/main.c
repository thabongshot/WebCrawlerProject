#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CR_secondchunk.h"
#include "CR_getchunk.h"

int main( int argc, char** argv){

	char* string;

	char* res = NULL;

if(argc<2) exit(1);

	string = (char*)malloc(sizeof(char)*1024*1024);

	string = CR_getChunkBodyMain(argv[1]);

	CR_TagRemover(string);
	
	res = CR_sentencemaker(string, "애플");

	puts("res:::::");
	puts(res);
	
	free(res);
	return 0;
}
