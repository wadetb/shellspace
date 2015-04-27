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
#ifndef __REFLIST_H__
#define __REFLIST_H__

#include "registry.h"

SRefLink *RefList_GetLink( void *t, uint linkOffset )
{
	return (SRefLink *)( (byte *)t + linkOffset );
}

template <typename T>
T *RefList_Get( SRef ref )
{
	assert( false );
	return S_NULL_REF;
}
template <>
SEntity *RefList_Get( SRef ref )
{
	return Registry_GetEntity( ref );
}

template <typename T>
SRef RefList_GetRef( T *t )
{
	assert( false );
}
template <>
SRef RefList_GetRef( SEntity *ent )
{
	return Registry_GetEntityRefByPointer( ent );
}

template <typename T>
void RefList_Insert( T* t, uint linkOffset, SRef *head )
{
	SRef 		first;
	SRef 		ref;
	SRefLink	*link;
	SEntity 	*next;
	SEntity 	*prev;
	SRefLink	*nextLink;
	SRefLink	*prevLink;

	first = *head;
	ref = RefList_GetRef( t );
	link = RefList_GetLink( t, linkOffset );

	assert( link->prev == S_NULL_REF );
	assert( link->next == S_NULL_REF );

	if ( first != S_NULL_REF )
	{
		next = RefList_Get<T>( first );
		nextLink = RefList_GetLink( next, linkOffset );
		nextLink->prev = ref; 
	}

	link->next = first;
	*head = ref;	
}

template <typename T>
void RefList_Remove( T* t, uint linkOffset, SRef *head )
{
	SRef 		ref;
	SRefLink	*link;
	SEntity 	*next;
	SEntity 	*prev;
	SRefLink	*nextLink;
	SRefLink	*prevLink;

	ref = RefList_GetRef( t );
	link = RefList_GetLink( t, linkOffset );

	if ( link->next != S_NULL_REF )
	{
		next = RefList_Get<T>( link->next );
		nextLink = RefList_GetLink( next, linkOffset );

		nextLink->prev = link->prev;
		link->next = S_NULL_REF;
	}

	if ( link->prev != S_NULL_REF )
	{
		prev = RefList_Get<T>( link->prev );
		prevLink = RefList_GetLink( prev, linkOffset );
		prevLink->next = link->next;
		link->prev = S_NULL_REF;
	}
	else
	{
		*head = ref;
	}
}

#endif
