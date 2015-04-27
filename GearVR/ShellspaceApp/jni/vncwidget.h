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
#ifndef __VNCWIDGET_H__
#define __VNCWIDGET_H__

#define INVALID_KEY_CODE 		(-1)

struct SVNCWidget;

SVNCWidget *VNC_CreateWidget();
void VNC_DestroyWidget( SVNCWidget *vnc );

void VNC_UpdateWidget( SVNCWidget *vnc );
void VNC_DrawWidget( SVNCWidget *vnc, const Matrix4f &view );

void VNC_Connect( SVNCWidget *vnc, const char *server, const char *password );
void VNC_Disconnect( SVNCWidget *vnc );

sbool VNC_IsConnected( SVNCWidget *vnc );

uint VNC_KeyCodeForAndroidCode( uint androidCode );

void VNC_KeyboardEvent( SVNCWidget *vnc, uint code, sbool down );
void VNC_MouseEvent( SVNCWidget *vnc, int x, int y, int buttons );

void VNC_UpdateHeadMouse( SVNCWidget *vnc, Vector3f eyePos, Vector3f eyeDir, sbool touch );

int VNC_GetTexID( SVNCWidget *vnc );

int VNC_GetWidth( SVNCWidget *vnc );
int VNC_GetHeight( SVNCWidget *vnc );
int VNC_GetTexWidth( SVNCWidget *vnc );
int VNC_GetTexHeight( SVNCWidget *vnc );

sbool VNC_Command( SVNCWidget *vnc );

#endif
