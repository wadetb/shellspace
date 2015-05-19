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
#ifndef __PROFILE_H__
#define __PROFILE_H__

enum EProfType
{
	PROF_FRAME,
	PROF_CMD,
	PROF_KEYBOARD,
	PROF_SCENE,
	PROF_DRAW,
	PROF_DRAW_EYE,
	PROF_DRAW_KEYBOARD,
	PROF_VNC_THREAD,
	PROF_VNC_THREAD_INPUT,
	PROF_VNC_THREAD_WAIT,
	PROF_VNC_THREAD_HANDLE,
	PROF_VNC_THREAD_HANDLE_RESIZE,
	PROF_VNC_THREAD_HANDLE_UPDATE,
	PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES,
	PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE,
	PROF_VNC_THREAD_HANDLE_CURSOR_POS,
	PROF_VNC_THREAD_UPDATE_TEXTURE_RECT,
	PROF_RFB_UPDATE,
	PROF_RFB_ENCODING_RAW,
	PROF_RFB_ENCODING_COPY_RECT,
	PROF_RFB_ENCODING_RRE,
	PROF_RFB_ENCODING_CO_RRE,
	PROF_RFB_ENCODING_HEXTILE,
	PROF_RFB_ENCODING_ULTRA,
	PROF_RFB_ENCODING_ULTRA_ZIP,
	PROF_RFB_ENCODING_LIBZ,
	PROF_RFB_ENCODING_TIGHT,
	PROF_RFB_ENCODING_TIGHT_JPEG,
	PROF_RFB_ENCODING_ZYWRLE,
	PROF_RFB_ENCODING_H264,
	PROF_VLC_THREAD,
	PROF_GPU_UPDATE,
	PROF_GPU_UPDATE_APPEND,
	PROF_TEXTURE_RESIZE,
	PROF_TEXTURE_UPDATE,
	PROF_TEXTURE_PRESENT,
	PROF_GEOMETRY_RESIZE,
	PROF_GEOMETRY_UPDATE,
	PROF_GEOMETRY_PRESENT,
	PROF_DRAW_ENTITY,
	PROF_COUNT
};

double Prof_MS();

void Prof_Start( EProfType prof );
void Prof_Stop( EProfType prof );
void Prof_Frame();

struct Prof_Scope
{
	EProfType savedProf;
	Prof_Scope( EProfType prof );
	~Prof_Scope();
};

#endif
