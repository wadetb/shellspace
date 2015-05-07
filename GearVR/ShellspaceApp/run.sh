#!/bin/bash

source ../bin/dev.sh

export BUILD_MODULE=Shellspace

echo "========================== Run "${BUILD_MODULE}" ==========================="

export BUILD_MODULE=Shellspace

adb shell am start com.wadeb.Shellspace/oculus.MainActivity
