#!/bin/bash

source ../bin/dev.sh

set -e 

# ./run.sh

echo "========================== Reloading scripts ==========================="

# adb push sdcard/safari.jpg /storage/extSdCard/Oculus/Shellspace/safari.jpg
# adb push sdcard/terminal.jpg /storage/extSdCard/Oculus/Shellspace/terminal.jpg

adb push sdcard/autoexec.vrcfg /storage/extSdCard/Oculus/Shellspace/autoexec.vrcfg
adb push sdcard/shellspace.js /storage/extSdCard/Oculus/Shellspace/shellspace.js
adb push sdcard/example.js /storage/extSdCard/Oculus/Shellspace/example.js
adb push sdcard/shell.js /storage/extSdCard/Oculus/Shellspace/shell.js
adb push sdcard/menu.js /storage/extSdCard/Oculus/Shellspace/menu.js
adb push sdcard/vector.js /storage/extSdCard/Oculus/Shellspace/vector.js
adb push sdcard/entity_f.glsl /storage/extSdCard/Oculus/Shellspace/entity_f.glsl
adb push sdcard/entity_v.glsl /storage/extSdCard/Oculus/Shellspace/entity_v.glsl

echo '
$$!
file http disable
shell unload
menu unload
shellspace_example unload
v8 load shell.js
v8 load menu.js
v8 load example.js
' | pbcopy
