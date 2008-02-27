
# ----------------------------------------------------------------------
#  VisViewer - 
#
#  This class is the base class for the various visualization viewers 
#  that use the nanoserver render farm.
#
#  My plan is to make this class the parent of the NanovisViewer and
#  HeightmapViewer classes. I'm going to move out all the server-related 
#  methods like _send*, _receive*, connect, disconnect, etc. from those
#  classes into this one.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

itcl::class ::Rappture::VisViewer {
    inherit itk::Widget

    itk_option define -sendcommand sendCommand SendCommand ""
    itk_option define -receivecommand receiveCommand ReceiveCommand ""

    private common _servers          ;# array of visualization server lists
    set _servers(nanovis) "128.210.189.216:2000" 
    set _servers(pymol) barney

    protected variable _dispatcher "" ;# dispatcher for !events
    protected variable _hosts ""     ;# list of hosts for server
    protected variable _sid ""       ;# socket connection to server
    protected variable _parser ""    ;# interpreter for incoming commands
    protected variable _inbuf        ;# buffer for incoming/outgoing commands
    protected variable _busy 0
    protected variable _image

    constructor { hostlist args } { 
	# defined below 
    }
    destructor { 
	# defined below 
    }
    # Used internally only.
    private method _CheckNameList {namelist} 
    private method _Shuffle { hostlist } 
    private method _ReceiveCmd {} 
    private method _ServerDown {}

    protected method SendEcho {channel {data ""}}
    protected method ReceiveEcho {channel {data ""}}
    protected method Connect { hostlist }
    protected method Disconnect {}
    protected method IsConnected {}
    protected method Send { cmdstr { switch "" } }
    protected method Receive { nbytes }
    protected method Flush {}
    protected method Color2RGB {color}
    protected method Euler2XYZ {theta phi psi}

    public proc GetServerList { tag } {
	return $_servers($tag)
    }
    public proc SetServerList {tag namelist} {
	_CheckNameList $namelist
	set _servers($tag) $namelist
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VisViewer::constructor { hostlist args } {

    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this _ServerDown]; list"

    _CheckNameList $hostlist
    set _hostlist $hostlist

    set _inbuf ""

    #
    # Create a parser to handle incoming requests
    #
    set _parser [interp create -safe]
    foreach cmd [$_parser eval {info commands}] {
        $_parser hide $cmd
    }

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
            -highlightthickness 0 -width 1 -height 1
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(3dview) -expand yes -fill both

    eval itk_initialize $args
}

# 
# destructor --
# 
itcl::body Rappture::VisViewer::destructor {} {
    interp delete $_parser
}

# 
# _Shuffle --  
#
#	Shuffle the list of server hosts. 
#
itcl::body Rappture::VisViewer::_Shuffle { hostlist } {
    set hosts [split $hostlist ,]
    set random_hosts {}
    for { set i [llength $hosts] } { $i > 0 } { incr i -1 } {
	set index [expr {round(rand()*$i - 0.5)}]
	if { $index == $i } {
	    set index [expr $i - 1]
	}
	lappend random_hosts [lindex $hosts $index]
	set hosts [lreplace $hosts $index $index]
    }
    return $random_hosts
}

#
# _CheckNameList --
# 
#    Used within the class to check if provide list of hostname:port are
#    correct.  Return an error if the list is invalid.
#
itcl::body Rappture::VisViewer::_CheckNameList { namelist } {
    set pattern {^[a-zA-Z0-9\.]+:[0-9]+(,[a-zA-Z0-9\.]+:[0-9]+)*$}
    if { ![regexp $pattern $namelist match] } {
	error "bad visualization server address \"$namelist\": should be host:port,host:port,..."
    }
}

#
# _ServerDown --
#
#    Used internally to let the user know when the connection to the
#    visualization server has been lost.  Puts up a tip encouraging the
#    user to press any control to reconnect.
#
itcl::body Rappture::VisViewer::_ServerDown {} {
    if { [info exists itk_component(area)] } {
	set x [expr {[winfo rootx $itk_component(area)]+10}]
	set y [expr {[winfo rooty $itk_component(area)]+10}]
    } else {
	set x 0; set y 0
    }
    Rappture::Tooltip::cue @$x,$y "Lost connection to visualization server.  This happens sometimes when there are too many users and the system runs out of memory.\n\nTo reconnect, reset the view or press any other control.  Your picture should come right back up."
}

#
# Connect --
# 
#    Connect to the visualization server (e.g. nanovis, pymolproxy).  Send
#    the server some estimate of the size of our job.  If it's too busy, that
#    server may forward us to another.  
#
itcl::body Rappture::VisViewer::Connect { hostlist } {
    blt::busy hold $itk_component(hull) -cursor watch
    update idletasks

    # Shuffle the list of servers so as to pick random 
    set servers [_Shuffle $hostlist]

    set memorySize 10000
    # Get the first server
    foreach {hostname port} [split [lindex $servers 0] :] break
    set servers [lrange $servers 1 end]

    while {1} {
        SendEcho <<line "connecting to $hostname:$port..."
        if { [catch {socket $hostname $port} _sid] != 0 } {
            if {[llength $servers] == 0} {
		blt::busy release $itk_component(hull)
                return 0
            }
	    # Get the next server
            foreach {hostname port} [split [lindex $servers 0] :] break
            set servers [lrange $servers 1 end]
            continue
        }
        fconfigure $_sid -translation binary -encoding binary

        # send memory requirement to the load balancer
        puts -nonewline $_sid [binary format I $memorySize]
        flush $_sid
	
        # read back a reconnection order
        set data [read $_sid 4]
        if {[binary scan $data cccc b1 b2 b3 b4] != 4} {
	    blt::busy release $itk_component(hull)
            error "couldn't read redirection request"
        }
        set addr [format "%u.%u.%u.%u" \
            [expr {$b1 & 0xff}] \
            [expr {$b2 & 0xff}] \
            [expr {$b3 & 0xff}] \
            [expr {$b4 & 0xff}]]

        if { [string equal $addr "0.0.0.0"] } {
	    # Success: Cancel any pending serverDown events and release the
	    # busy window over the hull.
	    $_dispatcher cancel !serverDown
	    blt::busy release $itk_component(hull)
	    fconfigure $_sid -buffering line 
	    fileevent $_sid readable [itcl::code $this _ReceiveCmd]
            return 1
        }
        set hostname $addr
    }
    #NOTREACHED
    blt::busy release $itk_component(hull)
    return 0
}


#
# SendEcho --
#
#     Used internally to echo sent data to clients interested in this widget.
#     If the -sendcommand option is set, then it is invoked in the global scope
#     with the <channel> and <data> values as arguments.  Otherwise, this does
#     nothing.
#
itcl::body Rappture::VisViewer::SendEcho {channel {data ""}} {
    #puts stderr ">>line $data"
    if {[string length $itk_option(-sendcommand)] > 0} {
        uplevel #0 $itk_option(-sendcommand) [list $channel $data]
    }
}

# 
# ReceiveEcho --
#
#     Echoes received data tzo clients interested in this widget.  If the
#     -receivecommand option is set, then it is # invoked in the global
#     scope with the <channel> and <data> values # as arguments.  Otherwise,
#     this does nothing.
#
itcl::body Rappture::VisViewer::ReceiveEcho {channel {data ""}} {
    #puts stderr "<<line $data"
    if {[string length $itk_option(-receivecommand)] > 0} {
        uplevel #0 $itk_option(-receivecommand) [list $channel $data]
    }
}

#
# Disconnect --
#
#    Clients use this method to disconnect from the current rendering
#    server.
#
itcl::body Rappture::VisViewer::Disconnect {} {
    if { [IsConnected] } {
        catch {close $_sid} err
        set _sid ""
	$_dispatcher event -after 750 !serverDown
    }
    set _inbuf ""			
}

#
# IsConnected --
#
#    Indicates if we are currently connected to a server.
#
itcl::body Rappture::VisViewer::IsConnected {} {
    return [expr {"" != $_sid}]
}

#
# Send --
#
#    Send a string to the visualization server.
#
itcl::body Rappture::VisViewer::Send { cmdstr { switch "" } } {
    SendEcho >>line $cmdstr
    if { $switch != "" } {
	set code [catch {puts $switch $_sid $cmdstr} err]
    } else {
	set code [catch {puts $_sid $cmdstr} err]
    }
    flush $_sid
    if { $code != 0 } {
	puts stderr "error sending data to $_sid: $err"
	Disconnect
	return 0
    }
    return 1
}


#
# Receive --
#
#    Read some number of bytes from the visualization server.
#
itcl::body Rappture::VisViewer::Receive { size } {
    if { [eof $_sid] } {
	error "unexpected eof on socket"
    }
    set bytes [read $_sid $size]
    ReceiveEcho <<line "<read $size bytes"
    return $bytes
}

#
# Flush --
#
#    Flushes the socket.
#
itcl::body Rappture::VisViewer::Flush {} {
    if { [IsConnected] } {
	flush $_sid
    }
}

# 
# _ReceiveCmd --
#
#     Invoked automatically whenever a command is received from the
#     rendering server.  Reads the incoming command and executes it in
#     a safe interpreter to handle the action.
# 
itcl::body Rappture::VisViewer::_ReceiveCmd {} {
    if { [IsConnected] } {
	if { [eof $_sid] } {
	    error "_receive: unexpected eof on socket"
	}
        if {[gets $_sid line] < 0} {
            Disconnect
        } elseif {[string equal [string range $line 0 2] "nv>"]} {
            ReceiveEcho <<line $line
            append _inbuf [string range $line 3 end]
            if {[info complete $_inbuf]} {
                set request $_inbuf
                set _inbuf ""
                $_parser eval $request
            }
        } else {
            # this shows errors coming back from the engine
            ReceiveEcho <<error $line
            puts stderr $line
        }
    }
}


#
# Color2RGB --
#
#	Converts a color name to a list of r,g,b values needed for the
#	engine.  Each r/g/b component is scaled in the # range 0-1.  
#
itcl::body Rappture::VisViewer::Color2RGB {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] break
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

#
# Euler2XYZ --
#
#	Converts euler angles for the camera placement the to angles of
#	rotation about the x/y/z axes, used by the engine.  Returns a list:
#	{xangle, yangle, zangle}.  
#
itcl::body Rappture::VisViewer::Euler2XYZ {theta phi psi} {
    set xangle [expr {$theta-90.0}]
    set yangle [expr {180-$phi}]
    set zangle $psi
    return [list $xangle $yangle $zangle]
}
