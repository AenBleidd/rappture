# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  VisViewer -
#
#  This class is the base class for the various visualization viewers
#  that use the nanoserver render farm.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

itcl::class ::Rappture::VisViewer {
    inherit itk::Widget

    itk_option define -sendcommand sendCommand SendCommand ""
    itk_option define -receivecommand receiveCommand ReceiveCommand ""

    constructor { args } {
        # defined below
    }
    destructor {
        # defined below
    }
    public proc GetServerList { type } {
        return $_servers($type)
    }
    public proc SetServerList { type namelist } {
        # Convert the comma separated list into a Tcl list.  OGRE also adds
        # a trailing comma that we want to ignore.
        regsub -all "," $namelist " " namelist
        CheckNameList $namelist
        set _servers($type) $namelist
    }
    public proc RemoveServerFromList { type server } {
        if { ![info exists _servers($type)] } {
            error "unknown server type \"$type\""
        }
        set i [lsearch $_servers($type) $server]
        if { $i < 0 } {
            return
        }
        set _servers($type) [lreplace $_servers($type) $i $i]
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

    protected method CheckConnection {}
    protected method Color2RGB { color }
    protected method ColorsToColormap { colors }
    protected method Connect { servers }
    protected method DebugOff {} {
        set _debug 0
    }
    protected method DebugOn {} {
        set _debug 1
    }
    protected method DebugTrace { args } {
        if { $_debug } {
            puts stderr "[info level -1]: $args"
        }
    }
    protected method DisableWaitDialog {}
    protected method Disconnect {}
    protected method EnableWaitDialog { timeout }
    protected method Euler2XYZ { theta phi psi }
    protected method Flush {}
    protected method GetColormapList { args }
    protected method HandleError { args }
    protected method HandleOk { args }
    protected method IsConnected {}
    protected method ReceiveBytes { nbytes }
    protected method ReceiveEcho { channel {data ""} }
    protected method SendBytes { bytes }
    protected method SendCmd { string }
    protected method SendData { bytes }
    protected method SendEcho { channel {data ""} }
    protected method StartBufferingCommands {}
    protected method StartWaiting {}
    protected method StopBufferingCommands {}
    protected method StopWaiting {}
    protected method ToggleConsole {}

    protected variable _debug 0
    protected variable _serverType "???";# Type of server.
    protected variable _sid "";         # socket connection to server
    protected variable _maxConnects 100
    protected variable _buffering 0
    protected variable _cmdSeq 0;       # Command sequence number
    protected variable _dispatcher "";  # dispatcher for !events
    protected variable _hosts "";       # list of hosts for server
    protected variable _parser "";      # interpreter for incoming commands
    protected variable _image
    protected variable _hostname
    protected variable _numConnectTries 0
    protected variable _debugConsole 0
    protected variable _reportClientInfo 1
    # Number of milliscends to wait for server reply before displaying wait
    # dialog.  If set to 0, dialog is never displayed.
    protected variable _waitTimeout 0

    private method BuildConsole {}
    private method DebugConsole {}
    private method HideConsole {}
    private method ReceiveHelper {}
    private method SendDebugCommand {}
    private method SendHelper {}
    private method ServerDown {}
    private method Shuffle { servers }
    private method TraceComm { channel {data {}} }
    private method WaitDialog { state }
    private method Waiting { option widget }

    private proc CheckNameList { namelist }  {
        foreach host $namelist {
            set pattern {^[a-zA-Z0-9\.]+:[0-9]}
            if { ![regexp $pattern $host match] } {
                error "bad visualization server address \"$host\": should be host:port,host:port,..."
            }
        }
    }

    private variable _buffer;           # buffer for incoming/outgoing commands
    private variable _outbuf;           # buffer for outgoing commands
    private variable _blockOnWrite 0;   # Should writes to socket block?
    private variable _initialized
    private variable _isOpen 0
    private variable _afterId -1
    private variable _icon 0
    private variable _trace 0;          # Protocol tracing for console
    private variable _logging 0;        # Command logging to file
    private variable _console;          # Debug console window path
    # Number of milliseconds to wait before idle timeout.  If greater than 0,
    # automatically disconnect from the visualization server when idle timeout
    # is reached.
    private variable _idleTimeout 43200000; # 12 hours
    #private variable _idleTimeout 5000;# 5 seconds
    #private variable _idleTimeout 0;   # No timeout

    private common _servers;            # array of visualization server lists
    set _servers(geovis)  "localhost:2015"
    set _servers(nanovis) "localhost:2000"
    set _servers(pymol)   "localhost:2020"
    set _servers(vmdmds)  "localhost:2018"
    set _servers(vtkvis)  "localhost:2010"
    private common _done;               # Used to indicate status of send.
    private common _consoleCnt 0;
}

itk::usual Panedwindow {
    keep -background -cursor
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VisViewer::constructor { args } {

    Rappture::dispatcher _dispatcher
    $_dispatcher register !serverDown
    $_dispatcher dispatch $this !serverDown "[itcl::code $this ServerDown]; list"
    $_dispatcher register !timeout
    $_dispatcher dispatch $this !timeout "[itcl::code $this Disconnect]; list"

    $_dispatcher register !waiting

    set _buffer(in) ""
    set _buffer(out) ""
    #
    # Create a parser to handle incoming requests
    #
    set _parser [interp create -safe]
    foreach cmd [$_parser eval {info commands}] {
        $_parser hide $cmd
    }
    # Add default handlers for "ok" acknowledgement and server errors.
    $_parser alias ok       [itcl::code $this HandleOk]
    $_parser alias viserror [itcl::code $this HandleError]

    #
    # Set up the widgets in the main body
    #
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add main {
        Rappture::SidebarFrame $itk_interior.main -resizeframe 1
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

    global env
    if { [info exists env(VISRECORDER)] } {
        set _logging 1
        if { [file exists /tmp/recording.log] } {
            file delete /tmp/recording.log
        }
    }
    if { [info exists env(VISTRACE)] } {
        set _trace 1
    }
    incr _consoleCnt
    set _console ".renderconsole$_consoleCnt"

    eval itk_initialize $args
}

#
# destructor --
#
itcl::body Rappture::VisViewer::destructor {} {
    $_dispatcher cancel !timeout
    interp delete $_parser
    array unset _done $this
    if {[winfo exists $_console]} {
        destroy $_console
    }
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
#    (no I/O with the server) for some specified time.
#
itcl::body Rappture::VisViewer::Connect { servers } {
    blt::busy hold $itk_component(hull) -cursor watch

    if { $_numConnectTries > $_maxConnects } {
        blt::busy release $itk_component(hull)
        set x [expr {[winfo rootx $itk_component(hull)]+10}]
        set y [expr {[winfo rooty $itk_component(hull)]+10}]
        Rappture::Tooltip::cue @$x,$y "Exceeded maximum number of connection attmepts to any $_serverType visualization server. Please contact support."
        return 0;
    }
    foreach server [Shuffle $servers] {
        puts stderr "connecting to $server..."
        foreach {hostname port} [split $server ":"] break
        if { [catch {socket $hostname $port} _sid] != 0 } {
            set _sid ""
            RemoveServerFromList $_serverType $server
            continue
        }
        incr _numConnectTries
        set _hostname $server
        fconfigure $_sid -translation binary -encoding binary

        # Read back the server identification string.
        if { [gets $_sid data] <= 0 } {
            set _sid ""
            puts stderr "ERORR reading from server data=($data)"
            RemoveServerFromList $_serverType $server
            continue
        }
        puts stderr "Render server is $data"
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
    blt::busy release $itk_component(hull)
    set x [expr {[winfo rootx $itk_component(hull)]+10}]
    set y [expr {[winfo rooty $itk_component(hull)]+10}]
    Rappture::Tooltip::cue @$x,$y "Can't connect to any $_serverType visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
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
    set _buffer(out) ""
    set _outbuf ""
    set _cmdSeq 0
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
#   This routine is called whenever we're about to send/receive data on
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
        Rappture::Tooltip::cue @$x,$y "Can't connect to any $_serverType visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
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
#   connection is writable (i.e. not blocked).  Sets a magic variable
#   _done($this) when we're done.
#
itcl::body Rappture::VisViewer::SendHelper {} {
    if { ![CheckConnection] } {
        return 0
    }
    puts -nonewline $_sid $_buffer(out)
    flush $_sid
    set _buffer(out) ""
    set _done($this) 1;                 # Success
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
    StartWaiting
    # Even though the data is sent in only 1 "puts", we need to verify that
    # the server is ready first.  Wait for the socket to become writable
    # before sending anything.
    set _done($this) 1
    if { $_debug && [string bytelength $_buffer(out)] > 0 } {
        puts stderr "SendBytes: appending to output buffer"
    }
    # Append to the buffer, since we may have been called recursively
    append _buffer(out) $bytes
    # There's problem when the user is interacting with the GUI at the
    # same time we're trying to write to the server.  Don't want to
    # block because, the GUI will look like it's dead.  We can start
    # by putting a busy window over plot so that inadvertent things like
    # mouse movements aren't received.
    if {$_blockOnWrite} {
        # Allow a write to block.  In this case we won't re-enter SendBytes.
        SendHelper
    } else {
        # NOTE: This can cause us to re-enter SendBytes during the tkwait.
        # For this reason, we append to the buffer in the code above.
        if { [info exists itk_component(main)] } {
            blt::busy hold $itk_component(main) -cursor ""
        }
        fileevent $_sid writable [itcl::code $this SendHelper]
        tkwait variable ::Rappture::VisViewer::_done($this)
        if { [info exists itk_component(main)] } {
            blt::busy release $itk_component(main)
        }
    }
    set _buffer(out) ""
    if { [IsConnected] } {
        # The connection may have closed while we were writing to the server.
        # This can happen if what we sent the server caused it to barf.
        fileevent $_sid writable ""
        flush $_sid
    }
    return $_done($this)
}

#
# StartWaiting --
#
#    Display a waiting dialog after a timeout has passed
#
itcl::body Rappture::VisViewer::StartWaiting {} {
    if { $_waitTimeout > 0 } {
        after cancel $_afterId
        set _afterId [after $_waitTimeout [itcl::code $this WaitDialog on]]
    }
}

#
# StopWaiting --
#
#    Take down waiting dialog
#
itcl::body Rappture::VisViewer::StopWaiting {} {
    if { $_waitTimeout > 0 } {
        WaitDialog off
    }
}

itcl::body Rappture::VisViewer::EnableWaitDialog { value } {
    set _waitTimeout $value
}

itcl::body Rappture::VisViewer::DisableWaitDialog {} {
    set _waitTimeout 0
}

#
# ReceiveBytes --
#
#    Read some number of bytes from the visualization server.
#
itcl::body Rappture::VisViewer::ReceiveBytes { size } {
    if { ![CheckConnection] } {
        return 0
    }
    set bytes [read $_sid $size]
    ReceiveEcho <<line "<read $size bytes"
    StopWaiting
    return $bytes
}

#
# ReceiveHelper --
#
#   Helper routine called from a file event when the connection is readable
#   (i.e. a command response has been sent by the rendering server.  Reads
#   the incoming command and executes it in a safe interpreter to handle the
#   action.
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
        if ($_trace) {
            puts stderr "<<[string range $line 0 70]"
        }
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
#   Converts a color name to a list of r,g,b values needed for the engine.
#   Each r/g/b component is scaled in the # range 0-1.
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
    set yangle [expr {180.0-$phi}]
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
    if { $_logging }  {
        set f [open "/tmp/recording.log" "a"]
        fconfigure $f -translation binary -encoding binary
        puts -nonewline $f $data
        close $f
    }
    #puts stderr ">>($data)"
    if {[string length $itk_option(-sendcommand)] > 0} {
        uplevel #0 $itk_option(-sendcommand) [list $channel $data]
    }
}

#
# ReceiveEcho --
#
#     Echoes received data to clients interested in this widget.  If the
#     -receivecommand option is set, then it is invoked in the global scope
#     with the <channel> and <data> values as arguments.  Otherwise, this
#     does nothing.
#
itcl::body Rappture::VisViewer::ReceiveEcho {channel {data ""}} {
    #puts stderr "<<line $data"
    if {[string length $itk_option(-receivecommand)] > 0} {
        uplevel #0 $itk_option(-receivecommand) [list $channel $data]
    }
}

itcl::body Rappture::VisViewer::WaitDialog { state } {
    after cancel $_afterId
    set _afterId -1
    if { $state } {
        if { [winfo exists $itk_component(plotarea).view.splash] } {
            return
        }
        set inner [frame $itk_component(plotarea).view.splash]
        $inner configure -relief raised -bd 2
        label $inner.text1 -text "Working...\nPlease wait." \
            -font "Arial 10"
        label $inner.icon
        pack $inner -expand yes -anchor c
        blt::table $inner \
            0,0 $inner.text1 -anchor w \
            0,1 $inner.icon
        Waiting start $inner.icon
    } else {
        if { ![winfo exists $itk_component(plotarea).view.splash] } {
            return
        }
        Waiting stop $itk_component(plotarea).view.splash
        destroy $itk_component(plotarea).view.splash
    }
}

itcl::body Rappture::VisViewer::Waiting { option widget } {
    switch -- $option {
        "start" {
            $_dispatcher dispatch $this !waiting \
                "[itcl::code $this Waiting "next" $widget] ; list"
            set _icon 0
            $widget configure -image [Rappture::icon bigroller${_icon}]
            $_dispatcher event -after 150 !waiting
        }
        "next" {
            incr _icon
            if { $_icon >= 8 } {
                set _icon 0
            }
            $widget configure -image [Rappture::icon bigroller${_icon}]
            $_dispatcher event -after 150 !waiting
        }
        "stop" {
            $_dispatcher cancel !waiting
        }
    }
}

#
# HideConsole --
#
#    Hide the debug console by withdrawing its toplevel window.
#
itcl::body Rappture::VisViewer::HideConsole {} {
    set _debugConsole 0
    DebugConsole
}

#
# BuildConsole --
#
#    Create and pack the widgets that make up the debug console: a text
#    widget to display the communication and an entry widget to type
#    in commands to send to the render server.
#
itcl::body Rappture::VisViewer::BuildConsole {} {
    toplevel $_console
    wm protocol $_console WM_DELETE_WINDOW [itcl::code $this HideConsole]
    set f $_console
    frame $f.send
    pack $f.send -side bottom -fill x
    label $f.send.l -text "Send:"
    pack $f.send.l -side left
    itk_component add command {
        entry $f.send.e -background white
    } {
        ignore -background
    }
    pack $f.send.e -side left -expand yes -fill x
    bind $f.send.e <Return> [itcl::code $this SendDebugCommand]
    bind $f.send.e <KP_Enter> [itcl::code $this SendDebugCommand]
    scrollbar $f.sb -orient vertical -command "$f.comm yview"
    pack $f.sb -side right -fill y
    itk_component add trace {
        text $f.comm -wrap char -yscrollcommand "$f.sb set" -background white
    } {
        ignore -background
    }
    pack $f.comm -expand yes -fill both
    bind $f.comm <Control-F1> [itcl::code $this ToggleConsole]
    bind $f.comm <Enter> [list focus %W]
    bind $f.send.e <Control-F1> [itcl::code $this ToggleConsole]

    $itk_component(trace) tag configure error -foreground red \
        -font -*-courier-medium-o-normal-*-*-120-*
    $itk_component(trace) tag configure incoming -foreground blue
}

#
# ToggleConsole --
#
#    This is used by derived classes to turn on/off debuging.  It's
#    up the to derived class to decide how to turn on/off debugging.
#
itcl::body Rappture::VisViewer::ToggleConsole {} {
    if { $_debugConsole } {
        set _debugConsole 0
    } else {
        set _debugConsole 1
    }
    DebugConsole
}

#
# DebugConsole --
#
#    Based on the value of the variable _debugConsole, turns on/off
#    debugging. This is done by setting/unsetting a procedure that
#    is called whenever new characters are received or sent on the
#    socket to the render server.  Additionally, the debug console
#    is created if necessary and hidden/shown.
#
itcl::body Rappture::VisViewer::DebugConsole {} {
    if { ![winfo exists $_console] } {
        BuildConsole
    }
    if { $_debugConsole } {
        $this configure -sendcommand [itcl::code $this TraceComm]
        $this configure -receivecommand [itcl::code $this TraceComm]
        wm deiconify $_console
    } else {
        $this configure -sendcommand ""
        $this configure -receivecommand ""
        wm withdraw $_console
    }
}

# ----------------------------------------------------------------------
# USAGE: TraceComm <channel> <data>
#
# Invoked automatically whenever there is communication between
# the rendering widget and the server.  Eavesdrops on the communication
# and posts the commands in a text viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::VisViewer::TraceComm {channel {data ""}} {
    $itk_component(trace) configure -state normal
    switch -- $channel {
        closed {
            $itk_component(trace) insert end "--CLOSED--\n" error
        }
        <<line {
            $itk_component(trace) insert end $data incoming "\n" incoming
        }
        >>line {
            $itk_component(trace) insert end $data outgoing "\n" outgoing
        }
        error {
            $itk_component(trace) insert end $data error "\n" error
        }
        default {
            $itk_component(trace) insert end "$data\n"
        }
    }
    $itk_component(trace) configure -state disabled
    $itk_component(trace) see end
}

# ----------------------------------------------------------------------
# USAGE: SendDebugCommand
#
# Invoked automatically whenever the user enters a command and
# presses <Return>.  Sends the command along to the rendering
# widget.
# ----------------------------------------------------------------------
itcl::body Rappture::VisViewer::SendDebugCommand {} {
    incr _cmdSeq
    set cmd [$itk_component(command) get]
    append cmd "\n"
    if {$_trace} {
        puts stderr "$_cmdSeq>>[string range $cmd 0 70]"
    }
    SendBytes $cmd
    $itk_component(command) delete 0 end
}

#
# HandleOk --
#
#       This handles the "ok" response from the server that acknowledges
#       the reception of a server command, but does not produce an image.
#       It may pass an argument such as "-token 9" that could be used to
#       determine how many commands have been processed by the server.
#
itcl::body Rappture::VisViewer::HandleOk { args } {
    if { $_waitTimeout > 0 } {
        StopWaiting
    }
}

#
# HandleError --
#
#       This handles the "viserror" response from the server that reports
#       that a client-initiated error has occurred on the server.
#
itcl::body Rappture::VisViewer::HandleError { args } {
    array set info {
        -token "???"
        -bytes 0
        -type "???"
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    if { $info(-type) == "error" } {
        set popup $itk_component(hull).error
        if { ![winfo exists $popup] } {
            Rappture::Balloon $popup \
                -title "Render Server Error"
            set inner [$popup component inner]
            label $inner.summary -text "" -anchor w

            Rappture::Scroller $inner.scrl \
                -xscrollmode auto -yscrollmode auto
            text $inner.scrl.text \
                -font "Arial 9" -background white -relief sunken -bd 1 \
                -height 5 -wrap word -width 60
            $inner.scrl contents $inner.scrl.text
            button $inner.ok -text "Dismiss" -command [list $popup deactivate] \
                -font "Arial 9"
            blt::table $inner \
                0,0 $inner.scrl -fill both \
                1,0 $inner.ok
            $inner.scrl.text tag configure normal -font "Arial 9"
            $inner.scrl.text tag configure italic -font "Arial 9 italic"
            $inner.scrl.text tag configure bold -font "Arial 10 bold"
            $inner.scrl.text tag configure code -font "Courier 10 bold"
        } else {
            $popup deactivate
        }
        update
        set inner [$popup component inner]
        $inner.scrl.text delete 0.0 end

        $inner.scrl.text configure -state normal
        $inner.scrl.text insert end "The following error was reported by the render server:\n\n" bold
        $inner.scrl.text insert end $bytes code
        $inner.scrl.text configure -state disabled
        update
        $popup activate $itk_component(hull) below
    } else {
        ReceiveEcho <<error $bytes
        puts stderr "Render server error:\n$bytes"
    }
}

itcl::body Rappture::VisViewer::GetColormapList { args } {
    array set opts {
        -includeDefault 0
        -includeElementDefault 0
        -includeNone 0
    }
    if {[llength $args] > 0} {
        foreach opt $args {
            set opts($opt) 1
        }
    }
    set colormaps [list]
    if {$opts(-includeDefault)} {
        lappend colormaps "default" "default"
    }
    if {$opts(-includeElementDefault)} {
        lappend colormaps "elementDefault" "elementDefault"
    }
    lappend colormaps \
        "BCGYR"              "BCGYR"            \
        "BGYOR"              "BGYOR"            \
        "blue-to-brown"      "blue-to-brown"    \
        "blue-to-orange"     "blue-to-orange"   \
        "blue-to-grey"       "blue-to-grey"     \
        "green-to-magenta"   "green-to-magenta" \
        "greyscale"          "greyscale"        \
        "nanohub"            "nanohub"          \
        "rainbow"            "rainbow"          \
        "spectral"           "spectral"         \
        "ROYGB"              "ROYGB"            \
        "RYGCB"              "RYGCB"            \
        "white-to-blue"      "white-to-blue"    \
        "brown-to-blue"      "brown-to-blue"    \
        "grey-to-blue"       "grey-to-blue"     \
        "orange-to-blue"     "orange-to-blue"
    if {$opts(-includeNone)} {
        lappend colormaps "none" "none"
    }
    return $colormaps
}

itcl::body Rappture::VisViewer::ColorsToColormap { colors } {
    set cmap {}
    switch -- $colors {
        "grey-to-blue" {
            set cmap {
                0.0                      0.200 0.200 0.200
                0.14285714285714285      0.400 0.400 0.400
                0.2857142857142857       0.600 0.600 0.600
                0.42857142857142855      0.900 0.900 0.900
                0.5714285714285714       0.800 1.000 1.000
                0.7142857142857143       0.600 1.000 1.000
                0.8571428571428571       0.400 0.900 1.000
                1.0                      0.000 0.600 0.800
            }
        }
        "blue-to-grey" {
            set cmap {
                0.0                     0.000 0.600 0.800
                0.14285714285714285     0.400 0.900 1.000
                0.2857142857142857      0.600 1.000 1.000
                0.42857142857142855     0.800 1.000 1.000
                0.5714285714285714      0.900 0.900 0.900
                0.7142857142857143      0.600 0.600 0.600
                0.8571428571428571      0.400 0.400 0.400
                1.0                     0.200 0.200 0.200
            }
        }
        "white-to-blue" {
            set cmap {
                0.0                     0.900 1.000 1.000
                0.1111111111111111      0.800 0.983 1.000
                0.2222222222222222      0.700 0.950 1.000
                0.3333333333333333      0.600 0.900 1.000
                0.4444444444444444      0.500 0.833 1.000
                0.5555555555555556      0.400 0.750 1.000
                0.6666666666666666      0.300 0.650 1.000
                0.7777777777777778      0.200 0.533 1.000
                0.8888888888888888      0.100 0.400 1.000
                1.0                     0.000 0.250 1.000
            }
        }
        "brown-to-blue" {
            set cmap {
                0.0                             0.200   0.100   0.000
                0.09090909090909091             0.400   0.187   0.000
                0.18181818181818182             0.600   0.379   0.210
                0.2727272727272727              0.800   0.608   0.480
                0.36363636363636365             0.850   0.688   0.595
                0.45454545454545453             0.950   0.855   0.808
                0.5454545454545454              0.800   0.993   1.000
                0.6363636363636364              0.600   0.973   1.000
                0.7272727272727273              0.400   0.940   1.000
                0.8181818181818182              0.200   0.893   1.000
                0.9090909090909091              0.000   0.667   0.800
                1.0                             0.000   0.480   0.600
            }
        }
        "blue-to-brown" {
            set cmap {
                0.0                             0.000   0.480   0.600
                0.09090909090909091             0.000   0.667   0.800
                0.18181818181818182             0.200   0.893   1.000
                0.2727272727272727              0.400   0.940   1.000
                0.36363636363636365             0.600   0.973   1.000
                0.45454545454545453             0.800   0.993   1.000
                0.5454545454545454              0.950   0.855   0.808
                0.6363636363636364              0.850   0.688   0.595
                0.7272727272727273              0.800   0.608   0.480
                0.8181818181818182              0.600   0.379   0.210
                0.9090909090909091              0.400   0.187   0.000
                1.0                             0.200   0.100   0.000
            }
        }
        "blue-to-orange" {
            set cmap {
                0.0                             0.000   0.167   1.000
                0.09090909090909091             0.100   0.400   1.000
                0.18181818181818182             0.200   0.600   1.000
                0.2727272727272727              0.400   0.800   1.000
                0.36363636363636365             0.600   0.933   1.000
                0.45454545454545453             0.800   1.000   1.000
                0.5454545454545454              1.000   1.000   0.800
                0.6363636363636364              1.000   0.933   0.600
                0.7272727272727273              1.000   0.800   0.400
                0.8181818181818182              1.000   0.600   0.200
                0.9090909090909091              1.000   0.400   0.100
                1.0                             1.000   0.167   0.000
            }
        }
        "orange-to-blue" {
            set cmap {
                0.0                             1.000   0.167   0.000
                0.09090909090909091             1.000   0.400   0.100
                0.18181818181818182             1.000   0.600   0.200
                0.2727272727272727              1.000   0.800   0.400
                0.36363636363636365             1.000   0.933   0.600
                0.45454545454545453             1.000   1.000   0.800
                0.5454545454545454              0.800   1.000   1.000
                0.6363636363636364              0.600   0.933   1.000
                0.7272727272727273              0.400   0.800   1.000
                0.8181818181818182              0.200   0.600   1.000
                0.9090909090909091              0.100   0.400   1.000
                1.0                             0.000   0.167   1.000
            }
        }
        "rainbow" {
            set clist {
                "#EE82EE"
                "#4B0082"
                "blue"
                "#008000"
                "yellow"
                "#FFA500"
                "red"
            }
        }
        "BGYOR" {
            set clist {
                "blue"
                "#008000"
                "yellow"
                "#FFA500"
                "red"
            }
        }
        "ROYGB" {
            set clist {
                "red"
                "#FFA500"
                "yellow"
                "#008000"
                "blue"
            }
        }
        "RYGCB" {
            set clist {
                "red"
                "yellow"
                "green"
                "cyan"
                "blue"
            }
        }
        "BCGYR" {
            set clist {
                "blue"
                "cyan"
                "green"
                "yellow"
                "red"
            }
        }
        "spectral" {
            set cmap {
                0.0 0.150 0.300 1.000
                0.1 0.250 0.630 1.000
                0.2 0.450 0.850 1.000
                0.3 0.670 0.970 1.000
                0.4 0.880 1.000 1.000
                0.5 1.000 1.000 0.750
                0.6 1.000 0.880 0.600
                0.7 1.000 0.680 0.450
                0.8 0.970 0.430 0.370
                0.9 0.850 0.150 0.196
                1.0 0.650 0.000 0.130
            }
        }
        "green-to-magenta" {
            set cmap {
                0.0 0.000 0.316 0.000
                0.06666666666666667 0.000 0.526 0.000
                0.13333333333333333 0.000 0.737 0.000
                0.2 0.000 0.947 0.000
                0.26666666666666666 0.316 1.000 0.316
                0.3333333333333333 0.526 1.000 0.526
                0.4 0.737 1.000 0.737
                0.4666666666666667 1.000 1.000 1.000
                0.5333333333333333 1.000 0.947 1.000
                0.6 1.000 0.737 1.000
                0.6666666666666666 1.000 0.526 1.000
                0.7333333333333333 1.000 0.316 1.000
                0.8 0.947 0.000 0.947
                0.8666666666666667 0.737 0.000 0.737
                0.9333333333333333 0.526 0.000 0.526
                1.0 0.316 0.000 0.316
            }
        }
        "greyscale" {
            set cmap {
                0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0
            }
        }
        "nanohub" {
            set clist "white yellow green cyan blue magenta"
        }
        default {
            set clist [split $colors ":"]
        }
    }
    if {$cmap == ""} {
        if { [llength $clist] == 1 } {
            set rgb [Color2RGB $clist]
            append cmap "0.0 $rgb 1.0 $rgb"
        } else {
            for {set i 0} {$i < [llength $clist]} {incr i} {
                set x [expr {double($i)/([llength $clist]-1)}]
                set color [lindex $clist $i]
                append cmap "$x [Color2RGB $color] "
            }
        }
    } else {
        regsub -all "\[ \t\r\n\]+" [string trim $cmap] " " cmap
    }
    return $cmap
}


#
# StartBufferingCommands --
#
itcl::body Rappture::VisViewer::StartBufferingCommands { } {
    incr _buffering
    if { $_buffering == 1 } {
        set _outbuf ""
    }
}

#
# StopBufferingCommands --
#
#       This gets called when we want to stop buffering the commands for
#       the server and actually send then to the server.  Note that there's
#       a reference count on buffering.  This is so that you can can
#       Start/Stop multiple times without worrying about the current state.
#
itcl::body Rappture::VisViewer::StopBufferingCommands { } {
    incr _buffering -1
    if { $_buffering == 0 && $_outbuf != "" } {
        SendBytes $_outbuf
        set _outbuf ""
    }
}

#
# SendCmd
#
#       Send command off to the rendering server.  If we're currently
#       buffering, the command is queued to be sent later.
#
itcl::body Rappture::VisViewer::SendCmd {string} {
    incr _cmdSeq
    if {$_trace} {
        puts stderr "$_cmdSeq>>[string range $string 0 70]"
    }
    if { $_buffering } {
        append _outbuf $string "\n"
    } else {
        SendBytes "$string\n"
    }
}

#
# SendData
#
#       Send data off to the rendering server.  If we're currently
#       buffering, the data is queued to be sent later.
#
itcl::body Rappture::VisViewer::SendData {bytes} {
    if {$_trace} {
        puts stderr "$_cmdSeq>>data payload"
    }
    if { $_buffering } {
        append _outbuf $bytes
    } else {
        SendBytes $bytes
    }
}
