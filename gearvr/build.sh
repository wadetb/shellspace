#!/bin/bash
source ../bin/android_dev.sh
set -e

pushd $OVR_MOBILE_SDK/VRLib
./build.sh $1
popd

export BUILD_MODULE=Shellspace

echo "========================== Update Shellspace Project ==========================="
android update project -t android-19 -p . -s
 
if [ "$1" == "clean" ]; then

    echo "========================== Clean Shellspace " $1 " ==========================="
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=1 
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=0
    ant clean 

else

    if [ "$1" == "" ] || [ "$1" == "debug" ]; then
        echo "========================== Build Shellspace Native ==========================="
        $ANDROID_NDK/ndk-build NDK_DEBUG=1 OVR_DEBUG=1 
    fi

    if [ "$1" == "release" ]; then
        echo "========================== Build Shellspace " $1 " Native ==========================="
        $ANDROID_NDK/ndk-build NDK_DEBUG=0 OVR_DEBUG=0 
    fi

    echo "========================== Build Shellspace Java ==========================="

    #For the time being, just build debug releases until we have a keystore for signing
    #We do the rename to avoid confusion, since ant will name this as -debug.apk but
    #the native code is all built with release optimizations.
    ant -quiet debug

    cp bin/Shellspace-debug.apk bin/Shellspace.apk

fi

