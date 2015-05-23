#!/bin/bash
source ../bin/android_dev.sh
set -e 

arm-linux-androideabi-addr2line -C -f -e obj/local/armeabi-v7a/libshellspace.so $1
