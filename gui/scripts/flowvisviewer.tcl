

# ----------------------------------------------------------------------
#  COMPONENT: flowvisviewer - 3D flow rendering
#
#
# This widget performs volume and flow rendering on 3D scalar/vector datasets.
# It connects to the Flowvis server running on a rendering farm, transmits
# data, and displays the results.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

option add *FlowvisViewer.width 4i widgetDefault
option add *FlowvisViewer*cursor crosshair widgetDefault
option add *FlowvisViewer.height 4i widgetDefault
option add *FlowvisViewer.foreground black widgetDefault
option add *FlowvisViewer.controlBackground gray widgetDefault
option add *FlowvisViewer.controlDarkBackground #999999 widgetDefault
option add *FlowvisViewer.plotBackground black widgetDefault
option add *FlowvisViewer.plotForeground white widgetDefault
#FIXME: why is plot outline gray? not white?
option add *FlowvisViewer.plotOutline gray widgetDefault
option add *FlowvisViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc FlowvisViewer_init_resources {} {
    Rappture::resources::register \
        nanovis_server Rappture::FlowvisViewer::SetServerList
}

itcl::class Rappture::FlowvisViewer {
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
    public method isconnected {}

    public method GetLimits { tf }
    public method UpdateTransferFunctions {}
    public method RemoveDuplicateIsoMarker { m x }
    public method OverIsoMarker { m x }

    public method drawer {what who}
    public method camera {option args}

    protected method Connect {}
    protected method Disconnect {}

    protected method SendCmd {string}
    protected method _send {string}
    protected method SendDataObjs {}
    protected method SendTransferFunctions {}

    protected method ReceiveImage { args }
    protected method ReceiveLegend { ivol vmin vmax size }
    protected method ReceiveData { args }

    protected method Rebuild {}
    protected method CurrentVolumeIds {{what -all}}
    protected method Zoom {option}
    protected method Pan {option x y}
    protected method Rotate {option x y}
    protected method Slice {option args}
    protected method SlicerTip {axis}

    protected method Flow {option args}
    protected method Play {}
    protected method Pause {}
    public method flow {option}

    protected method State {comp}
    protected method FixSettings {what {value ""}}
    protected method FixLegend {}

    # The following methods are only used by this class.
    private method NameTransferFunction { ivol }
    private method ComputeTransferFunction { tf }
    private method AddIsoMarker { x y }
    private method ParseMarkersOption { tf ivol markers }
    private method ParseLevelsOption { tf ivol levels }
    private method BuildCutplanesDrawer {}
    private method BuildSettingsDrawer {}
    private method BuildCameraDrawer {}
    private method PanCamera {}
    private method GetMovie { widget width height }
    private method WaitIcon { option widget }

    private variable outbuf_       ;# buffer for outgoing commands

    private variable dlist_ ""     ;# list of data objects
    private variable all_data_objs_
    private variable id2style_     ;# maps id => style settings
    private variable obj2ovride_   ;# maps dataobj => style override
    private variable obj2id_       ;# maps dataobj => volume ID in server
    private variable id2obj_       ;# maps dataobj => volume ID in server
    private variable sendobjs_ ""  ;# list of data objs to send to server
    private variable receiveIds_   ;# list of data objs to send to server
    private variable obj2styles_   ;# maps id => style settings
    private variable style2ids_    ;# maps id => style settings

    private variable click_        ;# info used for Rotate operations
    private variable limits_       ;# autoscale min/max for all axes
    private variable view_         ;# view params for 3D view
    private variable isomarkers_    ;# array of isosurface level values 0..1
    private common   settings_
    private variable activeTf_ ""  ;# The currently active transfer function.
    private variable flow_
    # This 
    # indicates which isomarkers and transfer
    # function to use when changing markers,
    # opacity, or thickness.
    common downloadPopup_          ;# download options from popup

    private variable drawer_
    private common hardcopy_
}

itk::usual FlowvisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::constructor { hostlist args } {

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
        "[itcl::code $this SendTransferFunctions]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    $_dispatcher register !play
    $_dispatcher dispatch $this !play "[itcl::code $this flow next]; list"
    
    set outbuf_ ""
    set flow_(state) "stopped"
    array set downloadPopup_ {
        format draft
    }
    array set play_ {
    }
    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]

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
    set settings_($this-speed) 10

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this Zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -side top -padx 2 -pady 1
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(controls).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this Zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -side top -padx 1 -pady 1
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(controls).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-out] \
            -command [itcl::code $this Zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -side top -padx 2 -pady 1
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add settingsButton {
	label $itk_component(controls).settingsbutton \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon wrench]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(settingsButton) -padx 2 -pady 1 \
	-ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(settingsButton) \
	"Configure settings"
    bind $itk_component(settingsButton) <ButtonPress> \
	[itcl::code $this drawer toggle settings]
    pack $itk_component(settingsButton) -side bottom \
	-padx 2 -pady 2 -anchor e

    BuildSettingsDrawer

    itk_component add cutplanesButton {
	label $itk_component(controls).cutplanesbutton \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon cutbutton]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    Rappture::Tooltip::for $itk_component(cutplanesButton) \
	"Set cutplanes"
    bind $itk_component(cutplanesButton) <ButtonPress> \
	[itcl::code $this drawer toggle cutplanes]
    pack $itk_component(cutplanesButton) -side bottom \
	-padx 2 -pady { 0 2 } -ipadx 1 -ipady 1

    BuildCutplanesDrawer

    itk_component add cameraButton {
	label $itk_component(controls).camerabutton \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon camera]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    Rappture::Tooltip::for $itk_component(cameraButton) \
	"Camera settings"
    bind $itk_component(cameraButton) <ButtonPress> \
	[itcl::code $this drawer toggle camera]
    pack $itk_component(cameraButton) -side bottom \
	-padx 2 -pady 1 -ipadx 1 -ipady 1

    BuildCameraDrawer

    #
    # Volume toggle...
    #
    itk_component add volume {
        label $itk_component(controls).volume \
            -borderwidth 1 -relief sunken -padx 1 -pady 1 \
            -text "Volume" \
	    -image [Rappture::icon playback-record]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(volume) <ButtonPress> \
        [itcl::code $this Slice volume toggle]
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    pack $itk_component(volume) -padx 1 -pady 1

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

    
    # Create flow controls...

    itk_component add flowctrl {
        frame $itk_interior.flowctrl 
    } {
	usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(flowctrl) -side top -padx 4 -pady 0 -fill x

    # flow record button...
    itk_component add rewind {
        button $itk_component(flowctrl).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-rewind] \
	    -command [itcl::code $this flow rewind]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(rewind) \
        "Rewind the flow sequence to the beginning"
    pack $itk_component(rewind) -padx 1 -side left

    # flow stop button...
    itk_component add stop {
        button $itk_component(flowctrl).stop \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-stop] \
	    -command [itcl::code $this flow stop]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(stop) \
        "Stop flow visualization"
    pack $itk_component(stop) -padx 1 -side left

    #
    # flow play/pause button...
    #
    itk_component add play {
        button $itk_component(flowctrl).play \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-play] \
	    -command [itcl::code $this flow toggle]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    set fg [option get $itk_component(hull) font Font]

    Rappture::Tooltip::for $itk_component(play) \
        "Play/Pause flow visualization"
    pack $itk_component(play) -padx 1 -side left

    # how do we know when to make something an itk component?

    ::scale $itk_component(flowctrl).value -from 100 -to 1 \
        -showvalue 0 -orient horizontal -width 12 -length 150 \
        -variable [itcl::scope settings_(speed)]
    pack $itk_component(flowctrl).value -side right

    itk_component add speedlabel {
	label $itk_component(flowctrl).speedl -text "Flow Rate:" -font $fg
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(speedlabel)  -side right

    Rappture::Spinint $itk_component(flowctrl).framecnt
    $itk_component(flowctrl).framecnt value 100
    pack $itk_component(flowctrl).framecnt  -side right
    label $itk_component(flowctrl).framecntlabel -text "# Frames" -font $fg
    pack $itk_component(flowctrl).framecntlabel  -side right


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
itcl::body Rappture::FlowvisViewer::destructor {} {
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
itcl::body Rappture::FlowvisViewer::add {dataobj {settings ""}} {
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
        set all_data_objs_($dataobj) 1
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
itcl::body Rappture::FlowvisViewer::get {args} {
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
itcl::body Rappture::FlowvisViewer::delete {args} {
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
itcl::body Rappture::FlowvisViewer::scale {args} {
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
itcl::body Rappture::FlowvisViewer::download {option args} {
    switch $option {
        coming {
            if {[catch {blt::winop snap $itk_component(area) $_image(download)}]} {
                $_image(download) configure -width 1 -height 1
                $_image(download) put #000000
            }
        }
        controls {
	    set popup .flowvisviewerdownload
	    if {![winfo exists .flowvisviewerdownload]} {
		# if we haven't created the popup yet, do it now
		Rappture::Balloon $popup \
		    -title "[Rappture::filexfer::label downloadWord] as..."
		set inner [$popup component inner]
		label $inner.summary -text "" -anchor w
		pack $inner.summary -side top
		set img $_image(plot)
		set res "[image width $img]x[image height $img]"
		radiobutton $inner.draft -text "Image (draft $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value draft
		pack $inner.draft -anchor w

		set res "640x480"
		radiobutton $inner.medium -text "Movie (standard $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value $res
		pack $inner.medium -anchor w

		set res "1024x768"
		radiobutton $inner.high -text "Movie (high quality $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value $res
		pack $inner.high -anchor w

	    } else {
		set inner [$popup component inner]
	    }
	    set num [llength [get]]
	    set num [expr {($num == 1) ? "1 result" : "$num results"}]
	    $inner.summary configure -text "[Rappture::filexfer::label downloadWord] $num in the following format:"
	    update idletasks ;# fix initial sizes
	    return $popup
	}
        now {
	    set popup .molvisviewerdownload
	    if {[winfo exists .molvisviewerdownload]} {
		$popup deactivate
	    }
	    switch -- $_downloadPopup(format) {
		draft {
		    # Get the image data (as base64) and decode it back to
		    # binary.  This is better than writing to temporary
		    # files.  When we switch to the BLT picture image it
		    # won't be necessary to decode the image data.
		    set bytes [$_image(plot) data -format "jpeg -quality 100"]
		    set data [Rappture::encoding::decode -as b64 $data]
		    return [list .jpg $data]
		}
		"640x480" {
		    return [$this GetMovie [lindex $args 0] 640 480]
		}
		"1024x768" {
		    return [$this GetMovie [lindex $args 0] 1024 768]
		}
	    }
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
itcl::body Rappture::FlowvisViewer::Connect {} {
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
itcl::body Rappture::FlowvisViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::FlowvisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set outbuf_ ""
    catch {unset obj2id_}
    array unset id2obj_
    set obj2id_(count) 0
    set id2obj_(count) 0
    set sendobjs_ ""
}

itcl::body Rappture::FlowvisViewer::_send {string} {
    SendCmd $string
}
#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::FlowvisViewer::SendCmd {string} {
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
itcl::body Rappture::FlowvisViewer::SendDataObjs {} {
    blt::busy hold $itk_component(hull); update idletasks
    foreach dataobj $sendobjs_ {
        foreach comp [$dataobj components] {
            # Send the data as one huge base64-encoded mess -- yuck!
            set data [$dataobj values $comp]
            set nbytes [string length $data]
            if { ![SendBytes "flow data follows $nbytes"] } {
                return
            }
            if { ![SendBytes $data] } {
                return
            }
            set ivol $obj2id_(count)
            incr obj2id_(count)

            set id2obj_($ivol) [list $dataobj $comp]
            set obj2id_($dataobj-$comp) $ivol
            NameTransferFunction $ivol
            set receiveIds_($ivol) 1
        }
    }
    set sendobjs_ ""
    blt::busy release $itk_component(hull)

    # activate the proper volume
    set first [lindex [get] 0]
    if { "" != $first } {
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
# USAGE: SendTransferFunctions
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::SendTransferFunctions {} {
    if { $activeTf_ == "" } {
        return
    }
    set tf $activeTf_
    set first [lindex [get] 0] 

    # Insure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update the
    # values in the settings_ varible.
    set inner $itk_component(settingsFrame)
    set value [$inner.opacity get]
    set opacity [expr { double($value) * 0.01 }]
    set settings_($this-$tf-opacity) $opacity
    set value [$inner.thickness get]
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($value) * 0.0001}]
    set settings_($this-$tf-thickness) $thickness

    if { ![info exists $obj2styles_($first)] } {
        foreach tf $obj2styles_($first) {
            ComputeTransferFunction $tf
        }
        FixLegend
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
set counter 0
itcl::body Rappture::FlowvisViewer::ReceiveImage { args } {
    if {![isconnected]} {
	return
    }
    array set info {
	-type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    ReceiveEcho <<line "<read $info(-bytes) bytes"
    if { $info(-type) == "image" } {
	ReceiveEcho "for [image width $_image(plot)]x[image height $_image(plot)] image>"	
	$_image(plot) configure -data $bytes
    } elseif { $info(type) == "movie" } {
	set tag $this-movie-$info(-token)
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
itcl::body Rappture::FlowvisViewer::ReceiveLegend { tf vmin vmax size } {
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

    array set limits [GetLimits $tf]
    $c itemconfigure vmin -text [format %.2g $limits(min)]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %.2g $limits(max)]
    $c coords vmax [expr {$w-$lx}] $ly

    if { [info exists isomarkers_($tf)] } {
        foreach m $isomarkers_($tf) {
            $m Show
        }
    }
}

#
# ReceiveData --
#
#	The procedure is the response from the render server to each "data
#	follows" command.  The server sends back a "data" command invoked our
#	the # slave interpreter.  The purpose is to collect the min/max of the
#	volume sent # to the render server.  Since the client (flowvisviewer)
#	doesn't parse 3D # data formats, we rely on the server (flowvis) to
#	tell us what the limits # are.  Once we've received the limits to all
#	the data we've sent (tracked by # receiveIds_) we can then determine
#	what the transfer functions are for these # volumes.
#
#       Note: There is a considerable tradeoff in having the server report
#             back what the data limits are.  It means that much of the code
#             having to do with transfer-functions has to wait for the data
#             to come back, since the isomarkers are calculated based upon
#             the data limits.  The client code is much messier because of
#             this.  The alternative is to parse any of the 3D formats on the
#             client side.
#
itcl::body Rappture::FlowvisViewer::ReceiveData { args } {
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
        UpdateTransferFunctions
    }
}

#
# Rebuild --
#
# Called automatically whenever something changes that affects the data
# in the widget.  Clears any existing data and rebuilds the widget to
# display new data.  
#
itcl::body Rappture::FlowvisViewer::Rebuild {} {

    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names isomarkers_] {
        foreach m $isomarkers_($tf) {
            $m Hide
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
    PanCamera
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
            NameTransferFunction $ivol
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
itcl::body Rappture::FlowvisViewer::CurrentVolumeIds {{what -all}} {
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
itcl::body Rappture::FlowvisViewer::Zoom {option} {
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

itcl::body Rappture::FlowvisViewer::PanCamera {} {
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
itcl::body Rappture::FlowvisViewer::Rotate {option x y} {
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
itcl::body Rappture::FlowvisViewer::Pan {option x y} {
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
# USAGE: Slice axis x|y|z ?on|off|toggle?
# USAGE: Slice move x|y|z <newval>
# USAGE: Slice volume ?on|off|toggle?
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Slice {option args} {
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
		    -image [Rappture::icon ${axis}-cutplane-on] \
		    -relief sunken
                SendCmd "cutplane state 1 $axis [CurrentVolumeIds -cutplanes]"
		$itk_component(${axis}CutScale) configure -state normal
            } else {
                $itk_component(${axis}CutButton) configure \
		    -image [Rappture::icon ${axis}-cutplane-off] \
		    -relief raised
                SendCmd "cutplane state 0 $axis [CurrentVolumeIds -cutplanes]"
		$itk_component(${axis}CutScale) configure -state disabled
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
itcl::body Rappture::FlowvisViewer::SlicerTip {axis} {
    set val [$itk_component(${axis}CutScale) get]
#    set val [expr {0.01*($val-50)
#        *($limits_(${axis}max)-$limits_(${axis}min))
#          + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}

# ----------------------------------------------------------------------
# USAGE: Flow movie record|stop|play ?on|off|toggle?
#
# Called when the user clicks on the record, stop or play buttons
# for flow visualization.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Flow {option args} {
    switch -- $option {
        movie {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"Flow movie record|stop|play ?on|off|toggle?\""
            }
            set action [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            set current [State $action]
            if {$op == "toggle"} {
                if {$current == "on"} {
                    set op "off"
                } else {
                    set op "on"
                }
            }
            set cmds ""
            switch -- $action {
                record {
		    if { [$itk_component(rewind) cget -relief] != "sunken" } {
			$itk_component(rewind) configure -relief sunken 
			$itk_component(stop) configure -relief raised 
			$itk_component(play) configure -relief raised 
			set inner $itk_component(settingsFrame)
			set frames [$inner.framecnt value]
			set settings_(numframes) $frames
			set cmds "flow capture $frames"
			_send $cmds
		    }
                }
                stop {
		    if { [$itk_component(stop) cget -relief] != "sunken" } {
			$itk_component(rewind) configure -relief raised 
			$itk_component(stop) configure -relief sunken 
			$itk_component(play) configure -relief raised 
			_pause
			set cmds "flow reset"
			_send $cmds
		    }
                }
                play {
		    if { [$itk_component(play) cget -relief] != "sunken" } {
			$itk_component(rewind) configure -relief raised
			$itk_component(stop) configure -relief raised 
			$itk_component(play) configure \
			    -image [Rappture::icon flow-pause] \
			    -relief sunken 
			bind $itk_component(play) <ButtonPress> \
			    [itcl::code $this _pause]
			flow next
		    }
                }
                default {
                    error "bad option \"$option\": should be one of record|stop|play"
                }

            }
        }
        default {
            error "bad option \"$option\": should be movie"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Play
#
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Play {} {
    SendCmd "flow next"
    set delay [expr {int(ceil(pow($settings_(speed)/10.0+2,2.0)*15))}]
    $_dispatcher event -after $delay !play
}

# ----------------------------------------------------------------------
# USAGE: Pause
#
# Invoked when the user hits the "pause" button to stop playing the
# current sequence of frames as a movie.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Pause {} {
    $_dispatcher cancel !play

    # Toggle the button to "play" mode
    $itk_component(play) configure \
        -image [Rappture::icon flow-start] \
        -relief raised 
    bind $itk_component(play) <ButtonPress> \
        [itcl::code $this Flow movie play toggle]
}

# ----------------------------------------------------------------------
# USAGE: State <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::State {comp} {
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
itcl::body Rappture::FlowvisViewer::FixSettings {what {value ""}} {
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
                UpdateTransferFunctions
            }
        }

        thickness {
            if {[isconnected] && $activeTf_ != "" } {
                set val $settings_($this-thickness)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                set tf $activeTf_
                set settings_($this-$tf-thickness) $sval
                UpdateTransferFunctions
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
		pack $itk_component(legend) -side bottom -fill x
	    } else {
		pack forget $itk_component(legend) 
	    }
	    FixLegend
	}
        "volume" {
	    Slice volume $settings_($this-volume)
        }
        "particles" {
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
itcl::body Rappture::FlowvisViewer::FixLegend {} {
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {[winfo width $itk_component(legend)]-20}]
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
# NameTransferFunction --
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
itcl::body Rappture::FlowvisViewer::NameTransferFunction { ivol } {
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
# ComputeTransferFunction --
#
#   Computes and sends the transfer function to the render server.  It's
#   assumed that the volume data limits are known and that the global
#   transfer-functions slider values have be setup.  Both parts are
#   needed to compute the relative value (location) of the marker, and
#   the alpha map of the transfer function.
#
itcl::body Rappture::FlowvisViewer::ComputeTransferFunction { tf } {
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
        lappend isovalues [$m GetRelativeValue]
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
itcl::configbody Rappture::FlowvisViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotoutline {
    # Must check if we are connected because this routine is called from the
    # class body when the -plotoutline itk_option is defined.  At that point
    # the FlowvisViewer class constructor hasn't been called, so we can't
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
itcl::body Rappture::FlowvisViewer::ParseLevelsOption { tf ivol levels } {
    set c $itk_component(legend)
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
        for {set i 1} { $i <= $levels } {incr i} {
            set x [expr {double($i)/($levels+1)}]
            set m [IsoMarker \#auto $c $this $tf]
            $m SetRelativeValue $x
            lappend isomarkers_($tf) $m 
        }
    } else {
        foreach x $levels {
            set m [IsoMarker \#auto $c $this $tf]
            $m SetRelativeValue $x
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
itcl::body Rappture::FlowvisViewer::ParseMarkersOption { tf ivol markers } {
    set c $itk_component(legend)
    regsub -all "," $markers " " markers
    foreach marker $markers {
        set n [scan $marker "%g%s" value suffix]
        if { $n == 2 && $suffix == "%" } {
            # ${n}% : Set relative value. 
            set value [expr {$value * 0.01}]
            set m [IsoMarker \#auto $c $this $tf]
            $m SetRelativeValue $value
            lappend isomarkers_($tf) $m
        } else {
            # ${n} : Set absolute value.
            set m [IsoMarker \#auto $c $this $tf]
            $m SetAbsoluteValue $value
            lappend isomarkers_($tf) $m
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: UndateTransferFunctions 
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::UpdateTransferFunctions {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::FlowvisViewer::AddIsoMarker { x y } {
    if { $activeTf_ == "" } {
        error "active transfer function isn't set"
    }
    set tf $activeTf_ 
    set c $itk_component(legend)
    set m [IsoMarker \#auto $c $this $tf]
    set w [winfo width $c]
    $m SetRelativeValue [expr {double($x-10)/($w-20)}]
    lappend isomarkers_($tf) $m
    UpdateTransferFunctions
    return 1
}

itcl::body Rappture::FlowvisViewer::RemoveDuplicateIsoMarker { marker x } {
    set tf [$marker GetTransferFunction]
    set bool 0
    if { [info exists isomarkers_($tf)] } {
        set list {}
        set marker [namespace tail $marker]
        foreach m $isomarkers_($tf) {
            set sx [$m GetScreenPosition]
            if { $m != $marker } {
                if { $x >= ($sx-3) && $x <= ($sx+3) } {
                    $marker SetRelativeValue [$m GetRelativeValue]
                    itcl::delete object $m
                    bell
                    set bool 1
                    continue
                }
            }
            lappend list $m
        }
        set isomarkers_($tf) $list
        UpdateTransferFunctions
    }
    return $bool
}

itcl::body Rappture::FlowvisViewer::OverIsoMarker { marker x } {
    set tf [$marker GetTransferFunction]
    if { [info exists isomarkers_($tf)] } {
        set marker [namespace tail $marker]
        foreach m $isomarkers_($tf) {
            set sx [$m GetScreenPosition]
            if { $m != $marker } {
                set bool [expr { $x >= ($sx-3) && $x <= ($sx+3) }]
                $m Activate $bool
            }
        }
    }
    return ""
}

itcl::body Rappture::FlowvisViewer::GetLimits { tf } {
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


#
# flow --
#
# Called when the user clicks on the stop or play buttons
# for flow visualization.
#
#	$this flow start
#	$this flow stop
#	$this flow toggle
#	$this flow rewind
#	$this flow pause
#	$this flow next
#
itcl::body Rappture::FlowvisViewer::flow {option} {
    switch -- $option {
	"stop" {
	    if { $flow_(state) != "stopped" } {
		# Reset the play button to off
		$itk_component(play) configure -image [Rappture::icon flow-play]
		$_dispatcher cancel !play
		SendCmd "flow reset"
		set flow_(state) "stopped"
	    }
	}
	"pause" {
	    if { $flow_(state) == "playing" } {
		# Reset the play button to pause
		# Convert to the play button.
		$itk_component(play) configure -image [Rappture::icon flow-play]
		$_dispatcher cancel !play
		set flow_(state) "paused"
	    }
	}
	"start" {
	    if { $flow_(state) != "playing" } {
		# Starts the "flow next" requests to the server.
		# Convert to the pause button.
		$itk_component(play) configure \
		    -image [Rappture::icon flow-pause] 
		flow next
		set flow_(state) "playing"
	    }
	}
	"toggle" {
	    if { $flow_(state) == "playing" } {
		flow pause
	    } else { 
		flow start
	    }
	}
	"rewind" {
	    if  { $flow_(state) != "stopped"  } {
		SendCmd "flow reset"
	    }
	}
	"next" {
	    # This operation should not be called by public routines.
	    SendCmd "flow next"
	    set delay [expr {int(ceil(pow($settings_(speed)/10.0+2,2.0)*15))}]
	    $_dispatcher event -after $delay !play
	} 
	default {
	    error "bad option \"$option\": should be start, stop, toggle, or reset."
	}
    }
}

itcl::body Rappture::FlowvisViewer::drawer { what who } {
    if { [info exists drawer_(current)] && $who != $drawer_(current) } {
	drawer deactivate $drawer_(current)
    }
    switch -- ${what} {
	"activate" {
	    $itk_component(drawer) add $itk_component($who) -sticky nsew
	    after idle [list focus $itk_component($who)]
	    if { ![info exists drawer_($who)] } {
		set rw [winfo reqwidth $itk_component($who)]
		set w [winfo width $itk_component(drawer)]
		set x [expr $w - $rw]
		$itk_component(drawer) sash place 0 $x 0
		set drawer_($who) 1
	    } else {
		set w [winfo width $itk_component(drawer)]
		puts stderr "w of drawer is $w"
		puts stderr "w of last($who) is $drawer_($who-lastx)"
		set x [expr $w - $drawer_($who-lastx) - 10]
		puts stderr "setting sash to $x for $who"
		$itk_component(drawer) sash place 0 $x 0
		$itk_component(drawer) paneconfigure $itk_component($who) \
		    -width $drawer_($who-lastx)
		$itk_component(3dview) configure -width $x
	    }
	    set drawer_(current) $who
	    $itk_component(${who}Button) configure -relief sunken -bd 1
	}
	"deactivate" {
	    # Save the current width of the drawer.
	    puts stderr "component=$who"
	    set width [winfo width $itk_component($who)]
	    set reqwidth [winfo reqwidth $itk_component($who)]
	    if { $reqwidth < $width } {
		set width $reqwidth
	    }
	    set x [lindex [$itk_component(drawer) sash coord 0] 0]
	    puts stderr "sashx=$x"
	    set drawer_($who-lastx) $width
	    $itk_component(drawer) forget $itk_component($who)
	    $itk_component(${who}Button) configure -relief raised -bd 1
	    unset drawer_(current) 
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
	    set bbox [$itk_component(${who}Canvas) bbox all]
	    set wid [winfo width $itk_component(${who}Frame)]
	    $itk_component(${who}Canvas) configure -width $wid \
		-scrollregion $bbox -yscrollincrement 0.1i
	}
    }
}

itcl::body Rappture::FlowvisViewer::BuildSettingsDrawer {} {
    foreach { key value } {
	grid		0
	axes		1
	outline		1
	volume		1
	legend		1
	particles	1
	lic		1
	light		40
	transp		50
	opacity		100
	thickness	350
    } {
	set settings_($this-$key) $value
    }
    itk_component add settings {
	Rappture::Scroller $itk_component(drawer).settings \
	    -xscrollmode auto -yscrollmode auto \
	    -highlightthickness 0
    }

    itk_component add settingsCanvas {
        canvas $itk_component(settings).canvas -highlightthickness 0
    } {
	ignore -highlightthickness
    }
    $itk_component(settings) contents $itk_component(settingsCanvas)

    itk_component add settingsFrame {
	frame $itk_component(settingsCanvas).frame \
	    -highlightthickness 0 
    } {
	ignore -highlightthickness
    }
    $itk_component(settingsCanvas) create window 0 0 \
	-anchor nw -window $itk_component(settingsFrame)
    bind $itk_component(settingsFrame) <Configure> \
	[itcl::code $this drawer resize settings]

    set inner $itk_component(settingsFrame)

    label $inner.title -text "View Settings" -font "Arial 10 bold"
    label $inner.volset -text "Volume Settings" -font "Arial 10 bold"

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

    set ::Rappture::FlowvisViewer::settings_($this-isosurface) 0
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

    checkbutton $inner.particles \
        -text "Particles" \
        -variable [itcl::scope settings_($this-particles)] \
        -command [itcl::code $this FixSettings particles] \
	-font "Arial 9"

    checkbutton $inner.lic \
        -text "Lic" \
        -variable [itcl::scope settings_($this-lic)] \
        -command [itcl::code $this FixSettings lic] \
	-font "Arial 9"

    blt::table $inner \
	0,0 $inner.title -anchor w  -columnspan 4 \
	1,1 $inner.axes  -columnspan 2 -anchor w \
	2,1 $inner.grid  -columnspan 2 -anchor w \
	3,1 $inner.outline  -columnspan 2 -anchor w \
	4,1 $inner.volume  -columnspan 2 -anchor w \
	1,3 $inner.legend  -columnspan 2 -anchor w \
	2,3 $inner.particles  -columnspan 2 -anchor w \
	3,3 $inner.lic  -columnspan 1 -anchor w \
	9,0 $inner.volset -anchor w  -columnspan 4 \
	11,1 $inner.dim  -anchor e \
	11,2 $inner.light -columnspan 2 \
	11,4 $inner.bright -anchor w \
	12,1 $inner.fog -anchor e \
	12,2 $inner.transp -columnspan 2 \
	12,4 $inner.plastic -anchor w \
	13,1 $inner.clear -anchor e \
	13,2 $inner.opacity -columnspan 2 \
	13,4 $inner.opaque -anchor w \
	14,1 $inner.thin -anchor e \
	14,2 $inner.thickness -columnspan 2 \
	14,4 $inner.thick -anchor w \

    blt::table configure $inner c0 -resize expand -width 4
}

itcl::body Rappture::FlowvisViewer::BuildCutplanesDrawer {} {
    itk_component add cutplanes {
	Rappture::Scroller $itk_component(drawer).cutplanes \
	    -xscrollmode auto -yscrollmode auto \
	    -highlightthickness 0
    }

    #
    # Create slicer controls...
    #
    itk_component add cutplanesCanvas {
        canvas $itk_component(cutplanes).canvas -highlightthickness 0
    } {
	ignore -highlightthickness
    }
    $itk_component(cutplanes) contents $itk_component(cutplanesCanvas)

    itk_component add cutplanesFrame {
	frame $itk_component(cutplanesCanvas).frame \
	    -highlightthickness 0 
    }  {
	ignore -highlightthickness
    }
    $itk_component(cutplanesCanvas) create window 0 0 \
	-anchor nw -window $itk_component(cutplanesFrame)
    bind $itk_component(cutplanesFrame) <Configure> \
	[itcl::code $this drawer resize cutplanes]

    set inner $itk_component(cutplanesFrame)

    label $inner.title -text "Cutplanes" -font "Arial 10 bold"

    # X-value slicer...
    itk_component add xCutButton {
        label $itk_component(cutplanes).xbutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon x-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness
        rename -highlightbackground -controlbackground \
	    controlBackground Background
    }
    bind $itk_component(xCutButton) <ButtonPress> \
        [itcl::code $this Slice axis x toggle]
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X cut plane on/off"

    itk_component add xCutScale {
        ::scale $itk_component(cutplanes).xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground \
	    controlBackground Background
        rename -troughcolor -controldarkbackground \
	    controlDarkBackground Background
    }
    $itk_component(xCutScale) set 50
    #$itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this SlicerTip x]"

    # Y-value slicer...
    itk_component add yCutButton {
        label $itk_component(cutplanes).ybutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon y-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness        
	rename -highlightbackground -controlbackground \
	    controlBackground Background
    }
    bind $itk_component(yCutButton) <ButtonPress> \
        [itcl::code $this Slice axis y toggle]
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y cut plane on/off"

    itk_component add yCutScale {
        ::scale $itk_component(cutplanes).yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(yCutScale) set 50
    #$itk_component(yCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this SlicerTip y]"

    # Z-value slicer...
    itk_component add zCutButton {
        label $itk_component(cutplanes).zbutton \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon z-cutplane-off] \
	    -highlightthickness 0 
    } {
        usual
        ignore -borderwidth -highlightthickness        
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(zCutButton) <ButtonPress> \
        [itcl::code $this Slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z cut plane on/off"

    itk_component add zCutScale {
        ::scale $itk_component(cutplanes).zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(zCutScale) set 50
    #$itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this SlicerTip z]"

    blt::table $inner \
	0,0 $inner.title -anchor w  -columnspan 4 \
	3,1 $itk_component(xCutButton) \
	3,2 $itk_component(yCutButton) \
	3,3 $itk_component(zCutButton) \
	2,1 $itk_component(xCutScale) \
	2,2 $itk_component(yCutScale) \
	2,3 $itk_component(zCutScale) \

    blt::table configure $inner c0 -resize expand -width 4
}

itcl::body Rappture::FlowvisViewer::BuildCameraDrawer {} {

    itk_component add camera {
	Rappture::Scroller $itk_component(drawer).camerascrl \
	    -xscrollmode auto -yscrollmode auto \
	    -highlightthickness 0
    }

    itk_component add cameraCanvas {
	canvas $itk_component(camera).canvas -highlightthickness 0
    } {
	ignore -highlightthickness
    }
    $itk_component(camera) contents $itk_component(cameraCanvas)

    itk_component add cameraFrame {
	frame $itk_component(cameraCanvas).frame \
	    -highlightthickness 0 
    } 
    $itk_component(cameraCanvas) create window 0 0 \
	-anchor nw -window $itk_component(cameraFrame)
    bind $itk_component(cameraFrame) <Configure> \
	[itcl::code $this drawer resize camera]

    set inner $itk_component(cameraFrame)

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

#  camera -- 
itcl::body Rappture::FlowvisViewer::camera {option args} {
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
		    PanCamera
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

itcl::body Rappture::FlowvisViewer::WaitIcon  { option widget } {
    switch -- $option {
	"start" {
	    $_dispatcher dispatch $this !waiticon \
		"[itcl::code $this WaitIcon "next" $widget] ; list"
	    set icon_ 0
	    $widget configure -image [Rappture::icon bigroller${icon_}]
	    $_dispatcher event -after 100 !waiticon
	}
	"next" {
	    incr icon_
	    if { $icon_ >= 8 } {
		set icon_ 0
	    }
	    $widget configure -image [Rappture::icon bigroller${icon_}]
	    $_dispatcher event -after 100 !waiticon
	}
	"stop" {
	    $_dispatcher cancel !waiticon
	}
    }
}

itcl::body Rappture::FlowvisViewer::GetMovie { widget width height } {
    set token "movie[incr nextToken_]"
    set var ::Rappture::MolvisViewer::hardcopy_($this-$token)
    set $var ""

    # Setup an automatic timeout procedure.
    $_dispatcher dispatch $this !movietimeout "set $var {} ; list"

    set popup [Rappture::Balloon .movie -title "Generating video..."]
    set inner [$popup component inner]
    label $inner.title -text "Generating Hardcopy" -font "Arial 10 bold"
    label $inner.please -text "This may take a few minutes." -font "Arial 10"
    label $inner.icon -image [Rappture::icon bigroller0]
    button $inner.cancel -text "Cancel" -font "Arial 10 bold" \
	-command [list set $var ""]
    $_dispatcher event -after 60000 !movietimeout
    WaitIcon start $inner.icon
    bind $inner.cancel <KeyPress-Return> [list $inner.cancel invoke]
    
    blt::table $inner \
	0,0 $inner.title -columnspan 2 \
	1,0 $inner.please -anchor w \
	1,1 $inner.icon -anchor e  \
	2,0 $inner.cancel -columnspan 2 
    blt::table configure $inner r0 -pady 4 
    blt::table configure $inner r2 -pady 4 
    grab set -local $inner
    focus $inner.cancel

    SendCmd "flow video $width $height $settings_(numframes) 2.0 1000"
    
    $popup activate $widget below
    update
    # We wait here for either 
    #  1) the png to be delivered or 
    #  2) timeout or  
    #  3) user cancels the operation.
    tkwait variable $var

    # Clean up.
    $_dispatcher cancel !pngtimeout
    WaitIcon stop $inner.icon
    grab release $inner
    $popup deactivate
    destroy $popup
    update

    if { $hardcopy_($this-$token) != "" } {
	return [list .png $hardcopy_($this-$token)]
    }
    return ""
}
