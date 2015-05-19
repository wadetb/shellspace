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
#include "vlcplugin.h"
#include "command.h"
#include "message.h"
#include "thread.h"

#include <android/keycodes.h>
#include <vlc/vlc.h>


#define VLC_WIDGET_LIMIT 			1

#define AKEYCODE_UNKNOWN 			(UINT_MAX)
#define INVALID_KEY_CODE            (-1)

#define GLOBE_HORIZONTAL 			64
#define GLOBE_VERTICAL				32

#define VLC_DEFAULT_WIDTH 			1280
#define VLC_DEFAULT_HEIGHT 			720


enum EVLCState
{
	VLCSTATE_DESTROYED,
	VLCSTATE_CREATING,
	VLCSTATE_CLOSED,
	VLCSTATE_OPENED
};


struct SVLCWidget
{
	SxWidgetHandle 			id;

	pthread_t 				thread;
	volatile sbool 			disconnect;

	SMsgQueue				msgQueue;

	char 					*mediaPath;
    libvlc_instance_t 		*libvlc;
    libvlc_media_t 			*m;
    libvlc_media_player_t 	*mp;

	volatile EVLCState		state;

	int 					width;
	int 					height;
	void 					*pixels;
};


struct SVLCGlobals
{
	sbool 				initialized;

	SVLCWidget 			widgetPool[VLC_WIDGET_LIMIT];

	pthread_t 			pluginThread;
};


static SVLCGlobals s_vlcGlob;


void *vlc_lock( void *data, void **p_pixels )
{
	SVLCWidget 	*vlc;

	vlc = (SVLCWidget *)data;
	assert( vlc );

	assert( vlc->pixels );
	*p_pixels = vlc->pixels;

	return NULL; // picture identifier, not needed here
}


void vlc_unlock( void *data, void *id, void *const *p_pixels )
{
	SVLCWidget 	*vlc;

	vlc = (SVLCWidget *)data;
	assert( vlc );

	assert( id == NULL ); // picture identifier, not needed here
}


void vlc_display( void *data, void *id )
{
	SVLCWidget 	*vlc;

	vlc = (SVLCWidget *)data;
	assert( vlc );

	g_pluginInterface.updateTextureRect( vlc->id, 0, 0, vlc->width, vlc->height, vlc->width * 4, vlc->pixels );

	assert( id == NULL ); // picture identifier, not needed here
}


const char *s_vlcLogLevelNames[] =
{
	"DEBUG", 	// LIBVLC_DEBUG=0,   /**< Debug message */
    "INVALID", 	
    "NOTICE", 	// LIBVLC_NOTICE=2,  /**< Important informational message */
    "WARNING", 	// LIBVLC_WARNING=3, /**< Warning (potential error) message */
    "ERROR", 	// LIBVLC_ERROR=4    /**< Error message */
};


void vlc_log( void *data, int level, const libvlc_log_t *ctx, const char *fmt, va_list args )
{
	char 		buffer[1024];
	const char 	*module;
	const char 	*file;
	unsigned 	line;

	libvlc_log_get_context( ctx, &module, &file, &line );

	while ( strncmp( file, "../", 3 ) == 0 )
		file += 3;

	snprintf( buffer, sizeof( buffer ), "%s: %s: %s(%u): ", s_vlcLogLevelNames[level], module, file, line );

    vsnprintf( buffer + strlen( buffer ), sizeof( buffer ) - strlen( buffer ), fmt, args );

    S_Log( "%s", buffer );
}


void VLCThread_Create( SVLCWidget *vlc )
{
	assert( vlc );
	assert( !vlc->libvlc );

	S_Log( "VLCThread_Create: Creating %s...", vlc->id );

    char const *vlc_argv[] =
    {
        "--no-audio", // skip any audio track
        "--no-xlib", // tell VLC to not use Xlib  $$$ What does that mean?
        "-vvv", 
    };
    int vlc_argc = sizeof( vlc_argv ) / sizeof( *vlc_argv );

    vlc->libvlc = libvlc_new( vlc_argc, vlc_argv );
    assert( vlc->libvlc );

	libvlc_log_set( vlc->libvlc, vlc_log, vlc );

	vlc->state = VLCSTATE_CLOSED;

	S_Log( "VLCThread_Create: Finished creating %s.", vlc->id );
}


void VLC_OpenCmd( const SMsg *msg, void *context )
{
	SVLCWidget 	*vlc;

	vlc = (SVLCWidget *)context;
	assert( vlc );

	if ( Msg_Argc( msg ) != 2 )
	{
		S_Log( "Usage: open <mediaPath>" );
		return;
	}

	if ( vlc->mediaPath )
	{
		S_Log( "open: %s is already open; please close it first.", vlc->mediaPath );
		return;
	}

	vlc->mediaPath = strdup( Msg_Argv( msg, 1 ) );
	assert( vlc->mediaPath );

	S_Log( "VLC_OpenCmd: Opening %s...", vlc->mediaPath );

	assert( vlc->libvlc );

    vlc->m = libvlc_media_new_path( vlc->libvlc, vlc->mediaPath );
    vlc->mp = libvlc_media_player_new_from_media( vlc->m );
    libvlc_media_release( vlc->m );

	assert( vlc->width );
	assert( vlc->height );

    vlc->pixels = malloc( vlc->width * vlc->height * 4 );
    assert( vlc->pixels );

    libvlc_video_set_callbacks( vlc->mp, vlc_lock, vlc_unlock, vlc_display, vlc );

    libvlc_video_set_format( vlc->mp, "RV32", vlc->width, vlc->height, vlc->width * 4 );

   	S_Log( "VLC_OpenCmd: Finished opening %s.", vlc->mediaPath );
}


void VLC_CloseCmd( const SMsg *msg, void *context )
{
	SVLCWidget 	*vlc;

	vlc = (SVLCWidget *)context;
	assert( vlc );

	if ( Msg_Argc( msg ) != 1 )
	{
		S_Log( "Usage: close" );
		return;
	}

	if ( !vlc->mediaPath )
	{
		S_Log( "close: No media is open." );
		return;
	}

    libvlc_media_player_stop( vlc->mp );
    libvlc_media_player_release( vlc->mp );
    vlc->mp = NULL;

    vlc->m = NULL;

    free( vlc->pixels );
    vlc->pixels = NULL;

    free( vlc->mediaPath );
    vlc->mediaPath = NULL;
}


void VLC_PlayCmd( const SMsg *msg, void *context )
{
	SVLCWidget *vlc;

	vlc = (SVLCWidget *)context;
	assert( vlc );

	if ( Msg_Argc( msg ) != 1 )
	{
		S_Log( "Usage: play" );
		return;
	}

	if ( !vlc->mediaPath )
	{
		S_Log( "play: No media is open." );
		return;
	}

    libvlc_media_player_play( vlc->mp );
}


void VLC_StopCmd( const SMsg *msg, void *context )
{
	SVLCWidget *vlc;

	vlc = (SVLCWidget *)context;
	assert( vlc );

	if ( Msg_Argc( msg ) != 1 )
	{
		S_Log( "Usage: stop" );
		return;
	}

	if ( !vlc->mediaPath )
	{
		S_Log( "stop: No media is open." );
		return;
	}

    libvlc_media_player_stop( vlc->mp );
}


SMsgCmd s_vlcWidgetCmds[] =
{
	{ "open", 			VLC_OpenCmd, 			"open" },
	{ "close", 			VLC_CloseCmd, 			"close" },
	{ "play", 			VLC_PlayCmd, 			"play" },
	{ "stop", 			VLC_StopCmd, 			"stop" },
	{ NULL, NULL, NULL }
};


void VLCThread_Messages( SVLCWidget *vlc )
{
	char 	*text;
	SMsg 	msg;

	assert( vlc );

	text = MsgQueue_Get( &vlc->msgQueue, 100 );
	if ( !text )
		return;
	
	Msg_ParseString( &msg, text );

	free( text );

	if ( Msg_Empty( &msg ) )
		return;

	if ( Msg_IsArgv( &msg, 0, "vlc" ) )
		Msg_Shift( &msg, 1 );

	if ( Msg_IsArgv( &msg, 0, vlc->id ) )
		Msg_Shift( &msg, 1 );

	MsgCmd_Dispatch( &msg, s_vlcWidgetCmds, vlc );
}


static void VLCThread_Loop( SVLCWidget *vlc )
{
	assert( vlc );
	assert( vlc->libvlc );

	while ( !vlc->disconnect )
	{	
		Prof_Start( PROF_VLC_THREAD );

		VLCThread_Messages( vlc );

		Prof_Stop( PROF_VLC_THREAD );
	}
}


static void VLCThread_Cleanup( SVLCWidget *vlc )
{
	assert( vlc );

	if ( vlc->mediaPath )
	{
		S_Log( "VLCThread_Cleanup: Closing %s.", vlc->mediaPath );

	    libvlc_media_player_stop( vlc->mp );
	    libvlc_media_player_release( vlc->mp );
	    vlc->mp = NULL;

	    vlc->m = NULL;

	    free( vlc->pixels );
	    vlc->pixels = NULL;

	    free( vlc->mediaPath );
	    vlc->mediaPath = NULL;
	}

    libvlc_release( vlc->libvlc );
    vlc->libvlc = NULL;

	vlc->state = VLCSTATE_DESTROYED;
}


static void *VLCThread( void *context )
{
	SVLCWidget	*vlc;

	pthread_setname_np( pthread_self(), "VLC" );

	vlc = (SVLCWidget *)context;
	assert( vlc );

	VLCThread_Create( vlc );
	VLCThread_Loop( vlc );
	VLCThread_Cleanup( vlc );

	return 0;
}


SVLCWidget *VLC_AllocWidget( SxWidgetHandle id )
{
	uint 		wIter;
	SVLCWidget 	*vlc;

	for ( wIter = 0; wIter < VLC_WIDGET_LIMIT; wIter++ )
	{
		vlc = &s_vlcGlob.widgetPool[wIter];
		if ( !vlc->id )
		{
			memset( vlc, 0, sizeof( SVLCWidget ) );
			vlc->id = strdup( id );
			return vlc;
		};
	}

	S_Log( "VLC_AllocWidget: Cannot allocate %s; limit of %i widgets reached.", id, VLC_WIDGET_LIMIT );

	return NULL;
}


void VLC_FreeWidget( SVLCWidget *vlc )
{
	assert( vlc->id );

	free( (char *)vlc->id );
	vlc->id = NULL;
}


sbool VLC_WidgetExists( SxWidgetHandle id )
{
	uint 		wIter;
	SVLCWidget 	*widget;

	for ( wIter = 0; wIter < VLC_WIDGET_LIMIT; wIter++ )
	{
		widget = &s_vlcGlob.widgetPool[wIter];
		if ( widget->id && S_strcmp( widget->id, id ) == 0 )
			return strue;
	}

	return sfalse;
}


SVLCWidget *VLC_GetWidget( SxWidgetHandle id )
{
	uint 		wIter;
	SVLCWidget 	*widget;

	for ( wIter = 0; wIter < VLC_WIDGET_LIMIT; wIter++ )
	{
		widget = &s_vlcGlob.widgetPool[wIter];
		if ( widget->id && S_streq( widget->id, id ) )
			return widget;
	}

	S_Log( "VLC_GetWidget: Widget %s does not exist.", id );

	return NULL;
}


void VLC_WidgetCmd( const SMsg *msg )
{
	SVLCWidget 		*widget;
	SxWidgetHandle 	wid;
	char 			msgBuf[MSG_LIMIT];

	wid = Msg_Argv( msg, 0 );

	Msg_Format( msg, msgBuf, MSG_LIMIT );

	widget = VLC_GetWidget( wid );
	if ( !widget )
	{
		S_Log( "VLC_WidgetCmd: This command was not recognized as either a plugin command or a valid widget id: %s", msgBuf );
		return;
	}

	MsgQueue_Put( &widget->msgQueue, msgBuf );
}


void VLC_CreateCmd( const SMsg *msg, void *context )
{
	SxWidgetHandle 	id;
	SVLCWidget 		*vlc;
	int 			err;
	char 			msgBuf[MSG_LIMIT];

	id = Msg_Argv( msg, 1 );
	
	if ( VLC_WidgetExists( id ) )
	{
		S_Log( "create: Widget %s already exists.", id );
		return;
	}

	vlc = VLC_AllocWidget( id );
	if ( !vlc )
		return;

	vlc->state = VLCSTATE_CREATING;

	vlc->width = VLC_DEFAULT_WIDTH;
	vlc->height = VLC_DEFAULT_HEIGHT;

	err = pthread_create( &vlc->thread, NULL, VLCThread, vlc );
	if ( err != 0 )
		S_Fail( "VLC_CreateCmd: pthread_create returned %i", err );

	g_pluginInterface.registerWidget( vlc->id );
	g_pluginInterface.registerEntity( vlc->id );
	g_pluginInterface.registerTexture( vlc->id );

	g_pluginInterface.setEntityTexture( vlc->id, vlc->id );
	g_pluginInterface.setEntityGeometry( vlc->id, "quad" );

	snprintf( msgBuf, MSG_LIMIT, "shell register vlc %s %s", vlc->id, vlc->id );
	g_pluginInterface.postMessage( msgBuf );
}


void VLC_DestroyCmd( const SMsg *msg, void *context )
{
	SxWidgetHandle 	id;
	SVLCWidget 		*vlc;
	char 			msgBuf[MSG_LIMIT];

	id = Msg_Argv( msg, 1 );
	
	vlc = VLC_GetWidget( id );
	if ( !vlc )
	{
		S_Log( "destroy: Widget %s does not exist.", id );
		return;
	}

	if ( vlc->state != VLCSTATE_DESTROYED )
	{
		vlc->disconnect = strue;

		S_Log( "VLC_DestroyCmd: Cleaning up the thread..." );

		while ( vlc->state != VLCSTATE_DESTROYED )
			Thread_Sleep( 3 );

		S_Log( "VLC_DestroyCmd: Finished cleaning up the thread." );

		vlc->disconnect = sfalse;
	}

	g_pluginInterface.unregisterWidget( vlc->id );
	g_pluginInterface.unregisterEntity( vlc->id );
	g_pluginInterface.unregisterTexture( vlc->id );

	snprintf( msgBuf, MSG_LIMIT, "shell unregister %s", vlc->id );
	g_pluginInterface.postMessage( msgBuf );

	VLC_FreeWidget( vlc );
}


SMsgCmd s_vlcCmds[] =
{
	{ "create", 		VLC_CreateCmd, 		"create <wid>" },
	{ "destroy", 		VLC_DestroyCmd, 	"destroy <wid>" },
	{ NULL, NULL, NULL }
};


void *VLC_PluginThread( void *context )
{
	char 	msgBuf[MSG_LIMIT];
	SMsg 	msg;

	pthread_setname_np( pthread_self(), "VLC" );

	g_pluginInterface.registerPlugin( "vlc", SxPluginKind_Widget );

	for ( ;; )
	{
		g_pluginInterface.receiveMessage( "vlc", SX_WAIT_INFINITE, msgBuf, MSG_LIMIT );

		Msg_ParseString( &msg, msgBuf );
		if ( Msg_Empty( &msg ) )
			continue;

		if ( Msg_IsArgv( &msg, 0, "vlc" ) )
			Msg_Shift( &msg, 1 );

		if ( Msg_IsArgv( &msg, 0, "unload" ) )
			break;

		if ( !MsgCmd_Dispatch( &msg, s_vlcCmds, NULL ) )
			VLC_WidgetCmd( &msg );
	}

	g_pluginInterface.unregisterPlugin( "vlc" );

	return 0;
}


void VLC_InitPlugin()
{
	int err;

	err = pthread_create( &s_vlcGlob.pluginThread, NULL, VLC_PluginThread, NULL );
	if ( err != 0 )
		S_Fail( "VLC_InitPlugin: pthread_create returned %i", err );
}


extern "C" {
int vlc_entry__a52 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__a52tofloat32 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__a52tospdif (int (*)(void *, void *, int, ...), void *);
int vlc_entry__access_mms (int (*)(void *, void *, int, ...), void *);
int vlc_entry__access_realrtsp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__adjust (int (*)(void *, void *, int, ...), void *);
int vlc_entry__adpcm (int (*)(void *, void *, int, ...), void *);
int vlc_entry__aes3 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__afile (int (*)(void *, void *, int, ...), void *);
int vlc_entry__aiff (int (*)(void *, void *, int, ...), void *);
int vlc_entry__amem (int (*)(void *, void *, int, ...), void *);
int vlc_entry__anaglyph (int (*)(void *, void *, int, ...), void *);
int vlc_entry__android_audiotrack (int (*)(void *, void *, int, ...), void *);
int vlc_entry__android_logger (int (*)(void *, void *, int, ...), void *);
int vlc_entry__android_native_window (int (*)(void *, void *, int, ...), void *);
int vlc_entry__android_surface (int (*)(void *, void *, int, ...), void *);
int vlc_entry__android_window (int (*)(void *, void *, int, ...), void *);
int vlc_entry__antiflicker (int (*)(void *, void *, int, ...), void *);
int vlc_entry__araw (int (*)(void *, void *, int, ...), void *);
int vlc_entry__asf (int (*)(void *, void *, int, ...), void *);
int vlc_entry__attachment (int (*)(void *, void *, int, ...), void *);
int vlc_entry__au (int (*)(void *, void *, int, ...), void *);
int vlc_entry__audio_format (int (*)(void *, void *, int, ...), void *);
int vlc_entry__avcodec (int (*)(void *, void *, int, ...), void *);
int vlc_entry__avformat (int (*)(void *, void *, int, ...), void *);
int vlc_entry__avi (int (*)(void *, void *, int, ...), void *);
int vlc_entry__avio (int (*)(void *, void *, int, ...), void *);
int vlc_entry__blend (int (*)(void *, void *, int, ...), void *);
int vlc_entry__caf (int (*)(void *, void *, int, ...), void *);
int vlc_entry__canvas (int (*)(void *, void *, int, ...), void *);
int vlc_entry__cc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__cdg (int (*)(void *, void *, int, ...), void *);
int vlc_entry__chain (int (*)(void *, void *, int, ...), void *);
int vlc_entry__chorus_flanger (int (*)(void *, void *, int, ...), void *);
int vlc_entry__chroma_yuv_neon (int (*)(void *, void *, int, ...), void *);
int vlc_entry__colorthres (int (*)(void *, void *, int, ...), void *);
int vlc_entry__compressor (int (*)(void *, void *, int, ...), void *);
int vlc_entry__console_logger (int (*)(void *, void *, int, ...), void *);
int vlc_entry__croppadd (int (*)(void *, void *, int, ...), void *);
int vlc_entry__cvdsub (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dash (int (*)(void *, void *, int, ...), void *);
int vlc_entry__decomp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__deinterlace (int (*)(void *, void *, int, ...), void *);
int vlc_entry__demux_cdg (int (*)(void *, void *, int, ...), void *);
int vlc_entry__demux_stl (int (*)(void *, void *, int, ...), void *);
int vlc_entry__demuxdump (int (*)(void *, void *, int, ...), void *);
int vlc_entry__diracsys (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dolby_surround_decoder (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dsm (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dts (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dtstospdif (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dummy (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dvbsub (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dvdnav (int (*)(void *, void *, int, ...), void *);
int vlc_entry__dvdread (int (*)(void *, void *, int, ...), void *);
int vlc_entry__egl_android (int (*)(void *, void *, int, ...), void *);
int vlc_entry__equalizer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__es (int (*)(void *, void *, int, ...), void *);
int vlc_entry__extract (int (*)(void *, void *, int, ...), void *);
int vlc_entry__file_logger (int (*)(void *, void *, int, ...), void *);
int vlc_entry__filesystem (int (*)(void *, void *, int, ...), void *);
int vlc_entry__fingerprinter (int (*)(void *, void *, int, ...), void *);
int vlc_entry__flac (int (*)(void *, void *, int, ...), void *);
int vlc_entry__flacsys (int (*)(void *, void *, int, ...), void *);
int vlc_entry__float_mixer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__folder (int (*)(void *, void *, int, ...), void *);
int vlc_entry__fps (int (*)(void *, void *, int, ...), void *);
int vlc_entry__freetype (int (*)(void *, void *, int, ...), void *);
int vlc_entry__freeze (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ftp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__g711 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__gain (int (*)(void *, void *, int, ...), void *);
int vlc_entry__gaussianblur (int (*)(void *, void *, int, ...), void *);
int vlc_entry__gles2 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__gnutls (int (*)(void *, void *, int, ...), void *);
int vlc_entry__gradfun (int (*)(void *, void *, int, ...), void *);
int vlc_entry__grey_yuv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__h264 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__hds (int (*)(void *, void *, int, ...), void *);
int vlc_entry__headphone_channel_mixer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__hevc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__hqdn3d (int (*)(void *, void *, int, ...), void *);
int vlc_entry__http (int (*)(void *, void *, int, ...), void *);
int vlc_entry__httplive (int (*)(void *, void *, int, ...), void *);
int vlc_entry__i420_rgb (int (*)(void *, void *, int, ...), void *);
int vlc_entry__i420_yuy2 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__i422_i420 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__i422_yuy2 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__image (int (*)(void *, void *, int, ...), void *);
int vlc_entry__imem (int (*)(void *, void *, int, ...), void *);
int vlc_entry__integer_mixer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__invert (int (*)(void *, void *, int, ...), void *);
int vlc_entry__iomx (int (*)(void *, void *, int, ...), void *);
int vlc_entry__jpeg (int (*)(void *, void *, int, ...), void *);
int vlc_entry__karaoke (int (*)(void *, void *, int, ...), void *);
int vlc_entry__liblibass (int (*)(void *, void *, int, ...), void *);
int vlc_entry__liblibmpeg2 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__live555 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__logo (int (*)(void *, void *, int, ...), void *);
int vlc_entry__lpcm (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mad (int (*)(void *, void *, int, ...), void *);
int vlc_entry__marq (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mediacodec (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mjpeg (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mkv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mod (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mono (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mp4 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mpeg_audio (int (*)(void *, void *, int, ...), void *);
int vlc_entry__mpgv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__normvol (int (*)(void *, void *, int, ...), void *);
int vlc_entry__nsc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__nsv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__nuv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ogg (int (*)(void *, void *, int, ...), void *);
int vlc_entry__oldmovie (int (*)(void *, void *, int, ...), void *);
int vlc_entry__opensles_android (int (*)(void *, void *, int, ...), void *);
int vlc_entry__opus (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_avparser (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_dirac (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_flac (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_h264 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_hevc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_mlp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_mpeg4audio (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_mpeg4video (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_mpegvideo (int (*)(void *, void *, int, ...), void *);
int vlc_entry__packetizer_vc1 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__param_eq (int (*)(void *, void *, int, ...), void *);
int vlc_entry__playlist (int (*)(void *, void *, int, ...), void *);
int vlc_entry__png (int (*)(void *, void *, int, ...), void *);
int vlc_entry__postproc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ps (int (*)(void *, void *, int, ...), void *);
int vlc_entry__pva (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rar (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rawaud (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rawdv (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rawvid (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rawvideo (int (*)(void *, void *, int, ...), void *);
int vlc_entry__record (int (*)(void *, void *, int, ...), void *);
int vlc_entry__remap (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rotate (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rtp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__rv32 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__scale (int (*)(void *, void *, int, ...), void *);
int vlc_entry__scaletempo (int (*)(void *, void *, int, ...), void *);
int vlc_entry__scte27 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__sdp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__sepia (int (*)(void *, void *, int, ...), void *);
int vlc_entry__sftp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__shm (int (*)(void *, void *, int, ...), void *);
int vlc_entry__simple_channel_mixer_neon (int (*)(void *, void *, int, ...), void *);
int vlc_entry__simple_channel_mixer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__smooth (int (*)(void *, void *, int, ...), void *);
int vlc_entry__spatializer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__speex (int (*)(void *, void *, int, ...), void *);
int vlc_entry__spudec (int (*)(void *, void *, int, ...), void *);
int vlc_entry__stereo_widen (int (*)(void *, void *, int, ...), void *);
int vlc_entry__stl (int (*)(void *, void *, int, ...), void *);
int vlc_entry__subsdec (int (*)(void *, void *, int, ...), void *);
int vlc_entry__subsdelay (int (*)(void *, void *, int, ...), void *);
int vlc_entry__substtml (int (*)(void *, void *, int, ...), void *);
int vlc_entry__substx3g (int (*)(void *, void *, int, ...), void *);
int vlc_entry__subsusf (int (*)(void *, void *, int, ...), void *);
int vlc_entry__subtitle (int (*)(void *, void *, int, ...), void *);
int vlc_entry__svcdsub (int (*)(void *, void *, int, ...), void *);
int vlc_entry__swscale (int (*)(void *, void *, int, ...), void *);
int vlc_entry__syslog (int (*)(void *, void *, int, ...), void *);
int vlc_entry__taglib (int (*)(void *, void *, int, ...), void *);
int vlc_entry__tcp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__telx (int (*)(void *, void *, int, ...), void *);
int vlc_entry__theora (int (*)(void *, void *, int, ...), void *);
int vlc_entry__timecode (int (*)(void *, void *, int, ...), void *);
int vlc_entry__transform (int (*)(void *, void *, int, ...), void *);
int vlc_entry__trivial_channel_mixer (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ts (int (*)(void *, void *, int, ...), void *);
int vlc_entry__tta (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ttml (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ty (int (*)(void *, void *, int, ...), void *);
int vlc_entry__udp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__ugly_resampler (int (*)(void *, void *, int, ...), void *);
int vlc_entry__uleaddvaudio (int (*)(void *, void *, int, ...), void *);
int vlc_entry__upnp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vc1 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vdr (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vhs (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vmem (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vobsub (int (*)(void *, void *, int, ...), void *);
int vlc_entry__voc (int (*)(void *, void *, int, ...), void *);
int vlc_entry__volume_neon (int (*)(void *, void *, int, ...), void *);
int vlc_entry__vorbis (int (*)(void *, void *, int, ...), void *);
int vlc_entry__wav (int (*)(void *, void *, int, ...), void *);
int vlc_entry__wave (int (*)(void *, void *, int, ...), void *);
int vlc_entry__xa (int (*)(void *, void *, int, ...), void *);
int vlc_entry__xml (int (*)(void *, void *, int, ...), void *);
int vlc_entry__yuv_rgb_neon (int (*)(void *, void *, int, ...), void *);
int vlc_entry__yuvp (int (*)(void *, void *, int, ...), void *);
int vlc_entry__yuy2_i420 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__yuy2_i422 (int (*)(void *, void *, int, ...), void *);
int vlc_entry__zip (int (*)(void *, void *, int, ...), void *);
int vlc_entry__zvbi (int (*)(void *, void *, int, ...), void *);
}


const void *vlc_static_modules[] = 
{
	(void *)vlc_entry__a52,
	(void *)vlc_entry__a52tofloat32,
	(void *)vlc_entry__a52tospdif,
	(void *)vlc_entry__access_mms,
	(void *)vlc_entry__access_realrtsp,
	(void *)vlc_entry__adjust,
	(void *)vlc_entry__adpcm,
	(void *)vlc_entry__aes3,
	(void *)vlc_entry__afile,
	(void *)vlc_entry__aiff,
	(void *)vlc_entry__amem,
	(void *)vlc_entry__anaglyph,
	// (void *)vlc_entry__android_audiotrack,
	// (void *)vlc_entry__android_logger,
	// (void *)vlc_entry__android_native_window,
	// (void *)vlc_entry__android_surface,
	// (void *)vlc_entry__android_window,
	(void *)vlc_entry__antiflicker,
	(void *)vlc_entry__araw,
	(void *)vlc_entry__asf,
	(void *)vlc_entry__attachment,
	(void *)vlc_entry__au,
	(void *)vlc_entry__audio_format,
	(void *)vlc_entry__avcodec,
	(void *)vlc_entry__avformat,
	(void *)vlc_entry__avi,
	(void *)vlc_entry__avio,
	(void *)vlc_entry__blend,
	(void *)vlc_entry__caf,
	(void *)vlc_entry__canvas,
	(void *)vlc_entry__cc,
	(void *)vlc_entry__cdg,
	(void *)vlc_entry__chain,
	(void *)vlc_entry__chorus_flanger,
	(void *)vlc_entry__chroma_yuv_neon,
	(void *)vlc_entry__colorthres,
	(void *)vlc_entry__compressor,
	// (void *)vlc_entry__console_logger,
	(void *)vlc_entry__croppadd,
	(void *)vlc_entry__cvdsub,
	(void *)vlc_entry__dash,
	(void *)vlc_entry__decomp,
	(void *)vlc_entry__deinterlace,
	(void *)vlc_entry__demux_cdg,
	(void *)vlc_entry__demux_stl,
	(void *)vlc_entry__demuxdump,
	(void *)vlc_entry__diracsys,
	// (void *)vlc_entry__dolby_surround_decoder,
	(void *)vlc_entry__dsm,
	(void *)vlc_entry__dts,
	(void *)vlc_entry__dtstospdif,
	(void *)vlc_entry__dummy,
	(void *)vlc_entry__dvbsub,
	(void *)vlc_entry__dvdnav,
	(void *)vlc_entry__dvdread,
	// (void *)vlc_entry__egl_android,
	(void *)vlc_entry__equalizer,
	(void *)vlc_entry__es,
	(void *)vlc_entry__extract,
	// (void *)vlc_entry__file_logger,
	(void *)vlc_entry__filesystem,
	(void *)vlc_entry__fingerprinter,
	(void *)vlc_entry__flac,
	(void *)vlc_entry__flacsys,
	(void *)vlc_entry__float_mixer,
	(void *)vlc_entry__folder,
	(void *)vlc_entry__fps,
	(void *)vlc_entry__freetype,
	(void *)vlc_entry__freeze,
	(void *)vlc_entry__ftp,
	(void *)vlc_entry__g711,
	(void *)vlc_entry__gain,
	(void *)vlc_entry__gaussianblur,
	// (void *)vlc_entry__gles2,
	(void *)vlc_entry__gnutls,
	(void *)vlc_entry__gradfun,
	(void *)vlc_entry__grey_yuv,
	(void *)vlc_entry__h264,
	(void *)vlc_entry__hds,
	// (void *)vlc_entry__headphone_channel_mixer,
	(void *)vlc_entry__hevc,
	(void *)vlc_entry__hqdn3d,
	(void *)vlc_entry__http,
	(void *)vlc_entry__httplive,
	(void *)vlc_entry__i420_rgb,
	(void *)vlc_entry__i420_yuy2,
	(void *)vlc_entry__i422_i420,
	(void *)vlc_entry__i422_yuy2,
	(void *)vlc_entry__image,
	// (void *)vlc_entry__imem,
	(void *)vlc_entry__integer_mixer,
	(void *)vlc_entry__invert,
	// (void *)vlc_entry__iomx,
	(void *)vlc_entry__jpeg,
	(void *)vlc_entry__karaoke,
	// (void *)vlc_entry__liblibass,
	// (void *)vlc_entry__liblibmpeg2,
	(void *)vlc_entry__live555,
	(void *)vlc_entry__logo,
	(void *)vlc_entry__lpcm,
	(void *)vlc_entry__mad,
	(void *)vlc_entry__marq,
	// (void *)vlc_entry__mediacodec,
	(void *)vlc_entry__mjpeg,
	(void *)vlc_entry__mkv,
	(void *)vlc_entry__mod,
	(void *)vlc_entry__mono,
	(void *)vlc_entry__mp4,
	(void *)vlc_entry__mpeg_audio,
	(void *)vlc_entry__mpgv,
	(void *)vlc_entry__normvol,
	(void *)vlc_entry__nsc,
	(void *)vlc_entry__nsv,
	(void *)vlc_entry__nuv,
	(void *)vlc_entry__ogg,
	(void *)vlc_entry__oldmovie,
	// (void *)vlc_entry__opensles_android,
	(void *)vlc_entry__opus,
	(void *)vlc_entry__packetizer_avparser,
	// (void *)vlc_entry__packetizer_dirac,
	// (void *)vlc_entry__packetizer_flac,
	// (void *)vlc_entry__packetizer_h264,
	// (void *)vlc_entry__packetizer_hevc,
	// (void *)vlc_entry__packetizer_mlp,
	// (void *)vlc_entry__packetizer_mpeg4audio,
	// (void *)vlc_entry__packetizer_mpeg4video,
	// (void *)vlc_entry__packetizer_mpegvideo,
	// (void *)vlc_entry__packetizer_vc1,
	(void *)vlc_entry__param_eq,
	(void *)vlc_entry__playlist,
	(void *)vlc_entry__png,
	(void *)vlc_entry__postproc,
	(void *)vlc_entry__ps,
	(void *)vlc_entry__pva,
	(void *)vlc_entry__rar,
	(void *)vlc_entry__rawaud,
	(void *)vlc_entry__rawdv,
	(void *)vlc_entry__rawvid,
	(void *)vlc_entry__rawvideo,
	(void *)vlc_entry__record,
	(void *)vlc_entry__remap,
	(void *)vlc_entry__rotate,
	(void *)vlc_entry__rtp,
	(void *)vlc_entry__rv32,
	(void *)vlc_entry__scale,
	(void *)vlc_entry__scaletempo,
	(void *)vlc_entry__scte27,
	(void *)vlc_entry__sdp,
	(void *)vlc_entry__sepia,
	(void *)vlc_entry__sftp,
	(void *)vlc_entry__shm,
	(void *)vlc_entry__simple_channel_mixer_neon,
	// (void *)vlc_entry__simple_channel_mixer,
	(void *)vlc_entry__smooth,
	(void *)vlc_entry__spatializer,
	(void *)vlc_entry__speex,
	(void *)vlc_entry__spudec,
	(void *)vlc_entry__stereo_widen,
	(void *)vlc_entry__stl,
	(void *)vlc_entry__subsdec,
	(void *)vlc_entry__subsdelay,
	(void *)vlc_entry__substtml,
	(void *)vlc_entry__substx3g,
	(void *)vlc_entry__subsusf,
	(void *)vlc_entry__subtitle,
	(void *)vlc_entry__svcdsub,
	(void *)vlc_entry__swscale,
	(void *)vlc_entry__syslog,
	(void *)vlc_entry__taglib,
	(void *)vlc_entry__tcp,
	(void *)vlc_entry__telx,
	(void *)vlc_entry__theora,
	(void *)vlc_entry__timecode,
	(void *)vlc_entry__transform,
	// (void *)vlc_entry__trivial_channel_mixer,
	(void *)vlc_entry__ts,
	(void *)vlc_entry__tta,
	(void *)vlc_entry__ttml,
	(void *)vlc_entry__ty,
	(void *)vlc_entry__udp,
	// (void *)vlc_entry__ugly_resampler,
	(void *)vlc_entry__uleaddvaudio,
	(void *)vlc_entry__upnp,
	(void *)vlc_entry__vc1,
	(void *)vlc_entry__vdr,
	(void *)vlc_entry__vhs,
	(void *)vlc_entry__vmem,
	(void *)vlc_entry__vobsub,
	(void *)vlc_entry__voc,
	(void *)vlc_entry__volume_neon,
	(void *)vlc_entry__vorbis,
	(void *)vlc_entry__wav,
	(void *)vlc_entry__wave,
	(void *)vlc_entry__xa,
	(void *)vlc_entry__xml,
	(void *)vlc_entry__yuv_rgb_neon,
	(void *)vlc_entry__yuvp,
	(void *)vlc_entry__yuy2_i420,
	(void *)vlc_entry__yuy2_i422,
	(void *)vlc_entry__zip,
	(void *)vlc_entry__zvbi,
	NULL
};

