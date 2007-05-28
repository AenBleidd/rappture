#! /bin/sh

DISPLAY=:0
export DISPLAY

dir=$( mktemp -d /tmp/nanorun.XXXXXX )

PATH=${dir}:/home/nanohub/dkearney/repo/viz_20070521/bin:$PATH
LD_LIBRARY_PATH=${dir}/lib:$LD_LIBRARY_PATH
export PATH LD_LIBRARY_PATH

mkdir ${dir}/bin ${dir}/lib

cp -r lib/shaders ${dir}
cp -r lib/resources ${dir}
cp lib/lib* ${dir}/lib
cp bin/voronoi bin/nanoscale bin/nanovis ${dir}

cd ${dir}

${dir}/nanoscale -l 2020 -b 2020 -s 172.18.3.255 -c ${dir}/nanovis
