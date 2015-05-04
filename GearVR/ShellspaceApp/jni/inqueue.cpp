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
#include "inqueue.h"
#include "geometry.h"
#include "registry.h"
#include "texture.h"
#include "thread.h"


#define INQUEUE_SIZE 	1024


enum EInQueueKind
{
	INQUEUE_NOP,
	INQUEUE_TEXTURE_RESIZE,
	INQUEUE_TEXTURE_UPDATE,
	INQUEUE_TEXTURE_PRESENT,
	INQUEUE_GEOMETRY_RESIZE,
	INQUEUE_GEOMETRY_UPDATE_INDEX,
	INQUEUE_GEOMETRY_UPDATE_POSITION,
	INQUEUE_GEOMETRY_UPDATE_TEXCOORD,
	INQUEUE_GEOMETRY_UPDATE_COLOR,
	INQUEUE_GEOMETRY_PRESENT,
	INQUEUE_COUNT
};


struct STextureItem
{
	SRef				ref;
	mutable byte 		updateMask;
	union
	{
		struct
		{
			ushort		width;
			ushort		height;
			byte		format;
		} resize;
		struct
		{
			void 		*data;
			ushort		x;
			ushort		y;
			ushort		width;
			ushort		height;
		} update;
	};
};


struct SGeometryItem
{
	SRef				ref;
	mutable byte 		updateMask;
	union
	{
		struct
		{
			uint		vertexCount;
			uint		indexCount;
		} resize;
		struct
		{
			void 		*data;
			uint 		first;
			uint 		count;
		} update;
	};
};


struct SItem
{
	EInQueueKind 		kind;
	union
	{
		STextureItem	texture;
		SGeometryItem	geometry;
	};
};


struct SInQueueGlobals
{
	SItem 				queue[INQUEUE_SIZE];
	uint 				count;
	uint 				presentFrame;
};


SInQueueGlobals s_iq;


const char *s_itemKindNames[] =
{
	"Nop", 							// INQUEUE_NOP
	"TextureResize", 				// INQUEUE_TEXTURE_RESIZE
	"TextureUpdate", 				// INQUEUE_TEXTURE_UPDATE
	"TexturePresent", 				// INQUEUE_TEXTURE_PRESENT
	"GeometryResize", 				// INQUEUE_GEOMETRY_RESIZE
	"GeometryUpdateIndex", 			// INQUEUE_GEOMETRY_UPDATE_INDEX
	"GeometryUpdatePosition", 		// INQUEUE_GEOMETRY_UPDATE_POSITION
	"GeometryUpdateTexcoord", 		// INQUEUE_GEOMETRY_UPDATE_TEXCOORD
	"GeometryUpdateColor", 			// INQUEUE_GEOMETRY_UPDATE_COLOR
	"GeometryPresent", 				// INQUEUE_GEOMETRY_PRESENT
};


void InQueue_CheckAdvance( uint count )
{
	uint 		index;
	SItem 		*in;
	STexture 	*texture;
	SGeometry 	*geometry;

	for ( index = 0; index < count; index++ )
	{
		in = &s_iq.queue[index];

		switch ( in->kind )
		{
		case INQUEUE_TEXTURE_RESIZE:
		case INQUEUE_TEXTURE_UPDATE:
			{
				texture = Registry_GetTexture( in->texture.ref );
				assert( texture );

				if ( texture->updateIndex == texture->drawIndex )
					texture->updateIndex++;
			}
			break;

		case INQUEUE_GEOMETRY_RESIZE:
		case INQUEUE_GEOMETRY_UPDATE_INDEX:
		case INQUEUE_GEOMETRY_UPDATE_POSITION:
		case INQUEUE_GEOMETRY_UPDATE_TEXCOORD:
		case INQUEUE_GEOMETRY_UPDATE_COLOR:
			{
				geometry = Registry_GetGeometry( in->geometry.ref );
				assert( geometry );

				if ( geometry->updateIndex == geometry->drawIndex )
					geometry->updateIndex++;
			}
			break;

		default: 
			break;
		}
	}
}


void InQueue_ProcessTextureItem( SItem *in )
{
	STexture 	*texture;
	uint 		updateMask;

	texture = Registry_GetTexture( in->texture.ref );
	assert( texture );

	if ( texture->presentFrame == s_iq.presentFrame )
		return;

	updateMask = 1 << (texture->updateIndex % BUFFER_COUNT);

	switch ( in->kind )
	{
	case INQUEUE_TEXTURE_RESIZE:
		if ( !(in->texture.updateMask & updateMask) )
		{
			if ( texture->updateIndex == texture->drawIndex )
			{
				texture->updateIndex++;
				updateMask = 1 << (texture->updateIndex % BUFFER_COUNT);
			}

			Texture_Resize( texture, 
				in->texture.resize.width, 
				in->texture.resize.height, 
				static_cast< SxTextureFormat >( in->texture.resize.format ) );

			in->texture.updateMask |= updateMask;				
			if ( in->texture.updateMask == ALL_BUFFERS_MASK )
				in->kind = INQUEUE_NOP;
		}
		break;

	case INQUEUE_TEXTURE_UPDATE:
		if ( !(in->texture.updateMask & updateMask) )
		{
			if ( texture->updateIndex == texture->drawIndex )
			{
				texture->updateIndex++;
				updateMask = 1 << (texture->updateIndex % BUFFER_COUNT);
			}

			Texture_Update( texture, 
				in->texture.update.x, 
				in->texture.update.y, 
				in->texture.update.width, 
				in->texture.update.height, 
				in->texture.update.data );

			in->texture.updateMask |= updateMask;
			if ( in->texture.updateMask == ALL_BUFFERS_MASK )
			{
				free( in->texture.update.data );
				in->kind = INQUEUE_NOP;
			}
		}
		break;

	case INQUEUE_TEXTURE_PRESENT:
		if ( !(in->texture.updateMask & updateMask) )
		{
			assert( texture->updateIndex != texture->drawIndex ); // two presents with no updates

			Texture_Present( texture );

			in->texture.updateMask |= updateMask;
			if ( in->texture.updateMask == ALL_BUFFERS_MASK )
				in->kind = INQUEUE_NOP;

			texture->presentFrame = s_iq.presentFrame;
		}
		break;

	default:
		assert( false );
		break;
	}
}


void InQueue_ProcessGeometryItem( SItem *in )
{
	SGeometry 	*geometry;
	uint 		updateMask;

	geometry = Registry_GetGeometry( in->geometry.ref );
	assert( geometry );

	if ( geometry->presentFrame == s_iq.presentFrame )
		return;

	updateMask = 1 << (geometry->updateIndex % BUFFER_COUNT);

	switch ( in->kind )
	{
	case INQUEUE_GEOMETRY_RESIZE:
		if ( !(in->geometry.updateMask & updateMask) )
		{
			Geometry_Resize( geometry, 
				in->geometry.resize.vertexCount, 
				in->geometry.resize.indexCount );

			in->geometry.updateMask |= updateMask;				
			if ( in->geometry.updateMask == ALL_BUFFERS_MASK )
				in->kind = INQUEUE_NOP;
		}
		break;

	case INQUEUE_GEOMETRY_UPDATE_INDEX:
	case INQUEUE_GEOMETRY_UPDATE_POSITION:
	case INQUEUE_GEOMETRY_UPDATE_COLOR:
	case INQUEUE_GEOMETRY_UPDATE_TEXCOORD:
		if ( !(in->geometry.updateMask & updateMask) )
		{
			switch ( in->kind )
			{
			case INQUEUE_GEOMETRY_UPDATE_INDEX:
				Geometry_UpdateIndices( geometry, 
					in->geometry.update.first, 
					in->geometry.update.count, 
					in->geometry.update.data );
				break;
			case INQUEUE_GEOMETRY_UPDATE_POSITION:
				Geometry_UpdateVertexPositions( geometry, 
					in->geometry.update.first, 
					in->geometry.update.count, 
					in->geometry.update.data );
				break;
			case INQUEUE_GEOMETRY_UPDATE_COLOR:
				Geometry_UpdateVertexColors( geometry, 
					in->geometry.update.first, 
					in->geometry.update.count, 
					in->geometry.update.data );
				break;
			case INQUEUE_GEOMETRY_UPDATE_TEXCOORD:
				Geometry_UpdateVertexTexCoords( geometry, 
					in->geometry.update.first, 
					in->geometry.update.count, 
					in->geometry.update.data );
				break;
			default:
				assert( false );
				break;
			}

			in->geometry.updateMask |= updateMask;
			if ( in->geometry.updateMask == ALL_BUFFERS_MASK )
			{
				free( in->geometry.update.data );
				in->kind = INQUEUE_NOP;
			}
		}
		break;

	case INQUEUE_GEOMETRY_PRESENT:
		if ( !(in->geometry.updateMask & updateMask) )
		{
			assert( geometry->updateIndex != geometry->drawIndex ); // two presents with no updates

			Geometry_Present( geometry );

			in->geometry.updateMask |= updateMask;
			if ( in->geometry.updateMask == ALL_BUFFERS_MASK )
				in->kind = INQUEUE_NOP;

			geometry->presentFrame = s_iq.presentFrame;
		}
		break;

	default:
		assert( false );
		break;
	}
}


void InQueue_Compact()
{
	int 		count;
	int 		index;
	SItem 		*in;

	Thread_Lock( MUTEX_INQUEUE );

	count = s_iq.count;

	for ( index = 0; index < count; index++ )
	{
		in = &s_iq.queue[index];
		if ( in->kind != INQUEUE_NOP )
			break;
	}

	count -= index;
	memmove( &s_iq.queue[0], &s_iq.queue[index], sizeof( SItem ) * count );

	s_iq.count = count;

	Thread_Unlock( MUTEX_INQUEUE );
}


void InQueue_Frame()
{
	int 		count;
	int 		index;
	SItem 		*in;
	double 		startMs;

	Prof_Start( PROF_GPU_UPDATE );

	Thread_Lock( MUTEX_INQUEUE );

	// This is safe because other threads can only append to or modify the queue but not decrease its count.
	count = s_iq.count;

	Thread_Unlock( MUTEX_INQUEUE );

	if ( !count )
		return;

	InQueue_CheckAdvance( count );

	startMs = Prof_MS();

	for ( index = 0; index < count; index++ )
	{
		in = &s_iq.queue[index];

		switch ( in->kind )
		{
		case INQUEUE_NOP:
			break;

		case INQUEUE_TEXTURE_RESIZE:
		case INQUEUE_TEXTURE_UPDATE:
		case INQUEUE_TEXTURE_PRESENT:
			InQueue_ProcessTextureItem( in );
			break;

		case INQUEUE_GEOMETRY_RESIZE:
		case INQUEUE_GEOMETRY_UPDATE_INDEX:
		case INQUEUE_GEOMETRY_UPDATE_POSITION:
		case INQUEUE_GEOMETRY_UPDATE_COLOR:
		case INQUEUE_GEOMETRY_UPDATE_TEXCOORD:
		case INQUEUE_GEOMETRY_PRESENT:
			InQueue_ProcessGeometryItem( in );
			break;

		default:
			assert( false );
			break;
		}

		if ( Prof_MS() - startMs > 10.0f )
			break;
	}

	InQueue_Compact();

	s_iq.presentFrame++;

	Prof_Stop( PROF_GPU_UPDATE );
}


void InQueue_ClearRefs( SRef ref )
{
	uint 		index;
	SItem 		*in;

	Thread_Lock( MUTEX_INQUEUE );

	for ( index = 0; index < s_iq.count; index++ )
	{
		in = &s_iq.queue[index];

		switch ( in->kind )
		{
		case INQUEUE_TEXTURE_UPDATE:
			free( in->texture.update.data );
			// fall through
		case INQUEUE_TEXTURE_RESIZE:
		case INQUEUE_TEXTURE_PRESENT:
			in->kind = INQUEUE_NOP;
			break;

		case INQUEUE_GEOMETRY_UPDATE_INDEX:
		case INQUEUE_GEOMETRY_UPDATE_POSITION:
		case INQUEUE_GEOMETRY_UPDATE_COLOR:
		case INQUEUE_GEOMETRY_UPDATE_TEXCOORD:
			free( in->geometry.update.data );
			// fall through
		case INQUEUE_GEOMETRY_RESIZE:
		case INQUEUE_GEOMETRY_PRESENT:
			in->kind = INQUEUE_NOP;
			break;

		default:
			break;
		}
	}

	Thread_Unlock( MUTEX_INQUEUE );
}


SItem *InQueue_BeginAppend( EInQueueKind kind )
{
	SItem 			*in;
	sbool 			logged;

	logged = sfalse;

	for ( ;; )
	{
		Thread_Lock( MUTEX_INQUEUE );
		Prof_Start( PROF_GPU_UPDATE_APPEND );

		if ( s_iq.count < INQUEUE_SIZE )
		{
			in = &s_iq.queue[s_iq.count];
			s_iq.count++;

			memset( in, 0, sizeof( *in ) );
			in->kind = kind;

			return in;
		}

		Prof_Stop( PROF_GPU_UPDATE_APPEND );
		Thread_Unlock( MUTEX_INQUEUE );

		if ( !logged )
		{
			LOG( "InQueue_BeginAppend: GPU update queue is full, stalling." );
			logged = strue;
		}

		Thread_Sleep( 1 );
	}
}


void InQueue_EndAppend()
{
	Prof_Stop( PROF_GPU_UPDATE_APPEND );
	Thread_Unlock( MUTEX_INQUEUE );
}


void InQueue_ResizeTexture( SRef ref, uint width, uint height, SxTextureFormat format )
{
	SItem 	*in;

	assert( width );
	assert( height );

	in = InQueue_BeginAppend( INQUEUE_TEXTURE_RESIZE );

	in->texture.ref = ref;
	in->texture.resize.width = width;
	in->texture.resize.height = height;

	InQueue_EndAppend();
}


void InQueue_UpdateTextureRect( SRef ref, uint x, uint y, uint width, uint height, const void *data )
{
	STexture 	*texture;
	SItem 		*in;
	uint 		dataSize;
	void 		*dataCopy;

	assert( width );
	assert( height );

	texture = Registry_GetTexture( ref );
	assert( texture );

	dataSize = Texture_GetDataSize( width, height, texture->format ); 

	dataCopy = malloc( dataSize );
	assert( dataCopy );
	memcpy( dataCopy, data, dataSize );

	in = InQueue_BeginAppend( INQUEUE_TEXTURE_UPDATE );

	in->texture.ref = ref;
	in->texture.update.x = x;
	in->texture.update.y = y;
	in->texture.update.width = width;
	in->texture.update.height = height;
	in->texture.update.data = dataCopy;

	InQueue_EndAppend();
}


void InQueue_PresentTexture( SRef ref )
{
	SItem 	*in;

	in = InQueue_BeginAppend( INQUEUE_TEXTURE_PRESENT );

	in->texture.ref = ref;

	InQueue_EndAppend();
}


void InQueue_ResizeGeometry( SRef ref, uint vertexCount, uint indexCount )
{
	SItem 	*in;

	assert( indexCount );
	assert( vertexCount );

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_RESIZE );

	in->geometry.ref = ref;
	in->geometry.resize.vertexCount = vertexCount;
	in->geometry.resize.indexCount = indexCount;

	InQueue_EndAppend();
}


void InQueue_UpdateGeometryIndices( SRef ref, uint firstIndex, uint indexCount, const void *data )
{
	SItem 	*in;
	uint 	dataSize;
	void 	*dataCopy;

	assert( indexCount );

	dataSize = indexCount * sizeof( ushort );

	dataCopy = malloc( dataSize );
	assert( dataCopy );
	memcpy( dataCopy, data, dataSize );

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_UPDATE_INDEX );

	in->geometry.ref = ref;
	in->geometry.update.first = firstIndex;
	in->geometry.update.count = indexCount;
	in->geometry.update.data = dataCopy;

	InQueue_EndAppend();
}


void InQueue_UpdateGeometryPositions( SRef ref, uint firstVertex, uint vertexCount, const void *data )
{
	SItem 	*in;
	uint 	dataSize;
	void 	*dataCopy;

	assert( vertexCount );

	dataSize = vertexCount * sizeof( float ) * 3;
	
	dataCopy = malloc( dataSize );
	assert( dataCopy );
	memcpy( dataCopy, data, dataSize );

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_UPDATE_POSITION );

	in->geometry.ref = ref;
	in->geometry.update.first = firstVertex;
	in->geometry.update.count = vertexCount;
	in->geometry.update.data = dataCopy;

	InQueue_EndAppend();
}


void InQueue_UpdateGeometryTexCoords( SRef ref, uint firstVertex, uint vertexCount, const void *data )
{
	SItem 	*in;
	uint 	dataSize;
	void 	*dataCopy;

	assert( vertexCount );

	dataSize = vertexCount * sizeof( float ) * 2;
	
	dataCopy = malloc( dataSize );
	assert( dataCopy );
	memcpy( dataCopy, data, dataSize );

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_UPDATE_TEXCOORD );

	in->geometry.ref = ref;
	in->geometry.update.first = firstVertex;
	in->geometry.update.count = vertexCount;
	in->geometry.update.data = dataCopy;

	InQueue_EndAppend();
}


void InQueue_UpdateGeometryColors( SRef ref, uint firstVertex, uint vertexCount, const void *data )
{
	SItem 	*in;
	uint 	dataSize;
	void 	*dataCopy;

	assert( vertexCount );

	dataSize = vertexCount * sizeof( byte ) * 4;
	
	dataCopy = malloc( dataSize );
	assert( dataCopy );
	memcpy( dataCopy, data, dataSize );

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_UPDATE_COLOR );

	in->geometry.ref = ref;
	in->geometry.update.first = firstVertex;
	in->geometry.update.count = vertexCount;
	in->geometry.update.data = dataCopy;

	InQueue_EndAppend();
}


void InQueue_PresentGeometry( SRef ref )
{
	SItem 	*in;

	in = InQueue_BeginAppend( INQUEUE_GEOMETRY_PRESENT );

	in->geometry.ref = ref;

	InQueue_EndAppend();
}


