#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CR_getchunk.h"




int main(int argc, char** argv)
{
	if(argc<2) exit(1);

	char* string = NULL;

	puts("");
	puts("*********************************************");
	printf("*\n*\n");
	puts("*  - CURL Test");
	puts("*  - iconv Test");
	puts("*  - 10 web URLs");
	puts("*  - print result");
	puts("*********************************************");
	puts("");

	//CR_getChunkBodyMain(url);
	string = CR_getChunkBodyMain(argv[1]);
	printf("%s",string);
	puts("");
	printf("string from CR_getChunkBodyMain :: %d\n", strlen(string) );

	return 0;
}
