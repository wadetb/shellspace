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
#include "entity.h"
#include "inqueue.h"
#include "registry.h"
#include "texture.h"
#include "thread.h"


SxResult sxRegisterPlugin( SxPluginHandle pl, SxPluginKind kind )
{
	SRef 		ref;
	char		*id;
	SPlugin 	*plugin;

	if ( !Registry_IsValidId( pl ) )
		return SX_INVALID_HANDLE;

	Thread_ScopeLock lock( MUTEX_API );

	id = strdup( pl );

	ref = Registry_Register( PLUGIN_REGISTRY, id );
	if ( ref == S_NULL_REF )
	{
		free( id );
		return SX_ALREADY_REGISTERED;
	}

	plugin = Registry_GetPlugin( ref );
	assert( plugin );

	plugin->id = id;
	plugin->kind = kind;

	MsgQueue_Create( &plugin->msgQueue );

	S_Log( "Registered plugin %s.", id );

	return SX_OK;
}


SxResult sxUnregisterPlugin( SxPluginHandle pl )
{
	SRef 		ref;
	SPlugin 	*plugin;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetPluginRef( pl );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	plugin = Registry_GetPlugin( ref );
	assert( plugin );

	MsgQueue_Destroy( &plugin->msgQueue );

	Registry_Unregister( PLUGIN_REGISTRY, ref );

	S_Log( "Unregistered plugin %s.", plugin->id );
	free( plugin->id );

	return SX_OK;
}


SxResult sxReceiveMessage( SxPluginHandle pl, uint waitMs, char *result, uint resultLen )
{
	SRef 		ref;
	SPlugin 	*plugin;
	char 		*text;

	Thread_Lock( MUTEX_API );

	ref = Registry_GetPluginRef( pl );
	if ( ref == S_NULL_REF )
	{
		Thread_Unlock( MUTEX_API );
		return SX_INVALID_HANDLE;
	}

	plugin = Registry_GetPlugin( ref );
	assert( plugin );

	Thread_Unlock( MUTEX_API );

	text = MsgQueue_Get( &plugin->msgQueue, waitMs );

	if ( text )
	{
		snprintf( result, resultLen, "%s", text );
		free( text );
	}
	else
	{
		strcpy( result, "" );
	}

	return SX_OK;
}


SxResult sxRegisterWidget( SxWidgetHandle wd )
{
	SRef 		ref;
	char		*id;
	SWidget 	*widget;

	if ( !Registry_IsValidId( wd ) )
		return SX_INVALID_HANDLE;

	Thread_ScopeLock lock( MUTEX_API );

	id = strdup( wd );

	ref = Registry_Register( WIDGET_REGISTRY, id );
	if ( ref == S_NULL_REF )
	{
		free( id );
		return SX_ALREADY_REGISTERED;
	}

	widget = Registry_GetWidget( ref );
	assert( widget );

	widget->id = id;

	return SX_OK;
}


SxResult sxUnregisterWidget( SxWidgetHandle wd )
{
	SRef 		ref;
	SWidget 	*widget;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetWidgetRef( wd );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	widget = Registry_GetWidget( ref );
	assert( widget );

	Registry_Unregister( WIDGET_REGISTRY, ref );

	free( widget->id );

	return SX_OK;
}


SxResult sxPostMessage( const char *message )
{
	Thread_ScopeLock lock( MUTEX_API );

	Cmd_Add( message );

	return SX_OK;
}


SxResult sxRegisterGeometry( SxGeometryHandle geo )
{
	SRef 		ref;
	char		*id;
	SGeometry 	*geometry;

	if ( !Registry_IsValidId( geo ) )
		return SX_INVALID_HANDLE;

	Thread_ScopeLock lock( MUTEX_API );

	id = strdup( geo );

	ref = Registry_Register( GEOMETRY_REGISTRY, id );
	if ( ref == S_NULL_REF )
	{
		free( id );
		return SX_ALREADY_REGISTERED;
	}

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	geometry->id = id;

	return SX_OK;
}


SxResult sxUnregisterGeometry( SxGeometryHandle geo )
{
	SRef 		ref;
	SGeometry 	*geometry;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	InQueue_ClearRefs( ref );

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	Registry_Unregister( GEOMETRY_REGISTRY, ref );

	free( geometry->id );

	return SX_OK;
}


SxResult sxSizeGeometry( SxGeometryHandle geo, unsigned int vertexCount, unsigned int indexCount )
{
	SRef 		ref;
	SGeometry 	*geometry;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	if ( !vertexCount || !indexCount )
		return SX_OUT_OF_RANGE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	geometry->vertexCount = vertexCount;
	geometry->indexCount = indexCount;

	InQueue_ResizeGeometry( ref, vertexCount, indexCount );

	return SX_OK;
}


SxResult sxUpdateGeometryIndexRange( SxGeometryHandle geo, unsigned int firstIndex, unsigned int indexCount, const ushort *indices )
{
	SRef 		ref;
	SGeometry 	*geometry;

	if ( !indexCount )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	if ( !geometry->indexCount )
		return SX_OUT_OF_RANGE;

	InQueue_UpdateGeometryIndices( ref, firstIndex, indexCount, indices );

	return SX_OK;
}


SxResult sxUpdateGeometryPositionRange( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxVector3 *positions )
{
	SRef 		ref;
	SGeometry 	*geometry;

	if ( !vertexCount )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	if ( !geometry->vertexCount )
		return SX_OUT_OF_RANGE;

	InQueue_UpdateGeometryPositions( ref, firstVertex, vertexCount, positions );

	return SX_OK;
}


SxResult sxUpdateGeometryTexCoordRange( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxVector2 *texCoords )
{
	SRef 		ref;
	SGeometry 	*geometry;

	if ( !vertexCount )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	if ( !geometry->vertexCount )
		return SX_OUT_OF_RANGE;

	InQueue_UpdateGeometryTexCoords( ref, firstVertex, vertexCount, texCoords );

	return SX_OK;
}


SxResult sxUpdateGeometryColorRange( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxColor *colors )
{
	SRef 		ref;
	SGeometry 	*geometry;

	if ( !vertexCount )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	if ( !geometry->vertexCount )
		return SX_OUT_OF_RANGE;

	InQueue_UpdateGeometryColors( ref, firstVertex, vertexCount, colors );

	return SX_OK;
}


SxResult sxPresentGeometry( SxGeometryHandle geo )
{
	SRef 		ref;
	SGeometry 	*geometry;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetGeometryRef( geo );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geometry = Registry_GetGeometry( ref );
	assert( geometry );

	if ( !geometry->vertexCount || !geometry->indexCount )
		return SX_OUT_OF_RANGE;

	InQueue_PresentGeometry( ref );

	return SX_OK;
}


SxResult sxRegisterTexture( SxTextureHandle tex )
{
	SRef 		ref;
	char		*id;
	STexture 	*texture;

	if ( !Registry_IsValidId( tex ) )
		return SX_INVALID_HANDLE;

	Thread_ScopeLock lock( MUTEX_API );

	id = strdup( tex );

	ref = Registry_Register( TEXTURE_REGISTRY, id );
	if ( ref == S_NULL_REF )
	{
		free( id );
		return SX_ALREADY_REGISTERED;
	}

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->id = id;

	return SX_OK;
}


SxResult sxUnregisterTexture( SxTextureHandle tex )
{
	SRef 		ref;
	STexture 	*texture;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	InQueue_ClearRefs( ref );

	texture = Registry_GetTexture( ref );
	assert( texture );

	Registry_Unregister( TEXTURE_REGISTRY, ref );

	free( texture->id );

	return SX_OK;
}


SxResult sxFormatTexture( SxTextureHandle tex, SxTextureFormat format )
{
	SRef 		ref;
	STexture 	*texture;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->format = format;

	if ( texture->width && texture->height )
		InQueue_ResizeTexture( ref, texture->width, texture->height, format );

	return SX_OK;
}


SxResult sxSizeTexture( SxTextureHandle tex, unsigned int width, unsigned int height )
{
	SRef 		ref;
	STexture 	*texture;

	if ( !width || !height )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->width = width;
	texture->height = height;

	InQueue_ResizeTexture( ref, width, height, texture->format );

	return SX_OK;
}


SxResult sxClearTexture( SxTextureHandle tex, SxColor color )
{
	SRef 		ref;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	return SX_NOT_IMPLEMENTED;	
}


SxResult sxUpdateTextureRect( SxTextureHandle tex, unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int pitch, const void *data )
{
	SRef 		ref;
	STexture 	*texture;

	if ( !width || !height )
		return SX_OUT_OF_RANGE;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	texture = Registry_GetTexture( ref );
	assert( texture );

	if ( !texture->width || !texture->height )
		return SX_OUT_OF_RANGE;

	// $$$ Support pitch in the update function by doing a strided memcpy.
	if ( pitch != width * 4 )
		return SX_NOT_IMPLEMENTED;

	if ( x > texture->width || y > texture->height )
		return SX_NOT_IMPLEMENTED;

	if ( x + width > texture->width || y + height > texture->height )
		return SX_NOT_IMPLEMENTED;

	InQueue_UpdateTextureRect( ref, x, y, width, height, data );

	return SX_OK;
}


SxResult sxLoadTextureSvg( SxTextureHandle tex, const char *svg )
{
	SRef 			ref;
	STexture 		*texture;
	uint 			width;
	uint 			height;
	SxTextureFormat format;
	void 			*data;

	if ( !Texture_LoadSvg( svg, &width, &height, &format, &data ) )
		return SX_INVALID_PARAMETER;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
	{
		free( data );
		return SX_INVALID_HANDLE;
	}

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->width = width;
	texture->height = height;
	texture->format = format;

	InQueue_ResizeTexture( ref, width, height, format );
	InQueue_UpdateTextureRect( ref, 0, 0, width, height, data );
	InQueue_PresentTexture( ref );

	free( data );

	return SX_OK;
}


SxResult sxLoadTextureJpeg( SxTextureHandle tex, const void *jpegData, uint jpegSize )
{
	SRef 			ref;
	STexture 		*texture;
	uint 			width;
	uint 			height;
	SxTextureFormat format;
	void 			*data;

	if ( !Texture_DecompressJpeg( jpegData, jpegSize, &width, &height, &format, &data ) )
		return SX_INVALID_PARAMETER;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
	{
		free( data );
		return SX_INVALID_HANDLE;
	}

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->width = width;
	texture->height = height;
	texture->format = format;

	InQueue_ResizeTexture( ref, width, height, format );
	InQueue_UpdateTextureRect( ref, 0, 0, width, height, data );
	InQueue_PresentTexture( ref );

	free( data );

	return SX_OK;
}


SxResult sxLoadTextureBitmap( SxTextureHandle tex, SkBitmap *bitmap )
{
	SRef 			ref;
	STexture 		*texture;
	uint 			width;
	uint 			height;
	SxTextureFormat format;
	void 			*data;

	if ( !Texture_LoadBitmap( bitmap, &width, &height, &format, &data ) )
		return SX_INVALID_PARAMETER;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
	{
		free( data );
		return SX_INVALID_HANDLE;
	}

	texture = Registry_GetTexture( ref );
	assert( texture );

	texture->width = width;
	texture->height = height;
	texture->format = format;

	InQueue_ResizeTexture( ref, width, height, format );
	InQueue_UpdateTextureRect( ref, 0, 0, width, height, data );
	InQueue_PresentTexture( ref );

	free( data );

	return SX_OK;
}


SxResult sxPresentTexture( SxTextureHandle tex )
{
	SRef 		ref;
	STexture 	*texture;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetTextureRef( tex );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	texture = Registry_GetTexture( ref );
	assert( texture );

	if ( !texture->width || !texture->height )
		return SX_OUT_OF_RANGE;

	InQueue_PresentTexture( ref );

	return SX_OK;
}


SxResult sxRegisterEntity( SxEntityHandle ent )
{
	SRef 		ref;
	char		*id;
	SEntity 	*entity;

	if ( !Registry_IsValidId( ent ) )
		return SX_INVALID_HANDLE;

	Thread_ScopeLock lock( MUTEX_API );

	id = strdup( ent );

	ref = Registry_Register( ENTITY_REGISTRY, id );
	if ( ref == S_NULL_REF )
	{
		free( id );
		return SX_ALREADY_REGISTERED;
	}

	entity = Registry_GetEntity( ref );
	assert( entity );

	Entity_Register( entity );

	entity->id = id;

	return SX_OK;
}


SxResult sxUnregisterEntity( SxEntityHandle ent )
{
	SRef 		ref;
	SEntity 	*entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	entity = Registry_GetEntity( ref );
	assert( entity );

	Entity_Unregister( entity );
	Registry_Unregister( ENTITY_REGISTRY, ref );

	free( entity->id );

	return SX_OK;
}


SxResult sxSetEntityGeometry( SxEntityHandle ent, SxGeometryHandle geo )
{
	SRef 	ref;
	SRef 	geoRef;
	SEntity *entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	geoRef = Registry_GetGeometryRef( geo );
	if ( geoRef == S_NULL_REF )
		return SX_INVALID_HANDLE;

	entity = Registry_GetEntity( ref );
	assert( entity );

	entity->geometryRef = geoRef;

	return SX_OK;	
}


SxResult sxSetEntityTexture( SxEntityHandle ent, SxTextureHandle tex )
{
	SRef 	ref;
	SRef 	texRef;
	SEntity *entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	texRef = Registry_GetTextureRef( tex );
	if ( texRef == S_NULL_REF )
		return SX_INVALID_HANDLE;

	entity = Registry_GetEntity( ref );
	assert( entity );

	entity->textureRef = texRef;

	return SX_OK;	
}


SxResult sxOrientEntity( SxEntityHandle ent, const SxOrientation *o, const SxTrajectory *tr )
{
	SRef 	ref;
	SEntity *entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	entity = Registry_GetEntity( ref );
	assert( entity );

	if ( tr->kind != SxTrajectoryKind_Instant )
		return SX_NOT_IMPLEMENTED;

	entity->orientation = *o;

	return SX_OK;
}


SxResult sxSetEntityVisibility( SxEntityHandle ent, float visibility, const SxTrajectory *tr )
{
	SRef 	ref;
	SEntity *entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	entity = Registry_GetEntity( ref );
	assert( entity );

	if ( tr->kind != SxTrajectoryKind_Instant )
		return SX_NOT_IMPLEMENTED;

	entity->visibility = visibility;

	return SX_OK;
}


SxResult sxParentEntity( SxEntityHandle ent, SxEntityHandle parent )
{
	SRef 	ref;
	SRef 	parentRef;
	SEntity *entity;

	Thread_ScopeLock lock( MUTEX_API );

	ref = Registry_GetEntityRef( ent );
	if ( ref == S_NULL_REF )
		return SX_INVALID_HANDLE;

	if ( S_strempty( parent ) )
	{
		parentRef = S_NULL_REF;
	}
	else
	{
		parentRef = Registry_GetEntityRef( parent );
		if ( parentRef == S_NULL_REF )
			return SX_INVALID_HANDLE;
	}

	entity = Registry_GetEntity( ref );
	assert( entity );

	Entity_SetParent( entity, parentRef );

	return SX_OK;
}


SxPluginInterface g_pluginInterface =
{
    SX_PLUGIN_INTERFACE_VERSION,			// version
    sxRegisterPlugin,                       // registerPlugin
    sxUnregisterPlugin,                     // unregisterPlugin
    sxReceiveMessage,						// receiveMessage
    sxRegisterWidget,                       // registerWidget
    sxUnregisterWidget,                     // unregisterWidget
    sxPostMessage,                    		// postMessage
    sxRegisterGeometry,                     // registerGeometry
    sxUnregisterGeometry,                   // unregisterGeometry
    sxSizeGeometry,                         // sizeGeometry
    sxUpdateGeometryIndexRange,             // updateGeometryIndexRange
    sxUpdateGeometryPositionRange,          // updateGeometryPositionRange
    sxUpdateGeometryTexCoordRange,          // updateGeometryTexCoordRange
    sxUpdateGeometryColorRange,             // updateGeometryColorRange
    sxPresentGeometry,                    	// presentGeometry
    sxRegisterTexture,                      // registerTexture
    sxUnregisterTexture,                    // unregisterTexture
    sxFormatTexture,                        // formatTexture
    sxSizeTexture,                          // sizeTexture
    sxClearTexture,                         // clearTexture
    sxUpdateTextureRect,                    // updateTextureRect
    sxLoadTextureSvg,                     	// loadTextureSvg
    sxLoadTextureJpeg,                    	// loadTextureJpeg
    sxLoadTextureBitmap,                    // loadTextureBitmap
    sxPresentTexture,                    	// presentTexture
    sxRegisterEntity,                       // registerEntity
    sxUnregisterEntity,                     // unregisterEntity
    sxSetEntityGeometry,                    // setEntityGeometry
    sxSetEntityTexture,                     // setEntityTexture
    sxOrientEntity,                         // orientEntity
    sxSetEntityVisibility,                  // setEntityVisibility
    sxParentEntity,                  		// parentEntity
};
