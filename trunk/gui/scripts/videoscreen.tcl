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

    # video dial tools
    private method toggleloop {}

    private common   _settings
    private common   _pendings

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
    private variable _waiting 0     ;# number of frames behind we are
    private variable _nextframe 0   ;#

    private variable _pbvlist ""    ;# list of the pushbutton variables
    private variable _pcnt 0        ;# counter for naming particles
    private variable _particles ""  ;# list of particles
    private variable _px2dist 0     ;# conversion for pixels to user specified distance
    private variable _measTags ""   ;# tags of measurement lines
    private variable _measCnt 0     ;# counter for naming measure lines
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
        $this-currenttime       0
        $this-framenum          0
        $this-loop              0
        $this-play              0
        $this-speed             1
    }]

    array set _pendings [subst {
        seek 0
        play 0
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

    # setup frame number frame
    itk_component add frnumfr {
        frame $itk_component(moviecontrols).frnumfr
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    set imagesDir [file join $RapptureGUI::library scripts images]


    # ==== measuring tool ====
    set measImg [image create photo -file [file join $imagesDir "line_darrow_green.png"]]
    itk_component add measure {
        Rappture::PushButton $itk_component(moviecontrols).measurepb \
            -onimage $measImg \
            -offimage $measImg \
            -command [itcl::code $this togglePtrCtrl measPbVar] \
            -variable measPbVar
    } {
        usual
    }
    Rappture::Tooltip::for $itk_component(measure) \
        "Measure the distance of a structure"
    lappend _pbvlist measPbVar

    # ==== particle mark tool ====
    set particleImg [image create photo -file [file join $imagesDir "volume-on.gif"]]
    itk_component add particle {
        Rappture::PushButton $itk_component(moviecontrols).particlepb \
           -onimage $particleImg \
            -offimage $particleImg \
            -command [itcl::code $this togglePtrCtrl partPbVar] \
            -variable partPbVar
    } {
        usual
    }
    Rappture::Tooltip::for $itk_component(particle) \
        "Mark the location of a particle to follow"
    lappend _pbvlist partPbVar


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
            -variable [itcl::scope _settings($this-loop)] \
            -command [itcl::code $this toggleloop]
    }
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously between marked sections"

    # Video Dial
    itk_component add dial {
        Rappture::Videodial $itk_component(moviecontrols).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0 -max 1 \
            -minortick 1 -majortick 5 \
            -variable [itcl::scope _settings($this-currenttime)] \
            -dialoutlinecolor black \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dial) current 0
    bind $itk_component(dial) <<Value>> [itcl::code $this video update]

    itk_component add framenumlabel {
        label $itk_component(frnumfr).framenuml -text "Frame:" -font $fg \
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
        label $itk_component(frnumfr).framenum \
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
        0,0 $itk_component(measure) -padx {2 0}  \
        0,1 $itk_component(particle) -padx {4 0} \
        0,5 $itk_component(dial) -fill x -padx {2 4} -rowspan 3 \
        1,0 $itk_component(rewind) -padx {2 0} \
        1,1 $itk_component(seekback) -padx {4 0} \
        1,2 $itk_component(play) -padx {4 0} \
        1,3 $itk_component(seekforward) -padx {4 0} \
        1,4 $itk_component(loop) -padx {4 0} \
        2,0 $itk_component(frnumfr) -padx {2 0} -columnspan 3 \
        2,3 $itk_component(speed) -padx {2 0} -columnspan 2

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
    set _delay [expr {${_mspf} - ${_ofrd}}]
    puts stderr "framerate = ${_framerate}"
    puts stderr "mspf = ${_mspf}"
    puts stderr "ofrd = ${_ofrd}"
    puts stderr "delay = ${_delay}"

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
                # bind $itk_component(dial) <<Value>> ""
                eventually play
            } else {
                # pause/stop
                after cancel $_id
                set _pendings(play) 0
                set _settings($this-play) 0
                # enable seek while paused
                # bind $itk_component(dial) <<Value>> [itcl::code $this updateFrame]
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
            set speed [$itk_component(speed) value]
            set _mspf [expr round(((1.0/${_framerate})*1000)/pow(2,$speed-1))]
            set _delay [expr {${_mspf} - ${_ofrd}}]
            puts stderr "_mspf = ${_mspf} | $speed | ${_ofrd} | ${_delay}"
        }
        "update" {
            #video stop
            # eventually seek [expr round($_settings($this-currenttime))]
            Seek [expr round($_settings($this-currenttime))]
            # Seek $_settings($this-framenum)
            #after idle [itcl::code $this video play]
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

    # time how long it takes to retrieve the next frame
    set _ofrd [time {
        $_movie seek $_nextframe
        $_imh put [$_movie get image ${_width} ${_height}]
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

    set cur [$_movie get position cur]

    # update the dial and framenum widgets
    set _settings($this-currenttime) $cur
    set _settings($this-framenum) $cur

    if {[expr $cur%100] == 0} {
#        puts stderr "ofrd = ${_ofrd}"
#        puts stderr "delay = ${_delay}"
#        puts stderr "after: [after info]"
#        puts stderr "id = ${_id}"
    }

    # no play cmds pending
    set _pendings(play) 0

    # if looping is turned on and markers setup,
    # then loop back to loopstart when cur hits loopend
    if {$_settings($this-loop)} {
        if {$cur == [$itk_component(dial) mark position loopend]} {
            Seek [$itk_component(dial) mark position loopstart]
        }
    }

    # schedule the next frame to be displayed
    if {$cur < ${_lastFrame}} {
        set _id [after ${_delay} [itcl::code $this eventually play]]
    }

#    event generate $itk_component(hull) <<Frame>>
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
    set cur [$_movie get position cur]
    if {[string compare $cur $val] == 0} {
        # already at the frame to seek to
        return
    }
    ${_movie} seek $val
    ${_imh} put [${_movie} get image ${_width} ${_height}]

    # update the dial and framenum widgets
    event generate $itk_component(main) <<Frame>>
    set cur [$_movie get position cur]
    set _settings($this-currenttime) $cur
    set _settings($this-framenum) $cur

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
                set _nextframe [expr {[$_movie get position cur] + 1}]
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
itcl::body Rappture::VideoScreen::togglePtrCtrl {pbvar} {

    upvar 1 $pbvar inState
    if {$inState == 1} {
        # unpush previously pushed buttons
        foreach pbv $_pbvlist {
            if {[string compare $pbvar $pbv] != 0} {
                upvar 1 $pbv var
                set var 0
            }
        }
    }
    togglePtrBind $pbvar
}


# ----------------------------------------------------------------------
# whatPtrCtrl - figure out the current pointer mode:
#                 rectangle,  measure, particlemark
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::whatPtrCtrl {} {
    foreach pbv $_pbvlist {
        upvar #0 $pbv var
        if {$var != "" && $var != 0} {
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

    if {[string compare $pbvar rectPbVar] == 0} {

        # Bindings for selecting rectangle
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Rubberband new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Rubberband drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Rubberband release %W %x %y]

    } elseif {[string compare $pbvar measPbVar] == 0} {

        # Bindings for measuring distance
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Measure new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Measure drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Measure release %W %x %y]

    } elseif {[string compare $pbvar partPbVar] == 0} {

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
            incr _measCnt
            set name "measure${_measCnt}"
            lappend _measTags $name

            set _obj [Rappture::VideoDistance $itk_component(main).$name $name $win \
                        -fncallback [itcl::code $this query framenum] \
                        -bindentercb [itcl::code $this togglePtrBind object] \
                        -bindleavecb [itcl::code $this togglePtrBind current] \
                        -writetextcb [itcl::code $this writeText] \
                        -px2dist [itcl::scope _px2dist] \
                        -units "m" \
                        -color green \
                        -bindings disable]
            ${_obj} Coords $x $y $x $y
            ${_obj} Show object
            $itk_component(dial) mark add arrow current
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
            incr _pcnt
            set name "particle${_pcnt}"
            set _obj [Rappture::VideoParticle $itk_component(main).$name $name $win \
                        -fncallback [itcl::code $this query framenum] \
                        -bindentercb [itcl::code $this togglePtrBind object] \
                        -bindleavecb [itcl::code $this togglePtrBind current] \
                        -trajcallback [itcl::code $this Trajectory] \
                        -halo 5 \
                        -color green \
                        -px2dist [itcl::scope _px2dist] \
                        -units "m/s"]
            ${_obj} Coords $x $y
            ${_obj} Show object
            $itk_component(dial) mark add $name current
            # bind $itk_component(hull) <<Frame>> [itcl::code $itk_component(main).$name UpdateFrame]

            # link the new particle to the last particle added
            if {[llength ${_particles}] > 0} {
                set lastp [lindex ${_particles} end]
                $lastp Link ${_obj}
            }

            # add the particle to the list
            lappend _particles ${_obj}

#            set pm [Rappture::VideoParticleManager]
#            $pm add $p0
#            set plist [$pm list]
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
    # write text up-left
    $itk_component(main) create text [expr $x-1] [expr $y-1] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

    # write text down-right
    $itk_component(main) create text [expr $x+1] [expr $y+1] \
        -tags $tags \
        -justify center \
        -text $text \
        -fill black \
        -width $width

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
        set t [expr 1.0*$px/$frames/${_px2dist}*${_framerate}]
    } else {
        set t 0.0
    }

    return $t
}

# ----------------------------------------------------------------------
# toggleloop - add/remove a start/end loop mark to video dial.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoScreen::toggleloop {} {
    if {$_settings($this-loop) == 0} {
        $itk_component(dial) mark remove loopstart
        $itk_component(dial) mark remove loopend
    } else {
        set cur [$_movie get position cur]
        set end [$_movie get position end]

        set startframe [expr $cur-10]
        if {$startframe < 0} {
            set startframe 0
        }

        set endframe [expr $cur+10]
        if {$endframe > $end} {
            set endframe $end
        }

        $itk_component(dial) mark add loopstart $startframe
        $itk_component(dial) mark add loopend $endframe
    }

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
