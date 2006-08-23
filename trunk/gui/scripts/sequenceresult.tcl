# ----------------------------------------------------------------------
#  COMPONENT: sequenceresult - series of results forming an animation
#
#  This widget displays a series of results of the same type that are
#  grouped together and displayed as an animation.  The user can play
#  through the results, or single step through individual values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *SequenceResult.width 3i widgetDefault
option add *SequenceResult.height 3i widgetDefault
option add *SequenceResult.controlBackground gray widgetDefault
option add *SequenceResult.dialProgressColor #ccccff widgetDefault
option add *SequenceResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *SequenceResult.boldFont \
    -*-helvetica-bold-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::SequenceResult {
    inherit itk::Widget

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {option args}

    public method play {}
    public method pause {}
    public method goto {{newval ""}}

    protected method _rebuild {args}
    protected method _playFrame {}
    protected method _fixValue {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _dlist ""      ;# list of data objects
    private variable _topmost ""    ;# topmost data object in _dlist
    private variable _indices ""    ;# list of active indices
    private variable _pos 0         ;# current position in the animation
    private variable _afterId ""    ;# current "after" event for play op

    private common _play            ;# options for "play" operation
    set _play(speed) 40
    set _play(loop) 0
}
                                                                                
itk::usual SequenceResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild [itcl::code $this _rebuild]

    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add player {
        frame $itk_interior.player
    }
    pack $itk_component(player) -side bottom -fill x
    grid columnconfigure $itk_component(player) 1 -weight 1

    itk_component add play {
        button $itk_component(player).play \
            -bitmap [Rappture::icon play] \
            -command [itcl::code $this play]
    }
    grid $itk_component(play) -row 0 -rowspan 2 -column 0 \
        -ipadx 2 -padx {0 4} -pady 4 -sticky nsew

    itk_component add dial {
        Rappture::Radiodial $itk_component(player).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        keep -dialprogresscolor
    }
    grid $itk_component(dial) -row 1 -column 1 -sticky ew
    bind $itk_component(dial) <<Value>> [itcl::code $this _fixValue]

    itk_component add info {
        frame $itk_component(player).info
    }
    grid $itk_component(info) -row 0 -column 1 -columnspan 2 -sticky ew

    itk_component add indexLabel {
        label $itk_component(info).ilabel
    } {
        usual
        rename -font -boldfont boldFont Font
    }
    pack $itk_component(indexLabel) -side left

    itk_component add indexValue {
        label $itk_component(info).ivalue -padx 0
    }
    pack $itk_component(indexValue) -side left

    itk_component add options {
        button $itk_component(player).options -text "Options..." \
            -padx 1 -pady 0 -relief flat -overrelief raised
    }
    grid $itk_component(options) -row 1 -column 2 -sticky sw

    #
    # Popup option panel
    #
    set fn [option get $itk_component(hull) font Font]
    set bfn [option get $itk_component(hull) boldFont Font]

    Rappture::Balloon $itk_component(hull).popup \
        -title "Player Settings" -padx 4 -pady 4
    set inner [$itk_component(hull).popup component inner]

    label $inner.loopl -text "Loop:" -font $bfn
    grid $inner.loopl -row 0 -column 0 -sticky e
    radiobutton $inner.loopOn -text "Play once and stop" -font $fn \
        -variable ::Rappture::SequenceResult::_play(loop) -value 0
    grid $inner.loopOn -row 0 -column 1 -sticky w
    radiobutton $inner.loopOff -text "Play continuously" -font $fn \
        -variable ::Rappture::SequenceResult::_play(loop) -value 1
    grid $inner.loopOff -row 1 -column 1 -sticky w
    grid rowconfigure $inner 2 -minsize 8

    label $inner.speedl -text "Speed:" -font $bfn
    grid $inner.speedl -row 3 -column 0 -sticky e
    frame $inner.speed
    grid $inner.speed -row 3 -column 1 -sticky ew
    label $inner.speed.slowl -text "Slower" -font $fn
    pack $inner.speed.slowl -side left
    ::scale $inner.speed.value -from 100 -to 1 \
        -showvalue 0 -orient horizontal \
        -variable ::Rappture::SequenceResult::_play(speed)
    pack $inner.speed.value -side left
    label $inner.speed.fastl -text "Faster" -font $fn
    pack $inner.speed.fastl -side left

    $itk_component(options) configure -command \
        [list $itk_component(hull).popup activate $itk_component(options) above]

    #
    # Main viewer
    #
    itk_component add area {
        frame $itk_interior.area
    }
    pack $itk_component(area) -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::destructor {} {
    pause  ;# stop any animation that might be playing
}

# ----------------------------------------------------------------------
# USAGE: add <sequence> ?<settings>?
#
# Clients use this to add a data sequence to the viewer.  The optional
# <settings> are used to configure the display of the data.  Allowed
# settings are -color, -brightness, -width, -linestyle and -raise.
# The only setting used here is -raise, which indicates the current
# object.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -raise 0
        -linestyle solid
        -description ""
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    if {$params(-raise) && "" == $_topmost} {
        set _topmost $dataobj
    }
    lappend _dlist $dataobj

    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of data objects being displayed,
# in order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist

    set i [lsearch $_dlist $_topmost]
    if {$i >= 0} {
        set dlist [lreplace $dlist $i $i]
        set dlist [linsert $dlist 0 $_topmost]
    }
    return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a data object from the viewer.  If no
# data objects are specified, then all data objects are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified curves
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            set changed 1

            if {$dataobj == $_topmost} {
                set _topmost ""
            }
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all data objects--even those not showing on the screen.
# Because of this, the limits are appropriate for all data objects as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::scale {args} {
    # do nothing
}

# ----------------------------------------------------------------------
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::download {option args} {
    switch $option {
        coming {
            return [$itk_component(area).viewer download coming]
        }
        controls {
            return [eval $itk_component(area).viewer download controls $args]
        }
        now {
            if {0} {
                # produce a movie of results
                set rval ""
                if {"" != $_topmost} {
                    set max [$_topmost size]
                    set all ""
                    for {set i 0} {$i < $max} {incr i} {
                        set dataobj [lindex [$_topmost value $i] 0]
                        if {[catch {$dataobj tkimage} imh] == 0} {
                            lappend all $imh
                        }
                    }
                    if {[llength $all] > 0} {
                        set delay [expr {int(ceil(pow($_play(speed)/10.0,2.0)*5))}]
                        set rval [eval Rappture::icon::gif_animate $delay $all]
                    }
                }
                if {[string length $rval] > 0} {
                    return [list .gif $rval]
                }
                return ""
            }

            # otherwise, return download of single frame
            return [$itk_component(area).viewer download now]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: play
#
# Invoked when the user hits the "play" button to play the current
# sequence of frames as a movie.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::play {} {
    # cancel any existing animation
    pause

    # at the end? then restart fresh
    if {$_pos >= [llength $_indices]-1} {
        goto 0
    }

    # toggle the button to "pause" mode
    $itk_component(play) configure \
        -bitmap [Rappture::icon pause] \
        -command [itcl::code $this pause]

    # schedule the first frame
    set delay [expr {int(ceil(pow($_play(speed)/10.0,2.0)*5))}]
    set _afterId [after $delay [itcl::code $this _playFrame]]
}

# ----------------------------------------------------------------------
# USAGE: pause
#
# Invoked when the user hits the "pause" button to stop playing the
# current sequence of frames as a movie.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::pause {} {
    if {"" != $_afterId} {
        catch {after cancel $_afterId}
        set _afterId ""
    }

    # toggle the button to "play" mode
    $itk_component(play) configure \
        -bitmap [Rappture::icon play] \
        -command [itcl::code $this play]
}

# ----------------------------------------------------------------------
# USAGE: goto ?<index>?
#
# Used internally to move the current position of the animation to
# the frame at a particular <index>.  If the <index> is not specified,
# then it returns the current position.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::goto {{newval ""}} {
    if {"" == $newval} {
        return $_pos
    }
    set _pos $newval
    set val [$itk_component(dial) get -format label @$_pos]
    $itk_component(dial) current $val
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Invoked automatically whenever the data displayed in this viewer
# changes.  Loads the data from the topmost (current) value into
# the viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::_rebuild {args} {
    if {"" == $_topmost && [llength $_dlist] > 0} {
        set _topmost [lindex $_dlist 0]
    }

    #
    # If we have any data, then show the viewer.
    # Otherwise, hide it.
    #
    set viewer $itk_component(area).viewer
    if {[winfo exists $viewer]} {
        if {"" == $_topmost} {
            pack forget $viewer
            pack forget $itk_component(player)
            return
        } else {
            pack $viewer -expand yes -fill both
            pack $itk_component(player) -side bottom -fill x
        }
    } else {
        if {"" == $_topmost} {
            return
        }

        set type ""
        if {[$_topmost size] > 0} {
            set dataobj [lindex [$_topmost value 0] 0]
            set type [$dataobj info class]
        }
        switch -- $type {
            ::Rappture::Curve {
                Rappture::XyResult $viewer
                pack $viewer -expand yes -fill both
            }
            ::Rappture::Image {
                Rappture::ImageResult $viewer
                pack $viewer -expand yes -fill both
            }
            ::Rappture::Field {
                Rappture::Field3DResult $viewer
                pack $viewer -expand yes -fill both
            }
            default {
                error "don't know how to view sequences of $type"
            }
        }
    }

    #
    # Load the current sequence info the viewer.
    #
    $itk_component(indexLabel) configure -text [$_topmost hints indexlabel]

    $viewer delete
    $itk_component(dial) clear

    set max [$_topmost size]
    set all ""
    for {set i 0} {$i < $max} {incr i} {
        eval lappend all [$_topmost value $i]
    }
    eval $viewer scale $all

    set _indices ""
    for {set i 0} {$i < $max} {incr i} {
        set index [$_topmost index $i]
        eval $itk_component(dial) add $index
        lappend _indices [lindex $index 0]
    }
    _fixValue
}

# ----------------------------------------------------------------------
# USAGE: _playFrame
#
# Used internally to advance each frame in the animation.  Advances
# the frame and displays it.  When we reach the end of the animation,
# we either loop back or stop.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::_playFrame {} {
    set _pos [expr {$_pos+1}]
    set last [expr {[llength $_indices]-1}]

    if {$_pos > $last} {
        if {$_play(loop)} {
            set _pos 0
        } else {
            set _pos $last
            pause
            return
        }
    }
    goto $_pos

    set delay [expr {int(ceil(pow($_play(speed)/10.0,2.0)*5))}]
    set _afterId [after $delay [itcl::code $this _playFrame]]
}

# ----------------------------------------------------------------------
# USAGE: _fixValue
#
# Invoked automatically whenever the value on the dial changes.
# Updates the viewer to display the value for the selected result.
# ----------------------------------------------------------------------
itcl::body Rappture::SequenceResult::_fixValue {} {
    set viewer $itk_component(area).viewer
    if {![winfo exists $viewer]} {
        return
    }

    set val [$itk_component(dial) get -format label current]
    $itk_component(indexValue) configure -text "= $val"
    set _pos [lsearch -glob $_indices $val*]

    $viewer delete
    if {"" != $_topmost} {
        foreach dataobj [$_topmost value $_pos] {
            $viewer add $dataobj ""
        }
    }
}
