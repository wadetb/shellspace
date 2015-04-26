#ifndef __REFLIST_H__
#define __REFLIST_H__

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
	return Registry_GetEntityRef( ent );
}

template <typename T>
void RefList_Insert( T* t, uint linkOffset, SRefLink *head )
{
	SRef 		ref;
	SRefLink	*link;
	SEntity 	*next;
	SEntity 	*prev;
	SRefLink	*nextLink;
	SRefLink	*prevLink;

	ref = RefList_GetRef( t );
	link = RefList_GetLink( t, linkOffset );

	assert( link->prev == S_NULL_REF );
	assert( link->next == S_NULL_REF );

	if ( head->next != S_NULL_REF )
	{
		next = RefList_Get<T>( head->next );
		nextLink = RefList_GetLink( next, linkOffset );
		nextLink->prev = ref; 
	}

	link->next = head->next;
	head->next = ref;	
}

template <typename T>
void RefList_Remove( T* t, uint linkOffset, SRefLink *head )
{
	SRef 		ref;
	SRefLink	*link;
	SEntity 	*next;
	SEntity 	*prev;
	SRefLink	*nextLink;
	SRefLink	*prevLink;

	ref = RefList_GetRef( t );
	link = RefList_GetLink( t, linkOffset );

	assert( link->prev != S_NULL_REF );
	assert( link->next != S_NULL_REF );

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
		entity->activeLink.prev = S_NULL_REF;
	}
	else
	{
		head->next = ref;
	}
}

#endif
