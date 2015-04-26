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

# Q&A questions

# How do I beta test my APK?

My app is developed using Android JNI tools, not a game engine.

I'd like to utilize beta testers who are not local to me.  How can I package and distribute builds in a way that is as convenient as possible for testers to install?

Specifically, I'd like to avoid requiring that they install a complete development environment, though a partial one would be acceptable.

# Why doesn't "both" mode work?

My app is developed using Android JNI tools, not a game engine.

When my AndroidManifest.xml contains the following key, it launches correctly when installed into the headset.

        <meta-data android:name="com.samsung.android.vr.application.mode" android:value="vr_only"/>

When I replace it with this key, everything seems fine from the development PC side, but the VR Home app stays open and my never launches.

        <meta-data android:name="com.samsung.android.vr.application.mode" android:value="vr_both"/>

This means I have to rebuild my APK in order to run it in the GearVR.  Am I missing some step?  Thanks!

# TODO for next milestone

+ Print connection status on VR window - use Toast API?
+ Restart connection automatically when lost 

/ Passthrough Android keyboard events.  (Need to do letters)

+ Keyboard rectangle intersect (draw background rect?).
+ Bluetooth mouse

+ BT console?

# TODO

# Registry

Widgets, Plugins, et al stored in an array.  
Hash table points to array indices.

# Texture queue

Do I want a single queue for all textures, or a separate queue per texture?  
(Or one in side the other?)

Do I want strict FIFO or do I want to prioritize by what the user is looking at?

Effectively I want a single large priority queue that is FIFO by default but adjusted for factors like widget priority (active widget gets higher priority) and per-widget / per-plugin bytes per second quotas.

For now I can just have a single global queue that advances like the VNC texture queue.  The meaning of the per element mask will be defined differently for different types, but I'm guessing initially there will be 3 of everything.

# DLLs

/Shellspace/GearVR/Plugins/Shell
    /jni
        Android.mk
        shell.cpp

What is the plan for DLLs?  Do I start by writing a "V8" plugin that exposes the API to JavaScript?  Would that be a "widget" plugin?  Not really, it's a "host" plugin and the individual JavaScript scripts that are downloaded and executed are the widget plugins.

Or do I build V8 directly into the runtime?  I'd rather not just for stability and extensibility, though it would surely be part of the "core" module set.

In practice I'd like to be able to say "v8 load http://shellspace.org/hula.js" and the V8 plugin would download the JavaScript file, create a runtime environment, and "run" that plugin in the runtime environment.  

So v8 needs to register that it supports the command "v8", and hula.js needs to register that it supports the command "hula" and also needs to execute "shell hula" or something in order to promote itself to be the shell.

(Note I haven't really defined what "running" a plugin means from a thread model standpoint- does hula.js get a CPU thread of its own, with IPC API calls via queues?)

(Ideally I'd like to go with the pluginhost model such that if the v8 process dies, it can be respawned without restarting the shellspace core process...)

But I have to consider what's actually possible by Monday.  Can I get V8 running in-process?  Can I get it running out-of-process?  Does the SDK even exist or do I have to download and compile it?  Are there gradual steps I can take toward the final goal that are still useful?

What if I simply make the SS API in-process, build V8 in-process, load JS files from disk, and proceed with iteration?  Then I just have to figure out how to hook V8 API calls back to in-process Shellspace API calls, and that doesn't sound insurmountable.  I can skip the entire dlopen mess by leaning on the Javascript.  

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

http://stackoverflow.com/questions/19213655/java-lang-outofmemoryerror-out-of-stack-in-jni-pushlocalframe

https://github.com/anoek/android-cairo

# Speed

+ 16bit pixel support (runtime option)
+ Server-side scaling support?  Check to see if it actually works.
+ Accelerated CopyRect.
+ More profiling inside libvncserver (turbojpeg, rectangle copy, etc).

# Keyboard

+ Bind a "console" hotkey with an editor for arbitrary console commands.
+ Keyboard string input.
+ All keyboard extended characters.
+ Keyboard modifier keys (toggle down, hold down until next non-modifier key press).

# Image quality improvements

+ Reorient command
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

# Throttling idea

Consider throttling on the VNC thread by not allowing it to contain more than N pixels (or MB?), instead of throttling on the render by not executing the entire queue.

# Threads and the API

Each plugin (and in some cases each widget) will be running on a separate thread inside the core process.  To avoid corrupting data structures like the registry, API calls need to be totally serialized.

So, I think I'm just going to have a global critical section that stays alive through _all_ shellspace.h API calls.

That means that if I destroy a texture SRef, I can lock and clear any updates from the InQueue, remove that texture SRef from the registry, and know that any future updates (from ANY plugin on ANY thread) will return S_INVALID_HANDLE.  (Hm should that be SX_INVALID_HANDLE...?)

# App ideas

Post-It
Twitter
Profiler
Plot

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

# Sticky

Pulled pork platter (& sausage?)- Caesar salad & Mac n Cheese
