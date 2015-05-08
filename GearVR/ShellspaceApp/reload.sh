#!/bin/bash

source ../bin/dev.sh

echo "========================== Reloading scripts ==========================="

adb push sdcard/autoexec.vrcfg /storage/extSdCard/Oculus/Shellspace/autoexec.vrcfg
adb push sdcard/connect.vrkey /storage/extSdCard/Oculus/Shellspace/connect.vrkey
adb push sdcard/example.js /storage/extSdCard/Oculus/Shellspace/example.js

echo '
$$!
shellspace_example unload
v8 load /storage/extSdCard/Oculus/Shellspace/example.js
' | pbcopy
