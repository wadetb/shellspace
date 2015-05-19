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
#include "common.h"
#include "texture.h"
#include "registry.h"

#include <core/SkCanvas.h>
#include <GlUtils.h>
#include <turbojpeg.h>


GLuint Texture_GetGLFormat( SxTextureFormat format )
{
	switch ( format )
	{
	case SxTextureFormat_R8G8B8X8:
		return GL_RGBA8;
		
	case SxTextureFormat_R8G8B8X8_SRGB:
		return GL_SRGB8_ALPHA8;

	case SxTextureFormat_R8G8B8A8:
		return GL_RGBA8;
		
	case SxTextureFormat_R8G8B8A8_SRGB:
		return GL_SRGB8_ALPHA8;

	default:
		assert( false );
		return 0;
	}
}


uint Texture_GetDataSize( uint width, uint height, SxTextureFormat format )
{
	switch ( format )
	{
	case SxTextureFormat_R8G8B8X8:
	case SxTextureFormat_R8G8B8X8_SRGB:
	case SxTextureFormat_R8G8B8A8:
	case SxTextureFormat_R8G8B8A8_SRGB:
		return width * height * 4;

	default:
		assert( false );
		return 0;
	}
}


void Texture_Resize( STexture *texture, uint width, uint height, SxTextureFormat format )
{
	uint 	index;
	uint 	texWidth;
	uint 	texHeight;
	GLuint 	texId;
	GLuint 	glFormat;

	Prof_Start( PROF_TEXTURE_RESIZE );

	OVR::GL_CheckErrors( "before Texture_Resize" );

	assert( texture );

	// S_Log( "Texture_Resize: %d by %d", width, height );

	index = texture->updateIndex % BUFFER_COUNT;

	texId = texture->texId[index];

	texWidth = S_NextPow2( width );
	texHeight = S_NextPow2( height );

	if ( texId )
		glDeleteTextures( 1, &texId );

	glGenTextures( 1, &texId );

	glBindTexture( GL_TEXTURE_2D, texId );

	glFormat = Texture_GetGLFormat( format );
	
	glTexStorage2D( GL_TEXTURE_2D, 1, glFormat, texWidth, texHeight );

	glBindTexture( GL_TEXTURE_2D, 0 );

	texture->texId[index] = texId;
	texture->texWidth[index] = texWidth;
	texture->texHeight[index] = texHeight;

	OVR::GL_CheckErrors( "after Texture_Resize" );

	Prof_Stop( PROF_TEXTURE_RESIZE );
}


void Texture_Update( STexture *texture, uint x, uint y, uint width, uint height, const void *data )
{
	int 	index;
	// float 	startMs;
	// float 	endMs;
	// float 	costMs;

	Prof_Start( PROF_TEXTURE_UPDATE );

	OVR::GL_CheckErrors( "before Texture_Update" );

	assert( texture );

	index = texture->updateIndex % BUFFER_COUNT;
	assert( texture->texId[index] );

	assert( x + width <= texture->texWidth[index] );
	assert( y + height <= texture->texHeight[index] );

	// startMs = 1000.0 * clock() / CLOCKS_PER_SEC;

	glBindTexture( GL_TEXTURE_2D, texture->texId[index] );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, width );

	glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data );

	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// endMs = 1000.0 * clock() / CLOCKS_PER_SEC;
	// costMs = endMs - startMs;

	// S_Log( "Texture_Update: (%d,%d) %d by %d (%d pixels) cost %f ms %fms/100kpx", x, y, width, height, 
	// 	width*height, costMs, 100000.0 * costMs / (width*height));

	OVR::GL_CheckErrors( "after Texture_Update" );

	Prof_Stop( PROF_TEXTURE_UPDATE );
}


void Texture_Present( STexture *texture )
{
	int 	index;

	Prof_Start( PROF_TEXTURE_PRESENT );

	OVR::GL_CheckErrors( "before Texture_Present" );

	assert( texture );

	texture->drawIndex = texture->updateIndex;

	index = texture->updateIndex % BUFFER_COUNT;
	assert( texture->texId[index] );

	glBindTexture( GL_TEXTURE_2D, texture->texId[index] );

	// $$$ Could do some mipping of just rectangles affected by updates.
	glGenerateMipmap( GL_TEXTURE_2D );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBindTexture( GL_TEXTURE_2D, 0 );

	OVR::GL_CheckErrors( "after Texture_Present" );

	Prof_Stop( PROF_TEXTURE_PRESENT );
}


void Texture_Decommit( STexture *texture )
{
	int 	index;

	texture->drawIndex = 0;
	texture->updateIndex = 0;

	for ( index = 0; index < BUFFER_COUNT; index++ )
	{
		if ( texture->texId[index] )
		{
			glDeleteTextures( 1, &texture->texId[index] );

			texture->texId[index] = 0;
			texture->texWidth[index] = 0;
			texture->texHeight[index] = 0;
		}
	}
}


sbool Texture_DecompressJpeg( const void *jpegData, uint jpegSize, uint *widthOut, uint *heightOut, SxTextureFormat *formatOut, void **dataOut )
{
	tjhandle 	tjh;
	int 		tjerr;
	int 		width;
	int 		height;
	void 		*data;

	tjh = tjInitDecompress();
	if ( !tjh )
	{
		S_Log( "Texture_DecompressJpeg: %s", tjGetErrorStr() );
		return sfalse;
	}

	tjerr = tjDecompressHeader( tjh, (byte *)jpegData, jpegSize, &width, &height );
	if ( tjerr < 0 )
	{
		S_Log( "Texture_DecompressJpeg: %s", tjGetErrorStr() );
		tjDestroy( tjh );
		return sfalse;
	}

	data = malloc( width * height * 4 );
	if ( !data )
	{
		S_Log( "Texture_DecompressJpeg: Unable to allocate %d bytes", width * height * 4 );
		tjDestroy( tjh );
		return sfalse;
	}

	tjerr = tjDecompress2( tjh, (byte *)jpegData, jpegSize, (byte *)data, width, width * 4, height, TJPF_RGBX, 0 );
	if ( tjerr < 0 )
	{
		free( data );
		S_Log( "Texture_DecompressJpeg: %s", tjGetErrorStr() );
		tjDestroy( tjh );
		return sfalse;
	}

	*widthOut = width;
	*heightOut = height;
	*formatOut = SxTextureFormat_R8G8B8X8;
	*dataOut = data;

	tjDestroy( tjh );

	return strue;
}


sbool Texture_LoadSvg( const char *svg, uint *widthOut, uint *heightOut, SxTextureFormat *formatOut, void **dataOut )
{
	SkPaint 	paint;
	SkBitmap 	bitmap;

	// $$$ This doesn't actually load a SVG :-)

	bitmap.setInfo( SkImageInfo::MakeN32( 400, 300, kOpaque_SkAlphaType ) );
	bitmap.allocPixels();

	paint.setAntiAlias(true);
	paint.setTextSize(30);

	SkCanvas 	canvas( bitmap );
    canvas.drawColor( SK_ColorWHITE );

    paint.setFlags(SkPaint::kAntiAlias_Flag);
    paint.setColor(SK_ColorBLACK);
    paint.setTextSize(SkIntToScalar(20));

    static const char message[] = "Hello World!";

    canvas.save();
    canvas.translate(SkIntToScalar(50), SkIntToScalar(100));
    canvas.drawText(message, strlen(message), SkIntToScalar(0), SkIntToScalar(0), paint);
    canvas.restore();

    void *data = malloc( 400 * 300 * 4 );

    canvas.readPixels( SkImageInfo::MakeN32( 400, 300, kOpaque_SkAlphaType ), data, 400 * 4, 0, 0 );

    *widthOut = 400;
    *heightOut = 300;
    *formatOut = SxTextureFormat_R8G8B8X8;
    *dataOut = data;

    return strue;
}


sbool Texture_LoadBitmap( SkBitmap *bitmap, uint *widthOut, uint *heightOut, SxTextureFormat *formatOut, void **dataOut )
{
	uint 			width;
	uint			height;
	SxTextureFormat format;
	void 			*data;

	width = bitmap->width();
	height = bitmap->height();

	if ( bitmap->alphaType() == kOpaque_SkAlphaType )
		format = SxTextureFormat_R8G8B8X8;
	else
		format = SxTextureFormat_R8G8B8A8;

    data = malloc( width * height * 4 );
    if ( !data )
    	return sfalse;

    bitmap->readPixels( SkImageInfo::MakeN32( width, height, bitmap->alphaType() ), data, width * 4, 0, 0 );

    *widthOut = width;
    *heightOut = height;
    *formatOut = format;
    *dataOut = data;

    return strue;
}

