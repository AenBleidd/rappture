# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer - 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
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

option add *NanovisViewer.width 4i widgetDefault
option add *NanovisViewer.height 4i widgetDefault
option add *NanovisViewer.foreground black widgetDefault
option add *NanovisViewer.controlBackground gray widgetDefault
option add *NanovisViewer.controlDarkBackground #999999 widgetDefault
option add *NanovisViewer.plotBackground black widgetDefault
option add *NanovisViewer.plotForeground white widgetDefault
option add *NanovisViewer.plotOutline gray widgetDefault
option add *NanovisViewer.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::NanovisViewer {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""
    itk_option define -plotoutline plotOutline PlotOutline ""

    constructor {host port args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {option}

    public method connect {{host ""} {port ""}}
    public method disconnect {}
    public method isconnected {}

    public method _send {args}
    protected method _receive {}
    protected method _receive_image {option size}

    protected method _rebuild {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _slice {option args}
    protected method _slicertip {axis}
    protected method _color2rgb {color}
    protected method _euler2xyz {theta phi psi}

    private variable _nvhost ""    ;# host name for nanovis server
    private variable _nvport ""    ;# port number for nanovis server
    private variable _sid ""       ;# socket connection to nanovis server
    private variable _parser ""    ;# interpreter for incoming commands
    private variable _buffer ""    ;# buffer for incoming commands
    private variable _image ""     ;# image displayed in plotting area
    private variable _dlist ""     ;# list of data objects
    private variable _dims ""      ;# dimensionality of data objects
    private variable _obj2color    ;# maps dataobj => plotting color
    private variable _obj2width    ;# maps dataobj => line width
    private variable _obj2raise    ;# maps dataobj => raise flag 0/1
    private variable _click        ;# info used for _move operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
}

itk::usual NanovisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::constructor {nvhost nvport args} {
puts "nanovisviewer $nvhost $nvport = $this"
    #
    # Create a parser to handle incoming requests
    #
    set _parser [interp create -safe]
    foreach cmd [$_parser eval {info commands}] {
        $_parser hide $cmd
    }
    $_parser alias image [itcl::code $this _receive_image]

    #
    # Set up the widgets in the main body
    #
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    set _view(theta) 45
    set _view(phi) 45
    set _view(psi) 0
    set _view(zoom) 1
    set _view(xfocus) 0
    set _view(yfocus) 0
    set _view(zfocus) 0

    itk_component add controls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add zoom {
        frame $itk_component(controls).zoom
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(zoom) -side top

    itk_component add reset {
        button $itk_component(zoom).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon reset] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
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
        button $itk_component(zoom).zout \
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

    #
    # Create slicer controls...
    #
    itk_component add slicers {
        frame $itk_component(controls).slicers
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(slicers) -side bottom -padx 4 -pady 4
    grid rowconfigure $itk_component(slicers) 1 -weight 1

    #
    # Volume toggle...
    #
    itk_component add volume {
        label $itk_component(slicers).volume \
            -borderwidth 1 -relief sunken -padx 1 -pady 1 \
            -text "Volume"
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(volume) <ButtonPress> \
        [itcl::code $this _slice volume toggle]
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    grid $itk_component(volume) -row 0 -column 0 -columnspan 3 \
        -sticky ew -padx 1

    #
    # X-value slicer...
    #
    itk_component add xslice {
        label $itk_component(slicers).xslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap [Rappture::icon x]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(xslice) <ButtonPress> \
        [itcl::code $this _slice axis x toggle]
    Rappture::Tooltip::for $itk_component(xslice) \
        "Toggle the X cut plane on/off"
    grid $itk_component(xslice) -row 1 -column 0 -sticky ew -padx 1

    itk_component add xslicer {
        ::scale $itk_component(slicers).xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move x]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(xslicer) set 50
    $itk_component(xslicer) configure -state disabled
    grid $itk_component(xslicer) -row 2 -column 0 -padx 1
    Rappture::Tooltip::for $itk_component(xslicer) \
        "@[itcl::code $this _slicertip x]"

    #
    # Y-value slicer...
    #
    itk_component add yslice {
        label $itk_component(slicers).yslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap [Rappture::icon y]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(yslice) <ButtonPress> \
        [itcl::code $this _slice axis y toggle]
    Rappture::Tooltip::for $itk_component(yslice) \
        "Toggle the Y cut plane on/off"
    grid $itk_component(yslice) -row 1 -column 1 -sticky ew -padx 1

    itk_component add yslicer {
        ::scale $itk_component(slicers).yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move y]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(yslicer) set 50
    $itk_component(yslicer) configure -state disabled
    grid $itk_component(yslicer) -row 2 -column 1 -padx 1
    Rappture::Tooltip::for $itk_component(yslicer) \
        "@[itcl::code $this _slicertip y]"

    #
    # Z-value slicer...
    #
    itk_component add zslice {
        label $itk_component(slicers).zslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap [Rappture::icon z]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    grid $itk_component(zslice) -row 1 -column 2 -sticky ew -padx 1
    bind $itk_component(zslice) <ButtonPress> \
        [itcl::code $this _slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zslice) \
        "Toggle the Z cut plane on/off"

    itk_component add zslicer {
        ::scale $itk_component(slicers).zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move z]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(zslicer) set 50
    $itk_component(zslicer) configure -state disabled
    grid $itk_component(zslicer) -row 2 -column 2 -padx 1
    Rappture::Tooltip::for $itk_component(zslicer) \
        "@[itcl::code $this _slicertip z]"

    #
    # RENDERING AREA
    #
    set _image [image create photo]
    itk_component add area {
        label $itk_interior.area -image $_image
    }
    pack $itk_component(area) -expand yes -fill both

    # set up bindings for rotation
    bind $itk_component(area) <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(area) <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(area) <ButtonRelease> \
        [itcl::code $this _move release %x %y]
    bind $itk_component(area) <Configure> \
        [itcl::code $this _send screen %w %h]

    eval itk_initialize $args

    connect $nvhost $nvport
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::destructor {} {
    after cancel [itcl::code $this _rebuild]
    image delete $_image
    interp delete $_parser
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -width 1
        -linestyle solid
        -brightness 0
        -raise 0
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }
    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
    }

    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _obj2color($dataobj) $params(-color)
        set _obj2width($dataobj) $params(-width)
        set _obj2raise($dataobj) $params(-raise)

        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist
    foreach obj $dlist {
        if {[info exists _obj2raise($obj)] && $_obj2raise($obj)} {
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
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _obj2color($dataobj)}
            catch {unset _obj2width($dataobj)}
            catch {unset _obj2raise($dataobj)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<data1> <data2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <data> objects.  This
# accounts for all objects--even those not showing on the screen.
# Because of this, the limits are appropriate for all objects as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::scale {args} {
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }
    foreach obj $args {
        foreach axis {x y z v} {
            foreach {min max} [$obj limits $axis] break
            if {"" != $min && "" != $max} {
                if {"" == $_limits(${axis}min)} {
                    set _limits(${axis}min) $min
                    set _limits(${axis}max) $max
                } else {
                    if {$min < $_limits(${axis}min)} {
                        set _limits(${axis}min) $min
                    }
                    if {$max > $_limits(${axis}max)} {
                        set _limits(${axis}max) $max
                    }
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: download coming
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::download {option} {
    switch $option {
        coming {
            # do nothing -- we have the image
        }
        now {
            #
            # Hack alert!  Need data in binary format,
            # so we'll save to a file and read it back.
            #
            set tmpfile /tmp/image[pid].jpg
            $_image write $tmpfile -format jpeg
            set fid [open $tmpfile r]
            fconfigure $fid -encoding binary -translation binary
            set bytes [read $fid]
            close $fid
            file delete -force $tmpfile

            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: connect ?<host>? ?<port>?
#
# Clients use this method to establish a connection to a new
# server, or to reestablish a connection to the previous server.
# Any existing connection is automatically closed.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::connect {{host ""} {port ""}} {
puts " connecting... $host $port"
    disconnect

    if {"" != $host} { set _nvhost $host }
    if {"" != $port} { set _nvport $port }

    if {"" == $_nvhost || "" == $_nvport} {
        return 0
    }

    # HACK ALERT! punt on this for now
    set memorySize 10000

    #
    # Connect to the nanovis server.  Send the server some estimate
    # of the size of our job.  If it's too busy, that server may
    # forward us to another.
    #
    while {1} {
puts "connecting to $_nvhost:$_nvport"
        if {[catch {socket $_nvhost $_nvport} sid]} {
            return 0
        }
        fconfigure $sid -translation binary -encoding binary

        # send memory requirement to the load balancer
        puts -nonewline $sid [binary format i $memorySize]
        flush $sid

        # read back a reconnection order
        set data [read $sid 4]
        if {[binary scan $data cccc b1 b2 b3 b4] != 4} {
            error "couldn't read redirection request"
        }
        set addr [format "%u.%u.%u.%u" \
            [expr {$b1 & 0xff}] \
            [expr {$b2 & 0xff}] \
            [expr {$b3 & 0xff}] \
            [expr {$b4 & 0xff}]]

puts "  got back $addr"
        if {[string equal $addr "0.0.0.0"]} {
            fconfigure $sid -buffering line
            fileevent $sid readable [itcl::code $this _receive]
            set _sid $sid
            return 1
        }
        set hostname $addr
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: disconnect
#
# Clients use this method to disconnect from the current rendering
# server.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::disconnect {} {
    if {"" != $_sid} {
        catch {close $_sid}
        set _sid ""
    }
}

# ----------------------------------------------------------------------
# USAGE: isconnected
#
# Clients use this method to see if we are currently connected to
# a server.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::isconnected {} {
    return [expr {"" != $_sid}]
}

# ----------------------------------------------------------------------
# USAGE: _send <arg> <arg> ...
#
# Used internally to send commands off to the rendering server.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_send {args} {
    if {"" == $_sid} {
        set x [expr {[winfo rootx $itk_component(area)]+10}]
        set y [expr {[winfo rooty $itk_component(area)]+10}]
        Rappture::Tooltip::cue @$x,$y "Connecting..."

        if {[catch {connect}] == 0} {
            set w [winfo width $itk_component(area)]
            set h [winfo height $itk_component(area)]
            _send screen $w $h
            _zoom reset
            Rappture::Tooltip::cue hide
        } else {
            Rappture::Tooltip::cue @$x,$y "Can't connect to visualization server.  This may be a network problem.  Wait a few moments and try resetting the view."
            return
        }
    }
    if {"" != $_sid} {
puts "send: $args"
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
itcl::body Rappture::NanovisViewer::_receive {} {
    if {"" != $_sid} {
        if {[gets $_sid line] < 0} {
puts "closed"
            disconnect
        } elseif {[string equal [string range $line 0 2] "nv>"]} {
            append _buffer [string range $line 3 end]
            if {[info complete $_buffer]} {
                set request $_buffer
                set _buffer ""
                $_parser eval $request
            }
        } else {
            puts $line
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _receive_image -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_receive_image {option size} {
    if {"" != $_sid} {
        set bytes [read $_sid $size]
        $_image configure -data $bytes
puts "--IMAGE"
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_rebuild {} {
    eval _send camera angle [_euler2xyz $_view(theta) $_view(phi) $_view(psi)]
    _send camera zoom $_view(zoom)

    if {"" == $itk_option(-plotoutline)} {
        _send volume outline state off
    } else {
        _send volume outline state on
        _send volume outline color [_color2rgb $itk_option(-plotoutline)]
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
itcl::body Rappture::NanovisViewer::_zoom {option} {
    switch -- $option {
        in {
            set _view(zoom) [expr {$_view(zoom)*1.25}]
            _send camera zoom $_view(zoom)
        }
        out {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
            _send camera zoom $_view(zoom)
        }
        reset {
            set _view(theta) 45
            set _view(phi) 45
            set _view(psi) 0
            set _view(zoom) 1.0
            eval _send camera angle [_euler2xyz $_view(theta) $_view(phi) $_view(psi)]
            _send camera zoom $_view(zoom)
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
itcl::body Rappture::NanovisViewer::_move {option x y} {
    switch -- $option {
        click {
            $itk_component(area) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
        }
        drag {
            if {[array size _click] == 0} {
                _move click $x $y
            } else {
                set w [winfo width $itk_component(area)]
                set h [winfo height $itk_component(area)]
                if {$w <= 0 || $h <= 0} {
                    return
                }

                if {[catch {
                    # this fails sometimes for no apparent reason
                    set dx [expr {double($x-$_click(x))/$w}]
                    set dy [expr {double($y-$_click(y))/$h}]
                }]} {
                    return
                }

                #
                # Rotate the camera in 3D
                #
                if {$_view(psi) > 90 || $_view(psi) < -90} {
                    # when psi is flipped around, theta moves backwards
                    set dy [expr {-$dy}]
                }
                set theta [expr {$_view(theta) - $dy*180}]
                while {$theta < 0} { set theta [expr {$theta+180}] }
                while {$theta > 180} { set theta [expr {$theta-180}] }

                if {$theta > 45 && $theta < 135} {
                    set phi [expr {$_view(phi) - $dx*360}]
                    while {$phi < 0} { set phi [expr {$phi+360}] }
                    while {$phi > 360} { set phi [expr {$phi-360}] }
                    set psi $_view(psi)
                } else {
                    set phi $_view(phi)
                    set psi [expr {$_view(psi) - $dx*360}]
                    while {$psi < -180} { set psi [expr {$psi+360}] }
                    while {$psi > 180} { set psi [expr {$psi-360}] }
                }

                set _view(theta) $theta
                set _view(phi) $phi
                set _view(psi) $psi
                eval _send camera angle [_euler2xyz $_view(theta) $_view(phi) $_view(psi)]

                set _click(x) $x
                set _click(y) $y
            }
        }
        release {
            _move drag $x $y
            $itk_component(area) configure -cursor ""
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _slice axis x|y|z ?on|off|toggle?
# USAGE: _slice move x|y|z <newval>
# USAGE: _slice volume ?on|off|toggle?
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_slice {option args} {
    switch -- $option {
        axis {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"_slice axis x|y|z ?on|off|toggle?\""
            }
            set axis [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            if {[$itk_component(${axis}slice) cget -relief] == "raised"} {
                set current "off"
            } else {
                set current "on"
            }

            if {$op == "toggle"} {
                if {$current == "on"} { set op "off" } else { set op "on" }
            }

            if {$op} {
                $itk_component(${axis}slicer) configure -state normal
                _send cutplane state on $axis 0
                $itk_component(${axis}slice) configure -relief sunken
            } else {
                $itk_component(${axis}slicer) configure -state disabled
                _send cutplane state off $axis 0
                $itk_component(${axis}slice) configure -relief raised
            }
        }
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set newpos [expr {0.01*$newval}]
            set newval [expr {0.01*($newval-50)
                *($_limits(${axis}max)-$_limits(${axis}min))
                  + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]

            # show the current value in the readout
puts "readout: $axis = $newval"

            _send cutplane position $newpos $axis 0
        }
        volume {
            if {[llength $args] > 1} {
                error "wrong # args: should be \"_slice volume ?on|off|toggle?\""
            }
            set op [lindex $args 0]
            if {$op == ""} { set op "on" }

            if {[$itk_component(volume) cget -relief] == "raised"} {
                set current "off"
            } else {
                set current "on"
            }

            if {$op == "toggle"} {
                if {$current == "on"} { set op "off" } else { set op "on" }
            }

            if {$op} {
                _send volume state on 0
                $itk_component(volume) configure -relief sunken
            } else {
                _send volume state off 0
                $itk_component(volume) configure -relief raised
            }
        }
        default {
            error "bad option \"$option\": should be axis, move, or volume"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _slicertip <axis>
#
# Used internally to generate a tooltip for the x/y/z slicer controls.
# Returns a message that includes the current slicer value.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_slicertip {axis} {
    set val [$itk_component(${axis}slicer) get]
    set val [expr {0.01*($val-50)
        *($_limits(${axis}max)-$_limits(${axis}min))
          + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val"
}

# ----------------------------------------------------------------------
# USAGE: _color2rgb <color>
#
# Used internally to convert a color name to a set of {r g b} values
# needed for the engine.  Each r/g/b component is scaled in the
# range 0-1.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_color2rgb {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] break
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

# ----------------------------------------------------------------------
# USAGE: _euler2xyz <theta> <phi> <psi>
#
# Used internally to convert euler angles for the camera placement
# the to angles of rotation about the x/y/z axes, used by the engine.
# Returns a list:  {xangle, yangle, zangle}.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_euler2xyz {theta phi psi} {
    set xangle [expr {$theta-90.0}]
    set yangle [expr {180-$phi}]
    set zangle $psi
    return [list $xangle $yangle $zangle]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotbackground {
    foreach {r g b} [_color2rgb $itk_option(-plotbackground)] break
puts "fix this!"
    #_send color background $r $g $b
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotforeground {
    foreach {r g b} [_color2rgb $itk_option(-plotforeground)] break
puts "fix this!"
    #_send color background $r $g $b
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotoutline {
    if {[isconnected]} {
        if {"" == $itk_option(-plotoutline)} {
            _send volume outline state off
        } else {
            _send volume outline state on
            _send volume outline color [_color2rgb $itk_option(-plotoutline)]
        }
    }
}
