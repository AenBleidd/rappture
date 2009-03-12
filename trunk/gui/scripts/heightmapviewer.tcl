
# ----------------------------------------------------------------------
#  COMPONENT: heightmapviewer - 3D volume rendering
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

option add *HeightmapViewer.width 4i widgetDefault
option add *HeightmapViewer.height 4i widgetDefault
option add *HeightmapViewer.foreground black widgetDefault
option add *HeightmapViewer.controlBackground gray widgetDefault
option add *HeightmapViewer.controlDarkBackground #999999 widgetDefault
option add *HeightmapViewer.plotBackground black widgetDefault
option add *HeightmapViewer.plotForeground white widgetDefault
option add *HeightmapViewer.plotOutline white widgetDefault
option add *HeightmapViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc HeightmapViewer_init_resources {} {
    Rappture::resources::register \
        nanovis_server Rappture::HeightmapViewer::SetServerList
}

itcl::class Rappture::HeightmapViewer {
    inherit Rappture::VisViewer

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""
    itk_option define -plotoutline plotOutline PlotOutline ""

    constructor { hostlist args } {
        Rappture::VisViewer::constructor $hostlist
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    public proc SetServerList { namelist } {
        Rappture::VisViewer::SetServerList "nanovis" $namelist
    }
    public method add {dataobj {settings ""}}
    public method get {args}
    public method delete {args}
    public method scale {args}
    public method download {option args}
    public method parameters {title args} {
        # do nothing
    }
    public method drawer {what who}
    public method camera {option args}
    protected method Connect {}
    protected method Disconnect {}

    protected method _send {string}
    protected method _send_dataobjs {}
    protected method ReceiveImage {option size}
    private method _ReceiveLegend {tf vmin vmax size}
    private method _BuildSettingsDrawer {}
    private method _BuildCameraDrawer {}
    private method _PanCamera {}
    protected method _receive_echo {channel {data ""}}

    protected method _rebuild {}
    protected method _zoom {option}
    protected method _pan {option x y}
    protected method _rotate {option x y}

    protected method _state {comp}
    protected method _fixSettings {what {value ""}}
    protected method _getTransfuncData {dataobj comp}


    private variable outbuf_       ;# buffer for outgoing commands

    private variable dlist_ ""     ;# list of data objects
    private variable obj2style_    ;# maps dataobj => style settings
    private variable obj2ovride_   ;# maps dataobj => style override
    private variable obj2id_       ;# maps dataobj => heightmap ID in server
    private variable id2obj_       ;# maps heightmap ID => dataobj in server
    private variable sendobjs_ ""  ;# list of data objs to send to server
    private variable receiveIds_   ;# list of data responses from the server
    private variable click_        ;# info used for _rotate operations
    private variable limits_       ;# autoscale min/max for all axes
    private variable view_         ;# view params for 3D view
    private common settings_      ;# Array used for checkbuttons and radiobuttons
    private variable initialized_
}

itk::usual HeightmapViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::constructor {hostlist args} {
    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend \
        "[itcl::code $this _fixSettings legend]; list"
    # Send dataobjs event
    $_dispatcher register !send_dataobjs
    $_dispatcher dispatch $this !send_dataobjs \
        "[itcl::code $this _send_dataobjs]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    set outbuf_ ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this _ReceiveLegend]

    # Initialize the view to some default parameters.
    array set view_ {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        pan-x	0
        pan-y	0
    }
    set obj2id_(count) 0

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
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -side top -padx 2 -pady { 2 0 }
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -side top -padx 2 -pady { 2 0 }
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(zoom).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-out] \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -side top -padx 2 -pady { 2 0 }
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add settings_button {
        label $itk_component(controls).settingsbutton \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -image [Rappture::icon wrench]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(settings_button) -padx 2 -pady { 0 2 } \
	-ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(settings_button) \
	"Configure settings"
    bind $itk_component(settings_button) <ButtonPress> \
        [itcl::code $this drawer toggle settings]
    pack $itk_component(settings_button) -side bottom \
	-padx 2 -pady 2 -anchor e

    itk_component add camera_button {
        label $itk_component(controls).camerabutton \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -image [Rappture::icon camera]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    Rappture::Tooltip::for $itk_component(camera_button) \
	"Camera settings"
    bind $itk_component(camera_button) <ButtonPress> \
        [itcl::code $this drawer toggle camera]
    pack $itk_component(camera_button) -side bottom \
	-padx 2 -pady { 0 2 } -ipadx 1 -ipady 1

    _BuildSettingsDrawer
    _BuildCameraDrawer

    # Legend
    set _image(legend) [image create photo]
    itk_component add legend {
        canvas $itk_component(area).legend -width 30 -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -plotbackground plotBackground Background
    }
    pack $itk_component(legend) -side right -fill y
    pack $itk_component(3dview) -side left -expand yes -fill both
    bind $itk_component(legend) <Configure> \
        [list $_dispatcher event -idle !legend]

    # Bindings for rotation via mouse
    bind $itk_component(3dview) <ButtonPress-1> \
        [itcl::code $this _rotate click %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _rotate drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-1> \
        [itcl::code $this _rotate release %x %y]
    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _send "screen %w %h"]

    # Bindings for panning via mouse
    bind $itk_component(3dview) <ButtonPress-2> \
        [itcl::code $this _pan click %x %y]
    bind $itk_component(3dview) <B2-Motion> \
        [itcl::code $this _pan drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-2> \
        [itcl::code $this _pan release %x %y]

    # Bindings for panning via keyboard
    bind $itk_component(3dview) <KeyPress-Left> \
        [itcl::code $this _pan set -10 0]
    bind $itk_component(3dview) <KeyPress-Right> \
        [itcl::code $this _pan set 10 0]
    bind $itk_component(3dview) <KeyPress-Up> \
        [itcl::code $this _pan set 0 -10]
    bind $itk_component(3dview) <KeyPress-Down> \
        [itcl::code $this _pan set 0 10]
    bind $itk_component(3dview) <Shift-KeyPress-Left> \
        [itcl::code $this _pan set -2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Right> \
        [itcl::code $this _pan set 2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Up> \
        [itcl::code $this _pan set 0 -2]
    bind $itk_component(3dview) <Shift-KeyPress-Down> \
        [itcl::code $this _pan set 0 2]

    # Bindings for zoom via keyboard
    bind $itk_component(3dview) <KeyPress-Prior> \
        [itcl::code $this _zoom out]
    bind $itk_component(3dview) <KeyPress-Next> \
        [itcl::code $this _zoom in]

    bind $itk_component(3dview) <Enter> "focus $itk_component(3dview)"

    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        bind $itk_component(3dview) <4> [itcl::code $this _zoom out]
        bind $itk_component(3dview) <5> [itcl::code $this _zoom in]
    }

    set _image(download) [image create photo]

    eval itk_initialize $args

    Connect 
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::destructor {} {
    set sendobjs_ ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -width 1
        -linestyle solid
        -brightness 0
        -raise 0
        -description ""
        -param ""
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
    set location [$dataobj hints camera]
    if { $location != "" } {
	array set view_ $location
    }
    set pos [lsearch -exact $dataobj $dlist_]
    if {$pos < 0} {
        lappend dlist_ $dataobj
        set obj2ovride_($dataobj-color) $params(-color)
        set obj2ovride_($dataobj-width) $params(-width)
        set obj2ovride_($dataobj-raise) $params(-raise)
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: get ?-objects?
# USAGE: get ?-image 3dview|legend?
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.  The optional "-image"
# flag can also request the internal images being shown.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::get {args} {
    if {[llength $args] == 0} {
        set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
      -objects {
        # put the dataobj list in order according to -raise options
        set dlist $dlist_
        foreach obj $dlist {
            if { [info exists obj2ovride_($obj-raise)] && 
                 $obj2ovride_($obj-raise)} {
                set i [lsearch -exact $dlist $obj]
                if {$i >= 0} {
                    set dlist [lreplace $dlist $i $i]
                    lappend dlist $obj
                }
            }
        }
        return $dlist
      }
      -image {
        if {[llength $args] != 2} {
            error "wrong # args: should be \"get -image 3dview|legend\""
        }
        switch -- [lindex $args end] {
            3dview {
                return $_image(plot)
            }
            legend {
                return $_image(legend)
            }
            default {
                error "bad image name \"[lindex $args end]\": should be 3dview or legend"
            }
        }
      }
      default {
        error "bad option \"$op\": should be -objects or -image"
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $dlist_
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $dlist_ $dataobj]
        if {$pos >= 0} {
            set dlist_ [lreplace $dlist_ $pos $pos]
            foreach key [array names obj2ovride_ $dataobj-*] {
                unset obj2ovride_($key)
            }
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !rebuild
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
itcl::body Rappture::HeightmapViewer::scale {args} {
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set limits_($val) ""
    }
    foreach obj $args {
        foreach axis {x y z v} {
            foreach {min max} [$obj limits $axis] break
            if {"" != $min && "" != $max} {
                if {"" == $limits_(${axis}min)} {
                    set limits_(${axis}min) $min
                    set limits_(${axis}max) $max
                } else {
                    if {$min < $limits_(${axis}min)} {
                        set limits_(${axis}min) $min
                    }
                    if {$max > $limits_(${axis}max)} {
                        set limits_(${axis}max) $max
                    }
                }
		set limits_(${axis}range) [expr {$max - $min}]
            }
        }
    }
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
itcl::body Rappture::HeightmapViewer::download {option args} {
    switch $option {
        coming {
            if {[catch {
                blt::winop snap $itk_component(area) $_image(download)
            }]} {
                $_image(download) configure -width 1 -height 1
                $_image(download) put #000000
            }
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            #
            # Hack alert!  Need data in binary format,
            # so we'll save to a file and read it back.
            #
            set tmpfile /tmp/image[pid].jpg
            $_image(download) write $tmpfile -format jpeg
            set fid [open $tmpfile r]
            fconfigure $fid -encoding binary -translation binary
            set bytes [read $fid]
            close $fid
            file delete -force $tmpfile

            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Connect ?<host:port>,<host:port>...?
#
# Clients use this method to establish a connection to a new
# server, or to reestablish a connection to the previous server.
# Any existing connection is automatically closed.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Connect {} {
    Disconnect
    set _hosts [GetServerList "nanovis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
    return $result
}

# ----------------------------------------------------------------------
# USAGE: Disconnect
#
# Clients use this method to disconnect from the current rendering
# server.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Disconnect {} {
    VisViewer::Disconnect

    set outbuf_ ""
    # disconnected -- no more data sitting on server
    catch {unset obj2id_}
    array unset id2obj_
    set obj2id_(count) 0
    set id2obj_(cound) 0
    set sendobjs_ ""
}

#
# _send
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::HeightmapViewer::_send {string} {
    if {[llength $sendobjs_] > 0} {
        append outbuf_ $string "\n"
    } else {
        if {[SendBytes $string]} {
            foreach line [split $string \n] {
                SendEcho >>line $line
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _send_dataobjs
#
# Used internally to send a series of volume objects off to the
# server.  Sends each object, a little at a time, with updates in
# between so the interface doesn't lock up.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_send_dataobjs {} {
    blt::busy hold $itk_component(hull); update idletasks

    # Reset the overall limits 
    if { $sendobjs_ != "" } {
        set limits_(vmin) ""
        set limits_(vmax) ""
    }
    foreach dataobj $sendobjs_ {
        foreach comp [$dataobj components] {
            set data [$dataobj blob $comp]

            foreach { vmin vmax }  [$dataobj limits v] break
            if { $limits_(vmin) == "" || $vmin < $limits_(vmin) } {
                set limits_(vmin) $vmin
            }
            if { $limits_(vmax) == "" || $vmax > $limits_(vmax) } {
                set limits_(vmax) $vmax
            }

            # tell the engine to expect some data
            set nbytes [string length $data]
            if { ![SendBytes "heightmap data follows $nbytes"] } {
                return

            }
            if { ![SendBytes $data] } {
                return
            }
            set id $obj2id_(count)
            incr obj2id_(count)
            set id2obj_($id) [list $dataobj $comp]
            set obj2id_($dataobj-$comp) $id
            set receiveIds_($id) 1

            #
            # Determine the transfer function needed for this volume
            # and make sure that it's defined on the server.
            #
            foreach {sname cmap wmap} [_getTransfuncData $dataobj $comp] break
            set cmdstr [list "transfunc" "define" $sname $cmap $wmap]
            if {![SendBytes $cmdstr]} {
                return
            }
            set obj2style_($dataobj-$comp) $sname
        }
    }
    set sendobjs_ ""
    blt::busy release $itk_component(hull)

    # activate the proper volume
    set first [lindex [get] 0]
    if {"" != $first} {
        set axis [$first hints updir]
        if {"" != $axis} {
            _send "up $axis"
        }
    }

    foreach key [array names obj2id_ *-*] {
        set state [string match $first-* $key]
        _send "heightmap data visible $state $obj2id_($key)"
        if {[info exists obj2style_($key)]} {
            _send "heightmap transfunc $obj2style_($key) $obj2id_($key)"
        }
    }

    # if there are any commands in the buffer, send them now that we're done
    SendBytes $outbuf_
    set outbuf_ ""

    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::ReceiveImage {option size} {
    if {[IsConnected]} {
        set bytes [ReceiveBytes $size]
        $_image(plot) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(plot)]x[image height $_image(plot)] image>"
    }
}

# ----------------------------------------------------------------------
# USAGE: _ReceiveLegend <tf> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_ReceiveLegend {tf vmin vmax size} {
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
	if 1 {
	set src [image create photo -data $bytes]
	blt::winop image rotate $src $_image(legend) 90
	set dst $_image(legend)
	} else {
	$_image(legend) configure -data $bytes
	}
        set c $itk_component(legend)
        set w [winfo width $c]
        set h [winfo height $c]
	set lineht [expr [font metrics $itk_option(-font) -linespace] + 4]
        if {"" == [$c find withtag transfunc]} {
            $c create image 0 [expr $lineht] -anchor ne \
                 -image $_image(legend) -tags transfunc
            $c create text 10 [expr {$h-8}] -anchor se \
                 -fill $itk_option(-plotforeground) -tags vmin
            $c create text [expr {$w-10}] [expr {$h-8}] -anchor ne \
                 -fill $itk_option(-plotforeground) -tags vmax
        }
	$c coords transfunc [expr $w - 5] [expr $lineht]
        $c itemconfigure vmin -text $vmin
        $c itemconfigure vmax -text $vmax
        $c coords vmax [expr $w - 5] 2
        $c coords vmin [expr $w - 5] [expr $h - 2]
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_rebuild {} {
    # in the midst of sending data? then bail out
    if {[llength $sendobjs_] > 0} {
        return
    }
    # Find any new data that needs to be sent to the server.  Queue this up on
    # the sendobjs_ list, and send it out a little at a time.  Do this first,
    # before we rebuild the rest.
    foreach dataobj [get] {
        set comp [lindex [$dataobj components] 0]
        if {![info exists obj2id_($dataobj-$comp)]} {
            set i [lsearch -exact $sendobjs_ $dataobj]
            if {$i < 0} {
                lappend sendobjs_ $dataobj
            }
        }
    }
    if {[llength $sendobjs_] > 0} {
        # Send off new data objects
        $_dispatcher event -idle !send_dataobjs
    } else {
        # Nothing to send -- activate the proper volume
        set first [lindex [get] 0]
        if {"" != $first} {
            set axis [$first hints updir]
            if {"" != $axis} {
                _send "up $axis"
            }
        }
        foreach key [array names obj2id_ *-*] {
            set state [string match $first-* $key]
            _send "heightmap data visible $state $obj2id_($key)"
            if {[info exists obj2style_($key)]} {
                _send "heightmap transfunc $obj2style_($key) $obj2id_($key)"
            }
        }
        $_dispatcher event -idle !legend
    }

    # Reset the screen size.  
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    _send "screen $w $h"

    # Reset the camera and other view parameters
    set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
    _send "camera angle $xyz"
    _PanCamera
    _send "camera zoom $view_(zoom)"

     if {"" == $itk_option(-plotoutline)} {
         _send "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
     }
    set settings_($this-theta) $view_(theta)
    set settings_($this-phi) $view_(phi)
    set settings_($this-psi) $view_(psi)
    set settings_($this-pan-x) $view_(pan-x)
    set settings_($this-pan-y) $view_(pan-y)
    set settings_($this-zoom) $view_(zoom)

    _fixSettings wireframe
    _fixSettings grid
    _fixSettings axes
    _fixSettings contourlines
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_zoom {option} {
    switch -- $option {
        "in" {
            set view_(zoom) [expr {$view_(zoom)*1.25}]
	    set settings_($this-zoom) $view_(zoom)
        }
        "out" {
            set view_(zoom) [expr {$view_(zoom)*0.8}]
	    set settings_($this-zoom) $view_(zoom)
        }
        "reset" {
            array set view_ {
                theta   45
                phi     45
                psi     0
                zoom	1.0
		pan-x	0
		pan-y	0
            }
	    set first [lindex [get] 0]
	    if { $first != "" } {
		set location [$first hints camera]
		if { $location != "" } {
		    array set view_ $location
		}
	    }
            set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
            _send "camera angle $xyz"
            _PanCamera
	    set settings_($this-theta) $view_(theta)
	    set settings_($this-phi) $view_(phi)
	    set settings_($this-psi) $view_(psi)
	    set settings_($this-pan-x) $view_(pan-x)
	    set settings_($this-pan-y) $view_(pan-y)
	    set settings_($this-zoom) $view_(zoom)
        }
    }
    _send "camera zoom $view_(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: $this _pan click x y
#        $this _pan drag x y
#        $this _pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
	set x [expr ($x / double($w)) * $limits_(xrange)]
	set y [expr ($y / double($h)) * $limits_(yrange)]
	set view_(pan-x) [expr $view_(pan-x) + $x]
	set view_(pan-y) [expr $view_(pan-y) + $y]
	_PanCamera
	set settings_($this-pan-x) $view_(pan-x)
	set settings_($this-pan-y) $view_(pan-y)
        return
    }
    if { $option == "click" } {
        set click_(x) $x
        set click_(y) $y
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
	set dx [expr (($click_(x) - $x)/double($w)) * $limits_(xrange)]
	set dy [expr (($click_(y) - $y)/double($h)) * $limits_(yrange)]
	set click_(x) $x
	set click_(y) $y
	set view_(pan-x) [expr $view_(pan-x) - $dx]
	set view_(pan-y) [expr $view_(pan-y) - $dy]
	_PanCamera
	set settings_($this-pan-x) $view_(pan-x)
	set settings_($this-pan-y) $view_(pan-y)
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

itcl::body Rappture::HeightmapViewer::_PanCamera {} {
    set x [expr ($view_(pan-x)) / $limits_(xrange)]
    set y [expr ($view_(pan-y)) / $limits_(yrange)]
    _send "camera pan $x $y"
}

# ----------------------------------------------------------------------
# USAGE: _rotate click <x> <y>
# USAGE: _rotate drag <x> <y>
# USAGE: _rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            array set click_ [subst {
                x       $x
                y       $y
                theta   $view_(theta)
                phi     $view_(phi)
            }]
        }
        drag {
            if {[array size click_] == 0} {
                _rotate click $x $y
            } else {
                set w [winfo width $itk_component(3dview)]
                set h [winfo height $itk_component(3dview)]
                if {$w <= 0 || $h <= 0} {
                    return
                }

                if {[catch {
                    # this fails sometimes for no apparent reason
                    set dx [expr {double($x-$click_(x))/$w}]
                    set dy [expr {double($y-$click_(y))/$h}]
                }] != 0 } {
                    return
                }

                #
                # Rotate the camera in 3D
                #
                if {$view_(psi) > 90 || $view_(psi) < -90} {
                    # when psi is flipped around, theta moves backwards
                    set dy [expr {-$dy}]
                }
                set theta [expr {$view_(theta) - $dy*180}]
                while {$theta < 0} { set theta [expr {$theta+180}] }
                while {$theta > 180} { set theta [expr {$theta-180}] }

                if {abs($theta) >= 30 && abs($theta) <= 160} {
                    set phi [expr {$view_(phi) - $dx*360}]
                    while {$phi < 0} { set phi [expr {$phi+360}] }
                    while {$phi > 360} { set phi [expr {$phi-360}] }
                    set psi $view_(psi)
                } else {
                    set phi $view_(phi)
                    set psi [expr {$view_(psi) - $dx*360}]
                    while {$psi < -180} { set psi [expr {$psi+360}] }
                    while {$psi > 180} { set psi [expr {$psi-360}] }
                }

                set view_(theta)        $theta
                set view_(phi)          $phi
                set view_(psi)          $psi
                set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
		set settings_($this-theta) $view_(theta)
		set settings_($this-phi) $view_(phi)
		set settings_($this-psi) $view_(psi)
                _send "camera angle $xyz"
                set click_(x) $x
                set click_(y) $y
            }
        }
        release {
            _rotate drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset click_}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _state <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_state {comp} {
    if {[$itk_component($comp) cget -relief] == "sunken"} {
        return "on"
    }
    return "off"
}

# ----------------------------------------------------------------------
# USAGE: _fixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_fixSettings { what {value ""} } {
    switch -- $what {
        "legend" {
	    if { $settings_($this-legend) } {
		pack $itk_component(legend) -side right -fill y
	    } else {
		pack forget $itk_component(legend) 
	    }
            set lineht [expr [font metrics $itk_option(-font) -linespace] + 4]
            set w [expr {[winfo height $itk_component(legend)] - 2*$lineht}]
            set h [expr {[winfo width $itk_component(legend)] - 16}]
            set imap ""
            set dataobj [lindex [get] 0]
            if {"" != $dataobj} {
                set comp [lindex [$dataobj components] 0]
                if {[info exists obj2id_($dataobj-$comp)]} {
                    set imap $obj2id_($dataobj-$comp)
                }
            }
            if {$w > 0 && $h > 0 && "" != $imap} {
                _send "heightmap legend $imap $w $h"
            } else {
                $itk_component(legend) delete all
            }
        }
        "grid" {
            if { [IsConnected] } {
                _send "grid visible $settings_($this-grid)"
            }
        }
        "axes" {
            if { [IsConnected] } {
                _send "axis visible $settings_($this-axes)"
            }
        }
        "wireframe" {
            if { [IsConnected] } {
                _send "heightmap polygon $settings_($this-wireframe)"
            }
        }
        "contourlines" {
            if {[IsConnected]} {
                set dataobj [lindex [get] 0]
                if {"" != $dataobj} {
                    set comp [lindex [$dataobj components] 0]
                    if {[info exists obj2id_($dataobj-$comp)]} {
                        set i $obj2id_($dataobj-$comp)
                        set bool $settings_($this-contourlines)
                        _send "heightmap linecontour visible $bool $i"
                    }
                }
            }
        }
        default {
            error "don't know how to fix $what: should be grid, axes, contourlines, or legend"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getTransfuncData <dataobj> <comp>
#
# Used internally to compute the colormap and alpha map used to define
# a transfer function for the specified component in a data object.
# Returns: name {v r g b ...} {v w ...}
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_getTransfuncData {dataobj comp} {
    array set style {
        -color rainbow
        -levels 6
        -opacity 0.5
    }
    array set style [lindex [$dataobj components -style $comp] 0]
    set sname "$style(-color):$style(-levels):$style(-opacity)"

    if {$style(-color) == "rainbow"} {
        set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    set clist [split $style(-color) :]
    set color white
    set cmap "0.0 [Color2RGB $color] "
    set range [expr $limits_(vmax) - $limits_(vmin)]
    for {set i 0} {$i < [llength $clist]} {incr i} {
        set xval [expr {double($i+1)/([llength $clist]+1)}]
        set color [lindex $clist $i]
        append cmap "$xval [Color2RGB $color] "
    }
    append cmap "1.0 [Color2RGB $color] "

    set opacity $style(-opacity)
    set levels $style(-levels)
    set wmap {}
    if {[string is int $levels]} {
        lappend wmap 0.0 0.0
        set delta [expr {0.125/($levels+1)}]
        for {set i 1} {$i <= $levels} {incr i} {
            # add spikes in the middle
            set xval [expr {double($i)/($levels+1)}]
            lappend wmap [expr {$xval-$delta-0.01}] 0.0
            lappend wmap [expr {$xval-$delta}] $opacity 
            lappend wmap [expr {$xval+$delta}] $opacity
            lappend wmap [expr {$xval+$delta+0.01}] 0.0
        }
        lappend wmap 1.0 0.0
    } else {
        lappend wmap 0.0 0.0
        set delta 0.05
        foreach xval [split $levels ,] {
            lappend wmap [expr {$xval-$delta}] 0.0
            lappend $xval $opacity
            lappend [expr {$xval+$delta}] 0.0
        }
        lappend wmap 1.0 0.0
    }
    return [list $sname $cmap $wmap]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::HeightmapViewer::plotbackground {
    foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
    #fix this!
    #_send "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::HeightmapViewer::plotforeground {
    foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
    #fix this!
    #_send "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::HeightmapViewer::plotoutline {
    if {[IsConnected]} {
        _send "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
    }
}



#  camera -- 
#
itcl::body Rappture::HeightmapViewer::camera {option args} {
    switch -- $option { 
	"show" {
	    puts [array get view_]
	}
	"set" {
	    set who [lindex $args 0]
	    set x $settings_($this-$who)
	    set code [catch { string is double $x } result]
	    if { $code != 0 || !$result } {
		set settings_($this-$who) $view_($who)
		return
	    }
	    switch -- $who {
		"pan-x" - "pan-y" {
		    set view_($who) $settings_($this-$who)
		    _PanCamera
		}
		"phi" - "theta" - "psi" {
		    set view_($who) $settings_($this-$who)
		    set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
		    _send "camera angle $xyz"
		}
		"zoom" {
		    set view_($who) $settings_($this-$who)
		    _send "camera zoom $view_(zoom)"
		}
	    }
	}
    }
}

itcl::body Rappture::HeightmapViewer::_BuildSettingsDrawer {} {

    itk_component add settings {
	Rappture::Scroller $itk_component(drawer).scrl \
	    -xscrollmode auto -yscrollmode auto \
	    -width 200 -height 100
    }

    itk_component add settings_canvas {
	canvas $itk_component(settings).canvas -highlightthickness 0
    }
    $itk_component(settings) contents $itk_component(settings_canvas)

    itk_component add settings_frame {
	frame $itk_component(settings_canvas).frame -bg white \
	    -highlightthickness 0 
    } 
    $itk_component(settings_canvas) create window 0 0 \
	-anchor nw -window $itk_component(settings_frame)
    bind $itk_component(settings_frame) <Configure> \
	[itcl::code $this drawer resize settings]

    set fg [option get $itk_component(hull) font Font]

    set inner $itk_component(settings_frame)

    foreach { key value } {
	grid		1
	axes		0
	contourlines	1
	wireframe	fill
	legend		1
    } {
	set settings_($this-$key) $value
    }
    set inner $itk_component(settings_frame)
    label $inner.title -text "View Settings" -font "Arial 10 bold"
    checkbutton $inner.grid \
        -text "grid" \
        -variable [itcl::scope settings_($this-grid)] \
        -command [itcl::code $this _fixSettings grid] \
	-font "Arial 9"
    checkbutton $inner.axes \
        -text "axes" \
        -variable ::Rappture::HeightmapViewer::settings_($this-axes) \
        -command [itcl::code $this _fixSettings axes] \
	-font "Arial 9"
    checkbutton $inner.contourlines \
        -text "contour lines" \
        -variable ::Rappture::HeightmapViewer::settings_($this-contourlines) \
        -command [itcl::code $this _fixSettings contourlines]\
	-font "Arial 9"
    checkbutton $inner.wireframe \
        -text "wireframe" \
	-onvalue "wireframe" -offvalue "fill" \
        -variable ::Rappture::HeightmapViewer::settings_($this-wireframe) \
        -command [itcl::code $this _fixSettings wireframe]\
	-font "Arial 9"
    checkbutton $inner.legend \
        -text "legend" \
        -variable ::Rappture::HeightmapViewer::settings_($this-legend) \
        -command [itcl::code $this _fixSettings legend]\
	-font "Arial 9"

    blt::table $inner \
	0,0 $inner.title -anchor w -columnspan 2 \
	1,1 $inner.grid -anchor w  \
	2,1 $inner.axes -anchor w \
	3,1 $inner.contourlines -anchor w \
	4,1 $inner.wireframe -anchor w \
	5,1 $inner.legend -anchor w 

    blt::table configure $inner c0 -resize expand -width 2
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner c1 -resize none

}

itcl::body Rappture::HeightmapViewer::_BuildCameraDrawer {} {

    itk_component add camera {
	Rappture::Scroller $itk_component(drawer).camerascrl \
	    -xscrollmode auto -yscrollmode auto \
	    -highlightthickness 0
    }

    itk_component add camera_canvas {
	canvas $itk_component(camera).canvas -highlightthickness 0
    } {
	ignore -highlightthickness
    }
    $itk_component(camera) contents $itk_component(camera_canvas)

    itk_component add camera_frame {
	frame $itk_component(camera_canvas).frame \
	    -highlightthickness 0 
    } 
    $itk_component(camera_canvas) create window 0 0 \
	-anchor nw -window $itk_component(camera_frame)
    bind $itk_component(camera_frame) <Configure> \
	[itcl::code $this drawer resize camera]

    set inner $itk_component(camera_frame)

    label $inner.title -text "Camera Settings" -font "Arial 10 bold"

    set labels { phi theta psi pan-x pan-y zoom }
    blt::table $inner \
	0,0 $inner.title -anchor w  -columnspan 4 
    set row 1
    foreach tag $labels {
	label $inner.${tag}label -text $tag -font "Arial 9"
	entry $inner.${tag} -font "Arial 9"  -bg white \
	    -textvariable [itcl::scope settings_($this-$tag)]
	bind $inner.${tag} <KeyPress-Return> \
	    [itcl::code $this camera set ${tag}]
	blt::table $inner \
	    $row,1 $inner.${tag}label -anchor e \
	    $row,2 $inner.${tag} -anchor w 
	incr row
    }
    bind $inner.title <Shift-ButtonPress> \
	[itcl::code $this camera show]
    blt::table configure $inner c0 -resize expand -width 4
    blt::table configure $inner c1 c2 -resize none
    blt::table configure $inner c3 -resize expand

}

itcl::body Rappture::HeightmapViewer::drawer { what who } {
    switch -- ${what} {
	"activate" {
	    $itk_component(drawer) add $itk_component($who) -sticky nsew
	    after idle [list focus $itk_component($who)]
	    if { ![info exists initialized_($who)] } {
		set w [winfo width $itk_component(drawer)]
		set x [expr $w - 120]
		$itk_component(drawer) sash place 0 $x 0
		set initialized_($who) 1
	    }
	    $itk_component(${who}_button) configure -relief sunken
	}
	"deactivate" {
	    $itk_component(drawer) forget $itk_component($who)
	    $itk_component(${who}_button) configure -relief raised
	}
	"toggle" {
	    set slaves [$itk_component(drawer) panes]
	    if { [lsearch $slaves $itk_component($who)] >= 0 } {
		drawer deactivate $who
	    } else {
		drawer activate $who
	    }
	}
	"resize" {
	    set bbox [$itk_component(${who}_canvas) bbox all]
	    set wid [winfo width $itk_component(${who}_frame)]
	    $itk_component(${who}_canvas) configure -width $wid \
		-scrollregion $bbox -yscrollincrement 0.1i
	}
    }
}

