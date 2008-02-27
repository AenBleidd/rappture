# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer::isomarker - Marker for 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

itcl::class Rappture::NanovisViewer::IsoMarker {
    private variable _value	"";	# Absolute value of marker.
    private variable _label	""
    private variable _tick	""
    private variable _canvas	""
    private variable _nvobj	"";	# Parent nanovis object.
    private variable _active_motion   0
    private variable _active_press    0
    private common   _normalIcon [Rappture::icon nvlegendmark]
    private common   _activeIcon [Rappture::icon nvlegendmark2]

    constructor {c obj args} { 
	puts stderr "call IsoMarker constructor"
	set _canvas $c
	set _nvobj $obj
	
	set w [winfo width $_canvas]
	set h [winfo height $_canvas]
	set _tick [$c create image 0 $h \
		-image $_normalIcon -anchor s \
		-tags "$this $obj" -state hidden]
	set _label [$c create text 0 $h \
		-anchor n -fill white -font "Helvetica 6" \
		-tags "$this $obj" -state hidden]
	$c bind $_tick <Enter> [itcl::code $this handle_event "enter"]
	$c bind $_tick <Leave> [itcl::code $this handle_event "leave"]
	$c bind $_tick <ButtonPress-1> \
	    [itcl::code $this handle_event "start" %x %y]
	$c bind $_tick <B1-Motion> \
	    [itcl::code $this handle_event "update" %x %y]
	$c bind $_tick <ButtonRelease-1> \
	    [itcl::code $this handle_event "end" %x %y]
    }
    destructor { 
	$_canvas delete $this
    }

    public method get_absolute_value {} {
	return $_value
    }
    public method get_relative_value {} {
	array set limits [$_nvobj get_limits] 
	if { $limits(vmax) == $limits(vmin) } {
	    set limits(vmin) 0.0
	    set limits(vmax) 1.0
	}
	return [expr {($_value-$limits(vmin))/($limits(vmax) - $limits(vmin))}]
    }
    public method activate { bool } {
	if  { $bool || $_active_press || $_active_motion } {
	    $_canvas itemconfigure $_label -state normal
	    $_canvas itemconfigure $_tick -image $_activeIcon
	} else {
	    $_canvas itemconfigure $_label -state hidden
	    $_canvas itemconfigure $_tick -image $_normalIcon
	}
    }
    public method show {} {
	set_absolute_value $_value
	$_canvas itemconfigure $_tick -state normal
	$_canvas raise $_tick
    }
    public method hide {} {
	$_canvas itemconfigure $_tick -state hidden
    }
    public method get_screen_position { } { 
	set x [get_relative_value]
	if { $x < 0.0 } {
	    set x 0.0
	} elseif { $x > 1.0 } {
	    set x 1.0 
	}
	set low 10 
	set w [winfo width $_canvas]
	set high [expr {$w  - 10}]
	set x [expr {round($x*($high - $low) + $low)}]
	return $x
    }
    public method set_absolute_value { x } {
	set _value $x
	set y 31
	$_canvas itemconfigure $_label -text [format %.4g $_value]
	set x [get_screen_position]
	$_canvas coords $_tick $x [expr {$y+3}]
	$_canvas coords $_label $x [expr {$y+5}]
    }
    public method set_relative_value { x } {
	array set limits [$_nvobj get_limits] 
	if { $limits(vmax) == $limits(vmin) } {
	    set limits(vmin) 0.0
	    set limits(vmax) 1.0
	}
	set r [expr $limits(vmax) - $limits(vmin)]
	set_absolute_value [expr {($x * $r) + $limits(vmin)}]
    }
    public method handle_event { option args } {
	switch -- $option {
	    enter {
		set _active_motion 1
		activate yes
		$_canvas raise $_tick
	    }
	    leave {
		set _active_motion 0
		activate no
	    }
	    start {
		$_canvas raise $_tick 
		set _active_press 1
		activate yes
	    }
	    update {
		set w [winfo width $_canvas]
		set x [lindex $args 0]
		set_relative_value [expr {double($x-10)/($w-20)}]
		$_nvobj OverIsoMarker $this $x
		$_nvobj UpdateTransferFunction
	    }
	    end {
		set x [lindex $args 0]
		if { ![$_nvobj RemoveDuplicateIsoMarker $this $x]} {
		    eval handle_event update $args
		}
		set _active_press 0
		activate no
	    }
	    default {
		error "bad option \"$option\": should be start, update, end"
	    }
	}
    }
}

