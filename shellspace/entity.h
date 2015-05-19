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
#ifndef __ENTITY_H__
#define __ENTITY_H__

struct SEntity;

void Entity_Init();
void Entity_Draw( const OVR::Matrix4f &view );

void Entity_Register( SEntity *entity );
void Entity_Unregister( SEntity *entity );

void Entity_SetParent( SEntity *entity, SRef parentRef );

sbool Entity_Command();

#endif
