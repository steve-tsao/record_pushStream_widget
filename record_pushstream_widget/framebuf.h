#ifndef FRAMEBUF_H
#define FRAMEBUF_H

#include "muxthread.h"


typedef struct VFrameBuf
{
public:
    unsigned char yChannel[WIDTH*HEIGHT]{0};
    unsigned char uChannel[WIDTH*HEIGHT/4]{0};
    unsigned char vChannel[WIDTH*HEIGHT/4]{0};
}VFrameBuf;

typedef struct AFrameBuf
{
public:
    unsigned char* paFrame=NULL;
    int pcmSize;
    int seek = 0;
}AFrameBuf;

#endif // FRAMEBUF_H
