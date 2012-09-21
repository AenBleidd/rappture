# ----------------------------------------------------------------------
#  VALIDATION: imformat
#
#  Handles the string:validate=imformat setting for an object attribute.
#  Checks the given string to see if it is one of the allowed image
#  format strings:  gif, jpeg, png, etc.  Returns an error if something
#  is wrong.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
proc validate_imformat {str} {
    set valid {bmp gif ico jpeg pcx png pgm ppm ps pdf sgi sun tga tiff xbm xpm}
    if {$str ne "" && [lsearch $valid $str] < 0} {
        error "bad format \"$str\": should be one of [join $valid {, }]"
    }
    return "ok"
}
