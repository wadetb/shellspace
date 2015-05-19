#include "common.h"
#include "file.h"
#include "command.h"
#include "OvrApp.h"

#include <PackageFiles.h>
#include <AppLocal.h>
#include <VrApi/JniUtils.h>

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define PATH_LIMIT 			8
#define MAX_PATH 			256


struct SFileGlobals
{
	uint 	pathCount;
	char 	*paths[PATH_LIMIT];

	char 	userDir[MAX_PATH];
	char 	cacheDir[MAX_PATH];

	sbool 	httpEnabled;
	char 	httpHost[MAX_PATH];
	int  	httpPort;
	char 	httpRoot[MAX_PATH];
};


SFileGlobals s_file;


void File_GetAndroidPaths()
{
	AppLocal *appLocal = (AppLocal *)( g_app->app );
	jclass vrLibClass = appLocal->VrLibClass;

	jmethodID internalCacheDirID = ovr_GetStaticMethodID( g_jni, vrLibClass, "getInternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
	if ( internalCacheDirID )
	{
		jobject returnString = g_jni->CallStaticObjectMethod( vrLibClass, internalCacheDirID, g_activityObject );
		snprintf( s_file.cacheDir, MAX_PATH, "%s", g_jni->GetStringUTFChars( (jstring)returnString, JNI_FALSE ) );
		g_jni->DeleteLocalRef( returnString );

		S_RemoveTrailingSlash( s_file.cacheDir );
	}

	strcpy( s_file.userDir, "/storage/extSdCard/Oculus/Shellspace/" );
}


void File_Init()
{
	File_GetAndroidPaths();

	// For absolute path support.
	File_AddPath( "" );

	// User file overrides.
	if ( !S_strempty( s_file.userDir ) )
		File_AddPath( s_file.userDir );

	// Cached downloaded files.
	if ( !S_strempty( s_file.cacheDir ) )
	{
		S_Log( "File_Init: Cache directory is %s", s_file.cacheDir );
		File_AddPath( s_file.cacheDir );
	}

	strcpy( s_file.httpHost, "wadeb.com" ); // $$$ shellspace.org!
	s_file.httpPort = 80;
	strcpy( s_file.httpRoot, "/shellspace/" );

	s_file.httpEnabled = !S_strempty( s_file.cacheDir ) ;
}


void File_Shutdown()
{
	uint pathIter;

	for ( pathIter = 0; pathIter < s_file.pathCount; pathIter++ )
		free( s_file.paths[pathIter] ); 
}


void File_AddPath( const char *path )
{
	char 	*newPath;

	if ( s_file.pathCount == PATH_LIMIT )
	{
		S_Log( "Exceeded the limit of %d search paths; not adding %s.", PATH_LIMIT, path );
		return;
	}

	newPath = strdup( path );
	assert( newPath );

	S_RemoveTrailingSlash( newPath );

	S_Log( "Added '%s' to the search path.", newPath );

	s_file.paths[s_file.pathCount] = newPath;
	s_file.pathCount++;
}


sbool File_Exists( const char *fullPath )
{
	FILE 	*in;

	in = fopen( fullPath, "rb" );
	if ( in )
	{
		fclose( in );
		return strue;
	}

	return sfalse;
}


sbool File_Find( const char *fileName, char fullPath[MAX_PATH] )
{
	uint 	pathIter;
	char 	*path;

	for ( pathIter = 0; pathIter < s_file.pathCount; pathIter++ )
	{
		path = s_file.paths[pathIter];

		snprintf( fullPath, MAX_PATH, "%s/%s", path, fileName );

		if ( File_Exists( fullPath ) )
			return strue;		
	}

	return sfalse;
}


void File_DownloadToCache( const char *fileName )
{
	int 				result;
	char 				httpPort[8];
	struct addrinfo 	hints;
	struct addrinfo 	*addr;
	struct addrinfo 	*a;
	int 				sockFd;
	int 				bytesRead;
    char 				buffer[1024];
    uint 				bufferPos;
    sbool 				inBody;
    char 				*header;
    uint 				headerSize;
    char 				*separator;
    int 				httpStatus;
    char 				fullPath[MAX_PATH];
    FILE 				*out;

    bzero( &hints, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	snprintf( httpPort, sizeof( httpPort ), "%d", s_file.httpPort );

	result = getaddrinfo( s_file.httpHost, httpPort, &hints, &addr );
	if ( result )
	{
    	S_Log( "File_DownloadToCache: Unable to resolve %s: %s", s_file.httpHost, gai_strerror( result ) );
    	return;
	}

	for ( a = addr; a; a = a->ai_next )
	{
        sockFd = socket( a->ai_family, a->ai_socktype, a->ai_protocol );
        if ( sockFd == -1 )
        {
        	S_Log( "File_DownloadToCache: Unable to create socket: %s", strerror( errno ) );
            continue;
        }

        if ( connect( sockFd, a->ai_addr, a->ai_addrlen ) != -1 )
            break;

    	S_Log( "File_DownloadToCache: Unable to connect to %s: %s", s_file.httpHost, strerror( errno ) );

        close( sockFd );
    }

    freeaddrinfo( addr ); 

    if ( a == NULL ) 
    	return;

    bufferPos = 0;
    S_sprintfPos( buffer, sizeof( buffer ), &bufferPos, "GET %s%s HTTP/1.1\r\n", s_file.httpRoot, fileName );
    S_sprintfPos( buffer, sizeof( buffer ), &bufferPos, "Host: %s\r\n", s_file.httpHost );
    S_sprintfPos( buffer, sizeof( buffer ), &bufferPos, "Connection: close\r\n" );
    S_sprintfPos( buffer, sizeof( buffer ), &bufferPos, "\r\n" );

    send( sockFd, buffer, strlen( buffer ), 0 );

    inBody = sfalse;
    strcpy( fullPath, "" );
    out = NULL;

    header = NULL;
    headerSize = 0;

    for ( ;; )
    {
        bzero( buffer, sizeof( buffer ) );

        bytesRead = recv( sockFd, buffer, sizeof( buffer ), 0 ); 
        if ( bytesRead == 0 )
        	break;

        if ( !inBody )
        {
        	header = (char *)realloc( header, headerSize + bytesRead );
        	
        	memcpy( header + headerSize, buffer, bytesRead );

			separator = strstr( header, "\r\n\r\n" );
			if ( separator )
			{
				*separator = 0;

				if ( sscanf( header, "HTTP/1.%*d %d", &httpStatus ) != 1 )
				{
					S_Log( "File_DownloadToCache: Invalid HTTP header: %s.", header );
					break;
				}

				snprintf( fullPath, MAX_PATH, "%s/%s", s_file.cacheDir, fileName );

				if ( httpStatus == 404 )
				{
					unlink( fullPath );
					break;
				}

				if ( httpStatus != 200 )
				{
					S_Log( "File_DownloadToCache: Unexpected HTTP status %d: %s", httpStatus, header );
					unlink( fullPath );
					break;
				}

				out = fopen( fullPath, "wb" );
				if ( !out )
				{
					S_Log( "File_DownloadToCache: Unable to open cache file %s for write", fullPath );
					unlink( fullPath );
					break;
				}

				bufferPos = separator + 4 - (header + headerSize);
				bytesRead -= bufferPos;

				memmove( buffer, buffer + bufferPos, bytesRead );

				inBody = strue;
			}
			else
			{
	        	headerSize += bytesRead;
			}
        }

        if ( inBody )
        	fwrite( buffer, 1, bytesRead, out );
    }

    if ( inBody )
    {
    	fclose( out );
		S_Log( "File_DownloadToCache: Saved %s", fullPath );
	}

	free( header );

    close( sockFd );
}


byte *File_ReadFromPackage( const char *fileName, uint *bytesRead )
{
	int 	length;
	void 	*buffer;
	char 	packagePath[MAX_PATH];

	if ( bytesRead )
		*bytesRead = 0;

	snprintf( packagePath, MAX_PATH, "assets/%s", fileName );

	OVR::ovr_ReadFileFromApplicationPackage( packagePath, length, buffer );

	if ( !buffer )
		return NULL;

	buffer = realloc( buffer, length + 1 );
	( (byte *)buffer )[length] = 0;

	if ( bytesRead )
		*bytesRead = length;

	return (byte *)buffer;
}


byte *File_Read( const char *fileName, uint *bytesRead )
{
	char 	fullPath[MAX_PATH];
	FILE 	*in;
	size_t 	length;
	byte 	*buffer;
	size_t 	read;
	uint 	pathIter;

	if ( bytesRead )
		*bytesRead = 0;

	if ( s_file.httpEnabled )
		File_DownloadToCache( fileName );

	if ( !File_Find( fileName, fullPath ) )
	{
		buffer = File_ReadFromPackage( fileName, bytesRead );
		if ( buffer )
			return buffer;

		S_Log( "Failed to find %s in the package or in the following search paths:", fileName );

		for ( pathIter = 0; pathIter < s_file.pathCount; pathIter++ )
			S_Log( "\t\"%s\"", s_file.paths[pathIter] );

		return NULL;
	}

	S_Log( "File_Read: %s", fullPath );

	in = fopen( fullPath, "rb" );
	if ( !in )
	{
		S_Log( "Failed to load %s", fullPath );
		return NULL;
	}

	fseek( in, 0, SEEK_END );
	length = ftell( in );
	fseek( in, 0, SEEK_SET );

	buffer = (byte *)malloc( length + 1 );
	if ( !buffer )
	{
		S_Log( "Failed to allocate %d bytes of memory for %s", length + 1, fullPath );
		fclose( in );
		return NULL;
	}

	read = fread( buffer, 1, length, in );
	if ( read != length )
	{
		S_Log( "Failed to read %d bytes from %s", length, fullPath );
		fclose( in );
		free( buffer );
		return NULL;
	}
	
	buffer[length] = 0;

	if ( bytesRead )
		*bytesRead = length;

	fclose( in );

	return buffer;
}


sbool File_Command()
{
	if ( strcasecmp( Cmd_Argv( 0 ), "file" ) == 0 )
	{
		if ( strcasecmp( Cmd_Argv( 1 ), "http" ) == 0 )
		{
			if ( strcasecmp( Cmd_Argv( 2 ), "enabled" ) == 0 )
			{
				if ( Cmd_Argc() != 3 )
				{
					S_Log( "Usage: file http enable" );
					return strue;
				}

				s_file.httpEnabled = strue;

				return strue;
			}

			if ( strcasecmp( Cmd_Argv( 2 ), "disable" ) == 0 )
			{
				if ( Cmd_Argc() != 3 )
				{
					S_Log( "Usage: file http disable" );
					return strue;
				}

				s_file.httpEnabled = sfalse;

				return strue;
			}

			if ( strcasecmp( Cmd_Argv( 2 ), "host" ) == 0 )
			{
				if ( Cmd_Argc() != 4 )
				{
					S_Log( "Usage: file http host <host>" );
					return strue;
				}

				S_strcpy( s_file.httpHost, MAX_PATH, Cmd_Argv( 3 ) );

				return strue;
			}

			if ( strcasecmp( Cmd_Argv( 2 ), "port" ) == 0 )
			{
				if ( Cmd_Argc() != 4 )
				{
					S_Log( "Usage: file http port <port>" );
					return strue;
				}

				s_file.httpPort = atoi( Cmd_Argv( 3 ) );

				return strue;
			}

			if ( strcasecmp( Cmd_Argv( 2 ), "root" ) == 0 )
			{
				if ( Cmd_Argc() != 4 )
				{
					S_Log( "Usage: file http root <host>" );
					return strue;
				}

				S_strcpy( s_file.httpRoot, MAX_PATH, Cmd_Argv( 3 ) );

				return strue;
			}
		}
	}

	return sfalse;
}
