#!/bin/bash
source ../bin/android_dev.sh
set -e 

# ./run.sh

echo "========================== Reloading scripts ==========================="

# adb push sdcard/safari.jpg /storage/extSdCard/Oculus/Shellspace/safari.jpg
# adb push sdcard/terminal.jpg /storage/extSdCard/Oculus/Shellspace/terminal.jpg

# adb push assets/autoexec.vrcfg /storage/extSdCard/Oculus/Shellspace/autoexec.vrcfg
# adb push assets/reset.cfg /storage/extSdCard/Oculus/Shellspace/reset.cfg
# adb push assets/shellspace.js /storage/extSdCard/Oculus/Shellspace/shellspace.js
# adb push assets/example.js /storage/extSdCard/Oculus/Shellspace/example.js
adb push assets/shell.js /storage/extSdCard/Oculus/Shellspace/shell.js
adb push assets/menu.js /storage/extSdCard/Oculus/Shellspace/menu.js
# adb push assets/vector.js /storage/extSdCard/Oculus/Shellspace/vector.js
# adb push assets/gl-matrix.js /storage/extSdCard/Oculus/Shellspace/gl-matrix.js
# adb push assets/sprintf.js /storage/extSdCard/Oculus/Shellspace/sprintf.js
adb push assets/entity_f.glsl /storage/extSdCard/Oculus/Shellspace/entity_f.glsl
adb push assets/entity_v.glsl /storage/extSdCard/Oculus/Shellspace/entity_v.glsl
adb push assets/vnc.menu /storage/extSdCard/Oculus/Shellspace/vnc.menu
adb push assets/start.menu /storage/extSdCard/Oculus/Shellspace/start.menu
# adb push assets/default.layout /storage/extSdCard/Oculus/Shellspace/default.layout

# adb push sdcard/user.cfg /storage/extSdCard/Oculus/Shellspace/user.cfg
#adb push sdcard/test.mp4 /storage/extSdCard/Oculus/Shellspace/test.mp4

echo '$$!exec reset.cfg' | pbcopy
