# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: videopreview - previewing movies
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itk
package require BLT
package require Img
package require Rappture
package require RapptureGUI

option add *Video.width 300 widgetDefault
option add *Video.height 300 widgetDefault
option add *Video.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VideoPreview {
    inherit itk::Widget

    itk_option define -width width Width -1
    itk_option define -height height Height -1
    itk_option define -variable variable Variable ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method load {type data}
    public method loadcb {args}
    public method video {args}
    public method query {type}

    protected method Play {}
    protected method Seek {n}
    protected method fixSize {}
    protected method _fixValue {args}
    protected method Upload {args}
    protected method eventually {args}


    private common   _settings
    private common   _pendings

    private variable _variable ""   ;# quick way to change movie
    private variable _path  ""      ;# path of movie
    private variable _width -1      ;# start x for rubberbanding
    private variable _height -1     ;# start x for rubberbanding
    private variable _movie ""      ;# movie we grab images from
    private variable _lastFrame 0   ;# last frame in the movie
    private variable _imh ""        ;# current image being displayed
    private variable _id ""         ;# id of the next play command from after
    private variable _framerate 30  ;# video frame rate
    private variable _mspf  7       ;# milliseconds per frame wait time
    private variable _ofrd  19      ;# observed frame retrieval delay of
                                    ;# underlying c lib in milliseconds
    private variable _delay  0      ;# milliseconds between play calls
    private variable _nextframe 0   ;#
}


itk::usual VideoPreview {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::constructor {args} {

    array set _settings [subst {
        $this-framenum          0
        $this-play              0
    }]

    array set _pendings [subst {
        play 0
    }]

    itk_component add main {
        canvas $itk_interior.main \
            -background black
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    bind $itk_component(main) <Configure> [itcl::code $this fixSize]

    # hold the video frames in an image on the canvas
    set _imh [image create photo]
    $itk_component(main) create image 0 0 -anchor nw -image $_imh

    # setup movie controls
    itk_component add moviecontrols {
        frame $itk_interior.moviecontrols
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    # setup frame number frame
    itk_component add frnumfr {
        frame $itk_component(moviecontrols).frnumfr
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    set imagesDir [file join $RapptureGUI::library scripts images]


    # Play
    itk_component add play {
        Rappture::PushButton $itk_component(moviecontrols).play \
            -onimage [Rappture::icon flow-pause] \
            -offimage [Rappture::icon flow-play] \
            -disabledimage [Rappture::icon flow-play] \
            -variable [itcl::scope _settings($this-play)] \
            -command [itcl::code $this video play]
    }
    set fg [option get $itk_component(hull) font Font]
    Rappture::Tooltip::for $itk_component(play) \
        "Play/Pause movie"
    $itk_component(play) disable

    # Video Dial Major
    itk_component add dialmajor {
        Rappture::Videodial1 $itk_component(moviecontrols).dialmajor \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0 -max 1 \
            -variable [itcl::scope _settings($this-framenum)] \
            -dialoutlinecolor black \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dialmajor) current 0
    bind $itk_component(dialmajor) <<Value>> [itcl::code $this video update]

    itk_component add framenumlabel {
        label $itk_component(frnumfr).framenuml -text "Frame:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    # Current Frame Number
    itk_component add framenum {
        label $itk_component(frnumfr).framenum \
            -width 5 -font "arial 9" \
            -textvariable [itcl::scope _settings($this-framenum)]
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(framenum) \
        "Current frame number"


    pack $itk_component(framenumlabel) -side left
    pack $itk_component(framenum) -side right


    blt::table $itk_component(moviecontrols) \
        0,0 $itk_component(play) -padx {2 0} \
        0,1 $itk_component(dialmajor) -fill x -padx {2 2} \
        0,2 $itk_component(frnumfr) -padx {2 4}

    blt::table configure $itk_component(moviecontrols) c* -resize none
    blt::table configure $itk_component(moviecontrols) c1 -resize both
    blt::table configure $itk_component(moviecontrols) r0 -pady 1


    blt::table $itk_interior \
        0,0 $itk_component(main) -fill both \
        1,0 $itk_component(moviecontrols) -fill x
    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize both
    blt::table configure $itk_interior r1 -resize both

    eval itk_initialize $args

    $itk_component(main) configure -background black
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::destructor {} {
    array unset _settings $this-*
    if {[info exists _imh]} {
        image delete $_imh
    }
}

# ----------------------------------------------------------------------
# load - load a video file
#   type - type of data, "data" or "file"
#   data - what to load.
#       if type == "data", data is treated like binary data
#       if type == "file", data is treated like the name of a file
#           and is opened and then loaded.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::load {type data} {

    # open the file

    set fname ""
    switch $type {
        "data" {
            set fname "/tmp/tmpVV[pid].video"
            set fid [open $fname "w"]
            fconfigure $fid -translation binary -encoding binary
            puts $fid $data
            close $fid
            set type "file"
            set data $fname
        }
        "file" {
            # do nothing
        }
        default {
            error "bad value: \"$type\": should be \"load \[data|file\] <data>\""
        }
    }

    video stop

    if {([info exists _movie]) && ("" != ${_movie})} {
        ${_movie} release
    }

    set _movie [Rappture::Video $type $data]
    file delete $fname
    set _framerate [${_movie} framerate]
    set _mspf [expr round(((1.0/${_framerate})*1000))]
    set _delay [expr {${_mspf} - ${_ofrd}}]

    video seek 0

    # update the dial and framenum widgets
    set _settings($this-framenum) 0

    # setup the image display

    foreach {w h} [query dimensions] break
    if {${_width} == -1} {
        set _width $w
    }
    if {${_height} == -1} {
        set _height $h
    }

    set _lastFrame [${_movie} get position end]

    # update the dial with video information
    $itk_component(dialmajor) configure -min 0 -max ${_lastFrame}
    $itk_component(play) enable

    fixSize
}

# ----------------------------------------------------------------------
# loadcb - load callback
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::loadcb {args} {
    video stop
    Rappture::filexfer::upload {piv tool} {id label desc} [itcl::code $this Upload]
}

# ----------------------------------------------------------------------
# Upload -
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::Upload {args} {
    array set data $args
    video stop

    if {[info exists data(error)]} {
        Rappture::Tooltip::cue $itk::component(main) $data(error)
        puts stderr $data(error)
    }

    if {[info exists data(path)] && [info exists data(data)]} {
        Rappture::Tooltip::cue hide  ;# take down note about the popup window

        # load data
        load "data" $data(data)
    }

}

# ----------------------------------------------------------------------
# USAGE: _fixValue ?<name1> <name2> <op>?
#
# Invoked automatically whenever the -variable associated with this
# widget is modified.  Copies the value to the current settings for
# the widget.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::_fixValue {args} {
    if {"" == $itk_option(-variable)} {
        return
    }
    upvar #0 $itk_option(-variable) var
    if {[file readable $var]} {
        # load and start playing the video
        set _path $var
        load file ${_path}
        set _settings($this-play) 1
        video play
    }
}

# ----------------------------------------------------------------------
# fixSize
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::fixSize {} {

    if {[string compare "" ${_movie}] == 0} {
        return
    }

    if {![winfo ismapped $itk_component(hull)]} {
        return
    }

    set cw [winfo width $itk_component(main)]
    set ch [winfo height $itk_component(main)]

    # FIXME: right now we only deal with 16:9 ratio videos
    # always keep the aspect ratio correct
    set _width [expr int(int($cw/16.0)*16.0)]
    set _height [expr int(${_width}/(16.0/9.0))]
    if {${_height} > $ch} {
        # if height is limiting us, shrink some more
        set _height [expr int(int($ch/9.0)*9.0)]
        set _width [expr int(${_height}*(16.0/9.0))]
    }

    # get an image with the new size
    ${_imh} blank
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # make the canvas fit the image
    $itk_component(main) configure -width ${_width} -height ${_height}
}

# ----------------------------------------------------------------------
# video - play, stop, rewind, fastforward the video
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::video { args } {
    set option [lindex $args 0]
    switch -- $option {
        "play" {
            if {$_settings($this-play) == 1} {
                eventually play
            } else {
                # pause/stop
                after cancel $_id
                set _pendings(play) 0
                set _settings($this-play) 0
            }
        }
        "seek" {
            Seek [lreplace $args 0 0]
        }
        "stop" {
            after cancel $_id
            set _settings($this-play) 0
        }
        "update" {
            # eventually seek [expr round($_settings($this-framenum))]
            Seek [expr round($_settings($this-framenum))]
        }
        default {
            error "bad option \"$option\": should be play, stop, toggle, position, or reset."
        }
    }
}

# ----------------------------------------------------------------------
# query - query things about the video
#
#   dimensions  - returns width and height as a list
#   frames      - number of frames in video (last frame + 1)
#   framenum    - current position
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::query { type } {
    set ret ""
    switch -- $type {
        "dimensions" {
            set ret [${_movie} size]
        }
        "frames" {
            set ret [expr [${_movie} get position end] + 1]
        }
        "framenum" {
            set ret [${_movie} get position cur]
        }
        default {
            error "bad type \"$type\": should be dimensions, frames, framenum."
        }
    }
    return $ret
}

# ----------------------------------------------------------------------
# Play - get the next video frame
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::Play {} {

    set cur ${_nextframe}

    # time how long it takes to retrieve the next frame
    set _ofrd [time {
        # use seek instead of next fxn incase the ${_nextframe} is
        # not the current frame+1. this happens when we skip frames
        # because the underlying c lib is too slow at reading.
        $_movie seek $cur
        $_imh put [${_movie} get image ${_width} ${_height}]
    } 1]
    regexp {(\d+\.?\d*) microseconds per iteration} ${_ofrd} match _ofrd
    set _ofrd [expr {round(${_ofrd}/1000)}]

    # calculate the delay we shoud see
    # between frames being placed on screen
    # taking into account the cost of retrieving the frame
    set _delay [expr {${_mspf}-${_ofrd}}]
    if {0 > ${_delay}} {
        set _delay 0
    }

    set cur [${_movie} get position cur]

    # update the dial and framenum widgets
    set _settings($this-framenum) $cur

    # no play cmds pending
    set _pendings(play) 0

    # schedule the next frame to be displayed
    if {$cur < ${_lastFrame}} {
        set _id [after ${_delay} [itcl::code $this eventually play]]
    } else {
        video stop
    }

    event generate $itk_component(hull) <<Frame>>
}


# ----------------------------------------------------------------------
# Seek - go to a frame in the video
#   Seek +5
#   Seek -5
#   Seek 35
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::Seek {args} {
    set val [lindex $args 0]
    if {"" == $val} {
        error "bad value: \"$val\": should be \"seek value\""
    }
    set cur [${_movie} get position cur]
    if {[string compare $cur $val] == 0} {
        # already at the frame to seek to
        return
    }
    ${_movie} seek $val
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # update the dial and framenum widgets
    set _settings($this-framenum) [${_movie} get position cur]
    event generate $itk_component(main) <<Frame>>

}


# ----------------------------------------------------------------------
# eventually -
#   seek
#   play
# ----------------------------------------------------------------------
itcl::body Rappture::VideoPreview::eventually {args} {
    set option [lindex $args 0]
    switch -- $option {
        "seek" {
            if {$_pendings(seek) == 0} {
                # no seek pending, schedule one
                set $_pendings(seek) 1
                after idle [itcl::code $this Seek [lindex $args 1]]
            } else {
                # there is a seek pending, update its seek value
            }
        }
        "play" {
            if {0 == $_pendings(play)} {
                # no play pending schedule one
                set _pendings(play) 1
                set _nextframe [expr {[${_movie} get position cur] + 1}]
                after idle [itcl::code $this Play]
            } else {
                # there is a play pending, update its frame value
                incr _nextframe
            }
        }
        default {
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURE: -variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoPreview::variable {
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
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoPreview::width {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-width)] == 0} {
        error "bad value: \"$itk_option(-width)\": width should be an integer"
    }
    set _width $itk_option(-width)
    after idle [itcl::code $this fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoPreview::height {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-height)] == 0} {
        error "bad value: \"$itk_option(-height)\": height should be an integer"
    }
    set _height $itk_option(-height)
    after idle [itcl::code $this fixSize]
}
