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
#include "entity.h"
#include "reflist.h"
#include "registry.h"
#include <GlProgram.h>


struct SEntityGlobals
{
	GlProgram 	shader;
};


SEntityGlobals	s_ent;


void Entity_Init()
{
	int err;

	// $$$ Shader needs to handle texture Pow2 scale.
	s_ent.shader = BuildProgram(
		"#version 300 es\n"
		"uniform mediump mat4 Mvpm;\n"
		"in vec4 Position;\n"
		"in vec4 VertexColor;\n"
		"in vec2 TexCoord;\n"
		"uniform mediump vec4 UniformColor;\n"
		"out  lowp vec4 oColor;\n"
		"out highp vec2 oTexCoord;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = Mvpm * Position;\n"
		"	oTexCoord = TexCoord;\n"
		"   oColor = VertexColor;\n"
		"}\n"
		,
		"#version 300 es\n"
		"uniform sampler2D Texture0;\n"
		"in  highp   vec2 oTexCoord;\n"
		"in  lowp    vec4 oColor;\n"
		"out mediump vec4 fragColor;\n"
		"void main()\n"
		"{\n"
		"	fragColor = oColor * texture( Texture0, oTexCoord );\n"
		"}\n"
		);
}


void Entity_DrawEntity( SEntity *entity, const Matrix4f &view )
{
	STexture 	*texture;
	SGeometry	*geometry;
	uint 		geometryIndex;
	uint 		textureIndex;
	GLuint 		texId;
	GLuint 		vertexArrayObject;
	int 		triCount;
	int 		indexOffset;
	int 		batchTriCount;
	int 		triCountLeft;

	Prof_Start( PROF_DRAW_ENTITY );

	assert( entity );

	GL_CheckErrors( "before Entity_DrawEntity" );

	geometry = Registry_GetGeometry( entity->geometryRef );
	assert( geometry );

	geometryIndex = geometry->drawIndex % BUFFER_COUNT;

	vertexArrayObject = geometry->vertexArrayObjects[geometryIndex];
	if ( !vertexArrayObject )
	{
		Prof_Stop( PROF_DRAW_ENTITY );
		return;
	}

	glUseProgram( s_ent.shader.program );

	glUniform4f( s_ent.shader.uColor, 1.0f, 1.0f, 1.0f, 1.0f );
	glUniformMatrix4fv( s_ent.shader.uMvp, 1, GL_FALSE, view.Transposed().M[0] );

	glActiveTexture( GL_TEXTURE0 );

	if ( entity->textureRef != S_NULL_REF )
	{
		texture = Registry_GetTexture( entity->textureRef );
		assert( texture );

		textureIndex = texture->drawIndex % BUFFER_COUNT;
		texId = texture->texId[textureIndex];

		glBindTexture( GL_TEXTURE_2D, texId );
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

	glBindVertexArrayOES_( vertexArrayObject );

	indexOffset = 0;
	triCount = geometry->indexCounts[geometryIndex] / 3;
	triCountLeft = triCount;

	while ( triCountLeft )
	{
#if USE_SPLIT_DRAW
		batchTriCount = S_Min( triCountLeft, S_Max( 1, triCount / 10 ) );
#else // #if USE_SPLIT_DRAW
		batchTriCount = triCount;
#endif // #else // #if USE_SPLIT_DRAW

		glDrawElements( GL_TRIANGLES, batchTriCount * 3, GL_UNSIGNED_SHORT, (void *)indexOffset );

		indexOffset += batchTriCount * sizeof( ushort ) * 3;
		triCountLeft -= batchTriCount;
	}

	glBindVertexArrayOES_( 0 );

	glBindTexture( GL_TEXTURE_2D, 0 );

	GL_CheckErrors( "after Entity_DrawEntity" );

	Prof_Stop( PROF_DRAW_ENTITY );
}


void Entity_Draw( const Matrix4f &view )
{
	uint 		entityCount;
	uint 		entityIter;
	SEntity 	*entity;

	// $$$ The way this will work is that there will be single hardcoded "root" entity
	//  and all other entities will be parented to it.  This code will then recursively 
	//  traverse entity children, starting with the root entity, doing bounding box
	//  culling to prune.  
	// The resulting entities can be drawn immediately or stored in a list for sorting,
	//  if that's beneficial to performance.
	entityCount = Registry_GetCount( ENTITY_REGISTRY );

	for ( entityIter = 0; entityIter < entityCount; entityIter++ )
	{
		entity = Registry_GetEntity( Registry_RefForIndex( entityIter ) );
		assert( entity );

		if ( entity->visibility <= 0.0f )
			continue;

		Entity_DrawEntity( entity, view );
	}
}


void Entity_SetParent( SEntity *entity, SRef parentRef )
{
	SEntity *parent;

	if ( entity->parentRef != S_NULL_REF )
	{
		parent = Registry_GetEntity( entity->parentRef );
		assert( parent );

		RefList_Remove( entity, offsetof( SEntity, parentLink ), &parent->firstChild );
	}

	entity->parentRef = parentRef;

	if ( parentRef != S_NULL_REF )
	{
		parent = Registry_GetEntity( parentRef );
		assert( parent );

		RefList_Insert( entity, offsetof( SEntity, parentLink ), &parent->firstChild );
	}
}

