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
+ Remove personal .sig
+ Run the release checker (ignore LAUNCHER msg)

# TODO for next build

+ Load a connection list from a JSON file.
+ Make toy apps load where aimed at.
+ Stability testing.
+ Push active app closer.

# Misc

+ Hook into the Oculus console system to allow sending console commands w/o VNC.

+ Write a console module with channels + levels (like log, warn, error) + fatal errors.  Broadcast "console" messages so a "console" plugin can receive and display them in a cell, and implement that using JS+Skia.  (This plugin could also support typed keyboard commands, or that could be built into the Shell spotlight style...my instinct is the latter)

+ Broadcast profiler data via a "profile" message, write a JS+Skia profiler display widget.

# Core 

+ Implement 3D transition animation support so things can animate smoothly.

+ Make a chain of event handlers like in the DOM, such that something can be added to intercept "key" events to make a watchdog script that resets things but never has to unload.

+ Move VNC to a .so plugin as an example so others can start to actually write plugins, and so C++ plugins can be fetched via HTTP.

+ Add Connection: close to the HTTP request.  (Or just implement multiple requests per connection, haha)

The browser model for Shellspace over HTTP would be to execute a "shellspace.org/somespace/index.cfg" script file to populate everything.

# Menu

+ Finish implementing the stacking.

+ Test a radial arrangement.

+ Support simple float/int slider, checkbox, enum controls such that data is communicated to/from the widget.

# Shell

+ Automatically zoom in on the active widget.
 
+ A Ctrl-Alt-Delete-like hotkey that exects 'reset.cfg' to recover from scripting errors that break the shell.

+ Context specific menus.

Idea: When back button is pressed, shell sends a "menu" message to the active widget.  The widget can respond by broadcasting a "menu xyz" message to the system, which will open and display xyz.vrkey.  (.vrkey is JSON, but could conceivably be injected JavaScript code via include( menuFile ).

Menu may contain widget-specific things like "vnc disconnect $activeId" where $activeId is the id of the activating widget.  

# VNC

+ When the connection is closed, we stop accepting messages and the queue fills up with Gaze events.  Probably need to make the connection status separate from the thread status and spawn widget-per-thread.

+ Did toasts disappear somehow?

+ Also the "already connected to %s" thing prints NULL if the connection is closed after being initiated, likely because "thread" is not cleared.

+ SRGB encode color values?  Not sure what space they come from the server in.

+ client->appData.qualityLevel - runtime setting?

+ When app suspends, detect this and stop sending updates, but accumulate a texture dirty rectangle.  When unsuspended, submit the entire dirty rectangle.

+ Use a Skia filter to darken and draw "disconnected" when the connection is lost.

# VLC

+ Thread about efficient display formats using OpenGL.
https://forum.videolan.org/viewtopic.php?t=97672

+ Use set_format_callback and accept the preferred format if possible.

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

# Thread safety

+ Entity_Draw needs to be protected against threads modifying entities & resources while drawing takes place.  
 
It could hold the API mutex, though this starts to act more like a global mutex and there could start to be bottlenecks there.

# JavaScript

make i18nsupport=off android_arm.release

remove -lrt from ./tools/gyp/mksnapshot.host.android_arm.release.mk

+ About embedding native modules into JNI apps:

http://stackoverflow.com/questions/15719149/how-to-integrate-native-runtime-library-with-dlopen-on-ndk

    The best strategy on Android is to load all native libraries from Java, even if you choose which libraries to load, by some runtime rules. You can use dlsym(0, ...) then to access exported functions there.

    APK builder will pick up all .so files it finds in libs/armeabi-v7a and the installer will unpack them into /data/data/<your.package>/lib directory.

+ Expose API via a global "sx" object instead of global namespace functions.

+ Wrap handles in JavaScript classes (via shellspace.js) with methods, such that ```foo = new Entity( 'foo' )``` translates to ```sx.registerEntity( 'foo' )``` and ```foo.orient( ... )``` translates to ```sx.orientEntity( 'foo' )```.

# Misc

+ Allow binding swipe axes to different features, with keyboard control.
  For example swipe in could be "get closer to the image, in the direction I'm looking"

+ Multiple sessions (window manager)
+ What is Comfort Mode?

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

# IPC and Network plugins

For network and IPC comms, libuv (used by Node.JS) seems like a good cross platform way to go, just run it on a thread and let it listen and interact with the API.

http://nikhilm.github.io/uvbook/filesystem.html

# Message queue design

This ties into plugins vs widgets and whether plugins are an explicit concept or something that's transparently managed by the core.

Right now there are two kinds of message loops; widget and plugin.  This is fine for a threaded plugin that creates a new message loop thread for each widget.  (And note, we could adopt this model fully by exposing the ability to spawn() or the like in javascript, though thread safety issues there get dicey)

Most things now adopt the plugin centric model, where they just watch for plugin messages and the plugin is essentially a widget singleton.

But if there were a plugin that was intended to spawn many widgets, there is currently no way to poll on an array of queues (and the syntax for that might become quite cumbersome if there were 100s?). 

One option would be to eliminate widget queues and just direct widget messages to the plugin.  This seems ideal for JavaScript, but for VNC and other network connection-based widgets where there is a thread-per-widget it adds extra synchronization that could affect the performance of the core plugin thread.

Need to keep thinking about it in the context of plugins vs widgets.  

# Menu

Thinking menus will be represented by JSON objects, a bit like Sublime Text menu files.

    [
        {
            "caption": "launch",
            "id": "launch", 
            "children": [
                {
                    "caption": "vnc 10.0.1.4",
                    "command": "vnc create vnc_$active_cell; vnc connect vnc_$active_cell 10.0.1.4 asdf"
                }
            ]
        }
    ]

The menu system will replace certain variables in the command string with special values like the ID of the active cell.

Plugins can "merge" new menus into the menu.  The active widget and the shell can also merge.  Events will be broadcast whenever the menu is opened to give things a chance to populate it.

So, user presses back button and OvrApp broadcasts a "menu open" message.  The menu data structure is initially cleared.  

The menu sends the shell a "shell menu opened" message.  This causes the shell to stop interpreting gaze data until it receives a "shell menu closed" message.

The shell checks the active cell.  

If there is no cell, the shell puts nothing in the menu.  (Eventually there might be global settings like background color that only come up when there is no active cell, that would be up to the shell.)

If it's an empty cell, the shell sends a "launch" menu and a "divide" menu (to make a grid).

If it's a widget, the "menu" message just gets forwarded to the widget, possibly with some initial data to merge in.

Ultimately someone (the shell and/or the widget) sends a "menu contents" message with the JSON for the final menu.  All the "menu contents" messages are merged together.  

Then the menu creates entities for the top level, and starts interpreting gaze data.

When a touch is received, if it hits a menu entity, the corresponding command is posted and the menu closes.

# Canvas API

Seems like the canvas commands are becoming more and more core to doing anything useful at all.  If they become central to the API, I should consider merging aspects of Skia's API into the core API.  One way to do it would be to automatically create a SkCanvas and SkBitmap for every STexture, and expose a decent subset of the canvas APIs as Texture methods.  That would allow quite casually registering and drawing to textures, which would be natural for many of the apps I have in mind.

# TEST

+ Radial menu?  Or a tree?
+ Rounded menu rectangles
+ Blue active highlight

# Hm. something like PackageControl?

# Plugin entity registration

To keep things tidy, it might be nice to automatically destroy things that plugins create when the plugin is unloaded.  There are some exceptions to this, but in general it seems reasonable.

To implement it I would add the plugin ID to every register / unregister function.  V8 would do this implictly by registering itself as a plugin when spawning the thread (registerPlugin would no longer be exposed) and implicitly adding the argument.  Would have to come up with something clever for the plugin name like the name of the file without path (example.js).

Possibly need to sanitize the name and/or add some more characters to the allowable set for registry entries.

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

# Build patch

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

