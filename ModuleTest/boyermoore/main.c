#include "CR_BoyerMoore.h"
#include <string.h>


int main()
{
	char* text = "df;oiasjdfp08zxjcvp08zx9jcv0zx89cjv-[09zxc8jv-098zxjcv9zuivj0z89sd7ujv0z9sdvhj09zx8v7hj0zx897cvh0z8x7fvh90z8sdf7vh09z7sdvh09zs78dvh09z7f8vh09z7fhv0z9d7fbh09z7fbh09zvcbh0z9xc87hvb09zxc78hvb09zx78chvb09zd7fvhb0z9sdf7hvb09zsdf78hv0z9sd78hv089z7sdhv09zd7hv09zsd7hv098z7hfvb09zd78fhvb09z7fdhbv09z7fhb09z7dfhb09zdhthabongshotzx0c9v8u90-zx8vhu0zd9f8bvh09zxcv78bh0z97bh0z9xcvb78h0z-98xcbvh09vb78h08x97cvbh09x87chvnb09xc7hn097vbhn09xcv78hb09xc78vhb 07cvhb0zxcv 7b0z7xc vh0zx 78cvh0z 789xch0 9z7xhcv097 zxch09 7zxhcvb097zhvc0b97hzx0c9b78hz0vc8b76g0z8cv6bg086cg7vb089czxvg b0x789cgvb079z cvgxb079z xchv07zsdfh0v87zdhbf078vbzd 0f8b708zdfh v08z7d fhv087zd hfads;lfkjasd;o fijsa;dofi ja;oeifja ;weoifj ;awoeijf ;aoiwej f;oai wefj;oijzxcv09zujv[09zxcuv-[0z89xucv-8zxuc-v89zuxc-v 09zux-cv09uzx-cv08u9z-xc0v8u thabongshot90x8cvu p0zxc8vu-z09x8cvu-z089xuv-0zsd8uv-z0s9d8vu z98sdfvupz09s8v up09z8fud vp098f uvpz098uv p9zs8uv p90zs8du vp9z8sdu vp9z8sud vp89z uvp98udf-v089 ure-89vauwer-v98uar9v8uaps9rv8u";
	
	char* pat = "thabongshot";
	char* loc = NULL;

	puts("");
	puts("==================================================");
	puts(text);
	puts("==================================================");

	printf("INFO :: strlen = %d\n\n", strlen(text));

	while(1){
		loc = CR_BoyerMoore( text, pat );
		if(loc == NULL){
			break;
		} else {
			printf("found from ...\n");
			fwrite(loc, sizeof(char), strlen(pat)+10, stdout);
			puts("");
			loc++;
			text = loc;
		}
	}


	return 0;
}
