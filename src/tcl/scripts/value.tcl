# ----------------------------------------------------------------------
# USAGE: value <object> <path>
#
# Used to query the "value" associated with the <path> in an XML
# <object>.  This is a little more complicated than the object's
# "get" method.  It handles things like structures and values
# with normalized units.
#
# Returns a list of two items:  {raw norm} where "raw" is the raw
# value from the "get" method and "norm" is the normalized value
# produced by this routine.  Example:  {300K 300}
#
# Right now, it is a handy little utility used by the "diff" method.
# Eventually, it should be moved to a better object-oriented
# implementation, where each Rappture type could overload the
# various bits of processing below.  So we leave it as a "proc"
# now instead of a method, since it should be deprecated soon.
# ----------------------------------------------------------------------
proc Rappture::LibraryObj::value {libobj path} {
    switch -- [$libobj element -as type $path] {
        structure {
            set raw $path
            # try to find a label to represent the structure
            set val [$libobj get $path.about.label]
            if {"" == $val} {
                set val [$libobj get $path.current.about.label]
            }
            if {"" == $val} {
                if {[$libobj element $path.current] != ""} {
                    set comps [$libobj children $path.current.components]
                    set val "<structure> with [llength $comps] components"
                } else {
                    set val "<structure>"
                }
            }
            return [list $raw $val]
        }
        number {
            # get the usual value...
            set raw ""
            if {"" != [$libobj element $path.current]} {
                set raw [$libobj get $path.current]
            } elseif {"" != [$libobj element $path.default]} {
                set raw [$libobj get $path.default]
            }
            if {"" != $raw} {
                set val $raw
                # then normalize to default units
                set units [$libobj get $path.units]
                if {"" != $units} {
                    set val [Rappture::Units::convert $val \
                        -context $units -to $units -units off]
                }
            }
            return [list $raw $val]
        }
    }

    # for all other types, get the value (current, or maybe default)
    set raw ""
    if {"" != [$libobj element $path.current]} {
        set raw [$libobj get $path.current]
    } elseif {"" != [$libobj element $path.default]} {
        set raw [$libobj get $path.default]
    }
    return [list $raw $raw]
}
