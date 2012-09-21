# ----------------------------------------------------------------------
#  COMPONENT: filmstrip - palette of objects arranged horizontally
#
#  This widget is similar to the "film strip" view in windows.  It
#  shows a horizontal arrangement of objects and lets you click on
#  each one to initiate an action.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Filmstrip.padding 8 widgetDefault
option add *Filmstrip.background #999999 widgetDefault
option add *Filmstrip.troughColor #666666 widgetDefault
option add *Filmstrip.activeBackground #d9d9d9 widgetDefault
option add *Filmstrip.titleBackground #888888 widgetDefault
option add *Filmstrip.titleForeground white widgetDefault
option add *Filmstrip.font {helvetica -12} widgetDefault

itcl::class Rappture::Filmstrip {
    inherit itk::Widget Rappture::Dragdrop

    itk_option define -orient orient Orient "horizontal"
    itk_option define -length length Length 0
    itk_option define -padding padding Padding 0
    itk_option define -titlebackground titleBackground Background ""
    itk_option define -titleforeground titleForeground Foreground ""
    itk_option define -activebackground activeBackground Foreground ""
    itk_option define -dragdropcommand dragDropCommand DragDropCommand ""

    constructor {args} { # defined below }
    public method add {name args}

    protected method _hilite {state name}
    protected method _fixLayout {args}
    protected method _scroll {args}

    # define these for drag-n-drop support of items
    protected method dd_get_source {widget x y}
    protected method dd_scan_target {x y data}
    protected method dd_finalize {option args}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _items ""       ;# list of item names
    private variable _data           ;# maps item name => title, image, etc.
}

itk::usual Filmstrip {
    keep -cursor -font -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::constructor {args} {
    component hull configure -height 100
    pack propagate $itk_component(hull) off

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout [itcl::code $this _fixLayout]

    itk_component add strip {
        canvas $itk_interior.strip -highlightthickness 0
    } {
        usual
        keep -borderwidth -relief
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(strip) -expand yes -fill both

    # this widget exports nodes via drag-n-drop
    dragdrop source $itk_component(strip)

    itk_component add sbar {
        scrollbar $itk_interior.sbar
    }

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: add <name> ?-title <string>? ?-image <handle>?
#
# Clients use this to add new items onto the film strip.  Each item
# has an internal <name> that refers to it, along with an icon and
# a title.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::add {name args} {
    Rappture::getopts args params {
        value -title ""
        value -image ""
    }
    if {[llength $args] > 0} {
        error "wrong # args: should be \"add name ?-option val -option val...?\""
    }

    set i [lsearch $_items $name]
    if {$i >= 0} {
        error "name \"$name\" already exists"
    }
    lappend _items $name
    set _data($name-title) $params(-title)
    set _data($name-image) $params(-image)
    set _data($name-pos) 0

    if {$itk_option(-orient) == "horizontal"} {
        set anchor w
        set whichsize wd
    } else {
        set anchor n
        set whichsize ht
    }

    $itk_component(strip) create rectangle 0 0 0 0 \
        -outline "" -fill "" -tags [list $name:all $name:bg]

    set htmax 0
    set sizemax 0
    if {"" != $params(-image)} {
        $itk_component(strip) create image 0 0 -anchor $anchor \
            -image $params(-image) -tags [list $name:all $name:image]
        foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:image] break
        set wd [expr {$x1-$x0}]
        set ht [expr {$y1-$y0}]
        set htmax $ht
        set sizemax [set $whichsize]
    }
    if {"" != $params(-title)} {
        $itk_component(strip) create placard 0 0 -anchor s \
            -foreground $itk_option(-titleforeground) \
            -background $itk_option(-titlebackground) \
            -text $params(-title) -tags [list $name:all $name:title]
        foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:title] break
        set wd [expr {$x1-$x0}]
        set ht [expr {$y1-$y0}]
        set htmax [expr {$htmax+$ht}]
        set s [set $whichsize]
        if {$s > $sizemax} {set sizemax $s}
    }

    $itk_component(strip) bind $name:all <Enter> \
        [itcl::code $this _hilite on $name]
    $itk_component(strip) bind $name:all <Leave> \
        [itcl::code $this _hilite off $name]

    # make sure we fix up the layout at some point
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: _hilite <state> <name>
#
# Called automatically when the mouse pointer enters/leaves an icon
# on the film strip.  Changes the background to highlight the option.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::_hilite {state name} {
    if {$state} {
        set bg $itk_option(-activebackground)
    } else {
        set bg ""
    }
    $itk_component(strip) itemconfigure $name:bg -fill $bg
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<eventArgs>...?
#
# Used internally to realign all items vertically after new items
# have been added to the strip.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::_fixLayout {args} {
    if {$itk_option(-orient) == "horizontal"} {
        # figure out the max height for overall strip
        set xpos $itk_option(-padding)
        set hmax 0
        foreach name $_items {
            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:image] break
            set h1 [expr {$y1-$y0}]
            set w [expr {$x1-$x0}]
            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:title] break
            set h2 [expr {$y1-$y0}]
            set h [expr {$h1+$h2}]
            if {$h > $hmax} { set hmax $h }

            set _data($name-pos) $xpos
            $itk_component(strip) coords $name:image $xpos 0
            set xpos [expr {$xpos + $w + $itk_option(-padding)}]
        }
        set hmax [expr {$hmax+2*$itk_option(-padding)}]

        set sbarh [winfo reqheight $itk_component(sbar)]
        component hull configure -height [expr {$hmax+$sbarh}]

        foreach name $_items {
            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:image] break
            set y0 $itk_option(-padding)
            set y1 $hmax
            set w [expr {$x1-$x0}]
            $itk_component(strip) coords $name:bg $x0 $y0 $x1 $y1

            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:title] break
            set titleh [expr {$y1-$y0}]
            set x [expr {$_data($name-pos) + $w/2}]
            set y [expr {$hmax-$itk_option(-padding)}]
            $itk_component(strip) coords $name:title $x $y

            foreach {x y} [$itk_component(strip) coords $name:image] break
            set y [expr {($hmax-$titleh)/2}]
            $itk_component(strip) coords $name:image $x $y
        }

        # fix up the scrolling region to include all of these items
        foreach {x0 y0 x1 y1} [$itk_component(strip) bbox all] break
        set x1 [expr {$x1+$itk_option(-padding)}]
        $itk_component(strip) configure -scrollregion [list 0 0 $x1 $y1]

        set size [winfo pixels $itk_component(hull) $itk_option(-length)]
        if {$size == 0} {
            component hull configure -width [expr {$x1+$itk_option(-padding)}]
        }

    } else {
        # figure out the max width for overall strip
        set ypos $itk_option(-padding)
        set wmax 0
        foreach name $_items {
            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:image] break
            set w [expr {$x1-$x0}]
            set ht [expr {$y1-$y0}]
            if {$w > $wmax} { set wmax $w }

            if {"" != [$itk_component(strip) find withtag $name:title]} {
                foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:title] break
                set w [expr {$x1-$x0}]
                set ht [expr {$ht+$y1-$y0}]
                if {$w > $wmax} { set wmax $w }
            }

            set _data($name-pos) $ypos
            $itk_component(strip) coords $name:image 0 $ypos
            set ypos [expr {$ypos + $ht + $itk_option(-padding)}]
        }
        set wmax [expr {$wmax+2*$itk_option(-padding)}]

        set sbarw [winfo reqwidth $itk_component(sbar)]
        component hull configure -width [expr {$wmax+$sbarw}]

        foreach name $_items {
            foreach {x y} [$itk_component(strip) coords $name:image] break
            set x [expr {$wmax/2}]
            $itk_component(strip) coords $name:image $x $y

            foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:image] break
            set imght [expr {$y1-$y0}]

            if {"" != [$itk_component(strip) find withtag $name:title]} {
                foreach {x0 y0 x1 y1} [$itk_component(strip) bbox $name:title] break
                set titleh [expr {$y1-$y0}]
                set x [expr {$wmax/2}]
                set y [expr {$_data($name-pos) + $imght + $titleh}]
                $itk_component(strip) coords $name:title $x $y
            } else {
                set titleh 0
            }

            set x0 $itk_option(-padding)
            set x1 $wmax
            set y0 $_data($name-pos)
            set y1 [expr {$y0 + $imght + $titleh}]
            $itk_component(strip) coords $name:bg $x0 $y0 $x1 $y1
        }

        # fix up the scrolling region to include all of these items
        set x1 0
        set y1 0
        foreach {x0 y0 x1 y1} [$itk_component(strip) bbox all] break
        set x1 [expr {$x1+$itk_option(-padding)}]
        $itk_component(strip) configure -scrollregion [list 0 0 $x1 $y1]

        set size [winfo pixels $itk_component(hull) $itk_option(-length)]
        if {$size == 0} {
            component hull configure -height [expr {$y1+$itk_option(-padding)}]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _scroll <arg> <arg>...
#
# Used internally to handle the automatic scrollbar.  If a scrollbar
# is needed, then it is packed into view.  Otherwise, it disappears.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::_scroll {args} {
    if {$itk_option(-orient) == "horizontal"} {
        set view [$itk_component(strip) xview]
        set side "bottom"
        set fill "x"
    } else {
        set view [$itk_component(strip) yview]
        set side "right"
        set fill "y"
    }

    if {$view != {0 1}} {
        pack $itk_component(sbar) -before $itk_component(strip) \
            -side $side -fill $fill
    } else {
        pack forget $itk_component(sbar)
    }
    eval $itk_component(sbar) set $args
}

# ----------------------------------------------------------------------
# USAGE: dd_get_source <widget> <x> <y>
#
# Looks at the given <widget> and <x>,<y> coordinate to figure out
# what data value the source is exporting.  Returns a string that
# identifies the type of the data.  This string is passed along to
# targets via the dd_scan_target method.  If the target may check
# the source type and reject the data.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::dd_get_source {widget x y} {
    if {[string length $itk_option(-dragdropcommand)] == 0} {
        return ""
    }

    # translate the screen x,y to the canvas x,y (may be scrolled down)
    set x [$itk_component(strip) canvasx $x]
    set y [$itk_component(strip) canvasy $y]

    foreach id [$itk_component(strip) find overlapping $x $y $x $y] {
        foreach tag [$itk_component(strip) gettags $id] {
            # search for a tag like XXX:all for item XXX
            if {[regexp {^([a-zA-Z0-9]+):all$} $tag match name]} {
                # invoke the dragdrop command with the item name
                # and see if it returns anything
                if {[catch {uplevel #0 $itk_option(-dragdropcommand) $name} result]} {
                    bgerror $result
                } elseif {[string length $result] > 0} {
                    return $result
                }
            }
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: dd_scan_target <x> <y> <data>
#
# Looks at the given <x>,<y> coordinate and checks to see if the
# dragdrop <data> can be accepted at that point.  Returns 1 if so,
# and 0 if the data is rejected.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::dd_scan_target {x y data} {
}

# ----------------------------------------------------------------------
# USAGE: dd_finalize drop -op start|end -from <w> -to <w> \
#                           -x <x> -y <y> -data <data>
# USAGE: dd_finalize cancel
#
# Handles the end of a drag and drop operation.  The operation can be
# completed with a successful drop of data, or cancelled.
# ----------------------------------------------------------------------
itcl::body Rappture::Filmstrip::dd_finalize {option args} {
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -orient
# ----------------------------------------------------------------------
itcl::configbody Rappture::Filmstrip::orient {
    switch -- $itk_option(-orient) {
        horizontal {
            $itk_component(strip) configure \
                -xscrollcommand [itcl::code $this _scroll]
            $itk_component(sbar) configure -orient horizontal \
                -command [list $itk_component(strip) xview]
            pack $itk_component(strip) -side top

            set size [winfo pixels $itk_component(hull) $itk_option(-length)]
            if {$size > 0} {
                component hull configure -width $size
            }
        }
        vertical {
            $itk_component(strip) configure \
                -yscrollcommand [itcl::code $this _scroll]
            $itk_component(sbar) configure -orient vertical \
                -command [list $itk_component(strip) yview]
            pack $itk_component(strip) -side left

            set size [winfo pixels $itk_component(hull) $itk_option(-length)]
            if {$size > 0} {
                component hull configure -height $size
            }
        }
        default {
            error "bad value \"$itk_option(-orient)\": should be horizontal, vertical"
        }
    }
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Filmstrip::length {
    set size [winfo pixels $itk_component(hull) $itk_option(-length)]
    if {$size > 0} {
        if {$itk_option(-orient) == "horizontal"} {
            component hull configure -width $size
        } else {
            component hull configure -height $size
        }
    } else {
        $_dispatcher event -idle !layout
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -titlebackground, -titleforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Filmstrip::titlebackground {
    foreach name $_items {
        $itk_component(strip) itemconfigure $name:title \
            -background $itk_option(-titlebackground)
    }
}
itcl::configbody Rappture::Filmstrip::titleforeground {
    foreach name $_items {
        $itk_component(strip) itemconfigure $name:title \
            -foreground $itk_option(-titleforeground)
    }
}
