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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Curve {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method components {{pattern *}}
    public method mesh {{what -overall}}
    public method values {{what -overall}}
    public method limits {which}
    public method hints {{key ""}}

    protected method _build {}

    private variable _xmlobj ""  ;# ref to lib obj with curve data
    private variable _curve ""   ;# lib obj representing this curve
    private variable _comp2xy    ;# maps component name => x,y vectors
    private variable _hints      ;# cache of hints stored in XML

    private common _counter 0    ;# counter for unique vector names
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj $xmlobj
    set _curve [$xmlobj element -as object $path]

    # build up vectors for various components of the curve
    _build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::destructor {} {
    itcl::delete object $_curve
    # don't destroy the _xmlobj! we don't own it!

    foreach name [array names _comp2xy] {
        eval blt::vector destroy $_comp2xy($name)
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
    foreach name [array names _comp2xy] {
        if {[string match $pattern $name]} {
            lappend rlist $name
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: mesh ?<name>?
#
# Returns the xvec for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::mesh {{what -overall}} {
    if {[info exists _comp2xy($what)]} {
        return [lindex $_comp2xy($what) 0]  ;# return xv
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2xy]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: values ?<name>?
#
# Returns the xvec for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::values {{what -overall}} {
    if {[info exists _comp2xy($what)]} {
        return [lindex $_comp2xy($what) 1]  ;# return yv
    }
    error "bad option \"$what\": should be [join [lsort [array names _comp2xy]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: limits x|xlin|xlog|y|ylin|ylog
#
# Returns the {min max} limits for the specified axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::limits {which} {
    set min ""
    set max ""
    switch -- $which {
        x - xlin { set pos 0; set log 0; set axis xaxis }
        xlog { set pos 0; set log 1; set axis xaxis }
        y - ylin - v - vlin { set pos 1; set log 0; set axis yaxis }
        ylog - vlog { set pos 1; set log 1; set axis yaxis }
        default {
            error "bad option \"$which\": should be x, xlin, xlog, y, ylin, ylog, v, vlin, vlog"
        }
    }

    blt::vector create tmp zero
    foreach comp [array names _comp2xy] {
        set vname [lindex $_comp2xy($comp) $pos]
        $vname variable vec

        if {$log} {
            # on a log scale, use abs value and ignore 0's
            $vname dup tmp
            $vname dup zero
            zero expr {tmp == 0}            ;# find the 0's
            tmp expr {abs(tmp)}             ;# get the abs value
            tmp expr {tmp + zero*max(tmp)}  ;# replace 0's with abs max
            set vmin [blt::vector expr min(tmp)]
            set vmax [blt::vector expr max(tmp)]
        } else {
            set vmin $vec(min)
            set vmax $vec(max)
        }

        if {"" == $min} {
            set min $vmin
        } elseif {$vmin < $min} {
            set min $vmin
        }
        if {"" == $max} {
            set max $vmax
        } elseif {$vmax > $max} {
            set max $vmax
        }
    }
    blt::vector destroy tmp zero

    set val [$_curve get $axis.min]
    if {"" != $val && "" != $min} {
        if {$val > $min} {
            # tool specified this min -- don't go any lower
            set min $val
        }
    }

    set val [$_curve get $axis.max]
    if {"" != $val && "" != $max} {
        if {$val < $max} {
            # tool specified this max -- don't go any higher
            set max $val
        }
    }

    return [list $min $max]
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about plotting
# this curve.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::hints {{keyword ""}} {
    if {![info exists _hints]} {
        foreach {key path} {
            group   about.group
            label   about.label
            color   about.color
            style   about.style
            xlabel  xaxis.label
            xunits  xaxis.units
            xscale  xaxis.scale
            xmin    xaxis.min
            xmax    xaxis.max
            ylabel  yaxis.label
            yunits  yaxis.units
            yscale  yaxis.scale
            ymin    yaxis.min
            ymax    yaxis.max
        } {
            set str [$_curve get $path]
            if {"" != $str} {
                set _hints($key) $str
            }
        }

        if {[info exists _hints(xlabel)] && "" != $_hints(xlabel)
              && [info exists _hints(xunits)] && "" != $_hints(xunits)} {
            set _hints(xlabel) "$_hints(xlabel) ($_hints(xunits))"
        }
        if {[info exists _hints(ylabel)] && "" != $_hints(ylabel)
              && [info exists _hints(yunits)] && "" != $_hints(yunits)} {
            set _hints(ylabel) "$_hints(ylabel) ($_hints(yunits))"
        }

        if {[info exists _hints(group)] && [info exists _hints(label)]} {
            # pop-up help for each curve
            set _hints(tooltip) $_hints(label)
        }
    }

    if {$keyword != ""} {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
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
    foreach name [array names _comp2xy] {
        eval blt::vector destroy $_comp2xy($name)
    }
    catch {unset _comp2xy}

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
            set _comp2xy($cname) [list $xv $yv]
            incr _counter
        }
    }
}
