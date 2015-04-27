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
#include "vncwidget.h"

#include <android/keycodes.h>
#include <jni.h>

#include "PathUtils.h"

extern "C" {

jlong Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity )
{
       LOG( "nativeSetAppInterface");
       g_jni = jni;
       return (new OvrApp())->SetActivity( jni, clazz, activity );
}

} // extern "C"

JNIEnv *g_jni;
OvrApp *g_app;
SVNCWidget *vnc;

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


void APITest()
{
	ushort indices[] = 
	{ 
		0, 1, 2, 
		2, 1, 3 
	};

	float positions[] = 
	{ 
		-4.0f, -4.0f, 0.0f, 
		 4.0f, -4.0f, 0.0f,
		-4.0f,  4.0f, 0.0f, 
		 4.0f,  4.0f, 0.0f, 
	};

	float texCoords[] = 
	{ 
		-1.0f, -1.0f, 
		 1.0f, -1.0f, 
		-1.0f,  1.0f, 
		 1.0f,  1.0f, 
	};

	byte colors[] = 
	{ 
		255, 128, 255, 255, 
		255, 128, 255, 255, 
		255, 128, 255, 255, 
		255, 128, 255, 255, 
	};

	byte texels[] = 
	{ 
		255, 255, 255, 255, 
	};

	g_pluginInterface.registerPlugin( "apitest", SxPluginKind_Widget );
	g_pluginInterface.registerWidget( "apitest" );

	g_pluginInterface.registerGeometry( "quad0" );
	g_pluginInterface.sizeGeometry( "quad0", 4, 6 );
	g_pluginInterface.updateGeometryIndexRange( "quad0", 0, 6, indices );
	g_pluginInterface.updateGeometryPositionRange( "quad0", 0, 4, (SxVector3 *)positions );
	g_pluginInterface.updateGeometryTexCoordRange( "quad0", 0, 4, (SxVector2 *)texCoords );
	g_pluginInterface.updateGeometryColorRange( "quad0", 0, 4, (SxColor *)colors );
	g_pluginInterface.presentGeometry( "quad0" );

	g_pluginInterface.registerTexture( "purple0" );
	g_pluginInterface.sizeTexture( "purple0", 1, 1 );
	g_pluginInterface.formatTexture( "purple0", SxTextureFormat_R8G8B8A8 );
	g_pluginInterface.updateTextureRect( "purple0", 0, 0, 1, 1, 4, texels );
	g_pluginInterface.presentTexture( "purple0" );

	SxTrajectory tr;
	SxOrientation o;

	tr.kind = SxTrajectoryKind_Instant;

	IdentityOrientation( &o );
	Vec3Set( &o.origin, 10.0f, 0.0f, -20.0f );
	Vec3Set( &o.scale, 0.5f, 0.5f, 0.5f );
	g_pluginInterface.registerEntity( "left" );
	g_pluginInterface.setEntityGeometry( "left", "quad0" );
	g_pluginInterface.setEntityTexture( "left", "purple0" );
	g_pluginInterface.orientEntity( "left", &o, &tr );

	IdentityOrientation( &o );
	Vec3Set( &o.origin, 0.0f, 10.0f, 0.0f );
	g_pluginInterface.registerEntity( "left_child" );
	g_pluginInterface.setEntityGeometry( "left_child", "quad0" );
	g_pluginInterface.setEntityTexture( "left_child", "purple0" );
	g_pluginInterface.orientEntity( "left_child", &o, &tr );
	g_pluginInterface.parentEntity( "left_child", "left" );

	IdentityOrientation( &o );
	Vec3Set( &o.origin, -10.0f, 0.0f, -20.0f );
	g_pluginInterface.registerEntity( "right" );
	g_pluginInterface.setEntityGeometry( "right", "quad0" );
	g_pluginInterface.setEntityTexture( "right", "purple0" );
	g_pluginInterface.orientEntity( "right", &o, &tr );

	IdentityOrientation( &o );
	Vec3Set( &o.origin, 0.0f, 10.0f, 0.0f );
	g_pluginInterface.registerEntity( "right_child" );
	g_pluginInterface.setEntityGeometry( "right_child", "quad0" );
	g_pluginInterface.setEntityTexture( "right_child", "purple0" );
	g_pluginInterface.orientEntity( "right_child", &o, &tr );
	g_pluginInterface.parentEntity( "right_child", "right" );
}


void OvrApp::OneTimeInit( const char * launchIntent )
{
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

	Keyboard_Init();

	vnc = VNC_CreateWidget();

	APITest();

	// VNC_Connect( vnc, "10.0.1.39:0", "asdf" ); // home; phone is 10.0.1.4
	VNC_Connect( vnc, "192.168.43.9:0", "asdf" ); // hotspot; phone is 192.168.43.1
	// VNC_Connect( vnc, "192.168.0.103:0", "asdf" ); // rangeley
	// VNC_Connect( vnc, "10.90.240.66:0", "asdf" );
	// VNC_Connect( vnc, "10.90.240.93:0", "asdf" );
}

void OvrApp::OneTimeShutdown()
{
	VNC_DestroyWidget( vnc );

	Registry_Shutdown();
	Thread_Shutdown();
}

void OvrApp::Command( const char * msg )
{
}

bool OvrApp::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	uint 	vncCode;
	bool 	isDown;

	if ( keyCode == AKEYCODE_BACK )
	{
		if ( eventType == KeyState::KEY_EVENT_SHORT_PRESS )
			Keyboard_Toggle();

		return true;
	}

	if ( eventType == KeyState::KEY_EVENT_DOWN ||
		 eventType == KeyState::KEY_EVENT_UP )
	{
		vncCode = VNC_KeyCodeForAndroidCode( keyCode );
		
		if ( vncCode != INVALID_KEY_CODE )
		{
			isDown = (eventType == KeyState::KEY_EVENT_DOWN);

			VNC_KeyboardEvent( vnc, vncCode, isDown );
		}

		return true;
	}

	return false;
}

Matrix4f OvrApp::DrawEyeView( const int eye, const float fovDegrees )
{
	Prof_Start( PROF_DRAW_EYE );

	const Matrix4f view = Scene.DrawEyeView( eye, fovDegrees );

	glClearColor( 0.5f, 0.5f, 0.5f, 1.0f );
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
		float uScale = (float)VNC_GetWidth( vnc ) / VNC_GetTexWidth( vnc );
		float vScale = (float)VNC_GetHeight( vnc ) / VNC_GetTexHeight( vnc );

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

	VNC_DrawWidget( vnc, view );
#endif

	Entity_Draw( view );

	Keyboard_Draw();

	GL_CheckErrors( "draw" );

	Prof_Stop( PROF_DRAW_EYE );

	return view;
}

Matrix4f OvrApp::Frame(const VrFrame vrFrame)
{
	Prof_Start( PROF_FRAME );

	Matrix4f centerViewMatrix = Scene.CenterViewMatrix();
	Vector3f eyeDir = GetViewMatrixForward( centerViewMatrix );
	Vector3f eyePos = GetViewMatrixPosition( centerViewMatrix );

	InQueue_Frame();

	VNC_UpdateWidget( vnc );

	if ( !Keyboard_IsVisible() )
	{
		sbool touch = (vrFrame.Input.buttonState & BUTTON_TOUCH) != 0;
		VNC_UpdateHeadMouse( vnc, eyePos, eyeDir, touch );
	}

	Keyboard_Frame( vrFrame.Input.buttonState, eyePos, eyeDir );

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

	return Scene.CenterViewMatrix();
}


sbool App_Command()
{
	return sfalse;
}

