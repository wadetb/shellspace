#include "common.h"
#include "vncwidget.h"
#include "command.h"
#include "glsl_noise.h"
#include <GlGeometry.h>
#include <GlProgram.h>
#include <android/keycodes.h>
#include "libvncserver/rfb/rfbclient.h"

#define STRESS_RESIZE 				0

#define MAX_VNC_THREADS				64
#define INVALID_VNC_THREAD 			UINT_MAX

#define AKEYCODE_UNKNOWN 			(UINT_MAX)

#define VNC_TEXTURE_COUNT 			3
#define VNC_ALL_TEXTURES_MASK		((1 << VNC_TEXTURE_COUNT) - 1)


enum EVNCState
{
	VNCSTATE_CONNECTING,
	VNCSTATE_CONNECTED,
	VNCSTATE_DISCONNECTED
};


#define VNC_INQUEUE_SIZE 	256
#define VNC_OUTQUEUE_SIZE 	64


enum EVNCInQueueKind
{
	VNC_INQUEUE_NOP,
	VNC_INQUEUE_STATE,
	VNC_INQUEUE_RESIZE,
	VNC_INQUEUE_UPDATE,
	VNC_INQUEUE_FINISHED_UPDATES,
	VNC_INQUEUE_CLIPBOARD
};


struct SVNCInQueueItem
{
	EVNCInQueueKind 	kind;
	mutable byte 		updateMask;
	union
	{
		struct
		{
			EVNCState 	state;
		} state;
		struct
		{
			ushort		width;
			ushort		height;
		} resize;
		struct
		{
			byte 		*buffer;
			ushort		x;
			ushort		y;
			ushort		width;
			ushort		height;
		} update;
		struct
		{
			char 		*text;
		} clipboard;
	};
};


enum EVNCOutQueueKind
{
	VNC_OUTQUEUE_KEYBOARD,
	VNC_OUTQUEUE_MOUSE
};


struct SVNCOutQueueItem
{
	EVNCOutQueueKind 	kind;
	union
	{
		struct
		{
			uint 		code;
			sbool		down;
		} keyboard;
		struct
		{
			ushort		x;
			ushort		y;
			ushort		buttons;
		} mouse;
	};
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


struct SVNCThread
{
	volatile sbool 		started;
	volatile sbool 		referenced;

	pthread_t 			thread;

	pthread_mutex_t 	inMutex;
	SVNCInQueueItem 	inQueue[VNC_INQUEUE_SIZE];
	uint 				inCount;

	pthread_mutex_t 	outMutex;
	SVNCOutQueueItem 	outQueue[VNC_OUTQUEUE_SIZE];
	uint 				outCount;

	SVNCCursor			cursor;

	rfbClient    		*client;
	char 				*server;
	char 				*password;
};


struct SVNCWidget
{
	SVNCThread 			*thread;

	EVNCState			state;

	int 				width;
	int 				height;

	uint 				updateTexIndex;
	uint 				drawTexIndex;
	GLuint 				texId[VNC_TEXTURE_COUNT];
	uint 				texWidth[VNC_TEXTURE_COUNT];
	uint  				texHeight[VNC_TEXTURE_COUNT];

	float	 			globeFov;
	float 				globeRadius;
	float 				globeZ;
	float 				globeSize;
	Vector3f 			globeCenter;
	int 				globeTriCount;
	GlGeometry 			geometry; // $$$ fixme has constructor
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

	GLint 				maxTextureSize;
	GlProgram 			shader;
	GLint 				noiseParams;

	sbool 				headmouse;

	pthread_mutex_t 	threadPoolMutex;
	SVNCThread 			threadPool[MAX_VNC_THREADS];
};


static int s_vncThreadClientKey;

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


static void VNC_OneTimeInit()
{
	int err;

	rfbClientLog = rfb_log;
	rfbClientErr = rfb_log;

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &s_vncGlob.maxTextureSize );

	s_vncGlob.shader = BuildProgram(
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
		GLSL_NOISE
		"uniform sampler2D Texture0;\n"
		"in  highp   vec2 oTexCoord;\n"
		"in  lowp    vec4 oColor;\n"
		"out mediump vec4 fragColor;\n"
		"void main()\n"
		"{\n"
#if USE_TEMPORAL
		"   float s = noiseParams.x;\n"
		"   float t = noiseParams.y;\n"
		"   vec2 jitterScale = s * vec2( dFdx( oTexCoord.x ), dFdy( oTexCoord.y ) );\n"
		"	vec2 texCoord0 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+1.0/8.0 );\n"
		"	vec2 texCoord1 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+2.0/8.0 );\n"
		"	vec2 texCoord2 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+3.0/8.0 );\n"
		"	vec2 texCoord3 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+4.0/8.0 );\n"
		"	vec2 texCoord4 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+5.0/8.0 );\n"
		"	vec2 texCoord5 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+6.0/8.0 );\n"
		"	vec2 texCoord6 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+7.0/8.0 );\n"
		"	vec2 texCoord7 = oTexCoord + NoiseSpatioTemporalDither2D( gl_FragCoord.xy, jitterScale, t+0.0/8.0 );\n"
		"	vec4 color0 = texture( Texture0, texCoord0 );\n"
		"	vec4 color1 = texture( Texture0, texCoord1 );\n"
		"	vec4 color2 = texture( Texture0, texCoord2 );\n"
		"	vec4 color3 = texture( Texture0, texCoord3 );\n"
		"	vec4 color4 = texture( Texture0, texCoord4 );\n"
		"	vec4 color5 = texture( Texture0, texCoord5 );\n"
		"	vec4 color6 = texture( Texture0, texCoord6 );\n"
		"	vec4 color7 = texture( Texture0, texCoord7 );\n"
		"	fragColor = (color0 + color1 + color2 + color3 + color4 + color5 + color6 + color7) * 1.0/8.0;\n"
#else 
		"	fragColor = texture( Texture0, oTexCoord );\n"
#endif
		"}\n"
		);

	s_vncGlob.noiseParams = glGetUniformLocation( s_vncGlob.shader.program, "noiseParams" );
	LOG( "%d", s_vncGlob.noiseParams );

	s_vncGlob.headmouse = sfalse;

	err = pthread_mutex_init( &s_vncGlob.threadPoolMutex, NULL );
	if ( err != 0 )
		FAIL( "VNC: pthread_mutex_init returned %i", err );
}


static SVNCInQueueItem *VNCThread_BeginAppendToInQueue( SVNCThread *vncThread )
{
	SVNCInQueueItem *in;
	struct timespec tim;
	struct timespec tim2;
	sbool 			logged;

	Prof_Start( PROF_VNC_THREAD_LOCK_IN_QUEUE );

	logged = sfalse;

	for ( ;; )
	{
		pthread_mutex_lock( &vncThread->inMutex );

		if ( vncThread->inCount < VNC_INQUEUE_SIZE )
		{
			Prof_Stop( PROF_VNC_THREAD_LOCK_IN_QUEUE );

			in = &vncThread->inQueue[vncThread->inCount];
			memset( in, 0, sizeof( *in ) );

			return in;
		}

		pthread_mutex_unlock( &vncThread->inMutex );

		if ( !logged )
		{
			LOG( "VNC input queue is full, stalling." );
			logged = strue;
		}

		tim.tv_sec  = 0;
		tim.tv_nsec = 1000000;
		nanosleep( &tim, &tim2 );

		if ( !vncThread->referenced )
		{
			Prof_Stop( PROF_VNC_THREAD_LOCK_IN_QUEUE );
			return NULL;
		}
	}
}


static void VNCThread_EndAppendToInQueue( SVNCThread *vncThread )
{
	vncThread->inCount++;
	pthread_mutex_unlock( &vncThread->inMutex );
}


static SVNCOutQueueItem *VNC_BeginAppendToOutQueue( SVNCThread *vncThread )
{
	pthread_mutex_lock( &vncThread->outMutex );

	if ( vncThread->outCount < VNC_OUTQUEUE_SIZE )
		return &vncThread->outQueue[vncThread->outCount];

	pthread_mutex_unlock( &vncThread->outMutex );
	return NULL;
}


static void VNC_EndAppendToOutQueue( SVNCThread *vncThread )
{
	vncThread->outCount++;
	pthread_mutex_unlock( &vncThread->outMutex );
}


static char *vnc_thread_get_password( rfbClient *client )
{
	SVNCThread 	*vncThread;

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	return strdup( vncThread->password );
}


static rfbBool vnc_thread_resize( rfbClient *client ) 
{
	SVNCThread 		*vncThread;
	int 			width;
	int 			height;
	SVNCInQueueItem *in;

	Prof_Start( PROF_VNC_THREAD_HANDLE_RESIZE );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

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

	in = VNCThread_BeginAppendToInQueue( vncThread );
	if ( !in )
	{
		Prof_Stop( PROF_VNC_THREAD_HANDLE_RESIZE );
		return FALSE;
	}

	in->kind = VNC_INQUEUE_RESIZE;
	in->resize.width = width;
	in->resize.height = height;

	VNCThread_EndAppendToInQueue( vncThread );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_RESIZE );

	return TRUE;
}


#if USE_OVERLAY

void VNCThread_SetAlpha( SVNCThread *vncThread, int x, int y, int w, int h )
{
	rfbClient 	*client;
	uint 		xi;
	uint 		yi;
	uint 		stride;
	byte 		*data;

	client = vncThread->client;
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


void VNCThread_UpdateTextureRect( SVNCThread *vncThread, int x, int y, int width, int height )
{
	rfbClient 		*client;
	byte 	 		*buffer;
	int 			yc;
	SVNCInQueueItem *in;
	byte 			*frameBuffer;
	uint 			frameBufferWidth;

	Prof_Start( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );

	client = vncThread->client;
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

	in = VNCThread_BeginAppendToInQueue( vncThread );
	if ( !in )
	{
		free( buffer );
		Prof_Stop( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );
		return;
	}

	in->kind = VNC_INQUEUE_UPDATE;
	in->update.buffer = buffer;
	in->update.x = x;
	in->update.y = y;
	in->update.width = width;
	in->update.height = height;

	VNCThread_EndAppendToInQueue( vncThread );

	Prof_Stop( PROF_VNC_THREAD_UPDATE_TEXTURE_RECT );
}


void vnc_thread_update( rfbClient *client, int x, int y, int w, int h )
{
	SVNCThread 		*vncThread;
	SVNCInQueueItem *in;
	byte 			color[4];
	byte 			*frameBuffer;
	uint 			frameBufferWidth;
	byte 			*buffer;
	int 			xc;
	int 			yc;

	Prof_Start( PROF_VNC_THREAD_HANDLE_UPDATE );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

#if USE_OVERLAY
	VNCThread_SetAlpha( vncThread, x, y, w, h );
#endif // #if USE_OVERLAY

	VNCThread_UpdateTextureRect( vncThread, x, y, w, h );

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
	SVNCThread 		*vncThread;
	SVNCInQueueItem *in;

	Prof_Start( PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	in = VNCThread_BeginAppendToInQueue( vncThread );
	if ( !in )
	{
		Prof_Stop( PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES );
		return;
	}

	in->kind = VNC_INQUEUE_FINISHED_UPDATES;

	VNCThread_EndAppendToInQueue( vncThread );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES );
}

sbool VNCThread_RectOverlapsCursor( SVNCThread *vncThread, int x, int y, int width, int height )
{
	int 	cursorX;
	int 	xMin;
	int 	xMax;
	int 	cursorY;
	int 	yMin;
	int 	yMax;

	assert( vncThread );

	if ( !vncThread->cursor.buffer )
		return sfalse;

	cursorX = vncThread->cursor.xPos - vncThread->cursor.xHot;
	xMin = S_Min( x + width, cursorX + vncThread->cursor.width );
	xMax = S_Max( x, cursorX );

	if ( xMax > xMin )
		return sfalse;

	cursorY = vncThread->cursor.yPos - vncThread->cursor.yHot;
	yMin = S_Min( y + height, cursorY + vncThread->cursor.width );
	yMax = S_Max( y, cursorY );

	if ( yMax > yMin )
		return sfalse;

	return strue;
}


void VNCThread_CopyCursor( SVNCThread *vncThread, uint direction )
{
	uint 	width;
	uint 	height;
	byte 	*frameBuffer;
	uint 	cursorWidth;
	uint 	cursorHeight;
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

	width = vncThread->client->width;
	height = vncThread->client->height;
	frameBuffer = vncThread->client->frameBuffer;
	assert( frameBuffer );

	cursorWidth = vncThread->cursor.width;
	cursorData = (direction == COPY_FROM_CURSOR) ? vncThread->cursor.buffer : vncThread->cursor.backup;
	if ( !cursorData )
		return;

	xStart = vncThread->cursor.xPos - vncThread->cursor.xHot;
	yStart = vncThread->cursor.yPos - vncThread->cursor.yHot;

	xEnd = xStart + vncThread->cursor.width;
	yEnd = yStart + vncThread->cursor.height;

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


void VNCThread_UpdateCursorTextureRect( SVNCThread *vncThread )
{
	int x;
	int y;
	int width;
	int height;

	x = vncThread->cursor.xPos - vncThread->cursor.xHot;
	y = vncThread->cursor.yPos - vncThread->cursor.yHot;
	width = vncThread->cursor.width;
	height = vncThread->cursor.height;

	VNCThread_UpdateTextureRect( vncThread, x, y, width, height );
}


void vnc_thread_got_cursor_shape( rfbClient *client, int xhot, int yhot, int width, int height, int bytesPerPixel )
{
	SVNCThread 		*vncThread;
	SVNCInQueueItem *in;
	byte 			*cursor;
	byte 			maskPixel;
	int 			x;
	int 			y;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	if ( !vncThread->client->frameBuffer )
	{
		LOG( "CursorShape message without preceding resize message." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );
		return;
	}

	if ( vncThread->cursor.buffer )
	{
		assert( vncThread->cursor.backup );

		VNCThread_CopyCursor( vncThread, COPY_FROM_BACKUP );
		VNCThread_UpdateCursorTextureRect( vncThread );

		free( vncThread->cursor.buffer );
		free( vncThread->cursor.backup );
	}

	vncThread->cursor.width = width;
	vncThread->cursor.height = height;
	vncThread->cursor.xHot = xhot;
	vncThread->cursor.yHot = yhot;

	vncThread->cursor.buffer = (byte *)malloc( vncThread->cursor.width * vncThread->cursor.height * 4 );
	assert( vncThread->cursor.buffer );

    memcpy( vncThread->cursor.buffer, client->rcSource, width * height * 4 );

    for ( y = 0; y < height; y++ )
    {
        for ( x = 0; x < width; x++ )
        {
            maskPixel = client->rcMask[y * width + x];
            vncThread->cursor.buffer[(y * width + x) * 4 + 3] = maskPixel ? 0xff : 0x00;
		}
	}

	vncThread->cursor.backup = (byte *)malloc( vncThread->cursor.width * vncThread->cursor.height * 4 );
	assert( vncThread->cursor.backup );

	VNCThread_CopyCursor( vncThread, COPY_TO_BACKUP );
	VNCThread_CopyCursor( vncThread, COPY_FROM_CURSOR );
	VNCThread_UpdateCursorTextureRect( vncThread );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE );
}


static rfbBool vnc_thread_handle_cursor_pos( rfbClient *client, int x, int y )
{
	SVNCThread 		*vncThread;
	SVNCInQueueItem *in;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_POS );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	if ( !vncThread->client->frameBuffer )
	{
		LOG( "CursorPos event without preceding resize event." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );
		return FALSE;
	}

	if ( !vncThread->cursor.buffer )
	{
		LOG( "CursorPos event without preceding shape event." );
		Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );
		return FALSE;
	}

	VNCThread_CopyCursor( vncThread, COPY_FROM_BACKUP );
	VNCThread_UpdateCursorTextureRect( vncThread );

	vncThread->cursor.xPos = x;
	vncThread->cursor.yPos = y;

	VNCThread_CopyCursor( vncThread, COPY_TO_BACKUP );
	VNCThread_CopyCursor( vncThread, COPY_FROM_CURSOR );
	VNCThread_UpdateCursorTextureRect( vncThread );

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_POS );

	return TRUE;
}


void vnc_thread_cursor_lock( rfbClient *client, int x, int y, int w, int h )
{
	SVNCThread 		*vncThread;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_SAVE );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	if ( VNCThread_RectOverlapsCursor( vncThread, x, y, w, h ) )
	{
		vncThread->cursor.needRestore = strue;
	    VNCThread_CopyCursor( vncThread, COPY_FROM_BACKUP );
	}

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_SAVE );
}


void vnc_thread_cursor_unlock( rfbClient *client )
{
	SVNCThread 		*vncThread;

	Prof_Start( PROF_VNC_THREAD_HANDLE_CURSOR_RESTORE );

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	if ( vncThread->cursor.needRestore )
	{	
		vncThread->cursor.needRestore = sfalse;
		VNCThread_CopyCursor( vncThread, COPY_TO_BACKUP );
		VNCThread_CopyCursor( vncThread, COPY_FROM_CURSOR );
	}

	Prof_Stop( PROF_VNC_THREAD_HANDLE_CURSOR_RESTORE );
}


static void vnc_thread_got_x_cut_text( rfbClient *client, const char *text, int textlen )
{
	SVNCThread 		*vncThread;
	SVNCInQueueItem *in;

	assert( client );

	vncThread = (SVNCThread *)rfbClientGetClientData( client, &s_vncThreadClientKey );
	assert( vncThread );

	in = VNCThread_BeginAppendToInQueue( vncThread );
	if ( !in )
		return;

	in->kind = VNC_INQUEUE_CLIPBOARD;
	in->clipboard.text = strdup( text );

	VNCThread_EndAppendToInQueue( vncThread );
}


static void VNCThread_ChangeState( SVNCThread *vncThread, EVNCState newState )
{
	SVNCInQueueItem *in;

	in = VNCThread_BeginAppendToInQueue( vncThread );
	if ( !in )
		return;

	in->kind = VNC_INQUEUE_STATE;
	in->state.state = newState;

	VNCThread_EndAppendToInQueue( vncThread );
}


static sbool VNCThread_Connect( SVNCThread *vncThread )
{
	rfbClient 	*client;
	int 		argc;
	const char 	*argv[2];

	assert( vncThread );
	assert( !vncThread->client );

	VNCThread_ChangeState( vncThread, VNCSTATE_CONNECTING );

	client = rfbGetClient( 8, 3, 4 );
	if ( !client )
		FAIL( "Failed to create VNC client." );

	vncThread->client = client;

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

	rfbClientSetClientData( client, &s_vncThreadClientKey, vncThread );

	argc = 2;
	argv[0] = "VrVnc";
	argv[1] = vncThread->server;

	if ( !rfbInitClient( client, &argc, const_cast< char ** >( argv ) ) ) 
	{
		VNCThread_ChangeState( vncThread, VNCSTATE_DISCONNECTED );
		return sfalse;
	}

	VNCThread_ChangeState( vncThread, VNCSTATE_CONNECTED );
	return strue;
}


static sbool VNCThread_Input( SVNCThread *vncThread )
{
	int 		timeout;
	rfbClient 	*client;
	int 		result;

	Prof_Start( PROF_VNC_THREAD_INPUT );

	client = vncThread->client;
	assert( client );

	timeout = 1 * 1000; // 1 millisecond

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


static void VNCThread_Output( SVNCThread *vncThread )
{
	rfbClient 			*client;
	int 				outIndex;
	SVNCOutQueueItem 	*out;

	Prof_Start( PROF_VNC_THREAD_OUTPUT );

	pthread_mutex_lock( &vncThread->outMutex );

	client = vncThread->client;
	assert( client );

	for ( outIndex = 0; outIndex < vncThread->outCount; outIndex++ )
	{
		out = &vncThread->outQueue[outIndex];

		switch ( out->kind )
		{
		case VNC_OUTQUEUE_KEYBOARD:
			SendKeyEvent( client, out->keyboard.code, out->keyboard.down );
			break;
		case VNC_OUTQUEUE_MOUSE:
			SendPointerEvent( client, out->mouse.x, out->mouse.y, out->mouse.buttons );
			break;
		default:
			assert( false );
			break;
		}
	}

	vncThread->outCount = 0;

	pthread_mutex_unlock( &vncThread->outMutex );

	Prof_Stop( PROF_VNC_THREAD_OUTPUT );
}


static void VNCThread_Loop( SVNCThread *vncThread )
{
	int result;

	assert( vncThread );
	assert( vncThread->client );

	while ( vncThread->referenced )
	{	
		Prof_Start( PROF_VNC_THREAD );

		if ( !VNCThread_Input( vncThread ) )
			break;
		VNCThread_Output( vncThread );

		Prof_Stop( PROF_VNC_THREAD );
	}
}


static void VNCThread_Cleanup( SVNCThread *vncThread )
{
	int err;

	assert( vncThread );
	assert( vncThread->client );

	VNCThread_ChangeState( vncThread, VNCSTATE_DISCONNECTED );

	rfbClientCleanup( vncThread->client );
	vncThread->client = NULL;

	free( vncThread->server );
	vncThread->server = NULL;

	free( vncThread->password );
	vncThread->password = NULL;

	err = pthread_mutex_destroy( &vncThread->inMutex );
	if ( err != 0 )
		FAIL( "VNC: pthread_mutex_destroy returned %i", err );

	err = pthread_mutex_destroy( &vncThread->outMutex );
	if ( err != 0 )
		FAIL( "VNC: pthread_mutex_destroy returned %i", err );
}


static void *VNCThread( void *context )
{
	SVNCThread	*vncThread;

	pthread_setname_np( pthread_self(), "VNC" );

	vncThread = (SVNCThread *)context;
	assert( vncThread );

	if ( !VNCThread_Connect( vncThread ) )
		return 0;

	VNCThread_Loop( vncThread );

	VNCThread_Cleanup( vncThread );

	vncThread->started = sfalse; // Thread memory may be reused after this point.

	return 0;
}


static void VNC_CleanupThread( SVNCThread *vncThread )
{
	int 			inIter;
	SVNCInQueueItem	*inItem;

	for ( inIter = 0; inIter < vncThread->inCount; inIter++ )
	{
		inItem = &vncThread->inQueue[inIter];
		switch ( inItem->kind )
		{
		case VNC_INQUEUE_UPDATE:
			free( inItem->update.buffer );
			break;
		}
	}

	if ( vncThread->cursor.buffer )
	{
		assert( vncThread->cursor.backup );

		free( vncThread->cursor.buffer );
		vncThread->cursor.buffer = NULL;

		free( vncThread->cursor.backup );
		vncThread->cursor.backup = NULL;
	}

	memset( vncThread, 0, sizeof( SVNCThread ) );
}


static uint VNC_AllocateThread()
{
	uint 		threadIter;
	SVNCThread 	*vncThread;

	pthread_mutex_lock( &s_vncGlob.threadPoolMutex );

	for ( threadIter = 0; threadIter < MAX_VNC_THREADS; threadIter++ )
	{
		vncThread = &s_vncGlob.threadPool[threadIter];
		if ( !vncThread->started && !vncThread->referenced )
		{
			// Cleanup any leftover data from the prior user.
			VNC_CleanupThread( vncThread );

			vncThread->referenced = strue;
			break;
		}
	}

	pthread_mutex_unlock( &s_vncGlob.threadPoolMutex );

	if ( threadIter == MAX_VNC_THREADS )
		return INVALID_VNC_THREAD;

	return threadIter;
}


static sbool VNC_SpawnThread( SVNCWidget *vnc, const char *server, const char *password )
{
	uint 					threadId;
	SVNCThread 				*vncThread;
	pthread_mutexattr_t 	mutexAttr;
	sched_param				sparam;
	int 					err;

	threadId = VNC_AllocateThread();
	if ( threadId == INVALID_VNC_THREAD )
		return sfalse;

	vncThread = &s_vncGlob.threadPool[threadId];
	assert( vncThread );

	vncThread->server = strdup( server );
	vncThread->password = strdup( password );

    err = pthread_mutex_init( &vncThread->inMutex, NULL );
	if ( err != 0 )
		FAIL( "VNC: pthread_mutex_init returned %i", err );

    err = pthread_mutex_init( &vncThread->outMutex, NULL );
	if ( err != 0 )
		FAIL( "VNC: pthread_mutex_init returned %i", err );

	vncThread->started = strue;

	err = pthread_create( &vncThread->thread, NULL, VNCThread, vncThread );
	if ( err != 0 )
		FAIL( "VNC: pthread_create returned %i", err );

	vnc->thread = vncThread;

	return strue;
}


SVNCWidget *VNC_CreateWidget()
{
	SVNCWidget 	*vnc;

	vnc = (SVNCWidget *)malloc( sizeof( SVNCWidget ) );
	assert( vnc );

	// $$$ Careful this wipes out GlGeometry.
	memset( vnc, 0, sizeof( *vnc ) );

	if ( !s_vncGlob.initialized )
	{
		s_vncGlob.initialized = TRUE;
		VNC_OneTimeInit();
	}

	vnc->globeFov = 30.0f;
	vnc->globeRadius = 100.0f;
	vnc->globeZ = 120.0f;
	vnc->globeSize = 60.0f;

	return vnc;
}


void VNC_DestroyWidget( SVNCWidget *vnc )
{
	uint texIter;

	if ( vnc->thread )
	{
		vnc->thread->referenced = sfalse;
		vnc->thread = NULL;
	}

	for ( texIter = 0; texIter < VNC_TEXTURE_COUNT; texIter++ )
		if ( vnc->texId[texIter] )
			glDeleteTextures( 1, &vnc->texId[texIter] );

	// $$$ geometry destructor?
}


void VNC_Connect( SVNCWidget *vnc, const char *server, const char *password )
{
	if ( vnc->thread )
	{
		LOG( "Tried to connect again without disconnecting first." );
		return;
	}

	if ( !VNC_SpawnThread( vnc, server, password ) )
	{
		LOG( "Cannot connect; too many open connectzions." );
		return;
	}
}


void VNC_Disconnect( SVNCWidget *vnc )
{
	if ( !vnc->thread )
	{
		LOG( "Tried to disconnect without connecting first." );
		return;
	}

	vnc->thread->referenced = sfalse;
	vnc->thread = NULL;
}


sbool VNC_IsConnected( SVNCWidget *vnc )
{
	if ( !vnc->thread )
		return sfalse;

	return vnc->state == VNCSTATE_CONNECTED;
}


void VNC_MouseEvent( SVNCWidget *vnc, int x, int y, int buttons )
{
	SVNCThread 			*vncThread;
	SVNCOutQueueItem 	*out;

	vncThread = vnc->thread;
	if ( !vncThread )
	{
		LOG( "VNC: Not connected; mouse event discarded." );
		return;
	}

	out = VNC_BeginAppendToOutQueue( vncThread );
	if ( !out )
	{
		LOG( "VNC: Keyboard/mouse queue is full; mouse input lost." );
		return;
	}

	out->kind = VNC_OUTQUEUE_MOUSE;
	out->mouse.x = x;
	out->mouse.y = y;
	out->mouse.buttons = buttons;

	VNC_EndAppendToOutQueue( vncThread );
}


void VNC_KeyboardEvent( SVNCWidget *vnc, uint code, sbool down )
{
	SVNCThread 			*vncThread;
	SVNCOutQueueItem 	*out;

	vncThread = vnc->thread;
	if ( !vncThread )
	{
		LOG( "VNC: Not connected; keyboard event discarded." );
		return;
	}

	out = VNC_BeginAppendToOutQueue( vncThread );
	if ( !out )
	{
		LOG( "VNC: Keyboard/mouse queue is full; keyboard input lost." );
		return;
	}

	out->kind = VNC_OUTQUEUE_KEYBOARD;
	out->keyboard.code = code;
	out->keyboard.down = down;

	VNC_EndAppendToOutQueue( vncThread );
}


void VNC_UpdateHeadMouse( SVNCWidget *vnc, Vector3f eyePos, Vector3f eyeDir, sbool touch )
{
	if ( !s_vncGlob.headmouse )
		return;

	Vector3f centerDir = vnc->globeCenter - eyePos;
	centerDir.Normalize(); // Only for later LOG call.

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

	static int lastXInt;
	static int lastYInt;
	static int lastButton;

	if ( xInt >= 0 && xInt < vnc->width && yInt >= 0 && yInt < vnc->height )
	{
		if ( xInt != lastXInt || yInt != lastYInt || button != lastButton )
		{
			VNC_MouseEvent( vnc, xInt, yInt, button );
		}
	}

	lastXInt = xInt;
	lastYInt = yInt;
	lastButton = button;
}


static void VNC_RebuildTexture( SVNCWidget *vnc, int width, int height )
{
	uint 		texIndex;
	GLuint 		texId;
	uint 		texWidth;
	uint 		texHeight;

	assert( vnc );
	assert( width && height );

	// LOG( "Maximum texture size       : %d", s_vncGlob.maxTextureSize );

	// // $$$ verify that this is working correctly by hacking it to a smaller size.
	// if ( width > s_vncGlob.maxTextureSize || height > s_vncGlob.maxTextureSize )
	// {
	// 	if ( width > s_vncGlob.maxTextureSize )
	// 		width = s_vncGlob.maxTextureSize;
	// 	if ( height > s_vncGlob.maxTextureSize )
	// 		height = s_vncGlob.maxTextureSize;

	// 	LOG( "Clamped client dimensions  : %dx%d", width, height );
	// }

	texIndex = vnc->updateTexIndex % VNC_TEXTURE_COUNT;

	texWidth = S_NextPow2( width );
	texHeight = S_NextPow2( height );

	LOG( "Building texture of size   : %dx%d", texWidth, texHeight );

	if ( vnc->texId[texIndex] )
		glDeleteTextures( 1, &vnc->texId[texIndex] );

	glGenTextures( 1, &texId );

	glBindTexture( GL_TEXTURE_2D, texId );
#if USE_SRGB
	glTexStorage2D( GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, texWidth, texHeight );
#else // #if USE_SRGB
	glTexStorage2D( GL_TEXTURE_2D, 1, GL_RGBA8, texWidth, texHeight );
#endif // #else // #if USE_SRGB
	glBindTexture( GL_TEXTURE_2D, 0 );

	vnc->texId[texIndex] = texId;
	vnc->texWidth[texIndex] = texWidth;
	vnc->texHeight[texIndex] = texHeight;
}


static void VNC_RebuildGlobe( SVNCWidget *vnc )
{
	// $$$ Move aspect, uScale and vScale into the shader so we can avoid rebuilding geometry
	//  when the server resizes the image?
	float aspect = (float)vnc->width / vnc->height;
	float uScale = (float)vnc->width / vnc->texWidth[vnc->updateTexIndex % VNC_TEXTURE_COUNT];
	float vScale = (float)vnc->height / vnc->texHeight[vnc->updateTexIndex % VNC_TEXTURE_COUNT];

	LOG( "Creating Globe Geometry" );

	const int horizontal = 64;
	const int vertical = 32;

	const float fov = vnc->globeFov * M_PI / 180.0f;
	const float radius = vnc->globeRadius;

	// $$$ TODO- handle 0 fov with a special case
	const float yExtent = radius * sinf( 0.5f * fov );
	const float zExtent = radius * cosf( 0.5f * fov ) * cosf( 0.5f * fov * aspect );

	const float scale = vnc->globeSize / yExtent;
	// Globe is set up to initially face down -z.  Love OpenGL.	
	const float zOffset = -vnc->globeZ + (scale * zExtent);

	const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

	VertexAttribs attribs;
	attribs.position.Resize( vertexCount );
	attribs.uv0.Resize( vertexCount );
	attribs.color.Resize( vertexCount );

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

			attribs.position[index].x = scale * radius * cosf( lon ) * cosLat;
			attribs.position[index].y = scale * radius * sinLat;
			attribs.position[index].z = (scale * radius * sinf( lon ) * cosLat) + zOffset;

			attribs.uv0[index].x = xf * uScale;
			attribs.uv0[index].y = ( 1.0 - yf ) * vScale;

			for ( int i = 0; i < 4; i++ )
				attribs.color[index][i] = 1.0f;
		}
	}

	Array< TriangleIndex > indices;
	indices.Resize( horizontal * vertical * 6 );

	vnc->globeTriCount = horizontal * vertical * 2;

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

	// Globe is set up to initially face down -z.  Love OpenGL.	
	vnc->globeCenter.x = 0.0f;
	vnc->globeCenter.y = 0.0f;
	vnc->globeCenter.z = (scale * -radius) + zOffset;

	vnc->geometry = GlGeometry( attribs, indices );
}


static void VNC_HandleState( SVNCWidget *vnc, const SVNCInQueueItem *in )
{
	assert( vnc );
	assert( in );
	assert( in->kind == VNC_INQUEUE_STATE );

	vnc->state = in->state.state;

	if ( vnc->state == VNCSTATE_DISCONNECTED )
	{
		vnc->thread->referenced = sfalse;
		vnc->thread = NULL;
	}
}


static void VNC_HandleResize( SVNCWidget *vnc, const SVNCInQueueItem *in )
{
	Prof_Start( PROF_VNC_WIDGET_RESIZE );

	assert( vnc );
	assert( in );
	assert( in->kind == VNC_INQUEUE_RESIZE );

	vnc->width = in->resize.width;
	vnc->height = in->resize.height;

	VNC_RebuildTexture( vnc, vnc->width, vnc->height );
	VNC_RebuildGlobe( vnc );

	Prof_Stop( PROF_VNC_WIDGET_RESIZE );
}


static void VNC_HandleUpdate( SVNCWidget *vnc, const SVNCInQueueItem *in )
{
	int 	texIndex;

	Prof_Start( PROF_VNC_WIDGET_UPDATE );

	assert( vnc );
	assert( in );
	assert( in->kind == VNC_INQUEUE_UPDATE );

	GL_CheckErrors( "VNC_UpdateTextureRect before" );

	texIndex = vnc->updateTexIndex % VNC_TEXTURE_COUNT;

	assert( vnc->texId[texIndex] );

	glBindTexture( GL_TEXTURE_2D, vnc->texId[texIndex] );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, in->update.width );

	glTexSubImage2D( GL_TEXTURE_2D, 0, in->update.x, in->update.y, in->update.width, in->update.height, GL_RGBA, GL_UNSIGNED_BYTE, in->update.buffer );

	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );

	GL_CheckErrors( "VNC_UpdateTextureRect after" );

	Prof_Stop( PROF_VNC_WIDGET_UPDATE );
}


static void VNC_HandleFinishedUpdates( SVNCWidget *vnc, const SVNCInQueueItem *in )
{
	int 	texIndex;

	Prof_Start( PROF_VNC_WIDGET_FINISHED_UPDATES );

	assert( vnc );
	assert( in );
	assert( in->kind == VNC_INQUEUE_FINISHED_UPDATES );

	vnc->drawTexIndex = vnc->updateTexIndex;

	texIndex = vnc->updateTexIndex % VNC_TEXTURE_COUNT;

	assert( vnc->texId[texIndex] );

	glBindTexture( GL_TEXTURE_2D, vnc->texId[texIndex] );
	// $$$ Could do some mipping of just rectangles affected by updates.
	glGenerateMipmap( GL_TEXTURE_2D );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	Prof_Stop( PROF_VNC_WIDGET_FINISHED_UPDATES );
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


static void VNC_HandleClipboard( SVNCWidget *vnc, SVNCInQueueItem *in )
{
	char 	*text;

	assert( vnc );
	assert( in );
	assert( in->kind == VNC_INQUEUE_CLIPBOARD );

	text = in->clipboard.text;

	if ( strncasecmp( text, "$vrvnc:", 7 ) == 0 )
	{
		Cmd_Add( text + 7 );
	}	
	else
	{
		LOG( "Clipboard text: %s", text );
	}

	free( text );
}


static void VNC_CheckAdvanceTexture( SVNCWidget *vnc, int inCount )
{
	SVNCThread 		*vncThread;
	int 			inIndex;
	SVNCInQueueItem *in;

	Prof_Start( PROF_VNC_WIDGET_ADVANCE );

	vncThread = vnc->thread;
	assert( vncThread );

	if ( vnc->updateTexIndex != vnc->drawTexIndex )
		return;

/*
	LOG( "inCount=%d updateTexIndex=%d drawTexIndex=%d\n", inIndex, inCount, vnc->updateTexIndex, vnc->drawTexIndex );
	for ( inIndex = 0; inIndex < inCount; inIndex++ )
	{
		in = &vncThread->inQueue[inIndex];
		LOG( "%d : kind=%d mask=%x\n", inIndex, in->kind, in->updateMask );
	}
*/

	for ( inIndex = inCount - 1; inIndex >= 0; inIndex-- )
	{
		in = &vncThread->inQueue[inIndex];

		if ( in->kind == VNC_INQUEUE_RESIZE )
		{
			vnc->updateTexIndex++;
			
			// Clear any prior resize and update events; the resize invalidates them.
			inIndex--;
			for ( ; inIndex >= 0; inIndex-- )
			{
				in = &vncThread->inQueue[inIndex];

				if ( in->kind == VNC_INQUEUE_RESIZE || in->kind == VNC_INQUEUE_UPDATE )
				{
					if ( in->kind == VNC_INQUEUE_UPDATE )
						free( in->update.buffer );
					in->kind = VNC_INQUEUE_NOP;
				}
			}

			Prof_Stop( PROF_VNC_WIDGET_ADVANCE );
			return;
		}
	}

	for ( inIndex = 0; inIndex < inCount; inIndex++ )
	{
		in = &vncThread->inQueue[inIndex];

		if ( in->kind == VNC_INQUEUE_UPDATE )
		{
			vnc->updateTexIndex++;
			
			Prof_Stop( PROF_VNC_WIDGET_ADVANCE );
			return;
		}
	}

	Prof_Stop( PROF_VNC_WIDGET_ADVANCE );
}


static void VNC_InQueue( SVNCWidget *vnc )
{
	SVNCThread 		*vncThread;
	int 			inCount;
	int 			inIndex;
	SVNCInQueueItem *in;
	uint 			updateMask;
	double 			startMs;

	Prof_Start( PROF_VNC_WIDGET_INQUEUE );

	vncThread = vnc->thread;
	assert( vncThread );

	pthread_mutex_lock( &vncThread->inMutex );

	// This is safe because the other thread can only append to the queue.
	inCount = vncThread->inCount;

	pthread_mutex_unlock( &vncThread->inMutex );

	if ( !inCount )
		return;

	VNC_CheckAdvanceTexture( vnc, inCount );

	updateMask = 1 << (vnc->updateTexIndex % VNC_TEXTURE_COUNT);

	startMs = Prof_MS();

	for ( inIndex = 0; inIndex < inCount; inIndex++ )
	{
		in = &vncThread->inQueue[inIndex];

		switch ( in->kind )
		{
		case VNC_INQUEUE_NOP:
			break;

		case VNC_INQUEUE_RESIZE:
			if ( !(in->updateMask & updateMask) )
			{
				VNC_HandleResize( vnc, in );

				in->updateMask |= updateMask;				
				if ( in->updateMask == VNC_ALL_TEXTURES_MASK )
					in->kind = VNC_INQUEUE_NOP;
			}
			break;

		case VNC_INQUEUE_UPDATE:
			if ( !(in->updateMask & updateMask) )
			{
				VNC_HandleUpdate( vnc, in );

				in->updateMask |= updateMask;
				if ( in->updateMask == VNC_ALL_TEXTURES_MASK )
				{
					free( in->update.buffer );
					in->kind = VNC_INQUEUE_NOP;
				}
			}
			break;

		case VNC_INQUEUE_FINISHED_UPDATES:
			if ( !(in->updateMask & updateMask) )
			{
				VNC_HandleFinishedUpdates( vnc, in );

				in->updateMask |= updateMask;
				if ( in->updateMask == VNC_ALL_TEXTURES_MASK )
					in->kind = VNC_INQUEUE_NOP;

				goto finished_updates;
			}
			break;

		case VNC_INQUEUE_STATE:
			VNC_HandleState( vnc, in );
			in->kind = VNC_INQUEUE_NOP;

			// Disconnection check
			if ( !vnc->thread ) 
			{
				Prof_Stop( PROF_VNC_WIDGET_INQUEUE );
				return;
			}
			break;

		case VNC_INQUEUE_CLIPBOARD:
			VNC_HandleClipboard( vnc, in );
			in->kind = VNC_INQUEUE_NOP;
			break;

		default:
			assert( false );
			break;
		}

		if ( Prof_MS() - startMs > 10.0f )
			break;
	}
finished_updates:

	pthread_mutex_lock( &vncThread->inMutex );

	inCount = vncThread->inCount;
	for ( inIndex = 0; inIndex < inCount; inIndex++ )
	{
		in = &vncThread->inQueue[inIndex];
		if ( in->kind != VNC_INQUEUE_NOP )
			break;
	}

	inCount -= inIndex;
	memmove( &vncThread->inQueue[0], &vncThread->inQueue[inIndex], sizeof( SVNCInQueueItem ) * inCount );
	vncThread->inCount = inCount;

	pthread_mutex_unlock( &vncThread->inMutex );

	Prof_Stop( PROF_VNC_WIDGET_INQUEUE );
}


void VNC_UpdateWidget( SVNCWidget *vnc )
{
	int 			inIndex;
	SVNCInQueueItem	*in;

	assert( vnc );

	Prof_Start( PROF_VNC_WIDGET );

	if ( vnc->thread )
		VNC_InQueue( vnc );

	Prof_Stop( PROF_VNC_WIDGET );
}


void VNC_DrawWidget( SVNCWidget *vnc, const Matrix4f &view )
{
	int 	triCount;
	int 	indexOffset;
	int 	batchTriCount;
	int 	triCountLeft;

	Prof_Start( PROF_DRAW_VNC_WIDGET );

	assert( vnc );

	glUseProgram( s_vncGlob.shader.program );

	glUniform4f( s_vncGlob.shader.uColor, 1.0f, 1.0f, 1.0f, 1.0f );
	glUniformMatrix4fv( s_vncGlob.shader.uMvp, 1, GL_FALSE, view.Transposed().M[ 0 ] );

	float t = (float)drand48();
	glUniform2f( s_vncGlob.noiseParams, 0.05f, t );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, vnc->texId[vnc->drawTexIndex % VNC_TEXTURE_COUNT] );

#if USE_SPLIT_DRAW
	glBindVertexArrayOES_( vnc->geometry.vertexArrayObject );

	indexOffset = 0;
	triCount = vnc->geometry.indexCount / 3;
	triCountLeft = triCount;

	while ( triCountLeft )
	{
		batchTriCount = S_Min( triCountLeft, triCount / 10 );

		glDrawElements( GL_TRIANGLES, batchTriCount * 3, ( sizeof( TriangleIndex ) == 2 ) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void *)indexOffset );

		indexOffset += batchTriCount * sizeof( TriangleIndex ) * 3;
		triCountLeft -= batchTriCount;
	}

	glBindTexture( GL_TEXTURE_2D, 0 );
#else // #if USE_SPLIT_DRAW
	vnc->geometry.Draw();
#endif // #else // #if USE_SPLIT_DRAW

	Prof_Stop( PROF_DRAW_VNC_WIDGET );
}


GLint VNC_GetTexID( SVNCWidget *vnc )
{
	return vnc->texId[vnc->drawTexIndex % VNC_TEXTURE_COUNT];
}


int VNC_GetWidth( SVNCWidget *vnc )
{
	return vnc->width;
}


int VNC_GetHeight( SVNCWidget *vnc )
{
	return vnc->height;
}


int VNC_GetTexWidth( SVNCWidget *vnc )
{
	return vnc->texWidth[vnc->drawTexIndex % VNC_TEXTURE_COUNT];
}


int VNC_GetTexHeight( SVNCWidget *vnc )
{
	return vnc->texHeight[vnc->drawTexIndex % VNC_TEXTURE_COUNT];
}


SVNCKeyMap s_vncKeyMap[] =
{
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
	{ NULL        , AKEYCODE_UNKNOWN        , 0              }
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


sbool VNC_Command( SVNCWidget *vnc )
{
	const char 	*cmd;
	int 		vncCode;
	SVNCKeyMap 	*map;

	cmd = Cmd_Argv( 0 );

	if ( cmd[0] == '+' || cmd[0] == '-' )
	{
		vncCode = INVALID_KEY_CODE;

		if ( cmd[1] >= 'a' && cmd[1] <= 'z' && cmd[2] == 0 )
		{
			vncCode = cmd[1];
		}
		else if ( cmd[1] >= '0' && cmd[1] <= '9' && cmd[2] == 0 )
		{
			vncCode = cmd[1];
		}
		else
		{
			map = s_vncKeyMap;

			while ( map->name )
			{
				if ( strcasecmp( map->name, cmd + 1 ) == 0 )
				{
					vncCode = map->vncCode;
					break;
				}

				map++;
			}
		}

		if ( vncCode != INVALID_KEY_CODE )
		{
			VNC_KeyboardEvent( vnc, vncCode, cmd[0] == '+' );
			return strue;
		}
	}

	if ( strcasecmp( cmd, "connect" ) == 0 )
	{
		if ( Cmd_Argc() < 2 )
		{
			LOG( "Usage: connect <ip> [password]" );
			return strue;
		}

		VNC_Connect( vnc, Cmd_Argv( 1 ), Cmd_Argv( 2 ) );

		return strue;
	}

	if ( strcasecmp( cmd, "disconnect" ) == 0 )
	{
		if ( Cmd_Argc() != 1 )
		{
			LOG( "Usage: disconnect" );
			return strue;
		}

		VNC_Disconnect( vnc );

		return strue;
	}

	if ( strcasecmp( cmd, "headmouse" ) == 0 )
	{
		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: headmouse <on/off/toggle>" );
			return strue;
		}

		if ( strcasecmp( Cmd_Argv( 1 ), "on" ) == 0 )
		{
			s_vncGlob.headmouse = strue;
		}
		else if ( strcasecmp( Cmd_Argv( 1 ), "off" ) == 0 )
		{	
			s_vncGlob.headmouse = sfalse;
		}
		else if ( strcasecmp( Cmd_Argv( 1 ), "toggle" ) == 0 )
		{
			s_vncGlob.headmouse = !s_vncGlob.headmouse;
		}
		else
		{
			LOG( "Usage: headmouse <on/off/toggle>" );
		}

		return strue;
	}

	if ( strcasecmp( cmd, "globeradius" ) == 0 )
	{
		const char *deltaStr;
		float delta;

		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: globeradius <+n/-n/n>" );
			return strue;
		}

		deltaStr = Cmd_Argv( 1 );
		delta = atof( deltaStr );

		if ( delta )
		{
			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
				vnc->globeRadius += delta;
			else
				vnc->globeRadius = delta;
		}

		LOG( "globe radius is %f", vnc->globeRadius );

		return strue;
	}

	if ( strcasecmp( cmd, "globefov" ) == 0 )
	{
		const char *deltaStr;
		float delta;

		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: globefov <+n/-n/n>" );
			return strue;
		}

		deltaStr = Cmd_Argv( 1 );
		delta = atof( deltaStr );

		if ( delta )
		{
			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
				vnc->globeFov += delta;
			else
				vnc->globeFov = delta;
		}

		vnc->globeFov = S_Maxf( vnc->globeFov, 1.0f );
		vnc->globeFov = S_Minf( vnc->globeFov, 90.0f );

		LOG( "globe fov is %f", vnc->globeFov );

		VNC_RebuildGlobe( vnc );

		return strue;
	}

	if ( strcasecmp( cmd, "globez" ) == 0 )
	{
		const char *deltaStr;
		float delta;

		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: globez <+n/-n/n>" );
			return strue;
		}

		deltaStr = Cmd_Argv( 1 );
		delta = atof( deltaStr );

		if ( delta )
		{
			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
				vnc->globeZ += delta;
			else
				vnc->globeZ = delta;
		}

		vnc->globeZ = S_Maxf( vnc->globeZ, 1.0f );
		vnc->globeZ = S_Minf( vnc->globeZ, 500.0f );

		LOG( "globe z is %f", vnc->globeZ );

		VNC_RebuildGlobe( vnc );

		return strue;
	}

	if ( strcasecmp( cmd, "globesize" ) == 0 )
	{
		const char *deltaStr;
		float delta;

		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: globesize <+n/-n/n>" );
			return strue;
		}

		deltaStr = Cmd_Argv( 1 );
		delta = atof( deltaStr );

		if ( delta )
		{
			if ( deltaStr[0] == '+' || deltaStr[0] == '-' )
				vnc->globeSize += delta;
			else
				vnc->globeSize = delta;
		}

		vnc->globeSize = S_Maxf( vnc->globeSize, 1.0f );
		vnc->globeSize = S_Minf( vnc->globeSize, 500.0f );

		LOG( "globe size is %f", vnc->globeSize );

		VNC_RebuildGlobe( vnc );

		return strue;
	}

	return sfalse;
}



