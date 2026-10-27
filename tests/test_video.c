#define __aligned(x)
#include "../app/src/video/player.h"
#include <assert.h>
#include <stdio.h>
static unsigned char gray[VIDEO_WIDTH*VIDEO_HEIGHT];
static void verify(FILE *f,unsigned frame) {
 assert(video_decode_to(frame));assert(!fseek(f,(long)frame*sizeof(gray),SEEK_SET));assert(fread(gray,1,sizeof(gray),f)==sizeof(gray));
 for(unsigned i=0;i<sizeof(gray);i++) assert(video_pixels[i]==(gray[i]>127?65535:0));
}
int main(int argc, char **argv) {FILE *f=fopen(argc > 1 ? argv[1] : "assets/private/source_192x144_15fps.gray","rb");assert(f);for(unsigned i=0;i<3285;i++)verify(f,i);unsigned frames[]={2000,4,2500,100,3284,0,29,30,31,12};for(unsigned i=0;i<sizeof(frames)/sizeof(frames[0]);i++)verify(f,frames[i]);fclose(f);puts("All 3285 frames and backward/forward seeks match source threshold pixels.");return 0;}
