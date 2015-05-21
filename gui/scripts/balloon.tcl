# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

namespace eval Rappture { # forward declaration }

option add *Balloon.dismissButton on widgetDefault
option add *Balloon.padX 4 widgetDefault
option add *Balloon.padY 4 widgetDefault
option add *Balloon.titleBackground #999999 widgetDefault
option add *Balloon.titleForeground white widgetDefault
option add *Balloon.relief raised widgetDefault
option add *Balloon.stemLength 16 widgetDefault

itcl::class Rappture::Balloon {
    inherit itk::Toplevel

    itk_option define -background background Background ""
    itk_option define -deactivatecommand deactivateCommand DeactivateCommand ""
    itk_option define -dismissbutton dismissButton DismissButton "on"
    itk_option define -padx padX Pad 0
    itk_option define -pady padY Pad 0
    itk_option define -title title Title ""
    itk_option define -titlebackground titleBackground Background ""
    itk_option define -titleforeground titleForeground Foreground ""
    itk_option define -stemlength stemLength StemLength 20

    constructor {args} { # defined below }

    public method activate {where placement}
    public method deactivate {}

    protected method _createStems {}
    protected method _place {where placement w h sw sh}

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
    component hull configure -borderwidth 1 -relief solid -padx 0 -pady 0

    itk_component add border {
        frame $itk_interior.border -borderwidth 2
    } {
        usual
        keep -relief
    }
    pack $itk_component(border) -expand yes -fill both

    itk_component add titlebar {
        frame $itk_component(border).tbar
    } {
        usual
        rename -background -titlebackground titleBackground Background
    }

    itk_component add title {
        label $itk_component(titlebar).title -width 1 -anchor w
    } {
        usual
        rename -background -titlebackground titleBackground Background
        rename -foreground -titleforeground titleForeground Foreground
        rename -highlightbackground -titlebackground titleBackground Background
        rename -text -title title Title
    }
    pack $itk_component(title) -side left -expand yes -fill both -padx 2

    itk_component add dismiss {
        button $itk_component(titlebar).dismiss \
            -bitmap [Rappture::icon dismiss] \
            -relief flat -overrelief raised -command "
              Rappture::Tooltip::cue hide
              [list $itk_component(hull) deactivate]
            "
    } {
        usual
        rename -background -titlebackground titleBackground Background
        rename -foreground -titleforeground titleForeground Foreground
        rename -highlightbackground -titlebackground titleBackground Background
    }

    itk_component add inner {
        frame $itk_component(border).inner
    }
    pack $itk_component(inner) -expand yes -fill both

    # add bindings to release the grab
    set btags [bindtags $itk_component(hull)]
    bindtags $itk_component(hull) [linsert $btags 1 RapptureBalloon]

    eval itk_initialize $args

    _createStems
}


# ----------------------------------------------------------------------
# USAGE: _place <where> <place> <pw> <ph> <screenw> <screenh>
#
# Called by activate. Returns the exact location information given
# the parameters.  If the window will not fit on the screen with the
# requested placement, will loop through all possible placements to
# find the best alternative.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::_place {where place pw ph screenw screenh} {
    # pw and ph are requested balloon window size

    # set placement preference order
    switch $place {
        left {set plist {left above below right}}
        right {set plist {right above below left}}
        above {set plist {above below right left}}
        below {set plist {below above right left}}
    }

    set ph_orig $ph
    set pw_orig $pw

    foreach placement $plist {
        set pw $pw_orig
        set ph $ph_orig
        if {[winfo exists $where]} {
            # location of top-left corner of root window
            set rx [winfo rootx $where]
            set ry [winfo rooty $where]

            # size of widget we want to popup over
            set width  [winfo width $where]
            set height [winfo height $where]

            # x and y will be location for popup
            set x [expr {$rx + $width/2}]
            set y [expr {$ry + $height/2}]

            switch -- $placement {
                left { set x [expr {$rx + 5}] }
                right { set x [expr {$rx + $width - 5}] }
                above { set y [expr {$ry + 5}] }
                below { set y [expr {$ry + $height - 5}] }
            }
        } elseif {[regexp {^@([0-9]+),([0-9]+)$} $where match x y]} {
            # got x and y
        } else {
            error "bad location \"$where\": should be widget or @x,y"
        }

        # compute stem image size
        set s $_stems($placement)
        set sw [image width $_fills($placement)]
        set sh [image height $_fills($placement)]
        set offscreen 0

        switch -- $placement {
            left {
                set sx [expr {$x-$sw+3}]
                set sy [expr {$y-$sh/2}]
                set px [expr {$sx-$pw+3}]
                set py [expr {$y-$ph/2}]

                # make sure that the panel doesn't go off-screen
                if {$py < 0} {
                    incr offscreen [expr -$py]
                    set py 0
                }
                if {$py+$ph > $screenh} {
                    incr offscreen [expr {$py + $ph - $screenh}]
                    set py [expr {$screenh - $ph}]
                }
                if {$px < 0} {
                    incr offscreen [expr -$px]
                    set pw [expr {$pw + $px}]
                    set px 0
                }
            }
            right {
                set sx $x
                set sy [expr {$y-$sh/2}]
                set px [expr {$x+$sw-3}]
                set py [expr {$y-$ph/2}]

                # make sure that the panel doesn't go off-screen
                if {$py < 0} {
                    incr offscreen [expr -$py]
                    set py 0
                }
                if {$py+$ph > $screenh} {
                    incr offscreen [expr {$py + $ph - $screenh}]
                    set py [expr {$screenh-$ph}]
                }
                if {$px+$pw > $screenw} {
                    incr offscreen [expr {$px + $pw - $screenw}]
                    set pw [expr {$screenw-$px}]
                }
            }
            above {
                set sx [expr {$x-$sw/2}]
                set sy [expr {$y-$sh+3}]
                set px [expr {$x-$pw/2}]
                set py [expr {$sy-$ph+3}]

                # make sure that the panel doesn't go off-screen
                if {$px < 0} {
                    incr offscreen [expr -$px]
                    set px 0
                }
                if {$px+$pw > $screenw} {
                    incr offscreen [expr {$px + $pw - $screenw}]
                    set px [expr {$screenw-$pw}]
                }
                if {$py < 0} {
                    incr offscreen [expr -$py]
                    set ph [expr {$ph+$py}]
                    set py 0
                }
            }
            below {
                set sx [expr {$x-$sw/2}]
                set sy $y
                set px [expr {$x-$pw/2}]
                set py [expr {$y+$sh-3}]

                # make sure that the panel doesn't go off-screen
                if {$px < 0} {
                    incr offscreen [expr -$px]
                    set px 0
                }
                if {$px+$pw > $screenw} {
                    incr offscreen [expr {$px + $pw - $screenw}]
                    set px [expr {$screenw-$pw}]
                }
                if {$py+$ph > $screenh} {
                    incr offscreen [expr {$py + $py - $screenh}]
                    set ph [expr {$screenh-$py}]
                }
            }
        }
        set res($placement) [list $placement $offscreen $pw $ph $px $py $sx $sy]
        if {$offscreen == 0} {
            return "$placement $pw $ph $px $py $sx $sy"
        }
    }

    # In the unlikely event that we arrived here, it is because no
    # placement allowed the entire balloon window to be displayed.
    # Loop through the results and return the best-case placement.
    set _min 10000
    foreach pl $plist {
        set offscreen [lindex $res($pl) 1]
        if {$offscreen < $_min} {
            set _min $offscreen
            set _min_pl $pl
        }
    }
    return "$_min_pl [lrange $res($_min_pl) 2 end]"
}

# ----------------------------------------------------------------------
# USAGE: activate <where> <placement>
#
# Clients use this to pop up this balloon panel pointing to the
# <where> location, which should be a widget name or @X,Y.  The
# <placement> indicates whether the panel should be left, right,
# above, or below the <where> coordinate. Plecement is considered
# a suggestion and may be changed to fit the popup in the screen.
# ----------------------------------------------------------------------
itcl::body Rappture::Balloon::activate {where placement} {
    if {![info exists _stems($placement)]} {
        error "bad placement \"$placement\": should be [join [lsort [array names _stems]] {, }]"
    }

    # if the panel is already up, take it down
    deactivate

    set p $itk_component(hull)
    set screenw [winfo screenwidth $p]
    set screenh [winfo screenheight $p]

    set pw [winfo reqwidth $p]
    if {$pw > $screenw} { set pw [expr {$screenw-10}] }
    set ph [winfo reqheight $p]
    if {$ph > $screenh} { set ph [expr {$screenh-10}] }

    foreach {place pw ph px py sx sy} [_place $where $placement $pw $ph $screenw $screenh] break

    set s $_stems($place)
    if {[info exists _masks($place)]} {
        shape set $s -bound photo $_masks($place)
    }

    if { $pw < 1 || $ph < 1 }  {
        # I really don't know why this is happenning.  I believe this occurs
        # when in a work space (i.e the main window is smaller than the root
        # window). So for now, better to place the balloon window somewhere
        # than to fail with a bad geometry.
        wm geometry $p +$px+$py
    } else {
        wm geometry $p ${pw}x${ph}+$px+$py
    }
    wm deiconify $p
    raise $p

    wm geometry $s +$sx+$sy
    wm deiconify $s
    raise $s

    # grab the mouse pointer
    update
    while {[catch {grab set $itk_component(hull)}]} {
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
            set rgb [winfo rgb . $bg]
            set flatbg [format "#%04x%04x%04x" [lindex $rgb 0] [lindex $rgb 1] [lindex $rgb 2]]
            switch -- $itk_option(-relief) {
                raised {
                    set light [Rappture::color::brightness $bg 0.4]
                    set dark [Rappture::color::brightness $bg -0.4]
                    set bg $flatbg
                }
                flat - solid {
                    set light $flatbg
                    set dark $flatbg
                    set bg $flatbg
                }
                sunken {
                    set light [Rappture::color::brightness $bg -0.4]
                    set dark [Rappture::color::brightness $bg 0.4]
                    set bg $flatbg
                }
            }
            set bg [format "#%04x%04x%04x" [lindex $rgb 0] [lindex $rgb 1] [lindex $rgb 2]]

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
# CONFIGURATION OPTION: -background
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::background {
    _createStems
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

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -dismissbutton
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::dismissbutton {
    if {![string is boolean $itk_option(-dismissbutton)]} {
        error "bad value \"$itk_option(-dismissbutton)\": should be on/off, 1/0, true/false, yes/no"
    }
    if {$itk_option(-dismissbutton)} {
        pack $itk_component(titlebar) -before $itk_component(inner) \
            -side top -fill x
        pack $itk_component(dismiss) -side right -padx 4
    } elseif {"" != $itk_option(-title)} {
        pack $itk_component(titlebar) -before $itk_component(inner) \
            -side top -fill x
        pack forget $itk_component(dismiss)
    } else {
        pack forget $itk_component(titlebar)
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -padx
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::padx {
    pack $itk_component(inner) -padx $itk_option(-padx)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -pady
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::pady {
    pack $itk_component(inner) -pady $itk_option(-pady)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -title
# ----------------------------------------------------------------------
itcl::configbody Rappture::Balloon::title {
    if {"" != $itk_option(-title) || $itk_option(-dismissbutton)} {
        pack $itk_component(titlebar) -before $itk_component(inner) \
            -side top -fill x
        if {$itk_option(-dismissbutton)} {
            pack $itk_component(dismiss) -side right -padx 4
        } else {
            pack forget $itk_component(dismiss)
        }
    } else {
        pack forget $itk_component(titlebar)
    }
}
