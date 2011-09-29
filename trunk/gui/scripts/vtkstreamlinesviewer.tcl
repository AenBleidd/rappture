
# ----------------------------------------------------------------------
#  COMPONENT: vtkviewer - Vtk drawing object viewer
#
#  It connects to the Vtk server running on a rendering farm,
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
#package require Img
package require vtk

option add *VtkStreamlinesViewer.width 4i widgetDefault
option add *VtkStreamlinesViewer*cursor crosshair widgetDefault
option add *VtkStreamlinesViewer.height 4i widgetDefault
option add *VtkStreamlinesViewer.foreground black widgetDefault
option add *VtkStreamlinesViewer.controlBackground gray widgetDefault
option add *VtkStreamlinesViewer.controlDarkBackground #999999 widgetDefault
option add *VtkStreamlinesViewer.plotBackground black widgetDefault
option add *VtkStreamlinesViewer.plotForeground white widgetDefault
option add *VtkStreamlinesViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkStreamlinesViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkStreamlinesViewer::SetServerList
}

itcl::class Rappture::VtkStreamlinesViewer {
    inherit Rappture::VisViewer

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""

    constructor { hostlist args } {
        Rappture::VisViewer::constructor $hostlist
    } {
        # defined below
    }
    destructor {
        # defined below
    }
    public proc SetServerList { namelist } {
        Rappture::VisViewer::SetServerList "vtkvis" $namelist
    }
    public method add {dataobj {settings ""}}
    public method camera {option args}
    public method delete {args}
    public method disconnect {}
    public method download {option args}
    public method get {args}
    public method isconnected {}
    public method limits { colormap }
    public method sendto { string }
    public method parameters {title args} { 
        # do nothing 
    }
    public method scale {args}

    protected method Connect {}
    protected method CurrentDatasets {args}
    protected method Disconnect {}
    protected method DoResize {}
    protected method DoRotate {}
    protected method AdjustSetting {what {value ""}}
    protected method FixSettings { args  }
    protected method Pan {option x y}
    protected method Pick {x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { colormap title vmin vmax size }
    protected method Rotate {option x y}
    protected method SendCmd {string}
    protected method Zoom {option}

    # The following methods are only used by this class.
    private method BuildAxisTab {}
    private method BuildCameraTab {}
    private method BuildColormap { name colors }
    private method BuildCutplaneTab {}
    private method BuildDownloadPopup { widget command } 
    private method BuildStreamsTab {}
    private method BuildVolumeTab {}
    private method ConvertToVtkData { dataobj comp } 
    private method DrawLegend { title }
    private method EnterLegend { x y } 
    private method EventuallyResize { w h } 
    private method EventuallyRotate { q } 
    private method EventuallySetCutplane { axis args } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method IsValidObject { dataobj } 
    private method LeaveLegend {}
    private method MotionLegend { x y } 
    private method PanCamera {}
    private method RequestLegend { {mode "vmag"} }
    private method SetColormap { dataobj comp }
    private method ChangeColormap { dataobj comp color }
    private method ColorsToColormap { color }
    private method SetLegendTip { x y }
    private method SetObjectStyle { dataobj comp } 
    private method Slice {option args} 

    private variable _arcball ""
    private variable _outbuf       ;	# buffer for outgoing commands

    private variable _dlist ""     ;	# list of data objects
    private variable _allDataObjs
    private variable _obj2datasets
    private variable _obj2ovride   ;	# maps dataobj => style override
    private variable _datasets     ;	# contains all the dataobj-component 
                                   ;	# datasets in the server
    private variable _colormaps    ;	# contains all the colormaps
                                   ;	# in the server.
    private variable _dataset2style    ;# maps dataobj-component to transfunc

    private variable _click        ;	# info used for rotate operations
    private variable _limits       ;	# autoscale min/max for all axes
    private variable _view         ;	# view params for 3D view
    private variable _settings
    private variable _volume
    private variable _axis
    private variable _cutplane
    private variable _streamlines
    private variable _style;		# Array of current component styles.
    private variable _initialStyle;	# Array of initial component styles.
    private variable _reset 1;		# indicates if camera needs to be reset
					# to starting position.

    private variable _first ""     ;	# This is the topmost dataset.
    private variable _start 0
    private variable _buffering 0
    private variable _title ""
    private variable _seeds

    common _downloadPopup;		# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _rotatePending 0
    private variable _cutplanePending 0
    private variable _outline
    private variable _fields 
}

itk::usual VtkStreamlinesViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::constructor {hostlist args} {
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # X-Cutplane event
    $_dispatcher register !xcutplane
    $_dispatcher dispatch $this !xcutplane \
	"[itcl::code $this AdjustSetting xcutplane-position]; list"

    # Y-Cutplane event
    $_dispatcher register !ycutplane
    $_dispatcher dispatch $this !ycutplane \
	"[itcl::code $this AdjustSetting ycutplane-position]; list"

    # Z-Cutplane event
    $_dispatcher register !zcutplane
    $_dispatcher dispatch $this !zcutplane \
	"[itcl::code $this AdjustSetting zcutplane-position]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias dataset [itcl::code $this ReceiveDataset]
    $_parser alias legend [itcl::code $this ReceiveLegend]

    array set _outline {
	id -1
	afterId -1
	x1 -1
	y1 -1
	x2 -1
	y2 -1
    }
    # Initialize the view to some default parameters.
    array set _view {
	qw		1
	qx		0
	qy		0
	qz		0
        zoom		1.0 
        xpan		0
        ypan		0
        ortho		0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(zmin) 0.0
    set _limits(zmax) 1.0

    array set _axis [subst {
        xgrid		0
        ygrid		0
        zgrid		0
        xcutplane	0
        ycutplane	0
        zcutplane	0
        xposition	0
        yposition	0
        zposition	0
        visible		1
	labels		1
    }]
    array set _cutplane [subst {
        xvisible	0
        yvisible	0
        zvisible	0
        xposition	0
        yposition	0
        zposition	0
        visible		1
    }]
    array set _volume [subst {
        edges		0
        lighting	1
        opacity		40
        visible		1
        wireframe	0
    }]
    array set _streamlines [subst {
        seeds		0
        visible		1
        opacity		100
        seeddensity	40
        lighting	1
        scale		1
    }]
    array set _settings [subst {
        legend		1
    }]

    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }

    set c $itk_component(view)
    bind $c <Configure> [itcl::code $this EventuallyResize %w %h]
    bind $c <4> [itcl::code $this Zoom in 0.25]
    bind $c <5> [itcl::code $this Zoom out 0.25]
    bind $c <KeyPress-Left>  [list %W xview scroll 10 units]
    bind $c <KeyPress-Right> [list %W xview scroll -10 units]
    bind $c <KeyPress-Up>    [list %W yview scroll 10 units]
    bind $c <KeyPress-Down>  [list %W yview scroll -10 units]
    bind $c <Enter> "focus %W"

    # Fix the scrollregion in case we go off screen
    $c configure -scrollregion [$c bbox all]

    set _map(id) [$c create image 0 0 -anchor nw -image $_image(plot)]
    set _map(cwidth) -1 
    set _map(cheight) -1 
    set _map(zoom) 1.0
    set _map(original) ""

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
	    -variable [itcl::scope _volume(visible)] \
	    -command [itcl::code $this AdjustSetting volume-visible] 
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Don't display the volume"
    pack $itk_component(volume) -padx 2 -pady 2

    itk_component add streamlines {
        Rappture::PushButton $f.streamlines \
            -onimage [Rappture::icon streamlines-on] \
            -offimage [Rappture::icon streamlines-off] \
	    -variable [itcl::scope _streamlines(visible)] \
	    -command [itcl::code $this AdjustSetting streamlines-visible] \
    }
    $itk_component(streamlines) select
    Rappture::Tooltip::for $itk_component(streamlines) \
        "Toggle the streamlines on/off"
    pack $itk_component(streamlines) -padx 2 -pady 2

    if { [catch {
	BuildVolumeTab
	BuildStreamsTab
	BuildAxisTab
	BuildCutplaneTab
	BuildCameraTab
    } errs] != 0 } {
	puts stderr errs=$errs
    }
    # Legend

    set _image(legend) [image create photo]
    itk_component add legend {
        canvas $itk_component(plotarea).legend -width 50 -highlightthickness 0 
    } {
        usual
        ignore -highlightthickness
        rename -background -plotbackground plotBackground Background
    }

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w 
    blt::table configure $itk_component(plotarea) c1 -resize none

    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> \
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    if 0 {
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]
    }
    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

    bind $itk_component(view) <ButtonRelease-3> \
        [itcl::code $this Pick %x %y]

    # Bindings for panning via keyboard
    bind $itk_component(view) <KeyPress-Left> \
        [itcl::code $this Pan set -10 0]
    bind $itk_component(view) <KeyPress-Right> \
        [itcl::code $this Pan set 10 0]
    bind $itk_component(view) <KeyPress-Up> \
        [itcl::code $this Pan set 0 -10]
    bind $itk_component(view) <KeyPress-Down> \
        [itcl::code $this Pan set 0 10]
    bind $itk_component(view) <Shift-KeyPress-Left> \
        [itcl::code $this Pan set -2 0]
    bind $itk_component(view) <Shift-KeyPress-Right> \
        [itcl::code $this Pan set 2 0]
    bind $itk_component(view) <Shift-KeyPress-Up> \
        [itcl::code $this Pan set 0 -2]
    bind $itk_component(view) <Shift-KeyPress-Down> \
        [itcl::code $this Pan set 0 2]

    # Bindings for zoom via keyboard
    bind $itk_component(view) <KeyPress-Prior> \
        [itcl::code $this Zoom out]
    bind $itk_component(view) <KeyPress-Next> \
        [itcl::code $this Zoom in]

    bind $itk_component(view) <Enter> "focus $itk_component(view)"

    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        bind $itk_component(view) <4> [itcl::code $this Zoom out]
        bind $itk_component(view) <5> [itcl::code $this Zoom in]
    }

    set _image(download) [image create photo]

    eval itk_initialize $args
    Connect
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::destructor {} {
    Disconnect
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    $_dispatcher cancel !xcutplane
    $_dispatcher cancel !ycutplane
    $_dispatcher cancel !zcutplane
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::VtkStreamlinesViewer::DoResize {} {
    if { $_width < 2 } {
	set _width 500
    }
    if { $_height < 2 } {
	set _height 500
    }
    #puts stderr "DoResize screen size $_width $_height"
    set _start [clock clicks -milliseconds]
    #puts stderr "screen size request width=$_width height=$_height"
    SendCmd "screen size $_width $_height"
    RequestLegend

    #SendCmd "imgflush"

    # Must reset camera to have object scaling to take effect.
    #SendCmd "camera reset"
    #SendCmd "camera zoom $_view(zoom)"
    set _resizePending 0
}

itcl::body Rappture::VtkStreamlinesViewer::DoRotate {} {
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallyResize { w h } {
    #puts stderr "EventuallyResize $w $h"
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

set rotate_delay 100

itcl::body Rappture::VtkStreamlinesViewer::EventuallyRotate { q } {
    #puts stderr "EventuallyRotate $w $h"
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
    if { !$_rotatePending } {
        set _rotatePending 1
	global rotate_delay 
        $_dispatcher event -after $rotate_delay !rotate
    }
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallySetCutplane { axis args } {
    if { !$_cutplanePending } {
        set _cutplanePending 1
        $_dispatcher event -after 100 !${axis}cutplane
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -width 1
        -linestyle solid
        -brightness 0
        -raise 0
        -description ""
        -param ""
	-type ""
    }
    array set params $settings
    set params(-description) ""
    set params(-param) ""
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
    }
    set _allDataObjs($dataobj) 1
    set _obj2ovride($dataobj-color) $params(-color)
    set _obj2ovride($dataobj-width) $params(-width)
    set _obj2ovride($dataobj-raise) $params(-raise)
    $_dispatcher event -idle !rebuild
}


# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
#       Clients use this to delete a dataobj from the plot.  If no dataobjs
#       are specified, then all dataobjs are deleted.  No data objects are
#       deleted.  They are only removed from the display list.
#
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::delete {args} {
    if { [llength $args] == 0} {
        set args $_dlist
    }
    # Delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
	if { $pos < 0 } {
	    continue;			# Don't know anything about it.
	}
	# Remove it from the dataobj list.
	set _dlist [lreplace $_dlist $pos $pos]
	foreach comp [$dataobj components] {
	    SendCmd "dataset visible 0 $dataobj-$comp"
	}
	array unset _obj2ovride $dataobj-*
	# Append to the end of the dataobj list.
	lappend _dlist $dataobj
	set changed 1
    }
    # If anything changed, then rebuild the plot
    if { $changed } {
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: get ?-objects?
# USAGE: get ?-visible?
# USAGE: get ?-image view?
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.  The optional "-image"
# flag can also request the internal images being shown.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::get {args} {
    if {[llength $args] == 0} {
        set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
	"-objects" {
	    # put the dataobj list in order according to -raise options
	    set dlist {}
	    foreach dataobj $_dlist {
		if { ![IsValidObject $dataobj] } {
		    continue
		}
		if {[info exists _obj2ovride($dataobj-raise)] && 
		    $_obj2ovride($dataobj-raise)} {
		    set dlist [linsert $dlist 0 $dataobj]
		} else {
		    lappend dlist $dataobj
		}
	    }
	    return $dlist
	}
	"-visible" {
	    set dlist {}
	    foreach dataobj $_dlist {
		if { ![IsValidObject $dataobj] } {
		    continue
		}
		if { ![info exists _obj2ovride($dataobj-raise)] } {
		    # No setting indicates that the object isn't invisible.
		    continue
		}
		# Otherwise use the -raise parameter to put the object to
		# the front of the list.
		if { $_obj2ovride($dataobj-raise) } {
		    set dlist [linsert $dlist 0 $dataobj]
		} else {
		    lappend dlist $dataobj
		}
	    }
	    return $dlist
	}	    
	-image {
	    if {[llength $args] != 2} {
		error "wrong # args: should be \"get -image view\""
	    }
	    switch -- [lindex $args end] {
		view {
		    return $_image(plot)
		}
		default {
		    error "bad image name \"[lindex $args end]\": should be view"
		}
	    }
	}
	default {
	    error "bad option \"$op\": should be -objects or -image"
	}
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
itcl::body Rappture::VtkStreamlinesViewer::scale {args} {
    array unset _limits
    foreach dataobj $args {
	set string [limits $dataobj]
	if { $string == "" } {
	    continue
	}
	array set bounds $string
	if {![info exists _limits(xmin)] || $_limits(xmin) > $bounds(xmin)} {
	    set _limits(xmin) $bounds(xmin)
	}
	if {![info exists _limits(xmax)] || $_limits(xmax) < $bounds(xmax)} {
	    set _limits(xmax) $bounds(xmax)
	}

	if {![info exists _limits(ymin)] || $_limits(ymin) > $bounds(ymin)} {
	    set _limits(ymin) $bounds(ymin)
	}
	if {![info exists _limits(ymax)] || $_limits(ymax) < $bounds(ymax)} {
	    set _limits(ymax) $bounds(ymax)
	}

	if {![info exists _limits(zmin)] || $_limits(zmin) > $bounds(zmin)} {
	    set _limits(zmin) $bounds(zmin)
	}
	if {![info exists _limits(zmax)] || $_limits(zmax) < $bounds(zmax)} {
	    set _limits(zmax) $bounds(zmax)
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
itcl::body Rappture::VtkStreamlinesViewer::download {option args} {
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
            set popup .vtkviewerdownload
            if { ![winfo exists .vtkviewerdownload] } {
                set inner [BuildDownloadPopup $popup [lindex $args 0]]
            } else {
                set inner [$popup component inner]
            }
            set _downloadPopup(image_controls) $inner.image_frame
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            set word [Rappture::filexfer::label downloadWord]
            $inner.summary configure -text "$word $num in the following format:"
            update idletasks		;# Fix initial sizes
            return $popup
        }
        now {
            set popup .vtkviewerdownload
            if {[winfo exists .vtkviewerdownload]} {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                "image" {
                    return [$this GetImage [lindex $args 0]]
                }
                "vtk" {
                    return [$this GetVtkData [lindex $args 0]]
                }
            }
            return ""
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
itcl::body Rappture::VtkStreamlinesViewer::Connect {} {
    #puts stderr "Enter Connect: [info level -1]"
    set _hosts [GetServerList "vtkvis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
    if { $result } {
	#puts stderr "Connected to $_hostname sid=$_sid"
        set w [winfo width $itk_component(view)]
        set h [winfo height $itk_component(view)]
        EventuallyResize $w $h
    }
    return $result
}

#
# isconnected --
#
#       Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::VtkStreamlinesViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkStreamlinesViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkStreamlinesViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _datasets 
    array unset _data 
    array unset _colormaps 
}

#
# sendto --
#
itcl::body Rappture::VtkStreamlinesViewer::sendto { bytes } {
    SendBytes "$bytes\n"
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::VtkStreamlinesViewer::SendCmd {string} {
    if { $_buffering } {
        append _outbuf $string "\n"
    } else {
        SendBytes "$string\n"
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::ReceiveImage { args } {
    array set info {
        -token "???"
        -bytes 0
        -type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    if { $info(-type) == "image" } {
	if 0 {
	    set f [open "last.ppm" "w"] 
	    puts $f $bytes
	    close $f
	}
        $_image(plot) configure -data $bytes
	set time [clock seconds]
	set date [clock format $time]
        #puts stderr "$date: received image [image width $_image(plot)]x[image height $_image(plot)] image>"        
	if { $_start > 0 } {
	    set finish [clock clicks -milliseconds]
	    #puts stderr "round trip time [expr $finish -$_start] milliseconds"
	    set _start 0
	}
    } elseif { $info(type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
}

#
# ReceiveDataset --
#
itcl::body Rappture::VtkStreamlinesViewer::ReceiveDataset { args } {
    if { ![isconnected] } {
        return
    }
    set option [lindex $args 0]
    switch -- $option {
	"scalar" {
	    set option [lindex $args 1]
	    switch -- $option {
		"world" {
		    foreach { x y z value tag } [lrange $args 2 end] break
		}
		"pixel" {
		    foreach { x y value tag } [lrange $args 2 end] break
		}
	    }
	}
	"vector" {
	    set option [lindex $args 1]
	    switch -- $option {
		"world" {
		    foreach { x y z vx vy vz tag } [lrange $args 2 end] break
		}
		"pixel" {
		    foreach { x y vx vy vz tag } [lrange $args 2 end] break
		}
	    }
	}
	"names" {
            foreach { name } [lindex $args 1] {
                #puts stderr "Dataset: $name"
            }
	}
	default {
	    error "unknown dataset option \"$option\" from server"
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
itcl::body Rappture::VtkStreamlinesViewer::Rebuild {} {
    update

    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $w < 2 || $h < 2 } {
	$_dispatcher event -idle !rebuild
	return
    }

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    set _width $w
    set _height $h
    $_arcball resize $w $h
    DoResize
    #
    # Reset the camera and other view parameters
    #
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q
    if {$_view(ortho)} {
        SendCmd "camera mode ortho"
    } else {
        SendCmd "camera mode persp"
    }
    DoRotate
    PanCamera
    set _first [lindex [get -objects] 0] 
    if { $_reset || $_first == "" } {
	Zoom reset
	set _reset 0
    }
    FixSettings axis-xgrid axis-ygrid axis-zgrid axis-mode \
	axis-visible axis-labels \
	streamlines-seeds streamlines-visible streamlines-opacity \
	volume-edges volume-lighting volume-opacity volume-visible \
	volume-wireframe 

    SendCmd "imgflush"

    set _limits(zmin) ""
    set _limits(zmax) ""
    set _first ""
    foreach dataobj [get -objects] {
	if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
	    set _first $dataobj
	}
	set _obj2datasets($dataobj) ""
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj blob $comp]
                set length [string length $bytes]
                append _outbuf "dataset add $tag data follows $length\n"
                append _outbuf $bytes
                set _datasets($tag) 1
		SetObjectStyle $dataobj $comp
	    }
	    lappend _obj2datasets($dataobj) $tag
	    if { [info exists _obj2ovride($dataobj-raise)] } {
		SendCmd "dataset visible 1 $tag"
	    } else {
		SendCmd "dataset visible 0 $tag"
	    }
        }
    }
    if {"" != $_first} {
        set location [$_first hints camera]
        if { $location != "" } {
            array set view $location
        }

	foreach axis { x y z } {
	    set label [$_first hints ${axis}label]
	    if { $label != "" } {
		SendCmd "axis name $axis $label"
	    }
	    set units [$_first hints ${axis}units]
	    if { $units != "" } {
		SendCmd "axis units $axis $units"
	    }
	}
    }
	
    set _buffering 0;                        # Turn off buffering.

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf;                        
    blt::busy release $itk_component(hull)
    set _outbuf "";                        # Clear the buffer.                
}

# ----------------------------------------------------------------------
# USAGE: CurrentDatasets ?-all -visible? ?dataobjs?
#
# Returns a list of server IDs for the current datasets being displayed.  This
# is normally a single ID, but it might be a list of IDs if the current data
# object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::CurrentDatasets {args} {
    set flag [lindex $args 0]
    switch -- $flag { 
	"-all" {
	    if { [llength $args] > 1 } {
		error "CurrentDatasets: can't specify dataobj after \"-all\""
	    }
	    set dlist [get -objects]
	}
	"-visible" {
	    if { [llength $args] > 1 } {
		set dlist {}
		set args [lrange $args 1 end]
		foreach dataobj $args {
		    if { [info exists _obj2ovride($dataobj-raise)] } {
			lappend dlist $dataobj
		    }
		}
	    } else {
		set dlist [get -visible]
	    }
	}	    
	default {
	    set dlist $args
	}
    }
    set rlist ""
    foreach dataobj $dlist {
	foreach comp [$dataobj components] {
	    set tag $dataobj-$comp
	    if { [info exists _datasets($tag)] && $_datasets($tag) } {
		lappend rlist $tag
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
itcl::body Rappture::VtkStreamlinesViewer::Zoom {option} {
    switch -- $option {
        "in" {
            set _view(zoom) [expr {$_view(zoom)*1.25}]
	    SendCmd "camera zoom $_view(zoom)"
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
	    SendCmd "camera zoom $_view(zoom)"
        }
        "reset" {
            array set _view {
		qw	1
		qx	0
		qy	0
		qz	0
                zoom    1.0
                xpan   0
                ypan   0
            }
            SendCmd "camera reset all"
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
	    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
	    $_arcball quaternion $q
	    DoRotate
        }
    }
}

itcl::body Rappture::VtkStreamlinesViewer::PanCamera {} {
    set x $_view(xpan)
    set y $_view(ypan)
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
itcl::body Rappture::VtkStreamlinesViewer::Rotate {option x y} {
    switch -- $option {
        "click" {
            $itk_component(view) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
        }
        "drag" {
            if {[array size _click] == 0} {
                Rotate click $x $y
            } else {
                set w [winfo width $itk_component(view)]
                set h [winfo height $itk_component(view)]
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
		if { $dx == 0 && $dy == 0 } {
		    return
		}
		set q [$_arcball rotate $x $y $_click(x) $_click(y)]
		EventuallyRotate $q
                set _click(x) $x
                set _click(y) $y
            }
        }
        "release" {
            Rotate drag $x $y
            $itk_component(view) configure -cursor ""
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

itcl::body Rappture::VtkStreamlinesViewer::Pick {x y} {
    foreach tag [CurrentDatasets -visible] {
        SendCmd "dataset getscalar pixel $x $y $tag"
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
itcl::body Rappture::VtkStreamlinesViewer::Pan {option x y} {
    switch -- $option {
	"set" {
	    set w [winfo width $itk_component(view)]
	    set h [winfo height $itk_component(view)]
	    set x [expr $x / double($w)]
	    set y [expr $y / double($h)]
	    set _view(xpan) [expr $_view(xpan) + $x]
	    set _view(ypan) [expr $_view(ypan) + $y]
	    PanCamera
	    return
	}
	"click" {
	    set _click(x) $x
	    set _click(y) $y
	    $itk_component(view) configure -cursor hand1
	}
	"drag" {
	    if { ![info exists _click(x)] } {
		set _click(x) $x
	    }
	    if { ![info exists _click(y)] } {
		set _click(y) $y
	    }
	    set w [winfo width $itk_component(view)]
	    set h [winfo height $itk_component(view)]
	    set dx [expr ($_click(x) - $x)/double($w)]
	    set dy [expr ($_click(y) - $y)/double($h)]
	    set _click(x) $x
	    set _click(y) $y
	    set _view(xpan) [expr $_view(xpan) - $dx]
	    set _view(ypan) [expr $_view(ypan) - $dy]
	    PanCamera
	}
	"release" {
	    Pan drag $x $y
	    $itk_component(view) configure -cursor ""
	}
	default {
	    error "unknown option \"$option\": should set, click, drag, or release"
	}
    }
}

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::FixSettings { args } {
    foreach setting $args {
	AdjustSetting $setting
    }
}

#
# AdjustSetting --
#
#	Changes/updates a specific setting in the widget.  There are
#	usually user-setable option.  Commands are sent to the render
#	server.
#
itcl::body Rappture::VtkStreamlinesViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
	return
    }
    switch -- $what {
        "volume-opacity" {
	    set val $_volume(opacity)
	    set sval [expr { 0.01 * double($val) }]
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "polydata opacity $sval $dataset"
	    }
        }
        "volume-wireframe" {
	    set bool $_volume(wireframe)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "polydata wireframe $bool $dataset"
            }
        }
        "volume-visible" {
	    set bool $_volume(visible)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "polydata visible $bool $dataset"
            }
	    if { $bool } {
		Rappture::Tooltip::for $itk_component(volume) \
		    "Hide the volume"
	    } else {
		Rappture::Tooltip::for $itk_component(volume) \
		    "Show the volume"
	    }
        }
        "volume-lighting" {
	    set bool $_volume(lighting)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "polydata lighting $bool $dataset"
            }
        }
        "volume-edges" {
	    set bool $_volume(edges)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "polydata edges $bool $dataset"
            }
        }
        "axis-visible" {
	    set bool $_axis(visible)
	    SendCmd "axis visible all $bool"
        }
        "axis-labels" {
	    set bool $_axis(labels)
	    SendCmd "axis labels all $bool"
        }
        "axis-xgrid" {
	    set bool $_axis(xgrid)
	    SendCmd "axis grid x $bool"
        }
        "axis-ygrid" {
	    set bool $_axis(ygrid)
	    SendCmd "axis grid y $bool"
        }
        "axis-zgrid" {
	    set bool $_axis(zgrid)
	    SendCmd "axis grid z $bool"
        }
        "axis-mode" {
	    set mode [$itk_component(axismode) value]
	    set mode [$itk_component(axismode) translate $mode]
	    SendCmd "axis flymode $mode"
        }
	"xcutplane-visible" - "ycutplane-visible" - "zcutplane-visible" {
	    set axis [string range $what 0 0]
	    set bool $_cutplane(${axis}visible)
            if { $bool } {
                $itk_component(${axis}CutScale) configure -state normal \
                    -troughcolor white
            } else {
                $itk_component(${axis}CutScale) configure -state disabled \
                    -troughcolor grey82
            }
	    SendCmd "cutplane axis $axis $bool"
	    SendCmd "cutplane colormode vmag"
	}
	"xcutplane-position" - "ycutplane-position" - "zcutplane-position" {
	    set axis [string range $what 0 0]
	    set pos [expr $_cutplane(${axis}position) * 0.01]
	    SendCmd "cutplane slice ${axis} ${pos}"
	    set _cutplanePending 0
	}
        "streamlines-seeds" {
	    set bool $_streamlines(seeds)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "streamlines seed visible $bool $dataset"
	    }
        }
        "streamlines-seeddensity" {
	    set density [expr int($_streamlines(seeddensity) * 5.0)]
	    foreach dataset [CurrentDatasets -visible $_first] {
                # This command works for either random or fmesh seeds
                SendCmd "streamlines seed numpts $density $dataset"
	    }
        }
        "streamlines-visible" {
	    set bool $_streamlines(visible)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "streamlines visible $bool $dataset"
            }
	    if { $bool } {
		Rappture::Tooltip::for $itk_component(streamlines) \
		    "Hide the streamlines"
	    } else {
		Rappture::Tooltip::for $itk_component(streamlines) \
		    "Show the streamlines"
	    }
        }
        "streamlines-mode" {
	    set mode [$itk_component(streammode) value]
	    foreach dataset [CurrentDatasets -visible $_first] {
		switch -- $mode {
		    "lines" {
			SendCmd "streamlines lines $dataset"
		    }
		    "ribbons" {
			SendCmd "streamlines ribbons 1 0 $dataset"
		    }
		    "tubes" {
			SendCmd "streamlines tubes 5 1 $dataset"
		    }
		}
            }
        }
        "streamlines-palette" {
	    set palette [$itk_component(palette) value]
	    foreach dataset [CurrentDatasets -visible $_first] {
		foreach {dataobj comp} [split $dataset -] break
		ChangeColormap $dataobj $comp $palette
            }
        }
        "streamlines-opacity" {
	    set val $_streamlines(opacity)
	    set sval [expr { 0.01 * double($val) }]
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "streamlines opacity $sval $dataset"
	    }
        }
        "streamlines-scale" {
	    set val $_streamlines(scale)
	    set sval [expr { 0.01 * double($val) }]
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "streamlines scale $sval $sval $sval $dataset"
	    }
        }
        "streamlines-lighting" {
	    set bool $_streamlines(lighting)
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "streamlines lighting $bool $dataset"
            }
        }
        "streamlines-field" {
	    set field [$itk_component(field) value]
	    set mode scalar
	    if { $field == "U" } {
		set mode vmag
	    }
	    foreach dataset [CurrentDatasets -visible $_first] {
		SendCmd "dataset scalar ${field} $dataset"
		SendCmd "streamlines colormode ${mode} $dataset"
		SendCmd "streamlines colormode ${mode} $dataset"
		#SendCmd "dataset vector ${field} $dataset"
            }
	    RequestLegend $mode
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

#
# RequestLegend --
#
#	Request a new legend from the server.  The size of the legend
#	is determined from the height of the canvas.  It will be rotated
#	to be vertical when drawn.
#
itcl::body Rappture::VtkStreamlinesViewer::RequestLegend { {mode vmag} } {
    #puts stderr "RequestLegend _first=$_first"
    #puts stderr "RequestLegend width=$_width height=$_height"
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set c $itk_component(legend)
    set w 12
    set h [expr {$_height - 3 * ($lineht + 2)}]
    if { $h < 1} {
	return
    }
    # Set the legend on the first streamlines dataset.
    foreach dataset [CurrentDatasets -visible] {
	foreach {dataobj comp} [split $dataset -] break
	if { [info exists _dataset2style($dataset)] } {
	    #puts stderr "RequestLegend w=$w h=$h"
            SendCmd "legend $_dataset2style($dataset) $mode {} $w $h 0"
	    break;
        }
    }
}

#
# ChangeColormap --
#
itcl::body Rappture::VtkStreamlinesViewer::ChangeColormap {dataobj comp color} {
    set tag $dataobj-$comp
    if { ![info exist _styles($tag)] } {
	error "no initial colormap"
    }
    set _style(-color) $color 
    SetColormap $dataobj $comp
}

#
# SetColormap --
#
itcl::body Rappture::VtkStreamlinesViewer::SetColormap { dataobj comp } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
    }
    set tag $dataobj-$comp
    if { ![info exists _initialStyle($tag)] } {
	# Save the initial component style.
	set _initialStyle($tag) [$dataobj style $comp]
    }

    # Override defaults with initial style defined in xml.
    array set style $_initialStyle($tag)

    if { ![info exists _style($tag)] } {
	set _style($tag) [array get style]
    }
    # Override initial style with current style.
    array set style $_style($tag)

    set name "$style(-color):$style(-levels):$style(-opacity)"
    if { ![info exists _colormaps($name)] } {
	BuildColormap $name [array get style]
	set _colormaps($name) 1
    }
    if { ![info exists _dataset2style($tag)] ||
	 $_dataset2style($tag) != $name } {
	SendCmd "streamlines colormap $name $tag"
	SendCmd "cutplane colormap $name $tag"
    }
}

itcl::body Rappture::VtkStreamlinesViewer::ColorsToColormap { colors } {
    switch -- $colors {
	"blue-to-gray" {
	    return {
		0.0 0.000 0.600 0.800 
		0.14285714285714285 0.400 0.900 1.000 
		0.2857142857142857 0.600 1.000 1.000 
		0.42857142857142855 0.800 1.000 1.000 
		0.5714285714285714 0.900 0.900 0.900 
		0.7142857142857143 0.600 0.600 0.600 
		0.8571428571428571 0.400 0.400 0.400 
		1.0 0.200 0.200 0.200
	    }
	}
	"blue" {
	    return { 
		0.0 0.900 1.000 1.000 
		0.1111111111111111 0.800 0.983 1.000 
		0.2222222222222222 0.700 0.950 1.000 
		0.3333333333333333 0.600 0.900 1.000 
		0.4444444444444444 0.500 0.833 1.000 
		0.5555555555555556 0.400 0.750 1.000 
		0.6666666666666666 0.300 0.650 1.000 
		0.7777777777777778 0.200 0.533 1.000 
		0.8888888888888888 0.100 0.400 1.000 
		1.0 0.000 0.250 1.000
	    }
	}
	"blue-to-brown" {
	    return {
		0.0 		    		0.200   0.100   0.000 
		0.09090909090909091 		0.400   0.187   0.000 
		0.18181818181818182 		0.600   0.379   0.210 
		0.2727272727272727  		0.800   0.608   0.480 
		0.36363636363636365 		0.850   0.688   0.595 
		0.45454545454545453 		0.950   0.855   0.808 
		0.5454545454545454  		0.800   0.993   1.000 
		0.6363636363636364  		0.600   0.973   1.000 
		0.7272727272727273  		0.400   0.940   1.000 
		0.8181818181818182  		0.200   0.893   1.000 
		0.9090909090909091  		0.000   0.667   0.800 
		1.0                 		0.000   0.480   0.600 
	    }
	}
	"blue-to-orange" {
	    return {
		0.0 		    		0.000   0.167   1.000
		0.09090909090909091 		0.100   0.400   1.000
		0.18181818181818182 		0.200   0.600   1.000
		0.2727272727272727  		0.400   0.800   1.000
		0.36363636363636365 		0.600   0.933   1.000
		0.45454545454545453 		0.800   1.000   1.000
		0.5454545454545454  		1.000   1.000   0.800
		0.6363636363636364  		1.000   0.933   0.600
		0.7272727272727273  		1.000   0.800   0.400
		0.8181818181818182  		1.000   0.600   0.200
		0.9090909090909091  		1.000   0.400   0.100
		1.0                 		1.000   0.167   0.000
	    }
	}
	"rainbow" {
	    set clist {
		"red" "#FFA500" "yellow" "#008000" "blue" "#4B0082" "#EE82EE"
	    }
	}
	"typical" {
	    set clist {
		"red" "#FFA500" "yellow" "#008000" "blue" 
	    }
	}
	"greyscale" {
	    return { 
		0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0
	    }
	}
	"nanohub" {
	    set clist "white yellow green cyan blue magenta"
	}
	default {
	    set clist $colors
	}
    }
    set cmap {}
    for {set i 0} {$i < [llength $clist]} {incr i} {
        set x [expr {double($i)/([llength $clist]-1)}]
        set color [lindex $clist $i]
        append cmap "$x [Color2RGB $color] "
    }
    return $cmap
}

#
# BuildColormap --
#
itcl::body Rappture::VtkStreamlinesViewer::BuildColormap { name styles } {
    array set style $styles
    set cmap [ColorsToColormap $style(-color)]
    if { [llength $cmap] == 0 } {
	set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    if { ![info exists _volume(opacity)] } {
        set _volume(opacity) $style(-opacity)
    }
    set max $_volume(opacity)

    set wmap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $wmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkStreamlinesViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkStreamlinesViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::VtkStreamlinesViewer::limits { dataobj } {
    array unset _limits $dataobj-*
    foreach comp [$dataobj components] {
	set tag $dataobj-$comp
	if { ![info exists _limits($tag)] } {
	    set data [$dataobj blob $comp]
	    set tmpfile file[pid].vtk
	    set f [open "$tmpfile" "w"]
	    puts $f $data 
	    close $f
	    set reader [vtkDataSetReader $tag-xvtkDataSetReader]
	    $reader SetFileName $tmpfile
	    $reader ReadAllScalarsOn
	    $reader ReadAllVectorsOn
	    $reader ReadAllFieldsOn
	    $reader Update
	    set output [$reader GetOutput]
	    set _limits($tag) [$output GetBounds]
	    set pointData [$output GetPointData]
	    puts stderr "\#scalars=[$reader GetNumberOfScalarsInFile]"
	    puts stderr "\#fielddata=[$reader GetNumberOfFieldDataInFile]"
	    puts stderr "fielddataname=[$reader GetFieldDataNameInFile 0]"
	    set fieldData [$output GetFieldData]
	    set pointData [$output GetPointData]
	    puts stderr "field \#arrays=[$fieldData GetNumberOfArrays]"
	    for { set i 0 } { $i < [$fieldData GetNumberOfArrays] } { incr i } {
		puts stderr [$fieldData GetArrayName $i]
	    }
	    puts stderr "point \#arrays=[$pointData GetNumberOfArrays]"
	    for { set i 0 } { $i < [$pointData GetNumberOfArrays] } { incr i } {
		set name [$pointData GetArrayName $i]
		if { ![info exists _fields($name)] } {
		    $itk_component(field) choices insert end "$name" "$name"
		    set _fields($name) 1
		}
	    }
	    puts stderr "field \#components=[$fieldData GetNumberOfComponents]"
	    puts stderr "point \#components=[$pointData GetNumberOfComponents]"
	    puts stderr "field \#tuples=[$fieldData GetNumberOfTuples]"
	    puts stderr "point \#tuples=[$pointData GetNumberOfTuples]"
	    puts stderr "point \#scalars=[$pointData GetScalars]"
	    puts stderr vectors=[$pointData GetVectors]
	    rename $output ""
	    rename $reader ""
	    file delete $tmpfile
	}
        foreach { xMin xMax yMin yMax zMin zMax} $_limits($tag) break
	if {![info exists limits(xmin)] || $limits(xmin) > $xMin} {
	    set limits(xmin) $xMin
	}
	if {![info exists limits(xmax)] || $limits(xmax) < $xMax} {
	    set limits(xmax) $xMax
	}
	if {![info exists limits(ymin)] || $limits(ymin) > $yMin} {
	    set limits(ymin) $xMin
	}
	if {![info exists limits(ymax)] || $limits(ymax) < $yMax} {
	    set limits(ymax) $yMax
	}
	if {![info exists limits(zmin)] || $limits(zmin) > $zMin} {
	    set limits(zmin) $zMin
	}
	if {![info exists limits(zmax)] || $limits(zmax) < $zMax} {
	    set limits(zmax) $zMax
	}
    }
    return [array get limits]
}

itcl::body Rappture::VtkStreamlinesViewer::BuildVolumeTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.volume \
        -text "Show Volume" \
        -variable [itcl::scope _volume(visible)] \
        -command [itcl::code $this AdjustSetting volume-visible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _volume(wireframe)] \
        -command [itcl::code $this AdjustSetting volume-wireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _volume(lighting)] \
        -command [itcl::code $this AdjustSetting volume-lighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _volume(edges)] \
        -command [itcl::code $this AdjustSetting volume-edges] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _volume(opacity)] \
        -width 10 \
        -showvalue off \
	-command [itcl::code $this AdjustSetting volume-opacity]

    blt::table $inner \
        0,0 $inner.volume    -anchor w -pady 2 \
        1,0 $inner.wireframe -anchor w -pady 2 \
        2,0 $inner.lighting  -anchor w -pady 2 \
        3,0 $inner.edges     -anchor w -pady 2 \
        4,0 $inner.opacity_l -anchor w -pady 2 \
        5,0 $inner.opacity   -fill x   -pady 2 

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r6 c1 -resize expand
}


itcl::body Rappture::VtkStreamlinesViewer::BuildStreamsTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Streams Settings" \
        -icon [Rappture::icon streamlines-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.streamlines \
        -text "Show Streamlines" \
        -variable [itcl::scope _streamlines(visible)] \
        -command [itcl::code $this AdjustSetting streamlines-visible] \
        -font "Arial 9"
    
    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _streamlines(lighting)] \
        -command [itcl::code $this AdjustSetting streamlines-lighting] \
        -font "Arial 9"

    checkbutton $inner.seeds \
        -text "Show Seeds" \
        -variable [itcl::scope _streamlines(seeds)] \
        -command [itcl::code $this AdjustSetting streamlines-seeds] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9" 
    itk_component add streammode {
	Rappture::Combobox $inner.mode -width 10 -editable no
    }
    $inner.mode choices insert end \
        "lines"    "lines" \
        "ribbons"   "ribbons" \
        "tubes"     "tubes" 
    $itk_component(streammode) value "lines"
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting streamlines-mode]

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _streamlines(opacity)] \
        -width 10 \
        -showvalue off \
	-command [itcl::code $this AdjustSetting streamlines-opacity]

    label $inner.density_l -text "Seed Density" -font "Arial 9"
    ::scale $inner.density -from 1 -to 100 -orient horizontal \
        -variable [itcl::scope _streamlines(seeddensity)] \
        -width 10 \
        -showvalue off \
	-command [itcl::code $this AdjustSetting streamlines-seeddensity]

    label $inner.scale_l -text "Scale" -font "Arial 9"
    ::scale $inner.scale -from 1 -to 100 -orient horizontal \
        -variable [itcl::scope _streamlines(scale)] \
        -width 10 \
        -showvalue off \
	-command [itcl::code $this AdjustSetting streamlines-scale]

    label $inner.field_l -text "Field" -font "Arial 9" 
    itk_component add field {
	Rappture::Combobox $inner.field -width 10 -editable no
    }
    bind $inner.field <<Value>> \
	[itcl::code $this AdjustSetting streamlines-field]

    label $inner.palette_l -text "Color Palette" -font "Arial 9" 
    itk_component add palette {
	Rappture::Combobox $inner.palette -width 10 -editable no
    }
    $inner.palette choices insert end \
	"blue-to-gray" 	     "blue-to-gray" 	\
	"blue" 		     "blue" 		\
	"blue-to-brown"      "blue-to-brown"    \
	"blue-to-orange"     "blue-to-orange"   \
	"rainbow" 	     "rainbow"		\
	"typical" 	     "typical"		\
	"greyscale" 	     "greyscale" 	\
	"nanohub"            "nanohub"          
    $itk_component(palette) value "typical"
    bind $inner.palette <<Value>> \
	[itcl::code $this AdjustSetting streamlines-palette]

    blt::table $inner \
        0,0 $inner.streamlines -anchor w -pady 2 -cspan 2 \
        1,0 $inner.lighting    -anchor w -pady 2 -cspan 2 \
        2,0 $inner.seeds       -anchor w -pady 2 -cspan 2 \
        3,0 $inner.density_l   -anchor w -pady 2 -cspan 2 \
        4,0 $inner.density     -fill x   -pady 2 -cspan 2 \
        5,0 $inner.mode_l      -anchor w -pady 2  \
        5,1 $inner.mode        -anchor w -pady 2  \
        6,0 $inner.opacity_l   -anchor w -pady 2 -cspan 2 \
        7,0 $inner.opacity     -fill x   -pady 2 -cspan 2 \
        8,0 $inner.field_l     -anchor w -pady 2  \
        8,1 $inner.field       -anchor w -pady 2  \
        9,0 $inner.palette_l   -anchor w -pady 2  \
        9,1 $inner.palette     -anchor w -pady 2  \
	

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r10 c1 c2 -resize expand
}

itcl::body Rappture::VtkStreamlinesViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon axis1]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Show Axes" \
        -variable [itcl::scope _axis(visible)] \
        -command [itcl::code $this AdjustSetting axis-visible] \
        -font "Arial 9"

    checkbutton $inner.labels \
        -text "Show Axis Labels" \
        -variable [itcl::scope _axis(labels)] \
        -command [itcl::code $this AdjustSetting axis-labels] \
        -font "Arial 9"

    checkbutton $inner.gridx \
        -text "Show X Grid" \
        -variable [itcl::scope _axis(xgrid)] \
        -command [itcl::code $this AdjustSetting axis-xgrid] \
        -font "Arial 9"
    checkbutton $inner.gridy \
        -text "Show Y Grid" \
        -variable [itcl::scope _axis(ygrid)] \
        -command [itcl::code $this AdjustSetting axis-ygrid] \
        -font "Arial 9"
    checkbutton $inner.gridz \
        -text "Show Z Grid" \
        -variable [itcl::scope _axis(zgrid)] \
        -command [itcl::code $this AdjustSetting axis-zgrid] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9" 

    itk_component add axismode {
	Rappture::Combobox $inner.mode -width 10 -editable no
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "furthest" \
        "outer_edges"     "outer"         
    $itk_component(axismode) value "static"
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting axis-mode]

    blt::table $inner \
        0,0 $inner.visible -anchor w -cspan 2 \
        1,0 $inner.labels  -anchor w -cspan 2 \
        2,0 $inner.gridx   -anchor w -cspan 2 \
        3,0 $inner.gridy   -anchor w -cspan 2 \
        4,0 $inner.gridz   -anchor w -cspan 2 \
        5,0 $inner.mode_l  -anchor w -cspan 2 -padx { 2 0 } \
        6,0 $inner.mode    -fill x   -cspan 2 

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c1 -resize expand
}


itcl::body Rappture::VtkStreamlinesViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set labels { qx qy qz qw xpan ypan zoom }
    set row 0
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _view($tag)]
        bind $inner.${tag} <KeyPress-Return> \
            [itcl::code $this camera set ${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }
    checkbutton $inner.ortho \
        -text "Orthographic Projection" \
        -variable [itcl::scope _view(ortho)] \
        -command [itcl::code $this camera set ortho] \
        -font "Arial 9"
    blt::table $inner \
            $row,0 $inner.ortho -columnspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::VtkStreamlinesViewer::BuildCutplaneTab {} {

    set fg [option get $itk_component(hull) font Font]
    
    set inner [$itk_component(main) insert end \
        -title "Cutplane Along Axis" \
	-icon [Rappture::icon cutbutton]] 

    $inner configure -borderwidth 4

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane] \
            -offimage [Rappture::icon x-cutplane] \
            -command [itcl::code $this AdjustSetting xcutplane-visible] \
            -variable [itcl::scope _cutplane(xvisible)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X-axis cutplane on/off"

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 1 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane x] \
            -variable [itcl::scope _cutplane(xposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    # Set the default cutplane value before disabling the scale.
    $itk_component(xCutScale) set 100
    $itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this Slice tooltip x]"

    # Y-value slicer...
    itk_component add yCutButton {
        Rappture::PushButton $inner.ybutton \
            -onimage [Rappture::icon y-cutplane] \
            -offimage [Rappture::icon y-cutplane] \
            -command [itcl::code $this AdjustSetting ycutplane-visible] \
            -variable [itcl::scope _cutplane(yvisible)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y-axis cutplane on/off"

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 1 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane y] \
            -variable [itcl::scope _cutplane(yposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this Slice tooltip y]"
    # Set the default cutplane value before disabling the scale.
    $itk_component(yCutScale) set 100
    $itk_component(yCutScale) configure -state disabled

    # Z-value slicer...
    itk_component add zCutButton {
        Rappture::PushButton $inner.zbutton \
            -onimage [Rappture::icon z-cutplane] \
            -offimage [Rappture::icon z-cutplane] \
            -command [itcl::code $this AdjustSetting zcutplane-visible] \
            -variable [itcl::scope _cutplane(zvisible)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z-axis cutplane on/off"

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 1 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane z] \
            -variable [itcl::scope _cutplane(zposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    $itk_component(zCutScale) set 100
    $itk_component(zCutScale) configure -state disabled
    #$itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this Slice tooltip z]"

    blt::table $inner \
        0,0 $itk_component(xCutButton)  -anchor e -padx 2 -pady 2 \
        1,0 $itk_component(xCutScale)   -fill y \
        0,1 $itk_component(yCutButton)  -anchor e -padx 2 -pady 2 \
        1,1 $itk_component(yCutScale)   -fill y \
        0,2 $itk_component(zCutButton)  -anchor e -padx 2 -pady 2 \
        1,2 $itk_component(zCutScale)   -fill y \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r1 c3 -resize expand
}



#
#  camera -- 
#
itcl::body Rappture::VtkStreamlinesViewer::camera {option args} {
    switch -- $option { 
        "show" {
            puts [array get _view]
        }
        "set" {
            set who [lindex $args 0]
            set x $_view($who)
            set code [catch { string is double $x } result]
            if { $code != 0 || !$result } {
                return
            }
            switch -- $who {
                "ortho" {
                    if {$_view(ortho)} {
                        SendCmd "camera mode ortho"
                    } else {
                        SendCmd "camera mode persp"
                    }
                }
                "xpan" - "ypan" {
                    PanCamera
                }
                "qx" - "qy" - "qz" - "qw" {
                    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
		    $_arcball quaternion $q
		    EventuallyRotate $q
                }
                "zoom" {
                    SendCmd "camera zoom $_view(zoom)"
                }
            }
        }
    }
}

itcl::body Rappture::VtkStreamlinesViewer::ConvertToVtkData { dataobj comp } {
    foreach { x1 x2 xN y1 y2 yN } [$dataobj mesh $comp] break
    set values [$dataobj values $comp]
    append out "# vtk DataFile Version 2.0 \n"
    append out "Test data \n"
    append out "ASCII \n"
    append out "DATASET STRUCTURED_POINTS \n"
    append out "DIMENSIONS $xN $yN 1 \n"
    append out "ORIGIN 0 0 0 \n"
    append out "SPACING 1 1 1 \n"
    append out "POINT_DATA [expr $xN * $yN] \n"
    append out "SCALARS field float 1 \n"
    append out "LOOKUP_TABLE default \n"
    append out [join $values "\n"]
    append out "\n"
    return $out
}


itcl::body Rappture::VtkStreamlinesViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
	    #set contents [ConvertToVtkData $dataobj $comp]
	    set contents [$dataobj blob $comp]
	    append bytes "$contents\n\n"
        }
    }
    return [list .vtk $bytes]
}

itcl::body Rappture::VtkStreamlinesViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
	 [image height $_image(download)] > 0 } {
	set bytes [$_image(download) data -format "jpeg -quality 100"]
	set bytes [Rappture::encoding::decode -as b64 $bytes]
	return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkStreamlinesViewer::BuildDownloadPopup { popup command } {
    Rappture::Balloon $popup \
        -title "[Rappture::filexfer::label downloadWord] as..."
    set inner [$popup component inner]
    label $inner.summary -text "" -anchor w 
    radiobutton $inner.vtk_button -text "VTK data file" \
        -variable [itcl::scope _downloadPopup(format)] \
        -font "Helvetica 9 " \
        -value vtk  
    Rappture::Tooltip::for $inner.vtk_button "Save as VTK data file."
    radiobutton $inner.image_button -text "Image File" \
        -variable [itcl::scope _downloadPopup(format)] \
        -value image 
    Rappture::Tooltip::for $inner.image_button \
        "Save as digital image."

    button $inner.ok -text "Save" \
	-highlightthickness 0 -pady 2 -padx 3 \
        -command $command \
	-compound left \
	-image [Rappture::icon download]

    button $inner.cancel -text "Cancel" \
	-highlightthickness 0 -pady 2 -padx 3 \
	-command [list $popup deactivate] \
	-compound left \
	-image [Rappture::icon cancel]

    blt::table $inner \
        0,0 $inner.summary -cspan 2  \
        1,0 $inner.vtk_button -anchor w -cspan 2 -padx { 4 0 } \
        2,0 $inner.image_button -anchor w -cspan 2 -padx { 4 0 } \
        4,1 $inner.cancel -width .9i -fill y \
        4,0 $inner.ok -padx 2 -width .9i -fill y 
    blt::table configure $inner r3 -height 4
    blt::table configure $inner r4 -pady 4
    raise $inner.image_button
    $inner.vtk_button invoke
    return $inner
}

itcl::body Rappture::VtkStreamlinesViewer::SetObjectStyle { dataobj comp } {
    # Parse style string.
    set tag $dataobj-$comp
    set style [$dataobj style $comp]
    #puts stderr "style $dataobj-$comp \"$style\""
    if { $dataobj != $_first } {
	set settings(-wireframe) 1
    }
    array set settings {
	-color \#808080
	-edges 0
	-edgecolor black
	-linewidth 1.0
	-opacity 0.4
	-wireframe 0
	-lighting 1
	-seeds 1
	-seedcolor white
	-visible 1
    }
    array set settings $style
    SendCmd "streamlines add $tag"
    SendCmd "streamlines seed visible off $tag"
    set seeds [$dataobj hints seeds]
    if { $seeds != "" && ![info exists _seeds($dataobj)] } {
	set length [string length $seeds]
	SendCmd "streamlines seed fmesh 200 data follows $length $tag"
	SendCmd "$seeds"
	set _seeds($dataobj) 1
    }
    SendCmd "cutplane add $tag"
    SendCmd "cutplane colormode vmag $tag"
    SendCmd "cutplane edges 0 $tag"
    SendCmd "cutplane wireframe 0 $tag"
    SendCmd "cutplane lighting 1 $tag"
    SendCmd "cutplane linewidth 1 $tag"
    #SendCmd "cutplane linecolor 1 1 1 $tag"
    #puts stderr "cutplane axis $axis $bool"
    #SendCmd "cutplane visible $tag"
    SendCmd "cutplane colormode vmag $tag"
    foreach axis { x y z } {
	SendCmd "cutplane slice $axis 1.0 $tag"
	SendCmd "cutplane axis $axis 0 $tag"
    }

    SendCmd "polydata add $tag"
    SendCmd "polydata edges $settings(-edges) $tag"
    set _volume(edges) $settings(-edges)
    SendCmd "polydata color [Color2RGB $settings(-color)] $tag"
    SendCmd "polydata lighting $settings(-lighting) $tag"
    set _volume(lighting) $settings(-lighting)
    SendCmd "polydata linecolor [Color2RGB $settings(-edgecolor)] $tag"
    SendCmd "polydata linewidth $settings(-linewidth) $tag"
    SendCmd "polydata opacity $settings(-opacity) $tag"
    set _volume(opacity) $settings(-opacity)
    SendCmd "polydata wireframe $settings(-wireframe) $tag"
    set _volume(wireframe) $settings(-wireframe)
    set _volume(opacity) [expr $settings(-opacity) * 100.0]
    SetColormap $dataobj $comp
}

itcl::body Rappture::VtkStreamlinesViewer::IsValidObject { dataobj } {
    if {[catch {$dataobj isa Rappture::Field} valid] != 0 || !$valid} {
	return 0
    }
    return 1
}

# ----------------------------------------------------------------------
# USAGE: ReceiveLegend <colormap> <title> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::ReceiveLegend { colormap title vmin vmax size } {
    #puts stderr "ReceiveLegend colormap=$colormap title=$title range=$vmin,$vmax size=$size"
    set _limits(vmin) $vmin
    set _limits(vmax) $vmax
    set _title $title
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend)] } {
            set _image(legend) [image create photo]
        }
        $_image(legend) configure -data $bytes
        #puts stderr "read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
	DrawLegend $title
    }
}

#
# DrawLegend --
#
#	Draws the legend in it's own canvas which resides to the right
#	of the contour plot area.
#
itcl::body Rappture::VtkStreamlinesViewer::DrawLegend { title } {
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    
    regsub {\(mag\)} $title "" title 
    if { $_settings(legend) } {
	set x [expr $w - 2]
	if { [$c find withtag "legend"] == "" } {
	    set y 2 
	    $c create text $x $y \
		-anchor ne \
		-fill $itk_option(-plotforeground) -tags "title legend" \
		-font $font
	    incr y $lineht
	    $c create text $x $y \
		-anchor ne \
		-fill $itk_option(-plotforeground) -tags "vmax legend" \
		-font $font
	    incr y $lineht
	    $c create image $x $y \
		-anchor ne \
		-image $_image(legend) -tags "colormap legend"
	    $c create text $x [expr {$h-2}] \
		-anchor se \
		-fill $itk_option(-plotforeground) -tags "vmin legend" \
		-font $font
	    #$c bind colormap <Enter> [itcl::code $this EnterLegend %x %y]
	    $c bind colormap <Leave> [itcl::code $this LeaveLegend]
	    $c bind colormap <Motion> [itcl::code $this MotionLegend %x %y]
	}
	# Reset the item coordinates according the current size of the plot.
	$c itemconfigure title -text $title
	if { $_limits(vmin) != "" } {
	    $c itemconfigure vmin -text [format %g $_limits(vmin)]
	}
	if { $_limits(vmax) != "" } {
	    $c itemconfigure vmax -text [format %g $_limits(vmax)]
	}
	set y 2
	$c coords title $x $y
	incr y $lineht
	$c coords vmax $x $y
	incr y $lineht
	$c coords colormap $x $y
	$c coords vmin $x [expr {$h - 2}]
    }
}

#
# EnterLegend --
#
itcl::body Rappture::VtkStreamlinesViewer::EnterLegend { x y } {
    SetLegendTip $x $y
}

#
# MotionLegend --
#
itcl::body Rappture::VtkStreamlinesViewer::MotionLegend { x y } {
    Rappture::Tooltip::tooltip cancel
    set c $itk_component(view)
    SetLegendTip $x $y
}

#
# LeaveLegend --
#
itcl::body Rappture::VtkStreamlinesViewer::LeaveLegend { } {
    Rappture::Tooltip::tooltip cancel
    .rappturetooltip configure -icon ""
}

#
# SetLegendTip --
#
itcl::body Rappture::VtkStreamlinesViewer::SetLegendTip { x y } {
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    
    set imgHeight [image height $_image(legend)]
    set coords [$c coords colormap]
    set imgX [expr $w - [image width $_image(legend)] - 2]
    set imgY [expr $y - 2 * ($lineht + 2)]

    # Make a swatch of the selected color
    if { [catch { $_image(legend) get 10 $imgY } pixel] != 0 } {
	#puts stderr "out of range: $imgY"
	return
    }
    if { ![info exists _image(swatch)] } {
	set _image(swatch) [image create photo -width 24 -height 24]
    }
    set color [eval format "\#%02x%02x%02x" $pixel]
    $_image(swatch) put black  -to 0 0 23 23 
    $_image(swatch) put $color -to 1 1 22 22 
    .rappturetooltip configure -icon $_image(swatch)

    # Compute the value of the point
    set t [expr 1.0 - (double($imgY) / double($imgHeight-1))]
    #puts stderr "t=$t x=$x y=$y imgY=$imgY"
    set value [expr $t * ($_limits(vmax) - $_limits(vmin)) + $_limits(vmin)]
    set tipx [expr $x + 15] 
    set tipy [expr $y - 5]
    #puts stderr "tipx=$tipx tipy=$tipy x=$x y=$y"
    Rappture::Tooltip::text $c "$_title $value"
    Rappture::Tooltip::tooltip show $c +$tipx,+$tipy    
}


# ----------------------------------------------------------------------
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::Slice {option args} {
    switch -- $option {
        "move" {
            set axis [lindex $args 0]
            set oldval $_axis(${axis}position)
            set newval [lindex $args 1]
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set newpos [expr {0.01*$newval}]
            SendCmd "cutplane slice $axis $newpos"
        }
	"tooltip" {
	    set axis [lindex $args 0]
	    set val [$itk_component(${axis}CutScale) get]
	    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
	}
        default {
            error "bad option \"$option\": should be axis, move, or tooltip"
        }
    }
}
