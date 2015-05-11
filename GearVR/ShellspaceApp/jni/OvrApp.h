/************************************************************************************

Filename    :   OvrApp.h
Content     :   Trivial use of VrLib
Created     :   February 10, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVRAPP_H
#define OVRAPP_H

#include "App.h"
#include "BitmapFont.h"
#include "ModelView.h"

struct SVNCWidget;
struct SKeyboard;

class OvrApp : public OVR::VrAppInterface
{
public:
						OvrApp();
    virtual				~OvrApp();

	bool 				GetWantSrgbFramebuffer() const;
	virtual void		OneTimeInit( const char * launchIntent );
	virtual void		OneTimeShutdown();
	virtual Matrix4f 	DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 	Frame( VrFrame vrFrame );
	virtual void		Command( const char * msg );
    virtual bool 		OnKeyEvent(int, OVR::KeyState::eKeyEventType);

	OvrSceneView		Scene;
};

extern JNIEnv 		*g_jni;
extern jobject 		g_activityObject;

extern OvrApp 		*g_app;

sbool App_Command();

#endif
