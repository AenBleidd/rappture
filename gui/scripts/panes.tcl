# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Panes - creates a series of adjustable panes
#
#  This is a simple paned window with an adjustable sash.
#  the same quantity, but for various ranges of input values.
#  It also manages the controls to select and visualize the data.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Panes.width 3i widgetDefault
option add *Panes.height 3i widgetDefault
option add *Panes.sashRelief sunken widgetDefault
option add *Panes.sashWidth 2 widgetDefault
option add *Panes.sashPadding 4 widgetDefault
option add *Panes.orientation vertical widgetDefault

itcl::class Rappture::Panes {
    inherit itk::Widget

    itk_option define -sashrelief sashRelief SashRelief ""
    itk_option define -sashwidth sashWidth SashWidth 0
    itk_option define -sashpadding sashPadding SashPadding 0
    itk_option define -orientation orientation Orientation ""

    constructor {args} { # defined below }

    public method insert {pos args}
    public method pane {pos}
    public method visibility {pos args}
    public method fraction {pos args}
    public method hilite {state sash}
    public method size {}

    protected method _grab {pane X Y}
    protected method _drag {pane X Y}
    protected method _drop {pane X Y}
    protected method _fixLayout {args}
    protected method _fixSashes {args}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _panes ""       ;# list of pane frames
    private variable _visibility ""  ;# list of visibilities for panes
    private variable _counter 0      ;# counter for auto-generated names
    private variable _reqfrac 0.0    ;# requested fraction size of each pane
    private variable _dragfrom 0     ;# starting coordinate of drag operation
    private variable _dragfrac 0     ;# limit on fraction of drag operation
}

itk::usual Panes {
    keep -background -cursor -sashwidth -sashrelief
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::constructor {args} {
    itk_option add hull.width hull.height

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout [itcl::code $this _fixLayout]
    $_dispatcher register !sashes
    $_dispatcher dispatch $this !sashes [itcl::code $this _fixSashes]

    # fix the layout whenever the window size changes
    bind Panes <Configure> [itcl::code %W _fixLayout]

    set pname "pane[incr _counter]"
    itk_component add $pname {
        frame $itk_interior.$pname
    }

    lappend _panes $pname
    lappend _visibility 1
    set _reqfrac 0.5

    eval itk_initialize $args

    # make sure we fix up the layout at some point
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?-fraction f?
#
# Adds a new page to this widget at the given position <pos>.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::insert {pos args} {
    Rappture::getopts args params {
        value -fraction 0.5
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"insert pos ?-fraction f?\""
    }

    set pname "pane[incr _counter]"
    set sash "${pname}sash"
    itk_component add $sash {
        frame $itk_interior.$sash
    } {
        usual
        ignore -cursor
    }
    bind $itk_component($sash) <Enter> [itcl::code $this hilite on $sash]
    bind $itk_component($sash) <Leave> [itcl::code $this hilite off $sash]

    itk_component add ${sash}ridge {
        frame $itk_component($sash).ridge
    } {
        usual
        rename -relief -sashrelief sashRelief SashRelief
        ignore -borderwidth
    }
    if {$itk_option(-orientation) eq "vertical"} {
        pack $itk_component(${sash}ridge) -fill x
        $itk_component($sash) configure -cursor sb_v_double_arrow
        $itk_component(${sash}ridge) configure -cursor sb_v_double_arrow
    } else {
        pack $itk_component(${sash}ridge) -fill y -side left
        $itk_component($sash) configure -cursor sb_h_double_arrow
        $itk_component(${sash}ridge) configure -cursor sb_h_double_arrow
    }
    foreach comp [list $sash ${sash}ridge] {
        bind $itk_component($comp) <ButtonPress-1> \
            [itcl::code $this _grab $pname %X %Y]
        bind $itk_component($comp) <B1-Motion> \
            [itcl::code $this _drag $pname %X %Y]
        bind $itk_component($comp) <ButtonRelease-1> \
            [itcl::code $this _drop $pname %X %Y]
    }


    itk_component add $pname {
        frame $itk_interior.$pname
    }
    set _panes [linsert $_panes $pos $pname]
    set _visibility [linsert $_visibility $pos 1]
    set _reqfrac [linsert $_reqfrac $pos $params(-fraction)]

    # fix sash characteristics
    $_dispatcher event -idle !sashes

    # make sure we fix up the layout at some point
    $_dispatcher event -idle !layout

    return $itk_component($pname)
}

# ----------------------------------------------------------------------
# USAGE: pane <pos>
#
# Returns the frame representing the pane at position <pos>.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::pane {pos} {
    set pname [lindex $_panes $pos]
    if {[info exists itk_component($pname)]} {
        return $itk_component($pname)
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: visibility <pos> ?<newval>? ?<pos> <newval> ...?
#
# Clients use this to get/set the visibility of the pane at position
# <pos>.  Can also be used to set the visibility for multiple panes
# if multiple <pos>/<newval> pairs are specified in the same command.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::visibility {pos args} {
    if {[llength $args] == 0} {
        return [lindex $_visibility $pos]
    }
    if {[llength $args] % 2 == 0} {
        error "wrong # args: should be \"visibility pos ?val pos val ...?\""
    }

    set args [linsert $args 0 $pos]
    foreach {pos newval} $args {
        if {![string is boolean -strict $newval]} {
            error "bad value \"$newval\": should be boolean"
        }
        if {$pos eq "end" || ($pos >= 0 && $pos < [llength $_visibility])} {
            set _visibility [lreplace $_visibility $pos $pos [expr {$newval}]]
            $_dispatcher event -idle !layout
        } else {
            error "bad index \"$pos\": out of range"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: fraction <pos> ?<newval>? ?<pos> <newval> ...?
#
# Clients use this to get/set the fraction of real estate associated
# with the pane at position <pos>.  Can also be used to set the
# fractions for multiple panes if multiple <pos>/<newval> pairs
# are specified in the same command.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::fraction {pos args} {
    if {[llength $args] == 0} {
        return [lindex $_reqfrac $pos]
    }
    if {[llength $args] % 2 == 0} {
        error "wrong # args: should be \"fraction pos ?val pos val ...?\""
    }

    set args [linsert $args 0 $pos]
    foreach {pos newval} $args {
        if {![string is double -strict $newval]} {
            error "bad value \"$newval\": should be fraction 0-1"
        }
        if {$pos eq "end" || ($pos >= 0 && $pos < [llength $_reqfrac])} {
            set _reqfrac [lreplace $_reqfrac $pos $pos $newval]
            $_dispatcher event -idle !layout
        } else {
            error "bad index \"$pos\": out of range"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: hilite <state> <sash>
#
# Invoked automatically whenever the user touches a sash.  Highlights
# the sash by changing its size or relief.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::hilite {state sash} {
    switch -- $itk_option(-sashrelief) {
      flat {
        if {$state} {
            $itk_component(${sash}ridge) configure -background black
        } else {
            $itk_component(${sash}ridge) configure -background $itk_option(-background)
        }
      }
      sunken {
        if {$state} {
            $itk_component(${sash}ridge) configure -relief raised
        } else {
            $itk_component(${sash}ridge) configure -relief sunken
        }
      }
      raised {
        if {$state} {
            $itk_component(${sash}ridge) configure -relief sunken
        } else {
            $itk_component(${sash}ridge) configure -relief raised
        }
      }
      solid {
        if {$state} {
            $itk_component($sash) configure -background black
        } else {
            $itk_component($sash) configure \
                -background $itk_option(-background)
        }
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of panes in this widget.  That makes it easier
# to index the various panes, since indices run from 0 to size-1.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::size {} {
    return [llength $_panes]
}

# ----------------------------------------------------------------------
# USAGE: _grab <pane> <X> <Y>
#
# Invoked automatically when the user clicks on a sash, to initiate
# movement.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_grab {pname X Y} {
    set pos [lsearch $_panes $pname]
    if {$pos < 0} return
    set frac0 [lindex $_reqfrac [expr {$pos-1}]]
    set frac1 [lindex $_reqfrac $pos]
    set _dragfrac [expr {$frac0+$frac1}]

    if {$itk_option(-orientation) eq "vertical"} {
        set _dragfrom $Y
    } else {
        set _dragfrom $X
    }
}

# ----------------------------------------------------------------------
# USAGE: _drag <pane> <X> <Y>
#
# Invoked automatically as the user drags a sash, to resize the panes.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_drag {pname X Y} {
    set pos [lsearch $_panes $pname]
    if {$pos < 0} return
    set frac [lindex $_reqfrac $pos]

    if {$itk_option(-orientation) eq "vertical"} {
        set delY [expr {$_dragfrom-$Y}]
        set Ymax  [winfo height $itk_component(hull)]
        set delta [expr {double($delY)/$Ymax}]
        set frac [expr {$frac + $delta}]
        set _dragfrom $Y
    } else {
        set delX [expr {$_dragfrom-$X}]
        set Xmax  [winfo width $itk_component(hull)]
        set delta [expr {double($delX)/$Xmax}]
        set frac [expr {$frac + $delta}]
        set _dragfrom $X
    }
    if {$delta == 0.0} {
        return
    }

    # set limits so the pane can't get too large or too small
    if {$frac < 0.05} {
        set frac 0.05
    }
    if {$frac > $_dragfrac-0.05} {
        set frac [expr {$_dragfrac-0.05}]
    }

    # replace the fractions for this pane and the one before it
    set prevfrac [expr {$_dragfrac-$frac}]
    set _reqfrac [lreplace $_reqfrac [expr {$pos-1}] $pos $prevfrac $frac]

    # normalize all fractions and fix the layout
    _fixLayout

    return $frac
}

# ----------------------------------------------------------------------
# USAGE: _drop <pane> <X> <Y>
#
# Invoked automatically as the user drops a sash, to resize the panes.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_drop {pname X Y} {
    set frac [_drag $pname $X $Y]
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<eventArgs>...?
#
# Used internally to update the layout of panes whenever a new pane
# is added or a sash is moved.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_fixLayout {args} {
    # normalize the fractions for all panes to they add to 1.0
    set total 0
    foreach f $_reqfrac v $_visibility {
        if {$v && $f > 0} {
            set total [expr {$total + $f}]
        }
    }
    if {$total == 0.0} { set total 1 }

    set normfrac ""
    foreach f $_reqfrac v $_visibility {
        if {$v} {
            lappend normfrac [expr {double($f)/$total}]
        } else {
            lappend normfrac [expr {double($f)/$total}]
        }
    }

    # note that sash padding can be a single number or different on each side
    if {[llength $itk_option(-sashpadding)] == 1} {
        set pad [expr {2*$itk_option(-sashpadding)}]
    } else {
        set pad [expr [join $itk_option(-sashpadding) +]]
    }

    if {$itk_option(-orientation) eq "vertical"} {
        set h [winfo height $itk_component(hull)]
        set sh [expr {$itk_option(-sashwidth) + $pad}]

        set plist ""
        set flist ""
        foreach p $_panes f $normfrac v $_visibility {
            set sash ${p}sash
            if {$v} {
                # this pane is visible -- make room for it
                lappend plist $p
                lappend flist $f
                if {[info exists itk_component($sash)]} {
                    set h [expr {$h - $sh}]
                }
            } else {
                # this pane is not visible -- remove sash
                if {[info exists itk_component($sash)]} {
                    place forget $itk_component($sash)
                }
                place forget $itk_component($p)
            }
        }

        # lay out the various panes
        set y 0
        foreach p $plist f $flist {
            set sash ${p}sash
            if {[info exists itk_component($sash)]} {
                place $itk_component($sash) -y $y -relx 0.5 -anchor n \
                    -relwidth 1.0 -height $sh
                set y [expr {$y + $sh}]
            }

            set ph [expr {$h*$f}]
            place $itk_component($p) -y $y -relx 0.5 -anchor n \
                -relwidth 1.0 -height $ph
            set y [expr {$y + $ph}]
        }
    } else {
        set w [winfo width $itk_component(hull)]
        set sw [expr {$itk_option(-sashwidth) + $pad}]

        set plist ""
        set flist ""
        foreach p $_panes f $normfrac v $_visibility {
            set sash ${p}sash
            if {$v} {
                # this pane is visible -- make room for it
                lappend plist $p
                lappend flist $f
                if {[info exists itk_component($sash)]} {
                    set w [expr {$w - $sw}]
                }
            } else {
                # this pane is not visible -- remove sash
                if {[info exists itk_component($sash)]} {
                    place forget $itk_component($sash)
                }
                place forget $itk_component($p)
            }
        }

        # lay out the various panes
        set x 0
        foreach p $plist f $flist {
            set sash ${p}sash
            if {[info exists itk_component($sash)]} {
                place $itk_component($sash) -x $x -rely 0.5 -anchor w \
                    -relheight 1.0 -width $sw
                set x [expr {$x + $sw}]
            }

            set pw [expr {$w*$f}]
            place $itk_component($p) -x $x -rely 0.5 -anchor w \
                -relheight 1.0 -width $pw
            set x [expr {$x + $pw}]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSashes
#
# Used internally to fix the appearance of sashes whenever a new
# sash appears or the controlling configuration options change.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_fixSashes {args} {
    if {$itk_option(-orientation) eq "vertical"} {
        set ht [winfo pixels $itk_component(hull) $itk_option(-sashwidth)]
        set bd [expr {$ht/2}]
        foreach pane $_panes {
            set sash "${pane}sashridge"
            if {[info exists itk_component($sash)]} {
                $itk_component($sash) configure -height $ht \
                    -borderwidth $bd -relief $itk_option(-sashrelief)
                pack $itk_component($sash) -pady $itk_option(-sashpadding) \
                    -side top
            }
        }
    } else {
        set w [winfo pixels $itk_component(hull) $itk_option(-sashwidth)]
        set bd [expr {$w/2}]
        foreach pane $_panes {
            set sash "${pane}sashridge"
            if {[info exists itk_component($sash)]} {
                $itk_component($sash) configure -width $w \
                    -borderwidth $bd -relief $itk_option(-sashrelief)
                pack $itk_component($sash) -padx $itk_option(-sashpadding) \
                    -side left
            }
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -sashrelief
# ----------------------------------------------------------------------
itcl::configbody Rappture::Panes::sashrelief {
    $_dispatcher event -idle !sashes
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -sashwidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::Panes::sashwidth {
    $_dispatcher event -idle !sashes
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -sashpadding
# ----------------------------------------------------------------------
itcl::configbody Rappture::Panes::sashpadding {
    set count 0
    foreach val $itk_option(-sashpadding) {
        if {![string is integer -strict $val]} {
            error "bad padding value \"$val\": should be integer"
        }
        incr count
    }
    if {$count < 1 || $count > 2} {
        error "bad padding value \"$itk_option(-sashpadding)\": should be \"#\" or \"# #\""
    }
    $_dispatcher event -idle !sashes
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -orientation
# ----------------------------------------------------------------------
itcl::configbody Rappture::Panes::orientation {
    foreach pname $_panes {
        set sash "${pname}sash"
        if {$itk_option(-orientation) eq "vertical"} {
            place $itk_component($pname) -x 0 -relx 0.5 -relwidth 1 \
                -y 0 -rely 0 -relheight 0

            if {[info exists itk_component($sash)]} {
                place $itk_component($sash) -x 0 -relx 0.5 -relwidth 1 \
                    -y 0 -rely 0 -relheight 0
                $itk_component($sash) configure \
                    -cursor sb_v_double_arrow

                pack $itk_component(${sash}ridge) -fill x -side top
                $itk_component(${sash}ridge) configure \
                    -cursor sb_v_double_arrow
            }
        } else {
            place $itk_component($pname) -y 0 -rely 0.5 -relheight 1 \
                -x 0 -relx 0 -relwidth 0

            if {[info exists itk_component($sash)]} {
                place $itk_component($sash) -y 0 -rely 0.5 -relheight 1 \
                    -x 0 -relx 0 -relwidth 0
                $itk_component($sash) configure \
                    -cursor sb_h_double_arrow

                pack $itk_component(${sash}ridge) -fill y -side left
                $itk_component(${sash}ridge) configure \
                    -cursor sb_h_double_arrow
            }
        }
    }

    # fix sash characteristics
    $_dispatcher event -idle !sashes

    # make sure we fix up the layout at some point
    $_dispatcher event -idle !layout
}
