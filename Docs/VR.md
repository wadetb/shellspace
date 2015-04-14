# TODO

+ Wondering what the FPS dip is whenever there's a packet received.  Perhaps I'm introducing a sync point and should be double or triple buffering the input texture?  Drops to 40fps every time there is a window update.  It might also be the mipmap generation.

+ Print connection status on VR window - use Toast API?
+ Restart connection automatically when lost 
+ Multiple sessions (window manager)
+ What is Comfort Mode?

# Keyboard

+ Bind a "console" hotkey with an editor for arbitrary console commands.
/ Passthrough Android keyboard events.  (Need to do letters)
+ Keyboard rectangle intersect (draw background rect?).
+ Keyboard string input.
+ All keyboard extended characters.
+ Keyboard modifier keys (toggle down, hold down until next non-modifier key press).

# Image quality improvements

+ Reorient command
+ Temporal jitter + reprojection with point filtering
+ Render direct to screen with homemade distortion (loses timewarp unless I reimplement it)

# Links

https://danielrapp.github.io/doppler/
https://github.com/rygorous/mini_arith/blob/master/main.cpp

# Video capture

Jose wrote:
Does anyone have any recommended methods for screen recording a gear vr game? On PC, I would normally use fraps or obs, but I'm unaware of any similar software for Android or gear vr.

You can use the apps mobizen or recordable from the Play Store

# Rendering to TimeWarp overlay plane

Dedicated “viewer” apps (e­book reader, picture viewers, remote monitor view, et cetera) that really do want to focus on peak quality should consider using the TimeWarp overlay plane to avoid the resolution compromise and double­resampling of distorting a separately rendered eye view. Using an sRGB framebuffer and source texture is important to avoid “edge crawling” effects in high contrast areas when sampling very close to optimal resolution.

# Connecting a bluetooth mouse / trackball

Using the Android Motion events:

    http://developer.android.com/reference/android/view/MotionEvent.html

Have to plumb this through from the Java code:

    // VRLib/src/com/oculusvr/vrlib/VrActivity.java:
    public static native void nativeKeyEvent(long appPtr, int keyNum, boolean down, int repeatCount );

    // VRLib/jni/App.cpp
    void Java_com_oculusvr_vrlib_VrActivity_nativeKeyEvent( JNIEnv *jni, jclass clazz,
            jlong appPtr, jint key, jboolean down, jint repeatCount )
    {
        AppLocal * local = ((AppLocal *)appPtr);
        // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
        if ( local->OneTimeInitCalled )
        {
            local->GetMessageQueue().PostPrintf( "key %i %i %i", key, down, repeatCount );
        }
    }

    if ( MatchesHead( "key ", msg ) )
    {
        int key, down, repeatCount;
        sscanf( msg, "key %i %i %i", &key, &down, &repeatCount );
        KeyEvent( key, down, repeatCount );
        return; 
    }
