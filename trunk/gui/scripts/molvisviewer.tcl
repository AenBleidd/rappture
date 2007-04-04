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
    public method representation {option}

    public method connect {{hostlist ""}}
    public method disconnect {}
    public method isconnected {}
    public method download {option args}
    protected method _rock {option}
    protected method _send {args}
    protected method _receive {}
    protected method _update { args }
    protected method _rebuild {}
    protected method _zoom {option}
    protected method _vmouse2 {option b m x y}
    protected method _vmouse  {option b m x y}
    protected method _serverDown {}
    protected method _decodeb64 { arg }

    private variable _base64 ""
    private variable _dispatcher "" ;# dispatcher for !events
    private variable _sid ""       ;# socket connection to nanovis server
    private variable _image        ;# image displayed in plotting area

    private variable _mevent       ;# info used for mouse event operations
    private variable _rocker       ;# info used for rock operations


    private variable _dataobjs     ;# data objects on server
    private variable _imagecache
    private variable _state 1
    private variable _labels 
    private variable _cacheid ""
    private variable _hostlist ""
    private variable _model ""
    private variable _mrepresentation "spheres"
    private variable _cacheimage ""
}

itk::usual MolvisViewer {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::constructor {hostlist args} {
    #puts stderr "MolvisViewer::_constructor()"

    set _rocker(dir) 1
    set _rocker(x) 0
    set _rocker(on) 0

    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this _serverDown]; list"
    #
    # Set up the widgets in the main body
    #
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

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
            -command [itcl::code $this representation ball-and-stick]
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
            -command [itcl::code $this representation spheres]
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
            -command [itcl::code $this representation lines]
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
        label $itk_component(controls).rock \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -text "R" \
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(rock) -padx 4 -pady 8 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(rock) "Rock model +/- 10 degrees"

    bind $itk_component(rock) <ButtonPress> \
        [itcl::code $this _rock toggle]

    #
    # RENDERING AREA
    #

    itk_component add area {
        frame $itk_interior.area
    }
    pack $itk_component(area) -expand yes -fill both

    set _image(plot) [image create photo]
    set _image(id) ""

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
        [itcl::code $this _vmouse click %b %s %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _vmouse drag 1 %s %x %y]
    bind $itk_component(3dview) <ButtonRelease> \
        [itcl::code $this _vmouse release %b %s %x %y]

    # set up bindings to bridge mouse events to server
    #bind $itk_component(3dview) <ButtonPress> \
    #   [itcl::code $this _vmouse2 click %b %s %x %y]
    #bind $itk_component(3dview) <ButtonRelease> \
    #    [itcl::code $this _vmouse2 release %b %s %x %y]
    #bind $itk_component(3dview) <B1-Motion> \
    #    [itcl::code $this _vmouse2 drag 1 %s %x %y]
    #bind $itk_component(3dview) <B2-Motion> \
    #    [itcl::code $this _vmouse2 drag 2 %s %x %y]
    #bind $itk_component(3dview) <B3-Motion> \
    #    [itcl::code $this _vmouse2 drag 3 %s %x %y]
    #bind $itk_component(3dview) <Motion> \
    #    [itcl::code $this _vmouse2 move 0 %s %x %y]

    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _send screen %w %h]

	connect $hostlist

    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"
    
    eval itk_initialize $args

    _update forever
    set _state 0
    set _model ""
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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::download {option args} {
    switch $option {
        coming {}
        controls {}
        now {
            return [list .jpg [Rappture::encoding::decode -as b64 [$_image(plot) data -format jpeg]]]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: connect ?<host:port>,<host:port>...?
#
# Clients use this method to establish a connection to a new
# server, or to reestablish a connection to the previous server.
# Any existing connection is automatically closed.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::connect {{hostlist ""}} {
    if { "" != $hostlist } { set _hostlist $hostlist }

    set hostlist $_hostlist

    puts stderr "MolvisViewer::connect($hostlist)"

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
        puts stderr "Connecting to $hostname:$port"
        if {[catch {socket $hostname $port} sid]} {
            if {[llength $hosts] == 0} {
                blt::busy release $itk_component(hull)
                return 0
            }
            foreach {hostname port} [split [lindex $hosts 0] :] break
            set hosts [lrange $hosts 1 end]
            continue
        }
        fconfigure $sid -translation binary -encoding binary -buffering line -buffersize 1000
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
            unset _dataobjs
            unset _imagecache
        }
        set _sid ""
        set _model ""
        set _state ""
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
    if {"" == $_sid} {
        $_dispatcher cancel !serverDown
        set x [expr {[winfo rootx $itk_component(area)]+10}]
        set y [expr {[winfo rooty $itk_component(area)]+10}]
        Rappture::Tooltip::cue @$x,$y "Connecting..."
        update idletasks

        if {[catch {connect} ok] == 0 && $ok} {
            set w [winfo width $itk_component(3dview)]
            set h [winfo height $itk_component(3dview)]
            puts $_sid "screen $w $h"
            flush $_sid
            after idle [itcl::code $this _rebuild]
            Rappture::Tooltip::cue hide
            return
        }

        Rappture::Tooltip::cue @$x,$y "Can't connect to visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
        
        return
    }

    if {"" != $_sid} {
        puts $_sid $args
        flush $_sid
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

    if {"" != $_sid} { fileevent $_sid readable {} }

    while {$_sid != ""} {
        fconfigure $_sid -buffering line -blocking 0
        
        if {[gets $_sid line] < 0} {
            if { [fblocked $_sid] } { 
                break; 
            }
            
            disconnect
            
            $_dispatcher event -after 750 !serverDown
        } elseif {[regexp {^\s*nv>\s*image\s+(\d+)\s*(\d+)\s*,\s*(\d+)\s*,\s*(-{0,1}\d+)} $line whole match cacheid frame rock]} {
            set tag "$frame,$rock"
    
            if { $cacheid != $_cacheid } {
                catch { unset _imagecache }
                set _cacheid $cacheid
            }

            fconfigure $_sid -buffering none -blocking 1
               set _imagecache($tag) [read $_sid $match]
            $_image(plot) configure -data $_imagecache($tag)
            set _image(id) $tag
            update idletasks
            break
        } else {
            # this shows errors coming back from the engine
            puts $line
        }
    }

    if { "" != $_sid } { fileevent $_sid readable [itcl::code $this _receive] }
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
        set model [$dev get components.molecule.model]
        set _state [$dev get components.molecule.state]
        
        if {"" == $model } { 
		    set model "molecule"
            scan $dev "::libraryObj%d" suffix
		    set model $model$suffix	
        }
        if {"" == $_state} { set _state 1 }

        if { $model != $_model && $_model != "" } {
            _send disable $_model 0
        }

        if { [info exists _dataobjs($model-$_state)] } {
            if { $model != $_model } {
                _send enable $model 1
                set _model $model
            }
        } else {

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

            if {"" != $data1} {
                    eval _send loadpdb \"$data1\" $model $_state
                    set _dataobjs($model-$_state)  1
                if {$_model != $model} {
                    set _model $model
                    representation $_mrepresentation
                }
                    puts stderr "loaded model $model into state $_state"
            }

            if {"" != $data2} {
                eval _send loadpdb \"$data2\" $model $_state
                    set _dataobjs($model-$_state)  1
                if {$_model != $model} {
                    set _model $model
                    representation $_mrepresentation
                }
                puts stderr "loaded model $model into state $_state"
            }
        }   
        if { ![info exists _imagecache($_state,$_rocker(x))] } {
            _send frame $_state 1
        } else {
            _send frame $_state 0
        }
    } else {
        _send raw disable all
    }
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

itcl::body Rappture::MolvisViewer::_update { args } {
    if { [info exists _imagecache($_state,$_rocker(x))] } {
            if { $_image(id) != "$_state,$_rocker(x)" } {
                $_image(plot) put $_imagecache($_state,$_rocker(x))
                update idletasks
            }
    }

    if { $args == "forever" } {
        after 100 [itcl::code $this _update forever]
    }

}

# ----------------------------------------------------------------------
# USAGE: _vmouse click <x> <y>
# USAGE: _vmouse drag <x> <y>
# USAGE: _vmouse release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::_rock { option } {
    # puts "MolvisViewer::_rock()"
    
    if { $option == "toggle" } {
        if { $_rocker(on) } {
            set option "off"
        } else {
            set option "on"
        }
    }

    if { $option == "on" || $option == "toggle" && !$_rocker(on) } {
        set _rocker(on) 1
        $itk_component(rock) configure -relief sunken
    } elseif { $option == "off" || $option == "toggle" && $_rocker(on) } {
        set _rocker(on) 0
        $itk_component(rock) configure -relief raised
    } elseif { $option == "step" } {

        if { $_rocker(x) >= 10 } {
            set _rocker(dir) -1
        } elseif { $_rocker(x) <= -10 } {
            set _rocker(dir) 1
        }
   
        set _rocker(x) [expr $_rocker(x) + $_rocker(dir) ]

        if { [info exists _imagecache($_state,$_rocker(x))] } {
            _send rock $_rocker(dir)
        } else {
            _send rock $_rocker(dir) $_rocker(x)
        }
    }

	if { $_rocker(on) } {
        after 200 [itcl::code $this _rock step]
    }
}

itcl::body Rappture::MolvisViewer::_vmouse2 {option b m x y} {
    # puts stderr "MolvisViewer::_vmouse2($option $b $m $x $y)"

    set vButton [expr $b - 1]
    set vModifier 0
    set vState 1

    if { $m & 1 }      { set vModifier [expr $vModifier | 1 ] }
    if { $m & 4 }      { set vModifier [expr $vModifier | 2 ] }
    if { $m & 131072 } { set vModifier [expr $vModifier | 4 ] }

    if { $option == "click"   } { set vState 0 }
    if { $option == "release" } { set vState 1 }
    if { $option == "drag"    } { set vState 2 }
    if { $option == "move"    } { set vState 3 }

    if { $vState == 2 || $vState == 3} {
        set now [clock clicks -milliseconds]
        set diff 0

	catch { set diff [expr {abs($_mevent(time) - $now)}] } 

        if {$diff < 75} { # 75ms between motion updates
            return
        }
    }

     _send vmouse $vButton $vModifier $vState $x $y

    set _mevent(time) [clock clicks -milliseconds]
}

itcl::body Rappture::MolvisViewer::_vmouse {option b m x y} {
    #puts stderr "MolvisViewer::_vmouse($option $b $m $x $y)"
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set _mevent(x) $x
            set _mevent(y) $y
            set _mevent(time) [clock clicks -milliseconds]
        }
        drag {
            if {[array size _mevent] == 0} {
                 _vmouse click $b $m $x $y
            } else {
                set now [clock clicks -milliseconds]
                set diff [expr {abs($_mevent(time) - $now)}]
                if {$diff < 75} { # 75ms between motion updates
                        return
                }
                set w [winfo width $itk_component(3dview)]
                set h [winfo height $itk_component(3dview)]
                if {$w <= 0 || $h <= 0} {
                    return
                }

		set x1 [expr $w / 3]
		set x2 [expr $x1 * 2]
		set x3 $w
                set y1 [expr $h / 3]
		set y2 [expr $y1 * 2]
		set y3 $h
		set dx [expr $x - $_mevent(x)]
		set dy [expr $y - $_mevent(y)]
                set mx 0
		set my 0
		set mz 0

		if { $_mevent(x) < $x1 } {
                    set mz $dy
		} elseif { $_mevent(x) < $x2 } {
		    set mx $dy	
                } else {
		    set mz [expr -$dy]
		}

		if { $_mevent(y) < $y1 } {
		    set mz [expr -$dx]
		} elseif { $_mevent(y) < $y2 } {
		    set my $dx	
                } else {
		    set mz $dx
		}

                _send camera angle $mx $my $mz
                set _mevent(x) $x
                set _mevent(y) $y
                set _mevent(time) $now
            }
        }
        release {
            _vmouse drag $b $m $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset _mevent}
        }
		move { }
        default {
            error "bad option \"$option\": should be click, drag, release, move"
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
    # this would automatically switch to vtk viewer:
    # set parent [winfo parent $itk_component(hull)]
    # $parent viewer vtk
    Rappture::Tooltip::cue @$x,$y "Lost connection to visualization server.  This happens sometimes when there are too many users and the system runs out of memory.\n\nTo reconnect, reset the view or press any other control.  Your picture should come right back up."
}

# ----------------------------------------------------------------------
# USAGE: representation spheres
# USAGE: representation ball-and-stick
# USAGE: representation lines
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::representation {option} {
    #puts "Rappture::MolvisViewer::representation($option)"
    switch -- $option {
        spheres {
            _send spheres
             set _mrepresentation "spheres"
        }
        ball-and-stick {
            _send ball_and_stick
             set _mrepresentation "ball-and-stick"
        }
        lines {
            _send lines 
             set _mrepresentation "lines"
        }
    }
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

    if {[$itk_component(labels) cget -relief] == "sunken"} {
        set current_emblem 1
    } else {
        set current_emblem 0
    }

    switch -- $option {
        on {
            set emblem 1
        }
        off {
            set emblem 0
        }
        toggle {
            if { $current_emblem == 1 } {
                set emblem 0
            } else {
                set emblem 1
            }
        }
        default {
            error "bad option \"$option\": should be on, off, toggle"
        }
    }

    set _labels $emblem

    if {$emblem == $current_emblem} { return }

    if {$emblem} {
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

	if { ![info exists _labels] } {
            set emblem [$itk_option(-device) get components.molecule.about.emblems]

            if {$emblem == "" || ![string is boolean $emblem] || !$emblem} {
                emblems off
            } else {
                emblems on
            }
        }
    }

    $_dispatcher event -idle !rebuild
}

