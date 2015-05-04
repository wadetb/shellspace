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

# TODO

# Shell

+ Add a gutter between cells (5%?) to fix rapid oscillation.
+ Fix edges not linking up exactly.
+ Make text readable.
+ Custom wraparound geometry for 2D.
+ Context specific menus.

The workflow is that the user gazes at an empty cell, taps the touchpad, and a menu appears with various choices like "add/del row, add/del column, spawn vnc widget".  They select "spawn vnc widget" and a command is executed.  (Does the shell pick the widget ID?)  After the widget initializes and creates its first entity, it executes "shell register <wid> <eid>" and that causes the entity to be parented, and the entity and widget to be remembered in the cell.

In the sense of a single cell, it occupies an arc of the globe in latitude and longitude.  Adjacent arcs don't need to be contiguous, in fact raycasting against entity geometry is likely a valuable feature to allow aiming at things in further cell layers, as is the notion of having some padding between cells.

When 2D apps are in focus, like virtual desktops, there is also a desire for apps to span multiple cells and "wrap around" the user.  If the app is just presenting a 2D quad, this wrap around is not possible without dividing the geometry.  Thus we need both the notion of widgets occupying a range of cells (likely leaving filler cells for bookkeeping) but we also want to create custom geometry for 2D applications that implements the curvature of the cell.

Lets think of the cell unwrapped, as a 2D coordinate system of U and V.  The grid is divided into W steps of U and V steps of H, perhaps with some empty padding around the cells within the steps.  U=[0,1] represents a 360 degree horizontal arc with U=0.5 at V=(0,0,-1).  V=[0,1] represents a roughly 120 degree vertical arc.

To position and size an element (like the frame) we assume the element bounds itself naturally in a -1..1 space in X,Y,Z.

(TBD - Are we going to start keeping CPU copies of vertex data for Bounds building?  Seems like a good idea...)

For 3D elements we prefer to scale them uniformly, but for 2D elements we actually want to create custom curved geometry.

(TBD - Runtime grid resizing in X and Y sounds like a useful thing... let's get these basics done so we can move on to the fun stuff!)

Starting with 3D... we take a UV=[a,b] range at constant depth D.  Project both a and b onto the sphere, measure, and take that as the scale factor.  Put the origin at depth.  Rotation is a "look at" matrix aimed at the origin.  (Can we cut use of AnglesToAxis?)

# VNC

+ client->appData.qualityLevel - runtime setting?

+ When app suspends, the texture input queue fills up and this (currently, thanks to Present semantics) deadlocks the app.  Detect this and stop sending updates, but accumulate a texture dirty rectangle.  When unsuspended, submit the entire dirty rectangle.

# Performance

+ Bounds accumulation for Geometry objects, frustum culling
+ Subrectangle texture update throttling
+ Optimize various decoders using NEON.
+ Accelerated CopyRect?
+ 16bit pixel support (runtime option)
+ Server-side scaling support?  Check to see if it actually works.
+ More profiling inside libvncserver (turbojpeg, rectangle copy, etc).

# Files

Idea for file widget- drag a file into a cell, it's represented by a filename and MIME icon.  Context menu provides "open with" options.

# DLLs

/Shellspace/GearVR/Plugins/Shell
    /jni
        Android.mk
        shell.cpp

# JavaScript

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
