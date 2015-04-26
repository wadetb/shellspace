#!/bin/bash

pushd ../ovr_mobile_sdk_0.4.3.1/VRLib
./build.sh $1
pushd

export BUILD_MODULE=Shellspace

echo "========================== Update "${BUILD_MODULE}" Project ==========================="
android update project -t android-19 -p . -s
 
if [ -z "$ANDROID_NDK" ]; then
  ANDROID_NDK=~/ndk
fi

if [ "$1" == "" ]; then
    echo "========================== Build "${BUILD_MODULE}" ==========================="
    $ANDROID_NDK/ndk-build NDK_LOG=1 NDK_DEBUG=1 OVR_DEBUG=1 
fi

if [ "$1" == "debug" ]; then
    echo "========================== Build "${BUILD_MODULE} $1 " ==========================="
    $ANDROID_NDK/ndk-build NDK_DEBUG=1 OVR_DEBUG=1 
fi

if [ "$1" == "release" ]; then
    echo "========================== Build "${BUILD_MODULE} $1 " ==========================="
    $ANDROID_NDK/ndk-build NDK_DEBUG=0 OVR_DEBUG=0 
fi

if [ "$1" == "clean" ]; then
    echo "========================== Build "${BUILD_MODULE} $1 " ==========================="
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=1 
    $ANDROID_NDK/ndk-build clean NDK_DEBUG=0
    ant clean 
fi
