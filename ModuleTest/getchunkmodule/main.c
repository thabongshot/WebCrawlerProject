#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CR_getchunk.h"


// 오오미 손가락이 정화되는 키보드당 ㅎㅎ
// 재환이형 멋져요 ㅎㅎ

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
	puts("");
	
	int i,j;
//	for(i=0,j=0; i<20; j++){
//		putchar(string[j]);
//		if(string[j] == '\n') i++;
//	}
	puts("");

	return 0;
}
