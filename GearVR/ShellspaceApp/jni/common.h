/*
    Shellspace - One tiny step towards the VR Desktop Operating System
    Copyright (C) 2015  Wade Brainerd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <assert.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3.h>
#include <GlUtils.h>
#include <Log.h>
#include <OVR.h>
// #include "coffeecatch/coffeecatch.h"
// #include "coffeecatch/coffeejni.h"

#define USE_OVERLAY   			0
#define USE_TEMPORAL 			0
#define USE_SUPERSAMPLE_2X 		1
#define USE_SUPERSAMPLE_1_5X 	0
#define USE_SRGB 				1
#define USE_SPLIT_DRAW 			0

typedef unsigned int sbool;
#define strue  1
#define sfalse 0

typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned int   uint;

#define KB      1024
#define MB      (1024 * KB)

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

inline int S_Clamp( int a, int mn, int mx )
{
    if ( a < mn )
        return mn;
    if ( a > mx )
        return mx;
    return a;
}

inline float S_Maxf( float a, float b )
{
	return a > b ? a : b;
}

inline float S_Minf( float a, float b )
{
	return a < b ? a : b;
}

inline sbool S_strempty( const char *s )
{
    return s[0] == '\0';
}

inline sbool S_streq( const char *a, const char *b )
{
    return strcmp( a, b ) == 0;
}

inline int S_strcmp( const char *a, const char *b )
{
    return strcmp( a, b );
}

inline int S_stricmp( const char *a, const char *b )
{
    return strcasecmp( a, b );
}

inline int S_sprintfPos( char *buffer, uint bufferLen, uint *pos, const char *format, ... )
{
    uint    currentPos;
    int     written;
    va_list args;

    currentPos = *pos;

    va_start( args, format );
    written = vsnprintf( buffer + currentPos, bufferLen - currentPos, format, args );
    va_end( args );

    if ( written >= 0 )
        *pos = currentPos + written;

    return written;
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

#define S_PI 3.141592653589793238462643383f

#define S_COS_ONE_TENTH_DEGREE 0.9999984769132877f

inline void S_SinCos( float angle, float *s, float *c )
{
    *s = sinf( angle );
    *c = cosf( angle );
}

inline float S_degToRad( float a )
{
    return a * (S_PI / 180.0f);
}

inline float S_radToDeg( float a )
{
    return a * (180.0f / S_PI);
}

#define S_NULL_REF     0xffff
#define S_DELETED_REF  0xfffe

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
