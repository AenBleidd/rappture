# ----------------------------------------------------------------------
#  EXAMPLE: Rappture <image> elements
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Tk
package require Rappture
wm withdraw .
package require BLT
package require Img

# open the XML file containing the run parameters
set driver [Rappture::library [lindex $argv 0]]

set data [$driver get input.image.current]
set angle [$driver get input.(angle).current]
set angle [Rappture::Units::convert $angle -to deg -units off]

set imh [image create photo -data $data]
set dest [image create photo]
blt::winop image rotate $imh $dest $angle

$driver put output.image(outi).about.label "Rotated Image"
$driver put output.image(outi).current [$dest data -format jpeg]

# add a little html note
set htmltext "html://<p style=\"text-align: center;\"><a href=\"angles.html\">Learn more about angles...</a></p>"
$driver put output.image(outi).note.contents $htmltext

# save the updated XML describing the run...
Rappture::result $driver
exit 0
