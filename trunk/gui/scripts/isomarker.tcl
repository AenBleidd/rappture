# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer::isomarker - Marker for 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
#  transmits data, and displays the results.
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

itcl::class Rappture::IsoMarker {
    private variable _value     0.0;    # Absolute value of marker.
    private variable _label     ""
    private variable _tick      ""
    private variable _canvas    ""
    private variable _nvobj     "";     # Parent nanovis object.
    private variable _tf        "";     # Transfer function that this marker is
                                        # associated with.
    private variable _activeMotion   0
    private variable _activePress    0
    private common   _normalIcon [Rappture::icon nvlegendmark]
    private common   _activeIcon [Rappture::icon nvlegendmark2]
    private method EnterTick {}
    private method LeaveTick {}
    private method StartDrag { x y }
    private method ContinueDrag { x y }
    private method StopDrag { x y }

    constructor {c obj tf args} {}
    destructor {}
    public method transferfunc {}
    public method activate { bool }
    public method visible { bool }
    public method screenpos {}
    public method absval { {x "-get"} }
    public method relval { {x "-get"} }
}

itcl::body Rappture::IsoMarker::constructor {c obj tf args} {
    set _canvas $c
    set _nvobj $obj
    set _tf $tf
    set w [winfo width $_canvas]
    set h [winfo height $_canvas]
    set _tick [$c create image 0 $h \
                   -image $_normalIcon -anchor s \
                   -tags "tick $this $obj" -state hidden]
    set _label [$c create text 0 $h \
                    -anchor n -fill white -font "Helvetica 8" \
                    -tags "labels $this $obj" -state hidden]
    $c bind $_tick <Enter>           [itcl::code $this EnterTick]
    $c bind $_tick <Leave>           [itcl::code $this LeaveTick]
    $c bind $_tick <ButtonPress-1>   [itcl::code $this StartDrag %x %y]
    $c bind $_tick <B1-Motion>       [itcl::code $this ContinueDrag %x %y]
    $c bind $_tick <ButtonRelease-1> [itcl::code $this StopDrag %x %y]
}

itcl::body Rappture::IsoMarker::destructor {} {
    if { [winfo exists $_canvas] } {
        $_canvas delete $this
    }
}

itcl::body Rappture::IsoMarker::transferfunc {} {
    return $_tf
}

itcl::body Rappture::IsoMarker::activate { bool } {
    if  { $bool || $_activePress || $_activeMotion } {
        $_canvas itemconfigure $_label -state normal
        $_canvas itemconfigure $_tick -image $_activeIcon
        $_canvas itemconfigure title -state hidden
    } else {
        $_canvas itemconfigure $_label -state hidden
        $_canvas itemconfigure $_tick -image $_normalIcon
        $_canvas itemconfigure title -state normal
    }
}

itcl::body Rappture::IsoMarker::visible { bool } {
    if { $bool } {
        absval $_value
        $_canvas itemconfigure $_tick -state normal
        $_canvas raise $_tick
    } else {
        $_canvas itemconfigure $_tick -state hidden
    }
}

itcl::body Rappture::IsoMarker::screenpos { } {
    set x [relval]
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

itcl::body Rappture::IsoMarker::absval { {x "-get"} } {
    if { $x != "-get" } {
        set _value $x
        set y 31
        $_canvas itemconfigure $_label -text [format %g $_value]
        set x [screenpos]
        $_canvas coords $_tick $x [expr {$y+3}]
        $_canvas coords $_label $x [expr {$y+5}]
    }
    return $_value
}

itcl::body Rappture::IsoMarker::relval { {x "-get"} } {
    foreach {min max} [$_nvobj limits $_tf] break
    if { $x == "-get" } {
        if { $max == $min } {
            if { $max == 0.0 } {
                set min 0.0
                set max 1.0
            } else {
                set max [expr $min + 1.0]
            }
        }
        return [expr {($_value - $min) / ($max - $min)}]
    }
    if { $max == $min } {
        set min 0.0
        set max 1.0
    }
    if { [catch {expr $max - $min} r] != 0 } {
        return 0.0
    }
    absval [expr {($x * $r) + $min}]
}

itcl::body Rappture::IsoMarker::EnterTick {} {
    set _activeMotion 1
    activate yes
    $_canvas raise $_tick
}

itcl::body Rappture::IsoMarker::LeaveTick {} {
    set _activeMotion 0
    activate no
}

itcl::body Rappture::IsoMarker::StartDrag { x y } {
    $_canvas raise $_tick
    set _activePress 1
    activate yes
    $_canvas itemconfigure limits -state hidden
    $_canvas itemconfigure title -state hidden
}

itcl::body Rappture::IsoMarker::StopDrag { x y } {
    if { ![$_nvobj removeDuplicateMarker $this $x]} {
        ContinueDrag $x $y
    }
    set _activePress 0
    activate no
    $_canvas itemconfigure limits -state normal
    $_canvas itemconfigure title -state normal
}

itcl::body Rappture::IsoMarker::ContinueDrag { x y } {
    set w [winfo width $_canvas]
    relval [expr {double($x-10)/($w-20)}]
    $_nvobj overMarker $this $x
    $_nvobj updateTransferFunctions
    $_canvas raise $_tick
    set _activePress 1
    activate yes
    $_canvas itemconfigure limits -state hidden
    $_canvas itemconfigure title -state hidden
}
