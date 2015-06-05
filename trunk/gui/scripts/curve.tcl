# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture {
    # forward declaration
}

itcl::class Rappture::Curve {
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method components {{pattern *}}
    public method mesh {cname }
    public method values { cname }
    public method limits {which}
    public method hints {{key ""}}
    public method xmarkers {}
    public method ymarkers {}
    public method xErrorValues { cname }
    public method yErrorValues { cname }

    protected method _build {}

    private variable _xmlobj ""  ;      # ref to lib obj with curve data
    private variable _curve ""   ;      # lib obj representing this curve
    private variable _comp2xy    ;      # maps component name => x,y vectors
    private variable _hints      ;      # cache of hints stored in XML

    private variable _xmarkers "";      # list of {x,label,options} triplets.
    private variable _ymarkers "";      # list of {y,label,options} triplets.
    private common _counter 0    ;      # counter for unique vector names
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
itcl::body Rappture::Curve::mesh {cname} {
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 0]  ;# return xv
    }
    error "bad component \"$cname\": should be one of [join [lsort [array names _comp2xy]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: values ?<name>?
#
# Returns the yvec for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::values {cname} {
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 1]  ;# return yv
    }
    error "bad component \"$cname\": should be one of [join [lsort [array names _comp2xy]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: xErrorValues <name>
#
# Returns the xvec for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::xErrorValues { cname } {
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 2]  ;# return xev
    }
    error "unknown component \"$cname\": should be one of [join [lsort [array names _comp2xy]] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: yErrorValues <name>
#
# Returns the xvec for the specified curve component <name>.
# If the name is not specified, then it returns the vectors for the
# overall curve (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::yErrorValues { cname } {
    if {[info exists _comp2xy($cname)]} {
        return [lindex $_comp2xy($cname) 3]  ;# return yev
    }
    error "unknown component \"$cname\": should be one of [join [lsort [array names _comp2xy]] {, }]"
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

    blt::vector tmp zero
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
            color   about.color
            group   about.group
            label   about.label
            style   about.style
            type    about.type
            xdesc   xaxis.description
            xlabel  xaxis.label
            xmax    xaxis.max
            xmin    xaxis.min
            xscale  xaxis.scale
            xticks  xaxis.ticklabels
            xunits  xaxis.units
            ydesc   yaxis.description
            ylabel  yaxis.label
            ymax    yaxis.max
            ymin    yaxis.min
            yscale  yaxis.scale
            yticks  yaxis.ticklabels
            yunits  yaxis.units
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
        set _hints(xmlobj) $_xmlobj
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
        set xv [blt::vector create \#auto]
        set yv [blt::vector create \#auto]
        set xev [blt::vector create \#auto]
        set yev [blt::vector create \#auto]

        set xydata [$_curve get $cname.xy]
        if { "" != $xydata} {
            set tmp [blt::vector create \#auto]
            $tmp set $xydata
            $tmp split $xv $yv
            blt::vector destroy $tmp
        } else {
            $xv set [$_curve get $cname.xvector]
            $yv set [$_curve get $cname.yvector]
        }
        if { (([$xv length] == 0) && ([$yv length] == 0)) ||
             ([$xv length] != [$yv length]) } {
            # FIXME: need to show an error about improper data.
            blt::vector destroy $xv $yv
            set xv ""; set yv ""
            continue;
        }
        $xev set [$_curve get "$cname.xerrorbars"]
        $yev set [$_curve get "$cname.yerrorbars"]
        set _comp2xy($cname) [list $xv $yv $xev $yev]
        incr _counter
    }

    # Creates lists of x and y marker data.
    set _xmarkers {}
    set _ymarkers {}
    foreach cname [$_curve children -type "marker" xaxis] {
        set at     [$_curve get "xaxis.$cname.at"]
        set label  [$_curve get "xaxis.$cname.label"]
        set styles [$_curve get "xaxis.$cname.style"]
        set data [list $at $label $styles]
        lappend _xmarkers $data
    }
    foreach cname [$_curve children -type "marker" yaxis] {
        set at     [$_curve get "yaxis.$cname.at"]
        set label  [$_curve get "yaxis.$cname.label"]
        set styles [$_curve get "yaxis.$cname.style"]
        set data [list $at $label $styles]
        lappend _ymarkers $data
    }
}

# ----------------------------------------------------------------------
# USAGE: xmarkers
#
# Returns the list of settings for each marker on the x-axis.
# If no markers have been specified the empty string is returned.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::xmarkers {} {
    return $_xmarkers;
}

# ----------------------------------------------------------------------
# USAGE: ymarkers
#
# Returns the list of settings for each marker on the y-axis.
# If no markers have been specified the empty string is returned.
# ----------------------------------------------------------------------
itcl::body Rappture::Curve::ymarkers {} {
    return $_ymarkers;
}

