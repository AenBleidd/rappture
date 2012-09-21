
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

    constructor {c obj tf args} { 
        set _canvas $c
        set _nvobj $obj
        set _tf $tf
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
    public method transferfunc {} {
        return $_tf
    }
    public method activate { bool } {
        if  { $bool || $_activePress || $_activeMotion } {
            $_canvas itemconfigure $_label -state normal
            $_canvas itemconfigure $_tick -image $_activeIcon
        } else {
            $_canvas itemconfigure $_label -state hidden
            $_canvas itemconfigure $_tick -image $_normalIcon
        }
    }
    public method visible { bool } {
        if { $bool } {
            absval $_value
            $_canvas itemconfigure $_tick -state normal
            $_canvas raise $_tick
        } else {
            $_canvas itemconfigure $_tick -state hidden
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
        set w [winfo width $_canvas]
        set high [expr {$w  - 10}]
        set x [expr {round($x*($high - $low) + $low)}]
        return $x
    }
    public method absval { {x "-get"} } {
        if { $x != "-get" } {
            set _value $x
            set y 31
            $_canvas itemconfigure $_label -text [format %.2g $_value]
            set x [screenpos]
            $_canvas coords $_tick $x [expr {$y+3}]
            $_canvas coords $_label $x [expr {$y+5}]
        }
        return $_value
    }
    public method relval  { {x "-get"} } {
        if { $x == "-get" } {
            array set limits [$_nvobj limits $_tf] 
            if { $limits(vmax) == $limits(vmin) } {
                if { $limits(vmax) == 0.0 } {
                    set limits(vmin) 0.0
                    set limits(vmax) 1.0
                } else {
                    set limits(vmax) [expr $limits(vmin) + 1.0]
                }
            }
            return [expr {($_value-$limits(vmin))/
                          ($limits(vmax) - $limits(vmin))}]
        } 
        array set limits [$_nvobj limits $_tf] 
        if { $limits(vmax) == $limits(vmin) } {
            set limits(min) 0.0
            set limits(max) 1.0
        }
        if { [catch {expr $limits(vmax) - $limits(vmin)} r] != 0 } {
            return 0.0
        }           
        absval [expr {($x * $r) + $limits(vmin)}]
    }
    private method HandleEvent { option args } {
        switch -- $option {
            enter {
                set _activeMotion 1
                activate yes
                $_canvas raise $_tick
            }
            leave {
                set _activeMotion 0
                activate no
            }
            start {
                $_canvas raise $_tick 
                set _activePress 1
                activate yes
                $_canvas itemconfigure limits -state hidden
            }
            update {
                set w [winfo width $_canvas]
                set x [lindex $args 0]
                relval [expr {double($x-10)/($w-20)}]
                $_nvobj overmarker $this $x
                $_nvobj updatetransferfuncs
            }
            end {
                set x [lindex $args 0]
                if { ![$_nvobj rmdupmarker $this $x]} {
                    eval HandleEvent update $args
                }
                set _activePress 0
                activate no
                $_canvas itemconfigure limits -state normal
            }
            default {
                error "bad option \"$option\": should be start, update, end"
            }
        }
    }
}
