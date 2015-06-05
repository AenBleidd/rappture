# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Coverflow - browse through a series of images
#
#  This widget works like the coverflow browser on iOS.  It presents
#  a series of images with one in the center and others off to the
#  sides.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Coverflow.width 5i widgetDefault
option add *Coverflow.height 2i widgetDefault
option add *Coverflow.background black widgetDefault
option add *Coverflow.padding 20 widgetDefault
option add *Coverflow.spacing 40 widgetDefault
option add *Coverflow.font {Helvetica 10} widgetDefault
option add *Coverflow.fontColor white widgetDefault

itcl::class Rappture::Coverflow {
    inherit itk::Widget

    itk_option define -selectcommand selectCommand SelectCommand ""
    itk_option define -padding padding Padding 0
    itk_option define -spacing spacing Spacing 0
    itk_option define -font font Font ""
    itk_option define -fontcolor fontColor Foreground ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method insert {where args}
    public method itemconfigure {index args}
    public method delete {args}
    public method get {index args}
    public method size {}
    public method select {index args}

    private method _itemForIndex {index}
    private method _redraw {args}
    private method _rescale {tid height}
    private method _event {type x y}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _tweener ""     ;# used for animations
    private variable _items ""       ;# list of tag names for items
    private variable _data           ;# data for coverflow items
    private variable _counter 0      ;# for generating unique item IDs
    private variable _current ""     ;# currently selected item
    private variable _click ""       ;# click point from last _event call
}

itk::usual Rappture::Coverflow {
    keep -background -cursor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw [itcl::code $this _redraw]

    # create a tweener for animations
    set _tweener [Rappture::Tweener ::#auto -duration 300 \
        -command [list $_dispatcher event -idle !redraw -position %v] \
        -finalize [list $_dispatcher event -idle !redraw]]

    itk_component add area {
        canvas $itk_interior.area
    } {
        keep -background -borderwidth -relief
    }
    pack $itk_component(area) -side left -expand yes -fill both

    bind $itk_component(area) <Configure> \
        [list $_dispatcher event -idle !redraw]

    bind $itk_component(area) <ButtonPress> \
        [itcl::code $this _event click %x %y]
    bind $itk_component(area) <B1-Motion> \
        [itcl::code $this _event drag %x %y]
    bind $itk_component(area) <ButtonRelease> \
        [itcl::code $this _event release %x %y]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::destructor {} {
    itcl::delete object $_tweener
}

# ----------------------------------------------------------------------
# USAGE: insert <index> <image> ?-option <value>...?
#
# Inserts a new image into the coverflow at the position <index>, which
# is an integer starting from 0 at the left, or the keyword "end" for
# the right.  The remaining arguments are configuration options:
#
#   -tag ....... unique tag name for this item
#   -text ...... text displayed beneath the image when selected
#   -data ...... data sent to the callback command
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::insert {where imh args} {
    array set params $args
    if {[info exists params(-tag)]} {
        set tid $params(-tag)
        unset params(-tag)
    } else {
        set tid "item[incr _counter]"
    }

    if {[info exists _data($tid-text)]} {
        error "item \"$tid\" already exists -- tags must be unique"
    }
    set _items [linsert $_items $where $tid]
    set _data($tid-image) $imh
    set _data($tid-scaled) [image create photo]
    set _data($tid-lastupdate) ""
    set _data($tid-text) ""
    set _data($tid-data) ""

    eval itemconfigure $tid [array get params]

    if {$_current eq ""} {
        select $tid
    }

    # return the tag name for this new item
    return $tid
}

# ----------------------------------------------------------------------
# USAGE: itemconfigure <indexOrTag> -option <value> ...
#
# Used to modify an existing item.  The <indexOrTag> can be an integer
# index for the item or a -tag name given when the item was created.
# The remaining -option/value pairs are used to modify the values
# associated with the item.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::itemconfigure {index args} {
    set n [_itemForIndex $index]
    if {$n eq ""} {
        error "bad index \"$index\": should be integer or -tag name"
    }
    set tid [lindex $_items $n]

    # store the new data and then redraw
    foreach {key val} $args {
        if {[info exists _data($tid$key)]} {
            set _data($tid$key) $val

            # make sure we redraw the flow at some point
            $_dispatcher event -idle !redraw
        } else {
            error "bad option \"$key\": should be -text, -data"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: delete ?<indexOrTag> <indexOrTag>...?
# USAGE: delete all
#
# Deletes one or more entries from this listbox.  With the "all"
# keyword, it deletes all entries in the listbox.  Otherwise, it
# deletes one or more entries as specified.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::delete {args} {
    if {$args eq "all"} {
        set args $_items
    }
    foreach index $args {
        set n [_itemForIndex $index]
        if {$n ne ""} {
            set tid [lindex $_items $n]
            set _items [lreplace $_items $n $n]

            image delete $_data($tid-scaled)
            unset _data($tid-image)
            unset _data($tid-scaled)
            unset _data($tid-lastupdate)
            unset _data($tid-text)
            unset _data($tid-data)

            # if this item was selected, then clear that
            if {$tid eq $_current} {
                if {$n < [llength $_items]} {
                    set _current [lindex $_items $n]
                } else {
                    set _current [lindex $_items end]
                }
            }

            # make sure we redraw the list at some point
            $_dispatcher event -idle !redraw
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: get <indexOrTag> ?-option -option...?
#
# Used to query information about an item.  With no extra args, it
# returns the tag with an entry, or "" if the entry is not recognized.
# If additional -option values are specified, then data values are
# returned for the requested options.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::get {index args} {
    set n [_itemForIndex $index]
    if {$n eq ""} {
        return ""
    }
    if {[llength $args] == 0} {
        return [lindex $_items $n]
    }

    set vlist ""
    foreach option $args {
        if {[info exists _data($tid$option)]} {
            lappend vlist $_data($tid$option)
        } else {
            error "invalid option \"$option\""
        }
    }
    return $vlist
}

# ----------------------------------------------------------------------
# USAGE: size
#
# Returns the number of items in the listbox.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::size {} {
    return [llength $_items]
}

# ----------------------------------------------------------------------
# USAGE: select <indexOrTag> ?-animate on|off?
#
# Makes the given item the "current" item in the coverflow.  This is
# the item that is shown front-and-center.  If this was not already
# the current item, then the -selectcommand is invoked with information
# related to the selected item.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::select {index args} {
    set n [_itemForIndex $index]
    if {$n eq ""} {
        error "bad index \"$index\": should be integer or -tag name"
    }
    set tid [lindex $_items $n]
    if {$tid eq ""} {
        error "bad index \"$index\": index out of range"
    }

    set animate on
    array set params $args
    if {[info exists params(-animate)]} {
        set animate $params(-animate)
    }

    if {$_current ne $tid} {
        set from [lsearch -exact $_items $_current]
        set _current $tid

        # invoke the callback to react to this selection
        if {$itk_option(-selectcommand) ne ""} {
            set arglist $_data($tid-image)
            foreach opt {-text -data} {
                lappend arglist $opt $_data($tid$opt)
            }
            uplevel #0 $itk_option(-selectcommand) $arglist
        }

        # animate the motion and redraw the current choice
        if {$from >= 0 && $animate} {
            $_tweener configure -from $from -to $n
            $_tweener go -restart
        } else {
            $_dispatcher event -idle !redraw
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _itemForIndex <indexOrTag>
#
# Converts the given <indexOrTag> to a valid entry name.  If the
# <indexOrTag> is an integer, then it is interpreted as an index
# in the range 0 to the size of the list.  Otherwise, it is treated
# as a -tag name that may have been used when creating the item.
#
# Returns an integer index or "" if the item is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::_itemForIndex {index} {
    if {[string is integer -strict $index]} {
        return $index
    }
    if {$index eq "end"} {
        return [llength $_items]
    }
    set n [lsearch -exact $_items $index]
    if {$n >= 0} {
        return $n
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: _redraw ?-position <float>?
#
# Used internally to redraw all items in the list after it has changed.
# With no args, it draws the current state with the selected image
# shown in the middle.  The -position specifies an index, but perhaps
# with a fractional part.  An index like 2.3 is drawn with the center
# between images 2 and 3 -- about 1/3 of the way through the animation.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::_redraw {args} {
    set c $itk_component(area)
    set w [winfo width $c]
    set h [winfo height $c]
    $c delete all

    if {$w == 1 && $h == 1} {
        # just starting up -- forget it
        return
    }

    # get the desired position from the args or the current position
    set pos [lsearch $_items $_current]

    array set params $args
    if {[info exists params(-position)]} {
        set pos $params(-position)
    }

    if {$pos < 0} { set pos 0 }
    if {$pos >= [llength $_items]-1} { set pos [expr {[llength $_items]-1}] }

    # does any image have text?  if so, make room for it along the bottom
    set textht 0
    foreach tid $_items {
        if {$_data($tid-text) ne ""} {
            set textht [font metrics $itk_option(-font) -linespace]
            break
        }
    }

    # figure out the height of the covers and their midpoint
    set ih [expr {$h-2*$itk_option(-padding)-$textht}]
    set y0 [expr {($h-$textht)/2}]

    #
    # If the position is an integer, then that image is centered
    # in the current view.  Otherwise, we're between two images.
    #
    set xmid [expr {$w/2}]
    set spacing $itk_option(-spacing)
    set shadow 0.8
    set shorten 0.8
    set len 0.3

    if {abs($pos-round($pos)) < 0.05} {
        # one image right in the middle
        set lpos [expr {round($pos)}]
        set rpos $lpos

        # find the x-coord for the left/right sides of that image
        set tid [lindex $_items $lpos]
        set origw [image width $_data($tid-image)]
        set origh [image height $_data($tid-image)]
        set iw [expr {$ih*$origw/$origh}]
        set lx [expr {$xmid-$iw/2}]
        set rx [expr {$xmid+$iw/2}]
    } else {
        # between images -- pos and pos+1
        set lpos [expr {round(floor($pos))}]
        set rpos [expr {$lpos+1}]

        set llen [expr {$len + (1-$len)*($rpos-$pos)}]
        set lside [expr {$shorten + (1-$shorten)*($rpos-$pos)}]
        set lshadow [expr {$shadow*($pos-$lpos)}]
        set rlen [expr {$len + (1-$len)*($pos-$lpos)}]
        set rside [expr {$shorten + (1-$shorten)*($pos-$lpos)}]
        set rshadow [expr {$shadow*($rpos-$pos)}]

        # find the x-coord for the left/right sides of the two images
        set tid [lindex $_items $lpos]
        set origw [image width $_data($tid-image)]
        set origh [image height $_data($tid-image)]
        set iw0 [expr {$ih*$origw/$origh}]
        set lx0 [expr {$xmid-$iw0/2}]
        set rx0 [expr {$xmid+$iw0/2}]

        set tid [lindex $_items $rpos]
        set origw [image width $_data($tid-image)]
        set origh [image height $_data($tid-image)]
        set iw1 [expr {$ih*$origw/$origh}]
        set lx1 [expr {$xmid-$iw1/2}]
        set rx1 [expr {$xmid+$iw1/2}]

        set lx [expr {($lx1-$lx0+$spacing)*($rpos-$pos) + $lx0-$spacing}]
        set rx [expr {($rx1-$rx0-$spacing)*($pos-$lpos) + $rx0+$spacing}]
    }

    # scan through all images and update their scaling
    # use -lastupdate to avoid scaling if we're in the same state
    for {set i 0} {$i < [llength $_items]} {incr i} {
        set tid [lindex $_items $i]
        if {$i < $lpos} {
            set state "left $ih"
            if {$_data($tid-lastupdate) ne $state} {
                _rescale $tid $ih
                squeezer -side right -shadow $shadow -shorten $len \
                    $_data($tid-scaled) $_data($tid-scaled)
                set _data($tid-lastupdate) $state
            }
        } elseif {$i > $rpos} {
            set state "right $ih"
            if {$_data($tid-lastupdate) ne $state} {
                _rescale $tid $ih
                squeezer -side left -shadow $shadow -shorten $len \
                    $_data($tid-scaled) $_data($tid-scaled)
                set _data($tid-lastupdate) $state
            }
        } elseif {$i == $lpos && $i == $rpos} {
            set state "center $ih"
            if {$_data($tid-lastupdate) ne $state} {
                _rescale $tid $ih
                set _data($tid-lastupdate) $state
            }
        } elseif {$i == $lpos} {
            set state "left $ih ~ $llen"
            if {$_data($tid-lastupdate) ne $state} {
                _rescale $tid $ih
                squeezer -side right -shadow $lshadow -shorten $llen \
                    -amount $lside $_data($tid-scaled) $_data($tid-scaled)
                set _data($tid-lastupdate) $state
            }
        } elseif {$i == $rpos} {
            set state "right $ih ~ $rlen"
            if {$_data($tid-lastupdate) ne $state} {
                _rescale $tid $ih
                squeezer -side left -shadow $rshadow -shorten $rlen \
                    -amount $rside $_data($tid-scaled) $_data($tid-scaled)
                set _data($tid-lastupdate) $state
            }
        }
    }

    #
    # Figure out the positions of the various images to the left,
    # to the right, and in the middle.  Build up a list of items
    # to draw.
    #
    set tiles ""

    # start at left and lay down tiles toward the middle
    set numOnLeft [expr {round(floor($lx/$spacing))+1}]
    set i [expr {$lpos-$numOnLeft}]
    if {$i < 0} { set i 0 }
    set x0 [expr {$lx - ($lpos-$i)*$spacing}]
    while {$i < $lpos} {
        lappend tiles $i $x0 w
        set x0 [expr {$x0+$spacing}]
        incr i
    }

    # start at right and lay down tiles toward the middle
    set numOnRight [expr {round(floor(($w-$rx)/$spacing))+1}]
    set i [expr {$rpos+$numOnRight}]
    if {$i >= [llength $_items]} { set i [expr {[llength $_items]-1}] }
    set x0 [expr {$rx + ($i-$rpos)*$spacing}]
    while {$i > $rpos} {
        lappend tiles $i $x0 e
        set x0 [expr {$x0-$spacing}]
        incr i -1
    }

    # lay down the center image or images
    if {$lpos == $rpos} {
        # just one image in the middle
        lappend tiles $lpos $xmid c
    } else {
        # two tiles in a transition
        if {$llen > $rlen} {
            lappend tiles $rpos $rx e $lpos $lx w
        } else {
            lappend tiles $lpos $lx w $rpos $rx e
        }
    }

    # run through the list and draw all items
    foreach {i x anchor} $tiles {
        set tid [lindex $_items $i]
        $c create image $x $y0 -anchor $anchor \
            -image $_data($tid-scaled) -tags "item$i"
    }

    # if there is a single center image, show its text
    if {$lpos == $rpos} {
        set tid [lindex $_items $lpos]
        if {$_data($tid-text) ne ""} {
            $c create text $xmid [expr {$h-$itk_option(-padding)/2}] \
                -anchor s -text $_data($tid-text) \
                -font $itk_option(-font) -fill $itk_option(-fontcolor)
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _rescale <tid> <height>
#
# Used internally to rescale a single image (associated with item ID
# tid) to the specified <height>.
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::_rescale {tid ht} {
    set iw [image width $_data($tid-image)]
    set ih [image height $_data($tid-image)]
    set wd [expr {$ht*$iw/$ih}]
    $_data($tid-scaled) configure -width $wd -height $ht
    blt::winop resample $_data($tid-image) $_data($tid-scaled) box
}

# ----------------------------------------------------------------------
# USAGE: _event <type> <x> <y>
#
# Called whenever the user clicks or interacts with the coverflow.
# The event <type> is something like "click" or "drag".
# ----------------------------------------------------------------------
itcl::body Rappture::Coverflow::_event {type x y} {
    set c $itk_component(area)
    switch -- $type {
        click {
            set _click [list $x $y]
        }
        drag {
            if {$_click ne "" && $_current ne ""} {
                foreach {x0 y0} $_click break
                set delta [expr {double($x-$x0)/$itk_option(-spacing)}]
                set pos [lsearch -exact $_items $_current]
                set newpos [expr {$pos+$delta}]
                $_dispatcher event -idle !redraw -position $newpos

                if {![regexp {dragged} $_click]} {
                    lappend _click dragged
                }
            }
        }
        release {
            if {$_click ne ""} {
                foreach {x0 y0} $_click break
                if {abs($x-$x0) > 3 || abs($y-$y0) > 3} {
                    # one last drag event
                    _event drag $x $y
                }

                if {[regexp {dragged} $_click]} {
                    # adjust the current selection to the current pos
                    set delta [expr {double($x-$x0)/$itk_option(-spacing)}]
                    set pos [lsearch -exact $_items $_current]
                    set newpos [expr {round($pos+$delta)}]
                    if {$newpos < 0} {set newpos 0}
                    set max [expr {[llength $_items]-1}]
                    if {$newpos > $max} {set newpos $max}

                    select $newpos -animate off

                    # select may not change, so do one last proper draw
                    $_dispatcher event -idle !redraw -position $newpos

                    set _click ""
                    return
                }
            }

            # click in place -- see what image we clicked on
            set found ""
            foreach id [$c find overlapping $x $y $x $y] {
                if {[regexp {item([0-9]+)} [$c gettags $id] match index]} {
                    set found $index
                }
                # keep searching for top-most item in stack
            }
            if {$found ne ""} {
                select $found
            }
            set _click ""
        }
        default {
            error "bad option \"$type\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTIONS
# ----------------------------------------------------------------------
itcl::configbody Rappture::Coverflow::padding {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Coverflow::spacing {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Coverflow::font {
    $_dispatcher event -idle !redraw
}
itcl::configbody Rappture::Coverflow::fontcolor {
    $_dispatcher event -idle !redraw
}
