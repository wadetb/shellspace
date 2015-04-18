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

# Texture buffering

Need to create & update new texture instances while the renderer runs unaffected.

(This will be useful for async geometry and texture updates for the full API, too)

When a new update comes through, it's immediately applied to the oldest texture (along with any other pending updates for that texture), and that texture becomes current and is rendered (with a frame delay?).  The update is marked pending for all other textures.  

If a texture hasn't been used in awhile, it gets destroyed.  If an update comes when the texture is not created, it's created.

Resizes are special cases of updates that always destroy/recreate the texture.

The cursor handling should be adjusted to run entirely on the VNC thread, just sending extra update requests when the cursor moves... this is appropriate as an example of the eventual API boundary for texture updates.  (Later the cursor could become geometry and be sent through origin updates)

It's really more like texture slots than textures that are managed.  There is a fixed # of slots and they are dynamically managed through the update queue.

# Texture buffering continued

To make the VNC thread handle the cursor, need to give it the ability to save/restore from behind the cursor.  If we use the shared system memory framework, an update event will silently trash whatever it covers before calling our callback, forcing us to redraw the cursor.  But before drawing the cursor, it will need to back up what comes behind it.  But if the cursor is only partially trashed, the backup would end up "backing up" some cursor pixels.  

If I modified the VNC library to call a "pre-update" callback before applying any update, I could check if the rectangle overlaps the cursor, and erase the cursor there, set a pending flag, and then backup & draw the cursor again after the update.

# Short term solution

While moving cursor handling into the VNC thread is ultimately an improvement, a better solution would be to use a forthcoming geometry update queue to just draw the cursor on its own polygon, but that is a long ways off.

In the short term, perhaps I can simply double or triple buffer the texId and queue all messages until they've been applied to all textures.

To do that each event will get a byte mask with one bit per texId in the array.  If the queue is not empty, the next texture will be chosen and the entire queue will be run for it.  (I'll add a NOP queue item to delete non-update items from the queue)  As the queue is run, the update events will mark themselves as having executed for the current texId, and when the mask has all bits set, the event will be removed from the queue.

Frame 0 - 
Receive 3 updates
Choose tex0
Apply 3 updates to tex0
Draw tex0

Frame 1 -
Receive 1 update
Choose tex1
Apply 4 updates to tex1

Frame 2 -
Receive no update
Choose tex2
Apply 4 updates to tex2

Frame 3 -
Receive no update
Choose tex0
Apply 1 update to tex0

This system biases towards keeping all the textures up to date, even if it means cycling textures despite no new data becoming available.  I could alter it to only choose a new texture if one or more new update events are received, e.g. one or more update events that do not have the bit for the current texture.  This would leave items in the queue longer, but would let the  texture updates be deferred.

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
