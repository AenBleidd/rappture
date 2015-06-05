# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Videodial2 - selector, like the dial on a flow
#
#  This widget looks like the dial on an old-fashioned car flow.
#  It draws a series of values along an axis, and allows a selector
#  to move back and forth to select the values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Videodial2.thickness 10 widgetDefault
option add *Videodial2.padding 0 widgetDefault
option add *Videodial2.foreground black widgetDefault
option add *Videodial2.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Videodial2 {
    inherit itk::Widget

    itk_option define -min min Min 0
    itk_option define -max max Max 1
    itk_option define -minortick minortick Minortick 1
    itk_option define -majortick majortick Majortick 5
    itk_option define -variable variable Variable ""
    itk_option define -offset offset Offset 1

    itk_option define -thickness thickness Thickness 0
    itk_option define -padding padding Padding 0

    itk_option define -dialoutlinecolor dialOutlineColor Color "black"
    itk_option define -dialfillcolor dialFillColor Color "white"
    itk_option define -foreground foreground Foreground "black"

    itk_option define -font font Font ""


    constructor {args} { # defined below }
    destructor { # defined below }

    public method current {value}
    public method clear {}
    public method mark {args}
    public method bball {}
    public method loop {action args}

    protected method _bindings {type args}
    protected method _redraw {args}
    protected method _marker {tag action x y}
    protected method _setmark {type args}
    protected method _move {action x y}
    protected method _navigate {offset}
    protected method _fixSize {}
    protected method _fixValue {args}
    protected method _fixOffsets {}

    private method _calculateDialDimensions {}
    private method _calculateDisplayRange {args}
    private method _drawDial {disp_min disp_max}
    private method _getFramenumFromId {id}
    private method _nearestFrameMarker {x y}
    private method _current {value}
    private method _see {item}
    private method _offsetx {x}
    private method ms2rel {value}
    private method rel2ms {value}
    private method _goToFrame {framenum}

    private common _click             ;# x,y point where user clicked
    private common _marks             ;# list of marks
    private common _dd                ;# holds the dimensions of the dial
    private variable _values ""       ;# list of all values on the dial
    private variable _val2label       ;# maps value => string label(s)
    private variable _current 0       ;# current value (where pointer is)
    private variable _variable ""     ;# variable associated with -variable
    private variable _offset_pos 1    ;#
    private variable _offset_neg -1   ;#
    private variable _imspace 10      ;# pixels between intermediate marks
    private variable _min 0
    private variable _max 1
    private variable _minortick 1
    private variable _majortick 5
    private variable _lockdial ""
    private variable _displayMin 0
    private variable _displayMax 0
}

itk::usual Videodial2 {
    keep -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::constructor {args} {

    # bind $itk_component(hull) <<Frame>> [itcl::code $this _updateCurrent]

    # ----------------------------------------------------------------------
    # controls for the minor timeline.
    # ----------------------------------------------------------------------
    itk_component add minordial {
        canvas $itk_interior.minordial -background blue
    }


    bind $itk_component(minordial) <Configure> [itcl::code $this _redraw]

    _bindings timeline

    # ----------------------------------------------------------------------
    # place controls in widget.
    # ----------------------------------------------------------------------

    blt::table $itk_interior \
        0,0 $itk_component(minordial) -fill x

    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize none


    eval itk_initialize $args

    $itk_component(minordial) configure -background cyan

    _fixOffsets
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::destructor {} {
    configure -variable ""  ;# remove variable trace
    after cancel [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# USAGE: current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::current {value} {
    if {"" == $value} {
        return
    }

    if {$value < ${_min}} {
        set value ${_min}
    }

    if {$value > ${_max}} {
        set value ${_max}
    }

    _current [ms2rel $value]
}

# ----------------------------------------------------------------------
# USAGE: _current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_current {relval} {

    if {[string is double $relval] == 0} {
        error "bad value \"$relval\": expected relval to be a double"
    }
    if {${_current} == $relval} {
        # nothing to do
        return
    }

    if { $relval < 0.0 } {
        set relval 0.0
    }
    if { $relval > 1.0 } {
        set relval 1.0
    }

    set _current $relval
    set framenum [expr round([rel2ms $_current])]

    set updated 0
    if {[llength ${_lockdial}] != 0} {
        # the dial is "locked" this means we don't center the dial
        # on the current value. we only move the dial if the
        # current value is less than or greater than the marks alredy
        # shown on the dial.
        if {[expr ${_displayMin}+${_majortick}] >= $framenum} {
            set newdmin [expr $framenum-${_majortick}]
            foreach {dmin dmax} [_calculateDisplayRange -min $newdmin] break
            incr updated
        } elseif {[expr ${_displayMax}-${_majortick}] <= $framenum} {
            set newdmax [expr $framenum+${_majortick}]
            foreach {dmin dmax} [_calculateDisplayRange -max $newdmax] break
            incr updated
        }
    } else {
        # center the current marker
        foreach {dmin dmax} [_calculateDisplayRange] break
        incr updated
    }

    if {$updated == 1} {
        _drawDial $dmin $dmax
    }

    after idle [_setmark currentmark $framenum]

    # update the upvar variable
    if { $_variable != "" } {
        upvar #0 $_variable var
        set var $framenum
    }
}


# ----------------------------------------------------------------------
# USAGE: _bindings <type> ?args?
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_bindings {type args} {
    set c $itk_component(minordial)
    switch -- $type {
        "marker" {
            set tag [lindex $args 0]
            bind $c <ButtonPress-1> [itcl::code $this _marker $tag click %x %y]
            bind $c <B1-Motion> [itcl::code $this _marker $tag drag %x %y]
            bind $c <ButtonRelease-1> [itcl::code $this _marker $tag release %x %y]
            $c configure -cursor sb_h_double_arrow
        }
        "timeline" {
            # bind $c <ButtonPress-1> [itcl::code $this _move click %x %y]
            # bind $c <B1-Motion> [itcl::code $this _move drag %x %y]
            # bind $c <ButtonRelease-1> [itcl::code $this _move release %x %y]
            bind $c <ButtonPress-1> { }
            bind $c <B1-Motion> { }
            bind $c <ButtonRelease-1> { }
            $c configure -cursor ""
        }
        "toolmark" {
            if {[llength $args] != 2} {
                error "wrong # args: should be _bindings toolmark \[enter|leave\] <tag>"
            }

            foreach {mode tag} $args break

            switch -- $mode {
                "enter" {
                    #$c create rectangle [$c bbox $tag] \
                        -outline red \
                        -tags "highlight"
                    bind $c <ButtonPress-1> [itcl::code $this _goToFrame $_marks($tag)]
                }
                "leave" {
                    #$c delete "highlight"
                    _bindings timeline
                }
                default {
                    error "bad argument \"$mode\": should be enter or leave"
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: mark <property> <args>
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::mark {property args} {
    set retval 0

    switch -- $property {
        add {
            set retval [eval _setmark $args]
        }
        remove {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"mark remove <type>\""
            }
            set type [lindex $args 0]
            if {[info exists _marks($type)]} {
                $itk_component(minordial) delete $type
                array unset _marks $type
            }
        }
        position {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"mark position <type>\""
            }
            set type [lindex $args 0]
            if {[info exists _marks($type)]} {
                return $_marks($type)
            }
            set retval [expr ${_min}-1]
        }
        default {
            error "bad value \"$property\": should be one of add, remove, position"
        }
    }

    return $retval
}

# ----------------------------------------------------------------------
# USAGE: _getFramenumFromId <id>
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_getFramenumFromId {id} {
    set where 0

    # find the frame# tag to associate with the marker with
    if {[regexp {frame([0-9]+)} $id] == 0} {
        foreach tags [$c gettags $id] {
            if {"" != [set tmp [lsearch -inline -regexp $tags {frame[0-9]+}]]} {
                set where $tmp
                break
            }
        }
    } else {
        set where $id
    }
    # store the frame number in where
    regexp {frame([0-9]+)} $where match where

    # restrict <where> to valid frames between min and max
    if {$where < ${_min}} {
        set where ${_min}
    }
    if {$where > ${_max}} {
        set where ${_max}
    }

    return $where
}

# ----------------------------------------------------------------------
# USAGE: _setmark <type> ?[-xcoord|-tag]? <where>
#
# Clients use this to add a mark to the timeline
#   type can be any of loopstart, loopend, particle, measure, currentmark
#   where is interpreted based on the preceeding flag if available.
#       in the default case, <where> is interpreted as a frame number
#       or "current". if the -xcoord flag is provided, where is
#       interpreted as the x coordinate of where to center the marker.
#       -xcoord should only be used for temporary placement of a
#       marker. when -xcoord is used, the marker is placed exactly at
#       the provided x coordinate, and is not associated with any
#       frame. It's purpose is mainly for <B1-Motion> events.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_setmark {type args} {

    set c $itk_component(minordial)

    set cx0 0
    set cy0 0
    set cx1 0
    set cy1 0
    foreach {cx0 cy0 cx1 cy1} [$c bbox "imbox"] break

    # get coords of where to place the marker
    set frx0 0
    set fry0 0
    set frx1 0
    set fry1 0

    set where ""
    set largs [llength $args]
    if {$largs == 1} {
        set where [lindex $args 0]
        if {[string compare "current" $where] == 0} {
            set where [expr round([rel2ms ${_current}])]
        } elseif {[string is integer $where] == 0} {
            error "bad value \"$where\": while trying to place marker \"$type\": <where> should be an integer value"
        }

        # restrict <where> to valid frames between min and max
        if {$where < ${_min}} {
            set where ${_min}
        }
        if {$where > ${_max}} {
            set where ${_max}
        }

        set coords [$c coords "frame$where"]
        if {![llength $coords]} {
            # frame marker does not exist
            # estimate where to put the marker
            # use frame0 marker as a x=0 point
            foreach {frx0 fry0 frx1 fry1} [$c coords "frame0"] break
            set frx0 [expr {$frx0 + ((1.0*$where/${_minortick})*${_imspace})}]
        } else {
            foreach {frx0 fry0 frx1 fry1} $coords break
        }
        # where already contains the frame number
    } elseif {$largs == 2} {
        set flag [lindex $args 0]
        switch -- $flag {
            "-xcoord" {
                set frx0 [lindex $args 1]
                # where is not set for the -xcoord flag
            }
            "-tag" {
                set id [lindex $args 1]
                set where [_getFramenumFromId $id]
                foreach {frx0 fry0 frx1 fry1} [$c coords frame$where] break
            }
            default {
                error "bad value \"$flag\": should be -xcoord or -tag"
            }
        }
        if {[string is double $frx0] == 0} {
            error "bad value \"$frx0\": <where> should be a double value"
        }
    } else {
        error "wrong # args: should be \"mark <type> ?-xcoord? <where>\""
    }

    # add/remove the marker

    switch -glob -- $type {
        "loopstart" {
            # add start marker

            if {($where >= ${_displayMin}) && ($where < ${_displayMax})} {
                set smx0 $frx0                              ;# loopstart marker x0
                set smy0 $cy0                               ;# loopstart marker y0

                # polygon's outline adds a border to only one
                # side of the object? so we have weird +1 in
                # the triangle base in loopstart marker

                # marker stem is 3 pixels thick
                set smx1 [expr {$smx0+1}]                   ;# triangle top x
                set smy1 [expr {$smy0-10}]                  ;# triangle top y
                set smx2 $smx1                              ;# stem bottom right x
                set smy2 $cy1                               ;# stem bottom right y
                set smx3 [expr {$smx0-1}]                   ;# stem bottom left x
                set smy3 $smy2                              ;# stem bottom left y
                set smx4 $smx3                              ;# stem middle left x
                set smy4 $smy0                              ;# stem middle left y
                set smx5 [expr {$smx0-10+1}]                ;# triangle bottom left x
                set smy5 $smy0                              ;# triangle bottom left y

                set tag $type
                $c delete $tag
                $c create polygon \
                    $smx1 $smy1 \
                    $smx2 $smy2 \
                    $smx3 $smy3 \
                    $smx4 $smy4 \
                    $smx5 $smy5 \
                    -outline black -fill black -tags [list $tag dialval]

                $c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
                $c bind $tag <Leave> [itcl::code $this _bindings timeline]
            }

            if {[string compare "" $where] != 0} {
                set _marks($type) $where

                # make sure loopstart marker is before loopend marker
                if {[info exists _marks(loopend)]} {
                    set endFrNum $_marks(loopend)
                    if {$endFrNum < $where} {
                        _setmark loopend -tag frame[expr $where+1]
                    }
                }
            }

        }
        "loopend" {
            # add loopend marker

            if {($where >= ${_displayMin}) && ($where < ${_displayMax})} {
                set emx0 $frx0                              ;# loopend marker x0
                set emy0 $cy0                               ;# loopend marker y0

                set emx1 [expr {$emx0-1}]                   ;# triangle top x
                set emy1 [expr {$emy0-10}]                  ;# triangle top y
                set emx2 $emx1                              ;# stem bottom left x
                set emy2 $cy1                               ;# stem bottom left y
                set emx3 [expr {$emx0+1}]                   ;# stem bottom right x
                set emy3 $emy2                              ;# stem bottom right y
                set emx4 $emx3                              ;# stem middle right x
                set emy4 $emy0                              ;# stem middle right  y
                set emx5 [expr {$emx0+10-1}]                ;# triangle bottom right x
                set emy5 $emy0                              ;# triangle bottom right y

                set tag $type
                $c delete $tag
                $c create polygon \
                    $emx1 $emy1 \
                    $emx2 $emy2 \
                    $emx3 $emy3 \
                    $emx4 $emy4 \
                    $emx5 $emy5 \
                    -outline black -fill black -tags [list $tag dialval]

                $c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
                $c bind $tag <Leave> [itcl::code $this _bindings timeline]
            }

            if {[string compare "" $where] != 0} {
                set _marks($type) $where

                # make sure loopend marker is after loopstart marker
                if {[info exists _marks(loopstart)]} {
                    set startFrNum $_marks(loopstart)
                    if {$startFrNum > $where} {
                        _setmark loopstart -tag frame[expr $where-1]
                    }
                }
            }
        }
        "particle*" {
            if {($where >= ${_displayMin}) && ($where < ${_displayMax})} {
                set radius 3
                set pmx0 $frx0
                set pmy0 [expr {$cy1+5}]
                set coords [list [expr $pmx0-$radius] [expr $pmy0-$radius] \
                                 [expr $pmx0+$radius] [expr $pmy0+$radius]]

                set tag $type
                $c delete $tag
                $c create oval $coords \
                    -fill green \
                    -outline black \
                    -width 1 \
                    -tags [list $tag dialval]

                $c bind $tag <Enter> [itcl::code $this _bindings toolmark enter $tag]
                $c bind $tag <Leave> [itcl::code $this _bindings toolmark leave $tag]
            }

            if {[string compare "" $where] != 0} {
                set _marks($type) $where
            }
        }
        "measure*" {
            if {($where >= ${_displayMin}) && ($where < ${_displayMax})} {
                set radius 3
                set amx0 $frx0
                set amy0 [expr {$cy1+15}]
                set coords [list [expr $amx0-$radius] [expr $amy0-$radius] \
                                 [expr $amx0+$radius] [expr $amy0+$radius]]

                set tag $type
                $c delete $tag
                $c create line $coords \
                    -fill red \
                    -width 3  \
                    -tags [list $tag dialval]

                $c bind $tag <Enter> [itcl::code $this _bindings toolmark enter $tag]
                $c bind $tag <Leave> [itcl::code $this _bindings toolmark leave $tag]
            }

            if {[string compare "" $where] != 0} {
                set _marks($type) $where
            }
        }
        "currentmark" {

            if {($where >= ${_displayMin}) && ($where < ${_displayMax})} {
                set cmx0 $frx0                              ;# current marker x0
                set cmy0 $cy0                               ;# current marker y0

                set cmx1 [expr {$cmx0+5}]                   ;# lower right diagonal edge x
                set cmy1 [expr {$cmy0-5}]                   ;# lower right diagonal edge y
                set cmx2 $cmx1                              ;# right top x
                set cmy2 [expr {$cmy1-5}]                   ;# right top y
                set cmx3 [expr {$cmx0-5}]                   ;# left top x
                set cmy3 $cmy2                              ;# left top y
                set cmx4 $cmx3                              ;# lower left diagonal edge x
                set cmy4 $cmy1                              ;# lower left diagonal edge y

                set tag $type
                $c delete $tag
                $c create polygon \
                    $cmx0 $cmy0 \
                    $cmx1 $cmy1 \
                    $cmx2 $cmy2 \
                    $cmx3 $cmy3 \
                    $cmx4 $cmy4 \
                    -outline black \
                    -fill red \
                    -tags [list $tag dialval]
                $c create line $cmx0 $cmy0 $cmx0 $cy1 \
                    -fill red \
                    -tags [list $tag dialval]

                $c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
                $c bind $tag <Leave> [itcl::code $this _bindings timeline]
            }

            if {[string compare "" $where] != 0} {
                set _marks($type) $where
            }
        }
        default {
            error "bad value \"$type\": should be \"loopstart\" or \"loopend\""
        }
    }
    return
}

# ----------------------------------------------------------------------
# USAGE: bball
#   debug function to print out the bounding box information for
#   minor dial
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::bball {} {
    set c $itk_component(minordial)
    foreach item [$c find all] {
        foreach {x0 y0 x1 y1} [$c bbox $item] break
        if {! [info exists y1]} continue
        puts stderr "$item : [expr $y1-$y0]: $y0 $y1"
        lappend q $y0 $y1
    }
    set q [lsort -real $q]
    puts stderr "q [lindex $q 0] [lindex $q end]"
    puts stderr "height [winfo height $c]"
    puts stderr "bbox all [$c bbox all]"
    puts stderr "parent height [winfo height [winfo parent $c]]"
}

# ----------------------------------------------------------------------
# USAGE: loop
#   loop between <start> <end> - setup looping between frames <start> and <end>
#   loop disable - disable looping
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::loop {action args} {
    switch -- $action {
        "between" {
            if {[llength $args] != 2} {
                error "wrong # args: should be loop between <start> <end>"
            }
            foreach {startframe endframe} $args break
            mark add loopstart $startframe
            mark add loopend $endframe
            lappend _lockdial "loop"
        }
        "disable" {
            if {[llength $args] != 0} {
                error "wrong # args: should be loop disable"
            }
            mark remove loopstart
            mark remove loopend
            set idx [lsearch -exact ${_lockdial} "loop"]
            set _lockdial [lreplace ${_lockdial} $idx $idx]
        }
        default {
            error "bad value \"$action\": should be \"between\" or \"disable\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _calculateDisplayRange
#   calculate the minimum and maximum values to display on the dial
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_calculateDisplayRange {args} {

    Rappture::getopts args params {
        value -min 0
        value -max 0
    }

    set disp_min $params(-min)
    set disp_max $params(-max)

    if {($disp_min == 0) && ($disp_max == 0)} {
        # calculate the display min and max by
        # centering the current value on the dial
        set framenum [expr round([rel2ms ${_current}])]
        set disp_min [expr ($framenum-$_dd(ni))]
        set disp_max [expr ($framenum+$_dd(ni))]
    } elseif {($disp_min != 0) && ($disp_max == 0)} {
        # user supplied disp_min value
        # calculate disp_max
        set disp_max [expr round($disp_min+($_dd(ni)*2.0))]
    } elseif {($disp_min == 0) && ($disp_max != 0)} {
        # user supplied disp_max value
        # calculate disp_min
        set disp_min [expr round($disp_max-($_dd(ni)*2.0))]
    }

    # deal with edge cases

    if {($disp_min < ${_min})} {
        # don't display marks less than ${_min}
        set disp_min ${_min}
        set disp_max [expr round(${_min}+($_dd(ni)*2.0))]
        if {$disp_max > ${_max}} {
            set disp_max ${_max}
        }
    }

    if {($disp_max > ${_max})} {
        # don't display marks greater than ${_max}
        set disp_min [expr round(${_max}-($_dd(ni)*2.0))]
        if {$disp_min < ${_min}} {
            set disp_min ${_min}
        }
        set disp_max ${_max}
    }

    return [list $disp_min $disp_max]
}

# ----------------------------------------------------------------------
# USAGE: _calculateDialDimensions
#   calculate dimensions used to draw the dial
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_calculateDialDimensions {} {
    set c $itk_component(minordial)
    set w [winfo width $c]
    set h [winfo height $c]
    set p [winfo pixels $c $itk_option(-padding)]
    set t [expr {$itk_option(-thickness)+1}]
    set y1 [expr {$h-1}]
    set y0 [expr {$y1-$t}]
    set x0 [expr {$p+1}]
    set x1 [expr {$w-$p-4}]


    # add intermediate marks between markers
    set imw 1.0                                 ;# intermediate mark width

    set imsh [expr {$t/3.0}]                    ;# intermediate mark short height
    set imsy0 [expr {$y0+(($t-$imsh)/2.0)}]     ;# precalc'd imark short y0 coord
    set imsy1 [expr {$imsy0+$imsh}]             ;# precalc'd imark short y1 coord

    set imlh [expr {$t*2.0/3.0}]                ;# intermediate mark long height
    set imly0 [expr {$y0+(($t-$imlh)/2.0)}]     ;# precalc'd imark long y0 coord
    set imly1 [expr {$imly0+$imlh}]             ;# precalc'd imark long y1 coord

    set imty [expr {$y0-5}]                     ;# height of marker value

    set x [expr ($x1-$x0)/2.0]
    set ni [expr int(($x-$x0)/${_imspace})]

    set _dd(x0) $x0
    set _dd(y0) $y0
    set _dd(x1) $x1
    set _dd(y1) $y1
    set _dd(ni) $ni
    set _dd(imly0) $imly0
    set _dd(imly1) $imly1
    set _dd(imw) $imw
    set _dd(imty) $imty
    set _dd(imsy0) $imsy0
    set _dd(imsy1) $imsy1

}

# ----------------------------------------------------------------------
# USAGE: _redraw
#   calculate dimensions, display range, and draw the dial, again
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_redraw {args} {

    _calculateDialDimensions

    foreach {disp_min disp_max} [_calculateDisplayRange] break

    set c $itk_component(minordial)
    $c delete all

    # draw the background rectangle for the minor time line
    $c create rectangle $_dd(x0) $_dd(y0) $_dd(x1) $_dd(y1) \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor) \
        -tags "imbox"

    _drawDial $disp_min $disp_max

    # calculate the height of the intermediate tick marks
    # and frame numbers on our canvas, resize the imbox
    # to include both of them.
    set box [$c bbox "all"]
    if {![llength $box]} {
        set box [list 0 0 0 0]
    }
    $c coords "imbox" $box

    # add an invisible box 20 pixels above and below imbox
    # to keep the canvas from changing sizes when the we add
    # particle, loopstart, loopend, and line markers
    foreach {x0 y0 x1 y1} $box break
    $c create rectangle $x0 [expr $y0-20] $x1 [expr $y1+20] \
        -outline "" \
        -fill "" \
        -tags "markerbox"

    # re-add any marks that the user previously specified
    # do it again because we changed the size of imbox
    foreach n [array names _marks] {
        if {($_marks($n) > ${_displayMin}) && ($_marks($n) < ${_displayMax})} {
            _setmark $n $_marks($n)
        }
    }

    _fixSize
}


# ----------------------------------------------------------------------
# USAGE: _drawDial
#   draw the dial
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_drawDial {dmin dmax} {
    set c $itk_component(minordial)
    $c delete dialval

    set _displayMin [expr int($dmin)]
    set _displayMax [expr int($dmax)]

    set imx [expr $_dd(x0)+5]
    for {set i ${_displayMin}} {$i <= ${_displayMax}} {incr i} {
        if {($i%${_majortick}) == 0} {
            # draw major tick
            $c create line $imx $_dd(imly0) $imx $_dd(imly1) \
                -fill red \
                -width $_dd(imw) \
                -tags [list dialval longmark imark "frame$i"]

            $c create text $imx $_dd(imty) \
                -anchor center \
                -text $i \
                -font $itk_option(-font) \
                -tags [list dialval frame$i frame$i-text]

            set imx [expr $imx+${_imspace}]
        } elseif {($i%${_minortick}) == 0 } {
            # draw minor tick
            $c create line $imx $_dd(imsy0) $imx $_dd(imsy1) \
                -fill blue \
                -width $_dd(imw) \
                -tags [list dialval shortmark imark "frame$i"]

            set imx [expr $imx+${_imspace}]
        }
    }

    if {(${_displayMin}%${_majortick}) == 0} {
        # first tick is numbered, delete the text if
        # it shows up outside of our desired imbox
        foreach {dmx0 dmy0 dmx1 dmy1} [$c bbox frame${_displayMin}-text] break
        # need to check for if dmx0 exists because there
        # are some cases when the widget is first being
        # created where _calculateDisplayRange gives
        # back bad values, dmin > dmax.
        if {[info exists dmx0] && ($dmx0 < $_dd(x0))} {
            $c delete frame${_displayMin}-text
        }
    }
    if {(${_displayMax}%${_majortick}) == 0} {
        # last tick is numbered, delete the text if
        # it shows up outside of our desired imbox
        foreach {dmx0 dmy0 dmx1 dmy1} [$c bbox frame${_displayMax}-text] break
        # need to check for if dmx1 exists because there
        # are some cases when the widget is first being
        # created where _calculateDisplayRange gives
        # back bad values, dmin > dmax.
        if {[info exists dmx1] && ($dmx1 > $_dd(x1))} {
            $c delete frame${_displayMax}-text
        }
    }

    # add any marks that the user previously specified
    foreach n [array names _marks] {
        if {($_marks($n) > ${_displayMin}) && ($_marks($n) < ${_displayMax})} {
            _setmark $n $_marks($n)
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the height of the minor dial based
# on the items placed on the canvas
#
# FIXME: instead of calling this in the mark command, figure out how to
#   make the canvas the correct size to start with
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_fixSize {} {
    # resize the height of the minor timeline canvas
    # to include everything we know about

    set c $itk_component(minordial)

    set box [$c bbox "all"]
    if {![llength $box]} {
        set box [list 0 0 0 0]
    }

    foreach {x0 y0 x1 y1} $box break
    set h [expr $y1-$y0]

    $c configure -height $h -scrollregion $box -xscrollincrement 1p
}

# ----------------------------------------------------------------------
# USAGE: _goToFrame <framenum>
#
# lock the dial and go to frame <framenum>
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_goToFrame {framenum} {
    lappend _lockdial "_goToFrame"
    current $framenum
    event generate $itk_component(hull) <<Value>>
    set idx [lsearch -exact ${_lockdial} "_goToFrame"]
    set _lockdial [lreplace ${_lockdial} $idx $idx]
}

# ----------------------------------------------------------------------
# USAGE: _offsetx <x>
#
# Calculate an x coordinate that has been offsetted by a scrolled canvas
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_offsetx {x} {
    set c $itk_component(minordial)
    set w [lindex [$c cget -scrollregion] 2]
    set x0 [lindex [$c xview] 0]
    set offset [expr {$w*$x0}]
    set x [expr {$x+$offset}]
    return $x
}

# ----------------------------------------------------------------------
# USAGE: _nearestFrameMarker <x> <y>
#
# Called automatically whenever the user clicks or drags on a marker
# widget.  Moves the selected marker to the next nearest tick mark.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_nearestFrameMarker {x y} {

    set c $itk_component(minordial)

    foreach {junk y0 junk y1} [$c bbox "imark"] break
    set id ""
    # FIXME:
    # the amount added to _imspace should be 1
    # but for some reason the dial is slightly shifted
    # on the canvas by 2 or 3 pixels. so we have to
    # add a larger value like 3 to _imspace to find
    # the next frame marker
    set x0 [expr {$x-round((${_imspace}+3)/2.0)}]
    set x1 [expr {$x+round((${_imspace}+3)/2.0)}]

    #$c delete scrap
    #$c create rectangle $x0 $y0 $x1 $y1 \
        -outline green -tags "scrap"

    foreach item [$c find enclosed $x0 $y0 $x1 $y1] {
        set itemtags [$c gettags $item]

        foreach {x00 junk x11 junk} [$c bbox $item] break

        if {[lsearch -exact $itemtags "imark"] != -1} {
            set id [lsearch -inline -regexp $itemtags {frame[0-9]}]
            break
        }
    }
    if {[string compare "" $id] == 0} {
        # something went wrong
        # we should have found an imark with
        # an associated "frame#" tag to snap to
        # bailout
        #error "could not find an intermediate mark to snap marker to"
        puts stderr "could not find an intermediate mark to snap marker to"
    }

    return $id

}

# ----------------------------------------------------------------------
# USAGE: _marker <tag> click <x> <y>
#        _marker <tag> drag <x> <y>
#        _marker <tag> release <x> <y>
#
# Called automatically whenever the user clicks or drags on a marker
# widget.  Moves the selected marker to the next nearest tick mark.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_marker {tag action x y} {
    set c $itk_component(minordial)
    switch $action {
        "click" {
            if {"currentmark" == $tag} {
                focus $itk_component(hull)
                lappend _lockdial "currentmark"
            }
        }
        "drag" {
            set x [_offsetx $x]

            # as the marker passes over frame marks, snap to the closest frame mark
            set id [_nearestFrameMarker $x $y]
            if {"" == $id} {
                return
            }

            if {"currentmark" == $tag} {
                # update the current frame shown on the screen
                # this doesn't quite work yet, i think because
                # the seek requests are buffered until the gui
                # is idle
                set newfn [_getFramenumFromId $id]
                set oldfn [expr round([rel2ms ${_current}])]
                if {$newfn != $oldfn} {
                    _goToFrame $newfn
                }
            } else {
                _setmark $tag -tag $id
            }
        }
        "release" {

            _marker $tag drag $x $y

            if {"currentmark" == $tag} {
                set idx [lsearch -exact ${_lockdial} "currentmark"]
                set _lockdial [lreplace ${_lockdial} $idx $idx]
            }

            # take care of cases where the mouse leaves the marker's boundries
            # before the button-1 has been released. we check if the last
            # coord was within the bounds of the marker. if not, we manually
            # generate the "Leave" event.
            set leave 1
            set x [_offsetx $x]
            foreach item [$c find overlapping $x $y $x $y] {
                if {[lsearch -exact [$c gettags $item] $tag] != -1} {
                    set leave 0
                }
            }
            if {$leave == 1} {
                # FIXME:
                # i want to generate the event rather than
                # calling the function myself...
                # event generate $c <Leave>
                _bindings timeline
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
#        _move drag <x> <y>
#        _move release <x> <y>
#
# Scroll the timeline canvas based on user's b1 click, drag, and release
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_move {action x y} {
    switch $action {
        "click" {
            set _click(x) $x
            set _click(y) $y
        }
        "drag" {
            set c $itk_component(minordial)
            set dist [expr $_click(x)-$x]
            #$c xview scroll $dist "units"
            set _click(x) $x
            set _click(y) $y


            #set disp_min
            #set disp_max
            #_redraw $disp_min $disp_max
        }
        "release" {
            _move drag $x $y
            catch {unset _click}
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _see <item>
#
# from http://tcl.sourceforge.net/faqs/tk/#canvas/see
# "see" method alternative for canvas
# Aligns the named item as best it can in the middle of the screen
#
# item - a canvas tagOrId
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_see {item} {
    set c $itk_component(minordial)
    set box [$c bbox $item]
    if {![llength $box]} return
    ## always properly set -scrollregion
    foreach {x y x1 y1}     $box \
            {top btm}       [$c yview] \
            {left right}    [$c xview] \
            {p q xmax ymax} [$c cget -scrollregion] {
        set xpos [expr {(($x1+$x)/2.0)/$xmax - ($right-$left)/2.0}]
        #set ypos [expr {(($y1+$y)/2.0)/$ymax - ($btm-$top)/2.0}]
    }
    $c xview moveto $xpos
    #$c yview moveto $ypos
}


# ----------------------------------------------------------------------
# USAGE: _navigate <offset>
#
# Called automatically whenever the user presses left/right keys
# to nudge the current value left or right by some <offset>.  If the
# value actually changes, it generates a <<Value>> event to notify
# clients.
# ----------------------------------------------------------------------
#itcl::body Rappture::Videodial2::_navigate {offset} {
#    set index [lsearch -exact $_values $_current]
#    if {$index >= 0} {
#        incr index $offset
#        if {$index >= [llength $_values]} {
#            set index [expr {[llength $_values]-1}]
#        } elseif {$index < 0} {
#            set index 0
#        }
#
#        set newval [lindex $_values $index]
#        if {$newval != $_current} {
#            current $newval
#            _redraw
#
#            event generate $itk_component(hull) <<Value>>
#        }
#    }
#}


# ----------------------------------------------------------------------
# USAGE: _navigate <offset>
#
# Called automatically whenever the user presses left/right keys
# to nudge the current value left or right by some <offset>.  If the
# value actually changes, it generates a <<Value>> event to notify
# clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_navigate {offset} {
    _goToFrame [expr [rel2ms ${_current}] + $offset]
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    _current [ms2rel $var]
}

# ----------------------------------------------------------------------
# USAGE: _fixOffsets
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::_fixOffsets {} {
    if {0 == $itk_option(-offset)} {
        return
    }
    set _offset_pos $itk_option(-offset)
    set _offset_neg [expr -1*$_offset_pos]
    bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate $_offset_neg]
    bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate $_offset_pos]
}

# ----------------------------------------------------------------------
# USAGE: ms2rel
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::ms2rel { value } {
    if { ${_max} > ${_min} } {
        return [expr {1.0 * ($value - ${_min}) / (${_max} - ${_min})}]
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: rel2ms
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial2::rel2ms { value } {
    return [expr $value * (${_max} - ${_min}) + ${_min}]
}

# ----------------------------------------------------------------------
# CONFIGURE: -thickness
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::thickness {
}

# ----------------------------------------------------------------------
# CONFIGURE: -font
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::font {
}

# ----------------------------------------------------------------------
# CONFIGURE: -foreground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::foreground {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialoutlinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::dialoutlinecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialfillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::dialfillcolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -padding
# This adds padding on left/right side of dial background.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::padding {
    if {[catch {winfo pixels $itk_component(hull) $itk_option(-padding)}]} {
        error "bad value \"$itk_option(-padding)\": should be size in pixels"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::variable {
    if {"" != $_variable} {
        upvar #0 $_variable var
        trace remove variable var write [itcl::code $this _fixValue]
    }

    set _variable $itk_option(-variable)

    if {"" != $_variable} {
        upvar #0 $_variable var
        trace add variable var write [itcl::code $this _fixValue]

        # sync to the current value of this variable
        if {[info exists var]} {
            _fixValue
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -offset
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::offset {
    if {![string is integer $itk_option(-offset)] &&
        ($itk_option(-offset) < 0)} {
        error "bad value \"$itk_option(-offset)\": should be >= 1"
    }
    _fixOffsets
}

# ----------------------------------------------------------------------
# CONFIGURE: -min
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::min {
    if {![string is integer $itk_option(-min)]} {
        error "bad value \"$itk_option(-min)\": should be an integer"
    }
    if {$itk_option(-min) < 0} {
        error "bad value \"$itk_option(-min)\": should be >= 0"
    }
    set _min $itk_option(-min)
    _redraw
}

# ----------------------------------------------------------------------
# CONFIGURE: -max
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::max {
    if {![string is integer $itk_option(-max)]} {
        error "bad value \"$itk_option(-max)\": should be an integer"
    }
    if {$itk_option(-max) < 0} {
        error "bad value \"$itk_option(-max)\": should be >= 0"
    }
    set _max $itk_option(-max)
    _redraw
}

# ----------------------------------------------------------------------
# CONFIGURE: -minortick
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::minortick {
    if {![string is integer $itk_option(-minortick)]} {
        error "bad value \"$itk_option(-minortick)\": should be an integer"
    }
    if {$itk_option(-minortick) <= 0} {
        error "bad value \"$itk_option(-minortick)\": should be > 0"
    }
    set _minortick $itk_option(-minortick)
    _redraw
}

# ----------------------------------------------------------------------
# CONFIGURE: -majortick
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial2::majortick {
    if {![string is integer $itk_option(-majortick)]} {
        error "bad value \"$itk_option(-majortick)\": should be an integer"
    }
    if {$itk_option(-majortick) <= 0} {
        error "bad value \"$itk_option(-majortick)\": should be > 0"
    }
    set _majortick $itk_option(-majortick)
    _redraw
}
