source ../bin/dev.sh
set -e 

export BUILD_MODULE=Shellspace

echo "========================== Install "${BUILD_MODULE}" ==========================="

adb install -r bin/${BUILD_MODULE}.apk
