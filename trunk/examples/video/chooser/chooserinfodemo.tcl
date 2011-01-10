#!/usr/bin/env sh
#\
exec wish "$0" $*

package require RapptureGUI

array set videoFiles [list \
    1 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-05-55.mp4" \
    2 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-09-56.mp4" \
    3 "/home/derrick/projects/piv/video/TOPHAT_06-03-10_16-13-56.mp4" \
    4 "/home/derrick/junk/coa360download56Kbps240x160.mpg" \
]


frame .f
pack .f -expand yes -fill both

foreach k [array names videoFiles] {
    set ci [Rappture::VideoChooserInfo .f.ci$k]
    $ci load $videoFiles($k) ""
    pack $ci -expand yes -fill both -side right
}

