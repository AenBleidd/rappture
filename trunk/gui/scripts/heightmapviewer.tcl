
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
    protected method Connect {}
    protected method Disconnect {}

    protected method _send {string}
    protected method _send_dataobjs {}
    protected method ReceiveImage {option size}
    private method ReceiveLegend {tf vmin vmax size}
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
    $_parser alias legend [itcl::code $this ReceiveLegend]

    # Initialize the view to some default parameters.
    array set view_ {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        dx	0
        dy	0
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
            -bitmap [Rappture::icon reset] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -side left -padx {4 1} -pady 4
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
    pack $itk_component(zoomin) -side left -padx 1 -pady 4
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
    pack $itk_component(zoomout) -side left -padx {1 4} -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    #
    # Settings panel...
    #
    itk_component add settings {
        button $itk_component(controls).settings -text "Settings..." \
            -borderwidth 1 -relief flat -overrelief raised \
            -padx 2 -pady 1 \
            -command [list $itk_component(controls).panel activate $itk_component(controls).settings left]
    } {
        usual
        ignore -borderwidth
        rename -background -controlbackground controlBackground Background
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(settings) -side top -pady 8

    Rappture::Balloon $itk_component(controls).panel -title "Settings"
    set inner [$itk_component(controls).panel component inner]
    
    frame $inner.f
    pack $inner.f -side top -fill x
    grid columnconfigure $inner.f 1 -weight 1
    set fg [option get $itk_component(hull) font Font]
    
    set ::Rappture::HeightmapViewer::settings_($this-grid) 1
    ::checkbutton $inner.f.grid \
        -text "Grid" \
        -variable ::Rappture::HeightmapViewer::settings_($this-grid) \
        -command [itcl::code $this _fixSettings grid]
    grid $inner.f.grid -row 0 -column 0 -sticky w

    set ::Rappture::HeightmapViewer::settings_($this-axes) 0
    ::checkbutton $inner.f.axes \
        -text "Axes" \
        -variable ::Rappture::HeightmapViewer::settings_($this-axes) \
        -command [itcl::code $this _fixSettings axes]
    grid $inner.f.axes -row 1 -column 0 -sticky w

    set ::Rappture::HeightmapViewer::settings_($this-contourlines) 1
    ::checkbutton $inner.f.contour \
        -text "Contour Lines" \
        -variable ::Rappture::HeightmapViewer::settings_($this-contourlines) \
        -command [itcl::code $this _fixSettings contourlines]
    grid $inner.f.contour -row 2 -column 0 -sticky w

    set ::Rappture::HeightmapViewer::settings_($this-wireframe) "fill"
    ::checkbutton $inner.f.wireframe \
        -text "Wireframe" \
	-onvalue "wireframe" -offvalue "fill" \
        -variable ::Rappture::HeightmapViewer::settings_($this-wireframe) \
        -command [itcl::code $this _fixSettings wireframe]
    grid $inner.f.wireframe -row 3 -column 0 -sticky w

    # Legend
    set _image(legend) [image create photo]
    itk_component add legend {
        canvas $itk_component(area).legend -height 50 -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -plotbackground plotBackground Background
    }
    pack $itk_component(legend) -side bottom -fill x
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
# USAGE: ReceiveLegend <tf> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::ReceiveLegend {tf vmin vmax size} {
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        $_image(legend) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

        set c $itk_component(legend)
        set w [winfo width $c]
        set h [winfo height $c]
        if {"" == [$c find withtag transfunc]} {
            $c create image 10 10 -anchor nw \
                 -image $_image(legend) -tags transfunc

            $c create text 10 [expr {$h-8}] -anchor sw \
                 -fill $itk_option(-plotforeground) -tags vmin
            $c create text [expr {$w-10}] [expr {$h-8}] -anchor se \
                 -fill $itk_option(-plotforeground) -tags vmax
        }
        $c itemconfigure vmin -text $vmin
        $c coords vmin 10 [expr {$h-8}]
        $c itemconfigure vmax -text $vmax
        $c coords vmax [expr {$w-10}] [expr {$h-8}]
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
    _send "camera zoom $view_(zoom)"

     if {"" == $itk_option(-plotoutline)} {
         _send "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
     }
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
        }
        "out" {
            set view_(zoom) [expr {$view_(zoom)*0.8}]
        }
        "reset" {
            array set view_ {
                theta   45
                phi     45
                psi     0
                zoom    1.0
		dx 0
		dy 0
            }
            set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
            _send "camera angle $xyz"
            _send "camera pan $view_(dx) $view_(dy)"
        }
    }
    _send "camera zoom $view_(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: $this _pan click x y
#        $this _pan drag x y
#	 $this _pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::_pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
	set x [expr $x / double($w)]
	set y [expr $y / double($h)]
	set view_(dx) [expr $view_(dx) + $x]
	set view_(dy) [expr $view_(dy) + $y]
        _send "camera pan $view_(dx) $view_(dy)"
	return
    }
    if { $option == "click" } { 
	set click_(x) $x
	set click_(y) $y
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
	set dx [expr ($click_(x) - $x)/double($w)]
	set dy [expr ($click_(y) - $y)/double($h)]
	set click_(x) $x
	set click_(y) $y
	set view_(dx) [expr $view_(dx) - $dx]
	set view_(dy) [expr $view_(dy) - $dy]
	_send "camera pan $view_(dx) $view_(dy)"
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
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
            set lineht [font metrics $itk_option(-font) -linespace]
            set w [expr {[winfo width $itk_component(legend)]-20}]
            set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]
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
