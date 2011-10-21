
# ----------------------------------------------------------------------
#  VisViewer - 
#
#  This class is the base class for the various visualization viewers 
#  that use the nanoserver render farm.
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

    private common _servers         ;# array of visualization server lists
    set _servers(nanovis) "localhost:2000"
    set _servers(pymol)   "localhost:2020"
    set _servers(vtkvis)  "localhost:2010"

    protected variable _sid ""        ;# socket connection to server
    private common _done            ;# Used to indicate status of send.
    private variable _buffer        ;# buffer for incoming/outgoing commands
    private variable _initialized 
    private variable _isOpen 0
    private variable _afterId -1

    # Number of milliseconds to wait before idle timeout.
    # If greater than 0, automatically disconnect from the visualization
    # server when idle timeout is reached.
    private variable _idleTimeout 43200000; # 12 hours
    #private variable _idleTimeout 5000;    # 5 seconds
    #private variable _idleTimeout 0;       # No timeout

    protected variable _dispatcher ""   ;# dispatcher for !events
    protected variable _hosts ""    ;# list of hosts for server
    protected variable _parser ""   ;# interpreter for incoming commands
    protected variable _image
    protected variable _hostname

    constructor { hostlist args } {
        # defined below
    }
    destructor {
        # defined below
    }
    # Used internally only.
    private method Shuffle { hostlist }
    private method ReceiveHelper {}
    private method ServerDown {}
    private method SendHelper {}
    private method SendHelper.old {}
    private method CheckConnection {}
    private method SplashScreen { state } 

    protected method SendEcho { channel {data ""} }
    protected method ReceiveEcho { channel {data ""} }
    protected method Connect { hostlist }
    protected method Disconnect {}
    protected method IsConnected {}
    protected method SendBytes { bytes }
    protected method ReceiveBytes { nbytes }
    protected method Flush {}
    protected method Color2RGB { color }
    protected method Euler2XYZ { theta phi psi }

    private proc CheckNameList { namelist }  {
        foreach host $namelist {
            set pattern {^[a-zA-Z0-9\.]+:[0-9]}
            if { ![regexp $pattern $host match] } {
                error "bad visualization server address \"$host\": should be host:port,host:port,..."
            }
        }
    }
    public proc GetServerList { tag } {
        return $_servers($tag)
    }
    public proc SetServerList { tag namelist } {
	# Convert the comma separated list into a Tcl list.  OGRE also adds
	# a trailing comma that we want to ignore.
        regsub -all "," $namelist " " namelist
        CheckNameList $namelist
        set _servers($tag) $namelist
    }
    public proc SetPymolServerList { namelist } {
        SetServerList "pymol" $namelist
    }
    public proc SetNanovisServerList { namelist } {
        SetServerList "nanovis" $namelist
    }
    public proc SetVtkServerList { namelist } {
        SetServerList "vtk" $namelist
    }
}

itk::usual Panedwindow {
    keep -background -cursor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VisViewer::constructor { hostlist args } {

    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this ServerDown]; list"
    $_dispatcher register !timeout
    $_dispatcher dispatch $this !timeout "[itcl::code $this Disconnect]; list"

    CheckNameList $hostlist
    set _hostlist $hostlist
    set _buffer(in) ""
    set _buffer(out) ""
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

    itk_component add main {
        Rappture::SidebarFrame $itk_interior.main
    }
    pack $itk_component(main) -expand yes -fill both
    set f [$itk_component(main) component frame]

    itk_component add plotarea {
        frame $f.plotarea -highlightthickness 0 -background black
    } {
        ignore -background
    }
    pack $itk_component(plotarea) -fill both -expand yes
    set _image(plot) [image create photo]
    eval itk_initialize $args
}

#
# destructor --
#
itcl::body Rappture::VisViewer::destructor {} {
    $_dispatcher cancel !timeout
    interp delete $_parser
    array unset _done $this
}

#
# Shuffle --
#
#   Shuffle the list of server hosts.
#
itcl::body Rappture::VisViewer::Shuffle { hosts } {
    set randomHosts {}
    set ticks [clock clicks]
    expr {srand($ticks)}
    for { set i [llength $hosts] } { $i > 0 } { incr i -1 } {
        set index [expr {round(rand()*$i - 0.5)}]
        if { $index == $i } {
            set index [expr $i - 1]
        }
        lappend randomHosts [lindex $hosts $index]
        set hosts [lreplace $hosts $index $index]
    }
    return $randomHosts
}

#
# ServerDown --
#
#    Used internally to let the user know when the connection to the
#    visualization server has been lost.  Puts up a tip encouraging the
#    user to press any control to reconnect.
#
itcl::body Rappture::VisViewer::ServerDown {} {
    if { [info exists itk_component(plotarea)] } {
        set x [expr {[winfo rootx $itk_component(plotarea)]+10}]
        set y [expr {[winfo rooty $itk_component(plotarea)]+10}]
    } else {
        set x 0; set y 0
    }
    Rappture::Tooltip::cue @$x,$y "Lost connection to visualization server.  This happens sometimes when there are too many users and the system runs out of memory.\n\nTo reconnect, reset the view or press any other control.  Your picture should come right back up."
}

#
# Connect --
#
#    Connect to the visualization server (e.g. nanovis, pymolproxy).
#    Creates an event callback that is triggered when we are idle
#    (no I/O with the server) for some specified time. Sends the server
#    some estimate of the size of our job [soon to be deprecated].
#    If it's too busy, that server may forward us to another [this
#    was been turned off in nanoscale].
#
itcl::body Rappture::VisViewer::Connect { hostlist } {
    blt::busy hold $itk_component(hull) -cursor watch
    # Can't call update because of all the pending stuff going on
    #update
    
    # Shuffle the list of servers so as to pick random 
    set servers [Shuffle $hostlist]
    
    # Get the first server
    foreach {hostname port} [split [lindex $servers 0] :] break
    set servers [lrange $servers 1 end]
    while {1} {
        puts stderr "connecting to $hostname:$port..."
        if { [catch {socket $hostname $port} _sid] != 0 } {
	    set _sid ""
            if {[llength $servers] == 0} {
                blt::busy release $itk_component(hull)
                return 0
            }
            # Get the next server
            foreach {hostname port} [split [lindex $servers 0] :] break
            set servers [lrange $servers 1 end]
            continue
        }
	set _hostname $hostname:$port
        fconfigure $_sid -translation binary -encoding binary
	
        # Read back the server identification string.
        if { [gets $_sid data] <= 0 } {
	    error "reading from server"
	}
	puts stderr "render server is $data"
	# We're connected. Cancel any pending serverDown events and
	# release the busy window over the hull.
	$_dispatcher cancel !serverDown
	if { $_idleTimeout > 0 } {
	    $_dispatcher event -after $_idleTimeout !timeout
	}
	blt::busy release $itk_component(hull)
	fconfigure $_sid -buffering line
	fileevent $_sid readable [itcl::code $this ReceiveHelper]
	return 1
    }
    #NOTREACHED
    blt::busy release $itk_component(hull)
    return 0
}

#
# Disconnect --
#
#    Clients use this method to disconnect from the current rendering
#    server.  Cancel any pending idle timeout events.
#
itcl::body Rappture::VisViewer::Disconnect {} {
    after cancel $_afterId
    $_dispatcher cancel !timeout
    catch {close $_sid} 
    set _sid ""
    set _buffer(in) ""
}

#
# IsConnected --
#
#    Indicates if we are currently connected to a server.
#
itcl::body Rappture::VisViewer::IsConnected {} {
    if { $_sid == "" } {
	return 0
    }
    if { [eof $_sid] } {
	set _sid ""
	return 0
    }
    return 1
}

#
# CheckConection --
#
#   This routine is called whenever we're about to send/recieve data on 
#   the socket connection to the visualization server.  If we're connected, 
#   then reset the timeout event.  Otherwise try to reconnect to the 
#   visualization server.
#
itcl::body Rappture::VisViewer::CheckConnection {} {
    $_dispatcher cancel !timeout
    if { $_idleTimeout > 0 } {
	$_dispatcher event -after $_idleTimeout !timeout
    }
    if { [IsConnected] } {
	return 1
    }
    if { $_sid != "" } {
	fileevent $_sid writable ""
    }
    # If we aren't connected, assume it's because the connection to the
    # visualization server broke. Try to open a connection and trigger a
    # rebuild.
    $_dispatcher cancel !serverDown
    set x [expr {[winfo rootx $itk_component(plotarea)]+10}]
    set y [expr {[winfo rooty $itk_component(plotarea)]+10}]
    Rappture::Tooltip::cue @$x,$y "Connecting..."
    set code [catch { Connect } ok]
    if { $code == 0 && $ok} {
        $_dispatcher event -idle !rebuild
        Rappture::Tooltip::cue hide
    } else {
        Rappture::Tooltip::cue @$x,$y "Can't connect to visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
        return 0
    }
    return 1
}

#
# Flush --
#
#    Flushes the socket.
#
itcl::body Rappture::VisViewer::Flush {} {
    if { [CheckConnection] } {
        flush $_sid
    }
}


#
# SendHelper --
#
#   Helper routine called from a file event to send data when the
#   connection is writable (i.e. not blocked).  Sets a magic
#   variable _done($this) when we're done.
#
itcl::body Rappture::VisViewer::SendHelper {} {
    if { ![CheckConnection] } {
        return 0
    }
    puts -nonewline $_sid $_buffer(out)
    flush $_sid 
    set _done($this) 1;			# Success
}

#
# SendHelper.old --
#
#   Helper routine called from a file event to send data when the
#   connection is writable (i.e. not blocked).  Sends data in chunks 
#   of 8k (or less).  Sets magic variable _done($this) to indicate
#   that we're either finished (success) or could not send bytes to
#   the server (failure).
#
itcl::body Rappture::VisViewer::SendHelper.old {} {
    if { ![CheckConnection] } {
        return 0
    }
    set bytesLeft [string length $_buffer(out)]
    if { $bytesLeft > 0} {
        set chunk [string range $_buffer(out) 0 8095]
        set _buffer(out)  [string range $_buffer(out) 8096 end]
        incr bytesLeft -8096
        set code [catch {
            if { $bytesLeft > 0 } {
                puts -nonewline $_sid $chunk
            } else {
                puts $_sid $chunk
            }
        } err]
        if { $code != 0 } {
            puts stderr "error sending data to $_sid: $err"
            Disconnect
            set _done($this) 0;     # Failure
        }
    } else {
        set _done($this) 1;     # Success
    }
}

#
# SendBytes --
#
#   Send a a string to the visualization server.
#
itcl::body Rappture::VisViewer::SendBytes { bytes } {
    SendEcho >>line $bytes
    if { ![CheckConnection] } {
        return 0
    }
    # Even though the data is sent in only 1 "puts", we need to verify that
    # the server is ready first.  Wait for the socket to become writable
    # before sending anything.
    set _done($this) 1
    set _buffer(out) $bytes
    fileevent $_sid writable [itcl::code $this SendHelper]
    tkwait variable ::Rappture::VisViewer::_done($this)
    set _buffer(out) ""
    if { [IsConnected] } {
        # The connection may have closed while we were writing to the server.
        # This can happen if what we sent the server caused it to barf.
        fileevent $_sid writable ""
        flush $_sid
    }
    after cancel $_afterId 
    set _afterId [after 500 [itcl::code $this SplashScreen on]]
    if 0 {
    if { ![CheckConnection] } {
        puts stderr "connection is now down"
        return 0
    }
    }
    return $_done($this)
}

#
# ReceiveBytes --
#
#    Read some number of bytes from the visualization server. 
#
itcl::body Rappture::VisViewer::ReceiveBytes { size } {
    SplashScreen off
    if { ![CheckConnection] } {
        return 0
    }
    set bytes [read $_sid $size]
    ReceiveEcho <<line "<read $size bytes"
    return $bytes
}

#
# ReceiveHelper --
#
#   Helper routine called from a file event when the connection is 
#   readable (i.e. a command response has been sent by the rendering 
#   server.  Reads the incoming command and executes it in a safe 
#   interpreter to handle the action.
#
#       Note: This routine currently only handles command responses from
#         the visualization server.  It doesn't handle non-blocking
#         reads from the visualization server.
#
#       nv>image -bytes 100000      yes
#       ...following 100000 bytes...    no
#
#   Note: All commands from the render server are on one line.
#         This is because the render server can send anything
#         as an error message (restricted again to one line).
#
itcl::body Rappture::VisViewer::ReceiveHelper {} {
    if { ![CheckConnection] } {
        return 0
    }
    set n [gets $_sid line]

    if { $n < 0 } {
        Disconnect
        return 0
    }
    set line [string trim $line]
    if { $line == "" } {
        return
    }
    if { [string compare -length 3 $line "nv>"] == 0 } {
        ReceiveEcho <<line $line
        append _buffer(in) [string range $line 3 end]
        append _buffer(in) "\n"
        if {[info complete $_buffer(in)]} {
            set request $_buffer(in)
            set _buffer(in) ""
            if { [catch {$_parser eval $request} err]  != 0 } {
                global errorInfo
                puts stderr "err=$err errorInfo=$errorInfo"
            }
        }
    } elseif { [string compare -length 21 $line "NanoVis Server Error:"] == 0 ||
               [string compare -length 20 $line "VtkVis Server Error:"] == 0} {
        # this shows errors coming back from the engine
        ReceiveEcho <<error $line
        puts stderr "Render Server Error: $line\n"
    } else {
        # this shows errors coming back from the engine
        ReceiveEcho <<error $line
        puts stderr "Garbled message: $line\n"
    }
}


#
# Color2RGB --
#
#   Converts a color name to a list of r,g,b values needed for the
#   engine.  Each r/g/b component is scaled in the # range 0-1.
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
#   Converts euler angles for the camera placement the to angles of
#   rotation about the x/y/z axes, used by the engine.  Returns a list:
#   {xangle, yangle, zangle}.
#
itcl::body Rappture::VisViewer::Euler2XYZ {theta phi psi} {
    set xangle [expr {$theta-90.0}]
    set yangle [expr {180-$phi}]
    set zangle $psi
    return [list $xangle $yangle $zangle]
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
    #puts stderr ">>($data)"
    if {[string length $itk_option(-sendcommand)] > 0} {
        uplevel #0 $itk_option(-sendcommand) [list $channel $data]
    }
}

# 
# ReceiveEcho --
#
#     Echoes received data to clients interested in this widget.  If the
#     -receivecommand option is set, then it is invoked in the global
#     scope with the <channel> and <data> values as arguments.  Otherwise,
#     this does nothing.
#
itcl::body Rappture::VisViewer::ReceiveEcho {channel {data ""}} {
    #puts stderr "<<line $data"
    if {[string length $itk_option(-receivecommand)] > 0} {
        uplevel #0 $itk_option(-receivecommand) [list $channel $data]
    }
}

itcl::body Rappture::VisViewer::SplashScreen { state } {
    after cancel $_afterId
    set _afterId -1
    if { $state } {
	if { [winfo exists $itk_component(plotarea).view.splash] } {
	    return
	}
	label $itk_component(plotarea).view.splash -text "Please wait"
	pack $itk_component(plotarea).view.splash -expand yes -anchor c
    } else {
	if { ![winfo exists $itk_component(plotarea).view.splash] } {
	    return
	}
	destroy $itk_component(plotarea).view.splash
    }
}

