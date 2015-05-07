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
#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MSG_ARG_LIMIT		32
#define MSG_LIMIT 			1024
#define MSG_QUEUE_LIMIT 	16

struct SMsg
{
	uint 	argCount;
	char 	*args[MSG_ARG_LIMIT];
	uint 	bufferUsed;
	char 	buffer[MSG_LIMIT];
};

typedef void (*FMsgCmdFn)( const SMsg *msg, void *context );

struct SMsgCmd
{
	const char 	*name;
	FMsgCmdFn 	fn;
	const char 	*doc;
};

struct SMsgQueue
{
	pthread_mutex_t	mutex;
	pthread_cond_t 	cond;
	uint 			get;
	uint 			put;
	char			*text[MSG_QUEUE_LIMIT];
};

sbool Msg_Parse( SMsg *msg, const char **cmd );
sbool Msg_ParseString( SMsg *msg, const char *str );

uint Msg_Argc( const SMsg *msg );
sbool Msg_Empty( const SMsg *msg );

const char *Msg_Argv( const SMsg *msg, uint argIndex );
sbool Msg_IsArgv( const SMsg *msg, uint argIndex, const char *value );

void Msg_Shift( SMsg *msg, uint count );
void Msg_Format( const SMsg *msg, char *result, uint resultLen );

sbool MsgCmd_Dispatch( const SMsg *msg, const SMsgCmd *cmdList, void *context );

sbool Msg_SetFloatCmd( const SMsg *msg, float *value, float mn, float mx ); 
sbool Msg_SetIntCmd( const SMsg *msg, int *value, int mn, int mx );
sbool Msg_SetBoolCmd( const SMsg *msg, sbool *value );

void MsgQueue_Create( SMsgQueue *queue );
void MsgQueue_Destroy( SMsgQueue *queue );

void MsgQueue_Put( SMsgQueue *queue, const char *text );
char *MsgQueue_Get( SMsgQueue *queue, uint waitMs );

#endif
