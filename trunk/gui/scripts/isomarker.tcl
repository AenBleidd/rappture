
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
    private variable _ivol	"";	# Transfer function that this marker is 
					# associated with.
    private variable _active_motion   0
    private variable _active_press    0
    private common   _normalIcon [Rappture::icon nvlegendmark]
    private common   _activeIcon [Rappture::icon nvlegendmark2]

    constructor {c obj ivol args} { 
	set _canvas $c
	set _nvobj $obj
	set _ivol $ivol
	set w [winfo width $_canvas]
	set h [winfo height $_canvas]
	set _tick [$c create image 0 $h \
		-image $_normalIcon -anchor s \
		-tags "$this $obj" -state hidden]
	set _label [$c create text 0 $h \
		-anchor n -fill white -font "Helvetica 8" \
		-tags "$this $obj" -state hidden]
	$c bind $_tick <Enter> [itcl::code $this HandleEvent "enter"]
	$c bind $_tick <Leave> [itcl::code $this HandleEvent "leave"]
	$c bind $_tick <ButtonPress-1> \
	    [itcl::code $this HandleEvent "start" %x %y]
	$c bind $_tick <B1-Motion> \
	    [itcl::code $this HandleEvent "update" %x %y]
	$c bind $_tick <ButtonRelease-1> \
	    [itcl::code $this HandleEvent "end" %x %y]
    }
    destructor { 
	$_canvas delete $this
    }
    public method GetVolume {} {
	return $_ivol
    }
    public method GetAbsoluteValue {} {
	return $_value
    }
    public method GetRelativeValue {} {
	array set limits [$_nvobj GetLimits $_ivol] 
	if { $limits(max) == $limits(min) } {
	    if { $limits(max) == 0.0 } {
		set limits(min) 0.0
		set limits(max) 1.0
	    } else {
		set limits(max) [expr $limits(min) + 1.0]
	    }
	}
	return [expr {($_value-$limits(min))/($limits(max) - $limits(min))}]
    }
    public method Activate { bool } {
	if  { $bool || $_active_press || $_active_motion } {
	    $_canvas itemconfigure $_label -state normal
	    $_canvas itemconfigure $_tick -image $_activeIcon
	} else {
	    $_canvas itemconfigure $_label -state hidden
	    $_canvas itemconfigure $_tick -image $_normalIcon
	}
    }
    public method Show {} {
	SetAbsoluteValue $_value
	$_canvas itemconfigure $_tick -state normal
	$_canvas raise $_tick
    }
    public method Hide {} {
	$_canvas itemconfigure $_tick -state hidden
    }
    public method GetScreenPosition { } { 
	set x [GetRelativeValue]
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
    public method SetAbsoluteValue { x } {
	set _value $x
	set y 31
	$_canvas itemconfigure $_label -text [format %.2g $_value]
	set x [GetScreenPosition]
	$_canvas coords $_tick $x [expr {$y+3}]
	$_canvas coords $_label $x [expr {$y+5}]
    }
    public method SetRelativeValue { x } {
	array set limits [$_nvobj GetLimits $_ivol] 
	if { $limits(max) == $limits(min) } {
	    set limits(min) 0.0
	    set limits(max) 1.0
	}
	set r [expr $limits(max) - $limits(min)]
	SetAbsoluteValue [expr {($x * $r) + $limits(min)}]
    }
    public method HandleEvent { option args } {
	switch -- $option {
	    enter {
		set _active_motion 1
		Activate yes
		$_canvas raise $_tick
	    }
	    leave {
		set _active_motion 0
		Activate no
	    }
	    start {
		$_canvas raise $_tick 
		set _active_press 1
		Activate yes
		$_canvas itemconfigure limits -state hidden
	    }
	    update {
		set w [winfo width $_canvas]
		set x [lindex $args 0]
		SetRelativeValue [expr {double($x-10)/($w-20)}]
		$_nvobj OverIsoMarker $this $x
		$_nvobj UpdateTransferFunctions
	    }
	    end {
		set x [lindex $args 0]
		if { ![$_nvobj RemoveDuplicateIsoMarker $this $x]} {
		    eval HandleEvent update $args
		}
		set _active_press 0
		Activate no
		$_canvas itemconfigure limits -state normal
	    }
	    default {
		error "bad option \"$option\": should be start, update, end"
	    }
	}
    }
}
