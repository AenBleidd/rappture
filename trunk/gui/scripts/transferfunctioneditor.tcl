# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: transferfunctioneditor - Rudimentary editor for 3D volume 
#                                      transfer functions
#
#  This class is used to modify transfer functions used in volume rendering 
#  on 3D scalar/vector datasets.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

itcl::class Rappture::TransferFunctionEditor {
    private variable _nextId 0;         # Used to create unique marker names
    private variable _values;           # Relative values for each marker.
    private variable _limits;           # Over limits of transfer function.
    private variable _labels;           # Label id for each marker.
    private variable _ticks;            # Tick id for each marker.
    private variable _canvas    ""
    private variable _name "";		# Name of transfer function.
    private variable _activeMotion   0
    private variable _activePress    0
    private variable _id2name
    private common   _normalIcon [Rappture::icon nvlegendmark]
    private common   _activeIcon [Rappture::icon nvlegendmark2]
    public variable command ""

    private method SetAbsoluteValue { name x } 
    private method GetAbsoluteValue { name } 
    private method ContinueDrag { name x y } 
    private method EnterTick { name } 
    private method GetOverlappingMarkers { x y } 
    private method GetScreenPosition { name } 
    private method LeaveTick { name } 
    private method SetRelativeValue  { name x }
    private method GetRelativeValue  { name }
    private method RemoveDuplicateMarkers {name x y} 
    private method SetScreenPosition { name } 
    private method SetVisibility { name bool } 
    private method StartDrag { name x y } 
    private method StopDrag { name x y } 
    private method Activate { name } 
    private method Deactivate { name } 
    private method UpdateViewer {} 

    constructor {c name args} {}
    destructor {} 
    public method limits { min max }
    public method names {}
    public method name {}
    public method values {}
    public method absoluteValues {}
    public method removeMarkers { list }
    public method addMarkers { values }
    public method newMarker { x y state }
    public method deleteMarker { x y }
    public method hideMarkers { {list {}} }
    public method showMarkers { {limits {}} }
}

itcl::body Rappture::TransferFunctionEditor::constructor {c name args} { 
    set _canvas $c
    set _name $name
    set _limits [list 0.0 1.0] 
    eval configure $args
}

itcl::body Rappture::TransferFunctionEditor::limits { min max } { 
    set _limits [list $min $max]
}

itcl::body Rappture::TransferFunctionEditor::names {} { 
    return [lsort [array names _values]]
}

itcl::body Rappture::TransferFunctionEditor::values {} { 
    set list {}
    foreach name [array names _ticks] {
	lappend list [GetRelativeValue $name]
    }
    return [lsort -real $list]
}

itcl::body Rappture::TransferFunctionEditor::absoluteValues {} { 
    set list {}
    foreach name [array names _values] {
	lappend list [GetAbsoluteValue $name]
    }
    return $list
}

itcl::body Rappture::TransferFunctionEditor::deleteMarker { x y } { 
    foreach marker [GetOverlappingMarkers $x $y] {
        $_canvas delete $_ticks($marker)
        $_canvas delete $_labels($marker)
        array unset _ticks $marker
        array unset _labels $marker
        array unset _values $marker
        bell
        UpdateViewer
    }
}

itcl::body Rappture::TransferFunctionEditor::newMarker { x y state } { 
    foreach id [$_canvas find overlapping \
                    [expr $x-5] [expr $y-5] [expr $x+5] [expr $y+5]] {
	if { [info exists _id2name($id)] } {
            puts stderr "Too close to existing marker"
	    return;                     # Too close to existing marker
	}
    }
    set name "tick[incr _nextId]"
    set w [winfo width $_canvas]
    set h [winfo height $_canvas]

    set _ticks($name) [$_canvas create image 0 $h \
                           -image $_normalIcon -anchor s \
                           -tags "tick $_name $this" -state $state]
    set _labels($name) [$_canvas create text 0 $h \
                            -anchor n -fill white -font "Helvetica 8" \
                            -tags "labels text $this $_name" -state $state]
    set _id2name($_ticks($name)) $name
    $_canvas bind $_ticks($name) <Enter> [itcl::code $this EnterTick $name]
    $_canvas bind $_ticks($name) <Leave> [itcl::code $this LeaveTick $name]
    $_canvas bind $_ticks($name) <ButtonPress-1> \
	[itcl::code $this StartDrag $name %x %y]
    $_canvas bind $_ticks($name) <B1-Motion> \
	[itcl::code $this ContinueDrag $name %x %y]
    $_canvas bind $_ticks($name) <ButtonRelease-1> \
	[itcl::code $this StopDrag $name %x %y]

    SetRelativeValue $name [expr {double($x-10)/($w-20)}]
    if { $state == "normal" } {
        UpdateViewer
    }
    return $name
}

itcl::body Rappture::TransferFunctionEditor::destructor {} {
    if { [winfo exists $_canvas] } {
        $_canvas delete $_name
    }
}

itcl::body Rappture::TransferFunctionEditor::name {} {
    return $_name
}

itcl::body Rappture::TransferFunctionEditor::Activate { name } {
    if  { $_activePress || $_activeMotion } {
        $_canvas itemconfigure $_labels($name) -state normal
        $_canvas itemconfigure $_ticks($name) -image $_activeIcon
        $_canvas itemconfigure title -state hidden
    }
}

itcl::body Rappture::TransferFunctionEditor::Deactivate { name } {
    if  { $_activePress || $_activeMotion } {
        #puts stderr "do nothing for Deactivate"
    } else {
        $_canvas itemconfigure $_labels($name) -state hidden
        $_canvas itemconfigure $_ticks($name) -image $_normalIcon
        $_canvas itemconfigure title -state normal
    }
}

itcl::body Rappture::TransferFunctionEditor::SetVisibility { name bool } {
    if { $bool } {
        $_canvas itemconfigure $_ticks($name) -state normal
        $_canvas raise $_ticks($name)
    } else {
        $_canvas itemconfigure $_ticks($name) -state hidden
    }
}

itcl::body Rappture::TransferFunctionEditor::GetScreenPosition { name } { 
    set x [GetRelativeValue $name]
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

itcl::body Rappture::TransferFunctionEditor::SetScreenPosition { name } { 
    set value $_values($name)
    set x [GetScreenPosition $name]
    set absval [GetAbsoluteValue $name]
    set y 31
    $_canvas itemconfigure $_labels($name) -text [format %g $absval]
    $_canvas coords $_ticks($name) $x [expr {$y+3}]
    $_canvas coords $_labels($name) $x [expr {$y+5}]
}

itcl::body Rappture::TransferFunctionEditor::GetAbsoluteValue { name } {
    foreach {min max} $_limits break
    return [expr {($_values($name) * ($max - $min)) + $min}]
}

itcl::body Rappture::TransferFunctionEditor::SetAbsoluteValue { name value } {
    foreach {min max} $_limits break
    set absval $value
    set relval [expr {($absval - $min) / ($max - $min)}]
    set _values($name) $relval
    set y 31
    $_canvas itemconfigure $_label -text [format %g $absval]
    set x [GetScreenPosition $name]
    $_canvas coords $_ticks($name) $x [expr {$y+3}]
    $_canvas coords $_labels($name) $x [expr {$y+5}]
    return $absval
}

itcl::body Rappture::TransferFunctionEditor::GetRelativeValue  { name } {
    return $_values($name)
}

itcl::body Rappture::TransferFunctionEditor::SetRelativeValue  { name value } {
    set _values($name) $value
    set x [GetScreenPosition $name]
    set y 31
    set absval [GetAbsoluteValue $name]
    $_canvas itemconfigure $_labels($name) -text [format %g $absval]
    $_canvas coords $_ticks($name) $x [expr {$y+3}]
    $_canvas coords $_labels($name) $x [expr {$y+5}]
}

itcl::body Rappture::TransferFunctionEditor::EnterTick { name } {
    set _activeMotion 1
    Activate $name
    $_canvas raise $_ticks($name)
}

itcl::body Rappture::TransferFunctionEditor::LeaveTick { name } {
    set _activeMotion 0
    Deactivate $name
}

itcl::body Rappture::TransferFunctionEditor::StartDrag { name x y } {
    $_canvas raise $_ticks($name) 
    set _activePress 1
    Activate $name
    $_canvas itemconfigure limits -state hidden
    $_canvas itemconfigure title -state hidden
}

itcl::body Rappture::TransferFunctionEditor::StopDrag { name x y } {
    ContinueDrag $name $x $y 
    RemoveDuplicateMarkers $name $x $y
    set _activePress 0
    Deactivate $name
    $_canvas itemconfigure limits -state normal
    $_canvas itemconfigure title -state normal
}

itcl::body Rappture::TransferFunctionEditor::ContinueDrag { name x y } {
    set w [winfo width $_canvas]
    SetRelativeValue $name [expr {double($x-10)/($w-20)}]
    UpdateViewer
    $_canvas raise $_ticks($name)
    set _activePress 1
    Activate $name
    $_canvas itemconfigure limits -state hidden
    $_canvas itemconfigure title -state hidden
}

itcl::body Rappture::TransferFunctionEditor::GetOverlappingMarkers { x y } {
    set list {}
    foreach id [$_canvas find overlapping $x $y $x $y] {
	if { [info exists _id2name($id)] } {
	    lappend list $_id2name($id)
	}
    }
    return $list
}

itcl::body Rappture::TransferFunctionEditor::hideMarkers { {list {}} } {
    if { $list == "" } {
	set list [array names _values]
    }
    foreach name $list {
	SetVisibility $name 0
    }
}

itcl::body Rappture::TransferFunctionEditor::showMarkers { {limits {}} } {
    if { $limits != "" } {
	set _limits $limits
	foreach name [array names _values] {
	    SetScreenPosition $name 
	}
    }
    foreach name [array names _values] {
	SetVisibility $name 1
    }
}

itcl::body Rappture::TransferFunctionEditor::RemoveDuplicateMarkers {name x y} {
    foreach marker [GetOverlappingMarkers $x $y] {
	if { $marker != $name } {
            Activate $marker 
            set markerx [GetScreenPosition $marker]
            if { ($x < ($markerx-3)) || ($x > ($markerx+3)) } {
                continue
            }
	    $_canvas delete $_ticks($marker)
	    $_canvas delete $_labels($marker)
	    array unset _ticks $marker
	    array unset _labels $marker
	    array unset _values $marker
            bell
	}
    }
}

itcl::body Rappture::TransferFunctionEditor::addMarkers { values } {
    foreach value $values {
	set name [newMarker 0 0 hidden]
	SetRelativeValue $name $value
    }
}

itcl::body Rappture::TransferFunctionEditor::removeMarkers { names } {
    if { $names == "" } {
	set names [array names _values]
    }
    foreach name $names {
	$_canvas delete $_ticks($name)
	$_canvas delete $_labels($name)
	array unset _ticks $name
	array unset _labels $name
	array unset _values $name
    }
    UpdateViewer
}

itcl::body Rappture::TransferFunctionEditor::UpdateViewer {} {
    # Tell the nanovis/flowvis client to update its transfer functions
    # now that a marker position has changed.
    if { $command != "" } {
	eval uplevel \#0 $command
    }
}
