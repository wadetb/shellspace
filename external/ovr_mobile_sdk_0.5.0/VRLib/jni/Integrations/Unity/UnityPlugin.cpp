/************************************************************************************

Filename    :   UnityPlugin.cpp
Content     :   Hijack the screen from Unity and mess with the render textures
Created     :   November 11, 2013
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
#include <jni.h>
#include <unistd.h>						// usleep, etc
#include <sys/syscall.h>

#include "OVR.h"
#include "Android/GlUtils.h"
#include "Android/LogUtils.h"
#include "Android/JniUtils.h"

#include "VrApi/VrApi.h"
#include "VrApi/VrApi_Android.h"
#include "VrApi/VrApi_Helpers.h"
#include "VrApi/LocalPreferences.h"		// FIXME:VRAPI move to VrApi_Android.h?
#include "VrApi/SystemActivities.h"

#include "Kernel/OVR_JSON.h"

#include "GlStateSave.h"
#include "MediaSurface.h"

// App.h should NOT be included, only stand-alone code!
#include "../../EyePostRender.h"
#include "../../UserProfile.h"
#include "PackageFiles.h"
#include "VrLocale.h"
#include "Console.h"

typedef enum
{
	ovrEye_Left  = 0,
	ovrEye_Right = 1,
	ovrEye_Count = 2
} ovrEyeType;

#define OCULUS_EXPORT

using namespace OVR;

class UnityPlugin
{
public:
                    UnityPlugin() : 
						initialized( false ),
						activity( NULL ),
						vrActivityClass( NULL ),
						OvrMobile( NULL ),
						resetClockLocks( false ),
						eyeTextures(),
						jni( NULL ),
						focused( false ),
						allowFovIncrease( false ),
						HighQualityWarpProgs( false ),
						OverlayPlaneProgram( 0 ),
						scriptThreadTid( 0 ),
						renderThreadTid( 0 ),
						viewCount( 0 ),
						fbWidth( 1024 ),
						fbHeight( 1024 ),
						monoscopic( false ),
						showVignette( true ),
						showLoadingIcon( false ),
						LogEyeSceneGpuTime(),
						countApplicationFrames( 0 ),
						lastReportTime( 0.0 ),
						eventData(),
						ErrorTexture( 0 ),
						ErrorTextureSize( 0 ),
						ErrorMessageEndTime( -1.0 )
					{
						// Default ovrModeParms
						VrModeParms.AsynchronousTimeWarp = true;
						VrModeParms.AllowPowerSave = true;
						VrModeParms.DistortionFileName = NULL;
						VrModeParms.EnableImageServer = false;
						VrModeParms.SkipWindowFullscreenReset = false;
						VrModeParms.CpuLevel = 2;
						VrModeParms.GpuLevel = 2;
						VrModeParms.GameThreadTid = 0;

						// Default ovrTimeWarpParms
						SwapParms = InitTimeWarpParms();

						memset( sensorForView, 0, sizeof( sensorForView ) );
						for ( int i = 0; i < MAX_VIEWS; i++ )
						{
							sensorForView[i].Predicted.Pose.Orientation = OVR::Quat<float>();
						}
					}

	bool			initialized;

	jobject			activity;
	jclass			vrActivityClass;

	ovrMobile *		OvrMobile;
	ovrModeParms	VrModeParms;
	bool			resetClockLocks;

	GLuint			eyeTextures[ovrEye_Count];

	JNIEnv *		jni;

	bool			focused;
	bool			allowFovIncrease;

	bool			HighQualityWarpProgs;
	int				OverlayPlaneProgram;

	int				scriptThreadTid;
	int				renderThreadTid;

	ovrHmdInfo		hmdInfo;
	ovrTimeWarpParms	SwapParms;			// passed to TimeWarp->WarpSwap()

	// Time and orientation for eye rendering, used as base for time warping.
	//
	// With the multithreaded renderer, we need to timewarp from two frames in the past
	int				viewCount;
	static const int	MAX_VIEWS = 64;		// deliberately large to highlight errors
	static const int	VIEW_MASK = MAX_VIEWS - 1;
	ovrSensorState	sensorForView[MAX_VIEWS];

	int				fbWidth;
	int				fbHeight;

	bool			monoscopic;			    // app wants to use the left eye texture for both eyes
	bool			showVignette;			// render the vignette
	bool			showLoadingIcon;		// render the loading indicator

	// GPU time queries around eye scene rendering.
	LogGpuTime<2>	LogEyeSceneGpuTime;

	int				countApplicationFrames;
	double			lastReportTime;

	// Plugin data channel
	static const int MAX_EVENTS = 32;       // plugin data channel relies on this value
	int             eventData[MAX_EVENTS * 2];  // allow 2 "2-bytes" data events per event type

	EyePostRender	EyeDecorations;

	// Media surface for video player support in Unity
	MediaSurface	VideoSurface;

	GLuint			ErrorTexture;
	int				ErrorTextureSize;
	double			ErrorMessageEndTime;
};

UnityPlugin	up;

extern "C"
{


void OVR_Resume()
{
	LOG( "OVR_Resume()" );
	if ( !up.initialized )
	{
		LOG( "OVR_Resume: Plugin uninitialized" );
		return;
	}
	if ( up.focused )
	{
		LOG( "Already focused, skipping" );
		return;
	}

	// Reload local preferences, in case we are coming back from a
	// switch to the dashboard that changed them.
	ovr_UpdateLocalPreferences();

	// Check for values that effect our mode settings
	{
		const char * imageServerStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_IMAGE_SERVER, "0" );
		up.VrModeParms.EnableImageServer = ( atoi( imageServerStr ) > 0 );

		const char * cpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_CPU_LEVEL, "-1" );
		const int cpuLevel = atoi( cpuLevelStr );
		if ( cpuLevel >= 0 )
		{
			up.VrModeParms.CpuLevel = cpuLevel;
			LOG( "Local Preferences: Setting cpuLevel %d", up.VrModeParms.CpuLevel );
		}
		const char * gpuLevelStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_GPU_LEVEL, "-1" );
		const int gpuLevel = atoi( gpuLevelStr );
		if ( gpuLevel >= 0 )
		{
			up.VrModeParms.GpuLevel = gpuLevel;
			LOG( "Local Preferences: Setting gpuLevel %d", up.VrModeParms.GpuLevel );
		}

		const char * showVignetteStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_SHOW_VIGNETTE, "1" );
		up.showVignette = ( atoi( showVignetteStr ) > 0 );

		const char * enableGpuTimingsStr = ovr_GetLocalPreferenceValueForKey( LOCAL_PREF_DEV_GPU_TIMINGS, "0" );
		SetAllowGpuTimerQueries( atoi( enableGpuTimingsStr ) > 0 );
	}

	up.OvrMobile = ovr_EnterVrMode( up.VrModeParms, &up.hmdInfo );

	up.focused = true;
}

void OVR_Pause()
{
	LOG( "OVR_Pause()" );
	if ( !up.initialized )
	{
		LOG( "OVR_Pause: Uninitialized" );
		return;
	}
	if ( !up.focused )
	{
		LOG( "Already paused, skipping" );
		return;
	}

	ovr_LeaveVrMode( up.OvrMobile );

	up.focused = false;
}

OCULUS_EXPORT jobject OVR_Media_Surface( int texId )
{
	LOG( "OVR_Media_Surface(%i)", texId );
	return up.VideoSurface.Bind( texId );
}

OCULUS_EXPORT void OVR_TW_SetDebugMode( int mode, int value )
{
	LOG( "OVR_TW_SetDebugMode(%i,%i)", mode, value );
	up.SwapParms.DebugGraphMode = (ovrTimeWarpDebugPerfMode)mode;
	up.SwapParms.DebugGraphValue = (ovrTimeWarpDebugPerfValue)value;
}

OCULUS_EXPORT void OVR_TW_SetMinimumVsyncs( int minimumVsyncs )
{
	LOG( "OVR_TW_SetMinimumVsyncs() %d", minimumVsyncs );
	up.SwapParms.MinimumVsyncs = minimumVsyncs;
}

OCULUS_EXPORT void OVR_TW_AllowFovIncrease( bool allow )
{
	up.allowFovIncrease = allow;
}

OCULUS_EXPORT void OVR_TW_EnableChromaticAberration( bool enable )
{
	LOG( "OVR_TW_EnableChromaticAberration() %d", enable );
	up.HighQualityWarpProgs = enable;
}

OCULUS_EXPORT void OVR_TW_SetOverlayPlane( int texId, int eye, int program,
		float m0, float m1, float m2, float m3,
		float m4, float m5, float m6, float m7,
		float m8, float m9, float m10, float m11,
		float m12, float m13, float m14, float m15 )
{
	const ovrMatrix4f mv =
	{ {
		{  m0,  m1,  m2,  m3 },
		{  m4,  m5,  m6,  m7 },
		{  m8,  m9, m10, m11 },
		{ m12, m13, m14, m15 }
	} };
//	LOG( "SetOverlayPlane( %i, %i, %i ) log", texId, eye, program );
//	LogMatrix( "overlay:", mv );
	up.SwapParms.Images[eye][1].TexId = texId;
	up.SwapParms.Images[eye][1].TexCoordsFromTanAngles = TanAngleMatrixFromUnitSquare( &mv );
	up.OverlayPlaneProgram = program;
}

OCULUS_EXPORT void OVR_TW_SetGazeCursor( int texId, bool trails, float interPupilaryDistance,
		float cursorDistance, float cursorSize, float r, float g, float b, float a )
{
#if 0
	up.SwapParms.Cursor.TexId = texId;
	up.SwapParms.Cursor.Trails = trails;
	up.SwapParms.Cursor.CursorDistance = cursorDistance;
	up.SwapParms.Cursor.InterPupilaryDistance = interPupilaryDistance;
	up.SwapParms.Cursor.CursorSize = cursorSize;
	up.SwapParms.Cursor.Color[0] = r;
	up.SwapParms.Cursor.Color[1] = g;
	up.SwapParms.Cursor.Color[2] = b;
	up.SwapParms.Cursor.Color[3] = a;
#endif
}

OCULUS_EXPORT void OVR_SetInitVariables( jobject activity, jclass vrActivityClass )
{
	LOG( "OVR_SetInitVariables()" );
	up.activity = activity;
	up.vrActivityClass = vrActivityClass;
	up.scriptThreadTid = gettid();
}

OCULUS_EXPORT void OVR_SetEyeParms( int width, int height )
{
	LOG( "OVR_SetEyeParms() w=%d h=%d", width, height );
	up.fbWidth = width;
	up.fbHeight = height;
}

OCULUS_EXPORT void OVR_VrModeParms_SetAsyncTimeWarp( bool enable )
{
	LOG( "OVR_VrModeParms_SetAsyncTimeWarp(): %d", enable );
	up.VrModeParms.AsynchronousTimeWarp = enable;
}

OCULUS_EXPORT void OVR_VrModeParms_SetAllowPowerSave( bool allow )
{
	LOG( "OVR_VrModeParms_SetAllowPowerSave(): %d", allow );
	up.VrModeParms.AllowPowerSave = allow;
}

OCULUS_EXPORT void OVR_VrModeParms_SetCpuLevel( int cpuLevel )
{
	LOG( "OVR_VrModeParms_SetCpuLevel(): L %d", cpuLevel );
	up.VrModeParms.CpuLevel = cpuLevel;
	up.resetClockLocks = true;
}

OCULUS_EXPORT void OVR_VrModeParms_SetGpuLevel( int gpuLevel )
{
	LOG( "OVR_VrModeParms_SetGpuLevel(): L %d", gpuLevel );
	up.VrModeParms.GpuLevel = gpuLevel;
	up.resetClockLocks = true;
}

// Apply and changes to VrModeParms
void OVR_VrModeParms_Reset()
{
	LOG( "OVR_VrModeParms_Reset()" );
	if ( !up.initialized ) {
		LOG( "OVR_VrModeParms_Reset: Uninitialized" );
		return;
	}

	// NOTE: Dynamically updating certain VrModeParms require us
	// to leave and then re-enter VR mode, causing a frame of flicker.
	// Clock Levels do not require a leave/re-enter cycle.

	if ( up.resetClockLocks )
	{
		LOG( "OVR_VrModeParms_Reset: Clock Lock Reset" );
		ovr_AdjustClockLevels( up.OvrMobile, up.VrModeParms.CpuLevel, up.VrModeParms.GpuLevel );
		up.resetClockLocks = false;
	}
}

// FIXME: rename this?
OCULUS_EXPORT void OVR_Platform_StartUI( const char * commandString )
{
	LOG( "OVR_StartPlatformUI( %s )", commandString );

	if ( ovr_StartSystemActivity( up.OvrMobile, commandString, NULL ) ) {
		return;
	}

	if ( up.ErrorTexture != 0 )
	{
		// already in an error state
		return;
	}

	// clear any pending exception because to ensure no pending exception causes the error message to fail
	if ( up.jni->ExceptionOccurred() )
	{
		up.jni->ExceptionClear();
	}

	OVR::String imageName = "dependency_error";
	char language[128];
	ovr_GetCurrentLanguage( up.OvrMobile, language, sizeof( language ) );
	imageName += "_";
	imageName += language;
	imageName += ".png";

	void * imageBuffer = NULL;
	int imageSize = 0;
	if ( !ovr_FindEmbeddedImage( up.OvrMobile, imageName.ToCStr(), imageBuffer, imageSize ) )
	{
		// try to default to English
		imageName = "dependency_error_en.png";
		if ( !ovr_FindEmbeddedImage( up.OvrMobile, imageName.ToCStr(), imageBuffer, imageSize ) )
		{
			FAIL( "Failed to load error message texture!" );
		}
	}

	OVR::MemBuffer memBuffer( imageBuffer, static_cast< int >( imageSize ) );
	int h = 0;
	// Note that the extension used on the filename passed here is important! It must match the type
	// of file that was embedded.
	up.ErrorTexture = LoadTextureFromBuffer( imageName, memBuffer, OVR::TextureFlags_t(), up.ErrorTextureSize, h );
	OVR_ASSERT( up.ErrorTextureSize == h );

	up.ErrorMessageEndTime = ovr_GetTimeInSeconds() + 7.5;
}

void OVR_InitRenderThread()
{
	if ( up.initialized )
	{
		return;
	}

	LOG( "OVR_InitRenderThread()" );
	GL_CheckErrors( "OVR_InitRenderThread() entry" );

	// We have a javaVM from the .so load
	if ( VrLibJavaVM != NULL )
	{
		VrLibJavaVM->AttachCurrentThread(&up.jni, 0);
	}
	else
	{
		FAIL( "!VrLibJavaVM -- ovr_OnLoad() not called yet" );
	}

	// Look up extensions
	GL_FindExtensions();

	up.VrModeParms.ActivityObject = up.activity;
	up.VrModeParms.AsynchronousTimeWarp = true;
	up.VrModeParms.DistortionFileName = NULL;

	up.VrModeParms.GameThreadTid = up.scriptThreadTid;

	LOG( "Mode Parms CpuLevel %d GpuLevel %d", up.VrModeParms.CpuLevel, up.VrModeParms.GpuLevel );

	up.renderThreadTid = gettid();

	up.HighQualityWarpProgs = false;

	// Screen vignettes, calibration grids, programs, etc.
	up.EyeDecorations.Init();

	up.initialized = true;

	up.VideoSurface.Init( up.jni );

	ovr_InitLocalPreferences( up.jni, up.activity );

	// Register console functions
	InitConsole();
	RegisterConsoleFunction( "print", OVR::DebugPrint );

	GL_CheckErrors( "OVR_InitRenderThread exit" );

	OVR_Resume();

	LOG( "OVR_InitRenderThread() - Finished" );
}

void OVR_ShutdownRenderThread()
{
	if ( !up.initialized )
	{
		return;
	}

	ShutdownConsole();

	LOG( "OVR_ShutdownRenderThread()" );

	up.EyeDecorations.Shutdown();

	up.VideoSurface.Shutdown();

	ovr_ShutdownLocalPreferences();

	ovr_Shutdown();

	if ( up.ErrorTexture != 0 )
	{
		glDeleteTextures( 1, &up.ErrorTexture );
		up.ErrorTexture = 0;
	}

	up.initialized = false;

	LOG( "OVR_ShutdownRenderThread() - Finished" );
}

// returns current volume level
OCULUS_EXPORT int OVR_GetVolume()
{
    if ( !up.initialized )
    {
    	LOG( "OVR_GetVolume() : Unity plugin not initialized" );
    	return -1;
    }

    int volume = ovr_GetVolume();
    //LOG( "OVR_GetVolume() : %d", volume );
    return volume;
}

// returns time since last volume change
OCULUS_EXPORT double OVR_GetTimeSinceLastVolumeChange()
{
    if ( !up.initialized )
    {
    	LOG( "OVR_GetTimeSinceLastVolumeChange() : Unity plugin not initialized" );
    	return -1;
    }

    const double deltaTime = ovr_GetTimeSinceLastVolumeChange();
    //LOG( "OVR_GetTimeSinceLastVolumeChange() : %f", ( float )deltaTime );
    return deltaTime;
}

// returns battery level [0.0,1.0]
OCULUS_EXPORT float OVR_GetBatteryLevel()
{
	if ( !up.initialized )
	{
		return 1.0f;
	}

	batteryState_t state = ovr_GetBatteryState();
	//LOG( "OVR_GetBatteryLevel() : %d", state.level );
	return OVR::Alg::Clamp( static_cast<float>( state.level ) / 100.0f, 0.0f, 1.0f );
}

// returns battery status - see eBatteryStatus
OCULUS_EXPORT int OVR_GetBatteryStatus()
{
	if ( !up.initialized )
	{
		return 0;
	}
	batteryState_t state = ovr_GetBatteryState();
	//LOG( "OVR_GetBatteryStatus() : %i", state.status );
	return state.status;
}

// returns battery temperature in degrees Celsius
OCULUS_EXPORT float OVR_GetBatteryTemperature()
{
	if ( !up.initialized )
	{
		return 0.0f;
	}

	batteryState_t state = ovr_GetBatteryState();

	// tenths of a degree centigrade
	const float temperature = static_cast<float>( state.temperature ) / 10.0f;
	//LOG( "OVR_GetBatteryTemperature() : %fC", temperature );

	return temperature;
}

OCULUS_EXPORT void OVR_RequestAudioFocus()
{
	LOG( "OVR_RequestAudioFocus()" );

	if ( !up.initialized )
	{
		return;
	}


	ovr_RequestAudioFocus( up.OvrMobile );
}

OCULUS_EXPORT void OVR_AbandonAudioFocus()
{
	LOG( "OVR_AbandonAudioFocus()" );

	if ( !up.initialized )
	{
		return;
	}

	ovr_ReleaseAudioFocus( up.OvrMobile );
}

OCULUS_EXPORT bool OVR_GetHeadsetPluggedState()
{
	return ovr_GetHeadsetPluggedState();
}

OCULUS_EXPORT bool OVR_IsPowerSaveActive()
{
	return ovr_GetPowerLevelStateThrottled();
}

OCULUS_EXPORT void OVR_ShowLoadingIcon( bool show )
{
	LOG( "OVR_ShowLoadingIcon() : %d", show );
	up.showLoadingIcon = show;
}

//---------------------------
// OVR_CameraEndFrame
//
// Called by Unity's OnPostRender().
//
// End Eye Rendering
//
//---------------------------
void OVR_CameraEndFrame( ovrEyeType eye, int textureId )
{
//	LOG( "%f OVR_CameraEndFrame(%i) : texId:%i", ovr_GetTimeInSeconds(), eye, textureId );

	if ( eye < 0 || eye > 1 )
	{
		return;
	}

	if ( !up.focused )
	{
		return;
	}

	// WORKAROUND: On Mali with static-batching enabled, Unity leaves
	// ibo mapped entire frame. When we inject our vignette and timewarp
	// rendering with the ibo mapped, rendering corruption will occur.
	// Explicitly unbind here.
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	up.eyeTextures[ eye ] = textureId;

	if ( up.showVignette )
	{
		GLStateSave glstate;	// restore state on destruction

		// Forcing as much as I can think of to get the vignette to always draw...
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_SCISSOR_TEST );
		glDisable( GL_CULL_FACE );

		// Draw a thin vignette at the edges of the view so clamping will give black
		up.EyeDecorations.FillEdge( up.fbWidth, up.fbHeight );
	}

	//	up.EyeDecorations.DrawEyeCalibrationLines( up.hmdInfo.SuggestedEyeFov[0], eye );

	// Discard the depth buffer, so the tiler won't need to write it back out to memory
	GL_InvalidateFramebuffer( INV_FBO, false, true );

	// Get this eye rendering right away, so it can
	// overlap with the commands for the next eye,
	// or game logic.
	glFlush();
}

float CalcFovIncrease()
{
	// Increase the fov by about 10 degrees if we are not holding 60 fps so
	// there is less black pull-in at the edges.
	//
	// Doing this dynamically based just on time causes visible flickering at the
	// periphery when the fov is increased, so only do it if minimumVsyncs is set.
	float fovIncrease = ( up.allowFovIncrease && 
								( ( up.SwapParms.MinimumVsyncs > 1 ) || ovr_GetPowerLevelStateThrottled() ) ) ? 10.0f : 0.0f;

	// Increase the fov when not rendering the vignette to hide
	// edge artifacts
	fovIncrease += ( !up.showVignette ) ? 5.0f : 0.0f;

	return fovIncrease;
}

// Returns the orientation to use for the next eye renders and
// a view index that can be passed to the renderer to fetch the
// same sensor data for time warp.
OCULUS_EXPORT void OVR_GetSensorState( bool monoscopic,
								float &w,
								float &x,
								float &y,
								float &z,
								float &fov,
								int & viewNumber )
{
	if ( !up.initialized )
	{
		x = 0;
		y = 0;
		z = 0;
		w = 1;
		viewNumber = 0;
		return;
	}

	up.monoscopic = monoscopic;

	// Get the latest head tracking state, predicted ahead to the midpoint of the time
	// it will be displayed.  It will always be corrected to the real values by the
	// time warp, but the closer we get, the less black will be pulled in at the edges.
	const double now = ovr_GetTimeInSeconds();
	static double prev;
	const double rawDelta = now - prev;
	prev = now;
	const double clampedPrediction = Alg::Min( 0.1, rawDelta * 2 );
	ovrSensorState sensor = ovr_GetPredictedSensorState( up.OvrMobile, now + clampedPrediction );

	// To test timewarp, always return a fixed prediction orientation for the rendering
	if ( 0 )
	{
		sensor.Predicted.Pose.Orientation.x = 0;
		sensor.Predicted.Pose.Orientation.y = 0;
		sensor.Predicted.Pose.Orientation.z = 0;
		sensor.Predicted.Pose.Orientation.w = 1;
	}

	viewNumber = up.viewCount & up.VIEW_MASK;
	up.viewCount++;
	up.sensorForView[viewNumber] = sensor;

	w = sensor.Predicted.Pose.Orientation.w;
	x = sensor.Predicted.Pose.Orientation.x;
	y = sensor.Predicted.Pose.Orientation.y;
	z = sensor.Predicted.Pose.Orientation.z;

	fov = up.hmdInfo.SuggestedEyeFov[0] + CalcFovIncrease();

//	LOG( "GetSensorState: view %i = %f %f %f %f", viewNumber, x, y, z, w );
}


// This is called by the script thread
void OVR_TimeWarpEvent( const int viewIndex )
{
	if ( !up.focused || viewIndex < 0 || viewIndex >= up.MAX_VIEWS )
	{
		return;
	}

	up.LogEyeSceneGpuTime.End( 0 );

	// if there is an error condition, warp swap and nothing else
	if ( up.ErrorTexture != 0 )
	{
		GLStateSave glState;
		if ( ovr_GetTimeInSeconds() >= up.ErrorMessageEndTime )
		{
			ovr_ReturnToHome( up.OvrMobile );
		} 
		else 
		{
			ovrTimeWarpParms warpSwapMessageParms = InitTimeWarpParms( WARP_INIT_MESSAGE, up.ErrorTexture );
			warpSwapMessageParms.ProgramParms[0] = 0.0f;							// rotation in radians
			warpSwapMessageParms.ProgramParms[1] = 1024.0f / up.ErrorTextureSize;	// message size factor
			ovr_WarpSwap( up.OvrMobile, &warpSwapMessageParms );
		}
		return;
	}
	if ( !up.eyeTextures[0] || ( !up.monoscopic && !up.eyeTextures[1] ) )
	{	// don't have both eyes yet
		LOG( "OVR_TimeWarp() -- don't have textures yet" );
	}
	else if ( up.showLoadingIcon )
	{
		GLStateSave glstate;	// restore state on destruction

		const ovrTimeWarpParms warpSwapLoadingIconParms = InitTimeWarpParms( WARP_INIT_LOADING_ICON );
		ovr_WarpSwap( up.OvrMobile, &warpSwapLoadingIconParms );
	}
	else
	{
		GLStateSave glstate;	// restore state on destruction

		const ovrSensorState & sensor = up.sensorForView[ viewIndex ];
		if ( 0 )
		{
			LOG( "TimeWarp: view %i = %f %f %f %f", viewIndex,
					sensor.Predicted.Pose.Orientation.x,
					sensor.Predicted.Pose.Orientation.y,
					sensor.Predicted.Pose.Orientation.z,
					sensor.Predicted.Pose.Orientation.w );
		}

		const float fovDegrees = up.hmdInfo.SuggestedEyeFov[0] + CalcFovIncrease();

		for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
		{
			up.SwapParms.Images[eye][0].TexCoordsFromTanAngles = TanAngleMatrixFromFov( fovDegrees );
			up.SwapParms.Images[eye][0].TexId = up.eyeTextures[up.monoscopic ? 0 : eye];
			up.SwapParms.Images[eye][0].Pose = sensor.Predicted;

			// Also update the pose on the second image, in case the overlay plane is active
			up.SwapParms.Images[eye][1].Pose = sensor.Predicted;
		}

		switch( up.OverlayPlaneProgram )
		{
		default:
			up.SwapParms.WarpProgram = up.HighQualityWarpProgs ? WP_CHROMATIC : WP_SIMPLE;
			break;
		case 1:
			up.SwapParms.WarpProgram = up.HighQualityWarpProgs ? WP_MASKED_PLANE : WP_CHROMATIC_MASKED_PLANE;
			break;
		case 2:
			up.SwapParms.WarpProgram = up.HighQualityWarpProgs ? WP_CHROMATIC_OVERLAY_PLANE : WP_OVERLAY_PLANE;
			break;
		case 3:
			up.SwapParms.WarpProgram = WP_OVERLAY_PLANE_SHOW_LOD;
			break;
		}

		ovr_WarpSwap( up.OvrMobile, &up.SwapParms );

		// The overlay must be re-specified every frame.
		up.OverlayPlaneProgram = 0;
	}

	ovr_HandleDeviceStateChanges( up.OvrMobile );

	up.LogEyeSceneGpuTime.Begin( 0 );

	// Report frame counts once a second
	up.countApplicationFrames++;
	const double timeNow = floor( ovr_GetTimeInSeconds() );
	if ( timeNow > up.lastReportTime )
	{
		LOG( "FPS: %i GPU time: %3.1f ms",
				up.countApplicationFrames,  up.LogEyeSceneGpuTime.GetTotalTime() );

		up.countApplicationFrames = 0;
		up.lastReportTime = timeNow;
	}
}

// Note: These must be kept in sync with the Unity RenderEventType
enum RenderEventType 
{
	EVENT_INIT_RENDERTHREAD,
	EVENT_PAUSE,
	EVENT_RESUME,
	EVENT_LEFTEYE_ENDFRAME,
	EVENT_RIGHTEYE_ENDFRAME,
	EVENT_TIMEWARP,
	EVENT_PLATFORMUI_GLOBALMENU,
	EVENT_PLATFORMUI_CONFIRM_QUIT,
	EVENT_RESET_VRMODEPARMS,
	EVENT_PLATFORMUI_TUTORIAL,
	EVENT_SHUTDOWN_RENDERTHREAD,
	NUM_EVENTS
};

// FIXME: OVR compile time assert doesn't work outside of functions
//OVR_COMPILER_ASSERT( NUM_EVENTS < up.MAX_EVENTS );

static const UInt32 IS_DATA_FLAG = 0x80000000;
static const UInt32 DATA_POS_MASK = 0x40000000;
static const UInt32 DATA_POS_SHIFT = 30;
static const UInt32 EVENT_TYPE_MASK = 0x3E000000;
static const UInt32 EVENT_TYPE_SHIFT = 25;
static const UInt32 PAYLOAD_MASK = 0x0000FFFF;
static const UInt32 PAYLOAD_SHIFT = 16;

static bool EventContainsData( const int eventID )
{
	return ( ( (UInt32)eventID & IS_DATA_FLAG ) != 0 );
}

static void DecodeDataEvent( const int eventData, int & outEventId, int & outPos, int & outData )
{
	assert( EventContainsData( eventData ) );

	UInt32 pos =     ( ( (UInt32)eventData & DATA_POS_MASK ) >> DATA_POS_SHIFT );
	UInt32 eventId = ( ( (UInt32)eventData & EVENT_TYPE_MASK ) >> EVENT_TYPE_SHIFT );
	UInt32 payload = ( ( (UInt32)eventData & PAYLOAD_MASK ) << ( PAYLOAD_SHIFT * pos ) );

	outEventId = eventId;
	outPos = pos;
	outData = payload;
}

// When Unity's multi-threaded renderer is enabled, the GL context is never current for
// the script execution thread, so the only way for a plugin to execute GL code is to
// have it done through the GL.IssuePluginEvent( int ) call, which calls this function.
OCULUS_EXPORT void UnityRenderEvent( int eventID )
{
	if ( EventContainsData( eventID ) ) {
		int outEventId = 0;
		int outPos = 0;
		int outData = 0;
		DecodeDataEvent( eventID, outEventId, outPos, outData );

		up.eventData[ outEventId * 2 + outPos ] = outData;
		//LOG( "UnityRenderEvent %i %i %i", outEventId, outPos, outData );
		return;
	}

//	LOG( "UnityRenderEvent %i", eventID );

	switch( eventID )
	{
	case EVENT_INIT_RENDERTHREAD:
		OVR_InitRenderThread();
		break;
	case EVENT_SHUTDOWN_RENDERTHREAD:
		OVR_ShutdownRenderThread();
		break;
	case EVENT_PAUSE:
		OVR_Pause();
		break;
	case EVENT_RESUME:
		OVR_Resume();
		break;
	case EVENT_LEFTEYE_ENDFRAME:
	{
		const int eventData = up.eventData[eventID * 2 + 0] + up.eventData[eventID * 2 + 1];
		OVR_CameraEndFrame( ovrEye_Left, eventData );
		break;
	}
	case EVENT_RIGHTEYE_ENDFRAME:
	{
		const int eventData = up.eventData[eventID * 2 + 0] + up.eventData[eventID * 2 + 1];
		OVR_CameraEndFrame( ovrEye_Right, eventData );
		break;
	}
	case EVENT_TIMEWARP:
	{
		const int eventData = up.eventData[eventID * 2 + 0] + up.eventData[eventID * 2 + 1];
	//	LOG( "OVR_TimeWarpEvent with view index %i", eventData );
		OVR_TimeWarpEvent( eventData );

		// Update the movie surface, if active.
		up.VideoSurface.Update();
		break;
	}
	case EVENT_PLATFORMUI_GLOBALMENU:
		OVR_Platform_StartUI( PUI_GLOBAL_MENU );
		break;
	case EVENT_PLATFORMUI_CONFIRM_QUIT:
		OVR_Platform_StartUI( PUI_CONFIRM_QUIT );
		break;
	case EVENT_RESET_VRMODEPARMS:
		OVR_VrModeParms_Reset();
		break;
	case EVENT_PLATFORMUI_TUTORIAL:
		OVR_Platform_StartUI( PUI_GLOBAL_MENU_TUTORIAL );
		break;
	default:
		LOG( "Invalid Event ID %i", eventID );
		break;
	}
}

// prints a message with a specific tag that can be captured and filtered with adb logcat
OCULUS_EXPORT int OVR_DebugPrint( const char * tag, const char * message ) 
{
	__android_log_print( ANDROID_LOG_WARN, tag, "%s", message );
	return 0;
}

//---------------------------
// VrApi Exports
//---------------------------

//---------------------------
// Sensor functions
//---------------------------

// Used for prediction
static bool			 s_PredictionOn		 = true;
static float		 s_PredictionTime    = 0.03f;

OCULUS_EXPORT bool OVR_IsHMDPresent()
{
	ovrSensorState ss = ovr_GetPredictedSensorState( up.OvrMobile, 0 );
	
	return ( ss.Status | ovrStatus_HmdConnected ) != 0;
}

//---------------------------
OCULUS_EXPORT bool OVR_ResetSensorOrientation()
{
	ovr_RecenterYaw( up.OvrMobile );
	return true;
}

//---------------------------
OCULUS_EXPORT bool OVR_GetAcceleration( float &x, float &y, float &z )
{
	ovrSensorState ss = ovr_GetPredictedSensorState( up.OvrMobile, 0 );
	x = ss.Recorded.LinearAcceleration.x;
	y = ss.Recorded.LinearAcceleration.y;
	z = ss.Recorded.LinearAcceleration.z;
	return true;
}

//---------------------------
OCULUS_EXPORT bool OVR_GetAngularVelocity( float &x, float &y, float &z )
{
	ovrSensorState ss = ovr_GetPredictedSensorState( up.OvrMobile, 0 );
	x = ss.Recorded.AngularVelocity.x;
	y = ss.Recorded.AngularVelocity.y;
	z = ss.Recorded.AngularVelocity.z;
	return true;
}

OCULUS_EXPORT bool OVR_GetCameraPositionOrientation( float &px, float &py, float &pz,
												    float &ox, float &oy, float &oz, float &ow, double atTime )
{
	double abs_time_plus_pred = ovr_GetTimeInSeconds();
	
	// TODO: Is this correct for mobile?
	if ( s_PredictionOn )
	{
		abs_time_plus_pred += s_PredictionTime;
	}

	ovrSensorState ss = ovr_GetPredictedSensorState( up.OvrMobile, abs_time_plus_pred );

	px = ss.Predicted.Pose.Position.x;
	py = ss.Predicted.Pose.Position.y;
	pz = ss.Predicted.Pose.Position.z;

	ox = ss.Predicted.Pose.Orientation.x;
	oy = ss.Predicted.Pose.Orientation.y;
	oz = ss.Predicted.Pose.Orientation.z;
	ow = ss.Predicted.Pose.Orientation.w;

	return true;
}

//---------------------------
// Profile Functions
//---------------------------

//---------------------------
// Kludge! This gates profile edits via Unity. This is not a security setting,
// but is simply designed to make accidental profile edits (which would change
// values for all apps) difficult. This should eventually be replaced with a
// content provider. Import only as needed with:
//
// [DllImport ("OculusPlugin")]
// private static extern void OVR_SetProfileEditable(bool allowProfileEdit);
static bool isProfileEditable = false;
OCULUS_EXPORT bool OVR_SetProfileEditable(bool allowProfileEdit)
{
    isProfileEditable = allowProfileEdit;
}

//---------------------------
// TODO: Rename this from "Get" to "Load"
OCULUS_EXPORT bool OVR_GetPlayerEyeHeight(float &eyeHeight)
{
    eyeHeight = LoadProfile().EyeHeight;
	return true;
}

//---------------------------
OCULUS_EXPORT bool OVR_SavePlayerEyeHeight(float eyeHeight)
{
    if (!isProfileEditable)
    {
        LOG("Rejecting OVR_SavePlayerEyeHeight - call OVR_SetProfileEditable(true) first.");
        return false;
    }

    UserProfile profile = LoadProfile();
    profile.EyeHeight = eyeHeight;
    SaveProfile(profile);
    return true;
}

//---------------------------
// TODO: Rename this from "Get" to "Load"
OCULUS_EXPORT bool OVR_GetInterpupillaryDistance(float &interpupillaryDistance)
{
    interpupillaryDistance = LoadProfile().Ipd;
	return true;
}

//---------------------------
OCULUS_EXPORT bool OVR_SaveInterpupillaryDistance(float interpupillaryDistance)
{
    if (!isProfileEditable)
    {
        LOG("Rejecting OVR_SaveInterpupillaryDistance - call OVR_SetProfileEditable(true) first.");
        return false;
    }

    UserProfile profile = LoadProfile();
    profile.Ipd = interpupillaryDistance;
    SaveProfile(profile);
    return true;
}

//---------------------------
// TODO: Rename this from "Get" to "Load"
OCULUS_EXPORT bool OVR_GetPlayerHeadModel(float &neckToEyeDepth, float &neckToEyeHeight)
{
    UserProfile profile = LoadProfile();
    neckToEyeDepth = profile.HeadModelDepth;
    neckToEyeHeight = profile.HeadModelHeight;
    return true;
}

//---------------------------
OCULUS_EXPORT bool OVR_SavePlayerHeadModel(float neckToEyeDepth, float neckToEyeHeight)
{
    if (!isProfileEditable)
    {
        LOG("Rejecting OVR_SavePlayerHeadModel - call OVR_SetProfileEditable(true) first.");
        return false;
    }

    UserProfile profile = LoadProfile();
    profile.HeadModelDepth = neckToEyeDepth;
    profile.HeadModelHeight = neckToEyeHeight;
    SaveProfile(profile);
    return true;
}

// OVR_GetNextPendingEvent
//
// Allows Unity to query for VrAPI Events.
// NOTE: the return type is an eSystemActivitiesEventStatus and NOT just a boolean! This function may return < 0 if 
// an error occured or if invalid parameters were passed.
OCULUS_EXPORT int OVR_GetNextPendingEvent( char * buffer, unsigned int const bufferSize ) {
	return ovr_GetNextPendingEvent( buffer, bufferSize );
}

}	// extern "C"

