# ----------------------------------------------------------------------
#  COMPONENT: molvisviewer - view a molecule in 3D
#
#  This widget brings up a 3D representation of a molecule
#  It connects to the Molvis server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

option add *MolvisViewer.width 4i widgetDefault
option add *MolvisViewer.height 4i widgetDefault
option add *MolvisViewer.foreground black widgetDefault
option add *MolvisViewer.controlBackground gray widgetDefault
option add *MolvisViewer.controlDarkBackground #999999 widgetDefault
option add *MolvisViewer.font -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::MolvisViewer {
    inherit itk::Widget
    itk_option define -device device Device ""

    constructor {hostlist args} { # defined below }
    destructor { # defined below }

    public method emblems {option}

    public method connect {{hostlist ""}}
    public method disconnect {}
    public method isconnected {}

    protected method _send {args}
    protected method _receive {}
    protected method _receive_image {size}
    protected method _rebuild {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _speed {option}
    protected method _serverDown {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _sid ""       ;# socket connection to nanovis server
    private variable _image        ;# image displayed in plotting area

    private variable _click        ;# info used for _move operations
}

itk::usual MolvisViewer {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::constructor {hostlist args} {
    #puts stderr "MolvisViewer::_constructor()"
    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this _serverDown]; list"

    #
    # Set up the widgets in the main body
    #
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add bottom_controls {
        frame $itk_interior.b_cntls
    } {
	usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(bottom_controls) -side bottom -fill y

    itk_component add mrewind {
	    button $itk_component(bottom_controls).mrewind \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text "|<" \
	    -command [itcl::code $this _send rewind]
    } {
        usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mrewind) -padx 4 -pady 4 -side left

    itk_component add mbackward {
	    button $itk_component(bottom_controls).mbackward \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text "<" \
	    -command [itcl::code $this _send backward]
    } {
        usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mbackward) -padx 4 -pady 4 -side left

    itk_component add mstop {
	    button $itk_component(bottom_controls).mstop \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text "Stop" \
	    -command [itcl::code $this _send mstop]
    } {
        usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mstop) -padx 4 -pady 4 -side left

    itk_component add mplay {
	    button $itk_component(bottom_controls).mplay \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text "Play" \
	    -command [itcl::code $this _send mplay]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mplay) -padx 4 -pady 4 -side left
    
    itk_component add mforward {
	    button $itk_component(bottom_controls).mforward \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text ">" \
	    -command [itcl::code $this _send forward]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mforward) -padx 4 -pady 4 -side left
    
    itk_component add mend {
	    button $itk_component(bottom_controls).mend \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text ">|" \
	    -command [itcl::code $this _send ending]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mend) -padx 4 -pady 4 -side left
    
    itk_component add mclear {
	    button $itk_component(bottom_controls).mclear \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -text "MClear" \
	    -command [itcl::code $this _send mclear]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(mclear) -padx 4 -pady 4 -side left
    
    itk_component add speed {
	    ::scale $itk_component(bottom_controls).speed \
	    -borderwidth 1 \
	    -from 100 -to 1000 -orient horizontal \
	    -command [itcl::code $this _speed]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(speed) -padx 4 -pady 4 -side right
            
    itk_component add left_controls {
        frame $itk_interior.l_cntls
	} {
        usual
        rename -background -controlbackground controlBackground Background
	}
    pack $itk_component(left_controls) -side left -fill y

    itk_component add show_ball_and_stick {
	    button $itk_component(left_controls).sbs \
	    -borderwidth 2 -padx 0 -pady 0 \
            -image [Rappture::icon ballnstick] \
	    -command [itcl::code $this _send ball_and_stick]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_ball_and_stick) -padx 4 -pady 4

    itk_component add show_spheres {
	    button $itk_component(left_controls).ss \
	    -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon spheres] \
	    -command [itcl::code $this _send spheres]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_spheres) -padx 4 -pady 4

    itk_component add show_lines {
	    button $itk_component(left_controls).sl \
	    -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon lines] \
	    -command [itcl::code $this _send lines]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_lines) -padx 4 -pady 4

    itk_component add controls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon reset] \
            -command [itcl::code $this _send reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(controls).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomin] \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(controls).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomout] \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add labels {
        label $itk_component(controls).labels \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon atoms]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(labels) -padx 4 -pady 8 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(labels) "Show/hide the labels on atoms"
    bind $itk_component(labels) <ButtonPress> \
        [itcl::code $this emblems toggle]

    itk_component add rock {
        button $itk_component(controls).rock \
            -borderwidth 1 -padx 1 -pady 1 \
            -text "R" \
            -command [itcl::code $this _send rock]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(rock) -padx 4 -pady 8 -ipadx 1 -ipady 1

    #
    # RENDERING AREA
    #

    itk_component add area {
        frame $itk_interior.area
    }
    pack $itk_component(area) -expand yes -fill both

    set _image(plot) [image create photo]

    itk_component add 3dview {
        label $itk_component(area).vol -image $_image(plot) \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(3dview) -expand yes -fill both

    # set up bindings for rotation
    bind $itk_component(3dview) <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(3dview) <ButtonRelease> \
        [itcl::code $this _move release %x %y]
    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _send screen %w %h]

    connect $hostlist

    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"
    
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::destructor {} {
    # puts stderr "MolvisViewer::destructor()"
    after cancel [itcl::code $this _rebuild]
    image delete $_image(plot)
}

# ----------------------------------------------------------------------
# USAGE: connect ?<host:port>,<host:port>...?
#
# Clients use this method to establish a connection to a new
# server, or to reestablish a connection to the previous server.
# Any existing connection is automatically closed.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::connect {{hostlist ""}} {
    # puts stderr "MolvisViewer::connect()"

    if ([isconnected]) {
        disconnect
    }

    if {"" == $hostlist} {
        return 0
    }

    blt::busy hold $itk_component(hull); 
    
    update idletasks

    #
    # Connect to the hubvis server.  
    # If it's too busy, that server may
    # forward us to another.
    #

    set hosts [split $hostlist ,]

    foreach {hostname port} [split [lindex $hosts 0] :] break

    set hosts [lrange $hosts 1 end]

    while {1} {
        if {[catch {socket $hostname $port} sid]} {
            if {[llength $hosts] == 0} {
                blt::busy release $itk_component(hull)
                return 0
            }
            foreach {hostname port} [split [lindex $hosts 0] :] break
            set hosts [lrange $hosts 1 end]
            continue
        }
        fconfigure $sid -translation binary -encoding binary -buffering line
        puts -nonewline $sid "AB01"
        flush $sid

        # read back a reconnection order
        set data [read $sid 4]

        if {[binary scan $data cccc b1 b2 b3 b4] != 4} {
            error "couldn't read redirection request"
        }

        set hostname [format "%u.%u.%u.%u" \
            [expr {$b1 & 0xff}] \
            [expr {$b2 & 0xff}] \
            [expr {$b3 & 0xff}] \
            [expr {$b4 & 0xff}]]

        if {[string equal $hostname "0.0.0.0"]} {
            fileevent $sid readable [itcl::code $this _receive]
            set _sid $sid
            blt::busy release $itk_component(hull)
            return 1
        }
    }

    blt::busy release $itk_component(hull)

    return 0
}

# ----------------------------------------------------------------------
# USAGE: disconnect
#
# Clients use this method to disconnect from the current rendering
# server.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::disconnect {} {
    #puts stderr "MolvisViewer::disconnect()"

    if {"" != $_sid} {
        catch {
            close $_sid
        }
        set _sid ""
    }
}

# ----------------------------------------------------------------------
# USAGE: isconnected
#
# Clients use this method to see if we are currently connected to
# a server.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::isconnected {} {
    #puts stderr "MolvisViewer::isconnected()"
    return [expr {"" != $_sid}]
}

# ----------------------------------------------------------------------
# USAGE: _send <arg> <arg> ...
#
# Used internally to send commands off to the rendering server.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_send {args} {
    # puts stderr "MolvisViewer::_send() $args"
    if {"" == $_sid} {
        $_dispatcher cancel !serverDown
        set x [expr {[winfo rootx $itk_component(area)]+10}]
        set y [expr {[winfo rooty $itk_component(area)]+10}]
        Rappture::Tooltip::cue @$x,$y "Connecting..."

        if {[catch {connect} ok] == 0 && $ok} {
            set w [winfo width $itk_component(3dview)]
            set h [winfo height $itk_component(3dview)]
            puts $_sid "screen $w $h"
            after idle [itcl::code $this _rebuild]
            Rappture::Tooltip::cue hide
            return
        }

        Rappture::Tooltip::cue @$x,$y "Can't connect to visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
        
	return
    }

    if {"" != $_sid} {
        puts $_sid $args
    }
}

# ----------------------------------------------------------------------
# USAGE: _receive
#
# Invoked automatically whenever a command is received from the
# rendering server.  Reads the incoming command and executes it in
# a safe interpreter to handle the action.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_receive {} {
    #puts stderr "MolvisViewer::_receive()"
    if {"" != $_sid} {
        if {[gets $_sid line] < 0} {
            disconnect
            $_dispatcher event -after 750 !serverDown
        } elseif {[regexp {^\s*nv>\s*image\s+(\d+)\s*$} $line whole match]} {
            set bytes [read $_sid $match]
            $_image(plot) configure -data $bytes
            update idletasks
        } else {
            # this shows errors coming back from the engine
            puts $line
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_rebuild {} {
    #puts stderr "MolvisViewer::_rebuild()"
    set recname  "ATOM  "
    set serial   0
    set atom     ""
    set altLoc   ""
    set resName  ""
    set chainID  ""
    set Seqno    ""
    set x        0
    set y        0
    set z        0
    set occupancy  1
    set tempFactor 0
    set recID      ""
    set segID      ""
    set element    ""
    set charge     ""
    set data1      ""
    set data2      ""
    
    if {$itk_option(-device) != ""} {
        set dev $itk_option(-device)

        foreach _atom [$dev children -type atom components.molecule] {
            set symbol [$dev get components.molecule.$_atom.symbol]
            set xyz [$dev get components.molecule.$_atom.xyz]
            regsub {,} $xyz {} xyz
	    scan $xyz "%f %f %f" x y z
	    set atom $symbol
	    set line [format "%6s%5d %4s%1s%3s %1s%5s   %8.3f%8.3f%8.3f%6.2f%6.2f%8s\n" $recname $serial $atom $altLoc $resName $chainID $Seqno $x $y $z $occupancy $tempFactor $recID]
	    append data1 $line
	    incr serial
	}

	set data2 [$dev get components.molecule.pdb]

    }

    if {"" != $data1} {
    	eval _send loadpdb \"$data1\" data1
    }

    if {"" != $data2} {
        eval _send loadpdb \"$data2\" data2
    }
}

itcl::body Rappture::MolvisViewer::_speed {option} {
	#puts stderr "MolvisViewer::_speed($option)"
	_send mspeed $option
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_zoom {option} {
    #puts stderr "MolvisViewer::_zoom()"
    switch -- $option {
        in {
            _send camera zoom 10
        }
        out {
            _send camera zoom -10
        }
        reset {
            _send reset
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
# USAGE: _move drag <x> <y>
# USAGE: _move release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_move {option x y} {
    #puts stderr "MolvisViewer::_move($option $x $y)"
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
	    set _click(time) [clock clicks -milliseconds]
        }
        drag {
            if {[array size _click] == 0} {
                _move click $x $y
            } else {
	    	set now [clock clicks -milliseconds]
		set diff [expr {abs($_click(time) - $now)}]
		if {$diff < 75} { # 75ms between motion updates
			return
		}
                set w [winfo width $itk_component(3dview)]
                set h [winfo height $itk_component(3dview)]
                if {$w <= 0 || $h <= 0} {
		    return
                }

                eval _send camera angle [expr $y-$_click(y)] [expr $x-$_click(x)] 

                set _click(x) $x
                set _click(y) $y
		set _click(time) $now
            }
        }
        release {
            _move drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _serverDown
#
# Used internally to let the user know when the connection to the
# visualization server has been lost.  Puts up a tip encouraging the
# user to press any control to reconnect.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_serverDown {} {
    #puts stderr "MolvisViewer::_serverDown()"
    set x [expr {[winfo rootx $itk_component(area)]+10}]
    set y [expr {[winfo rooty $itk_component(area)]+10}]
    Rappture::Tooltip::cue @$x,$y "Lost connection to visualization server.  This happens sometimes when there are too many users and the system runs out of memory.\n\nTo reconnect, reset the view or press any other control.  Your picture should come right back up."
}

# ----------------------------------------------------------------------
# USAGE: emblems on
# USAGE: emblems off
# USAGE: emblems toggle
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::emblems {option} {
    #puts stderr "MolvisViewer::emblems($option)"
    switch -- $option {
        on {
            set state 1
        }
        off {
            set state 0
        }
        toggle {
            if {[$itk_component(labels) cget -relief] == "sunken"} {
                set state 0
            } else {
                set state 1
            }
        }
        default {
            error "bad option \"$option\": should be on, off, toggle"
        }
    }

    if {$state} {
        $itk_component(labels) configure -relief sunken
        _send label on
    } else {
        $itk_component(labels) configure -relief raised
        _send label off
    }
}

# ----------------------------------------------------------------------
# OPTION: -device
# ----------------------------------------------------------------------
itcl::configbody Rappture::MolvisViewer::device {
    #puts stderr "MolvisViewer::device()"

    if {$itk_option(-device) != "" } {

        if {![Rappture::library isvalid $itk_option(-device)]} {
            error "bad value \"$itk_option(-device)\": should be Rappture::library object"
        }

        set state [$itk_option(-device) get components.molecule.about.emblems]

        if {$state == "" || ![string is boolean $state] || !$state} {
            emblems off
        } else {
            emblems on
        }
    }

    $_dispatcher event -idle !rebuild
}

