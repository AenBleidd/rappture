
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
    public method get {args}
    public method delete {args}
    public method scale {args}
    public method download {option args}
    public method parameters {title args} {
	# do nothing
    }
    public method limits { tf }
    public method isconnected {}
    public method updatetransferfuncs {}
    public method rmdupmarker { m x }
    public method overmarker { m x }

    protected method Connect {}
    protected method Disconnect {}

    protected method SendCmd {string}
    protected method SendDataObjs {}
    protected method SendTransferFuncs {}

    protected method ReceiveImage { args }
    protected method ReceiveLegend { ivol vmin vmax size }
    protected method ReceiveData { args }

    protected method Rebuild {}
    protected method CurrentVolumeIds {{what -all}}
    protected method Zoom {option}
    protected method Pan {option x y}
    protected method Rotate {option x y}
    protected method Probe {option args}
    protected method Marker {index option args}
    protected method Slice {option args}
    protected method SlicerTip {axis}

    protected method State {comp}
    protected method FixLegend {}
    protected method FixSettings {what {value ""}}

    # The following methods are only used by this class.
    private method NameTransferFunc { ivol }
    private method ComputeTransferFunc { tf }
    private method AddIsoMarker { x y }
    private method ParseMarkersOption { tf ivol markers }
    private method ParseLevelsOption { tf ivol levels }

    private method BuildCutplanesTab {}
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    private method BuildCameraTab {}
    private method PanCamera {}


    private variable outbuf_       ;# buffer for outgoing commands

    private variable dlist_ ""     ;# list of data objects
    private variable allDataObjs_
    private variable id2style_     ;# maps id => style settings
    private variable obj2ovride_   ;# maps dataobj => style override
    private variable obj2id_       ;# maps dataobj => volume ID in server
    private variable id2obj_       ;# maps dataobj => volume ID in server
    private variable sendobjs_ ""  ;# list of data objs to send to server
    private variable receiveIds_   ;# list of data objs to send to server
    private variable obj2styles_   ;# maps id => style settings
    private variable style2ids_    ;# maps id => style settings

    private variable click_        ;# info used for _rotate operations
    private variable limits_       ;# autoscale min/max for all axes
    private variable view_         ;# view params for 3D view
    private variable isomarkers_    ;# array of isosurface level values 0..1
    private common   settings_
    private variable activeTf_ ""  ;# The currently active transfer function.
    # This 
    # indicates which isomarkers and transfer
    # function to use when changing markers,
    # opacity, or thickness.
    #common _downloadPopup          ;# download options from popup
    private common hardcopy_
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
    # Send dataobjs event
    $_dispatcher register !send_dataobjs
    $_dispatcher dispatch $this !send_dataobjs \
	"[itcl::code $this SendDataObjs]; list"
    # Send transfer functions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
	"[itcl::code $this SendTransferFuncs]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    set outbuf_ ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]
    #$_parser alias print [itcl::code $this ReceivePrint]

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
    set id2obj_(count) 0
    set limits_(vmin) 0.0
    set limits_(vmax) 1.0

    set settings_($this-theta) $view_(theta)
    set settings_($this-phi)   $view_(phi)
    set settings_($this-psi)   $view_(psi)
    set settings_($this-pan-x) $view_(pan-x)
    set settings_($this-pan-y) $view_(pan-y)
    set settings_($this-zoom)  $view_(zoom)

    set f [$itk_component(main) component controls]
    itk_component add reset {
        button $f.reset -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this Zoom reset]
    }
    pack $itk_component(reset) -side top -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $f.zin -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this Zoom in]
    }
    pack $itk_component(zoomin) -side top -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $f.zout -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-out] \
            -command [itcl::code $this Zoom out]
    }
    pack $itk_component(zoomout) -side top -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    #
    # Volume toggle...
    #
    itk_component add volume {
        label $f.volume -borderwidth 1 -relief sunken -padx 1 -pady 1 \
            -text "Volume" \
	    -image [Rappture::icon playback-record]
    }
    bind $itk_component(volume) <ButtonPress> \
        [itcl::code $this Slice volume toggle]
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
    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w [expr [winfo reqwidth $itk_component(hull)] - 80]
    pack forget $itk_component(3dview)
    blt::table $itk_component(plotarea) \
	0,0 $itk_component(3dview) -fill both \
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
	[itcl::code $this SendCmd "screen %w %h"]

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
    set sendobjs_ ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    $_dispatcher cancel !send_transfunc
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
    array unset settings_ $this-*
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

    set pos [lsearch -exact $dataobj $dlist_]
    if {$pos < 0} {
	lappend dlist_ $dataobj
	set allDataObjs_($dataobj) 1
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
itcl::body Rappture::NanovisViewer::get {args} {
    if {[llength $args] == 0} {
	set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
      -objects {
	# put the dataobj list in order according to -raise options
	set dlist $dlist_
	foreach obj $dlist {
	    if {[info exists obj2ovride_($obj-raise)] && $obj2ovride_($obj-raise)} {
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
	set args $dlist_
    }
    # Delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
	set pos [lsearch -exact $dlist_ $dataobj]
	if { $pos >= 0 } {
	    set dlist_ [lreplace $dlist_ $pos $pos]
	    foreach key [array names obj2ovride_ $dataobj-*] {
		unset obj2ovride_($key)
	    }
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
	set limits_($val) ""
    }
    foreach obj $args {
	foreach axis {x y z v} {

	    foreach { min max } [$obj limits $axis] break

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
itcl::body Rappture::NanovisViewer::download {option args} {
    switch $option {
	coming {
	    if {[catch {blt::winop snap $itk_component(plotarea) $_image(download)}]} {
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
	    return [list .jpg $data]
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
	SendCmd "screen $w $h"
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
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::NanovisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set outbuf_ ""
    catch {unset obj2id_}
    array unset id2obj_
    set obj2id_(count) 0
    set id2obj_(count) 0
    set sendobjs_ ""
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::NanovisViewer::SendCmd {string} {
    if {[llength $sendobjs_] > 0} {
	append outbuf_ $string "\n"
    } else {
	foreach line [split $string \n] {
	    SendEcho >>line $line
	}
	SendBytes $string
    }
}

# ----------------------------------------------------------------------
# USAGE: SendDataObjs
#
# Used internally to send a series of volume objects off to the
# server.  Sends each object, a little at a time, with updates in
# between so the interface doesn't lock up.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SendDataObjs {} {
    blt::busy hold $itk_component(hull); update idletasks
    foreach dataobj $sendobjs_ {
	foreach comp [$dataobj components] {
	    # send the data as one huge base64-encoded mess -- yuck!
	    set data [$dataobj values $comp]
	    set nbytes [string length $data]
	    if { ![SendBytes "volume data follows $nbytes"] } {
		return
	    }
	    if { ![SendBytes $data] } {
		return
	    }
	    set ivol $obj2id_(count)
	    incr obj2id_(count)

	    set id2obj_($ivol) [list $dataobj $comp]
	    set obj2id_($dataobj-$comp) $ivol
	    NameTransferFunc $ivol
	    set receiveIds_($ivol) 1
	}
    }
    set sendobjs_ ""
    blt::busy release $itk_component(hull)

    # activate the proper volume
    set first [lindex [get] 0]
    if {"" != $first} {
	set axis [$first hints updir]
	if {"" != $axis} {
	    SendCmd "up $axis"
	}
	# The active transfer function is by default the first component of
	# the first data object.  This assumes that the data is always
	# successfully transferred.
	set comp [lindex [$first components] 0]
	set activeTf_ $id2style_($obj2id_($first-$comp))
    }
    foreach key [array names obj2id_ *-*] {
	set state [string match $first-* $key]
	set ivol $obj2id_($key)
	SendCmd "volume state $state $ivol"
    }

    # sync the state of slicers
    set vols [CurrentVolumeIds -cutplanes]
    foreach axis {x y z} {
	SendCmd "cutplane state [State ${axis}CutButton] $axis $vols"
	set pos [expr {0.01*[$itk_component(${axis}CutScale) get]}]
	SendCmd "cutplane position $pos $axis $vols"
    }
    SendCmd "volume data state [State volume] $vols"

    if 0 {
    # Add this when we fix grid for volumes
    SendCmd "volume axis label x \"\""
    SendCmd "volume axis label y \"\""
    SendCmd "volume axis label z \"\""
    SendCmd "grid axisname x X eV"
    SendCmd "grid axisname y Y eV"
    SendCmd "grid axisname z Z eV"
    }
    # if there are any commands in the buffer, send them now that we're done
    SendBytes $outbuf_
    set outbuf_ ""
}

# ----------------------------------------------------------------------
# USAGE: SendTransferFuncs
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SendTransferFuncs {} {
    if { $activeTf_ == "" } {
	return
    }
    set tf $activeTf_
    set first [lindex [get] 0] 

    # Insure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update the
    # values in the settings_ varible.
    set value $settings_($this-opacity)
    set opacity [expr { double($value) * 0.01 }]
    set settings_($this-$tf-opacity) $opacity
    set value $settings_($this-thickness)
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($value) * 0.0001}]
    set settings_($this-$tf-thickness) $thickness

    if { ![info exists $obj2styles_($first)] } {
	foreach tf $obj2styles_($first) {
	    ComputeTransferFunc $tf
	}
	FixLegend
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
set counter 0
itcl::body Rappture::NanovisViewer::ReceiveImage { args } {
    if { ![isconnected] } {
	return
    }
    global counter
    incr counter
    array set info {
	-token "???"
	-bytes 0
	-type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    ReceiveEcho <<line "<read $info(-bytes) bytes for [image width $_image(plot)]x[image height $_image(plot)] image>"
    if { $info(-type) == "image" } {
	$_image(plot) configure -data $bytes
    } elseif { $info(type) == "print" } {
	set tag $this-print-$info(-token)
	set hardcopy_($tag) $bytes
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
    #foreach { dataobj comp } $id2obj_($ivol) break
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
    #set tf $activeTf_

    array set limits [limits $tf]
    $c itemconfigure vmin -text [format %.2g $limits(min)]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %.2g $limits(max)]
    $c coords vmax [expr {$w-$lx}] $ly

    if { [info exists isomarkers_($tf)] } {
	foreach m $isomarkers_($tf) {
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
#       the data we've sent (tracked by receiveIds_) we can then determine
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

    set ivol $info(id);         # Id of volume created by server.

    set limits_($ivol-min) $info(min);  # Minimum value of the volume.
    set limits_($ivol-max) $info(max);  # Maximum value of the volume.
    set limits_(vmin)      $info(vmin); # Overall minimum value.
    set limits_(vmax)      $info(vmax); # Overall maximum value.

    unset receiveIds_($ivol)
    if { [array size receiveIds_] == 0 } {
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
    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names isomarkers_] {
	foreach m $isomarkers_($tf) {
	    $m visible no
	}
    }

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
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    SendCmd "screen $w $h"

    #
    # Reset the camera and other view parameters
    #
    set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
    SendCmd "camera angle $xyz"
    SendCmd "camera zoom $view_(zoom)"

    set settings_($this-theta) $view_(theta)
    set settings_($this-phi)   $view_(phi)
    set settings_($this-psi)   $view_(psi)
    set settings_($this-pan-x) $view_(pan-x)
    set settings_($this-pan-y) $view_(pan-y)
    set settings_($this-zoom)  $view_(zoom)

    FixSettings light
    FixSettings transp
    FixSettings isosurface
    FixSettings grid
    FixSettings axes
    FixSettings outline

    if {[llength $sendobjs_] > 0} {
	# send off new data objects
	$_dispatcher event -idle !send_dataobjs
	return
    }

    # nothing to send -- activate the proper ivol
    set first [lindex [get] 0]
    if {"" != $first} {
	set axis [$first hints updir]
	if {"" != $axis} {
	    SendCmd "up $axis"
	}
	foreach key [array names obj2id_ *-*] {
	    set state [string match $first-* $key]
	    SendCmd "volume state $state $obj2id_($key)"
	}
	#
	# The obj2id_ and id2style_ arrays may or may not have the right
	# information.  It's possible for the server to know about volumes
	# that the client has assumed it's deleted.  We could add checks.
	# But this problem needs to be fixed not bandaided.
	set comp [lindex [$first components] 0]
	set ivol $obj2id_($first-$comp)

	foreach comp [$first components] {
	    foreach ivol $obj2id_($first-$comp) {
		NameTransferFunc $ivol
	    }
	}
    }

    # sync the state of slicers
    set vols [CurrentVolumeIds -cutplanes]
    foreach axis {x y z} {
	SendCmd "cutplane state [State ${axis}CutButton] $axis $vols"
	set pos [expr {0.01*[$itk_component(${axis}CutScale) get]}]
	SendCmd "cutplane position $pos $axis $vols"
    }
    SendCmd "volume data state [State volume] $vols"
    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: CurrentVolumeIds ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::CurrentVolumeIds {{what -all}} {
    set rlist ""

    set first [lindex [get] 0]
    foreach key [array names obj2id_ *-*] {
	if {[string match $first-* $key]} {
	    array set style {
		-cutplanes 1
	    }
	    foreach {dataobj comp} [split $key -] break
	    array set style [lindex [$dataobj components -style $comp] 0]

	    if {$what != "-cutplanes" || $style(-cutplanes)} {
		lappend rlist $obj2id_($key)
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
            SendCmd "camera angle $xyz"
	    PanCamera
	    set settings_($this-theta) $view_(theta)
	    set settings_($this-phi)   $view_(phi)
	    set settings_($this-psi)   $view_(psi)
	    set settings_($this-pan-x) $view_(pan-x)
	    set settings_($this-pan-y) $view_(pan-y)
	    set settings_($this-zoom)  $view_(zoom)
        }
    }
    SendCmd "camera zoom $view_(zoom)"
}

itcl::body Rappture::NanovisViewer::PanCamera {} {
    #set x [expr ($view_(pan-x)) / $limits_(xrange)]
    #set y [expr ($view_(pan-y)) / $limits_(yrange)]
    set x $view_(pan-x)
    set y $view_(pan-y)
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
            set click_(x) $x
            set click_(y) $y
            set click_(theta) $view_(theta)
            set click_(phi) $view_(phi)
        }
        drag {
            if {[array size click_] == 0} {
                Rotate click $x $y
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
                }]} {
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
                set xyz [Euler2XYZ $theta $phi $psi]
		set settings_($this-theta) $view_(theta)
		set settings_($this-phi)   $view_(phi)
		set settings_($this-psi)   $view_(psi)
                SendCmd "camera angle $xyz"
                set click_(x) $x
                set click_(y) $y
            }
        }
        release {
            Rotate drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset click_}
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
        set view_(pan-x) [expr $view_(pan-x) + $x]
        set view_(pan-y) [expr $view_(pan-y) + $y]
        PanCamera
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
        set dx [expr ($click_(x) - $x)/double($w)]
        set dy [expr ($click_(y) - $y)/double($h)]
        set click_(x) $x
        set click_(y) $y
        set view_(pan-x) [expr $view_(pan-x) - $dx]
        set view_(pan-y) [expr $view_(pan-y) - $dy]
        PanCamera
	set settings_($this-pan-x) $view_(pan-x)
	set settings_($this-pan-y) $view_(pan-y)
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: State <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::State { comp } {
    if {[$itk_component(${comp}) cget -relief] == "sunken"} {
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
itcl::body Rappture::NanovisViewer::FixSettings {what {value ""}} {
    switch -- $what {
        light {
            if {[isconnected]} {
                set val $settings_($this-light)
                set sval [expr {0.1*$val}]
                SendCmd "volume shading diffuse $sval"
                set sval [expr {sqrt($val+1.0)}]
                SendCmd "volume shading specular $sval"
            }
        }
        transp {
            if {[isconnected]} {
                set val $settings_($this-transp)
                set sval [expr {0.2*$val+1}]
                SendCmd "volume shading opacity $sval"
            }
        }
        opacity {
            if {[isconnected] && $activeTf_ != "" } {
                set val $settings_($this-opacity)
                set sval [expr { 0.01 * double($val) }]
                set tf $activeTf_
                set settings_($this-$tf-opacity) $sval
                updatetransferfuncs
            }
        }

        thickness {
            if {[isconnected] && $activeTf_ != "" } {
                set val $settings_($this-thickness)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                set tf $activeTf_
                set settings_($this-$tf-thickness) $sval
                updatetransferfuncs
            }
        }
        "outline" {
            if {[isconnected]} {
                SendCmd "volume outline state $settings_($this-outline)"
            }
        }
        "isosurface" {
            if {[isconnected]} {
                SendCmd "volume shading isosurface $settings_($this-isosurface)"
            }
        }
        "grid" {
            if { [isconnected] } {
                SendCmd "grid visible $settings_($this-grid)"
            }
        }
        "axes" {
            if { [isconnected] } {
                SendCmd "axis visible $settings_($this-axes)"
            }
        }
	"legend" {
	    if { $settings_($this-legend) } {
		blt::table $itk_component(plotarea) \
		    0,0 $itk_component(3dview) -fill both \
		    1,0 $itk_component(legend) -fill x 
		blt::table configure $itk_component(plotarea) r1 -resize none
	    } else {
		blt::table forget $itk_component(legend)
	    }
	}
        "volume" {
	    Slice volume $settings_($this-volume)
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
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {[image width $_image(plot)]-20}]
    set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]
    if {$w > 0 && $h > 0 && "" != $activeTf_} {
	SendCmd "legend $activeTf_ $w $h"
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
itcl::body Rappture::NanovisViewer::NameTransferFunc { ivol } {
    array set style {
	-color rainbow
	-levels 6
	-opacity 1.0
    }
    foreach {dataobj comp} $id2obj_($ivol) break
    array set style [lindex [$dataobj components -style $comp] 0]
    set tf "$style(-color):$style(-levels):$style(-opacity)"

    set id2style_($ivol) $tf
    lappend obj2styles_($dataobj) $tf
    lappend style2ids_($tf) $ivol
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
    set dataobj ""; set comp ""
    foreach ivol $style2ids_($tf) {
	if { [info exists id2obj_($ivol)] } {
	    foreach {dataobj comp} $id2obj_($ivol) break
	    break
	}
    }
    if { $dataobj == "" } {
	return 0
    }
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

    if { ![info exists isomarkers_($tf)] } {
	# Have to defer creation of isomarkers until we have data limits
	if { [info exists style(-markers)] } {
	    ParseMarkersOption $tf $ivol $style(-markers)
	} else {
	    ParseLevelsOption $tf $ivol $style(-levels)
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
    if { ![info exists settings_($tag-opacity)] } {
	set settings_($tag-opacity) $style(-opacity)
    }
    set max $settings_($tag-opacity)

    set isovalues {}
    foreach m $isomarkers_($tf) {
	lappend isovalues [$m relval]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists settings_($tag-thickness)]} {
	set settings_($tag-thickness) 0.05
    }
    set delta $settings_($tag-thickness)

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
    SendBytes "transfunc define $tf { $cmap } { $wmap }\n"
    return [SendBytes "volume shading transfunc $tf $style2ids_($tf)\n"]
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
itcl::body Rappture::NanovisViewer::ParseLevelsOption { tf ivol levels } {
    set c $itk_component(legend)
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
	for {set i 1} { $i <= $levels } {incr i} {
	    set x [expr {double($i)/($levels+1)}]
	    set m [IsoMarker \#auto $c $this $tf]
	    $m relval $x
	    lappend isomarkers_($tf) $m 
	}
    } else {
	foreach x $levels {
	    set m [IsoMarker \#auto $c $this $tf]
	    $m relval $x
	    lappend isomarkers_($tf) $m 
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
itcl::body Rappture::NanovisViewer::ParseMarkersOption { tf ivol markers } {
    set c $itk_component(legend)
    regsub -all "," $markers " " markers
    foreach marker $markers {
	set n [scan $marker "%g%s" value suffix]
	if { $n == 2 && $suffix == "%" } {
	    # ${n}% : Set relative value. 
	    set value [expr {$value * 0.01}]
	    set m [IsoMarker \#auto $c $this $tf]
	    $m relval $value
	    lappend isomarkers_($tf) $m
	} else {
	    # ${n} : Set absolute value.
	    set m [IsoMarker \#auto $c $this $tf]
	    $m absval $value
	    lappend isomarkers_($tf) $m
	}
    }
}

# ----------------------------------------------------------------------
# USAGE: Marker start <x> <y>
# USAGE: Marker update <x> <y>
# USAGE: Marker end <x> <y>
#
# Used internally to handle the various marker operations performed
# when the user clicks and drags on the legend area.  The marker changes the
# transfer function to highlight the area being selected in the
# legend.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::updatetransferfuncs {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::AddIsoMarker { x y } {
    if { $activeTf_ == "" } {
	error "active transfer function isn't set"
    }
    set tf $activeTf_ 
    set c $itk_component(legend)
    set m [IsoMarker \#auto $c $this $tf]
    set w [winfo width $c]
    $m relval [expr {double($x-10)/($w-20)}]
    lappend isomarkers_($tf) $m
    updatetransferfuncs
    return 1
}

itcl::body Rappture::NanovisViewer::rmdupmarker { marker x } {
    set tf [$marker transferfunc]
    set bool 0
    if { [info exists isomarkers_($tf)] } {
	set list {}
	set marker [namespace tail $marker]
	foreach m $isomarkers_($tf) {
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
	set isomarkers_($tf) $list
	updatetransferfuncs
    }
    return $bool
}

itcl::body Rappture::NanovisViewer::overmarker { marker x } {
    set tf [$marker transferfunc]
    if { [info exists isomarkers_($tf)] } {
	set marker [namespace tail $marker]
	foreach m $isomarkers_($tf) {
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
    set limits_(min) ""
    set limits_(max) ""
    foreach ivol $style2ids_($tf) {
	if { ![info exists limits_($ivol-min)] } {
	    error "can't find $ivol limits"
	}
	if { $limits_(min) == "" || $limits_(min) > $limits_($ivol-min) } {
	    set limits_(min) $limits_($ivol-min)
	}
	if { $limits_(max) == "" || $limits_(max) < $limits_($ivol-max) } {
	    set limits_(max) $limits_($ivol-max)
	}
    }
    return [array get limits_]
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
	set settings_($this-$key) $value
    }

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    set ::Rappture::NanovisViewer::settings_($this-isosurface) 0
    checkbutton $inner.isosurface \
        -text "Isosurface shading" \
        -variable [itcl::scope settings_($this-isosurface)] \
        -command [itcl::code $this FixSettings isosurface] \
	-font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope settings_($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
	-font "Arial 9"

    checkbutton $inner.grid \
        -text "Grid" \
        -variable [itcl::scope settings_($this-grid)] \
        -command [itcl::code $this FixSettings grid] \
	-font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope settings_($this-outline)] \
        -command [itcl::code $this FixSettings outline] \
	-font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope settings_($this-legend)] \
        -command [itcl::code $this FixSettings legend] \
	-font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope settings_($this-volume)] \
        -command [itcl::code $this FixSettings volume] \
	-font "Arial 9"

    blt::table $inner \
	0,0 $inner.axes  -columnspan 2 -anchor w \
	1,0 $inner.grid  -columnspan 2 -anchor w \
	2,0 $inner.outline  -columnspan 2 -anchor w \
	3,0 $inner.volume  -columnspan 2 -anchor w \
	4,0 $inner.legend  -columnspan 2 -anchor w 

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
	set settings_($this-$key) $value
    }

    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon playback-record]]
    $inner configure -borderwidth 4

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    label $inner.dim -text "Dim" -font $fg
    ::scale $inner.light -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope settings_($this-light)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings light]
    label $inner.bright -text "Bright" -font $fg

    label $inner.fog -text "Fog" -font $fg
    ::scale $inner.transp -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope settings_($this-transp)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings transp]
    label $inner.plastic -text "Plastic" -font $fg

    label $inner.clear -text "Clear" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope settings_($this-opacity)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings opacity]
    label $inner.opaque -text "Opaque" -font $fg

    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope settings_($this-thickness)] \
	-width 10 \
        -showvalue off -command [itcl::code $this FixSettings thickness]
    label $inner.thick -text "Thick" -font $fg

    blt::table $inner \
	0,0 $inner.dim  -anchor e -pady 2 \
	0,1 $inner.light -columnspan 2 -pady 2 \
	0,3 $inner.bright -anchor w -pady 2 \
	1,0 $inner.fog -anchor e -pady 2 \
	1,1 $inner.transp -columnspan 2 -pady 2 \
	1,3 $inner.plastic -anchor w -pady 2 \
	2,0 $inner.clear -anchor e -pady 2 \
	2,1 $inner.opacity -columnspan 2 -pady 2 \
	2,3 $inner.opaque -anchor w -pady 2 \
	3,0 $inner.thin -anchor e -pady 2 \
	3,1 $inner.thickness -columnspan 2 -pady 2 \
	3,3 $inner.thick -anchor w -pady 2

    for {set n 0} {$n <= 3} {incr n} {
        blt::table configure $inner r$n -resize none
    }
    blt::table configure $inner r$n -resize expand
}

itcl::body Rappture::NanovisViewer::BuildCutplanesTab {} {
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]
    $inner configure -borderwidth 4

    # X-value slicer...
    itk_component add xCutButton {
        label $inner.xbutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
	    -image [Rappture::icon x-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness -font
    }
    bind $itk_component(xCutButton) <ButtonPress> \
        [itcl::code $this Slice axis x toggle]
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X cut plane on/off"

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x]
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
        label $inner.ybutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
	    -image [Rappture::icon y-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness -font
    }
    bind $itk_component(yCutButton) <ButtonPress> \
        [itcl::code $this Slice axis y toggle]
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y cut plane on/off"

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y]
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
        label $inner.zbutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
	    -image [Rappture::icon z-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness -font
    }
    bind $itk_component(zCutButton) <ButtonPress> \
        [itcl::code $this Slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z cut plane on/off"

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z]
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
	    -textvariable [itcl::scope settings_($this-$tag)]
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
# USAGE: Slice axis x|y|z ?on|off|toggle?
# USAGE: Slice move x|y|z <newval>
# USAGE: Slice volume ?on|off|toggle?
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::Slice {option args} {
    switch -- $option {
        axis {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"Slice axis x|y|z ?on|off|toggle?\""
            }
            set axis [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { 
		set op "on" 
	    }
            set current [State ${axis}CutButton]
            if {$op == "toggle"} {
                if {$current == "on"} { 
		    set op "off" 
		} else { 
		    set op "on" 
		}
            }
            if {$op} {
                $itk_component(${axis}CutButton) configure \
		    -relief sunken -image [Rappture::icon ${axis}-cutplane-on]
                SendCmd "cutplane state 1 $axis [CurrentVolumeIds -cutplanes]"
		$itk_component(${axis}CutScale) configure -state normal \
		    -troughcolor grey65
            } else {
                $itk_component(${axis}CutButton) configure \
		    -relief raised -image [Rappture::icon ${axis}-cutplane-off]
                SendCmd "cutplane state 0 $axis [CurrentVolumeIds -cutplanes]"
		$itk_component(${axis}CutScale) configure -state disabled \
		    -troughcolor grey82
            }
        }
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set newpos [expr {0.01*$newval}]
#            set newval [expr {0.01*($newval-50)
#                *($limits_(${axis}max)-$limits_(${axis}min))
#                  + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]

            # show the current value in the readout
            #puts "readout: $axis = $newval"

            set ids [CurrentVolumeIds -cutplanes]
            SendCmd "cutplane position $newpos $axis $ids"
        }
        volume {
            if {[llength $args] > 1} {
                error "wrong # args: should be \"Slice volume ?on|off|toggle?\""
            }
            set op [lindex $args 0]
            if {$op == ""} { set op "on" }

            set current $settings_($this-volume)
            if {$op == "toggle"} {
                if {$current} { 
		    set op "off" 
		} else { 
		    set op "on" 
		}
            }
	    
            if {$op} {
                SendCmd "volume data state on [CurrentVolumeIds]"
                $itk_component(volume) configure -relief sunken
		set settings_($this-volume) 1
            } else {
                SendCmd "volume data state off [CurrentVolumeIds]"
                $itk_component(volume) configure -relief raised
		set settings_($this-volume) 0
            }
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
#        *($limits_(${axis}max)-$limits_(${axis}min))
#          + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}
