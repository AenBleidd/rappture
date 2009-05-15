
# ----------------------------------------------------------------------
#  COMPONENT: flowhints - represents a uniform rectangular 2-D mesh.
#
#  This object represents one field in an XML description of a device.
#  It simplifies the process of extracting data vectors that represent
#  the field.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itcl
package require BLT

namespace eval Rappture { 
    # empty
}

itcl::class Rappture::FlowHints {
    constructor {field cname units} { 
	# defined below 
    }
    destructor { 
	# defined below 
    }

    public method hints {}     { return [array get _hints] }
    public method particles {} { return $_particles }
    public method boxes {}     { return $_boxes }

    private method ConvertUnits { value }
    private method GetAxis { path varName }
    private method GetPosition { path varName }
    private method GetCorner { path varName }
    private method GetBoolean { path varName }

    private variable _boxes;		# List of boxes for the flow.
    private variable _particles;	# List of particle injection planes.
    private variable _hints;		# Array of settings for the flow.
    private variable _flowobj
    private variable _units
}

# ----------------------------------------------------------------------
# Constructor
# ----------------------------------------------------------------------
itcl::body Rappture::FlowHints::constructor {field cname units} {
    if {[$field element $cname.flow] == ""} {
	puts stderr "no flow entry in $cname"
	return 
    }
    array set _hints {
	"axis"		"x"
	"volume"	"on"
	"streams"	"on"
	"position"	"0.0%"
	"outline"	"on"
    }
    set _units $units
    set _flowobj [$field element -as object $cname.flow]
    set _hints(name) [$field element -as id $cname.flow]
    foreach child [$_flowobj children] {
	set value [$_flowobj get $child]
	switch -glob -- $child {
	    "outline"  { GetBoolean $child _hints(outline) }
	    "volume"   { GetBoolean $child _hints(volume) }
	    "streams"  { GetBoolean $child _hints(streams) }
	    "axis"     { GetAxis  $child _hints(axis) }
	    "position" { GetPosition $child _hints(position) }
	    "particles*" {
		array unset data
		array set data {
		    "axis"	"x"
		    "hide"	"no"
		    "position"	"0.0%"
		    "color"	"blue"
		}
		set data(name) [$_flowobj element -as id $child]
		set data(color) [$_flowobj get $child.color]
		GetAxis $child.axis data(axis)
		GetPosition $child.position data(position)
		lappend _particles [array get data]
	    }
	    "box*" {
		array unset data
		array set data {
		    "axis"	"x"
		    "hide"	"no"
		    "color"	"green"
		}
		set boxobj [$_flowobj element -as object $child]
		set count 0
		set data(name) [$_flowobj element -as id $child]
		set data(color) [$_flowobj get $child.color]
		foreach corner [$boxobj children -type corner] {
		    incr count
		    GetCorner $child.$corner data(corner$count)
		}
		itcl::delete object $boxobj
		lappend _boxes [array get data]
	    }
	}
    }
    itcl::delete  object $_flowobj
}

itcl::body Rappture::FlowHints::ConvertUnits { value } {
    set cmd Rappture::Units::convert
    set n  [scan $value "%g%s" number suffix] 
    if { $n == 2 } {
	puts stderr "number=$number suffix=$suffix"
	if { $suffix == "%" } {
	    return $value
	} else {
	    puts stderr "$cmd $number -context $suffix -to $_units -units off"
	    return [$cmd $number -context $suffix -to $_units -units off]
	}
    } elseif { [scan $value "%g" number]  == 1 } {
	if { $_units == "" } {
	    return $number
	}
	puts stderr "$cmd $number -context $_units -to $_units -units off"
	return [$cmd $number -context $_units -to $_units -units off]
    }
    return ""
}

itcl::body Rappture::FlowHints::GetPosition { path varName } {
    set value [$_flowobj get $path]
    set pos [ConvertUnits $value]
    if { $pos == "" } {
	puts stderr "can't convert units \"$value\" of \"$path\""
    }
    upvar $varName position
    set position $pos
}

itcl::body Rappture::FlowHints::GetAxis { path varName } {
    set value [$_flowobj get $path]
    set value [string tolower $value]
    switch -- $value {
	"x" - "y" - "z" {
	    upvar $varName axis
	    set axis $value
	    return
	}
    }
    puts stderr "invalid axis \"$value\" in \"$path\""
}

itcl::body Rappture::FlowHints::GetCorner { path varName } {
    set value [$_flowobj get $path]
    set coords ""
    if { [llength $value] != 3 } {
	puts stderr "wrong number of coordinates \"$value\" in \"$path\""
	return ""
    }
    foreach coord $value {
	set v [ConvertUnits $coord]
	if { $v == "" } {
	    puts stderr "can't convert units \"$value\" of \"$path\""
	    return ""
	}
	lappend coords $v
    }
    upvar $varName corner
    set corner $coords
}

itcl::body Rappture::FlowHints::GetBoolean { path varName } {
    set value [$_flowobj get $path]
    if { [string is boolean $value] } {
	upvar $varName bool
	set bool $value
	return
    }
    puts stderr "invalid boolean \"$value\" in \"$path\""
}

