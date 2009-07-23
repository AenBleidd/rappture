
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
					
#
# FIXME:
#	Need to Add DX readers this client to examine the data before 
#	it's sent to the server.  This will eliminate 90% of the insanity in
#	computing the limits of all the volumes.  I can rip out all the 
#	"receive data" "send transfer function" event crap.
#
#       This means we can compute the transfer function (relative values) and
#	draw the legend min/max values without waiting for the information to
#	come from the server.  This will also prevent the flashing that occurs
#	when a new volume is drawn (using the default transfer function) and
#	then when the correct transfer function has been sent and linked to
#	the volume.  
#
option add *NanovisViewer.width 4i widgetDefault
option add *NanovisViewer*cursor crosshair widgetDefault
option add *NanovisViewer.height 4i widgetDefault
option add *NanovisViewer.foreground black widgetDefault
option add *NanovisViewer.controlBackground gray widgetDefault
option add *NanovisViewer.controlDarkBackground #999999 widgetDefault
option add *NanovisViewer.plotBackground black widgetDefault
option add *NanovisViewer.plotForeground white widgetDefault
option add *NanovisViewer.plotOutline gray widgetDefault
option add *NanovisViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc NanovisViewer_init_resources {} {
    Rappture::resources::register \
	nanovis_server Rappture::NanovisViewer::SetServerList
}

itcl::class Rappture::NanovisViewer {
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
    public method camera {option args}
    public method delete {args}
    public method disconnect {}
    public method download {option args}
    public method get {args}
    public method isconnected {}
    public method limits { tf }
    public method overmarker { m x }
    public method sendto { string }
    public method parameters {title args} { 
	# do nothing 
    }
    public method rmdupmarker { m x }
    public method scale {args}
    public method updatetransferfuncs {}

    protected method Connect {}
    protected method CurrentVolumes {{what -all}}
    protected method Disconnect {}
    protected method DoResize {}
    protected method FixLegend {}
    protected method FixSettings {what {value ""}}
    protected method Pan {option x y}
    protected method Rebuild {}
    protected method ReceiveData { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { tf vmin vmax size }
    protected method Rotate {option x y}
    protected method SendCmd {string}
    protected method SendTransferFuncs {}
    protected method Slice {option args}
    protected method SlicerTip {axis}
    protected method Zoom {option}

    # The following methods are only used by this class.
    private method AddIsoMarker { x y }
    private method BuildCameraTab {}
    private method BuildCutplanesTab {}
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    private method ComputeTransferFunc { tf }
    private method EventuallyResize { w h } 
    private method EventuallyResizeLegend { } 
    private method NameTransferFunc { dataobj comp }
    private method PanCamera {}
    private method ParseLevelsOption { tf levels }
    private method ParseMarkersOption { tf markers }
    private method volume { tag name }
    private method GetVolumeInfo { w }

    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _allDataObjs
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _serverVols   ;# contains all the dataobj-component 
				   ;# to volumes in the server
    private variable _serverTfs    ;# contains all the transfer functions 
				   ;# in the server.
    private variable _recvdVols    ;# list of data objs to send to server
    private variable _vol2style    ;# maps dataobj-component to transfunc
    private variable _style2vols   ;# maps tf back to list of 
				    # dataobj-components using the tf.

    private variable _click        ;# info used for rotate operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private variable _isomarkers    ;# array of isosurface level values 0..1
    private common   _settings
    # Array of transfer functions in server.  If 0 the transfer has been
    # defined but not loaded.  If 1 the transfer function has been named
    # and loaded.
    private variable _activeTfs
    private variable _first ""     ;# This is the topmost volume.
    private variable _buffering 0

    # This 
    # indicates which isomarkers and transfer
    # function to use when changing markers,
    # opacity, or thickness.
    common _downloadPopup          ;# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0
}

itk::usual NanovisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::constructor {hostlist args} {

    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this FixLegend]; list"

    # Send transfer functions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
	"[itcl::code $this SendTransferFuncs]; list"

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    set _outbuf ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]

    # Initialize the view to some default parameters.
    array set _view {
	theta   45
	phi     45
	psi     0
	zoom    1.0
	pan-x	0
	pan-y	0
    }
    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

    array set _settings [subst {
	$this-pan-x		$_view(pan-x)
	$this-pan-y		$_view(pan-y)
	$this-phi		$_view(phi)
	$this-psi		$_view(psi)
	$this-theta		$_view(theta)
	$this-volume		1
	$this-xcutplane		0
	$this-xcutposition	0
	$this-ycutplane		0
	$this-ycutposition	0
	$this-zcutplane		0
	$this-zcutposition	0
	$this-zoom		$_view(zoom)
    }]

    itk_component add 3dview {
	label $itk_component(plotarea).vol -image $_image(plot) \
	    -highlightthickness 0 -borderwidth 0
    } {
	usual
	ignore -highlightthickness -borderwidth  -background
    }

    set f [$itk_component(main) component controls]
    itk_component add reset {
        button $f.reset -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this Zoom reset]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(reset) -side top -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $f.zin -borderwidth 1 -padx 1 -pady 1 \
            -highlightthickness 0 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this Zoom in]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(zoomin) -side top -padx 2 -pady 2
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
    pack $itk_component(zoomout) -side top -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add volume {
        Rappture::PushButton $f.volume \
	    -onimage [Rappture::icon volume-on] \
	    -offimage [Rappture::icon volume-off] \
	    -command [itcl::code $this FixSettings volume] \
	    -variable [itcl::scope _settings($this-volume)]
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    pack $itk_component(volume) -padx 2 -pady 2

    BuildViewTab
    BuildVolumeTab
    BuildCutplanesTab
    BuildCameraTab

    # Legend

    set _image(legend) [image create photo]
    itk_component add legend {
	canvas $itk_component(plotarea).legend -height 50 -highlightthickness 0
    } {
	usual
	ignore -highlightthickness
	rename -background -plotbackground plotBackground Background
    }
    bind $itk_component(legend) <Configure> \
	[itcl::code $this EventuallyResizeLegend]

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(3dview)
    blt::table $itk_component(plotarea) \
	0,0 $itk_component(3dview) -fill both -reqwidth $w \
	1,0 $itk_component(legend) -fill x 
    blt::table configure $itk_component(plotarea) r1 -resize none

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
itcl::body Rappture::NanovisViewer::destructor {} {
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_transfunc
    $_dispatcher cancel !resize
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
    array unset _settings $this-*
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
	set _allDataObjs($dataobj) 1
	set _obj2ovride($dataobj-color) $params(-color)
	set _obj2ovride($dataobj-width) $params(-width)
	set _obj2ovride($dataobj-raise) $params(-raise)
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
itcl::body Rappture::NanovisViewer::get {args} {
    if {[llength $args] == 0} {
	set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
      -objects {
	# put the dataobj list in order according to -raise options
	set dlist $_dlist
	foreach obj $dlist {
	    if {[info exists _obj2ovride($obj-raise)] && $_obj2ovride($obj-raise)} {
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
#       Clients use this to delete a dataobj from the plot.  If no dataobjs
#       are specified, then all dataobjs are deleted.  No data objects are
#       deleted.  They are only removed from the display list.
#
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::delete {args} {
    if {[llength $args] == 0} {
	set args $_dlist
    }
    # Delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
	set pos [lsearch -exact $_dlist $dataobj]
	if { $pos >= 0 } {
	    set _dlist [lreplace $_dlist $pos $pos]
	    array unset _limits $dataobj*
	    array unset _obj2ovride $dataobj-*
	    array unset _vol2style $dataobj-*
	    set changed 1
	}
    }
    # If anything changed, then rebuild the plot
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
itcl::body Rappture::NanovisViewer::scale {args} {
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
	set _limits($val) ""
    }
    foreach obj $args {
	foreach axis {x y z v} {

	    foreach { min max } [$obj limits $axis] break

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
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::download {option args} {
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
	    set bytes [$_image(plot) data -format "jpeg -quality 100"]
	    set bytes [Rappture::encoding::decode -as b64 $bytes]
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
itcl::body Rappture::NanovisViewer::Connect {} {
    set _hosts [GetServerList "nanovis"]
    if { "" == $_hosts } {
	return 0
    }
    set result [VisViewer::Connect $_hosts]
    if { $result } {
	set w [winfo width $itk_component(3dview)]
	set h [winfo height $itk_component(3dview)]
	EventuallyResize $w $h
    }
    return $result
}

#
# isconnected --
#
#       Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::NanovisViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::NanovisViewer::disconnect {} {
    Disconnect
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::NanovisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _serverVols
}

#
# sendto --
#
itcl::body Rappture::NanovisViewer::sendto { bytes } {
    SendBytes "$bytes\n"
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::NanovisViewer::SendCmd {string} {
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
# USAGE: SendTransferFuncs
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SendTransferFuncs {} {
    if { $_first == "" } {
	puts stderr "first not set"
	return
    }
    # Insure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update
    # the values in the _settings varible.
    set opacity [expr { double($_settings($this-opacity)) * 0.01 }]
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($_settings($this-thickness)) * 0.0001}]

    foreach vol [CurrentVolumes] {
	set tf $_vol2style($vol)
	if { ![info exists _activeTfs($tf)] || !$_activeTfs($tf) } {
	    # Only update the transfer function as necessary
	    set _settings($this-$tf-opacity) $opacity
	    set _settings($this-$tf-thickness) $thickness
	    ComputeTransferFunc $tf
	    set _activeTfs($tf) 1
	}
	if { [info exists _serverVols($vol)] && $_serverVols($vol) } {
	    SendCmd "volume shading transfunc $tf $vol"
	}
    }
    FixLegend
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::ReceiveImage { args } {
    array set info {
	-token "???"
	-bytes 0
	-type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    ReceiveEcho <<line "<read $info(-bytes) bytes"
    if { $info(-type) == "image" } {
	ReceiveEcho "for [image width $_image(plot)]x[image height $_image(plot)] image>"	
	$_image(plot) configure -data $bytes
    } elseif { $info(type) == "print" } {
	set tag $this-print-$info(-token)
	set _hardcopy($tag) $bytes
    }
}

#
# ReceiveLegend --
#
#       The procedure is the response from the render server to each "legend"
#       command.  The server sends back a "legend" command invoked our
#       the slave interpreter.  The purpose is to collect data of the image 
#       representing the legend in the canvas.  In addition, the isomarkers
#       of the active transfer function are displayed.
#
#       I don't know is this is the right place to display the isomarkers.
#       I don't know all the different paths used to draw the plot. There's 
#       "Rebuild", "add", etc.
#
itcl::body Rappture::NanovisViewer::ReceiveLegend { tf vmin vmax size } {
    if { ![isconnected] } {
	return
    }
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

    set c $itk_component(legend)
    set w [winfo width $c]
    set h [winfo height $c]
    set lx 10
    set ly [expr {$h - 1}]
    if {"" == [$c find withtag transfunc]} {
	$c create image 10 10 -anchor nw \
	    -image $_image(legend) -tags transfunc
	$c create text $lx $ly -anchor sw \
	    -fill $itk_option(-plotforeground) -tags "limits vmin"
	$c create text [expr {$w-$lx}] $ly -anchor se \
	    -fill $itk_option(-plotforeground) -tags "limits vmax"
	$c lower transfunc
	$c bind transfunc <ButtonRelease-1> \
	    [itcl::code $this AddIsoMarker %x %y]
    }
    # Display the markers used by the active transfer function.

    array set limits [limits $tf]
    $c itemconfigure vmin -text [format %.2g $limits(min)]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %.2g $limits(max)]
    $c coords vmax [expr {$w-$lx}] $ly

    if { [info exists _isomarkers($tf)] } {
	foreach m $_isomarkers($tf) {
	    $m visible yes
	}
    }
}

#
# ReceiveData --
#
#       The procedure is the response from the render server to each "data
#       follows" command.  The server sends back a "data" command invoked our
#       the slave interpreter.  The purpose is to collect the min/max of the
#       volume sent to the render server.  Since the client (nanovisviewer)
#       doesn't parse 3D data formats, we rely on the server (nanovis) to
#       tell us what the limits are.  Once we've received the limits to all
#       the data we've sent (tracked by _recvdVols) we can then determine
#       what the transfer functions are for these volumes.
#
#
#       Note: There is a considerable tradeoff in having the server report
#             back what the data limits are.  It means that much of the code
#             having to do with transfer-functions has to wait for the data
#             to come back, since the isomarkers are calculated based upon
#             the data limits.  The client code is much messier because of
#             this.  The alternative is to parse any of the 3D formats on the
#             client side.
#
itcl::body Rappture::NanovisViewer::ReceiveData { args } {
    if { ![isconnected] } {
	return
    }
    # Arguments from server are name value pairs. Stuff them in an array.
    array set info $args

    set tag $info(tag)
    set parts [split $tag -]

    #
    # Volumes don't exist until we're told about them.
    #
    set dataobj [lindex $parts 0]
    set _serverVols($tag) 1
    if { $_settings($this-volume) && $dataobj == $_first } {
	SendCmd "volume state 1 $tag"
    }
    set _limits($tag-min) $info(min);  # Minimum value of the volume.
    set _limits($tag-max) $info(max);  # Maximum value of the volume.
    set _limits(vmin)      $info(vmin); # Overall minimum value.
    set _limits(vmax)      $info(vmax); # Overall maximum value.

    unset _recvdVols($tag)
    if { [array size _recvdVols] == 0 } {
	# The active transfer function is by default the first component of
	# the first data object.  This assumes that the data is always
	# successfully transferred.
	updatetransferfuncs
    }
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::Rebuild {} {

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names _isomarkers] {
	foreach m $_isomarkers($tf) {
	    $m visible no
	}
    }

    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    EventuallyResize $w $h

    foreach dataobj [get] {
	foreach comp [$dataobj components] {
	    set vol $dataobj-$comp
	    if { ![info exists _serverVols($vol)] } {
		# Send the data as one huge base64-encoded mess -- yuck!
		set data [$dataobj values $comp]
		set nbytes [string length $data]
		append _outbuf "volume data follows $nbytes $vol\n"
		append _outbuf $data
		set _recvdVols($vol) 1
		set _serverVols($vol) 0
	    }
	    NameTransferFunc $dataobj $comp
	}
    }
    #
    # Reset the camera and other view parameters
    #

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
    FixSettings light
    FixSettings transp
    FixSettings isosurface
    FixSettings grid
    FixSettings axes
    FixSettings outline

    # nothing to send -- activate the proper ivol
    SendCmd "volume state 0"
    set _first [lindex [get] 0]
    if {"" != $_first} {
	set axis [$_first hints updir]
	if { "" != $axis } {
	    SendCmd "up $axis"
	}
	set location [$_first hints camera]
	if { $location != "" } {
	    array set _view $location
	}
	set vols [array names _serverVols $_first-*] 
	if { $vols != "" } {
	    SendCmd "volume state 1 $vols"
	}
    }
    # If the first volume already exists on the server, then make sure we
    # display the proper transfer function in the legend.
    set comp [lindex [$_first components] 0]
    if { [info exists _serverVols($_first-$comp)] } {
	updatetransferfuncs
    }

    # Sync the state of slicers
    set vols [CurrentVolumes -cutplanes]
    foreach axis {x y z} {
	SendCmd "cutplane state $_settings($this-${axis}cutplane) $axis $vols"
	set pos [expr {0.01*$_settings($this-${axis}cutposition)}]
	SendCmd "cutplane position $pos $axis $vols"
    }
    SendCmd "volume data state $_settings($this-volume) $vols"
    set _buffering 0;			# Turn off buffering.
    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf;			
    blt::busy release $itk_component(hull)
    set _outbuf "";			# Clear the buffer.		
}

# ----------------------------------------------------------------------
# USAGE: CurrentVolumes ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::CurrentVolumes {{what -all}} {
    set rlist ""
    if { $_first == "" } {
	return
    }
    foreach comp [$_first components] {
	set vol $_first-$comp
	if { [info exists _serverVols($vol)] && $_serverVols($vol) } {
	    array set style {
		-cutplanes 1
	    }
	    array set style [lindex [$_first components -style $comp] 0]
	    if {$what != "-cutplanes" || $style(-cutplanes)} {
		lappend rlist $vol
	    }
	}
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: Zoom in
# USAGE: Zoom out
# USAGE: Zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::Zoom {option} {
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
	    set _settings($this-phi)   $_view(phi)
	    set _settings($this-psi)   $_view(psi)
	    set _settings($this-pan-x) $_view(pan-x)
	    set _settings($this-pan-y) $_view(pan-y)
	    set _settings($this-zoom)  $_view(zoom)
        }
    }
    SendCmd "camera zoom $_view(zoom)"
}

itcl::body Rappture::NanovisViewer::PanCamera {} {
    #set x [expr ($_view(pan-x)) / $_limits(xrange)]
    #set y [expr ($_view(pan-y)) / $_limits(yrange)]
    set x $_view(pan-x)
    set y $_view(pan-y)
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
itcl::body Rappture::NanovisViewer::Rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
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
                set xyz [Euler2XYZ $theta $phi $psi]
		set _settings($this-theta) $_view(theta)
		set _settings($this-phi)   $_view(phi)
		set _settings($this-psi)   $_view(psi)
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
# USAGE: $this Pan click x y
#        $this Pan drag x y
#        $this Pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::Pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
        set x [expr $x / double($w)]
        set y [expr $y / double($h)]
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
        set dx [expr ($_click(x) - $x)/double($w)]
        set dy [expr ($_click(y) - $y)/double($h)]
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

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::FixSettings {what {value ""}} {
    switch -- $what {
        light {
            if {[isconnected]} {
                set val $_settings($this-light)
                set sval [expr {0.1*$val}]
                SendCmd "volume shading diffuse $sval"
                set sval [expr {sqrt($val+1.0)}]
                SendCmd "volume shading specular $sval"
            }
        }
        transp {
            if {[isconnected]} {
                set val $_settings($this-transp)
                set sval [expr {0.2*$val+1}]
                SendCmd "volume shading opacity $sval"
            }
        }
        opacity {
            if {[isconnected] && [array size _activeTfs] > 0 } {
                set val $_settings($this-opacity)
                set sval [expr { 0.01 * double($val) }]
		foreach tf [array names _activeTfs] {
		    set _settings($this-$tf-opacity) $sval
		    set _activeTfs($tf) 0
		}
                updatetransferfuncs
            }
        }

        thickness {
            if {[isconnected] && [array names _activeTfs] > 0 } {
                set val $_settings($this-thickness)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
		foreach tf [array names _activeTfs] {
		    set _settings($this-$tf-thickness) $sval
		    set _activeTfs($tf) 0
		}
                updatetransferfuncs
            }
        }
        "outline" {
            if {[isconnected]} {
                SendCmd "volume outline state $_settings($this-outline)"
            }
        }
        "isosurface" {
            if {[isconnected]} {
                SendCmd "volume shading isosurface $_settings($this-isosurface)"
            }
        }
        "grid" {
            if { [isconnected] } {
                SendCmd "grid visible $_settings($this-grid)"
            }
        }
        "axes" {
            if { [isconnected] } {
                SendCmd "axis visible $_settings($this-axes)"
            }
        }
	"legend" {
	    if { $_settings($this-legend) } {
		blt::table $itk_component(plotarea) \
		    0,0 $itk_component(3dview) -fill both \
		    1,0 $itk_component(legend) -fill x 
		blt::table configure $itk_component(plotarea) r1 -resize none
	    } else {
		blt::table forget $itk_component(legend)
	    }
	}
        "volume" {
            if { [isconnected] } {
		set vols [CurrentVolumes -cutplanes] 
                SendCmd "volume data state $_settings($this-volume) $vols"
	    }
        }
        "xcutplane" - "ycutplane" - "zcutplane" {
	    set axis [string range $what 0 0]
	    set bool $_settings($this-$what)
            if { [isconnected] } {
		set vols [CurrentVolumes -cutplanes] 
		SendCmd "cutplane state $bool $axis $vols"
	    }
	    if { $bool } {
		$itk_component(${axis}CutScale) configure -state normal \
		    -troughcolor white
            } else {
		$itk_component(${axis}CutScale) configure -state disabled \
		    -troughcolor grey82
            }
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: FixLegend
#
# Used internally to update the legend area whenever it changes size
# or when the field changes.  Asks the server to send a new legend
# for the current field.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::FixLegend {} {
    set _resizeLegendPending 0
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {$_width-20}]
    set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]
    if {$w > 0 && $h > 0 && [array names _activeTfs] > 0 && $_first != "" } {
	set vol [lindex [CurrentVolumes] 0]
	if { [info exists _vol2style($vol)] } {
	    SendCmd "legend $_vol2style($vol) $w $h"
	}
    } else {
	# Can't do this as this will remove the items associated with the
	# isomarkers.
	
	#$itk_component(legend) delete all
    }
}

#
# NameTransferFunc --
#
#       Creates a transfer function name based on the <style> settings in the
#       library run.xml file. This placeholder will be used later to create
#       and send the actual transfer function once the data info has been sent
#       to us by the render server. [We won't know the volume limits until the
#       server parses the 3D data and sends back the limits via ReceiveData.]
#
#       FIXME: The current way we generate transfer-function names completely
#              ignores the -markers option.  The problem is that we are forced
#              to compute the name from an increasing complex set of values:
#              color, levels, marker, opacity.  I think we're stuck doing it
#              now.
#
itcl::body Rappture::NanovisViewer::NameTransferFunc { dataobj comp } {
    array set style {
	-color rainbow
	-levels 6
	-opacity 1.0
    }
    array set style [lindex [$dataobj components -style $comp] 0]
    set tf "$style(-color):$style(-levels):$style(-opacity)"
    set _vol2style($dataobj-$comp) $tf
    lappend _style2vols($tf) $dataobj-$comp
    return $tf
}

#
# ComputeTransferFunc --
#
#   Computes and sends the transfer function to the render server.  It's
#   assumed that the volume data limits are known and that the global
#   transfer-functions slider values have be setup.  Both parts are
#   needed to compute the relative value (location) of the marker, and
#   the alpha map of the transfer function.
#
itcl::body Rappture::NanovisViewer::ComputeTransferFunc { tf } {
    array set style {
	-color rainbow
	-levels 6
	-opacity 1.0
    }
    foreach {dataobj comp} [split $_style2vols($tf) -] break
    array set style [lindex [$dataobj components -style $comp] 0]


    # We have to parse the style attributes for a volume using this
    # transfer-function *once*.  This sets up the initial isomarkers for the
    # transfer function.  The user may add/delete markers, so we have to
    # maintain a list of markers for each transfer-function.  We use the one
    # of the volumes (the first in the list) using the transfer-function as a
    # reference.
    #
    # FIXME: The current way we generate transfer-function names completely 
    #        ignores the -markers option.  The problem is that we are forced 
    #        to compute the name from an increasing complex set of values: 
    #        color, levels, marker, opacity.  I think the cow's out of the
    #        barn on this one.

    if { ![info exists _isomarkers($tf)] } {
	# Have to defer creation of isomarkers until we have data limits
	if { [info exists style(-markers)] } {
	    ParseMarkersOption $tf $style(-markers)
	} else {
	    ParseLevelsOption $tf $style(-levels)
	}
    }
    if {$style(-color) == "rainbow"} {
	set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    set clist [split $style(-color) :]
    set cmap "0.0 [Color2RGB white] "
    for {set i 0} {$i < [llength $clist]} {incr i} {
	set x [expr {double($i+1)/([llength $clist]+1)}]
	set color [lindex $clist $i]
	append cmap "$x [Color2RGB $color] "
    }
    append cmap "1.0 [Color2RGB $color]"

    set tag $this-$tf
    if { ![info exists _settings($tag-opacity)] } {
	set _settings($tag-opacity) $style(-opacity)
    }
    set max $_settings($tag-opacity)

    set isovalues {}
    foreach m $_isomarkers($tf) {
	lappend isovalues [$m relval]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists _settings($tag-thickness)]} {
	set _settings($tag-thickness) 0.05
    }
    set delta $_settings($tag-thickness)

    set first [lindex $isovalues 0]
    set last [lindex $isovalues end]
    set wmap ""
    if { $first == "" || $first != 0.0 } {
	lappend wmap 0.0 0.0
    }
    foreach x $isovalues {
	set x1 [expr {$x-$delta-0.00001}]
	set x2 [expr {$x-$delta}]
	set x3 [expr {$x+$delta}]
	set x4 [expr {$x+$delta+0.00001}]
	if { $x1 < 0.0 } {
	    set x1 0.0
	} elseif { $x1 > 1.0 } {
	    set x1 1.0
	}
	if { $x2 < 0.0 } {
	    set x2 0.0
	} elseif { $x2 > 1.0 } {
	    set x2 1.0
	}
	if { $x3 < 0.0 } {
	    set x3 0.0
	} elseif { $x3 > 1.0 } {
	    set x3 1.0
	}
	if { $x4 < 0.0 } {
	    set x4 0.0
	} elseif { $x4 > 1.0 } {
	    set x4 1.0
	}
	# add spikes in the middle
	lappend wmap $x1 0.0
	lappend wmap $x2 $max
	lappend wmap $x3 $max
	lappend wmap $x4 0.0
    }
    if { $last == "" || $last != 1.0 } {
	lappend wmap 1.0 0.0
    }
    SendCmd "transfunc define $tf { $cmap } { $wmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotbackground {
    if { [isconnected] } {
	foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
	#fix this!
	#SendCmd "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotforeground {
    if { [isconnected] } {
	foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
	#fix this!
	#SendCmd "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotoutline {
    # Must check if we are connected because this routine is called from the
    # class body when the -plotoutline itk_option is defined.  At that point
    # the NanovisViewer class constructor hasn't been called, so we can't
    # start sending commands to visualization server.
    if { [isconnected] } {
	if {"" == $itk_option(-plotoutline)} {
	    SendCmd "volume outline state off"
	} else {
	    SendCmd "volume outline state on"
	    SendCmd "volume outline color [Color2RGB $itk_option(-plotoutline)]"
	}
    }
}

#
# The -levels option takes a single value that represents the number
# of evenly distributed markers based on the current data range. Each
# marker is a relative value from 0.0 to 1.0.
#
itcl::body Rappture::NanovisViewer::ParseLevelsOption { tf levels } {
    set c $itk_component(legend)
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
	for {set i 1} { $i <= $levels } {incr i} {
	    set x [expr {double($i)/($levels+1)}]
	    set m [Rappture::IsoMarker \#auto $c $this $tf]
	    $m relval $x
	    lappend _isomarkers($tf) $m 
	}
    } else {
	foreach x $levels {
	    set m [Rappture::IsoMarker \#auto $c $this $tf]
	    $m relval $x
	    lappend _isomarkers($tf) $m 
	}
    }
}

#
# The -markers option takes a list of zero or more values (the values
# may be separated either by spaces or commas) that have the following 
# format:
#
#   N%  Percent of current total data range.  Converted to
#       to a relative value between 0.0 and 1.0.
#   N   Absolute value of marker.  If the marker is outside of
#       the current range, it will be displayed on the outer
#       edge of the legends, but it range it represents will
#       not be seen.
#
itcl::body Rappture::NanovisViewer::ParseMarkersOption { tf markers } {
    set c $itk_component(legend)
    regsub -all "," $markers " " markers
    foreach marker $markers {
	set n [scan $marker "%g%s" value suffix]
	if { $n == 2 && $suffix == "%" } {
	    # ${n}% : Set relative value. 
	    set value [expr {$value * 0.01}]
	    set m [Rappture::IsoMarker \#auto $c $this $tf]
	    $m relval $value
	    lappend _isomarkers($tf) $m
	} else {
	    # ${n} : Set absolute value.
	    set m [Rappture::IsoMarker \#auto $c $this $tf]
	    $m absval $value
	    lappend _isomarkers($tf) $m
	}
    }
}

# ----------------------------------------------------------------------
# USAGE: UndateTransferFuncs 
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::updatetransferfuncs {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::AddIsoMarker { x y } {
    if { $_first == "" } {
	error "active transfer function isn't set"
    }
    set vol [lindex [CurrentVolumes] 0]
    set tf $_vol2style($vol)
    set c $itk_component(legend)
    set m [Rappture::IsoMarker \#auto $c $this $tf]
    set w [winfo width $c]
    $m relval [expr {double($x-10)/($w-20)}]
    lappend _isomarkers($tf) $m
    updatetransferfuncs
    return 1
}

itcl::body Rappture::NanovisViewer::rmdupmarker { marker x } {
    set tf [$marker transferfunc]
    set bool 0
    if { [info exists _isomarkers($tf)] } {
	set list {}
	set marker [namespace tail $marker]
	foreach m $_isomarkers($tf) {
	    set sx [$m screenpos]
	    if { $m != $marker } {
		if { $x >= ($sx-3) && $x <= ($sx+3) } {
		    $marker relval [$m relval]
		    itcl::delete object $m
		    bell
		    set bool 1
		    continue
		}
	    }
	    lappend list $m
	}
	set _isomarkers($tf) $list
	updatetransferfuncs
    }
    return $bool
}

itcl::body Rappture::NanovisViewer::overmarker { marker x } {
    set tf [$marker transferfunc]
    if { [info exists _isomarkers($tf)] } {
	set marker [namespace tail $marker]
	foreach m $_isomarkers($tf) {
	    set sx [$m screenpos]
	    if { $m != $marker } {
		set bool [expr { $x >= ($sx-3) && $x <= ($sx+3) }]
		$m activate $bool
	    }
	}
    }
    return ""
}

itcl::body Rappture::NanovisViewer::limits { tf } {
    set _limits(min) 0.0
    set _limits(max) 1.0
    if { ![info exists _style2vols($tf)] } {
	return [array get _limits]
    }
    set min ""; set max ""
    foreach vol $_style2vols($tf) {
	if { ![info exists _serverVols($vol)] } {
	    continue
	}
	if { ![info exists _limits($vol-min)] } {
	    continue
	}
	if { $min == "" || $min > $_limits($vol-min) } {
	    set min $_limits($vol-min)
	}
	if { $max == "" || $max < $_limits($vol-max) } {
	    set max $_limits($vol-max)
	}
    }
    if { $min != "" } {
	set _limits(min) $min
    } 
    if { $max != "" } {
	set _limits(max) $max
    }
    return [array get _limits]
}


itcl::body Rappture::NanovisViewer::BuildViewTab {} {
    foreach { key value } {
	grid		0
	axes		1
	outline		1
	volume		1
	legend		1
	particles	1
	lic		1
    } {
	set _settings($this-$key) $value
    }

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    set ::Rappture::NanovisViewer::_settings($this-isosurface) 0
    checkbutton $inner.isosurface \
        -text "Isosurface shading" \
        -variable [itcl::scope _settings($this-isosurface)] \
        -command [itcl::code $this FixSettings isosurface] \
	-font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
	-font "Arial 9"

    checkbutton $inner.grid \
        -text "Grid" \
        -variable [itcl::scope _settings($this-grid)] \
        -command [itcl::code $this FixSettings grid] \
	-font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings($this-outline)] \
        -command [itcl::code $this FixSettings outline] \
	-font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings($this-legend)] \
        -command [itcl::code $this FixSettings legend] \
	-font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume] \
	-font "Arial 9"

    blt::table $inner \
	0,0 $inner.axes  -columnspan 2 -anchor w \
	1,0 $inner.grid  -columnspan 2 -anchor w \
	2,0 $inner.outline  -columnspan 2 -anchor w \
	3,0 $inner.volume  -columnspan 2 -anchor w \
	4,0 $inner.legend  -columnspan 2 -anchor w 

    if 0 {
    bind $inner <Map> [itcl::code $this GetVolumeInfo $inner]
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r5 -resize expand
}

itcl::body Rappture::NanovisViewer::BuildVolumeTab {} {
    foreach { key value } {
	light		40
	transp		50
	opacity		100
	thickness	350
    } {
	set _settings($this-$key) $value
    }

    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    checkbutton $inner.vol -text "Show volume" -font $fg \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume]
    label $inner.shading -text "Shading:" -font $fg

    label $inner.dim -text "Dim" -font $fg
    ::scale $inner.light -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-light)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings light]
    label $inner.bright -text "Bright" -font $fg

    label $inner.fog -text "Fog" -font $fg
    ::scale $inner.transp -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-transp)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings transp]
    label $inner.plastic -text "Plastic" -font $fg

    label $inner.clear -text "Clear" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-opacity)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings opacity]
    label $inner.opaque -text "Opaque" -font $fg

    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings($this-thickness)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings thickness]
    label $inner.thick -text "Thick" -font $fg

    blt::table $inner \
	0,0 $inner.vol -columnspan 4 -anchor w -pady 2 \
	1,0 $inner.shading -columnspan 4 -anchor w -pady {10 2} \
	2,0 $inner.dim -anchor e -pady 2 \
	2,1 $inner.light -columnspan 2 -pady 2 -fill x \
	2,3 $inner.bright -anchor w -pady 2 \
	3,0 $inner.fog -anchor e -pady 2 \
	3,1 $inner.transp -columnspan 2 -pady 2 -fill x \
	3,3 $inner.plastic -anchor w -pady 2 \
	4,0 $inner.clear -anchor e -pady 2 \
	4,1 $inner.opacity -columnspan 2 -pady 2 -fill x\
	4,3 $inner.opaque -anchor w -pady 2 \
	5,0 $inner.thin -anchor e -pady 2 \
	5,1 $inner.thickness -columnspan 2 -pady 2 -fill x\
	5,3 $inner.thick -anchor w -pady 2

    blt::table configure $inner c0 c1 c3 r* -resize none
    blt::table configure $inner r6 -resize expand
}

itcl::body Rappture::NanovisViewer::BuildCutplanesTab {} {
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]
    $inner configure -borderwidth 4

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
	    -onimage [Rappture::icon x-cutplane] \
	    -offimage [Rappture::icon x-cutplane] \
	    -command [itcl::code $this FixSettings xcutplane] \
	    -variable [itcl::scope _settings($this-xcutplane)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X cut plane on/off"

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x] \
	    -variable [itcl::scope _settings($this-xcutposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    # Set the default cutplane value before disabling the scale.
    $itk_component(xCutScale) set 50
    $itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this SlicerTip x]"

    # Y-value slicer...
    itk_component add yCutButton {
        Rappture::PushButton $inner.ybutton \
	    -onimage [Rappture::icon y-cutplane] \
	    -offimage [Rappture::icon y-cutplane] \
	    -command [itcl::code $this FixSettings ycutplane] \
	    -variable [itcl::scope _settings($this-ycutplane)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y cut plane on/off"

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y] \
	    -variable [itcl::scope _settings($this-ycutposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this SlicerTip y]"
    # Set the default cutplane value before disabling the scale.
    $itk_component(yCutScale) set 50
    $itk_component(yCutScale) configure -state disabled

    # Z-value slicer...
    itk_component add zCutButton {
        Rappture::PushButton $inner.zbutton \
	    -onimage [Rappture::icon z-cutplane] \
	    -offimage [Rappture::icon z-cutplane] \
	    -command [itcl::code $this FixSettings zcutplane] \
	    -variable [itcl::scope _settings($this-zcutplane)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z cut plane on/off"

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z] \
	    -variable [itcl::scope _settings($this-zcutposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    $itk_component(zCutScale) set 50
    $itk_component(zCutScale) configure -state disabled
    #$itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this SlicerTip z]"

    blt::table $inner \
	1,1 $itk_component(xCutButton) \
	1,2 $itk_component(yCutButton) \
	1,3 $itk_component(zCutButton) \
	0,1 $itk_component(xCutScale) \
	0,2 $itk_component(yCutScale) \
	0,3 $itk_component(zCutScale) \

    blt::table configure $inner r0 r1 c* -resize none
    blt::table configure $inner r2 c4 -resize expand
    blt::table configure $inner c0 -width 2
    blt::table configure $inner c1 c2 c3 -padx 2
}

itcl::body Rappture::NanovisViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set labels { phi theta psi pan-x pan-y zoom }
    set row 0
    foreach tag $labels {
	label $inner.${tag}label -text $tag -font "Arial 9"
	entry $inner.${tag} -font "Arial 9"  -bg white \
	    -textvariable [itcl::scope _settings($this-$tag)]
	bind $inner.${tag} <KeyPress-Return> \
	    [itcl::code $this camera set ${tag}]
	blt::table $inner \
	    $row,0 $inner.${tag}label -anchor e -pady 2 \
	    $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
	incr row
    }
    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}


# ----------------------------------------------------------------------
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::Slice {option args} {
    switch -- $option {
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set newpos [expr {0.01*$newval}]
            set vols [CurrentVolumes -cutplanes]
            SendCmd "cutplane position $newpos $axis $vols"
        }
        default {
            error "bad option \"$option\": should be axis, move, or volume"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: SlicerTip <axis>
#
# Used internally to generate a tooltip for the x/y/z slicer controls.
# Returns a message that includes the current slicer value.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SlicerTip {axis} {
    set val [$itk_component(${axis}CutScale) get]
#    set val [expr {0.01*($val-50)
#        *($_limits(${axis}max)-$_limits(${axis}min))
#          + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}


itcl::body Rappture::NanovisViewer::DoResize {} {
    SendCmd "screen $_width $_height"
    set _resizePending 0
}

itcl::body Rappture::NanovisViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
	$_dispatcher event -idle !resize
	set _resizePending 1
    }
}

itcl::body Rappture::NanovisViewer::EventuallyResizeLegend {} {
    if { !$_resizeLegendPending } {
	$_dispatcher event -idle !legend
	set _resizeLegendPending 1
    }
}


#  camera -- 
#
itcl::body Rappture::NanovisViewer::camera {option args} {
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

itcl::body Rappture::NanovisViewer::GetVolumeInfo { w } {
    set flowobj ""
    foreach key [array names _obj2flow] {
	set flowobj $_obj2flow($key)
	break
    }
    if { $flowobj == "" } {
	return
    }
    if { [winfo exists $w.frame] } {
	destroy $w.frame
    }
    set inner [frame $w.frame]
    blt::table $w \
	5,0 $inner -fill both -columnspan 2 -anchor nw
    array set hints [$dataobj hints]

    label $inner.volumes -text "Volumes" -font "Arial 9 bold"
    blt::table $inner \
	1,0 $inner.volumes  -anchor w \
    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand

    set row 3
    set volumes [get]
    if { [llength $volumes] > 0 } {
	blt::table $inner $row,0 $inner.volumes  -anchor w
	incr row
    }
    foreach vol $volumes {
	array unset info
	array set info $vol
	set name $info(name)
	if { ![info exists _settings($this-volume-$name)] } {
	    set _settings($this-volume-$name) $info(hide)
	}
	checkbutton $inner.vol$row -text $info(label) \
	    -variable [itcl::scope _settings($this-volume-$name)] \
	    -onvalue 0 -offvalue 1 \
	    -command [itcl::code $this volume $key $name] \
	    -font "Arial 9"
	Rappture::Tooltip::for $inner.vol$row $info(description)
	blt::table $inner $row,0 $inner.vol$row -anchor w 
	if { !$_settings($this-volume-$name) } {
	    $inner.vol$row select
	} 
	incr row
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r$row -resize expand
    blt::table configure $inner c3 -resize expand
    event generate [winfo parent [winfo parent $w]] <Configure> 
}

itcl::body Rappture::NanovisViewer::volume { tag name } {
    set bool $_settings($this-volume-$name)
    SendCmd "volume statue $bool $name"
}

