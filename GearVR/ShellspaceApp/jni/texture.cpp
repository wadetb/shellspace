#include "common.h"
#include "texture.h"
#include "registry.h"


void Texture_Resize( STexture *texture, uint width, uint height, SxTextureFormat format )
{
	uint 	texIndex;
	uint 	texWidth;
	uint 	texHeight;
	GLuint 	texId;

	Prof_Start( PROF_TEXTURE_RESIZE );

	assert( texture );

	texIndex = texture->updateIndex % BUFFER_COUNT;

	texWidth = S_NextPow2( width );
	texHeight = S_NextPow2( height );

	if ( texture->texId[texIndex] )
		glDeleteTextures( 1, &texture->texId[texIndex] );

	glGenTextures( 1, &texId );

	glBindTexture( GL_TEXTURE_2D, texId );

	// $$$ Use format argument.
#if USE_SRGB
	glTexStorage2D( GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, texWidth, texHeight );
#else // #if USE_SRGB
	glTexStorage2D( GL_TEXTURE_2D, 1, GL_RGBA8, texWidth, texHeight );
#endif // #else // #if USE_SRGB

	glBindTexture( GL_TEXTURE_2D, 0 );

	texture->texId[texIndex] = texId;
	texture->texWidth[texIndex] = texWidth;
	texture->texHeight[texIndex] = texHeight;

	Prof_Stop( PROF_TEXTURE_RESIZE );
}


void Texture_Update( STexture *texture, uint x, uint y, uint width, uint height, const void *data )
{
	int 	texIndex;

	Prof_Start( PROF_TEXTURE_UPDATE );

	assert( texture );

	texIndex = texture->updateIndex % BUFFER_COUNT;
	assert( texture->texId[texIndex] );

	glBindTexture( GL_TEXTURE_2D, texture->texId[texIndex] );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, width );

	glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data );

	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	Prof_Stop( PROF_TEXTURE_UPDATE );
}


void Texture_Present( STexture *texture )
{
	int 	texIndex;

	Prof_Start( PROF_TEXTURE_PRESENT );

	assert( texture );

	texture->drawIndex = texture->updateIndex;

	texIndex = texture->updateIndex % BUFFER_COUNT;
	assert( texture->texId[texIndex] );

	glBindTexture( GL_TEXTURE_2D, texture->texId[texIndex] );

	// $$$ Could do some mipping of just rectangles affected by updates.
	glGenerateMipmap( GL_TEXTURE_2D );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glBindTexture( GL_TEXTURE_2D, 0 );

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
