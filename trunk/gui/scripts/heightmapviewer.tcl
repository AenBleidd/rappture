
# ----------------------------------------------------------------------
#  Component: heightmapviewer - 3D surface rendering
#
#  This widget performs surface rendering on 3D scalar/vector datasets.
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
option add *HeightmapViewer.plotBackground black widgetDefault
option add *HeightmapViewer.plotForeground white widgetDefault
option add *HeightmapViewer.plotOutline white widgetDefault
option add *HeightmapViewer.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

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
    public method snap { w h }
    public method download {option args}
    public method parameters {title args} {
        # do nothing
    }
    public method camera {option args}

    protected method Connect {}
    protected method Disconnect {}
    public method isconnected {}

    protected method SendCmd {string}
    protected method ReceiveImage { args }
    private method ReceiveLegend {tf vmin vmax size}
    private method BuildViewTab {}
    private method BuildCameraTab {}
    private method PanCamera {}
    protected method CurrentSurfaces {{what -all}}
    protected method Rebuild {}
    protected method Zoom {option}
    protected method Pan {option x y}
    protected method Rotate {option x y}

    protected method State {comp}
    protected method FixSettings {what {value ""}}
    protected method GetTransfuncData {dataobj comp}
    protected method Resize {}
    private method EventuallyResize { w h } 

    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _obj2style    ;# maps dataobj => style settings
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _click        ;# info used for Rotate operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private common _settings       ;# Array of used for global variables
                                    # for checkbuttons and radiobuttons.
    private variable _serverObjs   ;# contains all the dataobj-component 
                                   ;# to heightmaps in the server
    private variable _location ""
    private variable _first ""
    private variable _width 0
    private variable _height 0
    private common _hardcopy
    private variable _buffering 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0
}

itk::usual HeightmapViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::constructor {hostlist args} {
    set _serverType "nanovis"

    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend \
        "[itcl::code $this FixSettings legend]; list"

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event.
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this Resize]; list"

    set _outbuf ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]

    # Initialize the view to some default parameters.
    array set _view {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        pan-x	0
        pan-y	0
    }
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }
    array set _settings [subst {
        $this-pan-x		$_view(pan-x)
        $this-pan-y		$_view(pan-y)
        $this-phi		$_view(phi)
        $this-psi		$_view(psi)
        $this-theta		$_view(theta)
        $this-surface		1
        $this-xcutplane		0
        $this-xcutposition	0
        $this-ycutplane		0
        $this-ycutposition	0
        $this-zcutplane		0
        $this-zcutposition	0
        $this-zoom		$_view(zoom)
    }]

    itk_component add 3dview {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0 
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }
    $_image(plot) configure -data ""
    $itk_component(3dview) create image 0 0 -anchor nw -image $_image(plot) 
    set f [$itk_component(main) component controls]
    itk_component add zoom {
        frame $f.zoom
    }
    pack $itk_component(zoom) -side top

    itk_component add reset {
        button $f.reset -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this Zoom reset]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(reset) -side top -padx 1 -pady { 4 0 }
    Rappture::Tooltip::for $itk_component(reset) \
        "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $f.zin -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this Zoom in]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(zoomin) -side top -padx 1 -pady { 4 0 }
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $f.zout -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon zoom-out] \
            -command [itcl::code $this Zoom out]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(zoomout) -side top -padx 1 -pady { 4 }
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add surface {
        Rappture::PushButton $f.surface \
            -onimage [Rappture::icon volume-on] \
            -offimage [Rappture::icon volume-off] \
            -command [itcl::code $this FixSettings surface] \
            -variable [itcl::scope _settings($this-surface)]
    }
    $itk_component(surface) select
    Rappture::Tooltip::for $itk_component(surface) \
        "Toggle surfaces on/off"
    pack $itk_component(surface) -padx 2 -pady 2

    BuildViewTab
    BuildCameraTab

    set w [expr [winfo reqwidth $itk_component(hull)] - 80]
    pack forget $itk_component(3dview)
    pack $itk_component(3dview) -side left -fill both -expand yes


    # Bindings for rotation via mouse
    bind $itk_component(3dview) <ButtonPress-1> \
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]
    bind $itk_component(3dview) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    # Bindings for panning via mouse
    bind $itk_component(3dview) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(3dview) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

    # Bindings for panning via keyboard
    bind $itk_component(3dview) <KeyPress-Left> \
        [itcl::code $this Pan set -10 0]
    bind $itk_component(3dview) <KeyPress-Right> \
        [itcl::code $this Pan set 10 0]
    bind $itk_component(3dview) <KeyPress-Up> \
        [itcl::code $this Pan set 0 -10]
    bind $itk_component(3dview) <KeyPress-Down> \
        [itcl::code $this Pan set 0 10]
    bind $itk_component(3dview) <Shift-KeyPress-Left> \
        [itcl::code $this Pan set -2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Right> \
        [itcl::code $this Pan set 2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Up> \
        [itcl::code $this Pan set 0 -2]
    bind $itk_component(3dview) <Shift-KeyPress-Down> \
        [itcl::code $this Pan set 0 2]

    # Bindings for zoom via keyboard
    bind $itk_component(3dview) <KeyPress-Prior> \
        [itcl::code $this Zoom out]
    bind $itk_component(3dview) <KeyPress-Next> \
        [itcl::code $this Zoom in]

    bind $itk_component(3dview) <Enter> "focus $itk_component(3dview)"

    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        bind $itk_component(3dview) <4> [itcl::code $this Zoom out]
        bind $itk_component(3dview) <5> [itcl::code $this Zoom in]
    }

    set _image(download) [image create photo]
    eval itk_initialize $args
    Connect 
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::destructor {} {
    $_dispatcher cancel !rebuild
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
    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _obj2ovride($dataobj-color) $params(-color)
        set _obj2ovride($dataobj-width) $params(-width)
        set _obj2ovride($dataobj-raise) $params(-raise)
        set _obj2ovride($dataobj-brightness) $params(-brightness)
        $_dispatcher event -idle !rebuild
    }
    scale $dataobj
}

# ----------------------------------------------------------------------
# USAGE: get ?-objects?
# USAGE: get ?-image 3dview|legend?
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.  The optional "-image"
# flag can also request the internal images being shown.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::get { args } {
    if {[llength $args] == 0} {
        set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
      -objects {
        # put the dataobj list in order according to -raise options
        set dlist $_dlist
        foreach obj $dlist {
            if { [info exists _obj2ovride($obj-raise)] && 
                 $_obj2ovride($obj-raise)} {
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
itcl::body Rappture::HeightmapViewer::delete { args } {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            foreach key [array names _obj2ovride $dataobj-*] {
                unset _obj2ovride($key)
            }
            array unset _serverObjs $dataobj-*
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
itcl::body Rappture::HeightmapViewer::scale { args } {
    if 0 {
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }
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
                set _limits(${axis}range) [expr {$max - $min}]
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
                blt::winop snap $itk_component(plotarea) $_image(download)
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
            # Get the image data (as base64) and decode it back to binary.
            # This is better than writing to temporary files.  When we switch
            # to the BLT picture image it won't be necessary to decode the
            # image data.
            set bytes [$_image(download) data -format "jpeg -quality 100"]
            set bytes [Rappture::encoding::decode -as b64 $bytes]
            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

#
# isconnected --
#
#       Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::HeightmapViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

# ----------------------------------------------------------------------
# USAGE: Connect ?<host:port>,<host:port>...?
#
# Clients use this method to establish a connection to a new
# server, or to reestablish a connection to the previous server.
# Any existing connection is automatically closed.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Connect {} {
    global readyForNextFrame
    set readyForNextFrame 1
    Disconnect
    set _hosts [GetServerList "nanovis"]
    if { "" == $_hosts } {
        return 0
    }
    catch {unset _serverObjs}
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

    set _outbuf ""
    # disconnected -- no more data sitting on server
    global readyForNextFrame
    set readyForNextFrame 1
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::HeightmapViewer::SendCmd {string} {
    if { $_buffering } {
        append _outbuf $string "\n"
    } else {
        foreach line [split $string \n] {
            SendEcho >>line $line
        }
        SendBytes "$string\n"
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::ReceiveImage { args } {
    global readyForNextFrame
    set readyForNextFrame 1
    if {![IsConnected]} {
        return
    }
    array set info {
        -type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    ReceiveEcho <<line "<read $info(-bytes) bytes"
    if { $info(-type) == "image" } {
        $_image(plot) configure -data $bytes
        ReceiveEcho <<line "<read for [image width $_image(plot)]x[image height $_image(plot)] image>"
    } elseif { $info(type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveLegend <tf> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::ReceiveLegend {obj vmin vmax size} {
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend)] } {
            set _image(legend) [image create photo]
        }
        ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
        set src [image create photo -data $bytes]
        blt::winop image rotate $src $_image(legend) 90
        set dst $_image(legend)

        set c $itk_component(3dview)
        set w [winfo width $c]
        set h [winfo height $c]
        set lineht [font metrics $itk_option(-font) -linespace]

        if { $_settings($this-legend) } {
            if { [$c find withtag "legend"] == "" } {
                $c create image [expr {$w-2}] [expr {$lineht+2}] -anchor ne \
                    -image $_image(legend) -tags "transfunc legend"
                $c create text [expr {$w-2}] 2 -anchor ne \
                    -fill $itk_option(-plotforeground) -tags "vmax legend" \
                    -font "Arial 8 bold"
                $c create text [expr {$w-2}] [expr {$h-2}] -anchor se \
                    -fill $itk_option(-plotforeground) -tags "vmin legend" \
                    -font "Arial 8 bold"
            }
            # Reset the item coordinates according the current size of the plot.
            $c coords transfunc [expr {$w-2}] [expr {$lineht+2}]
            $c itemconfigure vmin -text $vmin
            $c itemconfigure vmax -text $vmax
            $c coords vmin [expr {$w-2}] [expr {$h-2}]
            $c coords vmax [expr {$w-2}] 2
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Rebuild {} {

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    # Reset the overall limits 
    set _limits(vmin) ""
    set _limits(vmax) ""

    set _first ""
    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    EventuallyResize $w $h

    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            # Tell the engine to expect some data
            set tag $dataobj-$comp
            if { ![info exists _serverObjs($tag)] } {
                set data [$dataobj blob $comp]
                set nbytes [string length $data]
                append _outbuf "heightmap data follows $nbytes $dataobj-$comp\n"
                append _outbuf $data
                
                set _serverObjs($tag) $tag
                
                # Determine the transfer function needed for this surface
                # and make sure that it's defined on the server.
                foreach {sname cmap wmap} [GetTransfuncData $dataobj $comp] break
                SendCmd [list "transfunc" "define" $sname $cmap $wmap]
                set _obj2style($tag) $sname
            }
        }
    }

    # Nothing to send -- activate the proper surface
    set _first [lindex [get] 0]
    if {"" != $_first} {
        set axis [$_first hints updir]
        if {"" != $axis} {
            SendCmd "up $axis"
        }
        # This is where the initial camera position is set.
        set location [$_first hints camera]
        if { $_location == "" && $location != "" } {
            array set _view $location
            set _location $location
        }
    }
    SendCmd "heightmap data visible 0"
    set heightmaps [CurrentSurfaces] 
    if { $heightmaps != ""  && $_settings($this-surface) } {
        SendCmd "heightmap data visible 1 $heightmaps"
    }
    set heightmaps [CurrentSurfaces -raise] 
    if { $heightmaps != "" } {
        SendCmd "heightmap opacity 0.25"
        SendCmd "heightmap opacity 0.95 $heightmaps"
    } else {
        SendCmd "heightmap opacity 0.85"
    }
    foreach key $heightmaps {
        if {[info exists _obj2style($key)]} {
            SendCmd "heightmap transfunc $_obj2style($key) $key"
        }
    }
    $_dispatcher event -idle !legend

    if {"" == $itk_option(-plotoutline)} {
        SendCmd "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
    }
    # Reset the camera and other view parameters
    set _settings($this-theta) $_view(theta)
    set _settings($this-phi)   $_view(phi)
    set _settings($this-psi)   $_view(psi)
    set _settings($this-pan-x) $_view(pan-x)
    set _settings($this-pan-y) $_view(pan-y)
    set _settings($this-zoom)  $_view(zoom)

    set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
    SendCmd "camera angle $xyz"
    PanCamera
    SendCmd "camera zoom $_view(zoom)"

    FixSettings wireframe
    FixSettings grid
    FixSettings axes
    FixSettings contourlines

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf;			
    blt::busy release $itk_component(hull)

    # The "readyForNextFrame" variable throttles the sequence play rate.
    global readyForNextFrame
    set readyForNextFrame 0;		# Don't advance to the next frame
					# until we get an image.
    set _buffering 0;			# Turn off buffering.
    set _outbuf "";			# Clear the buffer.		
}

# ----------------------------------------------------------------------
# USAGE: Zoom in
# USAGE: Zoom out
# USAGE: Zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Zoom {option} {
    switch -- $option {
        "in" {
            set _view(zoom) [expr {$_view(zoom)*1.25}]
            set _settings($this-zoom) $_view(zoom)
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
            set _settings($this-zoom) $_view(zoom)
        }
        "reset" {
            array set _view {
                theta   45
                phi     45
                psi     0
                zoom	1.0
                pan-x	0
                pan-y	0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
            SendCmd "camera angle $xyz"
            PanCamera
            set _settings($this-theta) $_view(theta)
            set _settings($this-phi) $_view(phi)
            set _settings($this-psi) $_view(psi)
            set _settings($this-pan-x) $_view(pan-x)
            set _settings($this-pan-y) $_view(pan-y)
            set _settings($this-zoom) $_view(zoom)
        }
    }
    SendCmd "camera zoom $_view(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: $this Pan click x y
#        $this Pan drag x y
#        $this Pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
        set x [expr ($x / double($w)) * $_limits(xrange)]
        set y [expr ($y / double($h)) * $_limits(yrange)]
        set _view(pan-x) [expr $_view(pan-x) + $x]
        set _view(pan-y) [expr $_view(pan-y) + $y]
        PanCamera
        set _settings($this-pan-x) $_view(pan-x)
        set _settings($this-pan-y) $_view(pan-y)
        return
    }
    if { $option == "click" } {
        set _click(x) $x
        set _click(y) $y
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
        set dx [expr (($_click(x) - $x)/double($w)) * $_limits(xrange)]
        set dy [expr (($_click(y) - $y)/double($h)) * $_limits(yrange)]
        set _click(x) $x
        set _click(y) $y
        set _view(pan-x) [expr $_view(pan-x) - $dx]
        set _view(pan-y) [expr $_view(pan-y) - $dy]
        PanCamera
        set _settings($this-pan-x) $_view(pan-x)
        set _settings($this-pan-y) $_view(pan-y)
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

itcl::body Rappture::HeightmapViewer::PanCamera {} {
    set x [expr ($_view(pan-x)) / $_limits(xrange)]
    set y [expr ($_view(pan-y)) / $_limits(yrange)]
    SendCmd "camera pan $x $y"
}

# ----------------------------------------------------------------------
# USAGE: Rotate click <x> <y>
# USAGE: Rotate drag <x> <y>
# USAGE: Rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::Rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            array set _click [subst {
                x       $x
                y       $y
                theta   $_view(theta)
                phi     $_view(phi)
            }]
        }
        drag {
            if {[array size _click] == 0} {
                Rotate click $x $y
            } else {
                set w [winfo width $itk_component(3dview)]
                set h [winfo height $itk_component(3dview)]
                if {$w <= 0 || $h <= 0} {
                    return
                }

                if {[catch {
                    # this fails sometimes for no apparent reason
                    set dx [expr {double($x-$_click(x))/$w}]
                    set dy [expr {double($y-$_click(y))/$h}]
                }] != 0 } {
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

                if {abs($theta) >= 30 && abs($theta) <= 160} {
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

                set _view(theta)        $theta
                set _view(phi)          $phi
                set _view(psi)          $psi
                set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
                set _settings($this-theta) $_view(theta)
                set _settings($this-phi) $_view(phi)
                set _settings($this-psi) $_view(psi)
                SendCmd "camera angle $xyz"
                set _click(x) $x
                set _click(y) $y
            }
        }
        release {
            Rotate drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: State <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::State {comp} {
    if {[$itk_component($comp) cget -relief] == "sunken"} {
        return "on"
    }
    return "off"
}

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::FixSettings { what {value ""} } {
    switch -- $what {
        "legend" {
            if { !$_settings($this-legend) } {
                $itk_component(3dview) delete "legend"
            }
            set lineht [font metrics $itk_option(-font) -linespace]
            set w [winfo height $itk_component(3dview)]
            set h [winfo width $itk_component(3dview)]
            set w [expr {$w - 2*$lineht - 4}]
            set h 12
            set tag ""
            if {"" != $_first} {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
            }
            if {$w > 0 && $h > 0 && "" != $tag} {
                SendCmd "heightmap legend $tag $w $h"
            } else {
                #$itk_component(legend) delete all
            }
        }
        "surface" {
            if { [isconnected] } {
                SendCmd "heightmap data visible $_settings($this-surface)"
            }
        }
        "grid" {
            if { [IsConnected] } {
                SendCmd "grid visible $_settings($this-grid)"
            }
        }
        "axes" {
            if { [IsConnected] } {
                SendCmd "axis visible $_settings($this-axes)"
            }
        }
        "wireframe" {
            if { [IsConnected] } {
                SendCmd "heightmap polygon $_settings($this-wireframe)"
            }
        }
        "contourlines" {
            if {[IsConnected]} {
                if {"" != $_first} {
                    set comp [lindex [$_first components] 0]
                    if { $comp != "" } {
                        set tag $_first-$comp
                        set bool $_settings($this-contourlines)
                        SendCmd "heightmap linecontour visible $bool $tag"
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
# USAGE: GetTransfuncData <dataobj> <comp>
#
# Used internally to compute the colormap and alpha map used to define
# a transfer function for the specified component in a data object.
# Returns: name {v r g b ...} {v w ...}
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::GetTransfuncData {dataobj comp} {
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
    if { [info exists style(-nonuniformcolors)] } {
        foreach { value color } $style(-nonuniformcolors) {
            append cmap "$value [Color2RGB $color] "
        }
    } else {
        set clist [split $style(-color) :]
        set cmap "0.0 [Color2RGB white] "
        for {set i 0} {$i < [llength $clist]} {incr i} {
            set x [expr {double($i+1)/([llength $clist]+1)}]
            set color [lindex $clist $i]
            append cmap "$x [Color2RGB $color] "
        }
        append cmap "1.0 [Color2RGB $color]"
    }
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
    #SendCmd "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::HeightmapViewer::plotforeground {
    foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
    #fix this!
    #SendCmd "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::HeightmapViewer::plotoutline {
    if {[IsConnected]} {
        SendCmd "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
    }
}



#  camera -- 
#
itcl::body Rappture::HeightmapViewer::camera {option args} {
    switch -- $option { 
        "show" {
            puts [array get _view]
        }
        "set" {
            set who [lindex $args 0]
            set x $_settings($this-$who)
            set code [catch { string is double $x } result]
            if { $code != 0 || !$result } {
                set _settings($this-$who) $_view($who)
                return
            }
            switch -- $who {
                "pan-x" - "pan-y" {
                    set _view($who) $_settings($this-$who)
                    PanCamera
                }
                "phi" - "theta" - "psi" {
                    set _view($who) $_settings($this-$who)
                    set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
                    SendCmd "camera angle $xyz"
                }
                "zoom" {
                    set _view($who) $_settings($this-$who)
                    SendCmd "camera zoom $_view(zoom)"
                }
            }
        }
    }
}

itcl::body Rappture::HeightmapViewer::BuildViewTab {} {
    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    foreach { key value } {
        grid		1
        axes		0
        contourlines	1
        wireframe	fill
        legend		1
    } {
        set _settings($this-$key) $value
    }

    checkbutton $inner.surface \
        -text "surface" \
        -variable [itcl::scope _settings($this-surface)] \
        -command [itcl::code $this FixSettings surface] \
        -font "Arial 9"
    checkbutton $inner.grid \
        -text "grid" \
        -variable [itcl::scope _settings($this-grid)] \
        -command [itcl::code $this FixSettings grid] \
        -font "Arial 9"
    checkbutton $inner.axes \
        -text "axes" \
        -variable ::Rappture::HeightmapViewer::_settings($this-axes) \
        -command [itcl::code $this FixSettings axes] \
        -font "Arial 9"
    checkbutton $inner.contourlines \
        -text "contour lines" \
        -variable ::Rappture::HeightmapViewer::_settings($this-contourlines) \
        -command [itcl::code $this FixSettings contourlines]\
        -font "Arial 9"
    checkbutton $inner.wireframe \
        -text "wireframe" \
        -onvalue "wireframe" -offvalue "fill" \
        -variable ::Rappture::HeightmapViewer::_settings($this-wireframe) \
        -command [itcl::code $this FixSettings wireframe]\
        -font "Arial 9"
    checkbutton $inner.legend \
        -text "legend" \
        -variable ::Rappture::HeightmapViewer::_settings($this-legend) \
        -command [itcl::code $this FixSettings legend]\
        -font "Arial 9"

    blt::table $inner \
        0,1 $inner.surface -anchor w  \
        1,1 $inner.grid -anchor w  \
        2,1 $inner.axes -anchor w \
        3,1 $inner.contourlines -anchor w \
        4,1 $inner.wireframe -anchor w \
        5,1 $inner.legend -anchor w 

    blt::table configure $inner c2 -resize expand
    blt::table configure $inner c1 -resize none
    blt::table configure $inner r* -resize none
    blt::table configure $inner r6 -resize expand
}

itcl::body Rappture::HeightmapViewer::BuildCameraTab {} {
    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set labels { phi theta psi pan-x pan-y zoom }
    set row 1
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9" -bg white -width 10 \
            -textvariable [itcl::scope _settings($this-$tag)]
        bind $inner.${tag} <KeyPress-Return> \
            [itcl::code $this camera set ${tag}]
        blt::table $inner \
            $row,1 $inner.${tag}label -anchor e \
            $row,2 $inner.${tag} -anchor w 
        blt::table configure $inner r$row -resize none
        incr row
    }
    blt::table configure $inner c1 c2 -resize none
    blt::table configure $inner c3 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::HeightmapViewer::Resize {} {
    SendCmd "screen $_width $_height"
    set _resizePending 0
    $_dispatcher event -idle !legend
}

itcl::body Rappture::HeightmapViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        $_dispatcher event -after 200 !resize
        set _resizePending 1
    }
}

# ----------------------------------------------------------------------
# USAGE: CurrentVolumes ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::HeightmapViewer::CurrentSurfaces {{what -all}} {
    set list {}
    if { $what == "-all" } {
        foreach key [array names _serverObjs] {
            foreach {dataobj comp} [split $key -] break
            if { [info exists _obj2ovride($dataobj-raise)] } {
                lappend list $dataobj-$comp
            }
        }
    } else {
        foreach key [array names _serverObjs] {
            foreach {dataobj comp} [split $key -] break
            if { [info exists _obj2ovride($dataobj$what)] && 
                 $_obj2ovride($dataobj$what) } {
                lappend list $dataobj-$comp
            }
        }
    }
    return $list
}

itcl::body Rappture::HeightmapViewer::snap { w h } {
    if { $w <= 0 || $h <= 0 } {
        set w [image width $_image(plot)]
        set h [image height $_image(plot)]
    } 
    set img [image create picture -width $w -height $h]
    $img resample $_image(plot)
    return $img
}
