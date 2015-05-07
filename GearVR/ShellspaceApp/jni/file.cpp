#include "common.h"
#include "file.h"


byte *File_Read( const char *fileName )
{
	FILE 	*in;
	size_t 	length;
	byte 	*buffer;
	size_t 	read;

	in = fopen( fileName, "r" );
	if ( !in )
	{
		LOG( "Failed to load %s", fileName );
		return NULL;
	}

	fseek( in, 0, SEEK_END );
	length = ftell( in );
	fseek( in, 0, SEEK_SET );

	buffer = (byte *)malloc( length + 1 );
	if ( !buffer )
	{
		LOG( "Failed to allocate %d bytes of memory for %s", length + 1, fileName );
		fclose( in );
		return NULL;
	}

	read = fread( buffer, 1, length, in );
	if ( read != length )
	{
		LOG( "Failed to read %d bytes from %s", length, fileName );
		fclose( in );
		free( buffer );
		return NULL;
	}
	
	buffer[length] = 0;

	fclose( in );

	return buffer;
}
