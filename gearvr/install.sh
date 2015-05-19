#!/bin/bash
source ../bin/android_dev.sh
set -e 

echo "========================== Install Shellspace ==========================="

adb install -r bin/Shellspace.apk
