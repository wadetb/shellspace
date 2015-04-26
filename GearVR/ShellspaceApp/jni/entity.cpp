#include "common.h"
#include "entity.h"
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
		"   oColor = /* VertexColor * */ UniformColor;\n"
		"}\n"
		,
		"#version 300 es\n"
		"uniform sampler2D Texture0;\n"
		"in  highp   vec2 oTexCoord;\n"
		"in  lowp    vec4 oColor;\n"
		"out mediump vec4 fragColor;\n"
		"void main()\n"
		"{\n"
		"	fragColor = texture( Texture0, oTexCoord );\n"
		"}\n"
		);
}


void Entity_Activate( SEntity *entity, sbool active )
{
	entity->active = active;
}


void Entity_DrawEntity( SEntity *entity, const Matrix4f &view )
{
	STexture 	*texture;
	SGeometry	*geometry;
	int 		triCount;
	int 		indexOffset;
	int 		batchTriCount;
	int 		triCountLeft;

	Prof_Start( PROF_DRAW_ENTITY );

	assert( entity );

	geometry = Registry_GetGeometry( entity->geometryRef );
	assert( geometry );

	texture = Registry_GetTexture( entity->textureRef );
	assert( texture );

	glUseProgram( s_ent.shader.program );

	glUniformMatrix4fv( s_ent.shader.uMvp, 1, GL_FALSE, view.Transposed().M[0] );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texture->texId[texture->drawIndex % BUFFER_COUNT] );

	glBindVertexArrayOES_( geometry->vertexArrayObjects[geometry->drawIndex % BUFFER_COUNT] );

	indexOffset = 0;
	triCount = geometry->indexCount / 3;
	triCountLeft = triCount;

	while ( triCountLeft )
	{
#if USE_SPLIT_DRAW
		batchTriCount = S_Min( triCountLeft, triCount / 10 );
#else // #if USE_SPLIT_DRAW
		batchTriCount = triCount;
#endif // #else // #if USE_SPLIT_DRAW

		glDrawElements( GL_TRIANGLES, batchTriCount * 3, GL_UNSIGNED_SHORT, (void *)indexOffset );

		indexOffset += batchTriCount * sizeof( ushort ) * 3;
		triCountLeft -= batchTriCount;
	}

	glBindTexture( GL_TEXTURE_2D, 0 );
	glBindVertexArrayOES_( 0 );

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

		// if ( !entity->active )
		// 	continue;

		Entity_DrawEntity( entity, view );
	}
}
