# Schedule

4/21 - Today's date

4/27 - First screenshot / video milestone
        Release to beta testers when possible.
        Add code license.  GPL
        Upload to GitHub.
        VNC client is solid - no crashes or major experience issues.
        Bluetooth keyboard & mouse support.
        Prettify the menu for the screenshot.
        Get a video of me using it to code?
        
5/4 - First video of app running
        This is all about the shell.
        Multiple widgets; the "shell" part.
        Shellspace geometry + texture API.
        Toy apps.

5/11 - Final build
        Polish
        Battery life & Framerate optimization

# Release checkist

+ Enable Crittercism
+ Zip and upload obj directory to Crittercism

# TODO

# For this release:

# Shell

+ Context specific menus.

Idea: When back button is pressed, shell sends a "menu" message to the active widget.  The widget can respond by broadcasting a "menu xyz" message to the system, which will open and display xyz.vrkey.  The issue is widget-specific things like "vnc disconnect vnc0" where VNC0 is the id of the activating widget.  

# VNC

+ SRGB encode color values

+ Separate tweaks for xCurve and yCurve.

+ client->appData.qualityLevel - runtime setting?

+ When app suspends, the texture input queue fills up and this (currently, thanks to Present semantics) deadlocks the app.  Detect this and stop sending updates, but accumulate a texture dirty rectangle.  When unsuspended, submit the entire dirty rectangle.

+ SRGB transform on the VNC thread.

# Performance

+ Tuning system for InQueue variables, possibly via console commands.
+ Bounds accumulation for Geometry objects, frustum culling
+ Optimize various decoders using NEON.
+ Accelerated CopyRect?
+ 16bit pixel support (runtime option)
+ Server-side scaling support?  Check to see if it actually works.
+ More profiling inside libvncserver (turbojpeg, rectangle copy, etc).

+ Add an "inqueue freeze" command to test how updates affect display perf.
  (See whether reintroducing throttling is important)

+ Memory tracking?  (CPU precise and GPU estimate)
  With memory tracking we could force certain operations (InQueue append) to stall until memory is available.  Would be hard to budget for the case of many VNC sessions though.

+ Test performance of double (vs triple) buffer.
  (This may require stddev profiling info over a long period of time, maybe just do a strict CSV dump mode, could even use a file)

# Misc

+ Fix the autoexec bug - issue is that command.cpp tries to execute the command before the thread creates the widget, so the widget isn't found.
  Could have a "wait vnc0" command that just cycles command.cpp though frames until vnc0 is created?  Or a way to queue messages for not-yet-created widgets/plugins?

# Files

Idea for file widget- drag a file into a cell, it's represented by a filename and MIME icon.  Context menu provides "open with" options.

# DLLs

/Shellspace/GearVR/Plugins/Shell
    /jni
        Android.mk
        shell.cpp

# JavaScript

make i18nsupport=off android_arm.release

remove -lrt from ./tools/gyp/mksnapshot.host.android_arm.release.mk


+ Install d8 (command line v8) on a Mac:
https://gist.github.com/kevincennis/0cd2138c78a07412ef21

+ About embedding native modules into JNI apps:

http://stackoverflow.com/questions/15719149/how-to-integrate-native-runtime-library-with-dlopen-on-ndk

    The best strategy on Android is to load all native libraries from Java, even if you choose which libraries to load, by some runtime rules. You can use dlsym(0, ...) then to access exported functions there.

    APK builder will pick up all .so files it finds in libs/armeabi-v7a and the installer will unpack them into /data/data/<your.package>/lib directory.

+ Guide to integrating V8, apparently this works on Android:

https://developers.google.com/v8/get_started

+ Allow binding swipe axes to different features, with keyboard control.
  For example swipe in could be "get closer to the image, in the direction I'm looking"

+ Multiple sessions (window manager)
+ What is Comfort Mode?

# SVG drawing

Skia (By Google) - Seems like the winner?
https://skia.org/user/sample/hello

https://sites.google.com/site/skiadocs/user-documentation/quick-start-guides/android

http://stackoverflow.com/questions/6342258/using-skia-in-android-ndk
https://code.google.com/p/skia/wiki/SkCanvas

# Terminal

https://github.com/jackpal/Android-Terminal-Emulator/blob/master/libtermexec/src/main/jni/process.cpp

# Keyboard

+ Bind a "console" hotkey with an editor for arbitrary console commands.
+ Keyboard string input.
+ All keyboard extended characters.
+ Keyboard modifier keys (toggle down, hold down until next non-modifier key press).

# Image quality improvements

+ Temporal jitter + reprojection with point filtering
+ Render direct to screen with homemade distortion (loses timewarp unless I reimplement it)

# Lazy free

A lazy free system could be implemented such that if inactive textures aren't used for N frames they are discarded, and created again on demand.  When a new texture is created the current texture is copied into it (assuming there is a GL API for this like CopySubResource) and the bits in the update queue are made equal between them.  This would cut the memory usage for inactive windows greatly but with some GPU cost on the first frame they are updated again.

# Note about memory usage

1440*900 -> 2048x1024 = 8MB x3 = 24MB texture data

GPU specs seem to indicate it uses shared GDDR3, and there is 3GB in the phone.  10x triple buffered VNC widgets + framebuffer copy (owned by the thread and not padded) would be ~320MB, significant but not worth worrying about right now.  

# Links

https://danielrapp.github.io/doppler/
https://github.com/rygorous/mini_arith/blob/master/main.cpp

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

# Throttling idea

Consider throttling on the VNC thread by not allowing it to contain more than N pixels (or MB?), instead of throttling on the render by not executing the entire queue.

This would be a form of cooperative throttling that might encourage the server to gather more updates together.

# App ideas

Post-It
Twitter
Profiler
Plot

# Recordable settings

http://www.reddit.com/r/oculus/comments/2p812e/gear_vr_recording_at_solid_720p60_using/

- Note had to set 720p and 30hz manually.

# Book readers

http://vrjam.challengepost.com/submissions/36769-vr-library
Basic book reader tied to Amazon?  Has prototype screenshot.  Asked about AA.

http://vrjam.challengepost.com/submissions/36786-vr-reader
Another book reader.  Has prototype screenshot.  Asked about AA.

http://vrjam.challengepost.com/submissions/36434-virtual-book-reader
Just a drawing but implies some image zooming behavior.

http://vrjam.challengepost.com/submissions/36887-virtuareader
Speed reader!  One word at a time.

# Similar projects

http://vrjam.challengepost.com/submissions/36938-process
Vaguely related as it uses a terminal and typing.

http://vrjam.challengepost.com/submissions/36305-panotrader
Stock market trader.

http://vrjam.challengepost.com/submissions/36148-learn-immersive
Language learner with some on screen instructions

http://vrjam.challengepost.com/submissions/36886-meetingright-vr
Web conferencing.  Very little information even on the main website.

http://vrjam.challengepost.com/submissions/36959-airvr
Render realtime on a PC and stream to the GearVR!  Posted some questions about video streaming.

http://vrjam.challengepost.com/submissions/36295-cnn
Wow, CNN doing VR news.

http://vrjam.challengepost.com/submissions/36890-social-symphony
Facebook app.  Not much in the way of specifics.

# Web browsers

http://vrjam.challengepost.com/submissions/36902-browser-bubble - Commented

# Video players / streaming video

http://vrjam.challengepost.com/submissions/36500-fdplayer - Commented

# School

http://vrjam.challengepost.com/submissions/36371-immerse-your-brain

# Other interesting projects

http://vrjam.challengepost.com/submissions/37014-rex - Real estate

http://vrjam.challengepost.com/submissions/36844-chess-vr - Really?

# Builds

+ Test1 -
  https://s3.amazonaws.com/vrjam-submissions/signed/2ad5dd20bc9b/rnpUzvDdRo6w6pkmAUqf_Shellspace-test1.apk
