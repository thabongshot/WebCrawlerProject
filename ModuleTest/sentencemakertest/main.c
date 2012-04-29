#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CR_secondchunk.h"


int main(){
	char* string = "\n\nsen                tence1\nsente                                      nce1\n\nsentence2sentence2\nsentence2\nsentence2\n\n\n\n\n\n\n\n\n";

	char* res = NULL;
	
	res = CR_sentencemaker(string, "sent");

	puts("res:::::");
	puts(res);
	
	free(res);
	return 0;
}
