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
#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

void Keyboard_Init();
void Keyboard_LoadKeyboards();
void Keyboard_Frame( uint buttonState, const Vector3f &eyePos, const Vector3f &eyeDir );
void Keyboard_Draw();
void Keyboard_Toggle();
bool Keyboard_IsVisible();

#endif
