#!/bin/bash

source ../bin/dev.sh

#pushd ../ovr_mobile_sdk_0.4.3.1/VRLib
#./build.sh $1
#pushd

export BUILD_MODULE=Shellspace

# echo "========================== Update "${BUILD_MODULE}" Project ==========================="
# android update project -t android-19 -p . -s
 
if [ "$1" == "clean" ]; then

    echo "========================== Clean "${BUILD_MODULE} $1 " ==========================="
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=1 
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=0
    ant clean 

else

    if [ "$1" == "" ] || [ "$1" == "debug" ]; then
        echo "========================== Build "${BUILD_MODULE}" Native ==========================="
        $ANDROID_NDK/ndk-build NDK_DEBUG=1 OVR_DEBUG=1 
    fi

    if [ "$1" == "release" ]; then
        echo "========================== Build "${BUILD_MODULE} $1 " Native ==========================="
        $ANDROID_NDK/ndk-build NDK_DEBUG=0 OVR_DEBUG=0 
    fi

    echo "========================== Build "${BUILD_MODULE}" Java ==========================="

    #For the time being, just build debug releases until we have a keystore for signing
    #We do the rename to avoid confusion, since ant will name this as -debug.apk but
    #the native code is all built with release optimizations.
    ant -quiet debug

    cp bin/${BUILD_MODULE}-debug.apk bin/${BUILD_MODULE}.apk

fi
