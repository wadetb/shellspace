#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "registry.h"

void Texture_Resize( STexture *texture, uint width, uint height, SxTextureFormat format );
void Texture_Update( STexture *texture, uint x, uint y, uint width, uint height, const void *data );
void Texture_Present( STexture *texture );
void Texture_Decommit( STexture *texture );

#endif
