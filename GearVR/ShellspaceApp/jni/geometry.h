#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "registry.h"

enum
{
	VERTEX_ATTRIBUTE_POSITION 	= 0,
	VERTEX_ATTRIBUTE_TEXCOORD 	= 1,
	VERTEX_ATTRIBUTE_COLOR 		= 2
};

void Geometry_Resize( SGeometry *geometry, uint vertexCount, uint indexCount );
void Geometry_UpdateIndices( SGeometry *geometry, uint firstIndex, uint indexCount, const void *data );
void Geometry_UpdateVertexPositions( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_UpdateVertexTexCoords( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_UpdateVertexColors( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_Present( SGeometry *geometry );
void Geometry_Decommit( SGeometry *geometry );

#endif
