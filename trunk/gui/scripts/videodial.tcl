# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Videodial - selector, like the dial on a flow
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

option add *Videodial.dialProgressColor #6666cc widgetDefault
option add *Videodial.thickness 10 widgetDefault
option add *Videodial.length 2i widgetDefault
option add *Videodial.knobImage knob widgetDefault
option add *Videodial.knobPosition n@middle widgetDefault
option add *Videodial.dialOutlineColor black widgetDefault
option add *Videodial.dialFillColor white widgetDefault
option add *Videodial.lineColor gray widgetDefault
option add *Videodial.activeLineColor black widgetDefault
option add *Videodial.padding 0 widgetDefault
option add *Videodial.valueWidth 10 widgetDefault
option add *Videodial.valuePadding 0.1 widgetDefault
option add *Videodial.foreground black widgetDefault
option add *Videodial.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Videodial {
    inherit itk::Widget

    itk_option define -min min Min 0
    itk_option define -max max Max 1
    itk_option define -minortick minortick Minortick 1
    itk_option define -majortick majortick Majortick 5
    itk_option define -variable variable Variable ""
    itk_option define -offset offset Offset 1

    itk_option define -thickness thickness Thickness 0
    itk_option define -length length Length 0
    itk_option define -padding padding Padding 0

    itk_option define -foreground foreground Foreground "black"
    itk_option define -dialoutlinecolor dialOutlineColor Color "black"
    itk_option define -dialfillcolor dialFillColor Color "white"
    itk_option define -dialprogresscolor dialProgressColor Color ""
    itk_option define -linecolor lineColor Color "black"
    itk_option define -activelinecolor activeLineColor Color "black"
    itk_option define -knobimage knobImage KnobImage ""
    itk_option define -knobposition knobPosition KnobPosition ""

    itk_option define -font font Font ""
    itk_option define -valuewidth valueWidth ValueWidth 0
    itk_option define -valuepadding valuePadding ValuePadding 0


    constructor {args} { # defined below }
    destructor { # defined below }

    public method current {value}
    public method clear {}
    public method mark {args}
    public method bball {}

    protected method _bindings {type args}
    protected method _redraw {}
    protected method _marker {tag action x y}
    protected method _setmark {type args}
    protected method _move {action x y}
    protected method _knob {x y}
    protected method _navigate {offset}
    protected method _fixSize {}
    protected method _fixMinorSize {}
    protected method _fixValue {args}
    protected method _fixOffsets {}

    private method _current {value}
    private method _see {item}
    private method _draw_major_timeline {}
    private method _draw_minor_timeline {}
    private method _offsetx {x}
    private method ms2rel {value}
    private method rel2ms {value}
    private common _click             ;# x,y point where user clicked
    private common _marks             ;# list of marks
    private variable _values ""       ;# list of all values on the dial
    private variable _val2label       ;# maps value => string label(s)
    private variable _current 0       ;# current value (where pointer is)
    private variable _variable ""     ;# variable associated with -variable
    private variable _knob ""         ;# image for knob
    private variable _spectrum ""     ;# width allocated for values
    private variable _activecolor ""  ;# width allocated for values
    private variable _vwidth 0        ;# width allocated for values
    private variable _offset_pos 1    ;#
    private variable _offset_neg -1   ;#
    private variable _imspace 10      ;# pixels between intermediate marks
    private variable _pmcnt 0         ;# particle marker count
    private variable _min 0
    private variable _max 1
    private variable _minortick 1
    private variable _majortick 5
}

itk::usual Videodial {
    keep -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::constructor {args} {

    # bind $itk_component(hull) <<Frame>> [itcl::code $this _updateCurrent]

    # ----------------------------------------------------------------------
    # controls for the major timeline.
    # ----------------------------------------------------------------------
    itk_component add majordial {
        canvas $itk_interior.majordial
    }

    bind $itk_component(majordial) <Configure> [itcl::code $this _draw_major_timeline]

    bind $itk_component(majordial) <ButtonPress-1> [itcl::code $this _knob %x %y]
    bind $itk_component(majordial) <B1-Motion> [itcl::code $this _knob %x %y]
    bind $itk_component(majordial) <ButtonRelease-1> [itcl::code $this _knob %x %y]

    #bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate $_offset_neg]
    #bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate $_offset_pos]

    $itk_component(majordial) bind  "knob" <Enter> \
        [list $itk_component(majordial) configure -cursor sb_h_double_arrow]
    $itk_component(majordial) bind  "knob" <Leave> \
        [list $itk_component(majordial) configure -cursor ""]

    # ----------------------------------------------------------------------
    # controls for the minor timeline.
    # ----------------------------------------------------------------------
    itk_component add minordial {
        canvas $itk_interior.minordial -background blue
    }


    bind $itk_component(minordial) <Configure> [itcl::code $this _draw_minor_timeline]

    bind $itk_component(minordial) <ButtonPress-1> [itcl::code $this _move click %x %y]
    bind $itk_component(minordial) <B1-Motion> [itcl::code $this _move drag %x %y]
    bind $itk_component(minordial) <ButtonRelease-1> [itcl::code $this _move release %x %y]

    # ----------------------------------------------------------------------
    # place controls in widget.
    # ----------------------------------------------------------------------

    blt::table $itk_interior \
        0,0 $itk_component(majordial) -fill x \
        1,0 $itk_component(minordial) -fill x

    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize none
    blt::table configure $itk_interior r1 -resize none


    eval itk_initialize $args

    $itk_component(majordial) configure -background green
    $itk_component(minordial) configure -background cyan

    #$itk_component(majordial) configure -relief sunken -borderwidth 1
    #$itk_component(minordial) configure -relief sunken -borderwidth 1

    _fixSize
    _fixOffsets
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::destructor {} {
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
itcl::body Rappture::Videodial::current {value} {
    if {"" == $value} {
        return
    }
    _current [ms2rel $value]
    # event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _current ?<value>?
#
# Clients use this to set a new value for the dial.  Values are always
# sorted in order along the dial.  If the value is not specified,
# then it is created automatically based on the number of elements
# on the dial.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_current {relval} {
    if { $relval < 0.0 } {
        set relval 0.0
    }
    if { $relval > 1.0 } {
        set relval 1.0
    }
    set _current $relval

    after cancel [itcl::code $this _draw_major_timeline]
    after idle [itcl::code $this _draw_major_timeline]

    # update the current marker and move the canvas so current is centered
    set framenum [expr round([rel2ms $_current])]
    #_see "frame$framenum"
    #mark current $framenum
    after idle [itcl::code $this _see "frame$framenum"]
    after idle [_setmark current $framenum]

    # update the upvar variable
    if { $_variable != "" } {
        upvar #0 $_variable var
        set var $framenum
    }
}

# ----------------------------------------------------------------------
# USAGE: _bindings <type> ?args?
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_bindings {type args} {
    switch -- $type {
        "marker" {
            set tag [lindex $args 0]
            bind $itk_component(minordial) <ButtonPress-1> [itcl::code $this _marker $tag click %x %y]
            bind $itk_component(minordial) <B1-Motion> [itcl::code $this _marker $tag drag %x %y]
            bind $itk_component(minordial) <ButtonRelease-1> [itcl::code $this _marker $tag release %x %y]
            $itk_component(minordial) configure -cursor hand2
        }
        "timeline" {
            bind $itk_component(minordial) <ButtonPress-1> [itcl::code $this _move click %x %y]
            bind $itk_component(minordial) <B1-Motion> [itcl::code $this _move drag %x %y]
            bind $itk_component(minordial) <ButtonRelease-1> [itcl::code $this _move release %x %y]
            $itk_component(minordial) configure -cursor ""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: mark <property> <args>
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::mark {property args} {
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
# USAGE: _setmark <type> ?[-xcoord|-tag]? <where>
#
# Clients use this to add a mark to the timeline
#   type can be any one of loopstart, loopend, particle, arrow
#   where is interpreted based on the preceeding flag if available.
#       in the default case, <where> is interpreted as a frame number
#       or "current". if the -xcoord flag is provided, where is
#       interpreted as the x coordinate of where to center the marker.
#       -xcoord should only be used for temporary placement of a
#       marker. when -xcoord is used, the marker is placed exactly at
#       the provided x coordinate, and is not associated with any
#       frame. It's purpose is mainly for <B1-Motion> events.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_setmark {type args} {

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
                -outline black -fill black -tags $tag

            $c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
            $c bind $tag <Leave> [itcl::code $this _bindings timeline]

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

            _fixMinorSize
        }
        "loopend" {
            # add loopend marker

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
                -outline black -fill black -tags $tag

            $c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
            $c bind $tag <Leave> [itcl::code $this _bindings timeline]

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

            _fixMinorSize
        }
        "particle*" {
            set radius 3
            set pmx0 $frx0
            set pmy0 [expr {$cy1+5}]
            set coords [list [expr $pmx0-$radius] [expr $pmy0-$radius] \
                             [expr $pmx0+$radius] [expr $pmy0+$radius]]

            set tag $type
            $c create oval $coords \
                -fill green \
                -outline black \
                -width 1 \
                -tags $tag

            #$c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
            #$c bind $tag <Leave> [itcl::code $this _bindings timeline]

            if {[string compare "" $where] != 0} {
                set _marks($type) $where
            }

            _fixMinorSize

        }
        "arrow" {
            set radius 3
            set amx0 $frx0
            set amy0 [expr {$cy1+15}]
            set coords [list [expr $amx0-$radius] [expr $amy0-$radius] \
                             [expr $amx0+$radius] [expr $amy0+$radius]]

            set tag $type
            $c create line $coords \
                -fill red \
                -width 3  \
                -tags $tag

            #$c bind $tag <Enter> [itcl::code $this _bindings marker $tag]
            #$c bind $tag <Leave> [itcl::code $this _bindings timeline]

            if {[string compare "" $where] != 0} {
                set _marks($type) $where
            }

            _fixMinorSize
        }
        "current" {

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
                -outline black -fill red -tags $tag
            $c create line $cmx0 $cmy0 $cmx0 $cy1 -fill red -tags $tag

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
# USAGE: _draw_major_timeline
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_draw_major_timeline {} {
    set c $itk_component(majordial)
    $c delete all

    set fg $itk_option(-foreground)

    set w [winfo width $c]
    set h [winfo height $c]
    set p [winfo pixels $c $itk_option(-padding)]
    set t [expr {$itk_option(-thickness)+1}]
    # FIXME: hack to get the reduce spacing in widget
    set y1 [expr {$h-2}]

    if {"" != $_knob} {
        set kw [image width $_knob]
        set kh [image height $_knob]

        # anchor refers to where on knob
        # top/middle/bottom refers to where on the dial
        # leave room for the bottom of the knob if needed
        switch -- $itk_option(-knobposition) {
            n@top - nw@top - ne@top {
                set extra [expr {$t-$kh}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }
            n@middle - nw@middle - ne@middle {
                set extra [expr {int(ceil($kh-0.5*$t))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }
            n@bottom - nw@bottom - ne@bottom {
               set y1 [expr {$y1-$kh}]
            }

            e@top - w@top - center@top -
            e@bottom - w@bottom - center@bottom {
                set extra [expr {int(ceil(0.5*$kh))}]
                set y1 [expr {$y1-$extra}]
            }
            e@middle - w@middle - center@middle {
                set extra [expr {int(ceil(0.5*($kh-$t)))}]
                if {$extra < 0} {set extra 0}
                set y1 [expr {$y1-$extra}]
            }

            s@top - sw@top - se@top -
            s@middle - sw@middle - se@middle -
            s@bottom - sw@bottom - se@bottom {
                set y1 [expr {$y1-1}]
            }
        }
    }
    set y0 [expr {$y1-$t}]
    set x0 [expr {$p+1}]
    set x1 [expr {$w-$_vwidth-$p-4}]

    # draw the background rectangle for the major time line
    $c create rectangle $x0 $y0 $x1 $y1 \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor) \
        -tags "majorbg"

    # draw the optional progress bar for the major time line,
    # from start to current
    if {"" != $itk_option(-dialprogresscolor) } {
        set xx1 [expr {$_current*($x1-$x0) + $x0}]
        $c create rectangle [expr {$x0+1}] [expr {$y0+3}] $xx1 [expr {$y1-2}] \
            -outline "" -fill $itk_option(-dialprogresscolor)
    }

    regexp {([nsew]+|center)@} $itk_option(-knobposition) match anchor
    switch -glob -- $itk_option(-knobposition) {
        *@top    { set kpos $y0 }
        *@middle { set kpos [expr {int(ceil(0.5*($y1+$y0)))}] }
        *@bottom { set kpos $y1 }
    }

    set x [expr {$_current*($x1-$x0) + $x0}]

    set color $_activecolor
    set thick 3
    if {"" != $color} {
        $c create line $x [expr {$y0+1}] $x $y1 -fill $color -width $thick
    }

    $c create image $x $kpos -anchor $anchor -image $_knob -tags "knob"
}

# ----------------------------------------------------------------------
# USAGE: bball
#   debug function to print out the bounding box information for
#   minor dial
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::bball {} {
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
# USAGE: _draw_minor_timeline
#
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_draw_minor_timeline {} {
    set c $itk_component(minordial)
    $c delete all

    set fg $itk_option(-foreground)

    set w [winfo width $c]
    set h [winfo height $c]
    set p [winfo pixels $c $itk_option(-padding)]
    set t [expr {$itk_option(-thickness)+1}]
    set y1 [expr {$h-1}]
    set y0 [expr {$y1-$t}]
    set x0 [expr {$p+1}]
    set x1 [expr {$w-$_vwidth-$p-4}]


    # draw the background rectangle for the minor time line
    $c create rectangle $x0 $y0 $x1 $y1 \
        -outline $itk_option(-dialoutlinecolor) \
        -fill $itk_option(-dialfillcolor) \
        -tags "imbox"

    # add intermediate marks between markers
    set imw 1.0                                 ;# intermediate mark width

    set imsh [expr {$t/3.0}]                    ;# intermediate mark short height
    set imsy0 [expr {$y0+(($t-$imsh)/2.0)}]     ;# precalc'd imark short y0 coord
    set imsy1 [expr {$imsy0+$imsh}]             ;# precalc'd imark short y1 coord

    set imlh [expr {$t*2.0/3.0}]                ;# intermediate mark long height
    set imly0 [expr {$y0+(($t-$imlh)/2.0)}]     ;# precalc'd imark long y0 coord
    set imly1 [expr {$imly0+$imlh}]             ;# precalc'd imark long y1 coord

    set imty [expr {$y0-5}]                     ;# height of marker value

    set imx $x0
    for {set i [expr {int(${_min})}]} {$i <= ${_max}} {incr i} {
        if {($i%${_majortick}) == 0} {
            # draw major tick
            $c create line $imx $imly0 $imx $imly1 \
                -fill red \
                -width $imw \
                -tags [list longmark-c imark-c "frame$i"]

            $c create text $imx $imty -anchor center -text $i \
                -font $itk_option(-font) -tags "frame$i"

            set imx [expr $imx+${_imspace}]
        } elseif {($i%${_minortick}) == 0 } {
            # draw minor tick
            $c create line $imx $imsy0 $imx $imsy1 \
                -fill blue \
                -width $imw \
                -tags [list shortmark-c imark-c "frame$i"]

            set imx [expr $imx+${_imspace}]
        }
    }


    # calculate the height of the intermediate tick marks
    # and frame numbers on our canvas, resize the imbox
    # to include both of them.
    set box [$c bbox "all"]
    if {![llength $box]} {
        set box [list 0 0 0 0]
    }
    foreach {x0 y0 x1 y1} $box break
    $c coords "imbox" $box

    # add any marks that the user previously specified
    foreach n [array names _marks] {
        # mark $n -tag $_marks($n)
        _setmark $n $_marks($n)
    }

    _fixMinorSize
}


# ----------------------------------------------------------------------
# USAGE: _fixMinorSize
#
# Used internally to compute the height of the minor dial based
# on the items placed on the canvas
#
# FIXME: instead of calling this in the mark command, figure out how to
#   make the canvas the correct size to start with
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_fixMinorSize {} {
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
# USAGE: _redraw
#
# Called automatically whenever the widget changes size to redraw
# all elements within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_redraw {} {
#    _draw_major_timeline
#    _draw_minor_timeline
}

# ----------------------------------------------------------------------
# USAGE: _knob <x> <y>
#
# Called automatically whenever the user clicks or drags on the widget
# to select a value.  Moves the current value to the one nearest the
# click point.  If the value actually changes, it generates a <<Value>>
# event to notify clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_knob {x y} {
    set c $itk_component(majordial)
    set w [winfo width $c]
    set h [winfo height $c]
    set x0 1
    set x1 [expr {$w-$_vwidth-4}]
    focus $itk_component(hull)
    if {$x >= $x0 && $x <= $x1} {
        current [rel2ms [expr double($x - $x0) / double($x1 - $x0)]]
    }
}

# ----------------------------------------------------------------------
# USAGE: _offsetx <x>
#
# Calculate an x coordinate that has been offsetted by a scrolled canvas
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_offsetx {x} {
    set c $itk_component(minordial)
    set w [lindex [$c cget -scrollregion] 2]
    set x0 [lindex [$c xview] 0]
    set offset [expr {$w*$x0}]
    set x [expr {$x+$offset}]
    return $x
}

# ----------------------------------------------------------------------
# USAGE: _marker <tag> click <x> <y>
#        _marker <tag> drag <x> <y>
#        _marker <tag> release <x> <y>
#
# Called automatically whenever the user clicks or drags on a marker
# widget.  Moves the selected marker to the next nearest tick mark.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_marker {tag action x y} {
    set c $itk_component(minordial)
    set x [_offsetx $x]
    switch $action {
        "click" {
        }
        "drag" {
            _setmark $tag -xcoord $x
            # if we are too close to the edge, scroll the canvas.
            # $c xview scroll $dist "unit"
        }
        "release" {
            # on release, snap to the closest imark
            foreach {junk y0 junk y1} [$c bbox "imark-c"] break
            set id ""
            foreach item [$c find enclosed [expr {$x-((${_imspace}+1)/2.0)}] $y0 \
                                           [expr {$x+((${_imspace}+1)/2.0)}] $y1] {
                set itemtags [$c gettags $item]
                if {[lsearch -exact $itemtags "imark-c"] != -1} {
                    set id [lsearch -inline -regexp $itemtags {frame[0-9]}]
                    break
                }
            }
            if {[string compare "" $id] == 0} {
                # something went wrong
                # we should have found an imark with
                # an associated "frame#" tag to snap to
                # bailout
                error "could not find an intermediate mark to snap marker to"
            }

            _setmark $tag -tag $id

            # take care of cases where the mouse leaves the marker's boundries
            # before the button-1 has been released. we check if the last
            # coord was within the bounds of the marker. if not, we manually
            # generate the "Leave" event.
            set leave 1
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
# Called automatically whenever the user clicks or drags on the widget
# to select a value.  Moves the current value to the one nearest the
# click point.  If the value actually changes, it generates a <<Value>>
# event to notify clients.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_move {action x y} {
    switch $action {
        "click" {
            set _click(x) $x
            set _click(y) $y
        }
        "drag" {
            set c $itk_component(minordial)
            set dist [expr $_click(x)-$x]
            $c xview scroll $dist "units"
            set _click(x) $x
            set _click(y) $y
        }
        "release" {
            _move drag $x $y
            catch {unset _click}
        }
    }
}

## from http://tcl.sourceforge.net/faqs/tk/#canvas/see
## "see" method alternative for canvas
## Aligns the named item as best it can in the middle of the screen
##
## item - a canvas tagOrId
itcl::body Rappture::Videodial::_see {item} {
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
#itcl::body Rappture::Videodial::_navigate {offset} {
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
itcl::body Rappture::Videodial::_navigate {offset} {
    _current [ms2rel [expr $_current + $offset]]
    event generate $itk_component(hull) <<Value>>
}


# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to compute the overall size of the widget based
# on the -thickness and -length options.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_fixSize {} {
    set h [winfo pixels $itk_component(hull) $itk_option(-thickness)]

    if {"" != $_knob} {
        set kh [image height $_knob]

        switch -- $itk_option(-knobposition) {
            n@top - nw@top - ne@top -
            s@bottom - sw@bottom - se@bottom {
                if {$kh > $h} { set h $kh }
            }
            n@middle - nw@middle - ne@middle -
            s@middle - sw@middle - se@middle {
                set h [expr {int(ceil(0.5*$h + $kh))}]
            }
            n@bottom - nw@bottom - ne@bottom -
            s@top - sw@top - se@top {
                set h [expr {$h + $kh}]
            }
            e@middle - w@middle - center@middle {
                set h [expr {(($h > $kh) ? $h : ($kh+1))}]
            }
            n@middle - ne@middle - nw@middle -
            s@middle - se@middle - sw@middle {
                set extra [expr {int(ceil($kh-0.5*$h))}]
                if {$extra < 0} { set extra 0 }
                set h [expr {$h+$extra}]
            }
        }
    }
    # FIXME: hack to get the reduce spacing in widget
    incr h -1

    set w [winfo pixels $itk_component(hull) $itk_option(-length)]

    # if the -valuewidth is > 0, then make room for the value
    if {$itk_option(-valuewidth) > 0} {
        set charw [font measure $itk_option(-font) "n"]
        set _vwidth [expr {$itk_option(-valuewidth)*$charw}]
        set w [expr {$w+$_vwidth+4}]
    } else {
        set _vwidth 0
    }

    $itk_component(majordial) configure -width $w -height $h

#    # resize the height of the minor canvas to include everything we know about
#    set box [$itk_component(minordial) bbox "all"]
#    if {![llength $box]} {
#        set box [list 0 0 0 0]
#    }
#    foreach {cx0 cy0 cx1 cy1} $box break
#    set h [expr $cy1-$cy0+1]
#    $itk_component(minordial) configure -height $h
}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Videodial::_fixValue {args} {
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
itcl::body Rappture::Videodial::_fixOffsets {} {
    if {0 == $itk_option(-offset)} {
        return
    }
    set _offset_pos $itk_option(-offset)
    set _offset_neg [expr -1*$_offset_pos]
    bind $itk_component(hull) <KeyPress-Left> [itcl::code $this _navigate $_offset_neg]
    bind $itk_component(hull) <KeyPress-Right> [itcl::code $this _navigate $_offset_pos]
}

itcl::body Rappture::Videodial::ms2rel { value } {
    if { ${_max} > ${_min} } {
        return [expr {1.0 * ($value - ${_min}) / (${_max} - ${_min})}]
    }
    return 0
}

itcl::body Rappture::Videodial::rel2ms { value } {
    return [expr $value * (${_max} - ${_min}) + ${_min}]
}

# ----------------------------------------------------------------------
# CONFIGURE: -thickness
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::thickness {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -length
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::length {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -font
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::font {
    _fixSize
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuewidth
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::valuewidth {
    if {![string is integer $itk_option(-valuewidth)]} {
        error "bad value \"$itk_option(-valuewidth)\": should be integer"
    }
    _fixSize
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -foreground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::foreground {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialoutlinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::dialoutlinecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialfillcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::dialfillcolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -dialprogresscolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::dialprogresscolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -linecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::linecolor {
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -activelinecolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::activelinecolor {
    set val $itk_option(-activelinecolor)
    if {[catch {$val isa ::Rappture::Spectrum} valid] == 0 && $valid} {
        set _spectrum $val
        set _activecolor ""
    } elseif {[catch {winfo rgb $itk_component(hull) $val}] == 0} {
        set _spectrum ""
        set _activecolor $val
    } elseif {"" != $val} {
        error "bad value \"$val\": should be Spectrum object or color"
    }
    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -knobimage
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::knobimage {
    if {[regexp {^image[0-9]+$} $itk_option(-knobimage)]} {
        set _knob $itk_option(-knobimage)
    } elseif {"" != $itk_option(-knobimage)} {
        set _knob [Rappture::icon $itk_option(-knobimage)]
    } else {
        set _knob ""
    }
    _fixSize

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -knobposition
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::knobposition {
    if {![regexp {^([nsew]+|center)@(top|middle|bottom)$} $itk_option(-knobposition)]} {
        error "bad value \"$itk_option(-knobposition)\": should be anchor@top|middle|bottom"
    }
    _fixSize

    after cancel [itcl::code $this _redraw]
    after idle [itcl::code $this _redraw]
}

# ----------------------------------------------------------------------
# CONFIGURE: -padding
# This adds padding on left/right side of dial background.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::padding {
    if {[catch {winfo pixels $itk_component(hull) $itk_option(-padding)}]} {
        error "bad value \"$itk_option(-padding)\": should be size in pixels"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -valuepadding
# This shifts min/max limits in by a fraction of the overall size.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::valuepadding {
    if {![string is double $itk_option(-valuepadding)]
          || $itk_option(-valuepadding) < 0} {
        error "bad value \"$itk_option(-valuepadding)\": should be >= 0.0"
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::variable {
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
itcl::configbody Rappture::Videodial::offset {
    if {![string is double $itk_option(-offset)]} {
        error "bad value \"$itk_option(-offset)\": should be >= 0.0"
    }
    _fixOffsets
}

# ----------------------------------------------------------------------
# CONFIGURE: -min
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::min {
    if {![string is integer $itk_option(-min)]} {
        error "bad value \"$itk_option(-min)\": should be an integer"
    }
    if {$itk_option(-min) < 0} {
        error "bad value \"$itk_option(-min)\": should be >= 0"
    }
    set _min $itk_option(-min)
    _draw_minor_timeline
}

# ----------------------------------------------------------------------
# CONFIGURE: -max
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::max {
    if {![string is integer $itk_option(-max)]} {
        error "bad value \"$itk_option(-max)\": should be an integer"
    }
    if {$itk_option(-max) < 0} {
        error "bad value \"$itk_option(-max)\": should be >= 0"
    }
    set _max $itk_option(-max)
    _draw_minor_timeline
}

# ----------------------------------------------------------------------
# CONFIGURE: -minortick
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::minortick {
    if {![string is integer $itk_option(-minortick)]} {
        error "bad value \"$itk_option(-minortick)\": should be an integer"
    }
    if {$itk_option(-minortick) <= 0} {
        error "bad value \"$itk_option(-minortick)\": should be > 0"
    }
    set _minortick $itk_option(-minortick)
    _draw_minor_timeline
}

# ----------------------------------------------------------------------
# CONFIGURE: -majortick
# ----------------------------------------------------------------------
itcl::configbody Rappture::Videodial::majortick {
    if {![string is integer $itk_option(-majortick)]} {
        error "bad value \"$itk_option(-majortick)\": should be an integer"
    }
    if {$itk_option(-majortick) <= 0} {
        error "bad value \"$itk_option(-majortick)\": should be > 0"
    }
    set _majortick $itk_option(-majortick)
    _draw_minor_timeline
}
