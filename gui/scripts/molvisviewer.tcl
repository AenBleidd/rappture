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
option add *MolvisViewer.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::MolvisViewer {
    inherit itk::Widget
    itk_option define -device device Device ""

    constructor {hostlist args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}

    public method emblems {option}
    public method representation {option {model "all"} }

    public method connect {{hostlist ""}}
    public method disconnect {}
    public method isconnected {}
    public method download {option args}
    protected method _rock {option}
    protected method _sendit {args}
    protected method _send {args}
    protected method _receive { {sid ""} }
    protected method _update { args }
    protected method _rebuild { }
    protected method _zoom {option}
	protected method _configure {w h}
	protected method _unmap {}
	protected method _map {}
    protected method _vmouse2 {option b m x y}
    protected method _vmouse  {option b m x y}
    protected method _serverDown {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _sid ""       ;# socket connection to nanovis server
    private variable _image        ;# image displayed in plotting area

    private variable _inrebuild 0

    private variable _mevent       ;# info used for mouse event operations
    private variable _rocker       ;# info used for rock operations
    private variable _dlist ""    ;# list of dataobj objects
    private variable _dataobjs     ;# data objects on server
    private variable _dobj2transparency  ;# maps dataobj => transparency
    private variable _dobj2raise  ;# maps dataobj => raise flag 0/1
    private variable _dobj2ghost

    private variable _model
    private variable _mlist

    private variable _imagecache
    private variable _state
    private variable _labels  "default"
    private variable _cacheid ""
    private variable _hostlist ""
    private variable _mrepresentation "spheres"
    private variable _cacheimage ""
	private variable _busy 0
	private variable _mapped 0
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
    set _rocker(client) 0
    set _rocker(server) 0
    set _rocker(on) 0
    set _state(server) 1
	set _state(client) 1

    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this _serverDown]; list"

    #
    # Set up the widgets in the main body
    #
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

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
            -relief "raised" -bitmap [Rappture::icon atoms]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(labels) -padx 4 -pady 4 -ipadx 1 -ipady 1
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
    pack $itk_component(rock) -padx 4 -pady 4 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(rock) "Rock model +/- 10 degrees"

    itk_component add show_lines {
            label $itk_component(controls).show_lines \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -text "/" \
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_lines) -padx 4 -pady 4 
    bind $itk_component(show_lines) <ButtonPress> \
        [itcl::code $this representation lines all]

	itk_component add show_spheres {
            label $itk_component(controls).show_spheres \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "sunken" -text "O" \
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_spheres) -padx 4 -pady 4
    bind $itk_component(show_spheres) <ButtonPress> \
        [itcl::code $this representation spheres all]

    itk_component add show_ball_and_stick {
            label $itk_component(controls).show_ball_and_stick \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -text "%" \
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(show_ball_and_stick) -padx 4 -pady 4
    bind $itk_component(show_ball_and_stick) <ButtonPress> \
        [itcl::code $this representation ball_and_stick all]
    
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

	connect $hostlist

    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _configure %w %h]
    bind $itk_component(3dview) <Unmap> \
        [itcl::code $this _unmap]
    bind $itk_component(3dview) <Map> \
        [itcl::code $this _map]

    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"
    
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::destructor {} {
    #puts stderr "MolvisViewer::destructor()"
    image delete $_image(plot)
	disconnect
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
    #puts stderr "Rappture::MolvisViewer::connect($hostlist)"

    if { "" != $hostlist } { set _hostlist $hostlist }

    set hostlist $_hostlist
    $_image(plot) blank

    disconnect

    if {"" == $hostlist} {
        return 0
    }

    blt::busy hold $itk_component(hull) -cursor watch
    
    update idletasks

    # HACK ALERT! punt on this for now
    set memorySize 10000

    #
    # Connect to the hubvis server.  
    # If it's too busy, that server may
    # forward us to another.
    #

    set hosts [split $hostlist ,]

    foreach {hostname port} [split [lindex $hosts 0] :] break

    set hosts [lrange $hosts 1 end]
	set result 0

    while {1} {
        if {[catch {socket $hostname $port} sid]} {
            if {[llength $hosts] == 0} {
                break;
            }
            foreach {hostname port} [split [lindex $hosts 0] :] break
            set hosts [lrange $hosts 1 end]
            continue
        }
        fconfigure $sid -translation binary -encoding binary -buffering line -buffersize 1000
        #puts $sid "pymol"
        puts -nonewline $sid [binary format I $memorySize]
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
            set _sid $sid
			set _rocker(server) 0
			set _cacheid 0

            fileevent $_sid readable [itcl::code $this _receive $_sid]

            _send raw -defer set auto_color,0
            _send raw -defer set auto_show_lines,0

            set result 1
			break
        }
    }

    blt::busy release $itk_component(hull)
    
    return $result
}

# ----------------------------------------------------------------------
# USAGE: disconnect
#
# Clients use this method to disconnect from the current rendering
# server.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::disconnect {} {
    #puts stderr "MolvisViewer::disconnect()"

	catch { fileevent $_sid readable {} }
    catch { after cancel $_rocker(afterid) }
	catch { after cancel $_mevent(afterid) }
    catch { close $_sid }
    catch { unset _dataobjs }
	catch { unset _model }
	catch {	unset _mlist }
    catch { unset _imagecache }

    set _sid ""
	set _state(server) 1
	set _state(client) 1
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
itcl::body Rappture::MolvisViewer::_sendit {args} {
    #puts stderr "Rappture::MolvisViewer::_sendit($args)"

    if { $_sid != "" } {
        if { ![catch { puts $_sid $args }] } {
		    flush $_sid
			return 0
		} else {
            catch { close $_sid }
            set _sid ""
		}
	}

    $_dispatcher event -after 1 !rebuild

	return 1
}

itcl::body Rappture::MolvisViewer::_send {args} {
    #puts stderr "Rappture::MolvisViewer::_send($args)"

    if { $_state(server) != $_state(client) } {
        if { [_sendit "frame -defer $_state(client)"] == 0 } {
            set _state(server) $_state(client)
        }
	}

    if { $_rocker(server) != $_rocker(client) } {
        if { [_sendit "rock -defer $_rocker(client)"]  == 0 } {
            set _rocker(server) $_rocker(client)
	    }
	}

    eval _sendit $args
}

# ----------------------------------------------------------------------
# USAGE: _receive
#
# Invoked automatically whenever a command is received from the
# rendering server.  Reads the incoming command and executes it in
# a safe interpreter to handle the action.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_receive { {sid ""} } {
    #puts stderr "Rappture::MolvisViewer::_receive($sid)"

    if { $sid == "" } {
	    return
	}

	fileevent $sid readable {} 

    if { $sid != $_sid } {
	    return
	}

    fconfigure $_sid -buffering line -blocking 0
        
    if {[gets $_sid line] < 0} {

        if { ![fblocked $_sid] } { 
		    catch { close $_sid }
			set _sid ""
            $_dispatcher event -after 750 !serverDown
		}

    }  elseif {[regexp {^\s*nv>\s*image\s+(\d+)\s*(\d+)\s*,\s*(\d+)\s*,\s*(-{0,1}\d+)} $line whole match cacheid frame rock]} {

        set tag "$frame,$rock"
    		
        if { $cacheid != $_cacheid } {
            catch { unset _imagecache }
            set _cacheid $cacheid
        }

        fconfigure $_sid -buffering none -blocking 1
        set _imagecache($tag) [read $_sid $match]
	    #puts stderr "CACHED: $tag,$cacheid"
        $_image(plot) put $_imagecache($tag)
        set _image(id) $tag

		if { $_busy } {
            $itk_component(3dview) configure -cursor ""
		    set _busy 0
		}

    } else {
        # this shows errors coming back from the engine
        puts $line
    }
    
	if { $_sid != "" } {
        fileevent $_sid readable [itcl::code $this _receive $_sid] 
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
    #puts stderr "Rappture::MolvisViewer::_rebuild()"

    if { $_inrebuild } { 
		# don't allow overlapping rebuild calls
	    return
	}

	set _inrebuild 1

    if {"" == $_sid} {
        $_dispatcher cancel !serverDown

        set x [expr {[winfo rootx $itk_component(area)]+10}]
        set y [expr {[winfo rooty $itk_component(area)]+10}]

        Rappture::Tooltip::cue @$x,$y "Connecting..."
        update idletasks

        if {[catch {connect} ok] == 0 && $ok} {
            set w [winfo width $itk_component(3dview)]
            set h [winfo height $itk_component(3dview)]
            _send screen -defer $w $h
            Rappture::Tooltip::cue hide
        } else {
            Rappture::Tooltip::cue @$x,$y "Can't connect to visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
		    set _inrebuild 0
	        set _busy 1
            return
		}
    }

	set changed 0
	set _busy 1

    $itk_component(3dview) configure -cursor watch

	# refresh GUI (primarily to make pending cursor changes visible)
    update idletasks 

    set dlist [get]

    foreach dev $dlist {
        set model [$dev get components.molecule.model]
        set state [$dev get components.molecule.state]
        
        if {"" == $model } { 
            set model "molecule"
            scan $dev "::libraryObj%d" suffix
            set model $model$suffix	
        }

        if {"" == $state} { set state $_state(server) }

		if { ![info exists _mlist($model)] } { # new, turn on
		    set _mlist($model) 2
		} elseif { $_mlist($model) == 1 } { # on, leave on
		    set _mlist($model) 3 
		} elseif { $_mlist($model) == 0 } { # off, turn on
		    set _mlist($model) 2
		}

        if { ![info exists _dataobjs($model-$state)] } {
    		set data1      ""
    		set serial   0

            foreach _atom [$dev children -type atom components.molecule] {
                set symbol [$dev get components.molecule.$_atom.symbol]
                set xyz [$dev get components.molecule.$_atom.xyz]
                regsub {,} $xyz {} xyz
                scan $xyz "%f %f %f" x y z
    			set recname  "ATOM  "
    			set altLoc   ""
    			set resName  ""
    			set chainID  ""
    			set Seqno    ""
    			set occupancy  1
    			set tempFactor 0
    			set recID      ""
    			set segID      ""
    			set element    ""
    			set charge     ""
                set atom $symbol
                set line [format "%6s%5d %4s%1s%3s %1s%5s   %8.3f%8.3f%8.3f%6.2f%6.2f%8s\n" $recname $serial $atom $altLoc $resName $chainID $Seqno $x $y $z $occupancy $tempFactor $recID]
                append data1 $line
                incr serial
            }

            set data2 [$dev get components.molecule.pdb]

            if {"" != $data1} {
                eval _send loadpdb -defer \"$data1\" $model $state
                set _dataobjs($model-$state)  1
                #puts stderr "loaded model $model into state $state"
            }
            
            if {"" != $data2} {
                eval _send loadpdb -defer \"$data2\" $model $state
                set _dataobjs($model-$state)  1
                #puts stderr "loaded model $model into state $state"
            }
        }

		if { ![info exists _model($model-transparency)] } {
			set _model($model-transparency) "undefined"
		}

		if { ![info exists _model($model-representation)] } {
			set _model($model-representation) "undefined"
			set _model($model-newrepresentation) $_mrepresentation
		}


		if { $_model($model-transparency) != $_dobj2transparency($dev) } {
			set  _model($model-newtransparency) $_dobj2transparency($dev)
		} 
    }

    # enable/disable models as required (0=off->off, 1=on->off, 2=off->on, 3=on->on)

    foreach obj [array names _mlist] {
        if { $_mlist($obj) == 1 } {
            _send disable -defer $obj
			set _mlist($obj) 0
	        set changed 1
		} elseif { $_mlist($obj) == 2 } {
			set _mlist($obj) 1
			_send enable -defer $obj
		    if { $_labels } {
				_send label -defer on
			} else {
				_send label -defer off
			}
	        set changed 1
		} elseif { $_mlist($obj) == 3 } {
		    set _mlist($obj) 1
		}


		if { $_mlist($obj) == 1 } {
			if {  [info exists _model($obj-newtransparency)] || [info exists _model($obj-newrepresentation)] } {
				if { ![info exists _model($obj-newrepresentation)] } {
					set _model($obj-newrepresentation) $_model($obj-representation)
				}
				if { ![info exists _model($obj-newtransparency)] } {
					set _model($obj-newtransparency) $_model($obj-transparency)
				}
				_send $_model($obj-newrepresentation) -defer -model $obj -$_model($obj-newtransparency)
				set changed 1
			    set _model($obj-transparency) $_model($obj-newtransparency)
			    set _model($obj-representation) $_model($obj-newrepresentation)
			    catch {
				    unset _model($obj-newtransparency)
			        unset _model($obj-newrepresentation)
				}
			}
		}

	}

	if { $changed } {
        catch { unset _imagecache }
	}

    if { $dlist == "" } {
		set _state(server) 1
		set _state(client) 1
		_send frame -push 1
	} elseif { ![info exists _imagecache($state,$_rocker(client))] } {
		set _state(server) $state
		set _state(client) $state
        _send frame -push $state
    } else {
		set _state(client) $state
		_update
	}

	set _inrebuild 0

	if { $_sid == "" } {
	    # connection failed during rebuild, don't attempt to reconnect/rebuild
		# until user initiates some action

		disconnect
        $_dispatcher cancel !rebuild
        $_dispatcher event -after 750 !serverDown
	}
}

itcl::body Rappture::MolvisViewer::_unmap { } {
    #puts stderr "Rappture::MolvisViewer::_unmap()"

    #pause rocking loop while unmapped (saves CPU time)
	_rock pause

	# blank image, mark current image dirty
	# this will force reload from cache, or remain blank if cache is cleared
	# this prevents old image from briefly appearing when a new result is added
	# by result viewer

    set _mapped 0
    $_image(plot) blank
	set _image(id) ""
}

itcl::body Rappture::MolvisViewer::_map { } {
    #puts stderr "Rappture::MolvisViewer::_map()"

    set _mapped 1

	# resume rocking loop if it was on
	_rock unpause

	# rebuild image if modified, or redisplay cached image if not
    $_dispatcher event -idle !rebuild
}

itcl::body Rappture::MolvisViewer::_configure { w h } {
    #puts stderr "Rappture::MolvisViewer::_configure($w $h)"

    $_image(plot) configure -width $w -height $h
    
	# immediately invalidate cache, defer update until mapped
	
	catch { unset _imagecache } 

    if { $_mapped } {
        _send screen $w $h
	} else {
        _send screen -defer $w $h
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
            _send zoom 10
        }
        out {
            _send zoom -10
        }
        reset {
            _send reset
        }
    }
}

itcl::body Rappture::MolvisViewer::_update { args } {
    #puts stderr "Rappture::MolvisViewer::_update($args)"

    if { $_image(id) != "$_state(client),$_rocker(client)" } {
        if { [info exists _imagecache($_state(client),$_rocker(client))] } {
	        #puts stderr "DISPLAYING CACHED IMAGE"
            $_image(plot) put $_imagecache($_state(client),$_rocker(client))
	        set _image(id) "$_state(client),$_rocker(client)"
        }
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
    #puts "MolvisViewer::_rock($option,$_rocker(client))"
    
    # cancel any pending rocks
    if { [info exists _rocker(afterid)] } { 
        after cancel $_rocker(afterid)
        unset _rocker(afterid)
    }

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
    } elseif { $option == "step"} {

        if { $_rocker(client) >= 10 } {
            set _rocker(dir) -1
        } elseif { $_rocker(client) <= -10 } {
		    set _rocker(dir) 1
        }

	    set _rocker(client) [expr $_rocker(client) + $_rocker(dir)]
   
        if { ![info exists _imagecache($_state(server),$_rocker(client))] } {
	        set _rocker(server) $_rocker(client)
            _send rock $_rocker(client)
        }
	    
	    _update
    }

	if { $_rocker(on) && $option != "pause" } {
		 set _rocker(afterid) [after 200 [itcl::code $this _rock step]]
	}
}

itcl::body Rappture::MolvisViewer::_vmouse2 {option b m x y} {
    set now [clock clicks -milliseconds]
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
        set diff 0

        catch { set diff [expr $now - $_mevent(time)] } 

        if {$diff < 75} { # 75ms between motion updates
            return
        }
    }

     _send vmouse $vButton $vModifier $vState $x $y

    set _mevent(time) $now
}

itcl::body Rappture::MolvisViewer::_vmouse {option b m x y} {
    #puts stderr "Rappture::MolvisViewer::_vmouse($option,$b,$m,$x,$y)"

    set now  [clock clicks -milliseconds]

    # cancel any pending delayed dragging events
    if { [info exists _mevent(afterid)] } { 
        after cancel $_mevent(afterid)
        unset _mevent(afterid)
    }

	if { ![info exists _mevent(x)] } {
		set option "click"
	}

    if { $option == "click" } { 
        $itk_component(3dview) configure -cursor fleur
    }

    if { $option == "drag" || $option == "release" } {
	    set diff 0
        catch { set diff [expr $now - $_mevent(time) ] }

        if {$diff < 75 && $option == "drag" } { # 75ms between motion updates
            set _mevent(afterid) [after [expr 75 - $diff] [itcl::code $this _vmouse drag $b $m $x $y]]
            return
        }

        set w [winfo width $itk_component(3dview)]
        set h [winfo height $itk_component(3dview)]

        if {$w <= 0 || $h <= 0} {
            return
        }

        set x1 [expr $w / 3]
        set x2 [expr $x1 * 2]
        set y1 [expr $h / 3]
        set y2 [expr $y1 * 2]
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

        _send rotate $mx $my $mz

    }

    set _mevent(x) $x
    set _mevent(y) $y
    set _mevent(time) $now

    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
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

	if { $_busy } {
        $itk_component(3dview) configure -cursor ""
        set _busy 0
	}

    Rappture::Tooltip::cue @$x,$y "Lost connection to visualization server.  This happens sometimes when there are too many users and the system runs out of memory.\n\nTo reconnect, reset the view or press any other control.  Your picture should come right back up."
}

# ----------------------------------------------------------------------
# USAGE: representation spheres
# USAGE: representation ball_and_stick
# USAGE: representation lines
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::representation {option {model "all"} } {
    #puts stderr "Rappture::MolvisViewer::representation($option,$model)"

    if { $option == $_mrepresentation } { return }

    switch -- $option {
        spheres {
             $itk_component(show_spheres) configure -relief sunken
             $itk_component(show_lines) configure -relief raised
             $itk_component(show_ball_and_stick) configure -relief raised
        }
        ball_and_stick {
             $itk_component(show_spheres) configure -relief raised
             $itk_component(show_lines) configure -relief raised
             $itk_component(show_ball_and_stick) configure -relief sunken
        }
        lines {
            $itk_component(show_spheres) configure -relief raised
            $itk_component(show_lines) configure -relief sunken
            $itk_component(show_ball_and_stick) configure -relief raised
        }
		default {
			return 
		}
	}

    set _mrepresentation $option

    if { $model == "all" } {
        set models [array names _mlist]
	} else {
	    set models $model
	}

    foreach obj $models {
		if { [info exists _model($obj-representation)] } {
			if { $_model($obj-representation) != $option } {
		        set _model($obj-newrepresentation) $option
			} else {
				catch { unset _model($obj-newrepresentation) }
			}
		}
	}

    $_dispatcher event -idle !rebuild
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
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise. Only
# -brightness and -raise do anything.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::add { dataobj {settings ""}} {
    #puts stderr "Rappture::MolvisViewer::add($dataobj)"

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
		    error "bad settings \"$opt\": should be [join [lsort [array names params]] {, }]"
		}
		set params($opt) $val
	}
 
	set pos [lsearch -exact $dataobj $_dlist]

	if {$pos < 0} {
        if {![Rappture::library isvalid $dataobj]} {
            error "bad value \"$dataobj\": should be Rappture::library object"
        }
	
	    if { $_labels == "default" } {
            set emblem [$dataobj get components.molecule.about.emblems]

            if {$emblem == "" || ![string is boolean $emblem] || !$emblem} {
                emblems off
            } else {
                emblems on
            }
        }

	    lappend _dlist $dataobj
		if { $params(-brightness) >= 0.5 } {
			set _dobj2transparency($dataobj) "ghost"
		} else {
			set _dobj2transparency($dataobj) "normal"
		}
		set _dobj2raise($dataobj) $params(-raise)

        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::get {} {
    #puts stderr "Rappture::MolvisViewer::get()"

    # put the dataobj list in order according to -raise options
	set dlist $_dlist
	foreach obj $dlist {
	    if {[info exists _dobj2raise($obj)] && $_dobj2raise($obj)} {
		    set i [lsearch -exact $dlist $obj]
			if {$i >= 0} {
			    set dlist [lreplace $dlist $i $i]
				lappend dlist $obj
			}
		}
	}
	return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj> <dataobj> ...?
#
# Clients use this to delete a dataobj from the plot. If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::delete {args} {
    #puts stderr "Rappture::MolvisViewer::delete($args)"

    if {[llength $args] == 0} {
	    set args $_dlist
	}

	# delete all specified dataobjs
	set changed 0
	foreach dataobj $args {
	    set pos [lsearch -exact $_dlist $dataobj]
		if {$pos >= 0} {
		    set _dlist [lreplace $_dlist $pos $pos]
			catch {unset _dobj2transparency($dataobj)}
			catch {unset _dobj2color($dataobj)}
			catch {unset _dobj2width($dataobj)}
			catch {unset _dobj2dashes($dataobj)}
			catch {unset _dobj2raise($dataobj)}
            set changed 1
		}
	}

	# if anything changed, then rebuild the plot
	if {$changed} {
        $_dispatcher event -idle !rebuild
	}
}

# ----------------------------------------------------------------------
# OPTION: -device
# ----------------------------------------------------------------------
itcl::configbody Rappture::MolvisViewer::device {
    #puts stderr "Rappture::MolvisViewer::device($itk_option(-device))"

    if {$itk_option(-device) != "" } {

        if {![Rappture::library isvalid $itk_option(-device)]} {
            error "bad value \"$itk_option(-device)\": should be Rappture::library object"
        }
		$this delete
		$this add $itk_option(-device)
	} else {
		$this delete
	}

    $_dispatcher event -idle !rebuild
}

