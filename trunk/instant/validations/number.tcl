# ----------------------------------------------------------------------
#  VALIDATION: number
#
#  Handles the type:validate=number setting for an object attribute.
#  Checks the given string to see if it is a valid Rappture "number"
#  value.  Returns an error if something is wrong.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
proc validate_number_parse {str numvar unitsvar} {
    upvar $numvar num
    upvar $unitsvar units
    return [regexp {^([-+]?(?:[0-9]+\.|\.)?[0-9]+(?:[eE][-+]?[0-9]+)?)( *(?:[a-zA-Z]+[0-9]*)+(?:\/(?:[a-zA-Z]+[0-9]*)+)*)*$} $str match num units]
}

proc validate_number {str} {
    if {"" == $str} {
        # empty string is okay
        return "ok"
    } elseif {[validate_number_parse $str num units]} {
        if {"" != $units && "" == [Rappture::Units::description $units]} {
            error "units \"$units\" not recognized"
        }
        return "ok"
    }
    error "should be a real number with units"
}
