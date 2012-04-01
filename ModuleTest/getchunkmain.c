#include <stdio.h>
#include <stdlib.h>

#include "CR_getchunk.h"




int main(int argc, char* argv)
{
	char in;
	char* result;
	
	result = CR_getChunkBodyMain("http://www.zdnet.co.kr");
	
	if(result!=NULL){
		puts("Get chunk done, press any key to print");
		in=getchar();
		printf("%s\n",result);
	} else {
		puts("String NULL in");
	}

	return 0;
}



