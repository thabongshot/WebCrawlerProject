#include "CR_secondchunk.h"
#include "CR_getchunk.h"

#include <fcntl.h>
#include <unistd.h>

	
int main(int argc, char** argv)
{
        if(argc<2) exit(1);

        char* string = NULL;

        puts("");
        puts("*********************************************");
        printf("*\n*\n");
        puts("*  - CURL Test");
        puts("*  - iconv Test");
        puts("*  - print result");
        puts("*********************************************");
        puts("");
	
	string = CR_getChunkBodyMain(argv[1]);
	
	CR_TagRemover(string);
	int fd = open("chunkout", O_WRONLY | O_CREAT);

	write(fd, string, strlen(string));

	return 0;
}
