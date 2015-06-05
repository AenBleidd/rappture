# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: video - viewing movies
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
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

option add *Video.width -1 widgetDefault
option add *Video.height -1 widgetDefault
option add *Video.foreground black widgetDefault
option add *Video.controlBackground gray widgetDefault
option add *Video.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VideoScreen {
    inherit itk::Widget

    itk_option define -width width Width -1
    itk_option define -height height Height -1
    itk_option define -fileopen fileopen Fileopen ""

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
    protected method eventually {args}

    private method togglePtrCtrl {pbvar}
    private method whatPtrCtrl {}
    private method togglePtrBind {pbvar}

    # drawing tools
    private method Rubberband {status win x y}
    private method updateMeasurements {}
    private method Measure {status win x y}
    private method Particle {status win x y}
    private method Trajectory {args}
    private method calculateTrajectory {args}
    private method writeText {x y text color tags width}
    private method clearDrawings {}

    # video dial tools
    private method toggleloop {}

    private common   _settings
    private common   _pendings
    private common   _pbvars
    private common   _counters

    private variable _width -1      ;# width of the movie
    private variable _height -1     ;# height of the movie
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

    private variable _px2dist 0     ;# conversion for pixels to user specified distance
    private variable _particles ""  ;# list of particles
    private variable _measurements "" ;# list of all measurement lines
    private variable _obj ""        ;# temp var holding the last created object
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
        framenum          0
        loop              0
        play              0
        speed             1
    }]

    array set _pendings {
        seek 0
        play 0
    }

    array set _counters {
        particle 0
        measure 0
    }

    # Create flow controls...

    itk_component add main {
        canvas $itk_interior.main
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    bind $itk_component(main) <Configure> [itcl::code $this fixSize]

    # setup the image display
    # hold the video frames in an image on the canvas
    set _imh [image create photo]

    $itk_component(main) create image 0 0 \
        -anchor center \
        -image ${_imh} \
        -tags videoframe

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

    # ==== fileopen ====
    itk_component add fileopen {
        button $itk_component(moviecontrols).fileopen \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon upload] \
            -command [itcl::code $this loadcb]
    } {
        usual
    }
    Rappture::Tooltip::for $itk_component(fileopen) \
        "Open file"

    # ==== measuring tool ====
    set measImg [image create photo -file [file join $imagesDir "line_darrow_green.png"]]
    itk_component add measure {
        Rappture::PushButton $itk_component(moviecontrols).measurepb \
            -onimage $measImg \
            -offimage $measImg \
            -disabledimage $measImg \
            -command [itcl::code $this togglePtrCtrl "measure"] \
            -variable [itcl::scope _pbvars(measure)]
    } {
        usual
    }
    $itk_component(measure) disable
    Rappture::Tooltip::for $itk_component(measure) \
        "Measure the distance of a structure"

    # ==== particle mark tool ====
    set particleImg [image create photo -file [file join $imagesDir "volume-on.gif"]]
    itk_component add particle {
        Rappture::PushButton $itk_component(moviecontrols).particlepb \
            -onimage $particleImg \
            -offimage $particleImg \
            -disabledimage $particleImg \
            -command [itcl::code $this togglePtrCtrl "particle"] \
            -variable [itcl::scope _pbvars(particle)]
    } {
        usual
    }
    $itk_component(particle) disable
    Rappture::Tooltip::for $itk_component(particle) \
        "Mark the location of a particle to follow"


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
    $itk_component(rewind) configure -state disabled
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
    $itk_component(seekback) configure -state disabled
    Rappture::Tooltip::for $itk_component(rewind) \
        "Seek backwards 1 frame"

    # Play
    itk_component add play {
        Rappture::PushButton $itk_component(moviecontrols).play \
            -onimage [Rappture::icon flow-pause] \
            -offimage [Rappture::icon flow-play] \
            -disabledimage [Rappture::icon flow-play] \
            -variable [itcl::scope _settings(play)] \
            -command [itcl::code $this video play]
    }
    $itk_component(play) disable
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
    $itk_component(seekforward) configure -state disabled
    Rappture::Tooltip::for $itk_component(seekforward) \
        "Seek forward 1 frame"

    # Loop
    itk_component add loop {
        Rappture::PushButton $itk_component(moviecontrols).loop \
            -onimage [Rappture::icon flow-loop] \
            -offimage [Rappture::icon flow-loop] \
            -disabledimage [Rappture::icon flow-loop] \
            -variable [itcl::scope _settings(loop)] \
            -command [itcl::code $this toggleloop]
    }
    $itk_component(loop) disable
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously between marked sections"

    itk_component add dial {
        frame $itk_interior.dial
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    # Video Dial Major
    itk_component add dialmajor {
        Rappture::Videodial1 $itk_component(dial).dialmajor \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0 -max 1 \
            -variable [itcl::scope _settings(framenum)] \
            -dialoutlinecolor black \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dialmajor) current 0
    bind $itk_component(dialmajor) <<Value>> [itcl::code $this video update]

    # Video Dial Minor
    itk_component add dialminor {
        Rappture::Videodial2 $itk_component(dial).dialminor \
            -padding 0 \
            -min 0 -max 1 \
            -minortick 1 -majortick 5 \
            -variable [itcl::scope _settings(framenum)] \
            -dialoutlinecolor black
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dialminor) current 0
    bind $itk_component(dialminor) <<Value>> [itcl::code $this video update]

    set fg [option get $itk_component(hull) font Font]

    itk_component add framenumlabel {
        label $itk_component(frnumfr).framenuml -text "Frame:" -font "arial 9" \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    # Current Frame Number
    itk_component add framenum {
        label $itk_component(frnumfr).framenum \
            -background white -font "arial 9" \
            -textvariable [itcl::scope _settings(framenum)]
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(framenum) \
        "Current frame number"


    pack $itk_component(framenumlabel) -side left
    pack $itk_component(framenum) -side right


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
        Rappture::Videospeed $itk_component(moviecontrols).speed \
            -min 0 -max 1 -width 4 -font "arial 9" -factor 2
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(speed) \
        "Change speed of movie"

    $itk_component(speed) value 0.25
    bind $itk_component(speed) <<Value>> [itcl::code $this video speed]


    blt::table $itk_component(dial) \
        0,0 $itk_component(dialmajor) -fill x \
        1,0 $itk_component(dialminor) -fill x

    blt::table $itk_component(moviecontrols) \
        0,0 $itk_component(fileopen) -padx {2 0}  \
        0,1 $itk_component(measure) -padx {4 0}  \
        0,2 $itk_component(particle) -padx {4 0} \
        0,5 $itk_component(dial) -fill x -padx {2 4} -rowspan 3 \
        1,0 $itk_component(rewind) -padx {2 0} \
        1,1 $itk_component(seekback) -padx {4 0} \
        1,2 $itk_component(play) -padx {4 0} \
        1,3 $itk_component(seekforward) -padx {4 0} \
        1,4 $itk_component(loop) -padx {4 0} \
        2,0 $itk_component(frnumfr) -padx {0 0} -cspan 3 \
        2,3 $itk_component(speed) -padx {2 0} -cspan 2

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

    $itk_component(main) configure -background black
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::destructor {} {
    array unset _settings *
    array unset _pendings *
    array unset _pbvars *
    array unset _counters *


    if {[info exists _imh] && ("" != ${_imh})} {
        image delete ${_imh}
        set _imh ""
    }

    if {[info exists measImg]} {
        image delete $measImg
        set measImg ""
    }

    if {[info exists particleImg]} {
        image delete $particleImg
        set particleImg ""
    }

    if {("" != [info commands ${_movie}])} {
        # clear the movie if it is still open
        ${_movie} release
        set _movie ""
    }

    clearDrawings
}

# ----------------------------------------------------------------------
# clearDrawings - delete all particle and measurement objects
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::clearDrawings {} {

    # delete all previously placed particles
    set obj [lindex ${_particles} end]
    while {"" != [info commands $obj]} {
        itcl::delete object $obj
        set _particles [lreplace ${_particles} end end]
        if {[llength ${_particles}] == 0} {
            break
        }
        set obj [lindex ${_particles} end]
    }

    # delete all previously placed measurements
    set obj [lindex ${_measurements} end]
    while {"" != [info commands $obj]} {
        itcl::delete object $obj
        set _measurements [lreplace ${_measurements} end end]
        if {[llength ${_measurements}] == 0} {
            break
        }
        set obj [lindex ${_measurements} end]
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
itcl::body Rappture::VideoScreen::load {type data} {

    video stop

    # open the file
    set fname ""
    switch $type {
        "data" {
            if {"" == $data} {
                error "bad value \"$data\": data should be a movie"
            }

            set fname "/tmp/tmpVV[pid].video"
            set fid [open $fname "w"]
            fconfigure $fid -translation binary -encoding binary
            puts $fid $data
            close $fid
            set type "file"
            set data $fname
        }
        "file" {
            if {"" == $data} {
                error "bad value \"$data\": data should be a movie file path"
            }
            # do nothing
        }
        default {
            error "bad value: \"$type\": should be \"load \[data|file\] <data>\""
        }
    }

    if {"file" == $type} {
        if {("" != [info commands ${_movie}])} {
            # compare the new file name to the name of the file
            # we already have open in our _movie object.
            # if they are the same, do not reopen the video.
            # if they are different, close the old movie
            # and clear out all old drawings from the canvas.
            set err [catch {${_movie} filename} filename]
            if {($err == 0)&& ($data == $filename)} {
                # video file already open, don't reopen it.
                return
            } else {
                # clear the old movie
                ${_movie} release
                set _width -1
                set _height -1

                # delete drawings objects from canvas
                clearDrawings
            }
        }
    }

    set _movie [Rappture::Video $type $data]
    if {"" != $fname} {
        file delete $fname
    }
    set _framerate [${_movie} framerate]
    video speed
    puts stderr "framerate: ${_framerate}"

    set _lastFrame [${_movie} get position end]

    # update the dial with video information
    $itk_component(dialmajor) configure -min 0 -max ${_lastFrame}
    $itk_component(dialminor) configure -min 0 -max ${_lastFrame}

    # turn on the buttons and dials
    $itk_component(measure) enable
    $itk_component(particle) enable
    $itk_component(rewind) configure -state normal
    $itk_component(seekback) configure -state normal
    $itk_component(play) enable
    $itk_component(seekforward) configure -state normal
    $itk_component(loop) enable

    # make sure looping is off
    set _settings(loop) 0
    $itk_component(dialminor) loop disable

    fixSize

    # goto the first frame
    # update the dial and framenum widgets
    video seek 0
    set _settings(framenum) 0
}

# ----------------------------------------------------------------------
# loadcb - load callback
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::loadcb {args} {
    video stop
    Rappture::filexfer::upload {piv tool} {id label desc} [itcl::code $this Upload]
}

# ----------------------------------------------------------------------
# Upload -
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Upload {args} {
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
# fixSize
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::fixSize {} {

    if {[string compare "" ${_movie}] == 0} {
        return
    }

    if {![winfo ismapped $itk_component(hull)]} {
        return
    }

    # get dimensions for the new image
    # adjust the aspect ratio, if necessary

    puts stderr "aspect ratio: [query aspectratio]"

    foreach {w h} [query dimensions] break
    foreach {num den} [query aspectratio] break

    if {[expr 1.0*$w/$h] != [expr 1.0*$num/$den]} {
        # we need to adjust the frame height and width
        # to keep the correct aspect ratio
        # hold the height constant,
        # adjust the width as close as we can
        # to the correct aspect ratio
        set w [expr int(1.0*$num/$den*$h)]
    }

    if {-1 == ${_width}} {
        set _width $w
    }
    if {-1 == ${_height}} {
        set _height $h
    }

    # get an image with the new size
    ${_imh} blank
    ${_imh} configure -width 1 -height 1
    ${_imh} configure -width 0 -height 0
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # place the image in the center of the canvas
    set ccw [winfo width $itk_component(main)]
    set cch [winfo height $itk_component(main)]
    $itk_component(main) coords videoframe [expr $ccw/2.0] [expr $cch/2.0]

    puts stderr "----------------------------"
    puts stderr "adjusted = $w $h"
    puts stderr "data     = ${_width} ${_height}"
    puts stderr "image    = [image width ${_imh}] [image height ${_imh}]"
    foreach {x0 y0 x1 y1} [$itk_component(main) bbox videoframe] break
    puts stderr "bbox     = $x0 $y0 $x1 $y1"
    puts stderr "ccw cch  = [expr $ccw/2.0] [expr $cch/2.0]"
    puts stderr "main     = [winfo width $itk_component(main)] [winfo height $itk_component(main)]"
    puts stderr "hull     = [winfo width $itk_component(hull)] [winfo height $itk_component(hull)]"
}

# ----------------------------------------------------------------------
# video - play, stop, rewind, fastforward the video
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::video { args } {
    set option [lindex $args 0]
    switch -- $option {
        "play" {
            if {$_settings(play) == 1} {
                eventually play
            } else {
                # pause/stop
                after cancel $_id
                set _pendings(play) 0
                set _settings(play) 0
            }
        }
        "seek" {
            Seek [lreplace $args 0 0]
        }
        "stop" {
            after cancel $_id
            set _settings(play) 0
        }
        "speed" {
            set speed [$itk_component(speed) value]
            set _mspf [expr round(((1.0/${_framerate})*1000)/$speed)]
            set _delay [expr {${_mspf} - ${_ofrd}}]
            puts stderr "_mspf = ${_mspf} | $speed | ${_ofrd} | ${_delay}"
        }
        "update" {
            eventually seek [expr round($_settings(framenum))]
            # Seek [expr round($_settings(framenum))]
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
        "aspectratio" {
            set ret [${_movie} aspect display]
        }
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

    set cur ${_nextframe}

    # time how long it takes to retrieve the next frame
    set _ofrd [time {
        # use seek instead of next fxn incase the ${_nextframe} is
        # not the current frame+1. this happens when we skip frames
        # because the underlying c lib is too slow at reading.
        ${_movie} seek $cur
        ${_imh} put [${_movie} get image ${_width} ${_height}]
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
    set _settings(framenum) $cur


    # no play cmds pending
    set _pendings(play) 0

    # if looping is turned on and markers setup,
    # then loop back to loopstart when cur hits loopend
    if {$_settings(loop)} {
        if {$cur == [$itk_component(dialminor) mark position loopend]} {
            Seek [$itk_component(dialminor) mark position loopstart]
        }
    }

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
itcl::body Rappture::VideoScreen::Seek {args} {
    set val [lindex $args 0]
    if {"" == $val} {
        error "bad value: \"$val\": should be \"seek value\""
    }
    set cur [${_movie} get position cur]
    if {[string compare $cur $val] == 0} {
        # already at the frame to seek to
        set _pendings(seek) 0
        return
    }
    ${_movie} seek $val
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # update the dial and framenum widgets
    set _settings(framenum) [${_movie} get position cur]
    event generate $itk_component(main) <<Frame>>

    # removing pending
    set _pendings(seek) 0
}


# ----------------------------------------------------------------------
# eventually -
#   seek
#   play
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::eventually {args} {
    set option [lindex $args 0]
    switch -- $option {
        "seek" {
            if {0 == $_pendings(seek)} {
                # no seek pending, schedule one
                set _pendings(seek) 1
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
# togglePtrCtrl - choose pointer mode:
#                 rectangle, measure, particlemark
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::togglePtrCtrl {tool} {

    if {[info exists _pbvars($tool)] == 0} {
        return
    }

    if {$_pbvars($tool) == 1} {
        # unpush previously pushed buttons
        foreach pbv [array names _pbvars] {
            if {[string compare $tool $pbv] != 0} {
                set _pbvars($pbv) 0
            }
        }
    }
    togglePtrBind $tool
}


# ----------------------------------------------------------------------
# whatPtrCtrl - figure out the current pointer mode:
#                 rectangle,  measure, particlemark
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::whatPtrCtrl {} {
    foreach pbv [array names _pbvars] {
        if {$_pbvars($pbv) != 0} {
            return $pbv
        }
    }
}


# ----------------------------------------------------------------------
# togglePtrBind - update the bindings based on pointer controls
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::togglePtrBind {pbvar} {

    if {[string compare $pbvar current] == 0} {
        set pbvar [whatPtrCtrl]
    }

    if {[string compare $pbvar rectangle] == 0} {

        # Bindings for selecting rectangle
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Rubberband new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Rubberband drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Rubberband release %W %x %y]

    } elseif {[string compare $pbvar measure] == 0} {

        # Bindings for measuring distance
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Measure new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Measure drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Measure release %W %x %y]

    } elseif {[string compare $pbvar particle] == 0} {

        # Bindings for marking particle locations
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Particle new %W %x %y]
        bind $itk_component(main) <B1-Motion> ""
        bind $itk_component(main) <ButtonRelease-1> ""


    } elseif {[string compare $pbvar object] == 0} {

        # Bindings for interacting with objects
        $itk_component(main) configure -cursor hand2

        bind $itk_component(main) <ButtonPress-1> { }
        bind $itk_component(main) <B1-Motion> { }
        bind $itk_component(main) <ButtonRelease-1> { }

    } else {

        # invalid pointer mode

    }
}





###### DRAWING TOOLS #####





# ----------------------------------------------------------------------
# Rubberband - draw a rubberband around something in the canvas
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Rubberband {status win x y} {
    switch -- $status {
        "new" {
            $win delete "rubbershape"
            set _x0 $x
            set _y0 $y
            $win create rectangle \
                $x $y $x $y -outline white -width 2  \
                -tags "rubbershape" -dash {4 4}
        }
        "drag" {
            foreach { x0 y0 x1 y1 } [$win coords "rubbershape"] break

            if {$_x0 > $x} {
                # backward direction
                set x0 $x
                set x1 $_x0
            } else {
                set x1 $x
            }

            if {$_y0 >= $y} {
                # backward direction
                set y0 $y
                set y1 $_y0
            } else {
                set y1 $y
            }

            eval $win coords "rubbershape" [list $x0 $y0 $x1 $y1]
        }
        "release" {
            Rubberband drag $win $x $y
        }
        default {
            error "bad status \"$status\": should be new, drag, or release"
        }
    }
}

# ----------------------------------------------------------------------
# Measure - draw a line to measure something on the canvas,
#           when user releases the line, user is given the
#           calculated measurement.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Measure {status win x y} {
    switch -- $status {
        "new" {
            set name "measure[incr _counters(measure)]"

            set _obj [Rappture::VideoDistance $itk_component(main).$name $name $win \
                        -fncallback [itcl::code $this query framenum] \
                        -bindentercb [itcl::code $this togglePtrBind object] \
                        -bindleavecb [itcl::code $this togglePtrBind current] \
                        -writetextcb [itcl::code $this writeText] \
                        -ondelete [itcl::code $itk_component(dialminor) mark remove $name] \
                        -onframe [itcl::code $itk_component(dialminor) mark add $name] \
                        -px2dist [itcl::scope _px2dist] \
                        -units "m" \
                        -color green \
                        -bindings disable]
            ${_obj} Coords $x $y $x $y
            ${_obj} Show object
            lappend _measurements ${_obj}
        }
        "drag" {
            # FIXME: something wrong with the bindings, if the objects menu is
            #        open, and you click on the canvas off the menu, a "drag"
            #        or "release" call is made. need to figure out how to
            #        disable bindings while obj's menu is open. for now
            #        we set _obj to "" when we are finished creating it and
            #        check to see if it's valid when we do a drag or release

            if {"" == ${_obj}} {
                return
            }

            ${_obj} Coords [lreplace [${_obj} Coords] 2 3 $x $y]
        }
        "release" {
            # if we enable ${_obj}'s bindings when we create it,
            # probably never entered because the object's <Enter>
            # bindings kick in before the window's release bindings do

            if {"" == ${_obj}} {
                return
            }

            Measure drag $win $x $y

            if {${_px2dist} == 0} {
                ${_obj} Menu activate $x $y
            }
            ${_obj} configure -bindings enable

            set _obj ""
        }
        default {
            error "bad status \"$status\": should be new, drag, or release"
        }
    }
}

# ----------------------------------------------------------------------
# Particle - mark a particle in the video, a new particle object is
#            created from information like the name, which video
#            frames it lives in, it's coords in the canvas in each
#            frame, it's color...
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Particle {status win x y} {
    switch -- $status {
        "new" {
            set name "particle[incr _counters(particle)]"
            set _obj [Rappture::VideoParticle $itk_component(main).$name $name $win \
                        -fncallback [itcl::code $this query framenum] \
                        -bindentercb [itcl::code $this togglePtrBind object] \
                        -bindleavecb [itcl::code $this togglePtrBind current] \
                        -trajcallback [itcl::code $this Trajectory] \
                        -ondelete [itcl::code $itk_component(dialminor) mark remove $name] \
                        -onframe [itcl::code $itk_component(dialminor) mark add $name] \
                        -framerange "0 ${_lastFrame}" \
                        -halo 5 \
                        -color green \
                        -px2dist [itcl::scope _px2dist] \
                        -units "m/s"]
            ${_obj} Coords $x $y
            ${_obj} Show object
            #$itk_component(dialminor) mark add $name current
            # bind $itk_component(hull) <<Frame>> [itcl::code $itk_component(main).$name UpdateFrame]

            # link the new particle to the last particle added, if it exists
            set lastp [lindex ${_particles} end]
            while {"" == [info commands $lastp]} {
                set _particles [lreplace ${_particles} end end]
                if {[llength ${_particles}] == 0} {
                    break
                }
                set lastp [lindex ${_particles} end]
            }
            if {"" != [info commands $lastp]} {
                $lastp Link ${_obj}
            }

            # add the particle to the list
            lappend _particles ${_obj}
        }
        default {
            error "bad status \"$status\": should be new"
        }
    }
}

# ----------------------------------------------------------------------
# Trajectory - draw a trajectory between two particles
#
#   Trajectory $p0 $p1
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::Trajectory {args} {

    set nargs [llength $args]
    if {($nargs != 1) && ($nargs != 2)} {
        error "wrong # args: should be \"Trajectory p0 p1\""
    }

    set p0 ""
    set p1 ""
    foreach {p0 p1} $args break

    if {[string compare "" $p0] == 0} {
        # p0 does not exist
        return
    }

    # remove any old trajectory links from p0
    set p0name [$p0 name]
    set oldlink "vec-$p0name"
    $itk_component(main) delete $oldlink

    # check to see if p1 exists anymore
    if {[string compare "" $p1] == 0} {
        # p1 does not exist
        return
    }

    foreach {x0 y0} [$p0 Coords] break
    foreach {x1 y1} [$p1 Coords] break
    set p1name [$p1 name]
    set link "vec-$p0name-$p1name"
    $itk_component(main) create line $x0 $y0 $x1 $y1 \
        -fill green \
        -width 2 \
        -tags "vector $link vec-$p0name" \
        -dash {4 4} \
        -arrow last
    $itk_component(main) lower $link $p0name

    # calculate trajectory, truncate it after 4 sigdigs
    set t [calculateTrajectory [$p0 Frame] $x0 $y0 [$p1 Frame] $x1 $y1]
    set tt [string range $t 0 [expr [string first . $t] + 4]]


    # calculate coords for text
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox $link] break
    set x [expr "$x0 + (abs($x1-$x0)/2)"]
    set y [expr "$y0 + (abs($y1-$y0)/2)"]

    set tt "$tt m/s"
    set tags "vectext $link vec-$p0name"
    set width [expr sqrt(pow(abs($x1-$x0),2)+pow(abs($y1-$y0),2))]

    writeText $x $y $tt green $tags $width
    return $link
}


# ----------------------------------------------------------------------
# writeText - write text to the canvas
#   writes text to the canvas in the color <color> at <x>,<y>
#   writes text twice more offset up-left and down right,
#   to add a shadowing effect so colors can be seen
#
#   FIXME: Not sure how the text wrapped due to -width collides with the
#          offset text.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::writeText {x y text color tags width} {
    $itk_component(main) create text [expr $x-1] [expr $y] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

    $itk_component(main) create text [expr $x+1] [expr $y] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

    $itk_component(main) create text [expr $x] [expr $y-1] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

    $itk_component(main) create text [expr $x] [expr $y+1] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

#    # write text up-left
#    $itk_component(main) create text [expr $x-1] [expr $y-1] \
#        -tags $tags \
#        -justify center \
#        -text $text \
#        -fill black \
#        -width $width
#
#    # write text down-right
#    $itk_component(main) create text [expr $x+1] [expr $y+1] \
#        -tags $tags \
#        -justify center \
#        -text $text \
#        -fill black \
#        -width $width

    # write text at x,y
    $itk_component(main) create text $x $y \
        -tags $tags \
        -justify center \
        -text $text \
        -fill $color \
        -width $width

}

# ----------------------------------------------------------------------
# calculateTrajectory - calculate the value of the trajectory
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::calculateTrajectory {args} {
    # set framerate 29.97         ;# frames per second
    # set px2dist    8.00         ;# px per meter

    foreach {f0 x0 y0 f1 x1 y1} $args break
    set px [expr sqrt(pow(abs($x1-$x0),2)+pow(abs($y1-$y0),2))]
    set frames [expr $f1 - $f0]

    if {($frames != 0) && (${_px2dist} != 0)} {
        set t [expr 1.0*$px/$frames*${_px2dist}*${_framerate}]
    } else {
        set t 0.0
    }

    return $t
}

# ----------------------------------------------------------------------
# toggleloop - add/remove a start/end loop mark to video dial.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::toggleloop {} {
    if {$_settings(loop) == 0} {
        $itk_component(dialminor) loop disable
    } else {
        set cur [${_movie} get position cur]
        set end [${_movie} get position end]

        set startframe [expr $cur-10]
        if {$startframe < 0} {
            set startframe 0
        }

        set endframe [expr $cur+10]
        if {$endframe > $end} {
            set endframe $end
        }

        $itk_component(dialminor) loop between $startframe $endframe
    }

}

# ----------------------------------------------------------------------
# OPTION: -width - width of the video
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoScreen::width {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-width)] == 0} {
        error "bad value: \"$itk_option(-width)\": width should be an integer"
    }
    set _width $itk_option(-width)
    after idle [itcl::code $this fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -height - height of the video
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoScreen::height {
    # $_dispatcher event -idle !fixsize
    if {[string is integer $itk_option(-height)] == 0} {
        error "bad value: \"$itk_option(-height)\": height should be an integer"
    }
    set _height $itk_option(-height)
    after idle [itcl::code $this fixSize]
}

# ----------------------------------------------------------------------
# OPTION: -fileopen
# ----------------------------------------------------------------------
itcl::configbody Rappture::VideoScreen::fileopen {
    $itk_component(fileopen) configure -command $itk_option(-fileopen)
}
