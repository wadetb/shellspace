# Shellspace

This is the source code repository for Shellspace, my entry into the Oculus VR Jam 2015.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=_mL8QvhKvOA" target="_blank"><img src="http://img.youtube.com/vi/_mL8QvhKvOA/0.jpg" alt="YouTube" width="560" height="315" border="10" /></a>

http://challengepost.com/software/shellspace

It's a prototypical VR desktop environment, starting with a VNC client and growing from there.

# Building

After getting the source code, add a section for your computer to `GearVR/bin/android_dev.sh`, setting various environment variables to the paths where your Android SDK parts are installed.

Run the script using `source bin/android_dev.sh`.

## Android NDK Makefile fix

Due to a library dependency issue, building Shellspace currently requires the following patch to `$ANDROID_NDK_ROOT/build/core/default-build-commands.mk`:

```patch
  define cmd-build-shared-library
  $(PRIVATE_CXX) \
      -Wl,-soname,$(notdir $(LOCAL_BUILT_MODULE)) \
      -shared \
      --sysroot=$(call host-path,$(PRIVATE_SYSROOT_LINK)) \
+     -Wl,--start-group \
      $(PRIVATE_LINKER_OBJECTS_AND_LIBRARIES) \
+     -Wl,--end-group \
      $(PRIVATE_LDFLAGS) \
      $(PRIVATE_LDLIBS) \
      -o $(call host-path,$(LOCAL_BUILT_MODULE))
  endef
```

This patch forces the GNU ld linker to resolve dependencies between linked libraries regardless of the order on the command line.

Some users have also reported that in order to build the Java parts of the application framework, the JDK must be installed and the `JAVA_HOME` environment variable must be set correctly.

Once your build environment is set up, change to the `gearvr` directory and type `./build.sh`.  Please open an issue on this GitHub project, or email me if you have issues.

# Usage

To run, connect your phone via USB, change directory to `gearvr` and enter these commands:

```
./build.sh
./install.sh
./run.sh
```

## Connecting to different servers

The stock Shellspace start menu includes connections to a few public VMs running on my webserver, these are intended to allow people to try out Shellspace before setting up their own server.

To create your own connections, copy `start.menu` from the `assets` folder to a `sdcard` folder and customize it with your own servers.  You may use the `reload.sh` script to deploy it to the phone, or else copy it using Android File Transfer or `adb push`.

## Live updates over Wifi

It is possible to deploy menu updates without removing the phone from the Gear VR or restarting Shellspace.

After setting up your adb connection, with the phone plugged in to USB, enter the following commends:

```
adb tcpip 5555
adb connect <phone IP address>
```

You may now disconnect the phone from USB and enter this command:

```
adb devices
```

If you see your phone's IP address listed, you are good to go.  Insert the phone into the Gear VR and use `adb push` to send files over.  

If you do not see your phone's IP address, retry the `adb connect` conmmand once or twice and it should succeed, otherwise troubleshoot your network connection.

The start menu reloads the `start.menu` file each time it is opened; there is no need to restart Shellspace between updates.

Wifi connections are convenient for iterating on menus and JavaScript code but are pretty slow for doing C++ development thanks to the transfer speed.  This situation will improve when the C++ plugins are split into individual .so files.

