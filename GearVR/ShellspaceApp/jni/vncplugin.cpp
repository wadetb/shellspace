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
#include "vncplugin.h"
#include "command.h"
#include "message.h"
#include "thread.h"
#include "libvncserver/rfb/rfbclient.h"
#include "../../../Common/shellspace.h"
#include <android/keycodes.h>


#define VNC_WIDGET_LIMIT 			16

#define AKEYCODE_UNKNOWN 			(UINT_MAX)
#define INVALID_KEY_CODE            (-1)


enum EVNCState
{
	VNCSTATE_CONNECTING,
	VNCSTATE_CONNECTED,
	VNCSTATE_DISCONNECTED
};


#define COPY_TO_BACKUP 		0
#define COPY_FROM_BACKUP 	1
#define COPY_FROM_CURSOR 	2

#define CURSOR_BUFFER 		0
#define CURSOR_BACKUP	 	1


struct SVNCCursor
{
	byte 				*buffer;
	byte 				*backup;
	uint 				xPos;
	uint 				yPos;
	uint 				xHot;
	uint 				yHot;
	uint 				width;
	uint 				height;
	sbool 				needRestore;
};


struct SVNCWidget
{
	SxWidgetHandle 		id;

	pthread_t 			thread;
	volatile sbool 		disconnect;

	rfbClient    		*client;
	char 				*server;
	char 				*password;

	EVNCState			state;

	SVNCCursor			cursor;

	int 				width;
	int 				height;

	float	 			globeFov;
	float 				globeRadius;
	float 				globeZ;
	float 				globeSize;
	Vector3f 			globeCenter;

	SxTextureHandle 	textureId;	
	SxGeometryHandle 	geometryId;	
};


struct SVNCKeyMap
{
	const char 			*name;
	uint 				androidCode;
	uint 				vncCode;
};


struct SVNCGlobals
{
	sbool 				initialized;

	sbool 				headmouse;

	SVNCWidget 			widgetPool[VNC_WIDGET_LIMIT];

	pthread_t 			pluginThread;
};


static SVNCGlobals s_vncGlob;


static void rfb_log(const char *format, ...)
{
    va_list args;
    char msg[256];

    va_start( args, format );
    vsnprintf( msg, sizeof( msg ) - 1, format, args );
    va_end( args );

    LOG( msg );
}


static void rfb_error(const char *format, ...)
{
    va_list args;
    char msg[256];

    va_start( args, format );
    vsnprintf( msg, sizeof( msg ) - 1, format, args );
    va_end( args );

    LOG( msg );

    Cmd_Add( "notify \"%s\"", msg );
}


static char *vnc_thread_get_password( rfbClient *client )
{
	SVNCWidget 	*vnc;

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	return strdup( vnc->password );
}


static void VNCThread_RebuildGlobe( SVNCWidget *vnc )
{
	assert( vnc->width );
	assert( vnc->height );

	float aspect = (float)vnc->width / vnc->height;

	const int horizontal = 64;
	const int vertical = 32;

	const float fov = vnc->globeFov * M_PI / 180.0f;

	// $$$ TODO- handle 0 fov with a special case
	const float yExtent = sinf( 0.5f * fov );
	const float zExtent = cosf( 0.5f * fov ) * cosf( 0.5f * fov * aspect );

	const float scale = 1.0f / yExtent;
	const float zOffset = (scale * zExtent);

	const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );
	const int indexCount = horizontal * vertical * 6;

	g_pluginInterface.sizeGeometry( vnc->geometryId, vertexCount, indexCount );

	SxVector3 *positions = (SxVector3 *)malloc( vertexCount * sizeof( SxVector3 ) );
	SxVector2 *texCoords = (SxVector2 *)malloc( vertexCount * sizeof( SxVector2 ) );
	SxColor *colors = (SxColor *)malloc( vertexCount * sizeof( SxColor ) );

	for ( int y = 0; y <= vertical; y++ )
	{
		float yf;
		yf = (float) y / vertical;
		const float lat = ( yf - 0.5f ) * fov;
		const float cosLat = cosf( lat );
		const float sinLat = sinf( lat );
		for ( int x = 0; x <= horizontal; x++ )
		{
			const float xf = (float)x / (float)horizontal;
			const float lon = 1.5f*M_PI + ( xf - 0.5f ) * fov * aspect;
			const int index = y * ( horizontal + 1 ) + x;

			positions[index].x = scale * cosf( lon ) * cosLat;
			positions[index].y = scale * sinLat;
			positions[index].z = (scale * sinf( lon ) * cosLat) + zOffset;

			texCoords[index].x = xf;
			texCoords[index].y = 1.0f - yf;

			colors[index].r = 255;
			colors[index].g = 255;
			colors[index].b = 255;
			colors[index].a = 255;
		}
	}

	g_pluginInterface.updateGeometryPositionRange( vnc->geometryId, 0, vertexCount, positions );
	g_pluginInterface.updateGeometryTexCoordRange( vnc->geometryId, 0, vertexCount, texCoords );
	g_pluginInterface.updateGeometryColorRange( vnc->geometryId, 0, vertexCount, colors );

	free( positions );
	free( texCoords );
	free( colors );

	ushort *indices = (ushort *)malloc( indexCount * sizeof( ushort ) );

	int index = 0;
	for ( int x = 0; x < horizontal; x++ )
	{
		for ( int y = 0; y < vertical; y++ )
		{
			indices[index + 0] = y * (horizontal + 1) + x;
			indices[index + 1] = y * (horizontal + 1) + x + 1;
			indices[index + 2] = (y + 1) * (horizontal + 1) + x;
			indices[index + 3] = (y + 1) * (horizontal + 1) + x;
			indices[index + 4] = y * (horizontal + 1) + x + 1;
			indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
			index += 6;
		}
	}

	g_pluginInterface.updateGeometryIndexRange( vnc->geometryId, 0, indexCount, indices );

	free( indices );

	g_pluginInterface.presentGeometry( vnc->geometryId );

	vnc->globeCenter.x = 0.0f;
	vnc->globeCenter.y = 0.0f;
	vnc->globeCenter.z = zOffset - scale;
}


static rfbBool vnc_thread_resize( rfbClient *client ) 
{
	SVNCWidget 		*vnc;
	int 			width;
	int 			height;

	Prof_Start( PROF_VNC_THREAD_HANDLE_RESIZE );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	width = client->width;
	height = client->height;

	LOG( "Requested client dimensions: %dx%d", width, height );

	client->updateRect.x = 0;
	client->updateRect.y = 0;
	client->updateRect.w = width; 
	client->updateRect.h = height;

	LOG( "Frame buffer size          : %d", width * height * 4 );

	if ( client->frameBuffer )
		free( client->frameBuffer );

	client->frameBuffer = (uint8_t *)malloc( width * height * 4 );
	if ( !client->frameBuffer )
		FAIL( "Failed to allocate %dx%dx32bpp client frame buffer.", width, height );

	LOG( "Frame buffer address       : %p", client->frameBuffer );

	client->format.bitsPerPixel = 32;
	client->format.redShift = 0;
	client->format.greenShift = 8;
	client->format.blueShift = 16;
	client->format.redMax = 255;
	client->format.greenMax = 255;
	client->format.blueMax = 255;

	SetFormatAndEncodings( client );

	g_pluginInterface.formatTexture( vnc->textureId, SxTextureFormat_R8G8B8A8_SRGB );
	g_pluginInterface.sizeTexture( vnc->textureId, width, height );

	vnc->width = width;
	vnc->height = height;

	VNCThread_RebuildGlobe( vnc );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_RESIZE );

	return TRUE;
}


#if USE_OVERLAY

void VNCThread_SetAlpha( SVNCWidget *vnc, int x, int y, int w, int h )
{
	rfbClient 	*client;
	uint 		xi;
	uint 		yi;
	uint 		stride;
	byte 		*data;

	client = vnc->client;
	assert( client );

	assert( client->frameBuffer );
	data = client->frameBuffer + (y * client->width + x) * 4;

	stride = (client->width - w) * 4;

	// Fill background alpha with 1s.
	for ( yi = 0; yi < h; yi++ )
	{
		for ( xi = 0; xi < w; xi++ )
		{
			data[3] = 0xff;
			data += 4;
		}
		data += stride;
	}

	// Fill border edge alpha with 0s.
	if ( x == 0 )
	{
		data = client->frameBuffer + (y * client->width + x) * 4;
		stride = client->width * 4;

		for ( yi = 0; yi < h; yi++ )
		{
			data[3] = 0;
			data += stride;
		}
	}

	if ( x + w == client->width )
	{
		data = client->frameBuffer + (y * client->width + x + w - 1) * 4;
		stride = client->width * 4;

		for ( yi = 0; yi < h; yi++ )
		{
			data[3] = 0;
			data += stride;
		}
	}

	if ( y == 0 )
	{
		data = client->frameBuffer + x * 4;

		for ( xi = 0; xi < w; xi++ )
		{
			data[3] = 0;
			data += 4;
		}
	}

	if ( y + h == client->height )
	{
		data = client->frameBuffer + ((y + h - 1) * client->width + x) * 4;

		for ( xi = 0; xi < w; xi++ )
		{
			data[3] = 0;
			data += 4;
		}
	}
}

#endif // #if USE_OVERLAY


void VNCThread_UpdateTextureRect( SVNCWidget *vnc, int x, int y, int width, int height )
{
	rfbClient 		*client;
	byte 	 		*buffer;
	int 			yc;
	byte 			*frameBuffer;
	uint 			frameBufferWidth;

	Prof_Start( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );

	client = vnc->client;
	assert( client );

	if ( x < 0 )
	{
		width += x;
		x = 0;
	}
	if ( y < 0 )
	{
		height += y;
		y = 0;
	}

	if ( x + width > client->width )
		width = client->width - x;
	if ( y + height > client->height )
		height = client->height - y;

	if ( width == 0 || height == 0 )
	{
		Prof_Stop( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );
		return;
	}

	frameBuffer = client->frameBuffer;
	frameBufferWidth = client->width;

	// $$$ This should be throttled such that no single block can cost more than ~1ns on the update thread.
	// (Once throttling is working on the update thread, that is)
	buffer = (byte *)malloc( width * height * 4 );
	assert( buffer );

	for ( yc = 0; yc < height; yc++ )
		memcpy( &buffer[(yc * width) * 4], &frameBuffer[((y + yc) * frameBufferWidth + x) * 4], width * 4 );

	g_pluginInterface.updateTextureRect( vnc->textureId, x, y, width, height, width * 4, buffer );

	free( buffer );

	Prof_Stop( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );
}


void vnc_thread_update( rfbClient *client, int x, int y, int w, int h )
{
	SVNCWidget 		*vnc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_UPDATE );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

#if USE_OVERLAY
	VNCThread_SetAlpha( vnc, x, y, w, h );
#endif // #if USE_OVERLAY

	VNCThread_UpdateTextureRect( vnc, x, y, w, h );

#if STRESS_RESIZE
	static int delay = 0;
	if ( ++delay == 1000 )
	{
		vnc_thread_resize( client );
		delay = 0;
	}
#endif // #if STRESS_RESIZE

	Prof_Stop( PROF_VNC_THREAD_HANDLE_UPDATE );
}


void vnc_thread_finished_updates( rfbClient *client )
{
	SVNCWidget 		*vnc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	g_pluginInterface.presentTexture( vnc->textureId );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES );
}


sbool VNCThread_RectOverlapsCursor( SVNCWidget *vnc, int x, int y, int width, int height )
{
	int 	cursorX;
	int 	xMin;
	int 	xMax;
	int 	cursorY;
	int 	yMin;
	int 	yMax;

	assert( vnc );

	if ( !vnc->cursor.buffer )
		return sfalse;

	cursorX = vnc->cursor.xPos - vnc->cursor.xHot;
	xMin = S_Min( x + width, cursorX + vnc->cursor.width );
	xMax = S_Max( x, cursorX );

	if ( xMax > xMin )
		return sfalse;

	cursorY = vnc->cursor.yPos - vnc->cursor.yHot;
	yMin = S_Min( y + height, cursorY + vnc->cursor.width );
	yMax = S_Max( y, cursorY );

	if ( yMax > yMin )
		return sfalse;

	return strue;
}


void VNCThread_CopyCursor( SVNCWidget *vnc, uint direction )
{
	uint 	width;
	uint 	height;
	byte 	*frameBuffer;
	uint 	cursorWidth;
	byte 	*cursorData;
	uint 	xStart;
	uint 	xEnd;
	uint 	yStart;
	uint 	yEnd;
	uint 	xOfs;
	uint 	yOfs;
	uint 	x;
	uint 	y;
	uint 	cx;
	uint 	cy;
	byte 	color[4];

	width = vnc->client->width;
	height = vnc->client->height;
	frameBuffer = vnc->client->frameBuffer;
	assert( frameBuffer );

	cursorWidth = vnc->cursor.width;
	cursorData = (direction == COPY_FROM_CURSOR) ? vnc->cursor.buffer : vnc->cursor.backup;
	if ( !cursorData )
		return;

	xStart = vnc->cursor.xPos - vnc->cursor.xHot;
	yStart = vnc->cursor.yPos - vnc->cursor.yHot;

	xEnd = xStart + vnc->cursor.width;
	yEnd = yStart + vnc->cursor.height;

	xOfs = 0;
	yOfs = 0;

	if ( xStart < 0 ) 
	{
		xOfs = -xStart;
		xStart = 0;
	}

	if ( yStart < 0 ) 
	{
		yOfs = -yStart;
		yStart = 0;
	}

	if ( xEnd > width ) 
		xEnd = width;
	if ( yEnd > height ) 
		yEnd = height;

	if ( xStart == xEnd || yStart == yEnd )
		return;

	if ( direction == COPY_FROM_CURSOR )
	{
		for ( y = yStart, cy = yOfs; y < yEnd; y++, cy++ )
		{
			for ( x = xStart, cx = xOfs; x < xEnd; x++, cx++ )
			{
				memcpy( color, &cursorData[(cy * cursorWidth + cx) * 4], 4 );
				if ( color[3] )
					memcpy( &frameBuffer[(y * width + x) * 4], color, 4 ); 
			}
		}
	}
	else if ( direction == COPY_FROM_BACKUP )
	{
		for ( y = yStart, cy = yOfs; y < yEnd; y++, cy++ )
			memcpy( &frameBuffer[(y * width + xStart) * 4], &cursorData[(cy * cursorWidth + xOfs) * 4], (xEnd - xStart) * 4 );
	}
	else if ( direction == COPY_TO_BACKUP )
	{
		for ( y = yStart, cy = yOfs; y < yEnd; y++, cy++ )
			memcpy( &cursorData[(cy * cursorWidth + xOfs) * 4], &frameBuffer[(y * width + xStart) * 4], (xEnd - xStart) * 4 ); 
	}
}


void VNCThread_UpdateCursorTextureRect( SVNCWidget *vnc )
{
	int x;
	int y;
	int width;
	int height;

	x = vnc->cursor.xPos - vnc->cursor.xHot;
	y = vnc->cursor.yPos - vnc->cursor.yHot;
	width = vnc->cursor.width;
	height = vnc->cursor.height;

	VNCThread_UpdateTextureRect( vnc, x, y, width, height );
}


void vnc_thread_got_cursor_shape( rfbClient *client, int xhot, int yhot, int width, int height, int bytesPerPixel )
{
	SVNCWidget 		*vnc;
	byte 			maskPixel;
	int 			x;
	int 			y;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	if ( !vnc->client->frameBuffer )
	{
		LOG( "CursorShape message without preceding resize message." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );
		return;
	}

	if ( vnc->cursor.buffer )
	{
		assert( vnc->cursor.backup );

		VNCThread_CopyCursor( vnc, COPY_FROM_BACKUP );
		VNCThread_UpdateCursorTextureRect( vnc );

		free( vnc->cursor.buffer );
		free( vnc->cursor.backup );
	}

	vnc->cursor.width = width;
	vnc->cursor.height = height;
	vnc->cursor.xHot = xhot;
	vnc->cursor.yHot = yhot;

	vnc->cursor.buffer = (byte *)malloc( vnc->cursor.width * vnc->cursor.height * 4 );
	assert( vnc->cursor.buffer );

    memcpy( vnc->cursor.buffer, client->rcSource, width * height * 4 );

    for ( y = 0; y < height; y++ )
    {
        for ( x = 0; x < width; x++ )
        {
            maskPixel = client->rcMask[y * width + x];
            vnc->cursor.buffer[(y * width + x) * 4 + 3] = maskPixel ? 0xff : 0x00;
		}
	}

	vnc->cursor.backup = (byte *)malloc( vnc->cursor.width * vnc->cursor.height * 4 );
	assert( vnc->cursor.backup );

	VNCThread_CopyCursor( vnc, COPY_TO_BACKUP );
	VNCThread_CopyCursor( vnc, COPY_FROM_CURSOR );
	VNCThread_UpdateCursorTextureRect( vnc );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );
}


static rfbBool vnc_thread_handle_cursor_pos( rfbClient *client, int x, int y )
{
	SVNCWidget 		*vnc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_POS );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	if ( !vnc->client->frameBuffer )
	{
		LOG( "CursorPos event without preceding resize event." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );
		return FALSE;
	}

	if ( !vnc->cursor.buffer )
	{
		LOG( "CursorPos event without preceding shape event." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );
		return FALSE;
	}

	VNCThread_CopyCursor( vnc, COPY_FROM_BACKUP );
	VNCThread_UpdateCursorTextureRect( vnc );

	vnc->cursor.xPos = x;
	vnc->cursor.yPos = y;

	VNCThread_CopyCursor( vnc, COPY_TO_BACKUP );
	VNCThread_CopyCursor( vnc, COPY_FROM_CURSOR );
	VNCThread_UpdateCursorTextureRect( vnc );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );

	return TRUE;
}


void vnc_thread_cursor_lock( rfbClient *client, int x, int y, int w, int h )
{
	SVNCWidget 		*vnc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_SAVE );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	if ( VNCThread_RectOverlapsCursor( vnc, x, y, w, h ) )
	{
		vnc->cursor.needRestore = strue;
	    VNCThread_CopyCursor( vnc, COPY_FROM_BACKUP );
	}

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SAVE );
}


void vnc_thread_cursor_unlock( rfbClient *client )
{
	SVNCWidget 		*vnc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_RESTORE );

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	if ( vnc->cursor.needRestore )
	{	
		vnc->cursor.needRestore = sfalse;
		VNCThread_CopyCursor( vnc, COPY_TO_BACKUP );
		VNCThread_CopyCursor( vnc, COPY_FROM_CURSOR );
	}

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_RESTORE );
}


static void vnc_thread_got_x_cut_text( rfbClient *client, const char *text, int textlen )
{
	SVNCWidget 		*vnc;

	assert( client );

	vnc = (SVNCWidget *)rfbClientGetClientData( client, &s_vncGlob );
	assert( vnc );

	if ( strncasecmp( text, "$$!", 3 ) == 0 )
	{
		Cmd_Add( text + 3 );
		// $$$ g_pluginInterface.sendMessage( text + 3 );
	}	
	else
	{
		LOG( "Clipboard text: %s", text );
	}
}


static sbool VNCThread_Connect( SVNCWidget *vnc )
{
	rfbClient 	*client;
	int 		argc;
	const char 	*argv[2];

	assert( vnc );
	assert( !vnc->client );

	vnc->state = VNCSTATE_CONNECTING;

	client = rfbGetClient( 8, 3, 4 );
	if ( !client )
		FAIL( "Failed to create VNC client." );

	vnc->client = client;

	client->GetPassword = vnc_thread_get_password;

	client->canHandleNewFBSize = TRUE;
	client->MallocFrameBuffer = vnc_thread_resize;

	client->GotFrameBufferUpdate = vnc_thread_update;
	client->FinishedFrameBufferUpdate = vnc_thread_finished_updates;
	// $$$ Can we implement this to use accelerated blits?
	// client->GotCopyRect = vnc_thread_copy_rect

	client->appData.useRemoteCursor = TRUE;
	client->GotCursorShape = vnc_thread_got_cursor_shape;
	client->HandleCursorPos = vnc_thread_handle_cursor_pos;
	client->SoftCursorLockArea = vnc_thread_cursor_lock;
	client->SoftCursorUnlockScreen = vnc_thread_cursor_unlock;
	client->GotXCutText = vnc_thread_got_x_cut_text;

	// Expedited Forwarding (EF) PHB
	// Note that Wikipedia gives the value as 46, but the libvncserver release notes give 184, which is 46 << 2.
	client->QoS_DSCP = 184;

	rfbClientSetClientData( client, &s_vncGlob, vnc );

	argc = 2;
	argv[0] = "Shellspace";
	argv[1] = vnc->server;

	if ( !rfbInitClient( client, &argc, const_cast< char ** >( argv ) ) ) 
	{
		vnc->state = VNCSTATE_DISCONNECTED;
		return sfalse;
	}

	vnc->state = VNCSTATE_CONNECTED;

	return strue;
}


static sbool VNCThread_Input( SVNCWidget *vnc )
{
	int 		timeout;
	rfbClient 	*client;
	int 		result;

	Prof_Start( PROF_VNC_THREAD_INPUT );

	client = vnc->client;
	assert( client );

	timeout = 10 * 1000; // 10 milliseconds

	Prof_Start( PROF_VNC_THREAD_WAIT );
	result = WaitForMessage( client, timeout );
	Prof_Stop( PROF_VNC_THREAD_WAIT );
	if ( result < 0 )
	{
		LOG( "VNC WaitForMessage failed: %i", result );
		Prof_Stop( PROF_VNC_THREAD_INPUT );
		return sfalse;
	}

	if ( result )
	{
		Prof_Start( PROF_VNC_THREAD_HANDLE );
		result = HandleRFBServerMessage( client );
		Prof_Stop( PROF_VNC_THREAD_HANDLE );
		if ( !result )
		{
			LOG( "HandleRFBServerMessage failed." );
			Prof_Stop( PROF_VNC_THREAD_INPUT );
			return sfalse;
		}
	}

	Prof_Stop( PROF_VNC_THREAD_INPUT );
	return strue;
}


SVNCKeyMap s_vncKeyMap[] =
{
	{ "unknown"   , AKEYCODE_UNKNOWN        , XK_Escape      },
	{ "bkspc"     , AKEYCODE_DEL            , XK_BackSpace   },
	{ "tab"       , AKEYCODE_TAB            , XK_Tab         },
	{ "clear"     , AKEYCODE_UNKNOWN        , XK_Clear       },
	{ "return"    , AKEYCODE_ENTER          , XK_Return      },
	{ "escape"    , AKEYCODE_ESCAPE         , XK_Escape      },
	{ "space"     , AKEYCODE_SPACE          , XK_space       },
	{ "delete"    , AKEYCODE_FORWARD_DEL    , XK_Delete      },
	{ "kp0"       , AKEYCODE_NUMPAD_0       , XK_KP_0        },
	{ "kp1"       , AKEYCODE_NUMPAD_1       , XK_KP_1        },
	{ "kp2"       , AKEYCODE_NUMPAD_2       , XK_KP_2        },
	{ "kp3"       , AKEYCODE_NUMPAD_3       , XK_KP_3        },
	{ "kp4"       , AKEYCODE_NUMPAD_4       , XK_KP_4        },
	{ "kp5"       , AKEYCODE_NUMPAD_5       , XK_KP_5        },
	{ "kp6"       , AKEYCODE_NUMPAD_6       , XK_KP_6        },
	{ "kp7"       , AKEYCODE_NUMPAD_7       , XK_KP_7        },
	{ "kp8"       , AKEYCODE_NUMPAD_8       , XK_KP_8        },
	{ "kp9"       , AKEYCODE_NUMPAD_9       , XK_KP_9        },
	{ "kpdot"     , AKEYCODE_NUMPAD_DOT     , XK_KP_Decimal  },
	{ "kpdiv"     , AKEYCODE_NUMPAD_DIVIDE  , XK_KP_Divide   },
	{ "kpmul"     , AKEYCODE_NUMPAD_MULTIPLY, XK_KP_Multiply },
	{ "kpsub"     , AKEYCODE_NUMPAD_SUBTRACT, XK_KP_Subtract },
	{ "kpadd"     , AKEYCODE_NUMPAD_ADD     , XK_KP_Add      },
	{ "kpenter"   , AKEYCODE_NUMPAD_ENTER   , XK_KP_Enter    },
	{ "kpequal"   , AKEYCODE_NUMPAD_EQUALS  , XK_KP_Equal    },
	{ "up"        , AKEYCODE_DPAD_UP        , XK_Up          },
	{ "down"      , AKEYCODE_DPAD_DOWN      , XK_Down        },
	{ "left"      , AKEYCODE_DPAD_LEFT      , XK_Left        },
	{ "right"     , AKEYCODE_DPAD_RIGHT     , XK_Right       },
	{ "insert"    , AKEYCODE_INSERT         , XK_Insert      },
	{ "home"      , AKEYCODE_MOVE_HOME      , XK_Home        },
	{ "end"       , AKEYCODE_MOVE_END       , XK_End         },
	{ "pgup"      , AKEYCODE_PAGE_UP        , XK_Page_Up     },
	{ "pgdn"      , AKEYCODE_PAGE_DOWN      , XK_Page_Down   },
	{ "f1"        , AKEYCODE_F1             , XK_F1          },
	{ "f2"        , AKEYCODE_F2             , XK_F2          },
	{ "f3"        , AKEYCODE_F3             , XK_F3          },
	{ "f4"        , AKEYCODE_F4             , XK_F4          },
	{ "f5"        , AKEYCODE_F5             , XK_F5          },
	{ "f6"        , AKEYCODE_F6             , XK_F6          },
	{ "f7"        , AKEYCODE_F7             , XK_F7          },
	{ "f8"        , AKEYCODE_F8             , XK_F8          },
	{ "f9"        , AKEYCODE_F9             , XK_F9          },
	{ "f10"       , AKEYCODE_F10            , XK_F10         },
	{ "f11"       , AKEYCODE_F11            , XK_F11         },
	{ "f12"       , AKEYCODE_F12            , XK_F12         },
	{ "f13"       , AKEYCODE_UNKNOWN        , XK_F13         },
	{ "f14"       , AKEYCODE_UNKNOWN        , XK_F14         },
	{ "f15"       , AKEYCODE_UNKNOWN        , XK_F15         },
	{ "numlock"   , AKEYCODE_NUM_LOCK       , XK_Num_Lock    },
	{ "capslock"  , AKEYCODE_CAPS_LOCK      , XK_Caps_Lock   },
	{ "scrolllock", AKEYCODE_SCROLL_LOCK    , XK_Scroll_Lock },
	{ "rshift"    , AKEYCODE_SHIFT_RIGHT    , XK_Shift_R     },
	{ "lshift"    , AKEYCODE_SHIFT_LEFT     , XK_Shift_L     },
	{ "rctrl"     , AKEYCODE_CTRL_RIGHT     , XK_Control_R   },
	{ "lctrl"     , AKEYCODE_CTRL_LEFT      , XK_Control_L   },
	{ "ralt"      , AKEYCODE_ALT_RIGHT      , XK_Alt_R       },
	{ "lalt"      , AKEYCODE_ALT_LEFT       , XK_Alt_L       },
	{ "rmeta"     , AKEYCODE_UNKNOWN        , XK_Meta_R      },
	{ "lmeta"     , AKEYCODE_UNKNOWN        , XK_Meta_L      },
	{ "rsuper"    , AKEYCODE_UNKNOWN        , XK_Super_R     },
	{ "lsuper"    , AKEYCODE_UNKNOWN        , XK_Super_L     },
	{ "mode"      , AKEYCODE_UNKNOWN        , XK_Mode_switch },
	{ "help"      , AKEYCODE_UNKNOWN        , XK_Help        },
	{ "print"     , AKEYCODE_BREAK          , XK_Print       },
	{ "sysreq"    , AKEYCODE_SYSRQ          , XK_Sys_Req     },
	{ "break"     , AKEYCODE_UNKNOWN        , XK_Break       },
	{ "0"         , AKEYCODE_0              , '0'            },
	{ "1"         , AKEYCODE_1              , '1'            },
	{ "2"         , AKEYCODE_2              , '2'            },
	{ "3"         , AKEYCODE_3              , '3'            },
	{ "4"         , AKEYCODE_4              , '4'            },
	{ "5"         , AKEYCODE_5              , '5'            },
	{ "6"         , AKEYCODE_6              , '6'            },
	{ "7"         , AKEYCODE_7              , '7'            },
	{ "8"         , AKEYCODE_8              , '8'            },
	{ "9"         , AKEYCODE_9              , '9'            },
	{ "a"         , AKEYCODE_A              , 'a'            },
	{ "b"         , AKEYCODE_B              , 'b'            },
	{ "c"         , AKEYCODE_C              , 'c'            },
	{ "d"         , AKEYCODE_D              , 'd'            },
	{ "e"         , AKEYCODE_E              , 'e'            },
	{ "f"         , AKEYCODE_F              , 'f'            },
	{ "g"         , AKEYCODE_G              , 'g'            },
	{ "h"         , AKEYCODE_H              , 'h'            },
	{ "i"         , AKEYCODE_I              , 'i'            },
	{ "j"         , AKEYCODE_J              , 'j'            },
	{ "k"         , AKEYCODE_K              , 'k'            },
	{ "l"         , AKEYCODE_L              , 'l'            },
	{ "m"         , AKEYCODE_M              , 'm'            },
	{ "n"         , AKEYCODE_N              , 'n'            },
	{ "o"         , AKEYCODE_O              , 'o'            },
	{ "p"         , AKEYCODE_P              , 'p'            },
	{ "q"         , AKEYCODE_Q              , 'q'            },
	{ "r"         , AKEYCODE_R              , 'r'            },
	{ "s"         , AKEYCODE_S              , 's'            },
	{ "t"         , AKEYCODE_T              , 't'            },
	{ "u"         , AKEYCODE_U              , 'u'            },
	{ "v"         , AKEYCODE_V              , 'v'            },
	{ "w"         , AKEYCODE_W              , 'w'            },
	{ "x"         , AKEYCODE_X              , 'x'            },
	{ "y"         , AKEYCODE_Y              , 'y'            },
	{ "z"         , AKEYCODE_Z              , 'z'            },
	{ "grave"     , AKEYCODE_GRAVE          , '~'            },
	{ "minus"     , AKEYCODE_MINUS          , '-'            },
	{ "plus"      , AKEYCODE_PLUS           , '+'            },
	{ "lbracket"  , AKEYCODE_LEFT_BRACKET   , '['            },
	{ "rbracket"  , AKEYCODE_RIGHT_BRACKET  , ']'            },
	{ "backslash" , AKEYCODE_BACKSLASH      , '\\'           },
	{ "semicolon" , AKEYCODE_SEMICOLON      , ';'            },
	{ "apostrophe", AKEYCODE_APOSTROPHE     , '\''           },
	{ "slash"     , AKEYCODE_SLASH          , '/'            },
	{ "comma"     , AKEYCODE_COMMA          , ','            },
	{ "period"    , AKEYCODE_PERIOD         , '.'            },
};


uint VNC_KeyCodeForAndroidCode( uint androidCode )
{
	SVNCKeyMap 	*map;

	map = s_vncKeyMap;

	while ( map->name )
	{
		if ( map->androidCode == androidCode )
			return map->vncCode;

		map++;
	}

	return INVALID_KEY_CODE;
}


void VNC_KeyCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;
	int 		code;
	int 		vncCode;
	sbool 		down;

	vnc = (SVNCWidget *)context;
	assert( vnc );

	code = atoi( Msg_Argv( msg, 1 ) );
	down = atoi( Msg_Argv( msg, 2 ) );

	LOG( "VNC_KeyCmd: %d %d", code, down );

	vncCode = VNC_KeyCodeForAndroidCode( code );
	if ( vncCode != INVALID_KEY_CODE )
		SendKeyEvent( vnc->client, vncCode, down );
}


void VNC_MouseCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;
	int 		x;
	int 		y;
	int 		buttons;

	vnc = (SVNCWidget *)context;
	assert( vnc );

	x = atoi( Msg_Argv( msg, 1 ) );
	y = atoi( Msg_Argv( msg, 2 ) );

	buttons = 0;

	if ( atoi( Msg_Argv( msg, 3 ) ) )
		buttons |= rfbButton1Mask;
	if ( atoi( Msg_Argv( msg, 4 ) ) )
		buttons |= rfbButton2Mask;
	if ( atoi( Msg_Argv( msg, 5 ) ) )
		buttons |= rfbButton3Mask;

	SendPointerEvent( vnc->client, x, y, buttons );
}


void VNC_TouchCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;
	// sbool 		touch;
	// float 		x;
	// float 		y;

	if ( !s_vncGlob.headmouse )
		return;

	vnc = (SVNCWidget *)context;
	assert( vnc );

	// touch = atoi( Msg_Argv( msg, 1 ) );
	// x = atof( Msg_Argv( msg, 2 ) );
	// y = atof( Msg_Argv( msg, 3 ) );

	// $$$ synthesize mouse down/up, sync with gaze events
}


void VNC_GazeCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;
	float 		gazeX;
	float 		gazeY;
	float 		gazeZ;
	sbool 		touch;

	if ( !s_vncGlob.headmouse )
		return;

	gazeX = atof( Msg_Argv( msg, 1 ) );
	gazeY = atof( Msg_Argv( msg, 2 ) );
	gazeZ = atof( Msg_Argv( msg, 3 ) );
	touch = sfalse; // $$$ sync with touch events

	vnc = (SVNCWidget *)context;
	assert( vnc );

	Vector3f eyeDir( gazeX, gazeY, gazeZ );
	Vector3f centerDir = vnc->globeCenter; // eye assumed at origin

	Vector3f xz = centerDir;
	xz.y = 0;
	xz.Normalize();

	Vector3f eyeXz = eyeDir;
	eyeXz.y = 0;
	eyeXz.Normalize();

	float xzDot = xz.Dot( eyeXz );
	float xzAngle = acosf( xzDot );

	Vector3f yz = centerDir;
	yz.x = 0;
	yz.Normalize();

	Vector3f eyeYz = eyeDir;
	eyeYz.x = 0;
	eyeYz.Normalize();

	float yzDot = yz.Dot( eyeYz );
	float yzAngle = acosf( yzDot );

	float x = xzAngle / (0.25f * (float)vnc->width / vnc->height);
	float y = yzAngle / 0.25f; 

	if ( eyeXz.x < xz.x )
		x = -x;

	if ( eyeYz.x > yz.y )
		y = -y;

	int xInt = vnc->width / 2 + (int)( x * vnc->width / 2 );
	int yInt = vnc->height / 2 + (int)( y * vnc->height / 2 );

	int button = touch ? rfbButton1Mask : 0;

	static int lastXInt; // $$$ store in widget
	static int lastYInt;
	static int lastButton;

	if ( xInt >= 0 && xInt < vnc->width && yInt >= 0 && yInt < vnc->height )
	{
		if ( xInt != lastXInt || yInt != lastYInt || button != lastButton )
		{
			SendPointerEvent( vnc->client, xInt, yInt, button );
		}
	}

	lastXInt = xInt;
	lastYInt = yInt;
	lastButton = button;
}


SMsgCmd s_vncWidgetCmds[] =
{
	{ "key", 			VNC_KeyCmd, 			"key <code> <down>" },
	{ "mouse", 			VNC_MouseCmd, 			"mouse <x> <y> <button1> <button2> <button3>" },
	{ "touch", 			VNC_TouchCmd, 			"touch <up|down|moved> <x> <y>" },
	{ "gaze", 			VNC_GazeCmd, 			"gaze <x> <y> <z>" },
	{ NULL, NULL, NULL }
};


void VNCThread_Messages( SVNCWidget *vnc )
{
	char 	msgBuf[MSG_LIMIT];
	SMsg 	msg;

	assert( vnc );

	g_pluginInterface.receiveWidgetMessage( vnc->id, 1, msgBuf, MSG_LIMIT );
	
	Msg_ParseString( &msg, msgBuf );
	if ( Msg_Empty( &msg ) )
		return;

	if ( Msg_IsArgv( &msg, 0, vnc->id ) )
		Msg_Shift( &msg, 1 );

	MsgCmd_Dispatch( &msg, s_vncWidgetCmds, vnc );
}


static void VNCThread_Loop( SVNCWidget *vnc )
{
	assert( vnc );
	assert( vnc->client );

	while ( !vnc->disconnect )
	{	
		Prof_Start( PROF_VNC_THREAD );

		if ( !VNCThread_Input( vnc ) )
			break;

		VNCThread_Messages( vnc );

		Prof_Stop( PROF_VNC_THREAD );
	}
}


static void VNCThread_Cleanup( SVNCWidget *vnc )
{
	assert( vnc );
	assert( vnc->client );

	vnc->state = VNCSTATE_DISCONNECTED;

	rfbClientCleanup( vnc->client );
	vnc->client = NULL;

	free( vnc->server );
	vnc->server = NULL;

	free( vnc->password );
	vnc->password = NULL;

	if ( vnc->cursor.buffer )
	{
		assert( vnc->cursor.backup );

		free( vnc->cursor.buffer );
		vnc->cursor.buffer = NULL;

		free( vnc->cursor.backup );
		vnc->cursor.backup = NULL;
	}
}


static void *VNCThread( void *context )
{
	SVNCWidget	*vnc;

	pthread_setname_np( pthread_self(), "VNC" );

	vnc = (SVNCWidget *)context;
	assert( vnc );

	if ( !VNCThread_Connect( vnc ) )
		return 0;

	VNCThread_Loop( vnc );

	VNCThread_Cleanup( vnc );

	return 0;
}


void VNC_Connect( SVNCWidget *vnc, const char *server, const char *password )
{
	int 	err;

	assert( vnc );

	if ( vnc->thread )
	{
		LOG( "VNC_Connect: Already connected to %s.", vnc->server );
		return;
	}

	assert( vnc );
	assert( !vnc->server );
	assert( !vnc->password );

	vnc->server = strdup( server );
	vnc->password = strdup( password );

	err = pthread_create( &vnc->thread, NULL, VNCThread, vnc );
	if ( err != 0 )
		FAIL( "VNC_Connect: pthread_create returned %i", err );
}


void VNC_Disconnect( SVNCWidget *vnc )
{
	assert( vnc );

	if ( !vnc->thread )
	{
		LOG( "VNC_Disconnect: Not connected." );
		return;
	}

	vnc->disconnect = strue;

	while ( vnc->thread )
		Thread_Sleep( 3 );

	vnc->disconnect = sfalse;
}


#if 0
#define SETUP_CLIPBOARD(error) \
    struct LocalReferenceHolder refs = LocalReferenceHolder_Setup(__FUNCTION__); \
    JNIEnv* env = Android_JNI_GetEnv(); \
    if (!LocalReferenceHolder_Init(&refs, env)) { \
        LocalReferenceHolder_Cleanup(&refs); \
        return error; \
    } \
    jobject clipboard = Android_JNI_GetSystemServiceObject("clipboard"); \
    if (!clipboard) { \
        LocalReferenceHolder_Cleanup(&refs); \
        return error; \
    }

#define CLEANUP_CLIPBOARD() \
    LocalReferenceHolder_Cleanup(&refs);

int Android_JNI_SetClipboardText(const char* text)
{
    SETUP_CLIPBOARD(-1)

    jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, clipboard), "setText", "(Ljava/lang/CharSequence;)V");
    jstring string = (*env)->NewStringUTF(env, text);
    (*env)->CallVoidMethod(env, clipboard, mid, string);
    (*env)->DeleteGlobalRef(env, clipboard);
    (*env)->DeleteLocalRef(env, string);

    CLEANUP_CLIPBOARD();

    return 0;
}

char* Android_JNI_GetClipboardText()
{
    SETUP_CLIPBOARD(SDL_strdup(""))

    jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, clipboard), "getText", "()Ljava/lang/CharSequence;");
    jobject sequence = (*env)->CallObjectMethod(env, clipboard, mid);
    (*env)->DeleteGlobalRef(env, clipboard);
    if (sequence) {
        mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, sequence), "toString", "()Ljava/lang/String;");
        jstring string = (jstring)((*env)->CallObjectMethod(env, sequence, mid));
        const char* utf = (*env)->GetStringUTFChars(env, string, 0);
        if (utf) {
            char* text = SDL_strdup(utf);
            (*env)->ReleaseStringUTFChars(env, string, utf);

            CLEANUP_CLIPBOARD();

            return text;
        }
    }

    CLEANUP_CLIPBOARD();    

    return SDL_strdup("");
}

SDL_bool Android_JNI_HasClipboardText()
{
    SETUP_CLIPBOARD(SDL_FALSE)

    jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, clipboard), "hasText", "()Z");
    jboolean has = (*env)->CallBooleanMethod(env, clipboard, mid);
    (*env)->DeleteGlobalRef(env, clipboard);

    CLEANUP_CLIPBOARD();
    
    return has ? SDL_TRUE : SDL_FALSE;
}
#endif


// sbool VNC_Command( SVNCWidget *vnc )
// {
// 	const char 	*cmd;
// 	int 		vncCode;
// 	SVNCKeyMap 	*map;

// 	cmd = Cmd_Argv( 0 );

// 	if ( cmd[0] == '+' || cmd[0] == '-' )
// 	{
// 		vncCode = INVALID_KEY_CODE;

// 		if ( cmd[1] >= 'a' && cmd[1] <= 'z' && cmd[2] == 0 )
// 		{
// 			vncCode = cmd[1];
// 		}
// 		else if ( cmd[1] >= '0' && cmd[1] <= '9' && cmd[2] == 0 )
// 		{
// 			vncCode = cmd[1];
// 		}
// 		else
// 		{
// 			map = s_vncKeyMap;

// 			while ( map->name )
// 			{
// 				if ( strcasecmp( map->name, cmd + 1 ) == 0 )
// 				{
// 					vncCode = map->vncCode;
// 					break;
// 				}

// 				map++;
// 			}
// 		}

// 		if ( vncCode != INVALID_KEY_CODE )
// 		{
// 			VNC_KeyboardEvent( vnc, vncCode, cmd[0] == '+' );
// 			return strue;
// 		}
// 	}

// 	if ( strcasecmp( cmd, "headmouse" ) == 0 )
// 	{
// 		if ( Cmd_Argc() != 2 )
// 		{
// 			LOG( "Usage: headmouse <on/off/toggle>" );
// 			return strue;
// 		}

// 		if ( strcasecmp( Cmd_Argv( 1 ), "on" ) == 0 )
// 		{
// 			s_vncGlob.headmouse = strue;
// 		}
// 		else if ( strcasecmp( Cmd_Argv( 1 ), "off" ) == 0 )
// 		{	
// 			s_vncGlob.headmouse = sfalse;
// 		}
// 		else if ( strcasecmp( Cmd_Argv( 1 ), "toggle" ) == 0 )
// 		{
// 			s_vncGlob.headmouse = !s_vncGlob.headmouse;
// 		}
// 		else
// 		{
// 			LOG( "Usage: headmouse <on/off/toggle>" );
// 		}

// 		return strue;
// 	}

// 	if ( strcasecmp( cmd, "globeradius" ) == 0 )
// 	{
// 		const char *deltaStr;
// 		float delta;

// 		if ( Cmd_Argc() != 2 )
// 		{
// 			LOG( "Usage: globeradius <+n/-n/n>" );
// 			return strue;
// 		}

// 		deltaStr = Cmd_Argv( 1 );
// 		delta = atof( deltaStr );

// 		if ( delta )
// 		{
// 			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
// 				vnc->globeRadius += delta;
// 			else
// 				vnc->globeRadius = delta;
// 		}

// 		LOG( "globe radius is %f", vnc->globeRadius );

// 		return strue;
// 	}

// 	if ( strcasecmp( cmd, "globefov" ) == 0 )
// 	{
// 		const char *deltaStr;
// 		float delta;

// 		if ( Cmd_Argc() != 2 )
// 		{
// 			LOG( "Usage: globefov <+n/-n/n>" );
// 			return strue;
// 		}

// 		deltaStr = Cmd_Argv( 1 );
// 		delta = atof( deltaStr );

// 		if ( delta )
// 		{
// 			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
// 				vnc->globeFov += delta;
// 			else
// 				vnc->globeFov = delta;
// 		}

// 		vnc->globeFov = S_Maxf( vnc->globeFov, 1.0f );

// 		LOG( "globe fov is %f", vnc->globeFov );

// 		VNC_RebuildGlobe( vnc );

// 		return strue;
// 	}

// 	if ( strcasecmp( cmd, "globez" ) == 0 )
// 	{
// 		const char *deltaStr;
// 		float delta;

// 		if ( Cmd_Argc() != 2 )
// 		{
// 			LOG( "Usage: globez <+n/-n/n>" );
// 			return strue;
// 		}

// 		deltaStr = Cmd_Argv( 1 );
// 		delta = atof( deltaStr );

// 		if ( delta )
// 		{
// 			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
// 				vnc->globeZ += delta;
// 			else
// 				vnc->globeZ = delta;
// 		}

// 		vnc->globeZ = S_Maxf( vnc->globeZ, 1.0f );
// 		vnc->globeZ = S_Minf( vnc->globeZ, 500.0f );

// 		LOG( "globe z is %f", vnc->globeZ );

// 		VNC_RebuildGlobe( vnc );

// 		return strue;
// 	}

// 	if ( strcasecmp( cmd, "globesize" ) == 0 )
// 	{
// 		const char *deltaStr;
// 		float delta;

// 		if ( Cmd_Argc() != 2 )
// 		{
// 			LOG( "Usage: globesize <+n/-n/n>" );
// 			return strue;
// 		}

// 		deltaStr = Cmd_Argv( 1 );
// 		delta = atof( deltaStr );

// 		if ( delta )
// 		{
// 			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
// 				vnc->globeSize += delta;
// 			else
// 				vnc->globeSize = delta;
// 		}

// 		vnc->globeSize = S_Maxf( vnc->globeSize, 1.0f );

// 		LOG( "globe size is %f", vnc->globeSize );

// 		VNC_RebuildGlobe( vnc );

// 		return strue;
// 	}

// 	return sfalse;
// }

SVNCWidget *VNC_AllocWidget( SxWidgetHandle id )
{
	uint 		wIter;
	SVNCWidget 	*vnc;

	for ( wIter = 0; wIter < VNC_WIDGET_LIMIT; wIter++ )
	{
		vnc = &s_vncGlob.widgetPool[wIter];
		if ( !vnc->id )
		{
			memset( vnc, 0, sizeof( SVNCWidget ) );
			vnc->id = strdup( id );
			return vnc;
		};
	}

	LOG( "VNC_AllocWidget: Cannot allocate %s; limit of %i widgets reached.", id, VNC_WIDGET_LIMIT );

	return NULL;
}


void VNC_FreeWidget( SVNCWidget *vnc )
{
	assert( vnc->id );

	free( (char *)vnc->id );
	vnc->id = NULL;
}


sbool VNC_WidgetExists( SxWidgetHandle id )
{
	uint 		wIter;
	SVNCWidget 	*widget;

	for ( wIter = 0; wIter < VNC_WIDGET_LIMIT; wIter++ )
	{
		widget = &s_vncGlob.widgetPool[wIter];
		if ( widget->id && S_strcmp( widget->id, id ) == 0 )
			return strue;
	}

	return sfalse;
}


SVNCWidget *VNC_GetWidget( SxWidgetHandle id )
{
	uint 		wIter;
	SVNCWidget 	*widget;

	for ( wIter = 0; wIter < VNC_WIDGET_LIMIT; wIter++ )
	{
		widget = &s_vncGlob.widgetPool[wIter];
		if ( widget->id && S_strcmp( widget->id, id ) == 0 )
			return widget;
	}

	LOG( "VNC_GetWidget: Widget %s does not exist.", id );

	return NULL;
}


void VNC_CreateCmd( const SMsg *msg, void *context )
{
	SxWidgetHandle 	id;
	SVNCWidget 		*vnc;
	char 			msgBuf[MSG_LIMIT];

	id = Msg_Argv( msg, 1 );
	
	if ( VNC_WidgetExists( id ) )
	{
		LOG( "VNC_CreateCmd: Widget %s already exists.", id );
		return;
	}

	vnc = VNC_AllocWidget( id );
	if ( !vnc )
		return;

	vnc->globeFov = 30.0f;
	vnc->globeRadius = 100.0f;
	vnc->globeZ = 120.0f;
	vnc->globeSize = 60.0f;

	vnc->textureId = vnc->id;
	vnc->geometryId = vnc->id;

	g_pluginInterface.registerTexture( vnc->textureId );
	g_pluginInterface.registerGeometry( vnc->geometryId );

	g_pluginInterface.registerWidget( vnc->id );

	g_pluginInterface.registerEntity( vnc->id );
	g_pluginInterface.setEntityTexture( vnc->id, vnc->textureId );
	g_pluginInterface.setEntityGeometry( vnc->id, vnc->geometryId );

	snprintf( msgBuf, MSG_LIMIT, "shell register %s %s", vnc->id, vnc->id );
	g_pluginInterface.postMessage( msgBuf );
}


void VNC_DestroyCmd( const SMsg *msg, void *context )
{
	SxWidgetHandle 	id;
	SVNCWidget 	*vnc;

	id = Msg_Argv( msg, 1 );
	
	vnc = VNC_GetWidget( id );
	if ( !vnc )
	{
		LOG( "VNC_DestroyCmd: Widget %s does not exist.", id );
		return;
	}

	if ( vnc->thread )
		VNC_Disconnect( vnc );

	g_pluginInterface.unregisterEntity( vnc->id );
	g_pluginInterface.unregisterWidget( vnc->id );
	g_pluginInterface.unregisterTexture( vnc->textureId );
	g_pluginInterface.unregisterGeometry( vnc->geometryId );

	VNC_FreeWidget( vnc );
}


void VNC_ConnectCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;
	const char 	*server;
	const char 	*password;

	if ( Msg_Argc( msg ) != 3 && Msg_Argc( msg ) != 4 )
	{
		LOG( "Usage: connect <wid> <server> [password]" );
		return;
	}

	vnc = VNC_GetWidget( Msg_Argv( msg, 1 ) );
	if ( !vnc )
		return;

	server = Msg_Argv( msg, 2 );
	password = Msg_Argv( msg, 3 );

	VNC_Connect( vnc, server, password );
}


void VNC_DisconnectCmd( const SMsg *msg, void *context )
{
	SVNCWidget 	*vnc;

	vnc = VNC_GetWidget( Msg_Argv( msg, 1 ) );
	if ( !vnc )
		return;

	VNC_Disconnect( vnc );
}


// $$$ Need some way to register a list of commands w/documentation.
//     Other plugins need to be able to interrogate the list of commands,
//     e.g. for console autocomplete and context menu population.
// 	   Really just need to formalize the help string syntax.
//     Could register a "help" command that takes the table and returns all the strings
//      by sending them to a given widget (like a menu widget).
SMsgCmd s_vncCmds[] =
{
	{ "create", 		VNC_CreateCmd, 			"create <wid>" },
	{ "destroy", 		VNC_DestroyCmd, 		"destroy <wid>" },
	{ "connect", 		VNC_ConnectCmd, 		"connect <wid> <server> <port>" },
	{ "disconnect", 	VNC_DisconnectCmd, 		"disconnect <wid>" },
	{ NULL, NULL, NULL }
};


static void *VNC_PluginThread( void *context )
{
	char 	msgBuf[MSG_LIMIT];
	SMsg 	msg;

	pthread_setname_np( pthread_self(), "VNC" );

	g_pluginInterface.registerPlugin( "vnc", SxPluginKind_Widget );

	for ( ;; )
	{
		g_pluginInterface.receivePluginMessage( "vnc", SX_WAIT_INFINITE, msgBuf, MSG_LIMIT );

		Msg_ParseString( &msg, msgBuf );
		if ( Msg_Empty( &msg ) )
			continue;

		if ( Msg_IsArgv( &msg, 0, "vnc" ) )
			Msg_Shift( &msg, 1 );

		if ( Msg_IsArgv( &msg, 0, "unload" ) )
			break;

		MsgCmd_Dispatch( &msg, s_vncCmds, NULL );
	}

	g_pluginInterface.unregisterPlugin( "vnc" );

	return 0;
}


void VNC_InitPlugin()
{
	int err;

	rfbClientLog = rfb_log;
	rfbClientErr = rfb_error;

	s_vncGlob.headmouse = sfalse;

	err = pthread_create( &s_vncGlob.pluginThread, NULL, VNC_PluginThread, NULL );
	if ( err != 0 )
		FAIL( "VNC_InitPlugin: pthread_create returned %i", err );
}
