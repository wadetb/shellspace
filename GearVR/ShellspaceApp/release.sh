#!/bin/bash
source ../bin/dev.sh
set -e 

cp -v signatures/oculussig_* assets/

./build.sh

rm -v assets/oculussig_*

cp -v bin/Shellspace.apk releases/$1.apk

zip -rqdd releases/$1.zip obj/

scp -P 10300 releases/$1.apk wvm@wadeb.com:www/shellspace/

echo
echo Release uploaded to:     http://wadeb.com/shellspace/$1.apk
echo
