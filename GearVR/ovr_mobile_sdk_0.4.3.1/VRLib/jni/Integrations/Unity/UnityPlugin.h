/************************************************************************************

Filename    :   UnityPlugin.h
Content     :   
Created     :   July 14, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

// This is set by JNI_OnLoad() when the .so is initially loaded.
// Must use to attach each thread that will use JNI:
namespace OVR
{
	// PLATFORMACTIVITY_REMOVAL: Temp workaround for PlatformActivity being
	// stripped from UnityPlugin. Alternate is to use LOCAL_WHOLE_STATIC_LIBRARIES
	// but that increases the size of the plugin by ~1MiB
	extern int linkerPlatformActivity;
}
