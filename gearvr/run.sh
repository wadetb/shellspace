#!/bin/bash

source ../bin/dev.sh

export BUILD_MODULE=Shellspace

echo "========================== Run "${BUILD_MODULE}" ==========================="

source ./reload.sh

adb shell am force-stop com.wadeb.Shellspace
adb shell am start com.wadeb.Shellspace/oculus.MainActivity
