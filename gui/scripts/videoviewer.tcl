# ----------------------------------------------------------------------
#  COMPONENT: videoviewer - viewing movies
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

option add *VideoViewer.width 5i widgetDefault
option add *VideoViewer*cursor crosshair widgetDefault
option add *VideoViewer.height 4i widgetDefault
option add *VideoViewer.foreground black widgetDefault
option add *VideoViewer.controlBackground gray widgetDefault
option add *VideoViewer.controlDarkBackground #999999 widgetDefault
option add *VideoViewer.plotBackground black widgetDefault
option add *VideoViewer.plotForeground white widgetDefault
option add *VideoViewer.plotOutline gray widgetDefault
option add *VideoViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VideoViewer {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""
    itk_option define -plotoutline plotOutline PlotOutline ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method load {filename}
    public method video {args}

    protected method togglePtrBind {pbvar}
    protected method togglePtrCtrl {pbvar}
    protected method whatPtrCtrl {}

    protected method Play {}
    protected method Seek {n}
    protected method Rubberband {status win x y}
    protected method Distance {status win x y}
    protected method Measure {status win x y}
    protected method Particle {status win x y}
    protected method Trajectory {args}
    protected method updateMeasurements {}
    protected method calculateTrajectory {args}

    private common   _settings

    private variable _width 0
    private variable _height 0
    private variable _x0 0          ;# start x for rubberbanding
    private variable _y0 0          ;# start y for rubberbanding
    private variable _units "m"
    private variable _movie ""      ;# movie we grab images from
    private variable _lastFrame 0   ;# last frame in the movie
    private variable _imh ""        ;# current image being displayed
    private variable _id ""         ;# id of the next play command from after
    private variable _pbvlist ""    ;# list of push button variables
    private variable _px2dist 0     ;# conversion for screen px to distance
    private variable _measCnt 0     ;# count of the number measure lines
    private variable _measTags ""   ;# list of measure line tags on canvas
    private variable _particles ""  ;# list of particles
    private variable _pcnt -1       ;# particle count
    private variable _framerate 30  ;# video frame rate
    private variable _mspf 20       ;# milliseconds per frame wait time
    private variable _waiting 0     ;# number of frames behind we are
}

itk::usual VideoViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::constructor {args} {

    array set _settings [subst {
        $this-arrows            0
        $this-currenttime       0
        $this-framenum          0
        $this-duration          1:00
        $this-loop              0
        $this-play              0
        $this-speed             500
        $this-step              0
    }]

    # Create flow controls...

    itk_component add main {
        canvas $itk_interior.main \
            -background black
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    itk_component add pointercontrols {
        frame $itk_interior.pointercontrols
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }


    itk_component add moviecontrols {
        frame $itk_interior.moviecontrols
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }

    pack forget $itk_component(main)
    blt::table $itk_interior \
        0,0 $itk_component(pointercontrols) -fill x \
        1,0 $itk_component(main) -fill both \
        2,0 $itk_component(moviecontrols) -fill x
    # why do i have to explicitly say r0 and r2 instead of r*
    blt::table configure $itk_interior r0 -resize none
    blt::table configure $itk_interior r2 -resize none
    blt::table configure $itk_interior c0 -padx 1


    # setup pointer controls

    set imagesDir [file join $RapptureGUI::library scripts images]

    # ==== rectangle select tool ====
    set rectImg [image create photo -file [file join $imagesDir "rect_dashed_black.png"]]
    itk_component add rectangle {
        Rappture::PushButton $itk_component(pointercontrols).rectanglepb \
            -onimage $rectImg \
            -offimage $rectImg \
            -command [itcl::code $this togglePtrCtrl rectPbVar] \
            -variable rectPbVar
    } {
        usual
    }
    Rappture::Tooltip::for $itk_component(rectangle) \
        "rectangle select tool"

    lappend _pbvlist rectPbVar

    # ==== distance specify tool ====
    set distImg [image create photo -file [file join $imagesDir "line_darrow_red.png"]]
    itk_component add distance {
        Rappture::PushButton $itk_component(pointercontrols).distancepb \
            -onimage $distImg \
            -offimage $distImg \
            -command [itcl::code $this togglePtrCtrl distPbVar] \
            -variable distPbVar
    } {
        usual
    }
    Rappture::Tooltip::for $itk_component(distance) \
        "Specify the distance of a structure"

    lappend _pbvlist distPbVar



    # ==== measuring tool ====
    set measImg [image create photo -file [file join $imagesDir "line_darrow_green.png"]]
    itk_component add measure {
        Rappture::PushButton $itk_component(pointercontrols).measurepb \
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
        Rappture::PushButton $itk_component(pointercontrols).particlepb \
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

    blt::table $itk_component(pointercontrols) \
        0,0 $itk_component(rectangle) -pady {3 0} \
        0,1 $itk_component(distance) -pady {3 0} \
        0,2 $itk_component(measure) -pady {3 0} \
        0,3 $itk_component(particle) -pady {3 0}

    blt::table configure $itk_component(pointercontrols) c* -resize none
    blt::table configure $itk_component(pointercontrols) r* -resize none


    # setup movie controls

    # Rewind
    itk_component add rewind {
        button $itk_component(moviecontrols).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-rewind] \
            -command [itcl::code $this video seek 0]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(rewind) \
        "Rewind movie"

    # Stop
    itk_component add stop {
        button $itk_component(moviecontrols).stop \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-stop] \
            -command [itcl::code $this video stop]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(stop) \
        "Stop movie"

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

    # Loop
    itk_component add loop {
        Rappture::PushButton $itk_component(moviecontrols).loop \
            -onimage [Rappture::icon flow-loop] \
            -offimage [Rappture::icon flow-loop] \
            -variable [itcl::scope _settings($this-loop)]
    }
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously"

    itk_component add dial {
        Rappture::Flowdial $itk_component(moviecontrols).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0.0 -max 1.0 \
            -variable [itcl::scope _settings($this-currenttime)] \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dial) current 0.0
    bind $itk_component(dial) <<Value>> [itcl::code $this video seek -currenttime]

    # Current Frame Number
    itk_component add framenum {
        Rappture::Spinint $itk_component(moviecontrols).framenum \
            -min 1 -max 1 -width 1 -font "arial 9"
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(framenum) value 1
    bind $itk_component(framenum) <<Value>> \
        [itcl::code $this video seek -framenum]
    Rappture::Tooltip::for $itk_component(framenum) \
        "Set the current frame number"


    # Duration
    itk_component add duration {
        entry $itk_component(moviecontrols).duration \
            -textvariable [itcl::scope _settings($this-duration)] \
            -bg white -width 6 -font "arial 9"
    } {
        usual
        ignore -highlightthickness -background
    }
    bind $itk_component(duration) <Return> [itcl::code $this flow duration]
    bind $itk_component(duration) <Tab> [itcl::code $this flow duration]
    Rappture::Tooltip::for $itk_component(duration) \
        "Set duration of movie (format is min:sec)"


    itk_component add durationlabel {
        label $itk_component(moviecontrols).durationl \
            -text "Duration:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness 
        rename -background -controlbackground controlBackground Background
    }

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
        0,1 $itk_component(stop) -padx {2 0} \
        0,2 $itk_component(play) -padx {2 0} \
        0,3 $itk_component(loop) -padx {2 0} \
        0,4 $itk_component(dial) -fill x -padx {2 0 } \
        0,5 $itk_component(framenum) -padx { 0 0} \
        0,6 $itk_component(duration) -padx { 0 0} \
        0,7 $itk_component(speed) -padx {2 3}

    blt::table configure $itk_component(moviecontrols) c* -resize none
    blt::table configure $itk_component(moviecontrols) c4 -resize both
    blt::table configure $itk_component(moviecontrols) r0 -pady 1

    itk_component add distGauge {
        Rappture::Gauge $itk_interior.distGauge \
            -units "m"
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(distGauge) \
        "Length of structure"

    bind $itk_component(distGauge) <<Value>> [itcl::code $this updateMeasurements]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::destructor {} {
    set _sendobjs ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    $_dispatcher cancel !send_transfunc
    array unset _settings $this-*
}

# ----------------------------------------------------------------------
# load - load a video file
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::load {filename} {
    set _movie [Rappture::Video $filename]
    set _framerate [${_movie} get framerate]
    set _mspf [expr round(((1.0/${_framerate})*1000)/pow(2,[$itk_component(speed) value]-1))]
    # set _mspf 7
    puts "framerate = ${_framerate}"
    puts "mspf = ${_mspf}"

    set _imh [image create photo]
    $_imh put [$_movie next]
    $itk_component(main) create image 0 0 -anchor nw -image $_imh

    set _lastFrame [$_movie get position end]
    set offset [expr 1.0/double(${_lastFrame})]
    puts "end = ${_lastFrame}"
    puts "offset = $offset"
    $itk_component(dial) configure -offset $offset

    set lcv ${_lastFrame}
    set cnt 1
    while {$lcv > 9} {
        set lcv [expr $lcv/10]
        incr cnt
    }
    $itk_component(framenum) configure -max ${_lastFrame} -width $cnt

    set pch [$itk_component(pointercontrols) cget -height]
    set mch [$itk_component(moviecontrols) cget -height]
    set pch 30
    set mch 30
    $itk_component(main) configure -scrollregion [$itk_component(main) bbox all]
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox all] break
    set w [expr abs($x1-$x0)]
    set h [expr abs($y1-$y0+$pch+$mch)]
    # $itk_component(main) configure -width $w -height $h
    .main configure -width $w -height $h

}

# ----------------------------------------------------------------------
# video - play, stop, rewind, fastforward the video
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::video { args } {
    set ret 0
    set option [lindex $args 0]
    switch -- $option {
        "play" {
            if {$_settings($this-play) == 1} {
                # while in play move, you can't seek using the
                # framenum spinint widget
                bind $itk_component(framenum) <<Value>> ""
                # start playing
                Play
            } else {
                # pause
                after cancel $_id
                set _settings($this-play) 0
                # setup seek bindings using the
                # framenum spinint widget
                bind $itk_component(framenum) <<Value>> \
                    [itcl::code $this video seek -framenum]
            }
        }
        "seek" {
            Seek [lreplace $args 0 0]
        }
        "stop" {
            after cancel $_id
            set _settings($this-play) 0
        }
        "position" {
            set ret [${_movie} get position cur]
        }
        "speed" {
            set _mspf [expr round(((1.0/${_framerate})*1000)/pow(2,[$itk_component(speed) value]-1))]
            puts "_mspf = ${_mspf}"
        }
        default {
            error "bad option \"$option\": should be play, stop, toggle, position, or reset."
        }
    }
    return $ret
}

# ----------------------------------------------------------------------
# togglePtrCtrl - choose pointer mode:
#                 rectangle, distance, measure, particlemark
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::togglePtrCtrl {pbvar} {

    upvar 1 $pbvar inState
    puts "togglePtrCtrl to $pbvar"
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
#                 rectangle, distance, measure, particlemark
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::whatPtrCtrl {} {
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
itcl::body Rappture::VideoViewer::togglePtrBind {pbvar} {

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

    } elseif {[string compare $pbvar distPbVar] == 0} {

        # Bindings for setting distance
        $itk_component(main) configure -cursor ""

        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Distance new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Distance drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Distance release %W %x %y]

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

    } elseif {[string compare $pbvar particle] == 0} {

        # Bindings for interacting with particles
        $itk_component(main) configure -cursor hand2

        bind $itk_component(main) <ButtonPress-1> ""
        bind $itk_component(main) <B1-Motion> ""
        bind $itk_component(main) <ButtonRelease-1> ""

    } else {

        # invalid pointer mode

    }
}


# ----------------------------------------------------------------------
# play - get the next video frame
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Play {} {

    set cur [$_movie get position cur]

#    # this probably is incorrect because other people
#    # could schedule stuff in the after queue
#    if {[llength [after info]] > 1} {
#        # drop frames that get caught up in the "after queue"
#        # in order to keep up with the frame rate
#        #foreach i [after info] {
#        #    after cancel $i
#        #}
#        incr _waiting
#    } else {
#        # display the next frame
#        $_imh put [$_movie seek +[incr _waiting]]
#        set _waiting 0
#
#        # update the dial and framenum widgets
#        set _settings($this-currenttime) [expr 1.0*$cur/${_lastFrame}]
#        $itk_component(framenum) value $cur
#
#    }

    # display the next frame
    $_imh put [$_movie next]

    # update the dial and framenum widgets
    set _settings($this-currenttime) [expr 1.0*$cur/${_lastFrame}]
    $itk_component(framenum) value $cur

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
# Seek - go to a frame in the video video frame
#   Seek -percent 43
#   Seek -percent 0.5
#   Seek +5
#   Seek -5
#   Seek 35
#   Seek -currenttime
#   Seek -framenum
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Seek {args} {
    set option [lindex $args 0]
    switch -- $option {
        "-percent" {
            set val [lindex $args 1]
            if {[string is integer -strict $val] == 1} {
                set val [expr double($val) / 100.0]
            }
            # convert the percentage to a frame number (new cur)
            set val [expr int($val * ${_lastFrame})]
        }
        "-currenttime" {
            set val $_settings($this-currenttime)
            set val [expr round($val * ${_lastFrame})]
        }
        "-framenum" {
            set val [$itk_component(framenum) value]
        }
        default {
            set val $option
        }
    }
    if {"" == $val} {
        error "bad value: \"$val\": should be \"seek \[-percent\] value\""
    }
    $_imh put [$_movie seek $val]
    set cur [$_movie get position cur]
    set _settings($this-currenttime) [expr double($cur) / double(${_lastFrame})]
}


# ----------------------------------------------------------------------
# Rubberband - draw a rubberband around something in the canvas
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Rubberband {status win x y} {
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
# updateMeasurements - update measurements based on provided distance
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::updateMeasurements {} {
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox "distance"] break
    set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
    set dist [Rappture::Units::convert [$itk_component(distGauge) value] -units off]
    set px2dist [expr $dist/$px]
    if {$px2dist != ${_px2dist}} {
        set _px2dist $px2dist
    }

    # if measure lines exist, update their values
    foreach tag ${_measTags} {
        foreach { x0 y0 x1 y1 } [$itk_component(main) bbox $tag] break
        set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
        set dist [expr $px*${_px2dist}]
        regexp {measure(\d+)} $tag match cnt
        $itk_component(measGauge$cnt) value $dist
    }
}

# ----------------------------------------------------------------------
# Distance - draw a line to measure something on the canvas,
#            when user releases the line, user is prompted for
#            a measurement which is stored and used as the bases
#            for future distance calculations.
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Distance {status win x y} {
    switch -- $status {
        "new" {
            $win delete "distance"
            $win delete "distance-val"
            $win create line \
                $x $y $x $y -fill red -width 2  \
                -tags "distance" -dash {4 4} -arrow both
        }
        "drag" {
            set coords [$win coords "distance"]
            eval $win coords "distance" [lreplace $coords 2 3 $x $y]
        }
        "release" {
            Distance drag $win $x $y
            foreach { x0 y0 x1 y1 } [$itk_component(main) bbox "distance"] break
            set rootx [winfo rootx $itk_component(main)]
            set rooty [winfo rooty $itk_component(main)]
            set x [expr "$x0 + (abs($x1-$x0)/2)"]
            set y [expr "$y0 + (abs($y1-$y0)/2)"]
            $itk_component(main) create window $x $y \
                -window $itk_component(distGauge) \
                -anchor center \
                -tags "distance-val"
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
itcl::body Rappture::VideoViewer::Measure {status win x y} {
    switch -- $status {
        "new" {
            $win delete "measure"
            $win create line \
                $x $y $x $y -fill green -width 2  \
                -tags "measure" -dash {4 4} -arrow both
        }
        "drag" {
            set coords [$win coords "measure"]
            eval $win coords "measure" [lreplace $coords 2 3 $x $y]
        }
        "release" {
            # finish drawing the measuring line
            Measure drag $win $x $y

            # calculate the location on the measuring line to place gauge
            foreach { x0 y0 x1 y1 } [$itk_component(main) bbox "measure"] break
            puts "bbox for $_measCnt is ($x0,$y0) ($x1,$y1)"
            set rootx [winfo rootx $itk_component(main)]
            set rooty [winfo rooty $itk_component(main)]
            set x [expr "$x0 + (abs($x1-$x0)/2)"]
            set y [expr "$y0 + (abs($y1-$y0)/2)"]

#            set popup ".measure$_measCnt-popup"
#            if { ![winfo exists $popup] } {
#                # Create a popup for the measure line dialog
#                Rappture::Balloon $popup -title "Configure measurement..."
#                set inner [$popup component inner]
#                # Create the print dialog widget and add it to the
#                # the balloon popup.
#                Rappture::XyPrint $inner.print-
#                $popup configure \
#                    -deactivatecommand [list $inner.print reset]-
#                blt::table $inner 0,0 $inner.print -fill both
#            }
#
#
            # create a new gauge for this measuring line
            itk_component add measGauge$_measCnt {
                Rappture::Gauge $itk_interior.measGauge$_measCnt \
                    -units "m"
            } {
                usual
                rename -background -controlbackground controlBackground Background
            }
            Rappture::Tooltip::for $itk_component(measGauge$_measCnt) \
                "Length of structure $_measCnt"

            # place the gauge on the measuring line
            $itk_component(main) create window $x $y \
                -window $itk_component(measGauge$_measCnt) \
                -anchor center \
                -tags "measure$_measCnt-val"

            # set the value of the gauge with the calculated distance
            set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
            set dist [expr $px*$_px2dist]
            $itk_component(measGauge$_measCnt) value $dist

            # rename the tag for the line
            # so we can have multiple measure lines
            # store tag name for future value updates
            $itk_component(main) addtag "measure$_measCnt" withtag "measure"
            $itk_component(main) dtag "measure" "measure"
            lappend _measTags "measure$_measCnt"
            incr _measCnt
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
itcl::body Rappture::VideoViewer::Particle {status win x y} {
    switch -- $status {
        "new" {
            incr _pcnt
            puts "pcnt = ${_pcnt}"
            set name "particle${_pcnt}"
            set p [Rappture::VideoParticle $itk_component(main).#auto $win \
                    -fncallback [itcl::code $this video position cur] \
                    -trajcallback [itcl::code $this Trajectory] \
                    -halo 5 \
                    -name $name \
                    -color green]
            set frameNum [$_movie get position cur]
            $p Add frame $frameNum $x $y
            $p Show particle

            # link the new particle to the last particle added
            set lastp ""
            while {[llength ${_particles}] > 0} {
                set lastp [lindex ${_particles} end]
                if {[llength [$lastp Coords]] != 0} {
                    break
                } else {
                    set _particles [lreplace ${_particles} end end]
                    set lastp ""
                }
            }

            if {[string compare "" $lastp] != 0} {
                $lastp Link $p
                bind $lastp <<Motion>> [itcl::code $lastp drawVectors]]
            }


            # add the particle to the list
            lappend _particles $p

            $win bind $name <ButtonPress-1> [itcl::code $p Move press %x %y]
            $win bind $name <B1-Motion> [itcl::code $p Move motion %x %y]
            $win bind $name <ButtonRelease-1> [itcl::code $p Move release %x %y]

            $win bind $name <ButtonPress-3> [itcl::code $p Menu activate %x %y]

            $win bind $name <Enter> [itcl::code $this togglePtrBind particle]
            $win bind $name <Leave> [itcl::code $this togglePtrBind current]

#            set pm [Rappture::VideoParticleManager]
#            $pm add $p0
#            set plist [$pm list]
        }
        default {
            error "bad status \"$status\": should be new, drag, or release"
        }
    }
}

# ----------------------------------------------------------------------
# Trajectory - draw a trajectory between two particles
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Trajectory {args} {

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
    set p0name [$p0 cget -name]
    set oldlink "vec-$p0name"
    puts "removing $oldlink"
    $itk_component(main) delete $oldlink

    # check to see if p1 exists anymore
    if {[string compare "" $p1] == 0} {
        # p1 does not exist
        return
    }

    foreach {x0 y0} [$p0 Coords] break
    foreach {x1 y1} [$p1 Coords] break
    set p1name [$p1 cget -name]
    set link "vec-$p0name-$p1name"
    puts "adding $link"
    $itk_component(main) create line $x0 $y0 $x1 $y1 \
        -fill green \
        -width 2 \
        -tags "vector $link vec-$p0name" \
        -dash {4 4} \
        -arrow last

    # calculate trajectory, truncate it after 4 sigdigs
    puts "---------$link---------"
    set t [calculateTrajectory [$p0 Frame] $x0 $y0 [$p1 Frame] $x1 $y1]
    set tt [string range $t 0 [expr [string first . $t] + 4]]


    # calculate coords for text
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox $link] break
    set x [expr "$x0 + (abs($x1-$x0)/2)"]
    set y [expr "$y0 + (abs($y1-$y0)/2)"]

    $itk_component(main) create text $x $y \
        -tags "vectext $link vec-$p0name" \
        -justify center \
        -text "$tt [$itk_component(distGauge) cget -units]/s" \
        -fill green \
        -width [expr sqrt(pow(abs($x1-$x0),2)+pow(abs($y1-$y0),2))]
}

# ----------------------------------------------------------------------
# calculateTrajectory - calculate the value of the trajectory
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::calculateTrajectory {args} {
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

    puts "px = $px"
    puts "frames = $frames"
    puts "px2dist = ${_px2dist}"
    puts "framerate = ${_framerate}"
    puts "trajectory = $t"

    return $t
}

