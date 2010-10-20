# ----------------------------------------------------------------------
#  COMPONENT: video - viewing movies
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
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
option add *Video*cursor crosshair widgetDefault
option add *Video.height 300 widgetDefault
option add *Video.foreground black widgetDefault
option add *Video.controlBackground gray widgetDefault
option add *Video.controlDarkBackground #999999 widgetDefault
option add *Video.plotBackground black widgetDefault
option add *Video.plotForeground white widgetDefault
option add *Video.plotOutline gray widgetDefault
option add *Video.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VideoScreen {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""
    itk_option define -plotoutline plotOutline PlotOutline ""
    itk_option define -width width Width -1
    itk_option define -height height Height -1

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
    protected method Upload {args}


    private common   _settings

    private variable _width -1      ;# start x for rubberbanding
    private variable _height -1     ;# start x for rubberbanding
    private variable _movie ""      ;# movie we grab images from
    private variable _lastFrame 0   ;# last frame in the movie
    private variable _imh ""        ;# current image being displayed
    private variable _id ""         ;# id of the next play command from after
    private variable _framerate 30  ;# video frame rate
    private variable _mspf  7       ;# milliseconds per frame wait time
    private variable _waiting 0     ;# number of frames behind we are
}

itk::usual VideoScreen {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::constructor {args} {

    array set _settings [subst {
        $this-currenttime       0
        $this-framenum          0
        $this-loop              0
        $this-play              0
        $this-speed             1
    }]

    # Create flow controls...

    itk_component add main {
        canvas $itk_interior.main \
            -background black
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    # pack $itk_component(main) -side top -expand yes -fill both
    bind $itk_component(main) <Configure> [itcl::code $this fixSize]

    # setup movie controls
    itk_component add moviecontrols {
        frame $itk_interior.moviecontrols
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    # Rewind
    itk_component add rewind {
        button $itk_component(moviecontrols).rewind \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon video-rewind] \
            -command [itcl::code $this video seek 0]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(rewind) \
        "Rewind movie"

    # Seek back
    itk_component add seekback {
        button $itk_component(moviecontrols).seekback \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-rewind] \
            -command [itcl::code $this video seek -1]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(rewind) \
        "Seek backwards 1 frame"

    # Play
    itk_component add play {
        Rappture::PushButton $itk_component(moviecontrols).play \
            -onimage [Rappture::icon flow-pause] \
            -offimage [Rappture::icon flow-play] \
            -variable [itcl::scope _settings($this-play)] \
            -command [itcl::code $this video play]
    }
    set fg [option get $itk_component(hull) font Font]
    Rappture::Tooltip::for $itk_component(play) \
        "Play/Pause movie"

    # Seek forward
    itk_component add seekforward {
        button $itk_component(moviecontrols).seekforward \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-forward] \
            -command [itcl::code $this video seek +1]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(seekforward) \
        "Seek forward 1 frame"

    # Loop
    itk_component add loop {
        Rappture::PushButton $itk_component(moviecontrols).loop \
            -onimage [Rappture::icon flow-loop] \
            -offimage [Rappture::icon flow-loop] \
            -variable [itcl::scope _settings($this-loop)]
    }
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously between marked sections"

    itk_component add dial {
        Rappture::Videodial $itk_component(moviecontrols).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0 -max 1 \
            -minortick 1 -majortick 5 \
            -variable [itcl::scope _settings($this-currenttime)] \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dial) current 0
    bind $itk_component(dial) <<Value>> \
        [itcl::code $this video seek [expr round($_settings($this-currenttime))]]

    itk_component add framenumlabel {
        label $itk_component(moviecontrols).framenuml -text "Frame:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    # Current Frame Number
    #set ffont "arial 9"
    #set fwidth [calcLabelWidth $ffont]
    itk_component add framenum {
        label $itk_component(moviecontrols).framenum \
            -background white -font "arial 9" \
            -textvariable [itcl::scope _settings($this-framenum)]
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    #$itk_component(framenum) value 1
    #bind $itk_component(framenum) <<Value>> \
    #    [itcl::code $this video seek -framenum]
    Rappture::Tooltip::for $itk_component(framenum) \
        "Current frame number"

    itk_component add speedlabel {
        label $itk_component(moviecontrols).speedl -text "Speed:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    # Speed
    itk_component add speed {
        Rappture::Flowspeed $itk_component(moviecontrols).speed \
            -min 1 -max 10 -width 3 -font "arial 9"
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(speed) \
        "Change speed of movie"

    $itk_component(speed) value 1
    bind $itk_component(speed) <<Value>> [itcl::code $this video speed]


    blt::table $itk_component(moviecontrols) \
        0,0 $itk_component(rewind) -padx {3 0} \
        0,1 $itk_component(seekback) -padx {2 0} \
        0,2 $itk_component(play) -padx {2 0} \
        0,3 $itk_component(seekforward) -padx { 2 0} \
        0,4 $itk_component(loop) -padx {2 0} \
        0,5 $itk_component(dial) -fill x -padx {2 0 } \
        0,6 $itk_component(framenumlabel) -padx {2 0} \
        0,7 $itk_component(framenum) -padx { 0 0} \
        0,8 $itk_component(speed) -padx {2 3}

    blt::table configure $itk_component(moviecontrols) c* -resize none
    blt::table configure $itk_component(moviecontrols) c5 -resize both
    blt::table configure $itk_component(moviecontrols) r0 -pady 1


    blt::table $itk_interior \
        0,0 $itk_component(main) -fill both \
        1,0 $itk_component(moviecontrols) -fill x
    blt::table configure $itk_interior c* -resize both
    blt::table configure $itk_interior r0 -resize both
    blt::table configure $itk_interior r1 -resize none

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::destructor {} {
    array unset _settings $this-*
}

# ----------------------------------------------------------------------
# load - load a video file
#   type - type of data, "data" or "file"
#   data - what to load.
#       if type == "data", data is treated like binary data
#       if type == "file", data is treated like the name of a file
#           and is opened and then loaded.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::load {type data} {

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

    set _movie [Rappture::Video $type $data]
    file delete $fname
    set _framerate [${_movie} get framerate]
    set _mspf [expr round(((1.0/${_framerate})*1000)/pow(2,$_settings($this-speed)-1))]
    puts "framerate = ${_framerate}"
    puts "mspf = ${_mspf}"

    ${_movie} seek 0

    # setup the image display

    set _imh [image create photo]
    foreach {w h} [query dimensions] break
    if {${_width} == -1} {
        set _width $w
    }
    if {${_height} == -1} {
        set _height $h
    }
    $itk_component(main) create image 0 0 -anchor nw -image $_imh

    set _lastFrame [$_movie get position end]

    # update the dial with video information
    $itk_component(dial) configure -min 0 -max ${_lastFrame}

    fixSize
}

# ----------------------------------------------------------------------
# loadcb - load callback
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::loadcb {args} {

    Rappture::filexfer::upload {piv tool} {id label desc} [itcl::code $this Upload]

    #uplevel 1 [list $args]
}

# ----------------------------------------------------------------------
# Upload -
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Upload {args} {
    array set data $args

    if {[info exists data(error)]} {
        Rappture::Tooltip::cue $itk::component(main) $data(error)
        puts $data(error)
    }

    if {[info exists data(path)] && [info exists data(data)]} {
        Rappture::Tooltip::cue hide  ;# take down note about the popup window

        # load data
        load "data" $data(data)
    }

}


# ----------------------------------------------------------------------
# fixSize
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::fixSize {} {

    if {[string compare "" ${_movie}] == 0} {
        return
    }

    set _width [winfo width $itk_component(main)]
    set _height [winfo height $itk_component(main)]

    # get an image with the new size
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # fix the dimesions of the canvas
    #$itk_component(main) configure -width ${_width} -height ${_height}

    $itk_component(main) configure -scrollregion [$itk_component(main) bbox all]
}

# ----------------------------------------------------------------------
# video - play, stop, rewind, fastforward the video
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::video { args } {
    set option [lindex $args 0]
    switch -- $option {
        "play" {
            if {$_settings($this-play) == 1} {
                # disable seek while playing
                bind $itk_component(dial) <<Value>> ""
                Play
            } else {
                # pause/stop
                after cancel $_id
                set _settings($this-play) 0
                # enable seek while paused
                bind $itk_component(dial) <<Value>> \
                    [itcl::code $this video seek [expr round($_settings($this-currenttime))]]
            }
        }
        "seek" {
            Seek [lreplace $args 0 0]
        }
        "stop" {
            after cancel $_id
            set _settings($this-play) 0
        }
        "speed" {
            set _mspf [expr round(((1.0/${_framerate})*1000)/pow(2,$_settings($this-speed)-1))]
            puts "_mspf = ${_mspf}"
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
itcl::body Rappture::VideoScreen::query { type } {
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
itcl::body Rappture::VideoScreen::Play {} {


    # display the next frame
    $_movie next
    $_imh put [$_movie get image ${_width} ${_height}]
    set cur [$_movie get position cur]

    event generate $itk_component(hull) <<Frame>>

    # update the dial and framenum widgets
    $itk_component(dial) current $cur
    set _settings($this-framenum) $cur

    if {[expr $cur%100] == 0} {
        puts "after: [after info]"
        puts "id = ${_id}"
    }

    # schedule the next frame to be displayed
    if {$cur < ${_lastFrame}} {
        set _id [after ${_mspf} [itcl::code $this Play]]
    }
}


# ----------------------------------------------------------------------
# Seek - go to a frame in the video frame
#   Seek +5
#   Seek -5
#   Seek 35
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Seek {args} {
    set val [lindex $args 0]
    if {"" == $val} {
        error "bad value: \"$val\": should be \"seek value\""
    }
    ${_movie} seek $val
    ${_imh} put [${_movie} get image ${_width} ${_height}]
}

# ----------------------------------------------------------------------
# OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoScreen::width {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-width)] == 0} {
        error "bad value: \"$itk_option(-width)\": width should be an integer"
    }
    set ${_width} $itk_option(-width)
    after idle [itcl::code $this fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoScreen::height {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-height)] == 0} {
        error "bad value: \"$itk_option(-height)\": height should be an integer"
    }
    set ${_height} $itk_option(-height)
    after idle [itcl::code $this fixSize]
}
