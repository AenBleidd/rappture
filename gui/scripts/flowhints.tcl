
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
    private method GetAxis { obj path varName }
    private method GetPosition { obj path varName }
    private method GetCorner { obj path varName }
    private method GetBoolean { obj path varName }

    private variable _boxes "";		# List of boxes for the flow.
    private variable _particles "";	# List of particle injection planes.
    private variable _hints;		# Array of settings for the flow.
    private variable _units ""
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
    set f [$field element -as object $cname.flow]
    set _hints(name) [$field element -as id $cname.flow]
    foreach child [$f children] {
	set value [$f get $child]
	switch -glob -- $child {
	    "label"    { set _hints(label) [$f get $child] }
	    "outline"  { GetBoolean $f $child _hints(outline) }
	    "volume"   { GetBoolean $f $child _hints(volume) }
	    "streams"  { GetBoolean $f $child _hints(streams) }
	    "axis"     { GetAxis $f  $child _hints(axis) }
	    "position" { GetPosition $f $child _hints(position) }
	    "particles*" {
		array unset data
		array set data {
		    "axis"	"x"
		    "hide"	"no"
		    "position"	"0.0%"
		    "color"	"blue"
		    "label"     ""
		}
		set p [$f element -as object $child]
		set data(name) [$f element -as id $child]
		foreach child [$p children] {
		    set value [$p get $child]
		    switch -exact -- $child {
			"label" { set data(label) $value }
			"color" { set data(color) $value }
			"hide"  { GetBoolean $p hide data(hide) }
			"axis"  { GetAxis $p axis data(axis) }
			"position" { GetPosition $p position data(position)}
		    }
		}
		if { $data(label) == "" } {
		    set data(label) $data(name)
		}
		itcl::delete object $p
		lappend _particles [array get data]
	    }
	    "box*" {
		array unset data
		array set data {
		    "hide"	"no"
		    "color"	"green"
		    "label"	""
		}
		set b [$f element -as object $child]
		set name [$f element -as id $child]
		set count 0
		set data(name) $name
		set data(color) [$f get $child.color]
		foreach child [$b children] {
		    set value [$b get $child]
		    switch -glob -- $child {
			"label" { set data(label) $value }
			"color" { set data(color) $value }
			"hide"  { GetBoolean $b hide data(hide) }
			"corner*" { 
			    incr count
			    GetCorner $b $child data(corner$count)
			}
		    }
		}
		if { $data(label) == "" } {
		    set data(label) $data(name)
		}
		itcl::delete object $b
		lappend _boxes [array get data]
	    }
	}
    }
    itcl::delete  object $f
}

itcl::body Rappture::FlowHints::ConvertUnits { value } {
    set cmd Rappture::Units::convert
    set n  [scan $value "%g%s" number suffix] 
    if { $n == 2 } {
	if { $suffix == "%" } {
	    return $value
	} else {
	    return [$cmd $number -context $suffix -to $_units -units off]
	}
    } elseif { [scan $value "%g" number]  == 1 } {
	if { $_units == "" } {
	    return $number
	}
	return [$cmd $number -context $_units -to $_units -units off]
    }
    return ""
}

itcl::body Rappture::FlowHints::GetPosition { obj path varName } {
    set value [$obj get $path]
    set pos [ConvertUnits $value]
    if { $pos == "" } {
	puts stderr "can't convert units \"$value\" of \"$path\""
    }
    upvar $varName position
    set position $pos
}

itcl::body Rappture::FlowHints::GetAxis { obj path varName } {
    set value [$obj get $path]
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

itcl::body Rappture::FlowHints::GetCorner { obj path varName } {
    set value [$obj get $path]
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

itcl::body Rappture::FlowHints::GetBoolean { obj path varName } {
    set value [$obj get $path]
    if { [string is boolean $value] } {
	upvar $varName bool
	set bool $value
	return
    }
    puts stderr "invalid boolean \"$value\" in \"$path\""
}

