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
#include "message.h"
#include "thread.h"


uint Msg_Argc( const SMsg *msg )
{
	return msg->argCount;
}


sbool Msg_Empty( const SMsg *msg )
{
	return msg->argCount == 0;
}


const char *Msg_Argv( const SMsg *msg, uint argIndex )
{
	assert( msg );

	if ( argIndex >= msg->argCount )
		return "";

	return msg->args[argIndex];
}


void Msg_Shift( SMsg *msg, uint count )
{
	uint argIndex;

	if ( count >= msg->argCount )
	{
		msg->argCount = 0;
		return;
	}

	for ( argIndex = count; argIndex < msg->argCount; argIndex++ )
		msg->args[argIndex - count] = msg->args[argIndex];

	msg->argCount -= count;
}


void Msg_Unshift( SMsg *msg, const char *text )
{
	uint argIndex;

	if ( msg->argCount == MSG_ARG_LIMIT )
		return;

	for ( argIndex = msg->argCount; argIndex > 0; argIndex-- )
		msg->args[argIndex] = msg->args[argIndex - 1];

	msg->args[0] = msg->buffer + msg->bufferUsed;

	S_sprintfPos( msg->buffer, MSG_LIMIT, &msg->bufferUsed, "%s", text );
	msg->bufferUsed++;

	msg->argCount++;
}


void Msg_Remove( SMsg *msg, uint index )
{
	uint argIndex;

	if ( index >= msg->argCount )
		return;

	for ( argIndex = index; argIndex < msg->argCount; argIndex++ )
		msg->args[argIndex] = msg->args[argIndex + 1];

	msg->argCount--;
}


void Msg_Format( const SMsg *msg, char *result, uint resultLen )
{
	uint textPos;
	uint argIndex;

	assert( msg );
	assert( result );

	textPos = 0;

	for ( argIndex = 0; argIndex < msg->argCount; argIndex++ )
		S_sprintfPos( result, resultLen, &textPos, "\"%s\" ", msg->args[argIndex] );
}


sbool Msg_IsArgv( const SMsg *msg, uint argIndex, const char *value )
{
	assert( msg );

	if ( argIndex >= msg->argCount )
		return sfalse;

	return S_strcmp( msg->args[argIndex], value ) == 0;
}


sbool MsgCmd_Dispatch( const SMsg *msg, const SMsgCmd *cmdList, void *context )
{
	const SMsgCmd 	*cmd;
	const char 		*inputCmdName;

	assert( msg );

	if ( !msg->argCount )
		return sfalse;

	inputCmdName = msg->args[0];

	for ( cmd = cmdList; cmd->name; cmd++ )
	{
		if ( S_strcmp( inputCmdName, cmd->name ) == 0 )
		{
			assert( cmd->fn );
			cmd->fn( msg, context );

			return strue;
		}
	}

	return sfalse;
}


sbool Msg_SetFloatCmd( const SMsg *msg, float *value, float mn, float mx )
{
	const char 	*arg;
	float 	newValue;

	assert( msg );
	assert( value );

	if ( Msg_Argc( msg ) < 2 )
	{
		LOG( "Float value expected" );
		return sfalse;
	}

	arg = Msg_Argv( msg, 1 );

	newValue = atof( arg );

	if ( arg[0] == '-' || arg[0] == '+' )
		newValue += *value;

	if ( newValue < mn )
	{
		LOG( "Value is below the minimum %f", mn );
		return sfalse;
	}	

	if ( newValue > mx )
	{
		LOG( "Value is above the maximum %f", mx );
		return sfalse;
	}

	*value = newValue;

	return strue;
}


sbool Msg_SetIntCmd( const SMsg *msg, int *value, int mn, int mx )
{
	const char 	*arg;
	int 		newValue;

	assert( msg );
	assert( value );

	if ( Msg_Argc( msg ) < 2 )
	{
		LOG( "Integer value expected" );
		return sfalse;
	}

	arg = Msg_Argv( msg, 1 );

	newValue = atoi( arg );

	if ( arg[0] == '-' || arg[0] == '+' )
		newValue += *value;

	if ( newValue < mn )
	{
		LOG( "Value is below the minimum %i", mn );
		return sfalse;
	}	

	if ( newValue > mx )
	{
		LOG( "Value is above the maximum %i", mx );
		return sfalse;
	}

	*value = newValue;

	return strue;
}


sbool Msg_SetBoolCmd( const SMsg *msg, sbool *value )
{
	const char 	*arg;

	assert( msg );
	assert( value );

	if ( Msg_Argc( msg ) < 2 )
	{
		LOG( "Boolean value expected" );
		return sfalse;
	}

	arg = Msg_Argv( msg, 1 );

	if ( S_strcmp( arg, "0" ) == 0 || S_strcmp( arg, "false" ) == 0 )
	{
		*value = 0;
		return strue;
	}
	else if ( S_strcmp( arg, "1" ) == 0 || S_strcmp( arg, "true" ) == 0 )
	{
		*value = 1;
		return strue;
	}
	else
	{
		LOG( "Expected 0/1/true/false for boolean value" );
		return sfalse;
	}
}


sbool Msg_IsSpace( char ch )
{
	if ( ch == ' ' || ch == '\t' )
		return true;

	return false;
}


sbool Msg_IsDelim( char ch )
{
	if ( ch == ';' )
		return true;

	if ( ch == '\r' || ch == '\n' )
		return true;

	return false;
}


sbool Msg_CopyOneToArgBuffer( SMsg *msg, const char **cmd )
{
	const char 	*in;
	char 		ch;
	char 		*out;
	char 		*end;
	sbool 		inQuote;

	in = *cmd;
	out = msg->buffer;
	end = msg->buffer + MSG_LIMIT;

	inQuote = false;

	for ( ;; )
	{
		ch = *in;

		if ( ch == '"' )
			inQuote ^= 1;

		if ( !ch || (Msg_IsDelim( ch ) && !inQuote) )
			break;

		if ( out <= end )
			*out = ch;

		in++;
		out++;
	}

	while ( Msg_IsDelim( *in ) || Msg_IsSpace( *in ) )
		in++;
	
	*cmd = in;

	if ( out > end )
	{
		LOG( "Message is too long; %d characters is the maximum.", MSG_LIMIT );
		return sfalse;
	}
	else
	{
		*out = 0;
		return strue;
	}
}


sbool Msg_Parse( SMsg *msg, const char **cmd )
{
	char 	*p;

	assert( msg );

	if ( !Msg_CopyOneToArgBuffer( msg, cmd ) )
		return sfalse;

	msg->argCount = 0;

	p = msg->buffer;

	for ( ;; )
	{
		while ( Msg_IsSpace( *p ) )
			p++;

		if ( *p == 0 )
			break;

		if ( *p == '"' )
		{
			p++;
			msg->args[msg->argCount] = p;

			while ( *p && *p != '"' )
				p++;

			if ( *p != '"' )
			{
				LOG( "Failed to parse argument %d; missing end quote.", msg->argCount + 1 );
				return sfalse;
			}

			*p = 0;
			p++;
		}
		else
		{
			msg->args[msg->argCount] = p;

			while ( *p && !Msg_IsSpace( *p ) )
				p++;

			if ( *p )
			{
				*p = 0;
				p++;
			}
		}

		if ( msg->argCount >= MSG_ARG_LIMIT )
		{
			LOG( "Too many arguments to parse; %d is the maximum.", MSG_ARG_LIMIT );
			return sfalse;
		}
		msg->argCount++;
	}

	msg->bufferUsed = p - msg->buffer;

	return strue;
}


sbool Msg_ParseString( SMsg *msg, const char *str )
{
	return Msg_Parse( msg, &str );
}


void MsgQueue_Create( SMsgQueue *queue )
{
	assert( queue );

	memset( queue, 0, sizeof( SMsgQueue ) );

	pthread_mutex_init( &queue->mutex, NULL );
	pthread_cond_init( &queue->cond, NULL );
}


void MsgQueue_Destroy( SMsgQueue *queue )
{
	uint 	msgIter;
	uint 	index;

	assert( queue );

	pthread_mutex_lock( &queue->mutex );

	for ( msgIter = queue->get; msgIter != queue->put; msgIter++ )
	{
		index = msgIter % MSG_QUEUE_LIMIT;
		assert( queue->text[index] );
		free( queue->text[index] );
		queue->text[index] = NULL;
	}

	queue->get = 0;
	queue->put = 0;

	pthread_mutex_unlock( &queue->mutex );

	pthread_mutex_destroy( &queue->mutex );
	pthread_cond_destroy( &queue->cond );
}


void MsgQueue_Put( SMsgQueue *queue, const char *text )
{
	uint 	index;

	assert( queue );

	// LOG( "MsgQueue_Put: %s", text );

	pthread_mutex_lock( &queue->mutex );

	if ( queue->put >= queue->get + MSG_QUEUE_LIMIT )
	{
		LOG( "MsgQueue_Put: Queue is full, skipping %s.", text );
		pthread_mutex_unlock( &queue->mutex );
		return;	// $$$ wait for drain?  drop msg and post warning?  expand queue until memory is exhausted?
	}

	index = queue->put % MSG_QUEUE_LIMIT;

	assert( queue->text[index] == NULL );
	queue->text[index] = strdup( text );
	queue->put++;

	pthread_cond_signal( &queue->cond );

	pthread_mutex_unlock( &queue->mutex );
}


char *MsgQueue_Get( SMsgQueue *queue, uint waitMs )
{
	char 			*result;
	struct timespec tim;
	uint 			index;

	assert( queue );

	pthread_mutex_lock( &queue->mutex );

	if ( queue->get == queue->put )
	{
		if ( waitMs )
		{
			tim.tv_sec  = 0;
			tim.tv_nsec = waitMs * 1000000;
		
			pthread_cond_timedwait( &queue->cond, &queue->mutex, &tim );

			if ( queue->get == queue->put )
			{
				pthread_mutex_unlock( &queue->mutex );
				return NULL;
			}
		}
		else
		{
			pthread_cond_wait( &queue->cond, &queue->mutex );
		}
	}

	index = queue->get;

	result = queue->text[index];
	queue->text[index] = NULL;
	queue->get++;

	if ( queue->get >= MSG_QUEUE_LIMIT )
	{
		queue->get -= MSG_QUEUE_LIMIT;
		queue->put -= MSG_QUEUE_LIMIT;
	}

	pthread_mutex_unlock( &queue->mutex );

	return result;
}

