#!/bin/bash
source ../bin/android_dev.sh
set -e 

echo "========================== Run Shellspace ==========================="

source ./reload.sh

adb shell am force-stop com.wadeb.Shellspace
adb shell am start com.wadeb.Shellspace/oculus.MainActivity
