# ----------------------------------------------------------------------
#  COMPONENT: table - extracts data from an XML description of a table
#
#  This object represents one table in an XML description of a device.
#  It simplifies the process of extracting data representing columns
#  in the table.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Table {
    constructor {libobj path} { # defined below }
    destructor { # defined below }

    public method rows {}
    public method columns {{pattern *}}
    public method vectors {{what -overall}}
    public method hints {{key ""}}

    protected method _build {}

    private variable _units ""   ;# system of units for this table
    private variable _limits     ;# maps slab name => {z0 z1} limits
    private variable _zmax 0     ;# length of the device

    private variable _table ""   ;# lib obj representing this table
    private variable _tree ""    ;# BLT tree used to contain table data

    private common _counter 0    ;# counter for unique vector names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Table::constructor {libobj path} {
    if {![Rappture::library isvalid $libobj]} {
        error "bad value \"$libobj\": should be LibraryObj"
    }
    set _table [$libobj element -as object $path]
    set _units [$_table get units]

    # determine the overall size of the device
    set z0 [set z1 0]
    foreach elem [$_device children recipe] {
        switch -glob -- $elem {
            slab* - molecule* {
                if {![regexp {[0-9]$} $elem]} {
                    set elem "${elem}0"
                }
                set tval [$_device get recipe.$elem.thickness]
                set tval [Rappture::Units::convert $tval \
                    -context um -to um -units off]
                set z1 [expr {$z0+$tval}]
                set _limits($elem) [list $z0 $z1]

                set z0 $z1
            }
        }
    }
    set _zmax $z1

    # build up vectors for various components of the table
    _build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Table::destructor {} {
    itcl::delete object $_table
    # don't destroy the _device! we don't own it!

    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
}

# ----------------------------------------------------------------------
# USAGE: components ?<pattern>?
#
# Returns a list of names for the various components of this table.
# If the optional glob-style <pattern> is specified, then it returns
# only the component names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::components {{pattern *}} {
    set rlist ""
    foreach name [array names _comp2vecs] {
        if {[string match $pattern $name]} {
            lappend rlist $name
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: vectors ?<name>?
#
# Returns a list {xvec yvec} for the specified table component <name>.
# If the name is not specified, then it returns the vectors for the
# overall table (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Table::vectors {{what -overall}} {
    if {[info exists _comp2vecs($what)]} {
        return $_comp2vecs($what)
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2vecs]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this table.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::hints {{keyword ""}} {
    foreach key {label scale color units restrict} {
        set str [$_table get $key]
        if {"" != $str} {
            set hints($key) $str
        }
    }

    if {$keyword != ""} {
        if {[info exists hints($keyword)]} {
            return $hints($keyword)
        }
        return ""
    }
    return [array get hints]
}

# ----------------------------------------------------------------------
# USAGE: _build
#
# Used internally to build up the vector representation for the
# table when the object is first constructed, or whenever the table
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Table::_build {} {
    # discard any existing data
    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
    catch {unset _comp2vecs}

    #
    # Scan through the components of the table and create
    # vectors for each part.
    #
    foreach cname [$_table children -type component] {
        set xv ""
        set yv ""

        set val [$_table get $cname.constant]
        if {$val != ""} {
            set domain [$_table get $cname.domain]
            if {$domain == "" || ![info exists _limits($domain)]} {
                set z0 0
                set z1 $_zmax
            } else {
                foreach {z0 z1} $_limits($domain) { break }
            }
            set xv [blt::vector create x$_counter]
            $xv append $z0 $z1

            if {$_units != ""} {
                set val [Rappture::Units::convert $val \
                    -context $_units -to $_units -units off]
            }
            set yv [blt::vector create y$_counter]
            $yv append $val $val

            set zm [expr {0.5*($z0+$z1)}]
        } else {
            set xydata [$_table get $cname.xy]
            if {"" != $xydata} {
                set xv [blt::vector create x$_counter]
                set yv [blt::vector create y$_counter]

                foreach line [split $xydata \n] {
                    if {[scan $line {%g %g} xval yval] == 2} {
                        $xv append $xval
                        $yv append $yval
                    }
                }
            }
        }

        if {$xv != "" && $yv != ""} {
            set _comp2vecs($cname) [list $xv $yv]
            incr _counter
        }
    }
}
