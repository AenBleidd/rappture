# ----------------------------------------------------------------------
#  VALIDATION: int
#
#  Handles the string:validate=int setting for an object attribute.
#  Checks the given string to see if it is an integer value.  Returns
#  an error if something is wrong.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
proc validate_int {str} {
    if {![string is integer $str]} {
        error "bad value \"$str\": should be an integer"
    }
}
