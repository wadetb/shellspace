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
#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include "message.h"

enum ERegistry
{
	PLUGIN_REGISTRY,
	WIDGET_REGISTRY,
	GEOMETRY_REGISTRY,
	TEXTURE_REGISTRY,
	ENTITY_REGISTRY,
	REGISTRY_COUNT
};

#define ID_LIMIT			32

#define BUFFER_COUNT 		3
#define ALL_BUFFERS_MASK	0x7

struct SPlugin
{
	SRefLink		poolLink;

	char			*id;

	SxPluginKind	kind;

	SMsgQueue		msgQueue;
};

struct SWidget
{
	SRefLink		poolLink;

	char			*id;
};

struct SGeometry
{
	SRefLink		poolLink;
	
	char			*id;
	
	uint 			vertexCount;
	uint 			indexCount;

	GLuint 			vertexArrayObjects[BUFFER_COUNT];
	GLuint 			vertexBuffers[BUFFER_COUNT];
	GLuint 			indexBuffers[BUFFER_COUNT];

	uint 			vertexCounts[BUFFER_COUNT];
	uint 			indexCounts[BUFFER_COUNT];

	byte 			updateIndex;
	byte 			drawIndex;

	uint 			presentFrame;
};

struct STexture
{
	SRefLink		poolLink;
	
	char			*id;
	
	SxTextureFormat	format;
	ushort 			width;
	ushort 			height;

	GLuint 			texId[BUFFER_COUNT];
	ushort 			texWidth[BUFFER_COUNT];
	ushort			texHeight[BUFFER_COUNT];

	byte 			updateIndex;
	byte 			drawIndex;

	uint 			presentFrame;
};

struct SEntity
{
	SRefLink		poolLink;
	SRefLink		activeLink;
	
	char			*id;
	
	SRef 			geometryRef;
	SRef 			textureRef;
	
	SxOrientation	orientation;
	float 			visibility;

	SRef 			parentRef;
	SRefLink		parentLink;
	SRef 			firstChild;
};

void Registry_Init();
void Registry_Shutdown();

sbool Registry_IsValidId( const char *id );
SRef Registry_Register( ERegistry reg, const char *id );
void Registry_Unregister( ERegistry reg, SRef ref );
uint Registry_GetCount( ERegistry reg );

#define Registry_RefForIndex( index ) (SRef)( 1 + (index) )

SRef Registry_GetPluginRef( const char *id );
SRef Registry_GetWidgetRef( const char *id );
SRef Registry_GetGeometryRef( const char *id );
SRef Registry_GetTextureRef( const char *id );
SRef Registry_GetEntityRef( const char *id );

SPlugin *Registry_GetPlugin( SRef ref );
SWidget *Registry_GetWidget( SRef ref );
SGeometry *Registry_GetGeometry( SRef ref );
STexture *Registry_GetTexture( SRef ref );
SEntity *Registry_GetEntity( SRef ref );

SRef Registry_GetEntityRefByPointer( SEntity *entity );

#endif
