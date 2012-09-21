# ----------------------------------------------------------------------
#  VALIDATION: size
#
#  Handles the string:validate=size setting for an object attribute.
#  Checks the given string to see if it is a valid size for a <string>
#  object.  Returns an error if something is wrong.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
proc validate_size {str} {
    switch -regexp -- $str {
        ^$ - ^[0-9]+$ - ^[0-9]+x[0-9]+$ - binary {
            return "ok"
        }
        default {
            error "bad size \"$str\": should be WW or WWxHH or binary"
        }
    }
}
