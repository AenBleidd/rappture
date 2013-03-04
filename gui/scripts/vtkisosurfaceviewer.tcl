# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: vtkisolinesviewer - Vtk 3D contour object viewer
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

option add *VtkIsosurfaceViewer.width 4i widgetDefault
option add *VtkIsosurfaceViewer*cursor crosshair widgetDefault
option add *VtkIsosurfaceViewer.height 4i widgetDefault
option add *VtkIsosurfaceViewer.foreground black widgetDefault
option add *VtkIsosurfaceViewer.controlBackground gray widgetDefault
option add *VtkIsosurfaceViewer.controlDarkBackground #999999 widgetDefault
option add *VtkIsosurfaceViewer.plotBackground black widgetDefault
option add *VtkIsosurfaceViewer.plotForeground white widgetDefault
option add *VtkIsosurfaceViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkIsosurfaceViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkIsosurfaceViewer::SetServerList
}

itcl::class Rappture::VtkIsosurfaceViewer {
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
    protected method InitSettings { args  }
    protected method Pan {option x y}
    protected method Pick {x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { colormap title vmin vmax size }
    protected method Rotate {option x y}
    protected method Zoom {option}

    # The following methods are only used by this class.
    private method BuildAxisTab {}
    private method BuildCameraTab {}
    private method BuildColormap { name colors }
    private method BuildCutplaneTab {}
    private method BuildDownloadPopup { widget command } 
    private method BuildIsosurfaceTab {}
    private method DrawLegend {}
    private method Combo { option }
    private method EnterLegend { x y } 
    private method EventuallyResize { w h } 
    private method EventuallyRotate { q } 
    private method EventuallyRequestLegend {} 
    private method EventuallySetCutplane { axis args } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method IsValidObject { dataobj } 
    private method LeaveLegend {}
    private method MotionLegend { x y } 
    private method PanCamera {}
    private method RequestLegend {}
    private method SetLegendTip { x y }
    private method SetObjectStyle { dataobj comp } 
    private method Slice {option args} 
    private method ResetColormap { color }
    private method SetCurrentColormap { stylelist }

    private variable _arcball ""

    private variable _dlist ""     ;    # list of data objects
    private variable _obj2datasets
    private variable _obj2ovride   ;    # maps dataobj => style override
    private variable _datasets     ;    # contains all the dataobj-component 
                                   ;    # datasets in the server
    private variable _colormaps    ;    # contains all the colormaps
                                   ;    # in the server.
    # The name of the current colormap used.  The colormap is global to all
    # heightmaps displayed.
    private variable _currentColormap "" ;    


    private variable _dataset2style    ;# maps dataobj-component to transfunc

    private variable _click        ;    # info used for rotate operations
    private variable _limits       ;    # autoscale min/max for all axes
    private variable _view         ;    # view params for 3D view
    private variable _settings
    private variable _style;            # Array of current component styles.
    private variable _initialStyle;     # Array of initial component styles.
    private variable _reset 1;          # indicates if camera needs to be reset
                                        # to starting position.

    private variable _first ""     ;    # This is the topmost dataset.
    private variable _start 0
    private variable _title ""
    private variable _isolines

    common _downloadPopup;              # download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _rotatePending 0
    private variable _cutplanePending 0
    private variable _legendPending 0
    private variable _field      ""
    private variable _colorMode "vmag";	#  Mode of colormap (vmag or scalar)
    private variable _fieldNames {} 
    private variable _fields 
    private variable _curFldName ""
    private variable _curFldLabel ""
    private variable _numContours 10
}

itk::usual VtkIsosurfaceViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::constructor {hostlist args} {
    package require vtk
    set _serverType "vtkvis"

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this RequestLegend]; list"

    # X-Cutplane event
    $_dispatcher register !xcutplane
    $_dispatcher dispatch $this !xcutplane \
        "[itcl::code $this AdjustSetting cutplaneXPosition]; list"

    # Y-Cutplane event
    $_dispatcher register !ycutplane
    $_dispatcher dispatch $this !ycutplane \
        "[itcl::code $this AdjustSetting cutplaneYPosition]; list"

    # Z-Cutplane event
    $_dispatcher register !zcutplane
    $_dispatcher dispatch $this !zcutplane \
        "[itcl::code $this AdjustSetting cutplaneZPosition]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias dataset [itcl::code $this ReceiveDataset]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias viserror [itcl::code $this ReceiveError]

    # Initialize the view to some default parameters.
    array set _view {
        qw              0.853553
        qx              -0.353553
        qy              0.353553
        qz              0.146447
        zoom            1.0 
        xpan            0
        ypan            0
        ortho           0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    array set _settings [subst {
	background		black
	colormapPalette		BCGYR
	colormapVisible		1
	field			"Default"
	isolineColor		white
        axesVisible		1
        axisLabelsVisible	1
        axisXGrid		0
        axisYGrid		0
        axisZGrid		0
        cutplaneEdges           0
        cutplaneLighting        1
        cutplaneOpacity		100
        cutplaneVisible		0
        cutplaneWireframe	0
        cutplaneXPosition	50
        cutplaneXVisible	1
        cutplaneYPosition	50
        cutplaneYVisible	1
        cutplaneZPosition	50
        cutplaneZVisible	1
        isosurfaceEdges         0
        isosurfaceLighting      1
        isosurfaceOpacity       60
        isosurfaceOutline       0
        isosurfaceVisible       1
        isosurfaceWireframe     0
        legendVisible		1
    }]

    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }

    itk_component add fieldmenu {
        menu $itk_component(plotarea).menu -bg black -fg white -relief flat \
            -tearoff no 
    } {
        usual
        ignore -background -foreground -relief -tearoff
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
    bind $c <Control-F1> [itcl::code $this ToggleConsole]

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

    itk_component add contour {
        Rappture::PushButton $f.contour \
            -onimage [Rappture::icon volume-on] \
            -offimage [Rappture::icon volume-off] \
            -variable [itcl::scope _settings(isosurfaceVisible)] \
            -command [itcl::code $this AdjustSetting isosurfaceVisible] 
    }
    $itk_component(contour) select
    Rappture::Tooltip::for $itk_component(contour) \
        "Don't display the isosurface"
    pack $itk_component(contour) -padx 2 -pady 2

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings(cutplaneVisible)] \
            -command [itcl::code $this AdjustSetting cutplaneVisible] 
    }
    Rappture::Tooltip::for $itk_component(cutplane) \
        "Show/Hide cutplanes"
    pack $itk_component(cutplane) -padx 2 -pady 2


    if { [catch {
        BuildIsosurfaceTab
        BuildCutplaneTab
        BuildAxisTab
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
    update
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::destructor {} {
    Disconnect
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::VtkIsosurfaceViewer::DoResize {} {
    if { $_width < 2 } {
        set _width 500
    }
    if { $_height < 2 } {
        set _height 500
    }
    set _start [clock clicks -milliseconds]
    SendCmd "screen size $_width $_height"

    EventuallyRequestLegend
    set _resizePending 0
}

itcl::body Rappture::VtkIsosurfaceViewer::DoRotate {} {
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::VtkIsosurfaceViewer::EventuallyRequestLegend {} {
    if { !$_legendPending } {
        set _legendPending 1
        $_dispatcher event -idle !legend
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 400 !resize
    }
}

set rotate_delay 100

itcl::body Rappture::VtkIsosurfaceViewer::EventuallyRotate { q } {
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
    if { !$_rotatePending } {
        set _rotatePending 1
        global rotate_delay 
        $_dispatcher event -after $rotate_delay !rotate
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::EventuallySetCutplane { axis args } {
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
itcl::body Rappture::VtkIsosurfaceViewer::add {dataobj {settings ""}} {
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
itcl::body Rappture::VtkIsosurfaceViewer::delete {args} {
    if { [llength $args] == 0} {
        set args $_dlist
    }
    # Delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if { $pos < 0 } {
            continue;                   # Don't know anything about it.
        }
        # Remove it from the dataobj list.
        set _dlist [lreplace $_dlist $pos $pos]
        SendCmd "contour3d visible 0"
        array unset _obj2ovride $dataobj-*
        array unset _settings $dataobj-*
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
itcl::body Rappture::VtkIsosurfaceViewer::get {args} {
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
                    # No setting indicates that the object isn't visible.
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
itcl::body Rappture::VtkIsosurfaceViewer::scale { args } {
    foreach dataobj $args {
        foreach axis { x y z } {
            set lim [$dataobj limits $axis]
            if { ![info exists _limits($axis)] } {
                set _limits($axis) $lim
                continue
            }
            foreach {min max} $lim break
            foreach {amin amax} $_limits($axis) break
            if { $amin > $min } {
                set amin $min
            }
            if { $amax < $max } {
                set amax $max
            }
            set _limits($axis) [list $amin $amax]
        }
        foreach { fname lim } [$dataobj fieldlimits] {
            if { ![info exists _limits($fname)] } {
                set _limits($fname) $lim
                continue
            }
            foreach {min max} $lim break
            foreach {fmin fmax} $_limits($fname) break
            if { $fmin > $min } {
                set fmin $min
            }
            if { $fmax < $max } {
                set fmax $max
            }
            set _limits($fname) [list $fmin $fmax]
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
itcl::body Rappture::VtkIsosurfaceViewer::download {option args} {
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
            update idletasks            ;# Fix initial sizes
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
itcl::body Rappture::VtkIsosurfaceViewer::Connect {} {
    set _hosts [GetServerList "vtkvis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
    if { $result } {
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
itcl::body Rappture::VtkIsosurfaceViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkIsosurfaceViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkIsosurfaceViewer::Disconnect {} {
    VisViewer::Disconnect

    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    $_dispatcher cancel !xcutplane
    $_dispatcher cancel !ycutplane
    $_dispatcher cancel !zcutplane
    $_dispatcher cancel !legend
    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _datasets 
    array unset _data 
    array unset _colormaps 
    array unset _seeds 
    array unset _dataset2style 
    array unset _obj2datasets 
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::ReceiveImage { args } {
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
    EventuallyRequestLegend
}

#
# ReceiveDataset --
#
itcl::body Rappture::VtkIsosurfaceViewer::ReceiveDataset { args } {
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
itcl::body Rappture::VtkIsosurfaceViewer::Rebuild {} {
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
    StartBufferingCommands

    if { $_reset } {
        if { $_reportClientInfo }  {
            # Tell the server the name of the tool, the version, and dataset
            # that we are rendering.  Have to do it here because we don't know
            # what data objects are using the renderer until be get here.
            global env

            set info {}
            set user "???"
	    if { [info exists env(USER)] } {
                set user $env(USER)
	    }
            set session "???"
	    if { [info exists env(SESSION)] } {
                set session $env(SESSION)
	    }
            lappend info "hub" [exec hostname]
            lappend info "client" "vtkisosurfaceviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }
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
        set _first ""
        InitSettings axisXGrid axisYGrid axisZGrid axisMode \
            axesVisible axisLabelsVisible 
        
        SendCmd "imgflush"
    }
    set _first ""
    SendCmd "contour3d visible 0"
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        set _obj2datasets($dataobj) ""
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj vtkdata $comp]
		if 0 { 
		    set f [open "/tmp/isosurface.vtk" "w"]
		    puts $f $bytes
		    close $f
                }
                set length [string length $bytes]
                if { $_reportClientInfo }  {
                    set info {}
                    lappend info "tool_id"       [$dataobj hints toolId]
                    lappend info "tool_name"     [$dataobj hints toolName]
                    lappend info "tool_version"  [$dataobj hints toolRevision]
                    lappend info "tool_title"    [$dataobj hints toolTitle]
                    lappend info "dataset_label" [$dataobj hints label]
                    lappend info "dataset_size"  $length
                    lappend info "dataset_tag"   $tag
                    SendCmd "clientinfo [list $info]"
                }
                append _outbuf "dataset add $tag data follows $length\n"
                append _outbuf $bytes
                set _datasets($tag) 1
                SetObjectStyle $dataobj $comp
            }
            lappend _obj2datasets($dataobj) $tag
            if { [info exists _obj2ovride($dataobj-raise)] } {
		SendCmd "contour3d visible 1 $tag"
            }
        }
    }
    if { $_first != "" } {
	$itk_component(field) choices delete 0 end
	$itk_component(fieldmenu) delete 0 end
	array unset _fields
        set _curFldName ""
        foreach cname [$_first components] {
            foreach fname [$_first fieldnames $cname] {
                if { [info exists _fields($fname)] } {
                    continue
                }
                foreach { label units components } \
                    [$_first fieldinfo $fname] break
                $itk_component(field) choices insert end "$fname" "$label"
                $itk_component(fieldmenu) add radiobutton -label "$label" \
                    -value $label -variable [itcl::scope _curFldLabel] \
                    -selectcolor red \
                    -activebackground $itk_option(-plotbackground) \
                    -activeforeground $itk_option(-plotforeground) \
                    -font "Arial 8" \
                    -command [itcl::code $this Combo invoke]
                set _fields($fname) [list $label $units $components]
                if { $_curFldName == "" } {
                    set _curFldName $fname
                    set _curFldLabel $label
                }
            }
        }
        $itk_component(field) value $_curFldLabel
    }
    InitSettings isosurfaceVisible colormapPalette isosurfaceVisible 

    if { $_reset } {
	# These are settings that rely on a dataset being loaded.
        InitSettings \
            isosurfaceLighting \
            field \
            isosurfaceEdges isosurfaceLighting isosurfaceOpacity \
	    isosurfaceWireframe isosurfaceOutline \
	    cutplaneXPosition cutplaneYPosition cutplaneZPosition \
	    cutplaneXVisible cutplaneYVisible cutplaneZVisible \
	    cutplaneVisible
        Zoom reset
	foreach axis { x y z } {
            # Another problem fixed by a <view>. We looking into a data
            # object for the name of the axes. This should be global to
            # the viewer itself.
	    set label [$_first hints ${axis}label]
	    if { $label == "" } {
		if {$axis == "z"} {
                    if { $_curFldName == "component" } {
                        set label [string toupper $axis]
                    } else {
                        set label $_curFldLabel
                    }
		} else {
		    set label [string toupper $axis]
		}
	    }
	    # May be a space in the axis label.
	    SendCmd [list axis name $axis $label]
        }
        set _reset 0
    }

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    StopBufferingCommands;              # Turn off buffering and send commands.
    blt::busy release $itk_component(hull)
}

# ----------------------------------------------------------------------
# USAGE: CurrentDatasets ?-all -visible? ?dataobjs?
#
# Returns a list of server IDs for the current datasets being displayed.  This
# is normally a single ID, but it might be a list of IDs if the current data
# object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::CurrentDatasets {args} {
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
itcl::body Rappture::VtkIsosurfaceViewer::Zoom {option} {
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
                qw              0.853553
                qx              -0.353553
                qy              0.353553
                qz              0.146447
                zoom    1.0
                xpan   0
                ypan   0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
            $_arcball quaternion $q
            SendCmd "camera reset"
            DoRotate
        }
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::PanCamera {} {
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
itcl::body Rappture::VtkIsosurfaceViewer::Rotate {option x y} {
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

itcl::body Rappture::VtkIsosurfaceViewer::Pick {x y} {
    foreach tag [CurrentDatasets -visible] {
        SendCmdNoSplash "dataset getscalar pixel $x $y $tag"
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
itcl::body Rappture::VtkIsosurfaceViewer::Pan {option x y} {
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
# USAGE: InitSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::InitSettings { args } {
    foreach spec $args {
        if { [info exists _settings($_first-$spec)] } {
            # Reset global setting with dataobj specific setting
            set _settings($spec) $_settings($_first-$spec)
        }
        AdjustSetting $spec
    }
}

#
# AdjustSetting --
#
#       Changes/updates a specific setting in the widget.  There are
#       usually user-setable option.  Commands are sent to the render
#       server.
#
itcl::body Rappture::VtkIsosurfaceViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "isosurfaceOpacity" {
            set val $_settings(isosurfaceOpacity)
            set sval [expr { 0.01 * double($val) }]
	    SendCmd "contour3d opacity $sval"
        }
        "isosurfaceWireframe" {
            set bool $_settings(isosurfaceWireframe)
	    SendCmd "contour3d wireframe $bool"
        }
        "isosurfaceVisible" {
            set bool $_settings(isosurfaceVisible)
	    SendCmd "contour3d visible $bool"
            if { $bool } {
                Rappture::Tooltip::for $itk_component(contour) \
                    "Hide the isosurface"
            } else {
                Rappture::Tooltip::for $itk_component(contour) \
                    "Show the isosurface"
            }
        }
        "isosurfaceLighting" {
            set bool $_settings(isosurfaceLighting)
	    SendCmd "contour3d lighting $bool"
        }
        "isosurfaceEdges" {
            set bool $_settings(isosurfaceEdges)
	    SendCmd "contour3d edges $bool"
        }
        "isosurfaceOutline" {
            set bool $_settings(isosurfaceOutline)
	    SendCmd "dataset outline $bool"
        }
        "background" {
            set bgcolor [$itk_component(background) value]
	    array set fgcolors {
		"black" "white"
		"white" "black"
		"grey"	"black"
	    }
            configure -plotbackground $bgcolor \
		-plotforeground $fgcolors($bgcolor)
	    $itk_component(view) delete "legend"
	    DrawLegend
        }
        "axesVisible" {
            set bool $_settings(axesVisible)
            SendCmd "axis visible all $bool"
        }
        "axisLabelsVisible" {
            set bool $_settings(axisLabelsVisible)
            SendCmd "axis labels all $bool"
        }
        "axisXGrid" - "axisYGrid" - "axisZGrid" {
            set axis [string tolower [string range $what 4 4]]
            set bool $_settings($what)
            SendCmd "axis grid $axis $bool"
        }
        "axisMode" {
            set mode [$itk_component(axisMode) value]
            set mode [$itk_component(axisMode) translate $mode]
            set _settings($what) $mode
            SendCmd "axis flymode $mode"
        }
        "cutplaneEdges" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane edges $bool $dataset"
            }
        }
        "cutplaneVisible" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane visible $bool $dataset"
            }
        }
        "cutplaneWireframe" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane wireframe $bool $dataset"
            }
        }
        "cutplaneLighting" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane lighting $bool $dataset"
            }
        }
        "cutplaneOpacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane opacity $sval $dataset"
            }
        }
        "cutplaneXVisible" - "cutplaneYVisible" - "cutplaneZVisible" {
            set axis [string tolower [string range $what 8 8]]
            set bool $_settings($what)
            if { $bool } {
                $itk_component(${axis}position) configure -state normal \
                    -troughcolor white
            } else {
                $itk_component(${axis}position) configure -state disabled \
                    -troughcolor grey82
            }
	    SendCmd "cutplane axis $axis $bool"
        }
        "cutplaneXPosition" - "cutplaneYPosition" - "cutplaneZPosition" {
            set axis [string tolower [string range $what 8 8]]
            set pos [expr $_settings($what) * 0.01]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane slice ${axis} ${pos} $dataset"
            }
            set _cutplanePending 0
        }
        "isosurfaceVisible" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "contour3d visible $bool $dataset"
            }
	    set _settings(isosurfaceVisible) $bool
            if { $bool } {
                Rappture::Tooltip::for $itk_component(contour) \
                    "Hide the isosurface"
            } else {
                Rappture::Tooltip::for $itk_component(contour) \
                    "Show the isosurface"
            }
	    DrawLegend
        }
        "colormapPalette" {
            set color [$itk_component(palette) value]
            set _settings(colormapPalette) $color
	    if { $color == "none" } {
		if { $_settings(colormapVisible) } {
		    SendCmd "contour3d surface 0"
		    set _settings(colormapVisible) 0
		}
	    } else {
		if { !$_settings(colormapVisible) } {
		    SendCmd "contour3d surface 1"
		    set _settings(colormapVisible) 1
		}
		ResetColormap $color
		SendCmd "contour3d colormap $_currentColormap"
		SendCmd "cutplane colormap $_currentColormap"
	    }
	    EventuallyRequestLegend
        }
        "isolineColor" {
            set color [$itk_component(isolineColor) value]
	    set _settings(isolineColor) $color
	    DrawLegend
        }
        "isosurfaceOpacity" {
            set val $_settings(isosurfaceOpacity)
            set sval [expr { 0.01 * double($val) }]
            foreach dataset [CurrentDatasets -visible $_first] {
                SendCmd "contour3d opacity $sval $dataset"
            }
        }
        "isosurfaceLighting" {
            set bool $_settings(isosurfaceLighting)
            foreach dataset [CurrentDatasets -visible $_first] {
                SendCmd "contour3d lighting $bool $dataset"
            }
        }
        "field" {
            set label [$itk_component(field) value]
            set fname [$itk_component(field) translate $label]
            set _settings(field) $fname
            if { [info exists _fields($fname)] } {
                foreach { label units components } $_fields($fname) break
                if { $components > 1 } {
                    set _colorMode vmag
                } else {
                    set _colorMode scalar
                }
                set _curFldName $fname
                set _curFldLabel $label
            } else {
                puts stderr "unknown field \"$fname\""
                return
            }
            SendCmd "dataset maprange explicit $_limits($_curFldName) $_curFldName"
            SendCmd "contour3d colormode $_colorMode $_curFldName"
            SendCmd "cutplane colormode $_colorMode $_curFldName"
            SendCmd "camera reset"
            DrawLegend
        }
        "legendVisible" {
            if { !$_settings(legendVisible) } {
                $itk_component(view) delete legend
	    }
	    DrawLegend
        }
        default {
            error "don't know how to fix $what"
        }
    }
}


#
# RequestLegend --
#
#       Request a new legend from the server.  The size of the legend
#       is determined from the height of the canvas.  
#
# This should be called when
#	1.  A new current colormap is set.
#	2.  Window is resized.
#	3.  The limits of the data have changed.  (Just need a redraw).
#	4.  Number of isolines have changed. (Just need a redraw).
#	5.  Legend becomes visible (Just need a redraw).
#
itcl::body Rappture::VtkIsosurfaceViewer::RequestLegend {} {
    set _legendPending 0
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set c $itk_component(view)
    set w 12
    set h [expr {$_height - 2 * ($lineht + 2)}]
    if { $h < 1} {
        return
    }
    if { [info exists _fields($_curFldName)] } {
        set title [lindex $_fields($_curFldName) 0]
	if { $title != "component" } {
	    set h [expr $h - ($lineht + 2)]
	}
    } else {
        return
    }
    # Set the legend on the first heightmap dataset.
    if { $_currentColormap != ""  } {
	set cmap $_currentColormap
	SendCmdNoWait "legend $cmap scalar $title {} $w $h 0"
    }
}

#
# BuildColormap --
#
itcl::body Rappture::VtkIsosurfaceViewer::BuildColormap { name stylelist } {
    array set style $stylelist
    set cmap [ColorsToColormap $style(-color)]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    if { ![info exists _settings(contour-opacity)] } {
        set _settings(contour-opacity) $style(-opacity)
    }
    set max $_settings(contour-opacity)

    set wmap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $wmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkIsosurfaceViewer::plotbackground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotbackground)]
        SendCmd "screen bgcolor $rgb"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkIsosurfaceViewer::plotforeground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotforeground)]
	SendCmd "axis color all $rgb"
        SendCmd "dataset color $rgb"
        SendCmd "cutplane ccolor $rgb"
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::limits { dataobj } {
    foreach { limits(xmin) limits(xmax) } [$dataobj limits x] break
    foreach { limits(ymin) limits(ymax) } [$dataobj limits y] break
    foreach { limits(zmin) limits(zmax) } [$dataobj limits z] break
    foreach { limits(vmin) limits(vmax) } [$dataobj limits v] break
    return [array get limits]
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildIsosurfaceTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Isosurface Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.contour \
        -text "Isosurfaces" \
        -variable [itcl::scope _settings(isosurfaceVisible)] \
        -command [itcl::code $this AdjustSetting isosurfaceVisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings(isosurfaceWireframe)] \
        -command [itcl::code $this AdjustSetting isosurfaceWireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(isosurfaceLighting)] \
        -command [itcl::code $this AdjustSetting isosurfaceLighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings(isosurfaceEdges)] \
        -command [itcl::code $this AdjustSetting isosurfaceEdges] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings(isosurfaceOutline)] \
        -command [itcl::code $this AdjustSetting isosurfaceOutline] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings(legendVisible)] \
        -command [itcl::code $this AdjustSetting legendVisible] \
        -font "Arial 9"

    label $inner.linecolor_l -text "Isolines" -font "Arial 9" 
    itk_component add isolineColor {
        Rappture::Combobox $inner.linecolor -width 10 -editable no
    }
    $inner.linecolor choices insert end \
        "black"              "black"            \
        "blue"               "blue"             \
        "cyan"               "cyan"             \
        "green"              "green"            \
        "grey"               "grey"             \
        "magenta"            "magenta"          \
        "orange"             "orange"           \
        "red"                "red"              \
        "white"              "white"            \
	"none"		     "none"

    $itk_component(isolineColor) value "white"
    bind $inner.linecolor <<Value>> \
	[itcl::code $this AdjustSetting isolineColor]

    label $inner.background_l -text "Background" -font "Arial 9" 
    itk_component add background {
        Rappture::Combobox $inner.background -width 10 -editable no
    }
    $inner.background choices insert end \
        "black"              "black"            \
        "white"              "white"            \
        "grey"               "grey"             

    $itk_component(background) value $_settings(background)
    bind $inner.background <<Value>> [itcl::code $this AdjustSetting background]

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(isosurfaceOpacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting isosurfaceOpacity]

    label $inner.scale_l -text "Scale" -font "Arial 9"
    ::scale $inner.scale -from 1 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(contour-scale)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting contour-scale]

    label $inner.field_l -text "Field" -font "Arial 9" 
    itk_component add field {
        Rappture::Combobox $inner.field -width 10 -editable no
    }
    bind $inner.field <<Value>> \
        [itcl::code $this AdjustSetting field]

    label $inner.palette_l -text "Palette" -font "Arial 9" 
    itk_component add palette {
        Rappture::Combobox $inner.palette -width 10 -editable no
    }
    $inner.palette choices insert end \
        "BCGYR"              "BCGYR"            \
        "BGYOR"              "BGYOR"            \
        "blue"               "blue"             \
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
        "brown-to-blue"      "brown-to-blue"    \
        "grey-to-blue"       "grey-to-blue"     \
        "orange-to-blue"     "orange-to-blue"   

    $itk_component(palette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting colormapPalette]

    blt::table $inner \
        0,0 $inner.palette_l -anchor w -pady 2  \
        0,1 $inner.palette   -anchor w -pady 2  -fill x \
        1,0 $inner.linecolor_l  -anchor w -pady 2  \
        1,1 $inner.linecolor    -anchor w -pady 2 -fill x  \
	2,0 $inner.background_l -anchor w -pady 2 \
	2,1 $inner.background -anchor w -pady 2  -fill x \
        3,0 $inner.contour   -anchor w -pady 2 -cspan 2 \
        4,0 $inner.wireframe -anchor w -pady 2 -cspan 2 \
        5,0 $inner.lighting  -anchor w -pady 2 -cspan 2 \
        6,0 $inner.edges     -anchor w -pady 2 -cspan 2 \
        7,0 $inner.outline   -anchor w -pady 2 -cspan 2 \
        8,0 $inner.legend    -anchor w -pady 2 \
        9,0 $inner.opacity_l -anchor w -pady 2 \
        9,1 $inner.opacity   -fill x   -pady 2 -fill x \
        10,0 $inner.field_l   -anchor w -pady 2  \
        10,1 $inner.field     -anchor w -pady 2  -fill x \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r11 c1 -resize expand
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon axis1]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Show Axes" \
        -variable [itcl::scope _settings(axesVisible)] \
        -command [itcl::code $this AdjustSetting axesVisible] \
        -font "Arial 9"

    checkbutton $inner.labels \
        -text "Show Axis Labels" \
        -variable [itcl::scope _settings(axisLabelsVisible)] \
        -command [itcl::code $this AdjustSetting axisLabelsVisible] \
        -font "Arial 9"

    checkbutton $inner.gridx \
        -text "Show X Grid" \
        -variable [itcl::scope _settings(axisXGrid)] \
        -command [itcl::code $this AdjustSetting axisXGrid] \
        -font "Arial 9"
    checkbutton $inner.gridy \
        -text "Show Y Grid" \
        -variable [itcl::scope _settings(axisYGrid)] \
        -command [itcl::code $this AdjustSetting axisYGrid] \
        -font "Arial 9"
    checkbutton $inner.gridz \
        -text "Show Z Grid" \
        -variable [itcl::scope _settings(axisZGrid)] \
        -command [itcl::code $this AdjustSetting axisZGrid] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9" 

    itk_component add axisMode {
        Rappture::Combobox $inner.mode -width 10 -editable no
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "furthest" \
        "outer_edges"     "outer"         
    $itk_component(axisMode) value "static"
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting axisMode]

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


itcl::body Rappture::VtkIsosurfaceViewer::BuildCameraTab {} {
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
            $row,0 $inner.ortho -cspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildCutplaneTab {} {

    set fg [option get $itk_component(hull) font Font]
    
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]] 

    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Cutplanes" \
        -variable [itcl::scope _settings(cutplaneVisible)] \
        -command [itcl::code $this AdjustSetting cutplaneVisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings(cutplaneWireframe)] \
        -command [itcl::code $this AdjustSetting cutplaneWireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(cutplaneLighting)] \
        -command [itcl::code $this AdjustSetting cutplaneLighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings(cutplaneEdges)] \
        -command [itcl::code $this AdjustSetting cutplaneEdges] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(cutplaneOpacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting cutplaneOpacity]
    $inner.opacity set $_settings(cutplaneOpacity)

    # X-value slicer...
    itk_component add xbutton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane-red] \
            -offimage [Rappture::icon x-cutplane-red] \
            -command [itcl::code $this AdjustSetting cutplaneXVisible] \
            -variable [itcl::scope _settings(cutplaneXVisible)] \
    }
    Rappture::Tooltip::for $itk_component(xbutton) \
        "Toggle the X-axis cutplane on/off"
    $itk_component(xbutton) select
    itk_component add xposition {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane x] \
            -variable [itcl::scope _settings(cutplaneXPosition)] \
	    -foreground red2 -font "Arial 9 bold"
    } {
        usual
        ignore -borderwidth -highlightthickness -foreground -font -background
    }
    # Set the default cutplane value before disabling the scale.
    $itk_component(xposition) set 50
    $itk_component(xposition) configure -state disabled
    Rappture::Tooltip::for $itk_component(xposition) \
        "@[itcl::code $this Slice tooltip x]"

    # Y-value slicer...
    itk_component add ybutton {
        Rappture::PushButton $inner.ybutton \
            -onimage [Rappture::icon y-cutplane-green] \
            -offimage [Rappture::icon y-cutplane-green] \
            -command [itcl::code $this AdjustSetting cutplaneYVisible] \
            -variable [itcl::scope _settings(cutplaneYVisible)] \
    }
    Rappture::Tooltip::for $itk_component(ybutton) \
        "Toggle the Y-axis cutplane on/off"
    $itk_component(ybutton) select

    itk_component add yposition {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane y] \
            -variable [itcl::scope _settings(cutplaneYPosition)] \
	    -foreground green3 -font "Arial 9 bold"
    } {
        usual
        ignore -borderwidth -highlightthickness -foreground -font
    }
    Rappture::Tooltip::for $itk_component(yposition) \
        "@[itcl::code $this Slice tooltip y]"
    # Set the default cutplane value before disabling the scale.
    $itk_component(yposition) set 50
    $itk_component(yposition) configure -state disabled

    # Z-value slicer...
    itk_component add zbutton {
        Rappture::PushButton $inner.zbutton \
            -onimage [Rappture::icon z-cutplane-blue] \
            -offimage [Rappture::icon z-cutplane-blue] \
            -command [itcl::code $this AdjustSetting cutplaneZVisible] \
            -variable [itcl::scope _settings(cutplaneZVisible)] \
    } {
	usual
	ignore -foreground
    }
    Rappture::Tooltip::for $itk_component(zbutton) \
        "Toggle the Z-axis cutplane on/off"
    $itk_component(zbutton) select

    itk_component add zposition {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane z] \
            -variable [itcl::scope _settings(cutplaneZPosition)] \
	    -foreground blue3 -font "Arial 9 bold"
    } {
        usual
        ignore -borderwidth -highlightthickness -foreground -font
    }
    $itk_component(zposition) set 50
    $itk_component(zposition) configure -state disabled
    Rappture::Tooltip::for $itk_component(zposition) \
        "@[itcl::code $this Slice tooltip z]"

    blt::table $inner \
        0,0 $inner.visible              -anchor w -pady 2 -cspan 3 \
        1,0 $inner.lighting             -anchor w -pady 2 -cspan 3 \
        2,0 $inner.wireframe            -anchor w -pady 2 -cspan 3 \
        3,0 $inner.edges                -anchor w -pady 2 -cspan 3 \
        4,0 $inner.opacity_l            -anchor w -pady 2 -cspan 1 \
        4,1 $inner.opacity              -fill x   -pady 2 -cspan 3 \
        6,0 $inner.xbutton		-anchor w -padx 2 -pady 2 \
        7,0 $inner.ybutton		-anchor w -padx 2 -pady 2 \
        8,0 $inner.zbutton		-anchor w -padx 2 -pady 2 \
        6,1 $inner.xval			-fill y -rspan 4 \
        6,2 $inner.yval			-fill y -rspan 4 \
        6,3 $inner.zval			-fill y -rspan 4 \


    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r9 c4 -resize expand
    blt::table configure $inner r5 -height 0.12i
}



#
#  camera -- 
#
itcl::body Rappture::VtkIsosurfaceViewer::camera {option args} {
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

itcl::body Rappture::VtkIsosurfaceViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach cname [$dataobj components] {
            set tag $dataobj-$cname
            set contents [$dataobj vtkdata $cname]
            append bytes "$contents\n\n"
        }
    }
    return [list .vtk $bytes]
}

itcl::body Rappture::VtkIsosurfaceViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildDownloadPopup { popup command } {
    Rappture::Balloon $popup \
        -title "[Rappture::filexfer::label downloadWord] as..."
    set inner [$popup component inner]
    label $inner.summary -text "" -anchor w 
    radiobutton $inner.vtk_button -text "VTK data file" \
        -variable [itcl::scope _downloadPopup(format)] \
        -font "Arial 9 " \
        -value vtk  
    Rappture::Tooltip::for $inner.vtk_button "Save as VTK data file."
    radiobutton $inner.image_button -text "Image File" \
        -variable [itcl::scope _downloadPopup(format)] \
        -font "Arial 9 " \
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

itcl::body Rappture::VtkIsosurfaceViewer::SetObjectStyle { dataobj comp } {
    # Parse style string.
    set tag $dataobj-$comp
    array set style {
        -color BCGYR
        -edges 0
        -edgecolor black
        -linewidth 1.0
        -opacity 0.6
        -wireframe 0
        -lighting 1
        -outline 0
	-levels 10
    }
    array set style [$dataobj style $comp]
    if { $dataobj != $_first } {
        set style(-opacity) 1
    }
    if { $_currentColormap == "" } {
	set stylelist [$dataobj style $comp]
	if { $stylelist != "" } {
	    array set style $stylelist
	    set stylelist [array get style]
	    SetCurrentColormap $stylelist
	}
	$itk_component(palette) value $style(-color)
    }
    SendCmd "cutplane add $tag"
    SendCmd "cutplane edges 0 $tag"
    SendCmd "cutplane wireframe 0 $tag"
    SendCmd "cutplane lighting 1 $tag"
    SendCmd "cutplane linewidth 1 $tag"

    #SendCmd "cutplane linecolor 1 1 1 $tag"
    #SendCmd "cutplane visible $tag"
    foreach axis { x y z } {
        SendCmd "cutplane slice $axis 0.5 $tag"
        SendCmd "cutplane axis $axis 0 $tag"
    }
    set _numContours $style(-levels)
    SendCmd "contour3d add numcontours $_numContours $tag"
    SendCmd "contour3d edges $style(-edges) $tag"
    SendCmd "dataset outline $style(-outline) $tag"
    set _settings(isosurfaceOutline) $style(-outline)
    set _settings(isosurfaceEdges) $style(-edges)
    #SendCmd "contour3d color [Color2RGB $settings(-color)] $tag"
    SendCmd "contour3d lighting $style(-lighting) $tag"
    set _settings(isosurfaceLighting) $style(-lighting)
    SendCmd "contour3d linecolor [Color2RGB $style(-edgecolor)] $tag"
    SendCmd "contour3d linewidth $style(-linewidth) $tag"
    SendCmd "contour3d opacity $style(-opacity) $tag"
    set _settings(isosurfaceOpacity) $style(-opacity)
    SendCmd "contour3d wireframe $style(-wireframe) $tag"
    set _settings(isosurfaceWireframe) $style(-wireframe)
    set _settings(isosurfaceOpacity) [expr $style(-opacity) * 100.0]
}

itcl::body Rappture::VtkIsosurfaceViewer::IsValidObject { dataobj } {
    if {[catch {$dataobj isa Rappture::Field} valid] != 0 || !$valid} {
        return 0
    }
    return 1
}

#
# EnterLegend --
#
itcl::body Rappture::VtkIsosurfaceViewer::EnterLegend { x y } {
    SetLegendTip $x $y
}

#
# MotionLegend --
#
itcl::body Rappture::VtkIsosurfaceViewer::MotionLegend { x y } {
    Rappture::Tooltip::tooltip cancel
    set c $itk_component(view)
    set cw [winfo width $c]
    set ch [winfo height $c]
    if { $x >= 0 && $x < $cw && $y >= 0 && $y < $ch } {
        SetLegendTip $x $y
    }
}

#
# LeaveLegend --
#
itcl::body Rappture::VtkIsosurfaceViewer::LeaveLegend { } {
    Rappture::Tooltip::tooltip cancel
    .rappturetooltip configure -icon ""
}

#
# SetLegendTip --
#
itcl::body Rappture::VtkIsosurfaceViewer::SetLegendTip { x y } {
    set fname $_curFldName
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]

    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    
    set ih [image height $_image(legend)]
    set iy [expr $y - ($lineht + 2)]

    if { $fname == "component" } {
	set title ""
    } else {
	if { [info exists _fields($fname)] } {
	    foreach { title units } $_fields($fname) break
	    if { $units != "" } {
		set title [format "%s (%s)" $title $units]
	    }
	} else {
	    set title $fname
	}
    }
    # If there's a legend title, increase the offset by the line height.
    if { $title != "" } {
        incr iy -$lineht
    }
    # Make a swatch of the selected color
    if { [catch { $_image(legend) get 10 $iy } pixel] != 0 } {
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
    if { [info exists _limits($_curFldName)] } {
        foreach { vmin vmax } $_limits($_curFldName) break
        set t [expr 1.0 - (double($iy) / double($ih-1))]
        set value [expr $t * ($vmax - $vmin) + $vmin]
    } else {
        set value 0.0
    }
    set tx [expr $x + 15] 
    set ty [expr $y - 5]
    if { [info exists _isolines($y)] } {
        Rappture::Tooltip::text $c [format "$title %g (isosurface)" $_isolines($y)]
    } else {
        Rappture::Tooltip::text $c [format "$title %g" $value]
    }
    Rappture::Tooltip::tooltip show $c +$tx,+$ty    
}


# ----------------------------------------------------------------------
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::Slice {option args} {
    switch -- $option {
        "move" {
            set axis [lindex $args 0]
	    set a [string toupper $axis]
            set oldval $_settings(cutplane${a}Position)
            set newval [lindex $args 1]
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set newpos [expr {0.01*$newval}]
            SendCmd "cutplane slice $axis $newpos"
        }
        "tooltip" {
            set axis [lindex $args 0]
            set val [$itk_component(${axis}position) get]
            return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
        }
        default {
            error "bad option \"$option\": should be axis, move, or tooltip"
        }
    }
}

#
# ReceiveLegend -- 
#
#	Invoked automatically whenever the "legend" command comes in from
#	the rendering server.  Indicates that binary image data with the
#	specified <size> will follow.
#
itcl::body Rappture::VtkIsosurfaceViewer::ReceiveLegend { colormap title min max size } {
    #puts stderr "ReceiveLegend colormap=$colormap title=$title range=$min,$max size=$size"
    set _title $title
    regsub {\(mag\)} $title "" _title 
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend)] } {
            set _image(legend) [image create photo]
        }
        $_image(legend) configure -data $bytes
        #puts stderr "read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
        if { [catch {DrawLegend} errs] != 0 } {
	    global errorInfo
	    puts stderr "errs=$errs errorInfo=$errorInfo"
        }
    }
}

#
# DrawLegend --
#
#       Draws the legend in the own canvas on the right side of the plot area.
#
itcl::body Rappture::VtkIsosurfaceViewer::DrawLegend {} {
    set fname $_curFldName
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    
    if { $fname == "component" } {
	set title ""
    } else {
	if { [info exists _fields($fname)] } {
	    foreach { title units } $_fields($fname) break
	    if { $units != "" } {
		set title [format "%s (%s)" $title $units]
	    }
	} else {
	    set title $fname
	}
    }
    set x [expr $w - 2]
    if { !$_settings(legendVisible) } {
	$c delete legend
	return
    } 
    if { [$c find withtag "legend"] == "" } {
	set y 2 
	# If there's a legend title, create a text item for the title.
	if { $title != "" } {
	    $c create text $x $y \
		-anchor ne \
		-fill $itk_option(-plotforeground) -tags "title legend" \
		-font $font 
	    incr y $lineht
	}
	$c create text $x $y \
	    -anchor ne \
	    -fill $itk_option(-plotforeground) -tags "zmax legend" \
	    -font $font
	incr y $lineht
	$c create image $x $y \
	    -anchor ne \
	    -image $_image(legend) -tags "colormap legend"
	$c create rectangle $x $y 1 1 \
	    -fill "" -outline "" -tags "sensor legend"
	$c create text $x [expr {$h-2}] \
	    -anchor se \
	    -fill $itk_option(-plotforeground) -tags "zmin legend" \
	    -font $font
	$c bind sensor <Enter> [itcl::code $this EnterLegend %x %y]
	$c bind sensor <Leave> [itcl::code $this LeaveLegend]
	$c bind sensor <Motion> [itcl::code $this MotionLegend %x %y]
    }
    $c delete isoline
    set x2 $x
    set iw [image width $_image(legend)]
    set ih [image height $_image(legend)]
    set x1 [expr $x2 - ($iw*12)/10]
    set color $_settings(isolineColor)
    # Draw the isolines on the legend.
    array unset _isolines
    if { $color != "none"  && [info exists _limits($_curFldName)] &&
         $_numContours > 0 } {
	set pixels [blt::vector create \#auto]
	set values [blt::vector create \#auto]
	set range [image height $_image(legend)]
	# Order of pixels is max to min (max is at top of legend).
	$pixels seq $ih 0 $_numContours
	set offset [expr 2 + $lineht]
	# If there's a legend title, increase the offset by the line height.
	if { $title != "" } {
	    incr offset $lineht
	}
	# Order of values is min to max.
        $pixels expr {round($pixels + $offset)}
        foreach { vmin vmax } $_limits($_curFldName) break
        $values seq $vmin $vmax $_numContours
        set tags "isoline legend"
        foreach pos [$pixels range 0 end] value [$values range end 0] {
	    set y1 [expr int($pos)]
            for { set off 0 } { $off < 3 } { incr off } {
                set _isolines([expr $y1 + $off]) $value
                set _isolines([expr $y1 - $off]) $value
            }
            set id [$c create line $x1 $y1 $x2 $y1 -fill $color -tags $tags]
        }
        blt::vector destroy $pixels $values
    }

    $c bind title <ButtonPress> [itcl::code $this Combo post]
    $c bind title <Enter> [itcl::code $this Combo activate]
    $c bind title <Leave> [itcl::code $this Combo deactivate]
    # Reset the item coordinates according the current size of the plot.
    $c itemconfigure title -text $title
    if { [info exists _limits($_curFldName)] } {
        foreach { vmin vmax } $_limits($_curFldName) break
	$c itemconfigure zmin -text [format %g $vmin]
	$c itemconfigure zmax -text [format %g $vmax]
    }
    set y 2
    # If there's a legend title, move the title to the correct position
    if { $title != "" } {
	$c coords title $x $y
	incr y $lineht
    }
    $c coords zmax $x $y
    incr y $lineht
    $c coords colormap $x $y
    set ix [image width $_image(legend)]
    set ih [image height $_image(legend)]
    $c coords sensor [expr $x - $iw] $y $x [expr $y + $ih]
    $c raise sensor
    $c coords zmin $x [expr {$h - 2}]
}

# ----------------------------------------------------------------------
# USAGE: _dropdown post
# USAGE: _dropdown unpost
# USAGE: _dropdown select
#
# Used internally to handle the dropdown list for this combobox.  The
# post/unpost options are invoked when the list is posted or unposted
# to manage the relief of the controlling button.  The select option
# is invoked whenever there is a selection from the list, to assign
# the value back to the gauge.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::Combo {option} {
    set c $itk_component(view) 
    switch -- $option {
        post {
            foreach { x1 y1 x2 y2 } [$c bbox title] break
            set x1 [expr [winfo width $itk_component(view)] - [winfo reqwidth $itk_component(fieldmenu)]]
            set x [expr $x1 + [winfo rootx $itk_component(view)]]
            set y [expr $y2 + [winfo rooty $itk_component(view)]]
            tk_popup $itk_component(fieldmenu) $x $y
        }
        activate {
            $c itemconfigure title -fill red
        }
        deactivate {
            $c itemconfigure title -fill $itk_option(-plotforeground) 
        }
        invoke {
            $itk_component(field) value $_curFldLabel
            AdjustSetting field
        }
        default {
            error "bad option \"$option\": should be post, unpost, select"
        }
    }
}

#
# SetCurrentColormap --
#
itcl::body Rappture::VtkIsosurfaceViewer::SetCurrentColormap { stylelist } {
    array set style {
        -color BCGYR
        -levels 10
        -opacity 1.0
    }
    array set style $stylelist

    set name "$style(-color):$style(-levels):$style(-opacity)"
    if { ![info exists _colormaps($name)] } {
	set stylelist [array get style]
        BuildColormap $name $stylelist
        set _colormaps($name) $stylelist
    }
    set _currentColormap $name
}

#
# BuildColormap --
#
itcl::body Rappture::VtkIsosurfaceViewer::BuildColormap { name stylelist } {
    array set style $stylelist
    set cmap [ColorsToColormap $style(-color)]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    if { ![info exists _settings(isosurfaceOpacity)] } {
        set _settings(isosurfaceOpacity) $style(-opacity)
    }
    set max $_settings(isosurfaceOpacity)

    set wmap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $wmap }"
}

itcl::body Rappture::VtkIsosurfaceViewer::ResetColormap { color } {
    array set style {
	-color BCGYR
	-levels 10
	-opacity 1.0
    }
    if { [info exists _colormap($_currentColormap)] } {
	array set style $_colormap($_currentColormap)
    }
    set style(-color) $color
    SetCurrentColormap [array get style]
}
