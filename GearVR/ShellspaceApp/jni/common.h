#ifndef __COMMON_H__
#define __COMMON_H__

#include <assert.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3.h>
#include <GlUtils.h>
#include <Log.h>
#include <OVR.h>

using namespace OVR;

#define USE_OVERLAY   			0
#define USE_TEMPORAL 			0
#define USE_SUPERSAMPLE_2X 		1
#define USE_SUPERSAMPLE_1_5X 	0
#define USE_SRGB 				1
#define USE_SPLIT_DRAW 			1

typedef unsigned int sbool;
#define strue  1
#define sfalse 0

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned int   uint;

#define assertindex( v, count ) assert( v < count )

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

inline int S_strcmp( const char * a, const char *b )
{
    return strcmp( a, b );
}

inline int S_stricmp( const char * a, const char *b )
{
    return strcasecmp( a, b );
}

#define FNV_32_PRIME ((uint)0x01000193)

inline uint S_FNV32( const char *str, uint hval )
{
    unsigned char *s = (unsigned char *)str;
    while ( *s ) 
    {
        hval *= FNV_32_PRIME;
        hval ^= (uint32_t)*s++;
    }
    return hval;
}

#define S_NULL_REF  0xffff

typedef ushort SRef;

struct SRefLink
{
    SRef     next;
    SRef     prev;
};

#include "../../../Common/shellspace.h"
#include "profile.h"
#include "vector.h"

#endif
