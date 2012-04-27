#include <stdio.h>
#include <string.h>

static unsigned long int CR_stringhash(const char* str)
{
        int i;
        int len;
        unsigned long int hash = 0;

        len = strlen(str);

        for(i=0; i<len; i++){
                hash += ( (i+1) * str[i] );
        }

        return hash;
}


int main()
{
	printf("<!-- :: %lu \n", CR_stringhash("<!--"));
	printf("<script :: %lu \n", CR_stringhash("<script"));
	printf("<style :: %lu \n" , CR_stringhash("<style"));
	printf("<a :: %lu \n" , CR_stringhash("<a") );
	printf("</a :: %lu \n", CR_stringhash("</a") );
	printf("<div :: %lu \n", CR_stringhash("<div") );
	printf("/div :: %lu \n", CR_stringhash("</div") );

	return 0;
}

