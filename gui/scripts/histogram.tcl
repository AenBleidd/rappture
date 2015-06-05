# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: histogram - extracts data from an XML description of a field
#
#  This object represents a histogram of data in an XML description of
#  simulator output.  A histogram is similar to a field, but a field is
#  a quantity versus position in device.  A histogram is any quantity
#  versus any other quantity.  This class simplifies the process of
#  extracting data vectors that represent the histogram.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Histogram {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method components {{pattern *}}
    public method mesh { component }
    public method values { component }
    public method widths { component }
    public method xlabels { component }
    public method limits {which}
    public method xmarkers {}
    public method ymarkers {}
    public method hints {{key ""}}

    protected method Build {}
    private method Clear { {comp ""} }
    private method ParseData { comp }

    private variable _xmlobj ""  ;# ref to lib obj with histogram data
    private variable _hist ""    ;# lib obj representing this histogram
    private variable _widths     ;# array of vectors of bin widths
    private variable _yvalues    ;# array of vectors of bin heights along
                                 ;# y-axis.
    private variable _xvalues    ;# array of vectors of bin locations along
                                 ;# x-axis.
    private variable _xlabels    ;# array of labels
    private variable _hints      ;# cache of hints stored in XML
    private variable _xmarkers "";# list of {x,label,options} triplets.
    private variable _ymarkers "";# list of {y,label,options} triplets.
    private common _counter 0    ;# counter for unique vector names
    private variable _comp2hist  ;# maps component name => x,y,w,l vectors
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj $xmlobj
    set _hist [$xmlobj element -as object $path]

    # build up vectors for various components of the histogram
    Build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::destructor {} {
    # don't destroy the _xmlobj! we don't own it!
    itcl::delete object $_hist
    Clear
}

# ----------------------------------------------------------------------
# USAGE: mesh
#
# Returns the vector for the histogram bin locations along the
# x-axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::mesh { comp } {
    if { [info exists _xvalues($comp)] } {
        return $_xvalues($comp)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: heights
#
# Returns the vector for the histogram bin heights along the y-axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::values { comp } {
    if { [info exists _yvalues($comp)] } {
        return $_yvalues($comp)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: widths
#
# Returns the vector for the specified histogram component <name>.
# If the name is not specified, then it returns the vectors for the
# overall histogram (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::widths { comp } {
    if { [info exists _widths($comp)] } {
        return $_widths($comp)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: xlabels
#
# Returns the vector for the specified histogram component <name>.
# If the name is not specified, then it returns the vectors for the
# overall histogram (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::xlabels { comp } {
    if { [info exists _xlabels($comp)] } {
        return $_xlabels($comp)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: xmarkers
#
# Returns the list of settings for each marker on the x-axis.
# If no markers have been specified the empty string is returned.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::xmarkers {} {
    return $_xmarkers;
}

# ----------------------------------------------------------------------
# USAGE: components ?<pattern>?
#
# Returns a list of names for the various components of this curve.
# If the optional glob-style <pattern> is specified, then it returns
# only the component names matching the pattern.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::components {{pattern *}} {
    set rlist ""
    foreach name [array names _comp2hist] {
        if {[string match $pattern $name]} {
            lappend rlist $name
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: ymarkers
#
# Returns the list of settings for each marker on the y-axis.
# If no markers have been specified the empty string is returned.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::ymarkers {} {
    return $_ymarkers;
}

# ----------------------------------------------------------------------
# USAGE: limits x|xlin|xlog|y|ylin|ylog
#
# Returns the {min max} limits for the specified axis.
#
# What does it mean to view a distribution (the bins) as log scale?
#
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::limits {which} {
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

    blt::vector create tmp
    blt::vector create zero
    foreach comp [array names _comphist] {
        set vname [lindex $_comp2hist($comp) $pos]
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

    set val [$_hist get $axis.min]
    if {"" != $val && "" != $min} {
        if {$val > $min} {
            # tool specified this min -- don't go any lower
            set min $val
        }
    }

    set val [$_hist get $axis.max]
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
# this histogram.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::hints {{keyword ""}} {
    if {![info exists _hints]} {
        foreach {key path} {
            group   about.group
            label   about.label
            color   about.color
            style   about.style
            type    about.type
            xlabel  xaxis.label
            xdesc   xaxis.description
            xunits  xaxis.units
            xorient xaxis.orientation
            xscale  xaxis.scale
            xmin    xaxis.min
            xmax    xaxis.max
            ylabel  yaxis.label
            ydesc   yaxis.description
            yunits  yaxis.units
            yscale  yaxis.scale
            ymin    yaxis.min
            ymax    yaxis.max
        } {
            set str [$_hist get $path]
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
            # pop-up help for each histogram
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
# USAGE: Build
#
# Used internally to build up the vector representation for the
# histogram when the object is first constructed, or whenever the histogram
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::Build {} {
    # discard any existing data
    Clear
    #
    # Scan through the components of the histogram and create
    # vectors for each part.  Right now there's only one
    # component.  I left in the component tag in case future
    # enhancements require more than one component.
    #
    foreach cname [$_hist children -type component] {
        ParseData $cname
    }
    # Creates lists of x and y marker data.
    set _xmarkers {}
    set _ymarkers {}
    foreach cname [$_hist children -type "marker" xaxis] {
        set at     [$_hist get "xaxis.$cname.at"]
        set label  [$_hist get "xaxis.$cname.label"]
        set styles [$_hist get "xaxis.$cname.style"]
        set data [list $at $label $styles]
        lappend _xmarkers $data
    }
    foreach cname [$_hist children -type "marker" yaxis] {
        set at     [$_hist get "yaxis.$cname.at"]
        set label  [$_hist get "yaxis.$cname.label"]
        set styles [$_hist get "yaxis.$cname.style"]
        set data [list $at $label $styles]
        lappend _xmarkers $data
    }
}

#
# ParseData --
#
#       Parse the components data representations.  The following
#       elements may be used <xy>, <xhw>, <namevalue>, <xvector>,
#       <yvector>.  Only one element is used for data.
#
itcl::body Rappture::Histogram::ParseData { comp } {
    # Create new vectors or discard any existing data
    set _xvalues($comp) [blt::vector create \#auto]
    set _yvalues($comp) [blt::vector create \#auto]
    set _widths($comp) [blt::vector create \#auto]
    set _xlabels($comp) {}

    set xydata [$_hist get ${comp}.xy]
    if { $xydata != "" } {
        set count 0
        foreach {name value} [regsub -all "\[ \t\n]+" $xydata { }] {
            $_yvalues($comp) append $value
            $_xvalues($comp) append $count
            lappend _xlabels($comp) $name
            incr count
        }
        set _comp2hist($comp) [list $_xvalues($comp) $_yvalues($comp)]
        return
    }
    set xhwdata [$_hist get ${comp}.xhw]
    if { $xhwdata != "" } {
        set count 0
        foreach {name h w} [regsub -all "\[ \t\n]+" $xhwdata { }] {
            lappend _xlabels($comp) $name
            $_xvalues($comp) append $count
            $_yvalues($comp) append $h
            $_widths($comp) append $w
            incr count
        }
        set _comp2hist($comp) [list $_xvalues($comp) $_yvalues($comp)]
        return
    }

    # If we reached here, must be <yvector>
    $_yvalues($comp) set [$_hist get ${comp}.yvector]
    $_xvalues($comp) length [$_yvalues($comp) length]
    $_xvalues($comp) seq 1 [$_yvalues($comp) length]
    set _xlabels($comp) [$_hist get ${comp}.xvector]
    set _comp2hist($comp) [list $_xvalues($comp) $_yvalues($comp)]
}

itcl::body Rappture::Histogram::Clear { {comp ""} } {
    if { $comp == "" } {
        foreach name [array names _widths] {
            blt::vector destroy $_widths($name)
        }
        array unset _widths
        foreach name [array names _yvalues] {
            blt::vector destroy $_yvalues($name)
        }
        array unset _yvalues
        foreach name [array names _xvalues] {
            blt::vector destroy $_xvalues($name)
        }
        array unset _xvalues
        array unset _xlabels
        array unset _comp2hist
        return
    }
    if { [info exists _widths($comp)] } {
        blt::vector destroy $_widths($comp)
    }
    if { [info exists _yvalues($comp)] } {
        blt::vector destroy $_yvalues($comp)
    }
    if { [info exists _xvalues($comp)] } {
        blt::vector destroy $_xvalues($comp)
    }
    array unset _xvalues $comp
    array unset _yvalues $comp
    array unset _widths $comp
    array unset _xlabels $comp
    array unset _comp2hist $comp
}

