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
#include "command.h"
#include "file.h"
#include "message.h"
#include "registry.h"
#include "OvrApp.h"
#include "thread.h"


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


sbool Cmd_IsSpace( char ch )
{
	if ( ch == ' ' || ch == '\t' )
		return true;

	return false;
}


sbool Cmd_IsDelim( char ch )
{
	if ( ch == ';' )
		return true;

	if ( ch == '\r' || ch == '\n' )
		return true;

	return false;
}


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

		if ( !ch || (Cmd_IsDelim( ch ) && !inQuote) )
			break;

		if ( out <= end )
			*out = ch;

		in++;
		out++;
	}

	while ( Cmd_IsDelim( *in ) || Cmd_IsSpace( *in ) )
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
	char 	*p;

	if ( !Cmd_CopyOneToArgBuffer( cmd ) )
		return sfalse;

	s_cmdGlob.argCount = 0;

	p = s_cmdGlob.argBuffer;
	for ( ;; )
	{
		while ( Cmd_IsSpace( *p ) )
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

			while ( *p && !Cmd_IsSpace( *p ) )
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


void Cmd_Add( const char *format, ... )
{
    va_list args;
	char 	*buffer;
	size_t 	bufferLen;
	uint 	written;

	Thread_ScopeLock lock( MUTEX_CMD );

	buffer = &s_cmdGlob.buffer[s_cmdGlob.bufferPos];
	bufferLen = CMD_BUFFER_SIZE - s_cmdGlob.bufferPos;

    va_start( args, format );
    written = vsnprintf( buffer, bufferLen - 1, format, args );
    va_end( args );

	if ( written < 0 )
	{
		LOG( "Invalid command characters." );
		return;
	}

	if ( written >= bufferLen )
	{
		LOG( "Command buffer too full." );
		return;
	}

    buffer[written] = ';';
    written++;

	s_cmdGlob.bufferPos += written;
	s_cmdGlob.buffer[s_cmdGlob.bufferPos] = 0;
}


void Cmd_AddToQueue( SMsgQueue *queue )
{
	char text[MSG_LIMIT];
	uint textPos;
	uint argIndex;

	assert( queue );
	assert( s_cmdGlob.argCount );

	textPos = 0;

	for ( argIndex = 0; argIndex < s_cmdGlob.argCount; argIndex++ )
		S_sprintfPos( text, MSG_LIMIT, &textPos, "\"%s\" ", s_cmdGlob.args[argIndex] );

	MsgQueue_Put( queue, text );
}


void Cmd_Frame()
{
	char 	*p;
	uint 	pluginIndex;
	SPlugin *plugin;
	uint 	widgetIndex;
	SWidget	*widget;

	if ( !s_cmdGlob.bufferPos )
		return;

	Prof_Start( PROF_CMD );
	
	p = s_cmdGlob.buffer;

	LOG( "> %s", s_cmdGlob.buffer );

	while ( *p )
	{
		if ( !Cmd_Parse( &p ) )
		{
			Cmd_Clear();
			goto next_cmd;
		}

		if ( !Cmd_Argc() )
			goto next_cmd;

		if ( App_Command() )
		{
			Cmd_Clear();
			goto next_cmd;
		}

		for ( pluginIndex = 0; pluginIndex < Registry_GetCount( PLUGIN_REGISTRY ); pluginIndex++ )
		{
			plugin = Registry_GetPlugin( Registry_RefForIndex( pluginIndex ) );
			assert( plugin );

			if ( S_strcmp( plugin->id, Cmd_Argv( 0 ) ) == 0 )
			{
				Cmd_AddToQueue( &plugin->msgQueue );
				Cmd_Clear();
				goto next_cmd;
			}
		}

		for ( widgetIndex = 0; widgetIndex < Registry_GetCount( WIDGET_REGISTRY ); widgetIndex++ )
		{
			widget = Registry_GetWidget( Registry_RefForIndex( widgetIndex ) );
			assert( widget );

			if ( S_strcmp( widget->id, Cmd_Argv( 0 ) ) == 0 )
			{
				Cmd_AddToQueue( &widget->msgQueue );
				Cmd_Clear();
				goto next_cmd;
			}
		}

		LOG( "Unrecognized command '%s'.", Cmd_Argv( 0 ) );
		Cmd_Clear();
next_cmd:;
	}

	s_cmdGlob.bufferPos = 0;
	s_cmdGlob.buffer[0] = 0;

	Prof_Stop( PROF_CMD );
}


void Cmd_AddFile( const char *fileName )
{
	char *text;

	LOG( "Executing %s", fileName );

	text = (char *)File_Read( fileName );
	if ( !text )
		return;

	Cmd_Add( text );

	free( text );
}
