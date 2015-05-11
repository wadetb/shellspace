#!/bin/bash

source ../bin/dev.sh

set -e 

# ./run.sh

echo "========================== Reloading scripts ==========================="

# adb push sdcard/safari.jpg /storage/extSdCard/Oculus/Shellspace/safari.jpg
# adb push sdcard/terminal.jpg /storage/extSdCard/Oculus/Shellspace/terminal.jpg

adb push assets/autoexec.vrcfg /storage/extSdCard/Oculus/Shellspace/autoexec.vrcfg
adb push assets/shellspace.js /storage/extSdCard/Oculus/Shellspace/shellspace.js
adb push assets/example.js /storage/extSdCard/Oculus/Shellspace/example.js
adb push assets/shell.js /storage/extSdCard/Oculus/Shellspace/shell.js
adb push assets/menu.js /storage/extSdCard/Oculus/Shellspace/menu.js
adb push assets/vector.js /storage/extSdCard/Oculus/Shellspace/vector.js
adb push assets/entity_f.glsl /storage/extSdCard/Oculus/Shellspace/entity_f.glsl
adb push assets/entity_v.glsl /storage/extSdCard/Oculus/Shellspace/entity_v.glsl

adb push sdcard/user.cfg /storage/extSdCard/Oculus/Shellspace/user.cfg

echo '
$$!
file http disable
shell unload
menu unload
example unload
v8 load shell.js
v8 load menu.js
v8 load example.js
' | pbcopy
