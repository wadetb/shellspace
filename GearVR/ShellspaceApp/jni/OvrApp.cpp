/************************************************************************************

Filename    :   OvrApp.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
#include "common.h"
#include "OvrApp.h"

#include "command.h"
#include "entity.h"
#include "inqueue.h"
#include "keyboard.h"
#include "registry.h"
#include "thread.h"

#include "vncplugin.h"
#include "v8plugin.h"
#include "shellplugin.h"

#include <android/keycodes.h>
#include <jni.h>

#include "PathUtils.h"

extern "C" {

jlong Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity )
{
       LOG( "nativeSetAppInterface");
       return (new OvrApp())->SetActivity( jni, clazz, activity );
}

} // extern "C"

struct SAppGlobals
{
	SxVector3 	lastGazeDir;
	float 		clearColor[3];
};

static SAppGlobals s_app;

JNIEnv *g_jni;
OvrApp *g_app;

OvrApp::OvrApp()
{
	g_app = this;
}

OvrApp::~OvrApp()
{
}

bool OvrApp::GetWantSrgbFramebuffer() const
{
#if USE_SRGB
	return true;
#else
	return false;
#endif
}


void APITest_Init()
{
	ushort indices[] = 
	{ 
		0, 1, 2, 
		2, 1, 3, 
		0, 1, 4, 
		2, 0, 4, 
		1, 3, 4, 
		3, 2, 4, 
	};

	float positions[] = 
	{ 
		-1.0f, -1.0f, -1.0f, 
		 1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f, 
		 1.0f,  1.0f, -1.0f, 
		 0.0f,  0.0f,  1.0f, 
	};

	float texCoords[] = 
	{ 
		 0.0f,  1.0f, 
		 1.0f,  1.0f, 
		 0.0f,  0.0f, 
		 1.0f,  0.0f, 
		 0.0f,  0.0f, 
	};

	byte colors[] = 
	{ 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
	};

	byte whiteTexels[] = 
	{ 
		255, 255, 255, 255, 
	};

	g_pluginInterface.registerPlugin( "apitest", SxPluginKind_Widget );
	g_pluginInterface.registerWidget( "apitest" );

	g_pluginInterface.registerGeometry( "tet" );
	g_pluginInterface.sizeGeometry( "tet", 5, 18 );
	g_pluginInterface.updateGeometryIndexRange( "tet", 0, 18, indices );
	g_pluginInterface.updateGeometryPositionRange( "tet", 0, 5, (SxVector3 *)positions );
	g_pluginInterface.updateGeometryTexCoordRange( "tet", 0, 5, (SxVector2 *)texCoords );
	g_pluginInterface.updateGeometryColorRange( "tet", 0, 5, (SxColor *)colors );
	g_pluginInterface.presentGeometry( "tet" );

	g_pluginInterface.registerGeometry( "quad" );
	g_pluginInterface.sizeGeometry( "quad", 4, 6 );
	g_pluginInterface.updateGeometryIndexRange( "quad", 0, 6, indices );
	g_pluginInterface.updateGeometryPositionRange( "quad", 0, 4, (SxVector3 *)positions );
	g_pluginInterface.updateGeometryTexCoordRange( "quad", 0, 4, (SxVector2 *)texCoords );
	g_pluginInterface.updateGeometryColorRange( "quad", 0, 4, (SxColor *)colors );
	g_pluginInterface.presentGeometry( "quad" );

	g_pluginInterface.registerTexture( "white" );
	g_pluginInterface.sizeTexture( "white", 1, 1 );
	g_pluginInterface.formatTexture( "white", SxTextureFormat_R8G8B8X8 );
	g_pluginInterface.updateTextureRect( "white", 0, 0, 1, 1, 4, whiteTexels );
	g_pluginInterface.presentTexture( "white" );

	// SxTrajectory tr;
	// SxOrientation o;

	// tr.kind = SxTrajectoryKind_Instant;

	// IdentityOrientation( &o );
	// Vec3Set( &o.origin, 10.0f, 0.0f, -20.0f );
	// Vec3Set( &o.scale, 0.5f, 0.5f, 0.5f );
	// g_pluginInterface.registerEntity( "left" );
	// g_pluginInterface.setEntityGeometry( "left", "tet" );
	// g_pluginInterface.setEntityTexture( "left", "white" );
	// g_pluginInterface.orientEntity( "left", &o, &tr );

	// IdentityOrientation( &o );
	// Vec3Set( &o.origin, 0.0f, 10.0f, 0.0f );
	// g_pluginInterface.registerEntity( "left_child" );
	// g_pluginInterface.setEntityGeometry( "left_child", "tet" );
	// g_pluginInterface.setEntityTexture( "left_child", "white" );
	// g_pluginInterface.orientEntity( "left_child", &o, &tr );
	// g_pluginInterface.parentEntity( "left_child", "left" );

	// IdentityOrientation( &o );
	// Vec3Set( &o.origin, -10.0f, 0.0f, -20.0f );
	// g_pluginInterface.registerEntity( "right" );
	// g_pluginInterface.setEntityGeometry( "right", "tet" );
	// g_pluginInterface.setEntityTexture( "right", "white" );
	// g_pluginInterface.orientEntity( "right", &o, &tr );

	// IdentityOrientation( &o );
	// Vec3Set( &o.origin, 0.0f, 10.0f, 0.0f );
	// g_pluginInterface.registerEntity( "right_child" );
	// g_pluginInterface.setEntityGeometry( "right_child", "tet" );
	// g_pluginInterface.setEntityTexture( "right_child", "white" );
	// g_pluginInterface.orientEntity( "right_child", &o, &tr );
	// g_pluginInterface.parentEntity( "right_child", "right" );
}


void APITest_Frame()
{
	SxTrajectory tr;
	SxOrientation o;

	tr.kind = SxTrajectoryKind_Instant;

	static float frame = 0;
	frame += 1.0f/60.0f;
	IdentityOrientation( &o );
	Vec3Set( &o.origin, 10.0f, 0.0f, -20.0f );
	Vec3Set( &o.scale, 0.5f, 0.5f, 0.5f );
	o.angles.yaw = frame*0.1f*360.0f;
	o.angles.roll = frame*0.2f*360.0f;
	o.angles.pitch = frame*0.3f*360.0f;
	g_pluginInterface.orientEntity( "left", &o, &tr );
}


void OvrApp::OneTimeInit( const char * launchIntent )
{
	g_jni = app->GetVrJni();

	s_app.clearColor[0] = 0.0f;
	s_app.clearColor[1] = 0.0f;
	s_app.clearColor[2] = 0.0f;

	Thread_Init();
	Registry_Init();
	Entity_Init();

	EyeParms &vrParms = app->GetVrParms();
#if USE_SUPERSAMPLE_2X
	vrParms.resolution = 2048;
#elif USE_SUPERSAMPLE_1_5X
	vrParms.resolution = 1536;
#else
	vrParms.resolution = 1024;
#endif
	vrParms.multisamples = 1;
	vrParms.colorFormat = COLOR_8888;
	vrParms.depthFormat = DEPTH_16;

	ovrModeParms VrModeParms = app->GetVrModeParms();
	VrModeParms.AsynchronousTimeWarp = true;
	VrModeParms.AllowPowerSave = true;
	VrModeParms.DistortionFileName = NULL;
	VrModeParms.EnableImageServer = false;
	app->SetVrModeParms( VrModeParms );

	// ovrHmdInfo &hmdInfo = app->GetHmdInfo();

	// app->SetShowFPS( true );

	// Stay exactly at the origin, so the panorama globe is equidistant
	// Don't clear the head model neck length, or swipe view panels feel wrong.
	VrViewParms viewParms = app->GetVrViewParms();
	viewParms.EyeHeight = 0.0f;
	app->SetVrViewParms( viewParms );

	// Optimize for 16 bit depth in a modest globe size
	Scene.Znear = 1.0f;
	Scene.Zfar = 1000.0f;

	Vec3Set( &s_app.lastGazeDir, 0.0f, 0.0f, -1.0f );

	APITest_Init();

	Shell_InitPlugin();
	VNC_InitPlugin();
	V8_InitPlugin();

	Keyboard_Init();

	Cmd_AddFile( "/storage/extSdCard/Oculus/Shellspace/autoexec.vrcfg" );
}

void OvrApp::OneTimeShutdown()
{
	// $$$ Destroy all widgets, entities, textures, geometries, plugins.

	Registry_Shutdown();
	Thread_Shutdown();
}

void OvrApp::Command( const char * msg )
{
}

bool OvrApp::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	sbool 	isDown;

	if ( keyCode == AKEYCODE_BACK )
	{
		if ( eventType == KeyState::KEY_EVENT_SHORT_PRESS )
		{
			Keyboard_Orient( s_app.lastGazeDir, 20.0f );
			Keyboard_Toggle();
		}

		return true;
	}

	if ( eventType == KeyState::KEY_EVENT_DOWN ||
		 eventType == KeyState::KEY_EVENT_UP )
	{
		isDown = (eventType == KeyState::KEY_EVENT_DOWN);
		Cmd_Add( "shell key %d %d", keyCode, isDown );
		return true;
	}

	return false;
}

Matrix4f OvrApp::DrawEyeView( const int eye, const float fovDegrees )
{
	Prof_Start( PROF_DRAW_EYE );

	const Matrix4f view = Scene.DrawEyeView( eye, fovDegrees );

	glClearColor( s_app.clearColor[0], s_app.clearColor[1], s_app.clearColor[2], 1.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	TimeWarpParms & swapParms = app->GetSwapParms();
	
	swapParms.SwapOptions = 0;

	for ( int i = 0; i < 4; i++ )
		swapParms.ProgramParms[ i ] = 1.0f;

#if USE_OVERLAY
	swapParms.WarpProgram = WP_OVERLAY_PLANE;

	if ( vnc && VNC_GetHeight( vnc ) && VNC_GetTexWidth( vnc ) && VNC_GetTexHeight( vnc ) )
	{
		swapParms.Images[eye][1].TexId = VNC_GetTexID( vnc );

		float aspect = (float)VNC_GetWidth( vnc ) / VNC_GetHeight( vnc );
		float uScale = (float)VNC_GetWidth( vnc ) / VNC_GetTexWidth( vnc );  	// $$$ Need to query into a widget
		float vScale = (float)VNC_GetHeight( vnc ) / VNC_GetTexHeight( vnc );   //     to make this work again.

		float uOffset = (1.0f - uScale);
		float vOffset = (1.0f - vScale);

		// LOG( "aspect=%f uScale=%f vScale=%f uOffset=%f vOffset=%f", aspect, uScale, vScale, uOffset, vOffset);

		Matrix4f m = 
			Matrix4f::Scaling( aspect * -uScale, -vScale, 1.0f ) * 
		    Matrix4f::Translation( Vector3f( uOffset, vOffset, 1.25f ) );
		Matrix4f mvp = TanAngleMatrixFromUnitSquare( m );

	  	swapParms.Images[eye][1].TexCoordsFromTanAngles = mvp;
	}
#else
	swapParms.WarpProgram = WP_CHROMATIC;
#endif

	Entity_Draw( view );

	Keyboard_Draw();

	GL_CheckErrors( "draw" );

	Prof_Stop( PROF_DRAW_EYE );

	return view;
}

Matrix4f OvrApp::Frame(const VrFrame vrFrame)
{
	// LOG( "OvrApp::Frame Enter" );

	Prof_Start( PROF_FRAME );

	Matrix4f centerViewMatrix = Scene.CenterViewMatrix();
	Vector3f eyeDir = GetViewMatrixForward( centerViewMatrix );
	Vector3f eyePos = GetViewMatrixPosition( centerViewMatrix );

	// LOG( "APITest_Frame Enter" );
	// APITest_Frame();
	// LOG( "APITest_Frame Leave" );

	InQueue_Frame();

	Keyboard_Frame( vrFrame.Input.buttonState, eyePos, eyeDir );

	if ( !Keyboard_IsVisible() )
	{
		// Cmd_Add( "vnc0 touch %d", (vrFrame.Input.buttonState & BUTTON_TOUCH) != 0 );
	}

	SxVector3 gazeDir;
	Vec3Set( &gazeDir, eyeDir.x, eyeDir.y, eyeDir.z );

	if ( Vec3Dot( gazeDir, s_app.lastGazeDir ) < S_COS_ONE_TENTH_DEGREE )
	{
		Vec3Copy( gazeDir, &s_app.lastGazeDir );
		Cmd_Add( "shell gaze %f %f %f", gazeDir.x, gazeDir.y, gazeDir.z );
	}

	Cmd_Frame();

	Prof_Start( PROF_SCENE );
	VrFrame vrFrameWithoutMove = vrFrame;
	vrFrameWithoutMove.Input.sticks[0][0] = 0.0f;
	vrFrameWithoutMove.Input.sticks[0][1] = 0.0f;
	Scene.Frame( app->GetVrViewParms(), vrFrameWithoutMove, app->GetSwapParms().ExternalVelocity );
	Prof_Stop( PROF_SCENE );

	app->GetVrParms().colorFormat = COLOR_8888;

	// $$$ This sometimes spikes to 60ms, I have no idea why yet- need to instrument Oculus code.
	Prof_Start( PROF_DRAW );
	app->DrawEyeViewsPostDistorted( Scene.CenterViewMatrix() );
	Prof_Stop( PROF_DRAW );

	Prof_Stop( PROF_FRAME );

	Prof_Frame();

	// LOG( "OvrApp::Frame Leave" );

	return Scene.CenterViewMatrix();
}


sbool App_Command()
{
	if ( strcasecmp( Cmd_Argv( 0 ), "clearcolor" ) == 0 )
	{
		if ( Cmd_Argc() != 4 )
		{
			LOG( "Usage: clearcolor <r> <g> <b>" );
			return strue;
		}

		s_app.clearColor[0] = atoi( Cmd_Argv( 1 ) ) / 255.0f;
		s_app.clearColor[1] = atoi( Cmd_Argv( 2 ) ) / 255.0f;
		s_app.clearColor[2] = atoi( Cmd_Argv( 3 ) ) / 255.0f;

		LOG( "clear color is %f %f %f", s_app.clearColor[0], s_app.clearColor[1], s_app.clearColor[2] );

		return strue;
	}

	if ( strcasecmp( Cmd_Argv( 0 ), "notify" ) == 0 )
	{
		if ( Cmd_Argc() != 2 )
		{
			LOG( "Usage: notify <msg>" );
			return strue;
		}

		g_app->app->CreateToast( Cmd_Argv( 1 ) );

		return strue;
	}

	return sfalse;
}

