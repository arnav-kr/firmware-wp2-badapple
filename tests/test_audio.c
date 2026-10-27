#include "../app/src/audio/badapple_codec.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
int main(int argc, char **argv) {
 FILE *f=fopen(argc > 1 ? argv[1] : "assets/private/badapple_song_16k.mp3","rb");assert(f);fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);unsigned char *input=malloc((size_t)size);assert(input);assert(fread(input,1,(size_t)size,f)==(size_t)size);fclose(f);
 assert(!badapple_codec_open(input,(size_t)size));
 short pcm[576],seekref[576];size_t total=0,nonzero=0;int peak=0;
 for(;;){size_t n=badapple_codec_read(pcm,576);if(!n)break;if(total==16128)memcpy(seekref,pcm,sizeof(seekref));for(size_t i=0;i<n;i++){int v=pcm[i];if(v)nonzero++;if(v<0)v=-v;if(v>peak)peak=v;}total+=n;}
 printf("samples=%zu expected=%u nonzero=%zu peak=%d decoder_bytes=%zu\n",total,219*16000,nonzero,peak,(size_t)0);fflush(stdout);assert(total>=219*16000-16 && total<=219*16000+16);assert(nonzero>total/2 && peak>1000);
 assert(!badapple_codec_seek(16128));assert(badapple_codec_read(pcm,576)==576);assert(!memcmp(pcm,seekref,sizeof(pcm)));
 badapple_codec_close();free(input);puts("Full song decode, duration, mono16k, nonzero signal and sample-accurate seek passed.");return 0;
}
