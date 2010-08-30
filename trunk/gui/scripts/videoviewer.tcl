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

    protected method togglePtrCtrl {pbvar}
    protected method togglePtrBind {pbvar}

    protected method Play {}
    protected method Rubberband {status win x y}
    protected method Distance {status win x y}
    protected method Measure {status win x y}
    protected method updateMeasurements {}

    private common   _settings

    private variable _width 0
    private variable _height 0
    private variable _x0 0          ;# start x for rubberbanding
    private variable _y0 0          ;# start y for rubberbanding
    private variable _units "m"
    private variable _movie ""      ;# movie we grab images from
    private variable _imh ""        ;# current image being displayed
    private variable _id ""         ;# id of the next play command from after
    private variable _pbvlist ""    ;# list of push button variables
    private variable _px2dist 0     ;# conversion for screen px to distance
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


    blt::table $itk_component(pointercontrols) \
        0,0 $itk_component(rectangle) -pady {3 0} \
        0,1 $itk_component(distance) -pady {3 0} \
        0,2 $itk_component(measure) -pady {3 0}

    blt::table configure $itk_component(pointercontrols) c* -resize none
    blt::table configure $itk_component(pointercontrols) r* -resize none


    # setup movie controls

    # Rewind
    itk_component add rewind {
        button $itk_component(moviecontrols).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-rewind] \
            -command [itcl::code $this video reset]
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
    bind $itk_component(dial) <<Value>> [itcl::code $this flow goto]
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
    bind $itk_component(speed) <<Value>> [itcl::code $this flow speed]


    blt::table $itk_component(moviecontrols) \
        0,0 $itk_component(rewind) -padx {3 0} \
        0,1 $itk_component(stop) -padx {2 0} \
        0,2 $itk_component(play) -padx {2 0} \
        0,3 $itk_component(loop) -padx {2 0} \
        0,4 $itk_component(dial) -fill x -padx {2 0 } \
        0,5 $itk_component(duration) -padx { 0 0} \
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

    itk_component add measGauge {
        Rappture::Gauge $itk_interior.measGauge \
            -units "m"
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(measGauge) \
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
    set _movie [Rappture::MediaPlayer $filename]

    set _imh [image create photo]
    $_imh put [$_movie read]
    $itk_component(main) create image 0 0 -anchor nw -image $_imh

    $itk_component(main) configure -scrollregion [$itk_component(main) bbox all]
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox all] break
    set w [expr abs($x1-$x0)]
    set h [expr abs($y1-$y0)]
    $itk_component(main) configure -width $w -height $h
    .main configure -width $w -height $h

}

# ----------------------------------------------------------------------
# video - play, stop, rewind, fastforward the video
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::video { args } {
    set option [lindex $args 0]
    switch -- $option {
        "play" {
            if {$_settings($this-play) == 1} {
                # start playing
                Play
            } else {
                # pause
                after cancel $_id
                set _settings($this-play) 0
            }
        }
        "seek" {
        }
        "stop" {
            after cancel $_id
            set _settings($this-play) 0
        }
        default {
            error "bad option \"$option\": should be play, stop, toggle, or reset."
        }
    }
}

# ----------------------------------------------------------------------
# togglePtrCtrl - choose pointer mode:
#                 rectangle, distance, or measure
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::togglePtrCtrl {pbvar} {

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
# togglePtrBind - update the bindings based on pointer controls
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::togglePtrBind {pbvar} {

    if {[string compare $pbvar rectPbVar] == 0} {

        # Bindings for selecting rectangle
        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Rubberband new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Rubberband drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Rubberband release %W %x %y]

    } elseif {[string compare $pbvar distPbVar] == 0} {

        # Bindings for setting distance
        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Distance new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Distance drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Distance release %W %x %y]

    } elseif {[string compare $pbvar measPbVar] == 0} {

        # Bindings for measuring distance
        bind $itk_component(main) <ButtonPress-1> \
            [itcl::code $this Measure new %W %x %y]
        bind $itk_component(main) <B1-Motion> \
            [itcl::code $this Measure drag %W %x %y]
        bind $itk_component(main) <ButtonRelease-1> \
            [itcl::code $this Measure release %W %x %y]

    } else {

        # invalid pointer mode

    }

}


# ----------------------------------------------------------------------
# play - get the next video frame
# ----------------------------------------------------------------------
itcl::body Rappture::VideoViewer::Play {} {
    $_imh put [$_movie read]
    set _id [after 20 [itcl::code $this Play]]
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
    if {$px2dist != $_px2dist} {
        set _px2dist $px2dist
    }

    # if the measure object exists?
    foreach { x0 y0 x1 y1 } [$itk_component(main) bbox "measure"] break
    set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
    set dist [expr $px*$_px2dist]
    $itk_component(measGauge) value $dist
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
            $win delete "distance_val"
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
                -tags "distance_val"
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
            $win delete "measure_val"
            $win create line \
                $x $y $x $y -fill green -width 2  \
                -tags "measure" -dash {4 4} -arrow both
        }
        "drag" {
            set coords [$win coords "measure"]
            eval $win coords "measure" [lreplace $coords 2 3 $x $y]
        }
        "release" {
            Measure drag $win $x $y
            foreach { x0 y0 x1 y1 } [$itk_component(main) bbox "measure"] break
            set rootx [winfo rootx $itk_component(main)]
            set rooty [winfo rooty $itk_component(main)]
            set x [expr "$x0 + (abs($x1-$x0)/2)"]
            set y [expr "$y0 + (abs($y1-$y0)/2)"]
            $itk_component(main) create window $x $y \
                -window $itk_component(measGauge) \
                -anchor center \
                -tags "measure_val"
            set px [expr sqrt(pow(($x1-$x0),2)+pow(($y1-$y0),2))]
            set dist [expr $px*$_px2dist]
            $itk_component(measGauge) value $dist
        }
        default {
            error "bad status \"$status\": should be new, drag, or release"
        }
    }
}
