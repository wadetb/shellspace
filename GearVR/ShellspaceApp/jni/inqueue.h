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
