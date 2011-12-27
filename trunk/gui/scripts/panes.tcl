
# ----------------------------------------------------------------------
#  COMPONENT: Panes - creates a series of adjustable panes
#
#  This is a simple paned window with an adjustable sash.
#  the same quantity, but for various ranges of input values.
#  It also manages the controls to select and visualize the data.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
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
option add *Panes.sashCursor sb_v_double_arrow

itcl::class Rappture::Panes {
    inherit itk::Widget

    itk_option define -sashcursor sashCursor SashCursor ""
    itk_option define -sashrelief sashRelief SashRelief ""
    itk_option define -sashwidth sashWidth SashWidth 0
    itk_option define -sashpadding sashPadding SashPadding 0

    constructor {args} { # defined below }

    public method insert {pos args}
    public method pane {pos}
    public method visibility {pos {newval ""}}
    public method fraction {pos {newval ""}}
    public method hilite {state sash}

    protected method _grab {pane X Y}
    protected method _drag {pane X Y}
    protected method _drop {pane X Y}
    protected method _fixLayout {args}
    protected method _fixSashes {args}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _panes ""       ;# list of pane frames
    private variable _visibility ""  ;# list of visibilities for panes
    private variable _counter 0      ;# counter for auto-generated names
    private variable _frac 0.0       ;# list of fractions
    public variable orientation "vertical"
}

itk::usual Panes {
    keep -background -cursor
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
    set _frac 0.5

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
        rename -cursor -sashcursor sashCursor SashCursor
    }
    bind $itk_component($sash) <Enter> [itcl::code $this hilite on $sash]
    bind $itk_component($sash) <Leave> [itcl::code $this hilite off $sash]

    itk_component add ${sash}ridge {
        frame $itk_component($sash).ridge
    } {
        usual
        rename -cursor -sashcursor sashCursor SashCursor
        rename -relief -sashrelief sashRelief SashRelief
        ignore -borderwidth
    }
    if { $orientation == "vertical" } {
        pack $itk_component(${sash}ridge) -fill x 
    } else {
        pack $itk_component(${sash}ridge) -fill y -side left
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
    set _frac [linsert $_frac $pos $params(-fraction)]

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
# USAGE: visibility <pos> ?<newval>?
#
# Clients use this to get/set the visibility of the pane at position
# <pos>.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::visibility {pos {newval ""}} {
    if {"" == $newval} {
        return [lindex $_visibility $pos]
    }
    if {![string is boolean $newval]} {
        error "bad value \"$newval\": should be boolean"
    }
    if {$pos == "end" || ($pos >= 0 && $pos < [llength $_visibility])} {
        set _visibility [lreplace $_visibility $pos $pos [expr {$newval}]]
        $_dispatcher event -idle !layout
    } else {
        error "bad index \"$pos\": out of range"
    }
}

# ----------------------------------------------------------------------
# USAGE: fraction <pos> ?<newval>?
#
# Clients use this to get/set the fraction of real estate associated
# with the pane at position <pos>.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::fraction {pos {newval ""}} {
    if {"" == $newval} {
        return [lindex $_frac $pos]
    }
    if {![string is double $newval]} {
        error "bad value \"$newval\": should be fraction 0-1"
    }
    if {$pos == "end" || ($pos >= 0 && $pos < [llength $_frac])} {
        set len [llength $_frac]
        set _frac [lreplace $_frac $pos $pos xxx]
        set total 0
        foreach f $_frac {
            if {"xxx" != $f} {
                set total [expr {$total+$f}]
            }
        }
        for {set i 0} {$i < $len} {incr i} {
            set f [lindex $_frac $i]
            if {"xxx" == $f} {
                set f $newval
            } else {
                set f [expr {$f/$total - $newval/double($len-1)}]
            }
            set _frac [lreplace $_frac $i $i $f]
        }
        $_dispatcher event -idle !layout
    } else {
        error "bad index \"$pos\": out of range"
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
# USAGE: _grab <pane> <X> <Y>
#
# Invoked automatically when the user clicks on a sash, to initiate
# movement.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_grab {pname X Y} {
}

# ----------------------------------------------------------------------
# USAGE: _drag <pane> <X> <Y>
#
# Invoked automatically as the user drags a sash, to resize the panes.
# ----------------------------------------------------------------------
itcl::body Rappture::Panes::_drag {pname X Y} {
    if { $orientation == "vertical" } {
        set realY [expr {$Y-[winfo rooty $itk_component(hull)]}]
        set Ymax  [winfo height $itk_component(hull)]
        set frac [expr double($realY)/$Ymax]
    } else {
        set realX [expr {$X-[winfo rootx $itk_component(hull)]}]
        set Xmax  [winfo width $itk_component(hull)]
        set frac [expr double($realX)/$Xmax]
    }
    if {$frac < 0.05} {
        set frac 0.05
    }
    if {$frac > 0.95} {
        set frac 0.95
    }
    if {[llength $_frac] == 2} {
        set _frac [list $frac [expr {1-$frac}]]
    } else {
        set i [expr {[lsearch $_panes $pname]-1}]
        if {$i >= 0} {
            set _frac [lreplace $_frac $i $i $frac]
        }
    }
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
    if { $orientation == "vertical" } {
        set h [winfo height $itk_component(hull)]

        set plist ""
        set flist ""
        foreach p $_panes f $_frac v $_visibility {
            set sash ${p}sash
            if {$v} {
                # this pane is visible -- make room for it
                lappend plist $p
                lappend flist $f
                if {[info exists itk_component($sash)]} {
                    set h [expr {$h - [winfo reqheight $itk_component($sash)]}]
                }
            } else {
                # this pane is not visible -- remove sash
                if {[info exists itk_component($sash)]} {
                    place forget $itk_component($sash)
                }
                place forget $itk_component($p)
            }
        }
        
        # normalize the fractions so they add up to 1
        set total 0
        foreach f $flist { set total [expr {$total+$f}] }
        set newflist ""
        foreach f $flist {
            lappend newflist [expr {double($f)/$total}]
        }
        set flist $newflist
        
        # lay out the various panes
        set y 0
        foreach p $plist f $flist {
            set sash ${p}sash
            if {[info exists itk_component($sash)]} {
                set sh [winfo reqheight $itk_component($sash)]
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

        set plist ""
        set flist ""
        foreach p $_panes f $_frac v $_visibility {
            set sash ${p}sash
            if {$v} {
                # this pane is visible -- make room for it
                lappend plist $p
                lappend flist $f
                if {[info exists itk_component($sash)]} {
                    set w [expr {$w - [winfo reqwidth $itk_component($sash)]}]
                }
            } else {
                # this pane is not visible -- remove sash
                if {[info exists itk_component($sash)]} {
                    place forget $itk_component($sash)
                }
                place forget $itk_component($p)
            }
        }
        
        # normalize the fractions so they add up to 1
        set total 0
        foreach f $flist { set total [expr {$total+$f}] }
        set newflist ""
        foreach f $flist {
            lappend newflist [expr {double($f)/$total}]
        }
        set flist $newflist
        
        # lay out the various panes
        set x 0
        foreach p $plist f $flist {
            set sash ${p}sash
            if {[info exists itk_component($sash)]} {
                set sw [winfo reqwidth $itk_component($sash)]
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
    if { $orientation == "vertical" } {
        set ht [winfo pixels $itk_component(hull) $itk_option(-sashwidth)]
        set bd [expr {$ht/2}]
        foreach pane $_panes {
            set sash "${pane}sashridge"
            if {[info exists itk_component($sash)]} {
                $itk_component($sash) configure -height $ht -borderwidth $bd
                if {$itk_option(-sashrelief) == "solid"} {
                    $itk_component($sash) configure -background black
                } else {
                    $itk_component($sash) configure \
                        -background $itk_option(-background)
                }
                pack $itk_component($sash) -pady $itk_option(-sashpadding)
            }
        }
    } else {
        set w [winfo pixels $itk_component(hull) $itk_option(-sashwidth)]
        set bd [expr {$w/2}]
        foreach pane $_panes {
            set sash "${pane}sashridge"
            if {[info exists itk_component($sash)]} {
                $itk_component($sash) configure -width $w -borderwidth $bd
                if {$itk_option(-sashrelief) == "solid"} {
                    $itk_component($sash) configure -background black
                } else {
                    $itk_component($sash) configure \
                        -background $itk_option(-background)
                }
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
    $_dispatcher event -idle !sashes
}
