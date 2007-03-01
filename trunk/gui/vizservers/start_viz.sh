#! /bin/sh

dir=$( mktemp -d /tmp/nanorun.XXXXXX )

cp nanoscale/nanoscale nanovis/nanovis ${dir}

cd ${dir}

./nanoscale -l 2000 -b 2000 -c 172.18.3.255 -c ./nanovis
