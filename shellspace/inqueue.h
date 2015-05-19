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
#ifndef INQUEUE_H
#define INQUEUE_H

void InQueue_Frame();
void InQueue_ClearRefs( SRef ref );

void InQueue_ResizeTexture( SRef ref, uint width, uint height, SxTextureFormat format );
void InQueue_UpdateTextureRect( SRef ref, uint x, uint y, uint width, uint height, const void *data );
void InQueue_PresentTexture( SRef ref );

void InQueue_ResizeGeometry( SRef ref, uint vertexCount, uint indexCount );
void InQueue_UpdateGeometryIndices( SRef ref, uint firstIndex, uint indexCount, const void *data );
void InQueue_UpdateGeometryPositions( SRef ref, uint firstVertex, uint vertexCount, const void *data );
void InQueue_UpdateGeometryTexCoords( SRef ref, uint firstVertex, uint vertexCount, const void *data );
void InQueue_UpdateGeometryColors( SRef ref, uint firstVertex, uint vertexCount, const void *data );
void InQueue_PresentGeometry( SRef ref );

#endif
