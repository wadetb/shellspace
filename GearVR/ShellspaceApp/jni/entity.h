#ifndef __ENTITY_H__
#define __ENTITY_H__

struct SEntity;

void Entity_Init();
void Entity_Draw( const Matrix4f &view );

void Entity_RemoveFromParent();
void Entity_SetParent( SEntity *entity, SRef parentRef );

#endif
