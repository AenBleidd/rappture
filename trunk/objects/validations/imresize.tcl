# ----------------------------------------------------------------------
#  VALIDATION: imresize
#
#  Handles the string:validate=imresize setting for an object attribute.
#  Checks the given string to see if it is one of the allowed resize
#  settings:  auto, none, width=XX, height=XX.  Returns an error if
#  something is wrong.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
proc validate_imresize {str} {
    switch -regexp -- $str {
        ^$ - ^auto$ - ^none$ - ^width=[0-9]+$ - ^height=[0-9]+$ {
            return "ok"
        }
        default {
            error "bad size \"$str\": should be auto, none, width=WW, or height=HH"
        }
    }
}
