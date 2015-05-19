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
#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "registry.h"

enum
{
	VERTEX_ATTRIBUTE_POSITION 	= 0,
	VERTEX_ATTRIBUTE_TEXCOORD 	= 5,
	VERTEX_ATTRIBUTE_COLOR 		= 4
};

void Geometry_Resize( SGeometry *geometry, uint vertexCount, uint indexCount );
void Geometry_UpdateIndices( SGeometry *geometry, uint firstIndex, uint indexCount, const void *data );
void Geometry_UpdateVertexPositions( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_UpdateVertexTexCoords( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_UpdateVertexColors( SGeometry *geometry, uint firstVertex, uint vertexCount, const void *data );
void Geometry_Present( SGeometry *geometry );
void Geometry_Decommit( SGeometry *geometry );

#endif
