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
#include "geometry.h"
#include "registry.h"

#include <GlUtils.h>
    

void Geometry_MakeVertexArrayObject( SGeometry *geometry )
{
	uint 	index;
	GLuint 	vertexArrayObject;
	GLuint 	vertexBuffer;
	GLuint 	indexBuffer;
	uint 	vertexCount;
	uint 	positionSize;
	uint 	texCoordSize;
	uint 	positionOffset;
	uint 	texCoordOffset;
	uint 	colorOffset;

	index = geometry->updateIndex % BUFFER_COUNT;

	vertexBuffer = geometry->vertexBuffers[index];
	indexBuffer = geometry->indexBuffers[index];

	assert( vertexBuffer );
	assert( indexBuffer );

	OVR::GL_CheckErrors( "before Geometry_MakeVertexArrayObject" );

	vertexArrayObject = geometry->vertexArrayObjects[index];

	if ( !vertexArrayObject )
		glGenVertexArraysOES_( 1, &vertexArrayObject );

	glBindVertexArrayOES_( vertexArrayObject );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	glEnableVertexAttribArray( VERTEX_ATTRIBUTE_POSITION );
	glEnableVertexAttribArray( VERTEX_ATTRIBUTE_TEXCOORD );
	glEnableVertexAttribArray( VERTEX_ATTRIBUTE_COLOR );

	vertexCount = geometry->vertexCount;

	positionSize = sizeof( float ) * 3 * vertexCount;
	texCoordSize = sizeof( float ) * 2 * vertexCount;

	positionOffset = 0;
	texCoordOffset = positionOffset + positionSize;
	colorOffset = texCoordOffset + texCoordSize;

	glVertexAttribPointer( VERTEX_ATTRIBUTE_POSITION, 
		3, GL_FLOAT, GL_FALSE, sizeof( float ) * 3, 
		(void *)( positionOffset ) );

	glVertexAttribPointer( VERTEX_ATTRIBUTE_TEXCOORD, 
		2, GL_FLOAT, GL_FALSE, sizeof( float ) * 2, 
		(void *)( texCoordOffset ) );

	glVertexAttribPointer( VERTEX_ATTRIBUTE_COLOR, 
		4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( byte ) * 4, 
		(void *)( colorOffset ) );

	glBindVertexArrayOES_( 0 );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_POSITION );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_TEXCOORD );
	glDisableVertexAttribArray( VERTEX_ATTRIBUTE_COLOR );

	geometry->vertexArrayObjects[index] = vertexArrayObject;

	OVR::GL_CheckErrors( "after Geometry_MakeVertexArrayObject" );
}


void Geometry_ResizeVertexBuffer( SGeometry *geometry, uint vertexCount )
{
	uint 	index;
	GLuint 	vertexBuffer;
	uint 	vertexSize;

	OVR::GL_CheckErrors( "before Geometry_ResizeVertexBuffer" );

	index = geometry->updateIndex % BUFFER_COUNT;

	if ( geometry->vertexBuffers[index] )
		glDeleteBuffers( 1, &geometry->vertexBuffers[index] );

	glGenBuffers( 1, &vertexBuffer );

	vertexSize = sizeof( float ) * 3 + sizeof( float ) * 2 + sizeof( byte ) * 4;

	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, vertexCount * vertexSize, NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	geometry->vertexBuffers[index] = vertexBuffer; 
	geometry->vertexCounts[index] = vertexCount;

	OVR::GL_CheckErrors( "after Geometry_ResizeVertexBuffer" );
}


void Geometry_ResizeIndexBuffer( SGeometry *geometry, uint indexCount )
{
	uint 	index;
	GLuint 	indexBuffer;

	OVR::GL_CheckErrors( "before Geometry_ResizeIndexBuffer" );

	index = geometry->updateIndex % BUFFER_COUNT;

	if ( geometry->indexBuffers[index] )
		glDeleteBuffers( 1, &geometry->indexBuffers[index] );

	glGenBuffers( 1, &indexBuffer );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof( ushort ), NULL, GL_STATIC_DRAW );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	geometry->indexBuffers[index] = indexBuffer;
	geometry->indexCounts[index] = indexCount;

	OVR::GL_CheckErrors( "after Geometry_ResizeIndexBuffer" );
}


void Geometry_Resize( SGeometry *geometry, uint vertexCount, uint indexCount )
{
	Prof_Start( PROF_GEOMETRY_RESIZE );

	OVR::GL_CheckErrors( "before Geometry_Resize" );

	Geometry_ResizeVertexBuffer( geometry, vertexCount );
	Geometry_ResizeIndexBuffer( geometry, indexCount );
	Geometry_MakeVertexArrayObject( geometry );

	OVR::GL_CheckErrors( "after Geometry_Resize" );

	Prof_Stop( PROF_GEOMETRY_RESIZE );
}


void Geometry_UpdateIndices( SGeometry *geometry, uint firstIndex, uint indexCount, const void *data )
{
	uint 	index;
	GLuint 	vertexArrayObject;
	GLuint 	indexBuffer;
	uint	offset;
	uint 	size;

	Prof_Start( PROF_GEOMETRY_UPDATE );

	OVR::GL_CheckErrors( "before Geometry_UpdateIndices" );

	index = geometry->updateIndex % BUFFER_COUNT;

	vertexArrayObject = geometry->vertexArrayObjects[index];
	assert( vertexArrayObject );

	glBindVertexArrayOES_( vertexArrayObject );

	indexBuffer = geometry->indexBuffers[index];
	assert( indexBuffer );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );

	offset = firstIndex * sizeof( ushort );
	size = indexCount * sizeof( ushort );

	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, offset, size, data );

	glBindVertexArrayOES_( 0 );

	OVR::GL_CheckErrors( "after Geometry_UpdateIndices" );

	Prof_Stop( PROF_GEOMETRY_UPDATE );
}


void Geometry_UpdateVertexPositions( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data )
{
	uint 	index;
	GLuint 	vertexArrayObject;
	GLuint 	vertexBuffer;
	uint 	offset;
	uint 	size;

	Prof_Start( PROF_GEOMETRY_UPDATE );

	OVR::GL_CheckErrors( "before Geometry_UpdateVertexPositions" );

	index = geometry->updateIndex % BUFFER_COUNT;

	vertexArrayObject = geometry->vertexArrayObjects[index];
	assert( vertexArrayObject );

	glBindVertexArrayOES_( vertexArrayObject );

	vertexBuffer = geometry->vertexBuffers[index];
	assert( vertexBuffer );

	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	offset = firstVertex * sizeof( float ) * 3;
	size = vertexCount * sizeof( float ) * 3;

	glBufferSubData( GL_ARRAY_BUFFER, offset, size, data );

	glBindVertexArrayOES_( 0 );

	OVR::GL_CheckErrors( "after Geometry_UpdateVertexPositions" );

	Prof_Stop( PROF_GEOMETRY_UPDATE );
}


void Geometry_UpdateVertexTexCoords( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data )
{
	uint 	index;
	GLuint 	vertexArrayObject;
	GLuint 	vertexBuffer;
	uint 	streamOffset;
	uint 	offset;
	uint 	size;

	Prof_Start( PROF_GEOMETRY_UPDATE );

	OVR::GL_CheckErrors( "before Geometry_UpdateVertexTexCoords" );

	index = geometry->updateIndex % BUFFER_COUNT;

	vertexArrayObject = geometry->vertexArrayObjects[index];
	assert( vertexArrayObject );

	glBindVertexArrayOES_( vertexArrayObject );

	vertexBuffer = geometry->vertexBuffers[index];
	assert( vertexBuffer );

	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	streamOffset = geometry->vertexCount * sizeof( float ) * 3;

	offset = streamOffset + firstVertex * sizeof( float ) * 2;
	size = vertexCount * sizeof( float ) * 2;

	glBufferSubData( GL_ARRAY_BUFFER, offset, size, data );

	glBindVertexArrayOES_( 0 );

	OVR::GL_CheckErrors( "after Geometry_UpdateVertexTexCoords" );

	Prof_Stop( PROF_GEOMETRY_UPDATE );
}


void Geometry_UpdateVertexColors( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data )
{
	uint 	index;
	GLuint 	vertexArrayObject;
	GLuint 	vertexBuffer;
	uint 	streamOffset;
	uint 	offset;
	uint 	size;

	Prof_Start( PROF_GEOMETRY_UPDATE );

	OVR::GL_CheckErrors( "before Geometry_UpdateVertexColors" );

	index = geometry->updateIndex % BUFFER_COUNT;

	vertexArrayObject = geometry->vertexArrayObjects[index];
	assert( vertexArrayObject );

	glBindVertexArrayOES_( vertexArrayObject );

	vertexBuffer = geometry->vertexBuffers[index];
	assert( vertexBuffer );

	glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	streamOffset = geometry->vertexCount * (sizeof( float ) * 3 + sizeof( float ) * 2);

	offset = streamOffset + firstVertex * sizeof( byte ) * 4;
	size = vertexCount * sizeof( byte ) * 4;

	glBufferSubData( GL_ARRAY_BUFFER, offset, size, data );

	glBindVertexArrayOES_( 0 );

	OVR::GL_CheckErrors( "after Geometry_UpdateVertexColors" );

	Prof_Stop( PROF_GEOMETRY_UPDATE );
}


void Geometry_Present( SGeometry *geometry )
{
	Prof_Start( PROF_GEOMETRY_PRESENT );

	assert( geometry );

	geometry->drawIndex = geometry->updateIndex;

	Prof_Stop( PROF_GEOMETRY_PRESENT );
}


void Geometry_Decommit( SGeometry *geometry )
{
	int 	index;

	geometry->drawIndex = 0;
	geometry->updateIndex = 0;

	for ( index = 0; index < BUFFER_COUNT; index++ )
	{
		if ( geometry->vertexBuffers[index] )
		{
			assert( geometry->vertexArrayObjects[index] );
			assert( geometry->indexBuffers[index] );

			glDeleteVertexArraysOES_( 1, &geometry->vertexArrayObjects[index] );
			glDeleteBuffers( 1, &geometry->vertexBuffers[index] );
			glDeleteBuffers( 1, &geometry->indexBuffers[index] );

			geometry->vertexBuffers[index] = 0;
			geometry->vertexArrayObjects[index] = 0;
			geometry->indexBuffers[index] = 0;
		}
	}
}
