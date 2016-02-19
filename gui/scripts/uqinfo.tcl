# -*- mode: tcl; indent-tabs-mode: nil -*-

package require Itcl
package require BLT

namespace eval Rappture {
    # forward declaration
}

itcl::class Rappture::UqInfo {
    constructor {xmlobj path uq_part} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method add {xmlobj path uq_part}
    public method get {}
    public method desc {}
    public method hints {{key ""}}
    private method _renumber {}
    private variable _objlist;      # UQ objects
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::UqInfo::constructor {xmlobj path uq_part} {
    #puts "UQ INFO CONST $xmlobj $path $uq_part"
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    add $xmlobj $path $uq_part
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::UqInfo::destructor {} {
}

itcl::body Rappture::UqInfo::_renumber {} {
    set numtabs [array size _objlist]
    set index 0

    foreach name [list PDF Probability Sensitivity Response "All Runs" Sequence ] {
        if {[info exists _objlist($name)]} {
            foreach {i items} $_objlist($name) break
            set _objlist($name) [list $index $items]
            incr index
        }
    }
}

#
# _objlist is an array of lists.
# lindex 0 is the index (tab number)
# lindex 1 is a list of lists containing the xmlobj and path
# of each element.
itcl::body Rappture::UqInfo::add {xmlobj path uq_part} {
    #puts "\nUQ INFO ADD $xmlobj $path $uq_part"

    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }

    if {![info exists _objlist($uq_part)]} {
        set _objlist($uq_part)  {0 {}}
    }

    foreach {index items} $_objlist($uq_part) break
    lappend items [list $xmlobj $path]
    set _objlist($uq_part) [list $index $items]
    _renumber
}


# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted.
# ----------------------------------------------------------------------
itcl::body Rappture::UqInfo::get {} {
    #uts "UqInfo::get"
    return [array get _objlist]
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::UqInfo::hints {{keyword ""}} {
    #puts "UqInfo hints $keyword"
}