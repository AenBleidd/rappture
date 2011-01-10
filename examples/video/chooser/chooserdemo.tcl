#!/usr/bin/env sh
#\
exec wish "$0" $*

package require RapptureGUI

set videoFiles [list \
    "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-05-55.mp4" \
    "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-09-56.mp4" \
    "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-13-56.mp4" \
    "/home/derrick/junk/coa360download56Kbps240x160.mpg" \
]


frame .f
pack .f -expand yes -fill both

set vc [Rappture::VideoChooser .f.vc]
$vc load $videoFiles

pack .f.vc -expand yes -fill both
