#! /bin/sh

DISPLAY=:0
export DISPLAY

dir=$( mktemp -d /tmp/nanorun.XXXXXX )

cp -r nanovis/shaders ${dir}
cp -r nanovis/resources ${dir}
cp nanoscale/nanoscale nanovis/nanovis ${dir}

cd ${dir}

./nanoscale -l 2000 -b 2000 -c 172.18.3.255 -c ${dir}/nanovis
