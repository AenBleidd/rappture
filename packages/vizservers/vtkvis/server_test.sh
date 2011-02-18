#!/bin/bash

PATH=.:/usr/local/bin:/usr/bin:/bin
LD_LIBRARY_PATH=/usr/local/lib64

export PATH LD_LIBRARY_PATH

#dir=/usr/local/bin
dir=/home/ldelgass/src/svn/hubzero/rappture/packages/vizservers/vtkvis

nanoscale -x 2 -l 2010 -c "${dir}/vtkvis"
