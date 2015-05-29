# Release checkist

+ Enable Crittercism
+ Zip and upload obj directory to Crittercism
+ Remove personal .sig
+ Run the release checker (ignore LAUNCHER msg)

# TODO for next build

+ One present-per-frame is killing VNC cursor responsiveness.  Fix it!
+ Auto-push active app closer.
+ Menu text measurement auto-dimensionality.
+ Fix headmouse.
+ Intro text widget (IDs for slots, ability to spawn into slots)
+ Hide cursor until first update received
+ Render "Connecting..." into VNC widget while connecting.
+ Unregistering texture/geometry doesn't actually destroy the GL object.
+ Widgets can resize their cell to their own size temporarily, until unregistered.

# Misc

+ Moving plugins to .so files will speed up my turnaround time thanks to a smaller .APK.

+ Consider serializing all API calls through something like the InQueue, to ensure correct ordering in the presence of performance throttling.

+ Hook into the Oculus console system to allow sending console commands w/o VNC.

+ Write a console module with channels + levels (like log, warn, error) + fatal errors.  Broadcast "console" messages so a "console" plugin can receive and display them in a cell, and implement that using JS+Skia.  (This plugin could also support typed keyboard commands, or that could be built into the Shell spotlight style...my instinct is the latter)

+ Broadcast profiler data via a "profile" message, write a JS+Skia profiler display widget.

# Core

+ Implement 3D transition animation support so things can animate smoothly.

+ Make a chain of event handlers like in the DOM, such that something can be added to intercept "key" events to make a watchdog script that resets things but never has to unload.

+ Move VNC to a .so plugin as an example so others can start to actually write plugins, and so C++ plugins can be fetched via HTTP.

+ The browser model for Shellspace over HTTP would be to execute a "shellspace.org/somespace/index.cfg" script file to populate everything.

# Menu

+ Finish implementing the stacking.

+ Test a radial arrangement.

+ Support simple float/int slider, checkbox, enum controls such that data is communicated to/from the widget.

+ Support commands with editable completion, e.g. "vnc open ..." where the ... is editable with an onscreen keyboard.

# Shell

+ Automatically zoom in on the active widget.
 
+ A Ctrl-Alt-Delete-like hotkey that exects 'reset.cfg' to recover from scripting errors that break the shell.

# VNC

+ When the connection is closed, we stop accepting messages and the queue fills up with Gaze events.  Probably need to make the connection status separate from the thread status and spawn widget-per-thread, like the VLC widget.

+ Did toasts disappear somehow?

+ Also the "already connected to %s" thing prints NULL if the connection is closed after being initiated, likely because "thread" is not cleared.

+ SRGB encode color values?  Not sure what space they come from the server in.

+ client->appData.qualityLevel - runtime setting?

+ When app suspends, detect this and stop sending updates, but accumulate a texture dirty rectangle.  When unsuspended, submit the entire dirty rectangle.

+ Use a Skia filter to darken and draw "disconnected" when the connection is lost.  Actually, use Skia in general to draw connection status.

# VLC

+ Thread about efficient display formats using OpenGL.
https://forum.videolan.org/viewtopic.php?t=97672

+ Use set_format_callback and accept the preferred format if possible, instead of RV32.  Use of a YUV shader would be ideal.

+ Documentation
https://www.videolan.org/developers/vlc/doc/doxygen/html/group__libvlc__media__player.html#ga46f687a8bc8a7aad325b58cb8fb37af0

vlc create vlc0
vlc vlc0 open /storage/extSdCard/Oculus/Shellspace/test.mp4

# RDP

+ Use freerdp.org / libfreerdp.

# Menu

+ Animate "square" entity from the launching widget to the menu.
+ Give the menu a title.
+ Auto detect the required text width.
+ Show the command to be executed as a subtitle.

# Performance

+ Textures should start initially cleared, NOT uninitialized.  Same goes for geometry.  Currently if a texture is created and never updated, it will hold garbage memory.
 
+ Investigate https://vec.io/posts/faster-alternatives-to-glreadpixels-and-glteximage2d-in-opengl-es PBOs for texture updates.  Not clear whether this is better than triple buffered texture objects, but maybe?

+ VNC stuff:
+  Tuning system for InQueue variables, possibly via console commands.
+  Bounds accumulation for Geometry objects, frustum culling
+  Optimize various VNCdecoders using NEON.
+  Accelerated CopyRect?
+  16bit pixel support (runtime option)
+  Server-side scaling support?  Check to see if it actually works.
+  More profiling inside libvncserver (turbojpeg, rectangle copy, etc).

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

# Thread safety

+ Entity_Draw needs to be protected against threads modifying entities & resources while drawing takes place.  
 
It could hold the API mutex, though this starts to act more like a global mutex and there could start to be bottlenecks there.

# V8 Build notes

make i18nsupport=off android_arm.release

remove -lrt from ./tools/gyp/mksnapshot.host.android_arm.release.mk

+ Expose API via a global "sx" object instead of global namespace functions.

+ Wrap handles in JavaScript classes (via shellspace.js) with methods, such that ```foo = new Entity( 'foo' )``` translates to ```sx.registerEntity( 'foo' )``` and ```foo.orient( ... )``` translates to ```sx.orientEntity( 'foo' )```.

# Misc

+ Allow binding swipe axes to different features, with keyboard control.
  For example swipe in could be "get closer to the image, in the direction I'm looking"

+ What is Comfort Mode?

# Editor

Try atom.io; it uses V8 internally so might be more easy to integrate?  OTOH it actually *uses* node.js, wow.  Should I do that too?  Looks pretty crazy, the whole thing is written in "coffee".

# Terminal

https://github.com/jackpal/Android-Terminal-Emulator/blob/master/libtermexec/src/main/jni/process.cpp

http://www.freedesktop.org/wiki/Software/kmscon/libtsm/
http://cgit.freedesktop.org/~dvdhrm/wlterm/tree/src

https://bitbucket.org/caveadventure/tiletel/overview

# Browser

http://www.netsurf-browser.org/about/

# Keyboard

+ Bind a "console" hotkey with an editor for arbitrary console commands.
+ Keyboard string input.
+ All keyboard extended characters.
+ Keyboard modifier keys (toggle down, hold down until next non-modifier key press).

# Image quality improvements

+ Temporal jitter + reprojection with point filtering

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

# IPC and Network plugins

For network and IPC comms, libuv (used by Node.JS) seems like a good cross platform way to go, just run it on a thread and let it listen and interact with the API.

http://nikhilm.github.io/uvbook/filesystem.html

# Canvas API

Seems like the canvas commands are becoming more and more core to doing anything useful at all.  If they become central to the API, I should consider merging aspects of Skia's API into the core API.  One way to do it would be to automatically create a SkCanvas and SkBitmap for every STexture, and expose a decent subset of the canvas APIs as Texture methods.  That would allow quite casually registering and drawing to textures, which would be natural for many of the apps I have in mind.

# TEST

+ Radial menu?  Or a tree?
+ Rounded menu rectangles
+ Blue active highlight

# Something like PackageControl?

# Plugin entity registration

To keep things tidy, it might be nice to automatically destroy things that plugins create when the plugin is unloaded.  There are some exceptions to this, but in general it seems reasonable.

To implement it I would add the plugin ID to every register / unregister function.  V8 would do this implictly by registering itself as a plugin when spawning the thread (registerPlugin would no longer be exposed) and implicitly adding the argument.  Would have to come up with something clever for the plugin name like the name of the file without path (example.js).

Possibly need to sanitize the name and/or add some more characters to the allowable set for registry entries.

# App ideas

Post-It
Twitter
Profiler
Math Plot

# Recordable settings

http://www.reddit.com/r/oculus/comments/2p812e/gear_vr_recording_at_solid_720p60_using/

- Note had to set 720p and 30hz manually.

# NDK build patch

define cmd-build-shared-library
$(PRIVATE_CXX) \
    -Wl,-soname,$(notdir $(LOCAL_BUILT_MODULE)) \
    -shared \
    --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
    -Wl,--start-group \  <----
    $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
    -Wl,--end-group \    <----
    $(PRIVATE_LDFLAGS) \
    $(PRIVATE_LDLIBS) \
    -o $(call host-path,$(LOCAL_BUILT_MODULE))
endef

# About embedding native modules into JNI apps:

http://stackoverflow.com/questions/15719149/how-to-integrate-native-runtime-library-with-dlopen-on-ndk

    The best strategy on Android is to load all native libraries from Java, even if you choose which libraries to load, by some runtime rules. You can use dlsym(0, ...) then to access exported functions there.

    APK builder will pick up all .so files it finds in libs/armeabi-v7a and the installer will unpack them into /data/data/<your.package>/lib directory.

# Plugin compiling notes

Practically speaking the .so files would like to use various systems from the Shellspace core, like common types, the message queue, vector math library, etc.

Also in many instances they will want to use shared libraries like Skia.

One option is to migrate the shellspace core into a library libshellspacecore, similar to libvlccore.  Then the plugins and the app can both use the code provided in the core library, in fact they will get their own copy of it.  (So care must be taken that isolation is assumed between the core and the plugin w.r.t. globals)

It's not clear to me that there is a string reason for libshellspacecore to actually be a library file, as opposed to a set of source files that may be arbitrarily included into the plugins.  The source file method gives the opportunity to inject defines into the shared source files, e.g. lists of profilers or threads.

Directory structure:

/core/*.cpp
/shellspace/*.cpp
/plugins/vnc/*.cpp
/plugins/vlc/*.cpp
/plugins/v8/*.cpp
/gearvr/AndroidManifest.xml
/gearvr/jni/Android.mk
/external/v8/armeabi-v7a/...
/external/vlc/armeabi-v7a/...


