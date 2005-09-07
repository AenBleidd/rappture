# ----------------------------------------------------------------------
#  COMPONENT: Balloon - toplevel popup window, like a cartoon balloon
#
#  This widget is used as a container for pop-up panels in Rappture.
#  If shape extensions are supported, then the window is drawn as
#  a cartoon balloon.  Otherwise, it is a raised panel, with a little
#  line connecting it to the widget that brought it up.  When active,
#  the panel has a global grab and focus, so the user must interact
#  with it or dismiss it.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

namespace eval Rappture { # forward declaration }

option add *Balloon.stemLength 16 widgetDefault

itcl::class Rappture::Balloon {
    inherit itk::Toplevel

    itk_option define -stemlength stemLength StemLength 20
    itk_option define -deactivatecommand deactivateCommand DeactivateCommand ""

    constructor {args} { # defined below }

    public method activate {where placement}
    public method deactivate {}

    protected method _createStems {}

    protected variable _stems   ;# windows for cartoon balloon stems
    protected variable _masks   ;# masks for cartoon balloon stems
    protected variable _fills   ;# lines for cartoon balloon stems

    public proc outside {widget x y}

    bind RapptureBalloon <ButtonPress> \
        {if {[Rappture::Balloon::outside %W %X %Y]} {%W deactivate}}
}

itk::usual Balloon {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::constructor {args} {
    wm overrideredirect $itk_component(hull) yes
    wm withdraw $itk_component(hull)
    component hull configure -borderwidth 1 -relief solid

    itk_component add inner {
        frame $itk_interior.inner -borderwidth 2 -relief raised
    }
    pack $itk_component(inner) -expand yes -fill both

    # add bindings to release the grab
    set btags [bindtags $itk_component(hull)]
    bindtags $itk_component(hull) [linsert $btags 1 RapptureBalloon]

    eval itk_initialize $args

    _createStems
}

# ----------------------------------------------------------------------
# USAGE: activate <where> <placement>
#
# Clients use this to pop up this balloon panel pointing to the
# <where> location, which should be a widget name or @X,Y.  The
# <placement> indicates whether the panel should be left, right,
# above, or below the <where> coordinate.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::activate {where placement} {
    if {![info exists _stems($placement)]} {
        error "bad placement \"$placement\": should be [join [lsort [array names _stems]] {, }]"
    }
    set s $_stems($placement)
    set sw [image width $_fills($placement)]
    set sh [image height $_fills($placement)]
    set p $itk_component(hull)

    if {[winfo exists $where]} {
        set x [expr {[winfo rootx $where]+[winfo width $where]/2}]
        set y [expr {[winfo rooty $where]+[winfo height $where]/2}]
        switch -- $placement {
            left { set x [expr {[winfo rootx $where]+5}] }
            right { set x [expr {[winfo rootx $where]+[winfo width $where]-5}] }
            above { set y [expr {[winfo rooty $where]+5}] }
            below { set y [expr {[winfo rooty $where]+[winfo height $where]-5}] }
        }
    } elseif {[regexp {^@([0-9]+),([0-9]+)$} $where match x y]} {
        # got x and y
    } else {
        error "bad location \"$where\": should be widget or @x,y"
    }

    # if the panel is already up, take it down
    deactivate

    switch -- $placement {
        left {
            set sx [expr {$x-$sw+3}]
            set sy [expr {$y-$sh/2}]
            set px [expr {$sx-[winfo reqwidth $p]+3}]
            set py [expr {$y-[winfo reqheight $p]/2}]
        }
        right {
            set sx $x
            set sy [expr {$y-$sh/2}]
            set px [expr {$x+$sw-3}]
            set py [expr {$y-[winfo reqheight $p]/2}]
        }
        above {
            set sx [expr {$x-$sw/2}]
            set sy [expr {$y-$sh+3}]
            set px [expr {$x-[winfo reqwidth $p]/2}]
            set py [expr {$sy-[winfo reqheight $p]+3}]
        }
        below {
            set sx [expr {$x-$sw/2}]
            set sy $y
            set px [expr {$x-[winfo reqwidth $p]/2}]
            set py [expr {$y+$sh-3}]
        }
    }
    if {[info exists _masks($placement)]} {
        shape set $s -bound photo $_masks($placement)
    }

    wm geometry $p +$px+$py
    wm deiconify $p
    raise $p

    wm geometry $s +$sx+$sy
    wm deiconify $s
    raise $s

    # grab the mouse pointer
    update
    while {[catch {grab set -global $itk_component(hull)}]} {
        after 100
    }
    focus $itk_component(hull)
}

# ----------------------------------------------------------------------
# USAGE: deactivate
#
# Clients use this to take down the balloon panel if it is on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::deactivate {} {
    if {[string length $itk_option(-deactivatecommand)] > 0} {
        uplevel #0 $itk_option(-deactivatecommand)
    }

    grab release $itk_component(hull)

    wm withdraw $itk_component(hull)
    foreach dir {left right above below} {
        wm withdraw $_stems($dir)
    }
}

# ----------------------------------------------------------------------
# USAGE: _createStems
#
# Used internally to create the stems that connect a balloon panel
# to its anchor point, in all four possible directions:  left, right,
# above, and below.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::_createStems {} {
    # destroy any existing stems
    foreach dir [array names _stems] {
        destroy $_stems($dir)
        unset _stems($dir)
    }
    foreach dir [array names _masks] {
        image delete $_masks($dir)
        unset _masks($dir)
    }
    foreach dir [array names _fills] {
        image delete $_fills($dir)
        unset _fills($dir)
    }

    if {[catch {package require Shape}] == 0} {
        #
        # We have the Shape extension.  Use it to create nice
        # looking (triangle-shaped) stems.
        #
        set s $itk_option(-stemlength)
        foreach dir {left right above below} {
            switch -- $dir {
                left - right {
                    set sw [expr {$s+2}]
                    set sh $s
                }
                above - below {
                    set sw $s
                    set sh [expr {$s+2}]
                }
            }

            set _stems($dir) [toplevel $itk_interior.s$dir -borderwidth 0]
            label $_stems($dir).l \
                -width $sw -height $sh -borderwidth 0
            pack $_stems($dir).l -expand yes -fill both

            wm withdraw $_stems($dir)
            wm overrideredirect $_stems($dir) yes

            #
            # Draw the triangle part of the stem, with a black outline
            # and light/dark highlights:
            #
            #     --------  ---       LEFT STEM
            #    |..##    |  ^  
            #    |  ..##  |  |        . = light color
            #    |    ..##|  | s      @ = dark color
            #    |    @@##|  |        # = black
            #    |  @@##  |  |
            #    |@@##    |  v
            #     --------  ---
            #    |<------>|
            #        s+2
            #
            set _masks($dir) [image create photo -width $sw -height $sh]
            set _fills($dir) [image create photo -width $sw -height $sh]

            set bg $itk_option(-background)
            set light [Rappture::color::brightness $bg 0.4]
            set dark [Rappture::color::brightness $bg -0.4]

            $_fills($dir) put $bg -to 0 0 $sw $sh

            switch -- $dir {
              left {
                set i 0
                for {set j 0} {$j < $s/2} {incr j} {
                    set ybtm [expr {$s-$j-1}]
                    $_fills($dir) put $dark \
                        -to $i [expr {$ybtm-1}] [expr {$i+2}] [expr {$ybtm+1}]
                    $_fills($dir) put black \
                        -to [expr {$i+2}] $ybtm [expr {$i+4}] [expr {$ybtm+1}]

                    set ytop $j
                    set ytoffs [expr {($j == $s/2-1) ? 1 : 2}]
                    $_fills($dir) put $light \
                        -to $i $ytop [expr {$i+2}] [expr {$ytop+$ytoffs}]
                    $_fills($dir) put black \
                        -to [expr {$i+2}] $ytop [expr {$i+4}] [expr {$ytop+1}]
                    incr i 2
                }
                $_stems($dir).l configure -image $_fills($dir)

                $_masks($dir) put black -to 0 0 $sw $sh
                set i 0
                for {set j 0} {$j < $s/2} {incr j} {
                    for {set k [expr {$i+4}]} {$k < $s+2} {incr k} {
                        $_masks($dir) transparency set $k $j yes
                        $_masks($dir) transparency set $k [expr {$s-$j-1}] yes
                    }
                    incr i 2
                }
              }
              right {
                set i $sw
                for {set j 0} {$j < $s/2} {incr j} {
                    set ybtm [expr {$s-$j-1}]
                    $_fills($dir) put $dark \
                        -to [expr {$i-2}] [expr {$ybtm-1}] $i [expr {$ybtm+1}]
                    $_fills($dir) put black \
                        -to [expr {$i-4}] $ybtm [expr {$i-2}] [expr {$ybtm+1}]

                    set ytop $j
                    set ytoffs [expr {($j == $s/2-1) ? 1 : 2}]
                    $_fills($dir) put $light \
                        -to [expr {$i-2}] $ytop $i [expr {$ytop+$ytoffs}]
                    $_fills($dir) put black \
                        -to [expr {$i-4}] $ytop [expr {$i-2}] [expr {$ytop+1}]
                    incr i -2
                }
                $_stems($dir).l configure -image $_fills($dir)

                $_masks($dir) put black -to 0 0 $sw $sh
                set i $sw
                for {set j 0} {$j < $s/2} {incr j} {
                    for {set k 0} {$k < $i-4} {incr k} {
                        $_masks($dir) transparency set $k $j yes
                        $_masks($dir) transparency set $k [expr {$s-$j-1}] yes
                    }
                    incr i -2
                }
              }
              above {
                set i 0
                for {set j 0} {$j < $s/2} {incr j} {
                    set xrhs [expr {$s-$j-1}]
                    $_fills($dir) put $dark \
                        -to [expr {$xrhs-1}] $i [expr {$xrhs+1}] [expr {$i+2}]
                    $_fills($dir) put black \
                        -to $xrhs [expr {$i+2}] [expr {$xrhs+1}] [expr {$i+4}]

                    set xlhs $j
                    set xloffs [expr {($j == $s/2-1) ? 1 : 2}]
                    $_fills($dir) put $light \
                        -to $xlhs $i [expr {$xlhs+$xloffs}] [expr {$i+2}]
                    $_fills($dir) put black \
                        -to $xlhs [expr {$i+2}] [expr {$xlhs+1}] [expr {$i+4}]
                    incr i 2
                }
                $_stems($dir).l configure -image $_fills($dir)

                $_masks($dir) put black -to 0 0 $sw $sh
                set i 0
                for {set j 0} {$j < $s/2} {incr j} {
                    for {set k [expr {$i+4}]} {$k < $s+2} {incr k} {
                        $_masks($dir) transparency set $j $k yes
                        $_masks($dir) transparency set [expr {$s-$j-1}] $k yes
                    }
                    incr i 2
                }
              }
              below {
                set i $sh
                for {set j 0} {$j < $s/2} {incr j} {
                    set xrhs [expr {$s-$j-1}]
                    $_fills($dir) put $dark \
                        -to [expr {$xrhs-1}] [expr {$i-2}] [expr {$xrhs+1}] $i
                    $_fills($dir) put black \
                        -to $xrhs [expr {$i-4}] [expr {$xrhs+1}] [expr {$i-2}]

                    set xlhs $j
                    set xloffs [expr {($j == $s/2-1) ? 1 : 2}]
                    $_fills($dir) put $light \
                        -to $xlhs [expr {$i-2}] [expr {$xlhs+$xloffs}] $i
                    $_fills($dir) put black \
                        -to $xlhs [expr {$i-4}] [expr {$xlhs+1}] [expr {$i-2}]
                    incr i -2
                }
                $_stems($dir).l configure -image $_fills($dir)

                $_masks($dir) put black -to 0 0 $sw $sh
                set i $sh
                for {set j 0} {$j < $s/2} {incr j} {
                    for {set k 0} {$k < $i-4} {incr k} {
                        $_masks($dir) transparency set $j $k yes
                        $_masks($dir) transparency set [expr {$s-$j-1}] $k yes
                    }
                    incr i -2
                }
              }
            }
        }
    } else {
        #
        # No shape extension.  Do the best we can by creating a
        # black line for all directions.
        #
        foreach {dir w h} [list \
            left   $itk_option(-stemlength) 3 \
            right  $itk_option(-stemlength) 3 \
            above  3 $itk_option(-stemlength) \
            below  3 $itk_option(-stemlength) \
        ] {
            set _stems($dir) [toplevel $itk_interior.s$dir \
                -width $w -height $h \
                -borderwidth 1 -relief solid -background black]
            wm withdraw $_stems($dir)
            wm overrideredirect $_stems($dir) yes

            # create this for size, even though we don't really use it
            set _fills($dir) [image create photo -width $w -height $h]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: outside <widget> <x> <y>
#
# Used internally to see if the click point <x>,<y> is outside of
# this widget.  If so, the widget usually releases is grab and
# deactivates.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::outside {widget x y} {
    return [expr {$x < [winfo rootx $widget]
             || $x > [winfo rootx $widget]+[winfo width $widget]
             || $y < [winfo rooty $widget]
             || $y > [winfo rooty $widget]+[winfo height $widget]}]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -stemlength
#
# Used internally to create the stems that connect a balloon panel
# to its anchor point, in all four possible directions:  left, right,
# above, and below.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::stemlength {
    if {$itk_option(-stemlength) % 2 != 0} {
        error "stem length should be an even number of pixels"
    }
}
