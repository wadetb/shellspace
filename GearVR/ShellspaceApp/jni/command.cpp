#include "common.h"
#include "command.h"
#include "OvrApp.h"
#include "vncwidget.h"


#define CMD_BUFFER_SIZE		(64 * 1024)

#define MAX_CMD_LENGTH 		1024
#define MAX_CMD_ARGS 		128


struct SCmdGlob
{
	char 	buffer[CMD_BUFFER_SIZE];
	char 	bufferNull[1];
	uint 	bufferPos;

	char	argBuffer[MAX_CMD_LENGTH];
	char 	argNull[1];
	char	*args[MAX_CMD_ARGS];
	uint 	argCount;
};


static SCmdGlob s_cmdGlob;


sbool Cmd_CopyOneToArgBuffer( char **cmd )
{
	char 	*in;
	char 	ch;
	char 	*out;
	char 	*end;
	sbool 	inQuote;

	in = *cmd;
	out = s_cmdGlob.argBuffer;
	end = s_cmdGlob.argBuffer + MAX_CMD_LENGTH;

	inQuote = false;

	for ( ;; )
	{
		ch = *in;

		if ( ch == '"' )
			inQuote ^= 1;

		if ( !ch || (ch == ';' && !inQuote) )
			break;

		if ( out <= end )
			*out = ch;

		in++;
		out++;
	}

	while ( *in == ';' || *in == ' ' )
		in++;
		
	*cmd = in;

	if ( out > end )
	{
		LOG( "Command is too long; %d characters is the maximum.", MAX_CMD_LENGTH );
		return sfalse;
	}
	else
	{
		*out = 0;
		return strue;
	}
}


// Copies one (; separated) cmd into the global argBuffer, splitting and NULL-terminating 
// (optionally quoted) arguments.
// Returns number of bytes read from cmd
sbool Cmd_Parse( char **cmd )
{
	uint 	read;
	int 	argIndex;
	char 	*p;

	if ( !Cmd_CopyOneToArgBuffer( cmd ) )
		return sfalse;

	s_cmdGlob.argCount = 0;

	p = s_cmdGlob.argBuffer;
	for ( ;; )
	{
		while ( *p == ' ' )
			p++;

		if ( *p == 0 )
			break;

		if ( *p == '"' )
		{
			p++;
			s_cmdGlob.args[s_cmdGlob.argCount] = p;

			while ( *p && *p != '"' )
				p++;

			if ( *p != '"' )
			{
				LOG( "Failed to parse argument %d; missing end quote.", s_cmdGlob.argCount + 1 );
				return sfalse;
			}

			*p = 0;
			p++;
		}
		else
		{
			s_cmdGlob.args[s_cmdGlob.argCount] = p;

			while ( *p && *p != ' ' )
				p++;

			if ( *p )
			{
				*p = 0;
				p++;
			}
		}

		if ( s_cmdGlob.argCount >= MAX_CMD_ARGS )
		{
			LOG( "Too many arguments to parse; %d is the maximum.", MAX_CMD_ARGS );
			return sfalse;
		}
		s_cmdGlob.argCount++;
	}

	return strue;
}


static void Cmd_Clear()
{
	s_cmdGlob.argCount = 0;
}


uint Cmd_Argc()
{
	return s_cmdGlob.argCount;
}


const char *Cmd_Argv( uint argIndex )
{
	if ( argIndex >= s_cmdGlob.argCount )
		return "";

	return s_cmdGlob.args[argIndex];
}


void Cmd_Add( const char *cmd )
{
	char 	*buffer;
	size_t 	bufferLen;
	int 	written;

	buffer = &s_cmdGlob.buffer[s_cmdGlob.bufferPos];
	bufferLen = CMD_BUFFER_SIZE - s_cmdGlob.bufferPos;

	written = snprintf( buffer, bufferLen, "%s;", cmd );

	if ( written < 0 )
	{
		LOG( "Invalid command characters '%s'", cmd );
		return;
	}

	if ( written >= bufferLen )
	{
		LOG( "Command buffer too full for '%s'", cmd );
		return;
	}

	s_cmdGlob.bufferPos += written;
	s_cmdGlob.buffer[s_cmdGlob.bufferPos] = 0;
}


void Cmd_Frame()
{
	uint 	read;
	char 	*p;

	if ( !s_cmdGlob.bufferPos )
		return;

	Prof_Start( PROF_CMD );

	LOG( "> %s", s_cmdGlob.buffer );
	
	p = s_cmdGlob.buffer;
	while ( *p )
	{
		if ( !Cmd_Parse( &p ) )
		{
			Cmd_Clear();
			continue;
		}

		if ( !Cmd_Argc() )
			continue;

		// $$$ When there are multiple widgets, send to the active one here.
		if ( VNC_Command( vnc ) )
		{
			Cmd_Clear();
			continue;
		}

		if ( App_Command() )
		{
			Cmd_Clear();
			continue;
		}

		LOG( "Unrecognized command '%s'.", Cmd_Argv( 0 ) );
		Cmd_Clear();
	}

	s_cmdGlob.bufferPos = 0;
	s_cmdGlob.buffer[0] = 0;

	Prof_Stop( PROF_CMD );
}


