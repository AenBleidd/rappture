#! /bin/sh

DISPLAY=:0
export DISPLAY

# Fail on errors
set -e

exec_prefix="@exec_prefix@"
bindir="@bindir@"
libdir="@libdir@"
. ${bindir}/rappture.env

# Determine the number of video cards we have.  The new render server
# motherboards have the useless XGI Volari onboard video controllers (no 3D
# capabilities) so we have to make sure we count only the nVidia cards.

numVideo=`lspci | fgrep VGA | fgrep nVidia | wc -l`

# Don't let nanoscale and the visualization servers run away.  Limit cpu time
# to 20 minutes.

minutes=20
ulimit -t $(expr ${minutes} \* 60 )

# Make /tmp the current working dirctory.
cd /tmp

exec ${bindir}/nanoscale -x ${numVideo} -f ${libdir}/renderservers.tcl


