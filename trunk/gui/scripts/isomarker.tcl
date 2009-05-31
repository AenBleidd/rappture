
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

itcl::class Rappture::IsoMarker {
    private variable value_	"";	# Absolute value of marker.
    private variable label_	""
    private variable tick_	""
    private variable canvas_	""
    private variable nvobj_	"";	# Parent nanovis object.
    private variable tf_	"";	# Transfer function that this marker is 
					# associated with.
    private variable active_motion_   0
    private variable active_press_    0
    private common   normalIcon_ [Rappture::icon nvlegendmark]
    private common   activeIcon_ [Rappture::icon nvlegendmark2]

    constructor {c obj tf args} { 
	set canvas_ $c
	set nvobj_ $obj
	set tf_ $tf
	set w [winfo width $canvas_]
	set h [winfo height $canvas_]
	set tick_ [$c create image 0 $h \
		-image $normalIcon_ -anchor s \
		-tags "$this $obj" -state hidden]
	set label_ [$c create text 0 $h \
		-anchor n -fill white -font "Helvetica 8" \
		-tags "$this $obj" -state hidden]
	$c bind $tick_ <Enter> [itcl::code $this HandleEvent "enter"]
	$c bind $tick_ <Leave> [itcl::code $this HandleEvent "leave"]
	$c bind $tick_ <ButtonPress-1> \
	    [itcl::code $this HandleEvent "start" %x %y]
	$c bind $tick_ <B1-Motion> \
	    [itcl::code $this HandleEvent "update" %x %y]
	$c bind $tick_ <ButtonRelease-1> \
	    [itcl::code $this HandleEvent "end" %x %y]
    }
    destructor { 
	$canvas_ delete $this
    }
    public method transferfunc {} {
	return $tf_
    }
    public method activate { bool } {
	if  { $bool || $active_press_ || $active_motion_ } {
	    $canvas_ itemconfigure $label_ -state normal
	    $canvas_ itemconfigure $tick_ -image $activeIcon_
	} else {
	    $canvas_ itemconfigure $label_ -state hidden
	    $canvas_ itemconfigure $tick_ -image $normalIcon_
	}
    }
    public method visible { bool } {
	if { $bool } {
	    absval $value_
	    $canvas_ itemconfigure $tick_ -state normal
	    $canvas_ raise $tick_
	} else {
	    $canvas_ itemconfigure $tick_ -state hidden
	}
    }
    public method screenpos { } { 
	set x [relval]
	if { $x < 0.0 } {
	    set x 0.0
	} elseif { $x > 1.0 } {
	    set x 1.0 
	}
	set low 10 
	set w [winfo width $canvas_]
	set high [expr {$w  - 10}]
	set x [expr {round($x*($high - $low) + $low)}]
	return $x
    }
    public method absval { {x "-get"} } {
	if { $x != "-get" } {
	    set value_ $x
	    set y 31
	    $canvas_ itemconfigure $label_ -text [format %.2g $value_]
	    set x [screenpos]
	    $canvas_ coords $tick_ $x [expr {$y+3}]
	    $canvas_ coords $label_ $x [expr {$y+5}]
	}
	return $value_
    }
    public method relval  { {x "-get"} } {
	if { $x == "-get" } {
	    array set limits [$nvobj_ limits $tf_] 
	    if { $limits(vmax) == $limits(vmin) } {
		if { $limits(vmax) == 0.0 } {
		    set limits(vmin) 0.0
		    set limits(vmax) 1.0
		} else {
		    set limits(vmax) [expr $limits(vmin) + 1.0]
		}
	    }
	    return [expr {($value_-$limits(vmin))/
			  ($limits(vmax) - $limits(vmin))}]
	} 
	array set limits [$nvobj_ limits $tf_] 
	if { $limits(vmax) == $limits(vmin) } {
	    set limits(min) 0.0
	    set limits(max) 1.0
	}
	set r [expr $limits(vmax) - $limits(vmin)]
	absval [expr {($x * $r) + $limits(vmin)}]
    }
    private method HandleEvent { option args } {
	switch -- $option {
	    enter {
		set active_motion_ 1
		activate yes
		$canvas_ raise $tick_
	    }
	    leave {
		set active_motion_ 0
		activate no
	    }
	    start {
		$canvas_ raise $tick_ 
		set active_press_ 1
		activate yes
		$canvas_ itemconfigure limits -state hidden
	    }
	    update {
		set w [winfo width $canvas_]
		set x [lindex $args 0]
		relval [expr {double($x-10)/($w-20)}]
		$nvobj_ overmarker $this $x
		$nvobj_ updatetransferfuncs
	    }
	    end {
		set x [lindex $args 0]
		if { ![$nvobj_ rmdupmarker $this $x]} {
		    eval HandleEvent update $args
		}
		set active_press_ 0
		activate no
		$canvas_ itemconfigure limits -state normal
	    }
	    default {
		error "bad option \"$option\": should be start, update, end"
	    }
	}
    }
}
