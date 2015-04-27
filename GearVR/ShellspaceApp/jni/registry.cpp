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
#include "registry.h"

#define MAX_PLUGINS			32
#define MAX_WIDGETS			512
#define MAX_GEOMETRIES		256
#define MAX_TEXTURES		256
#define MAX_ENTITIES		2048

#define HASH_MULTIPLIER 	3
#define INVALID_HASH		0xffff

#define S_HEAD_REF			0


struct SRegistry
{
	const char 	**names;
	ushort 		*hash;
	uint 		hashSize;
};


struct SPool
{
	void 		*entries;
	uint 		entrySize;
	uint 		count;
	uint 		limit;
};


SRegistry s_reg[REGISTRY_COUNT];
SPool s_pool[REGISTRY_COUNT];


void Registry_InitRegistry( SRegistry *registry, uint limit )
{
	uint hashSize;

	assert( limit < USHRT_MAX );

	hashSize = limit * HASH_MULTIPLIER;

	registry->names = (const char **)malloc( limit * sizeof( char * ) );
	assert( registry->names );

	registry->hash = (ushort *)malloc( hashSize * sizeof( ushort ) );
	assert( registry->hash );

	memset( registry->hash, 0xff, hashSize * sizeof( ushort ) );

	registry->hashSize = hashSize;
}


void Registry_InitPool( SPool *pool, uint entrySize, uint limit )
{
	void 		*entries;
	uint	 	entryIter;
	SRefLink	*link;

	assert( limit < S_NULL_REF );

	entries = malloc( (limit + 1) * entrySize );
	assert( entries );

	memset( entries, 0, (limit + 1) * entrySize );

	// The first entry is reserved as the free list head.
	pool->entries = entries;
	pool->entrySize = entrySize;
	pool->count = 0;
	pool->limit = limit;

	// The free list link is the first member in the entry.
	link = (SRefLink *)pool->entries;

	for ( entryIter = 0; entryIter < limit; entryIter++ )
	{
		link->prev = (entryIter + limit - 1) % limit;
		link->next = (entryIter + 1) % limit;
		link = (SRefLink *)( (byte *)link + entrySize );
	}
}


void Registry_Init()
{
	Registry_InitRegistry( &s_reg[PLUGIN_REGISTRY], MAX_PLUGINS );
	Registry_InitRegistry( &s_reg[WIDGET_REGISTRY], MAX_WIDGETS );
	Registry_InitRegistry( &s_reg[GEOMETRY_REGISTRY], MAX_GEOMETRIES );
	Registry_InitRegistry( &s_reg[TEXTURE_REGISTRY], MAX_TEXTURES );
	Registry_InitRegistry( &s_reg[ENTITY_REGISTRY], MAX_ENTITIES );

	Registry_InitPool( &s_pool[PLUGIN_REGISTRY], sizeof( SPlugin ), MAX_PLUGINS );
	Registry_InitPool( &s_pool[WIDGET_REGISTRY], sizeof( SWidget ), MAX_WIDGETS );
	Registry_InitPool( &s_pool[GEOMETRY_REGISTRY], sizeof( SGeometry ), MAX_GEOMETRIES );
	Registry_InitPool( &s_pool[TEXTURE_REGISTRY], sizeof( STexture ), MAX_TEXTURES );
	Registry_InitPool( &s_pool[ENTITY_REGISTRY], sizeof( SEntity ), MAX_ENTITIES );
}


void Registry_Shutdown()
{
	uint regIter;

	for ( regIter = 0; regIter < REGISTRY_COUNT; regIter++ )
	{
		free( s_reg[regIter].names );
		free( s_reg[regIter].hash );
		free( s_pool[regIter].entries );
	}
}


SRef Registry_Get( ERegistry reg, const char *id )
{
	SRegistry 	*r;
	SRef 		*hash;
	const char 	**names;
	uint 		hashSize;
	uint 		slot;
	SRef 		hashRef;

	r = &s_reg[reg];

	names = r->names;
	hash = r->hash;
	hashSize = r->hashSize;

    slot = S_FNV32( id, 0 ) % hashSize;

    for ( ;; )
    {
        hashRef = hash[slot];

        if ( hashRef == S_NULL_REF )
            return hashRef; // not found

        if ( S_strcmp( names[hashRef], id ) == 0 )
            return hashRef;

        slot++;
        if ( slot == hashSize )
        	slot = 0;
    }
}


void Registry_Add( ERegistry reg, const char *id, SRef ref )
{
	SRegistry 	*r;
	SRef 		*hash;
	const char 	**names;
	uint 		hashSize;
	uint 		slot;
	SRef 		hashRef;

	assertindex( ref, s_pool[reg].limit );

	r = &s_reg[reg];

	names = r->names;
	hash = r->hash;
	hashSize = r->hashSize;

    slot = S_FNV32( id, 0 ) % hashSize;

    for ( ;; )
    {
        hashRef = hash[slot];

        if ( hashRef == S_NULL_REF )
        {
        	names[ref] = id;
        	hash[slot] = ref;
            return;
        }

        assert( S_strcmp( names[hashRef], id ) ); // id already registered

        slot++;
        if ( slot == hashSize )
        	slot = 0;
    }	
}


void Registry_Remove( ERegistry reg, SRef ref )
{
	// $$$ TODO
}


SRefLink *Registry_GetHead( ERegistry reg )
{
	SPool 		*pool;

	pool = &s_pool[reg];

	return (SRefLink *)pool->entries;
}


SRefLink *Registry_GetLink( ERegistry reg, SRef ref )
{
	SPool 		*pool;
	
	pool = &s_pool[reg];

	assertindex( ref, pool->limit );

	return (SRefLink *)( (byte *)pool->entries + ref * pool->entrySize );
}


SRef Registry_Alloc( ERegistry reg )
{
	SRefLink	*head;
	SRef 		ref;
	SRefLink	*alloc;
	SRefLink	*next;

	head = Registry_GetHead( reg );

	if ( head->next == head->prev )
	{
		assert( s_pool[reg].count == s_pool[reg].limit );
		return S_NULL_REF;
	}

	ref = head->next;

	alloc = Registry_GetLink( reg, ref );
	next = Registry_GetLink( reg, alloc->next );

	next->prev = alloc->prev;
	head->next = alloc->next;

	alloc->prev = S_NULL_REF;
	alloc->next = S_NULL_REF;

	s_pool[reg].count++;

	return ref;
}


sbool Registry_IsAllocated( ERegistry reg, SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( reg, ref );

	if ( link->prev == S_NULL_REF )
	{
		assert( link->next == S_NULL_REF );
		return strue;
	}

	return sfalse;
}


SRef Registry_Free( ERegistry reg, SRef ref )
{
	SRefLink	*head;
	SRefLink	*alloc;
	SRefLink	*next;

	assert( Registry_IsAllocated( reg, ref ) );

	alloc = Registry_GetLink( reg, ref );

	head = Registry_GetHead( reg );
	next = Registry_GetLink( reg, head->next );

	alloc->prev = S_HEAD_REF;
	alloc->next = head->next;

	next->prev = ref;
	head->next = ref;

	s_pool[reg].count--;

	return ref;
}


SRef Registry_Register( ERegistry reg, const char *id )
{
	SRef 	ref;

	if ( Registry_Get( reg, id ) != S_NULL_REF )
		return S_NULL_REF;

	ref = Registry_Alloc( reg );
	if ( ref == S_NULL_REF )
		return S_NULL_REF;

	Registry_Add( reg, id, ref );

	return ref;
}


void Registry_Unregister( ERegistry reg, SRef ref )
{
	Registry_Free( reg, ref );
	Registry_Remove( reg, ref );
}


SRef Registry_GetPluginRef( const char *id )
{
	return Registry_Get( PLUGIN_REGISTRY, id );
}


SRef Registry_GetWidgetRef( const char *id )
{
	return Registry_Get( WIDGET_REGISTRY, id );
}


SRef Registry_GetGeometryRef( const char *id )
{
	return Registry_Get( GEOMETRY_REGISTRY, id );
}


SRef Registry_GetTextureRef( const char *id )
{
	return Registry_Get( TEXTURE_REGISTRY, id );
}


SRef Registry_GetEntityRef( const char *id )
{
	return Registry_Get( ENTITY_REGISTRY, id );
}


SPlugin *Registry_GetPlugin( SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( PLUGIN_REGISTRY, ref );

	return (SPlugin *)link;
}


SWidget *Registry_GetWidget( SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( WIDGET_REGISTRY, ref );

	return (SWidget *)link;
}


SGeometry *Registry_GetGeometry( SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( GEOMETRY_REGISTRY, ref );

	return (SGeometry *)link;
}


STexture *Registry_GetTexture( SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( TEXTURE_REGISTRY, ref );

	return (STexture *)link;
}


SEntity *Registry_GetEntity( SRef ref )
{
	SRefLink	*link;

	link = Registry_GetLink( ENTITY_REGISTRY, ref );

	return (SEntity *)link;
}


uint Registry_GetCount( ERegistry reg )
{
	return s_pool[reg].count;
}


SRef Registry_GetEntityRefByPointer( SEntity *entity )
{
	ptrdiff_t 	offset;
	uint 		index;

	offset = (byte *)entity - (byte *)s_pool[ENTITY_REGISTRY].entries;
	assert( offset % sizeof( SEntity ) == 0 );

	index = offset / sizeof( SEntity );
	assertindex( index, s_pool[ENTITY_REGISTRY].count );

	return (SRef)index;
}

