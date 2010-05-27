 
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
#  Copyright (c) 2004-2005  Purdue Research Foundation
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
    public method locations {}
    public method heights {}
    public method widths {}
    public method limits {which}
    public method xmarkers {}
    public method ymarkers {}
    public method hints {{key ""}}

    protected method _build {}

    private variable _xmlobj ""  ;# ref to lib obj with histogram data
    private variable _hist ""    ;# lib obj representing this histogram
    private variable _widths ""  ;# vector of bin widths (may be empty string).
    private variable _heights ""  ;# vector of bin heights along y-axis.
    private variable _locations "" ;# vector of bin locations along x-axis.

    private variable _hints      ;# cache of hints stored in XML
    private variable _xmarkers "";# list of {x,label,options} triplets.
    private variable _ymarkers "";# list of {y,label,options} triplets.
    private common _counter 0    ;# counter for unique vector names
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
    _build
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::destructor {} {
    itcl::delete object $_hist
    # don't destroy the _xmlobj! we don't own it!
    if {"" != $_widths} {
        blt::vector destroy $_widths
    }
    if {"" != $_heights} {
        blt::vector destroy $_heights
    }
    if {"" != $_locations} {
        blt::vector destroy $_locations
    }
}

# ----------------------------------------------------------------------
# USAGE: locations 
#
# Returns the vector for the histogram bin locations along the
# x-axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::locations {} {
    return $_locations
}

# ----------------------------------------------------------------------
# USAGE: heights 
#
# Returns the vector for the histogram bin heights along the y-axis.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::heights {} {
    return $_heights
}

# ----------------------------------------------------------------------
# USAGE: widths 
#
# Returns the vector for the specified histogram component <name>.
# If the name is not specified, then it returns the vectors for the
# overall histogram (sum of all components).
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::widths {} {
    return $_widths 
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
	x - xlin { 
	    set vname $_locations; 
	    set log 0; 
	    set axis xaxis 
	}
	xlog { 
	    set vname $_locations; 
	    set log 1; 
	    set axis xaxis 
	}
	y - ylin { 
	    set vname $_heights; 
	    set log 0; 
	    set axis yaxis 
	}
	ylog { 
	    set vname $_heights; 
	    set log 1; 
	    set axis yaxis 
	}
	default {
	    error "bad option \"$which\": should be x, xlin, xlog, y, ylin, ylog"
	}
    }

    if {"" == $vname} {
        return {0 1}
    }
    $vname dup tmp
    $vname dup zero
    if {$log} {
	# on a log scale, use abs value and ignore 0's
	zero expr {tmp == 0}            ;# find the 0's
	tmp expr {abs(tmp)}             ;# get the abs value
	tmp expr {tmp + zero*max(tmp)}  ;# replace 0's with abs max
	set vmin [blt::vector expr min(tmp)]
	set vmax [blt::vector expr max(tmp)]
    } else {
	set vmin [blt::vector expr min($vname)]
	set vmax [blt::vector expr max($vname)]
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
# USAGE: _build
#
# Used internally to build up the vector representation for the
# histogram when the object is first constructed, or whenever the histogram
# data changes.  Discards any existing vectors and builds everything
# from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::Histogram::_build {} {
    # discard any existing data
    if { $_locations != "" } {
	blt::vector destroy $_locations
	set _locations ""
    }
    if { $_widths != "" } {
	blt::vector destroy $_widths
	set _widths ""
    }
    if { $_heights != "" } {
	blt::vector destroy $_heights
	set _heights ""
    }

    #
    # Scan through the components of the histogram and create
    # vectors for each part.  Right now there's only one
    # component.  I left in the component tag in case future
    # enhancements require more than one component.
    #
    set xhwdata [$_hist get component.xhw]
    if {"" != $xhwdata} {
	set _widths [blt::vector create \#auto]
	set _heights [blt::vector create \#auto]
	set _locations [blt::vector create \#auto]

	foreach line [split $xhwdata \n] {
	    set n [scan $line {%s %s %s} x h w]
	    if { $n == 2 } {
		$_locations append $x
		$_heights append $h
	    } elseif { $n == 3 } { 
		$_locations append $x
		$_heights append $h
		$_widths append $w
	    }
	}
	# FIXME:  There must be a width specified for each bin location.
	#	  If this isn't true, we default to uniform widths
	#	  (zero-length _widths vector == uniform).
	if { [$_locations length] != [$_widths length] } {
	    $_widths set {}
	}
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
