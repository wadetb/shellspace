#ifndef __COMMON_H__
#define __COMMON_H__

#include <OVR.h>
#include <Log.h>

using namespace OVR;

#define USE_OVERLAY   			0
#define USE_TEMPORAL 			0
#define USE_SUPERSAMPLE_2X 		0
#define USE_SUPERSAMPLE_1_5X 	0
#define USE_SRGB 				1
#define USE_SPLIT_DRAW 			1

typedef unsigned int sbool;
#define strue  1
#define sfalse 0

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned int   uint;

inline unsigned int S_NextPow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline int S_Max( int a, int b )
{
	return a > b ? a : b;
}

inline int S_Min( int a, int b )
{
	return a < b ? a : b;
}

inline float S_Maxf( float a, float b )
{
	return a > b ? a : b;
}

inline float S_Minf( float a, float b )
{
	return a < b ? a : b;
}

#include "profile.h"

#endif
