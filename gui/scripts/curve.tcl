# ----------------------------------------------------------------------
#  COMPONENT: curve - extracts data from an XML description of a field
#
#  This object represents a curve of data in an XML description of
#  simulator output.  A curve is similar to a field, but a field is
#  a quantity versus position in device.  A curve is any quantity
#  versus any other quantity.  This class simplifies the process of
#  extracting data vectors that represent the curve.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Curve {
    constructor {libobj path} { # defined below }
    destructor { # defined below }

    public method components {{pattern *}}
    public method vectors {{what -overall}}
    public method controls {option args}
    public method hints {{key ""}}

    protected method _build {}

    private variable _libobj ""  ;# ref to lib obj with curve data
    private variable _curve ""   ;# lib obj representing this curve
    private variable _comp2vecs  ;# maps component name => x,y vectors

    private common _counter 0    ;# counter for unique vector names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::constructor {libobj path} {
    if {![Rappture::library isvalid $libobj]} {
        error "bad value \"$libobj\": should be LibraryObj"
    }
    set _libobj $libobj
    set _curve [$libobj element -flavor object $path]

    # build up vectors for various components of the curve
    _build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::destructor {} {
    itcl::delete object $_curve
    # don't destroy the _libobj! we don't own it!

    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
}

# ----------------------------------------------------------------------
# USAGE: components ?<pattern>?
#
# Returns a list of names for the various components of this curve.
# If the optional glob-style <pattern> is specified, then it returns
# only the component names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::components {{pattern *}} {
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
# Returns a list {xvec yvec} for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::vectors {{what -overall}} {
    if {[info exists _comp2vecs($what)]} {
        return $_comp2vecs($what)
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2vecs]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::hints {{keyword ""}} {
    foreach {key path} {
        label   label
        color   color
        color   style
        xlabel  xaxis.label
        xunits  xaxis.units
        xscale  xaxis.scale
        ylabel  yaxis.label
        yunits  yaxis.units
        yscale  yaxis.scale
    } {
        set str [$_curve get $path]
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
# curve when the object is first constructed, or whenever the curve
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::_build {} {
    # discard any existing data
    foreach name [array names _comp2vecs] {
        eval blt::vector destroy $_comp2vecs($name)
    }
    catch {unset _comp2vecs}

    #
    # Scan through the components of the curve and create
    # vectors for each part.
    #
    foreach cname [$_curve children -type component] {
        set xv ""
        set yv ""

        set xydata [$_curve get $cname.xy]
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

        if {$xv != "" && $yv != ""} {
            set _comp2vecs($cname) [list $xv $yv]
            incr _counter
        }
    }
}
