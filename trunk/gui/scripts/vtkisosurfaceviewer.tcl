# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: vtkisosurfaceviewer - Vtk 3D contour object viewer
#
#  It connects to the Vtkvis server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2016  HUBzero Foundation, LLC
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

    constructor { args } {
        Rappture::VisViewer::constructor
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
    public method parameters {title args} {
        # do nothing
    }
    public method scale {args}

    # The following methods are only used by this class.
    private method AdjustSetting {what {value ""}}
    private method BuildAxisTab {}
    private method BuildCameraTab {}
    private method BuildColormap { name }
    private method BuildCutplanesTab {}
    private method BuildDownloadPopup { widget command }
    private method BuildIsosurfaceTab {}
    private method Connect {}
    private method CurrentDatasets {args}
    private method DisableMouseRotationBindings {}
    private method Disconnect {}
    private method DoChangeContourLevels {}
    private method DoResize {}
    private method DoRotate {}
    private method DrawLegend {}
    private method EnterLegend { x y }
    private method EventuallyChangeContourLevels {}
    private method EventuallyRequestLegend {}
    private method EventuallyResize { w h }
    private method EventuallyRotate { q }
    private method EventuallySetCutplane { axis args }
    private method GenerateContourList {}
    private method GetImage { args }
    private method GetVtkData { args }
    private method InitSettings { args }
    private method IsValidObject { dataobj }
    private method LeaveLegend {}
    private method LegendB1Motion {status x y}
    private method LegendPointToValue { x y }
    private method LegendProbeSingleContour { x y }
    private method LegendRangeAction { option args }
    private method LegendRangeValidate { widget which value }
    private method LegendTitleAction { option }
    private method MotionLegend { x y }
    private method MouseOver2Which {}
    private method Pan {option x y}
    private method PanCamera {}
    private method Pick {x y}
    private method QuaternionToView { q } {
        foreach { _view(-qw) _view(-qx) _view(-qy) _view(-qz) } $q break
    }
    private method Rebuild {}
    private method ReceiveDataset { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { colormap title vmin vmax size }
    private method RequestLegend {}
    private method Rotate {option x y}
    private method SetCurrentColormap { color }
    private method SetCurrentFieldName { dataobj }
    private method SetLegendTip { x y }
    private method SetMinMaxGauges { min max }
    private method SetObjectStyle { dataobj comp }
    private method SetOrientation { side }
    private method SetupKeyboardBindings {}
    private method SetupMousePanningBindings {}
    private method SetupMouseRotationBindings {}
    private method SetupMouseZoomBindings {}
    private method Slice {option args}
    private method ToggleCustomRange { args }
    private method ViewToQuaternion {} {
        return [list $_view(-qw) $_view(-qx) $_view(-qy) $_view(-qz)]
    }
    private method Zoom {option}

    private variable _arcball ""

    private variable _dlist "";         # list of data objects
    private variable _obj2ovride;       # maps dataobj => style override
    private variable _datasets;         # contains all the dataobj-component
                                        # datasets in the server
    private variable _colormaps;        # contains all the colormaps
                                        # in the server.
    # The name of the current colormap used.  The colormap is global to all
    # heightmaps displayed.
    private variable _currentColormap ""
    private variable _currentNumContours -1

    private variable _click;            # info used for rotate operations
    private variable _limits;           # autoscale min/max for all axes
    private variable _view;             # view params for 3D view
    private variable _settings
    private variable _changed
    private variable _reset 1;          # Connection to server has been reset.

    private variable _first "";         # This is the topmost dataset.
    private variable _start 0
    private variable _title ""
    private variable _isolines
    private variable _contourList
    private variable _currentLimits ""
    private variable _widget
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _legendPending 0
    private variable _rotatePending 0
    private variable _cutplanePending 0
    private variable _field      ""
    private variable _colorMode "scalar"; # Mode of colormap (vmag or scalar)
    private variable _fields
    private variable _curFldName ""
    private variable _curFldLabel ""
    private variable _mouseOver "";     # what called LegendRangeAction:
                                        # vmin or vmax
    private variable _customRangeClick 1; # what called ToggleCustomRange

    private common _downloadPopup;      # download options from popup
    private common _hardcopy
}

itk::usual VtkIsosurfaceViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::constructor {args} {
    set _serverType "vtkvis"

    #DebugOn
    EnableWaitDialog 900

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this RequestLegend]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Contour levels event
    $_dispatcher register !contours
    $_dispatcher dispatch $this !contours \
        "[itcl::code $this DoChangeContourLevels]; list"

    # X-Cutplane event
    $_dispatcher register !xcutplane
    $_dispatcher dispatch $this !xcutplane \
        "[itcl::code $this AdjustSetting -xcutplaneposition]; list"

    # Y-Cutplane event
    $_dispatcher register !ycutplane
    $_dispatcher dispatch $this !ycutplane \
        "[itcl::code $this AdjustSetting -ycutplaneposition]; list"

    # Z-Cutplane event
    $_dispatcher register !zcutplane
    $_dispatcher dispatch $this !zcutplane \
        "[itcl::code $this AdjustSetting -zcutplaneposition]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias dataset [itcl::code $this ReceiveDataset]

    # Initialize the view to some default parameters.
    array set _view {
        -ortho    0
        -qw       0.853553
        -qx       -0.353553
        -qy       0.353553
        -qz       0.146447
        -xpan     0
        -ypan     0
        -zoom     1.0
    }
    set _arcball [blt::arcball create 100 100]
    $_arcball quaternion [ViewToQuaternion]

    array set _contourList {
        numLevels       10
        reqValues       ""
        updatePending   0
        values          ""
    }
    array set _settings {
        -axesvisible            1
        -axislabels             1
        -axisminorticks         1
        -axismode               "static"
        -background             black
        -colormap               BCGYR
        -colormapvisible        1
        -customrange            0
        -customrangemin         0
        -customrangemax         1
        -cutplaneedges          0
        -cutplanelighting       1
        -cutplaneopacity        1.0
        -cutplanepreinterp      1
        -cutplanesvisible       0
        -cutplanewireframe      0
        -field                  "Default"
        -isolinecolor           white
        -isosurfaceedges        0
        -isosurfacelighting     1
        -isosurfaceopacity      0.6
        -isosurfacevisible      1
        -isosurfacewireframe    0
        -legendvisible          1
        -numcontours            10
        -outline                0
        -xcutplaneposition      50
        -xcutplanevisible       1
        -xgrid                  0
        -ycutplaneposition      50
        -ycutplanevisible       1
        -ygrid                  0
        -zcutplaneposition      50
        -zcutplanevisible       1
        -zgrid                  0
    }
    array set _changed {
        -colormap               0
        -cutplaneedges          0
        -cutplanelighting       0
        -cutplaneopacity        0
        -cutplanepreinterp      0
        -cutplanesvisible       0
        -cutplanewireframe      0
        -isosurfaceedges        0
        -isosurfacelighting     0
        -isosurfaceopacity      0
        -isosurfacevisible      0
        -isosurfacewireframe    0
        -numcontours            0
        -outline                0
        -xcutplaneposition      0
        -xcutplanevisible       0
        -ycutplaneposition      0
        -ycutplanevisible       0
        -zcutplaneposition      0
        -zcutplanevisible       0
    }
    array set _widget {
        -isosurfaceopacity      60
        -cutplaneopacity        100
    }

    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth -background
    }

    itk_component add fieldmenu {
        menu $itk_component(plotarea).menu -bg black -fg white -relief flat \
            -tearoff 0
    } {
        usual
        ignore -background -foreground -relief -tearoff
    }

    # add an editor for adjusting the legend min and max values
    itk_component add editor {
        Rappture::Editor $itk_interior.editor \
            -activatecommand [itcl::code $this LegendRangeAction activate] \
            -validatecommand [itcl::code $this LegendRangeAction validate] \
            -applycommand [itcl::code $this LegendRangeAction apply]
    }

    set c $itk_component(view)
    bind $c <Configure> [itcl::code $this EventuallyResize %w %h]
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
            -variable [itcl::scope _settings(-isosurfacevisible)] \
            -command [itcl::code $this AdjustSetting -isosurfacevisible]
    }
    $itk_component(contour) select
    Rappture::Tooltip::for $itk_component(contour) \
        "Hide the isosurface"
    pack $itk_component(contour) -padx 2 -pady 2

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings(-cutplanesvisible)] \
            -command [itcl::code $this AdjustSetting -cutplanesvisible]
    }
    Rappture::Tooltip::for $itk_component(cutplane) \
        "Show the cutplanes"
    pack $itk_component(cutplane) -padx 2 -pady 2

    if { [catch {
        BuildIsosurfaceTab
        BuildCutplanesTab
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

    SetupMouseRotationBindings
    SetupMousePanningBindings
    SetupMouseZoomBindings
    SetupKeyboardBindings

    #bind $itk_component(view) <ButtonRelease-3> \
    #    [itcl::code $this Pick %x %y]

    set _image(download) [image create photo]

    eval itk_initialize $args

    Connect
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

itcl::body Rappture::VtkIsosurfaceViewer::SetupMouseRotationBindings {} {
    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> \
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]
}

itcl::body Rappture::VtkIsosurfaceViewer::DisableMouseRotationBindings {} {
    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> ""
    bind $itk_component(view) <B1-Motion> ""
    bind $itk_component(view) <ButtonRelease-1> ""
}

itcl::body Rappture::VtkIsosurfaceViewer::SetupMousePanningBindings {} {
    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]
}

itcl::body Rappture::VtkIsosurfaceViewer::SetupMouseZoomBindings {} {
    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        bind $itk_component(view) <4> [itcl::code $this Zoom out]
        bind $itk_component(view) <5> [itcl::code $this Zoom in]
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::SetupKeyboardBindings {} {
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

itcl::body Rappture::VtkIsosurfaceViewer::DoChangeContourLevels {} {
    GenerateContourList
    SendCmd [list contour3d contourlist $_contourList(values)]
    SendCmd [list camera reset]
    DrawLegend
    set _contourList(updatePending) 0
}

itcl::body Rappture::VtkIsosurfaceViewer::DoRotate {} {
    SendCmd "camera orient [ViewToQuaternion]"
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
    QuaternionToView $q
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

itcl::body Rappture::VtkIsosurfaceViewer::EventuallyChangeContourLevels {} {
    set n $_contourList(numLevels)
    set _contourList(values) ""
    if { !$_contourList(updatePending) } {
        set _contourList(updatePending) 1
        global rotate_delay
        $_dispatcher event -after $rotate_delay !contours
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
    if { ![$dataobj isvalid] } {
        return;                         # Object doesn't contain valid data.
    }
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
    array set params $settings

    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
    }
    set pos [lsearch -exact $_dlist $dataobj]
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
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.  No data objects are
# deleted.  They are only removed from the display list.
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
        array unset _obj2ovride $dataobj-*
        array unset _settings $dataobj-*
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
        if { ![$dataobj isvalid] } {
            continue;                   # Object doesn't contain valid data.
        }
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

                # set reasonable defaults for
                # customrangevmin and customrangevmax
                foreach {min max} $lim break
                SetMinMaxGauges $min $max
                set _settings(-customrangemin) $min
                set _settings(-customrangemax) $max

                continue
            }
            foreach {min max} $lim break
            foreach {fmin fmax} $_limits($fname) break
            if { ! $_settings(-customrange) } {
                SetMinMaxGauges $fmin $fmax
            }
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
    set _reset 1
    set result [VisViewer::Connect $_hosts]
    if { $result } {
        if { $_reportClientInfo }  {
            # Tell the server the viewer, hub, user and session.
            # Do this immediately on connect before buffering any commands
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
            lappend info "version" "$Rappture::version"
            lappend info "build" "$Rappture::build"
            lappend info "svnurl" "$Rappture::svnurl"
            lappend info "installdir" "$Rappture::installdir"
            lappend info "hub" [exec hostname]
            lappend info "client" "vtkisosurfaceviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }

        set w [winfo width $itk_component(view)]
        set h [winfo height $itk_component(view)]
        EventuallyResize $w $h
    }
    return $result
}

#
# isconnected --
#
# Indicates if we are currently connected to the visualization server.
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
# Clients use this method to disconnect from the current rendering server.
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
    array unset _datasets
    array unset _colormaps
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
            fconfigure $f -encoding binary
            puts -nonewline $f $bytes
            close $f
        }
        $_image(plot) configure -data $bytes
        #set time [clock seconds]
        #set date [clock format $time]
        #set w [image width $_image(plot)]
        #set h [image height $_image(plot)]
        #puts stderr "$date: received image ${w}x${h} image"
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
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $w < 2 || $h < 2 } {
        update
        $_dispatcher event -idle !rebuild
        return
    }

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).
    StartBufferingCommands

    if { $_reset } {
        set _width $w
        set _height $h
        $_arcball resize $w $h
        DoResize

        # Reset the camera and other view parameters
        $_arcball quaternion [ViewToQuaternion]
        InitSettings -ortho
        DoRotate
        PanCamera
        set _first ""
        InitSettings -background \
            -xgrid -ygrid -zgrid -axismode \
            -axesvisible -axislabels -axisminorticks
        #SendCmd "axis lformat all %g"
        StopBufferingCommands
        SendCmd "imgflush"
        StartBufferingCommands
    }
    set _first ""
    SendCmd "dataset visible 0"
    eval scale $_dlist
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
            SetCurrentFieldName $dataobj
        }
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj vtkdata $comp]
                if 0 {
                    set f [open "/tmp/isosurface.vtk" "w"]
                    fconfigure $f -translation binary -encoding binary
                    puts -nonewline $f $bytes
                    close $f
                }
                set length [string length $bytes]
                if { $_reportClientInfo }  {
                    set info {}
                    lappend info "tool_id"       [$dataobj hints toolid]
                    lappend info "tool_name"     [$dataobj hints toolname]
                    lappend info "tool_title"    [$dataobj hints tooltitle]
                    lappend info "tool_command"  [$dataobj hints toolcommand]
                    lappend info "tool_revision" [$dataobj hints toolrevision]
                    lappend info "dataset_label" [$dataobj hints label]
                    lappend info "dataset_size"  $length
                    lappend info "dataset_tag"   $tag
                    SendCmd "clientinfo [list $info]"
                }
                SendCmd "dataset add $tag data follows $length"
                SendData $bytes
                set _datasets($tag) 1
                SetObjectStyle $dataobj $comp
            }
            if { [info exists _obj2ovride($dataobj-raise)] } {
                SendCmd "contour3d visible 1 $tag"
            }
        }
    }

    InitSettings -cutplanesvisible -isosurfacevisible -outline
    if { $_reset } {
        # These are settings that rely on a dataset being loaded.
        InitSettings \
            -field -range \
            -isosurfaceedges -isosurfacelighting -isosurfaceopacity \
            -isosurfacewireframe \
            -xcutplaneposition -ycutplaneposition -zcutplaneposition \
            -xcutplanevisible -ycutplanevisible -zcutplanevisible \
            -cutplanepreinterp -numcontours

        Zoom reset
        foreach axis { x y z } {
            set label ""
            if { $_first != "" } {
                set label [$_first hints ${axis}label]
            }
            if { $label == "" } {
                set label [string toupper $axis]
            }
            # There may be a space in the axis label.
            SendCmd [list axis name $axis $label]
        }
        if { [array size _fields] < 2 } {
            catch {blt::table forget $itk_component(field) $itk_component(field_l)}
        }
        set _reset 0
    }
    # Redraw the legend even if we're using the same colormap. The position
    # of the isolines may have changed because the range of data changed.
    DrawLegend

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
            set _view(-zoom) [expr {$_view(-zoom)*1.25}]
            SendCmd "camera zoom $_view(-zoom)"
        }
        "out" {
            set _view(-zoom) [expr {$_view(-zoom)*0.8}]
            SendCmd "camera zoom $_view(-zoom)"
        }
        "reset" {
            array set _view {
                -qw      0.853553
                -qx      -0.353553
                -qy      0.353553
                -qz      0.146447
                -xpan    0
                -ypan    0
                -zoom    1.0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            $_arcball quaternion [ViewToQuaternion]
            DoRotate
            SendCmd "camera reset"
        }
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::PanCamera {} {
    set x $_view(-xpan)
    set y $_view(-ypan)
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
itcl::body Rappture::VtkIsosurfaceViewer::Pan {option x y} {
    switch -- $option {
        "set" {
            set w [winfo width $itk_component(view)]
            set h [winfo height $itk_component(view)]
            set x [expr $x / double($w)]
            set y [expr $y / double($h)]
            set _view(-xpan) [expr $_view(-xpan) + $x]
            set _view(-ypan) [expr $_view(-ypan) + $y]
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
            set _view(-xpan) [expr $_view(-xpan) - $dx]
            set _view(-ypan) [expr $_view(-ypan) - $dy]
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
        if { [info exists _settings($_first${spec})] } {
            # Reset global setting with dataobj specific setting
            set _settings($spec) $_settings($_first${spec})
        }
        AdjustSetting $spec
    }
}

#
# AdjustSetting --
#
# Changes/updates a specific setting in the widget.  There are
# usually user-setable option.  Commands are sent to the render
# server.
#
itcl::body Rappture::VtkIsosurfaceViewer::AdjustSetting {what {value ""}} {
    DebugTrace "Enter"
    if { ![isconnected] } {
        DebugTrace "Not connected"
        return
    }
    switch -- $what {
        "-axesvisible" {
            set bool $_settings($what)
            SendCmd "axis visible all $bool"
        }
        "-axislabels" {
            set bool $_settings($what)
            SendCmd "axis labels all $bool"
        }
        "-axisminorticks" {
            set bool $_settings($what)
            SendCmd "axis minticks all $bool"
        }
        "-axismode" {
            set mode [$itk_component(axisMode) value]
            set mode [$itk_component(axisMode) translate $mode]
            set _settings($what) $mode
            SendCmd "axis flymode $mode"
        }
        "-background" {
            set bgcolor [$itk_component(background) value]
            array set fgcolors {
                "black" "white"
                "white" "black"
                "grey"  "black"
            }
            configure -plotbackground $bgcolor \
                -plotforeground $fgcolors($bgcolor)
            $itk_component(view) delete "legend"
            DrawLegend
        }
        "-colormap" {
            set _changed($what) 1
            StartBufferingCommands
            set color [$itk_component(colormap) value]
            set _settings($what) $color
            if { $color == "none" } {
                if { $_settings(-colormapvisible) } {
                    SendCmd "contour3d colormode constant {}"
                    set _settings(-colormapvisible) 0
                }
            } else {
                if { !$_settings(-colormapvisible) } {
                    SendCmd "contour3d colormode $_colorMode $_curFldName"
                    set _settings(-colormapvisible) 1
                }
                SetCurrentColormap $color
            }
            StopBufferingCommands
            EventuallyRequestLegend
        }
        "-cutplaneedges" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "cutplane edges $bool"
        }
        "-cutplanelighting" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "cutplane lighting $bool"
        }
        "-cutplaneopacity" {
            set _changed($what) 1
            set _settings($what) [expr $_widget($what) * 0.01]
            SendCmd "cutplane opacity $_settings($what)"
        }
        "-cutplanepreinterp" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "cutplane preinterp $bool"
        }
        "-cutplanesvisible" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "cutplane visible 0"
            if { $bool } {
                foreach tag [CurrentDatasets -visible] {
                    SendCmd "cutplane visible $bool $tag"
                }
            }
            if { $bool } {
                Rappture::Tooltip::for $itk_component(cutplane) \
                    "Hide the cutplanes"
            } else {
                Rappture::Tooltip::for $itk_component(cutplane) \
                    "Show the cutplanes"
            }
        }
        "-cutplanewireframe" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "cutplane wireframe $bool"
        }
        "-field" {
            set label [$itk_component(field) value]
            set fname [$itk_component(field) translate $label]
            set _settings($what) $fname
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
            SendCmd "dataset scalar $_curFldName"
            if { ![info exists _limits($_curFldName)] } {
                SendCmd "dataset maprange all"
            } else {
                if { $_settings(-customrange) } {
                    set vmin [$itk_component(min) value]
                    set vmax [$itk_component(max) value]
                } else {
                    foreach { vmin vmax } $_limits($_curFldName) break
                    # set the min / max gauges with limits from the field
                    # the legend's min and max text will be updated
                    # when the legend is redrawn in DrawLegend
                    SetMinMaxGauges $vmin $vmax
                }
                SendCmd "dataset maprange explicit $vmin $vmax $_curFldName"
            }
            SendCmd "cutplane colormode $_colorMode $_curFldName"
            SendCmd "contour3d colormode $_colorMode $_curFldName"
            GenerateContourList
            SendCmd [list contour3d contourlist $_contourList(values)]
            SendCmd "camera reset"
            DrawLegend
        }
        "-isolinecolor" {
            set color [$itk_component(isolineColor) value]
            set _settings($what) $color
            DrawLegend
        }
        "-isosurfaceedges" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "contour3d edges $bool"
        }
        "-isosurfacelighting" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "contour3d lighting $bool"
        }
        "-isosurfaceopacity" {
            set _changed($what) 1
            set _settings($what) [expr $_widget($what) * 0.01]
            SendCmd "contour3d opacity $_settings($what)"
        }
        "-isosurfacevisible" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "contour3d visible 0"
            if { $bool } {
                foreach tag [CurrentDatasets -visible] {
                    SendCmd "contour3d visible $bool $tag"
                }
                Rappture::Tooltip::for $itk_component(contour) \
                    "Hide the isosurface"
            } else {
                Rappture::Tooltip::for $itk_component(contour) \
                    "Show the isosurface"
            }
        }
        "-isosurfacewireframe" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "contour3d wireframe $bool"
        }
        "-legendvisible" {
            if { !$_settings($what) } {
                $itk_component(view) delete legend
            }
            DrawLegend
        }
        "-numcontours" {
            set _changed($what) 1
            set _settings($what) [$itk_component(numcontours) value]
            if { $_contourList(numLevels) != $_settings($what) } {
                set _contourList(numLevels) $_settings($what)
                EventuallyChangeContourLevels
            }
        }
        "-ortho" {
            set bool $_view($what)
            if { $bool } {
                SendCmd "camera mode ortho"
            } else {
                SendCmd "camera mode persp"
            }
        }
        "-outline" {
            set _changed($what) 1
            set bool $_settings($what)
            SendCmd "outline visible 0"
            if { $bool } {
                foreach tag [CurrentDatasets -visible] {
                    SendCmd "outline visible $bool $tag"
                }
            }
        }
        "-range" {
            if { $_settings(-customrange) } {
                set vmin [$itk_component(min) value]
                set vmax [$itk_component(max) value]
            } else {
                foreach { vmin vmax } $_limits($_curFldName) break
            }
            GenerateContourList
            SendCmd [list contour3d contourlist $_contourList(values)]
            SendCmd "dataset maprange explicit $vmin $vmax $_curFldName"
            DrawLegend
        }
        "-xcutplanevisible" - "-ycutplanevisible" - "-zcutplanevisible" {
            set _changed($what) 1
            set axis [string tolower [string range $what 1 1]]
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
        "-xcutplaneposition" - "-ycutplaneposition" - "-zcutplaneposition" {
            set _changed($what) 1
            set axis [string tolower [string range $what 1 1]]
            set pos [expr $_settings($what) * 0.01]
            SendCmd "cutplane slice ${axis} ${pos}"
            set _cutplanePending 0
        }
        "-xgrid" - "-ygrid" - "-zgrid" {
            set axis [string tolower [string range $what 1 1]]
            set bool $_settings($what)
            SendCmd "axis grid $axis $bool"
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

#
# RequestLegend --
#
# Request a new legend from the server.  The size of the legend
# is determined from the height of the canvas.
#
# This should be called when
#   1.  A new current colormap is set.
#   2.  Window is resized.
#   3.  The limits of the data have changed.  (Just need a redraw).
#   4.  Number of isolines have changed. (Just need a redraw).
#   5.  Legend becomes visible (Just need a redraw).
#
itcl::body Rappture::VtkIsosurfaceViewer::RequestLegend {} {
    set _legendPending 0
    if { ![info exists _fields($_curFldName)] } {
        return
    }
    set fname $_curFldName
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set w 12
    set h [expr {$_height - 2 * ($lineht + 2)}]
    if { $h < 1 } {
        return
    }
    if { [string match "component*" $fname] } {
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
    # If there's a title too, subtract one more line
    if { $title != "" } {
        incr h -$lineht
    }
    # Set the legend on the first dataset.
    if { $_currentColormap != "" } {
        set cmap $_currentColormap
        if { ![info exists _colormaps($cmap)] } {
            BuildColormap $cmap
            set _colormaps($cmap) 1
        }
        #SendCmd "legend $cmap $_colorMode $_curFldName {} $w $h 0"
        SendCmd "legend2 $cmap $w $h"
    }
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
        SendCmd "outline color $rgb"
        SendCmd "cutplane color $rgb"
    }
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
        -variable [itcl::scope _settings(-isosurfacevisible)] \
        -command [itcl::code $this AdjustSetting -isosurfacevisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings(-isosurfacewireframe)] \
        -command [itcl::code $this AdjustSetting -isosurfacewireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(-isosurfacelighting)] \
        -command [itcl::code $this AdjustSetting -isosurfacelighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings(-isosurfaceedges)] \
        -command [itcl::code $this AdjustSetting -isosurfaceedges] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings(-outline)] \
        -command [itcl::code $this AdjustSetting -outline] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings(-legendvisible)] \
        -command [itcl::code $this AdjustSetting -legendvisible] \
        -font "Arial 9"

    label $inner.linecolor_l -text "Isolines" -font "Arial 9"
    itk_component add isolineColor {
        Rappture::Combobox $inner.linecolor -width 10 -editable 0
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
        "none"               "none"

    $itk_component(isolineColor) value "white"
    bind $inner.linecolor <<Value>> \
        [itcl::code $this AdjustSetting -isolinecolor]

    label $inner.background_l -text "Background" -font "Arial 9"
    itk_component add background {
        Rappture::Combobox $inner.background -width 10 -editable 0
    }
    $inner.background choices insert end \
        "black"              "black"            \
        "white"              "white"            \
        "grey"               "grey"

    $itk_component(background) value $_settings(-background)
    bind $inner.background <<Value>> \
        [itcl::code $this AdjustSetting -background]

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _widget(-isosurfaceopacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -isosurfaceopacity]
    $inner.opacity set [expr $_settings(-isosurfaceopacity) * 100.0]

    itk_component add field_l {
        label $inner.field_l -text "Field" -font "Arial 9"
    } {
        ignore -font
    }
    itk_component add field {
        Rappture::Combobox $inner.field -width 10 -editable 0
    }
    bind $inner.field <<Value>> \
        [itcl::code $this AdjustSetting -field]

    label $inner.colormap_l -text "Colormap" -font "Arial 9"
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable 0
    }
    $inner.colormap choices insert end [GetColormapList]
    $itk_component(colormap) value "BCGYR"
    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting -colormap]

    label $inner.numcontours_l -text "Number of Isosurfaces" -font "Arial 9"
    itk_component add numcontours {
        Rappture::Spinint $inner.numcontours \
            -min 0 -max 50 -font "arial 9"
    }
    $itk_component(numcontours) value $_settings(-numcontours)
    bind $itk_component(numcontours) <<Value>> \
        [itcl::code $this AdjustSetting -numcontours]

    # add widgets for setting a custom range on the legend

    itk_component add crange {
        checkbutton $inner.crange \
            -text "Use Custom Range:" \
            -variable [itcl::scope _settings(-customrange)] \
            -command [itcl::code $this ToggleCustomRange] \
            -font "Arial 9"
    }

    itk_component add l_min {
        label $inner.l_min -text "Min" -font "Arial 9"
    }
    itk_component add min {
        Rappture::Gauge $inner.min -font "Arial 9" \
            -validatecommand [itcl::code $this LegendRangeValidate "" vmin]
    }
    bind $itk_component(min) <<Value>> \
        [itcl::code $this AdjustSetting -range]

    itk_component add l_max {
        label $inner.l_max -text "Max" -font "Arial 9"
    }
    itk_component add max {
        Rappture::Gauge $inner.max -font "Arial 9" \
            -validatecommand [itcl::code $this LegendRangeValidate "" vmax]
    }
    bind $itk_component(max) <<Value>> \
        [itcl::code $this AdjustSetting -range]

    $itk_component(min) configure -state disabled
    $itk_component(max) configure -state disabled

    blt::table $inner \
        0,0 $inner.field_l      -anchor w -pady 2 \
        0,1 $inner.field        -anchor w -pady 2 -fill x \
        1,0 $inner.colormap_l   -anchor w -pady 2 \
        1,1 $inner.colormap     -anchor w -pady 2 -fill x \
        2,0 $inner.linecolor_l  -anchor w -pady 2 \
        2,1 $inner.linecolor    -anchor w -pady 2 -fill x \
        3,0 $inner.background_l -anchor w -pady 2 \
        3,1 $inner.background   -anchor w -pady 2 -fill x \
        4,0 $inner.numcontours_l -anchor w -pady 2 \
        4,1 $inner.numcontours  -anchor w -pady 2 \
        5,0 $inner.wireframe    -anchor w -pady 2 -cspan 2 \
        6,0 $inner.lighting     -anchor w -pady 2 -cspan 2 \
        7,0 $inner.edges        -anchor w -pady 2 -cspan 2 \
        8,0 $inner.outline      -anchor w -pady 2 -cspan 2 \
        9,0 $inner.legend       -anchor w -pady 2 \
        10,0 $inner.opacity_l   -anchor w -pady 2 \
        10,1 $inner.opacity     -fill x   -pady 2 -fill x \
        11,0 $inner.crange      -anchor w -pady 2 -cspan 2 \
        12,0 $inner.l_min       -anchor w -pady 2 \
        12,1 $inner.min         -anchor w -pady 2 -fill x \
        13,0 $inner.l_max       -anchor w -pady 2 \
        13,1 $inner.max         -anchor w -pady 2 -fill x \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r14 c1 -resize expand
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon axis2]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Axes" \
        -variable [itcl::scope _settings(-axesvisible)] \
        -command [itcl::code $this AdjustSetting -axesvisible] \
        -font "Arial 9"

    checkbutton $inner.labels \
        -text "Axis Labels" \
        -variable [itcl::scope _settings(-axislabels)] \
        -command [itcl::code $this AdjustSetting -axislabels] \
        -font "Arial 9"
    label $inner.grid_l -text "Grid" -font "Arial 9"
    checkbutton $inner.xgrid \
        -text "X" \
        -variable [itcl::scope _settings(-xgrid)] \
        -command [itcl::code $this AdjustSetting -xgrid] \
        -font "Arial 9"
    checkbutton $inner.ygrid \
        -text "Y" \
        -variable [itcl::scope _settings(-ygrid)] \
        -command [itcl::code $this AdjustSetting -ygrid] \
        -font "Arial 9"
    checkbutton $inner.zgrid \
        -text "Z" \
        -variable [itcl::scope _settings(-zgrid)] \
        -command [itcl::code $this AdjustSetting -zgrid] \
        -font "Arial 9"
    checkbutton $inner.minorticks \
        -text "Minor Ticks" \
        -variable [itcl::scope _settings(-axisminorticks)] \
        -command [itcl::code $this AdjustSetting -axisminorticks] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9"

    itk_component add axisMode {
        Rappture::Combobox $inner.mode -width 10 -editable 0
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "farthest" \
        "outer_edges"     "outer"
    $itk_component(axisMode) value $_settings(-axismode)
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting -axismode]

    blt::table $inner \
        0,0 $inner.visible    -anchor w -cspan 4 \
        1,0 $inner.labels     -anchor w -cspan 4 \
        2,0 $inner.minorticks -anchor w -cspan 4 \
        4,0 $inner.grid_l     -anchor w \
        4,1 $inner.xgrid      -anchor w \
        4,2 $inner.ygrid      -anchor w \
        4,3 $inner.zgrid      -anchor w \
        5,0 $inner.mode_l     -anchor w -padx { 2 0 } \
        5,1 $inner.mode       -fill x   -cspan 3

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c6 -resize expand
    blt::table configure $inner r3 -height 0.125i
}


itcl::body Rappture::VtkIsosurfaceViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    label $inner.view_l -text "view" -font "Arial 9"
    set f [frame $inner.view]
    foreach side { front back left right top bottom } {
        button $f.$side  -image [Rappture::icon view$side] \
            -command [itcl::code $this SetOrientation $side]
        Rappture::Tooltip::for $f.$side "Change the view to $side"
        pack $f.$side -side left
    }

    blt::table $inner \
        0,0 $inner.view_l -anchor e -pady 2 \
        0,1 $inner.view -anchor w -pady 2
    blt::table configure $inner r0 -resize none

    set labels { qx qy qz qw xpan ypan zoom }
    set row 1
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _view(-$tag)]
        bind $inner.${tag} <Return> \
            [itcl::code $this camera set -${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this camera set -${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }
    checkbutton $inner.ortho \
        -text "Orthographic Projection" \
        -variable [itcl::scope _view(-ortho)] \
        -command [itcl::code $this AdjustSetting -ortho] \
        -font "Arial 9"
    blt::table $inner \
            $row,0 $inner.ortho -cspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::VtkIsosurfaceViewer::BuildCutplanesTab {} {

    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]

    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Cutplanes" \
        -variable [itcl::scope _settings(-cutplanesvisible)] \
        -command [itcl::code $this AdjustSetting -cutplanesvisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings(-cutplanewireframe)] \
        -command [itcl::code $this AdjustSetting -cutplanewireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(-cutplanelighting)] \
        -command [itcl::code $this AdjustSetting -cutplanelighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings(-cutplaneedges)] \
        -command [itcl::code $this AdjustSetting -cutplaneedges] \
        -font "Arial 9"

    checkbutton $inner.preinterp \
        -text "Interpolate Scalars" \
        -variable [itcl::scope _settings(-cutplanepreinterp)] \
        -command [itcl::code $this AdjustSetting -cutplanepreinterp] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _widget(-cutplaneopacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -cutplaneopacity]
    $inner.opacity set [expr $_settings(-cutplaneopacity) * 100.0]

    # X-value slicer...
    itk_component add xbutton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane-red] \
            -offimage [Rappture::icon x-cutplane-red] \
            -command [itcl::code $this AdjustSetting -xcutplanevisible] \
            -variable [itcl::scope _settings(-xcutplanevisible)] \
    }
    Rappture::Tooltip::for $itk_component(xbutton) \
        "Toggle the X-axis cutplane on/off"
    $itk_component(xbutton) select
    itk_component add xposition {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane x] \
            -variable [itcl::scope _settings(-xcutplaneposition)] \
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
            -command [itcl::code $this AdjustSetting -ycutplanevisible] \
            -variable [itcl::scope _settings(-ycutplanevisible)] \
    }
    Rappture::Tooltip::for $itk_component(ybutton) \
        "Toggle the Y-axis cutplane on/off"
    $itk_component(ybutton) select

    itk_component add yposition {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane y] \
            -variable [itcl::scope _settings(-ycutplaneposition)] \
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
            -command [itcl::code $this AdjustSetting -zcutplanevisible] \
            -variable [itcl::scope _settings(-zcutplanevisible)] \
    } {
        usual
        ignore -foreground
    }
    Rappture::Tooltip::for $itk_component(zbutton) \
        "Toggle the Z-axis cutplane on/off"
    $itk_component(zbutton) select

    itk_component add zposition {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane z] \
            -variable [itcl::scope _settings(-zcutplaneposition)] \
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
        0,0 $inner.visible   -anchor w -pady 2 -cspan 3 \
        1,0 $inner.lighting  -anchor w -pady 2 -cspan 3 \
        2,0 $inner.wireframe -anchor w -pady 2 -cspan 3 \
        3,0 $inner.edges     -anchor w -pady 2 -cspan 3 \
        4,0 $inner.preinterp -anchor w -pady 2 -cspan 3 \
        5,0 $inner.opacity_l -anchor w -pady 2 -cspan 1 \
        5,1 $inner.opacity   -fill x   -pady 2 -cspan 3 \
        6,0 $inner.xbutton   -anchor w -padx 2 -pady 2 \
        7,0 $inner.ybutton   -anchor w -padx 2 -pady 2 \
        8,0 $inner.zbutton   -anchor w -padx 2 -pady 2 \
        6,1 $inner.xval      -fill y -rspan 4 \
        6,2 $inner.yval      -fill y -rspan 4 \
        6,3 $inner.zval      -fill y -rspan 4 \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r9 c4 -resize expand
}

#
# camera --
#
itcl::body Rappture::VtkIsosurfaceViewer::camera {option args} {
    switch -- $option {
        "show" {
            puts [array get _view]
        }
        "set" {
            set what [lindex $args 0]
            set x $_view($what)
            set code [catch { string is double $x } result]
            if { $code != 0 || !$result } {
                return
            }
            switch -- $what {
                "-ortho" {
                    if {$_view($what)} {
                        SendCmd "camera mode ortho"
                    } else {
                        SendCmd "camera mode persp"
                    }
                }
                "-xpan" - "-ypan" {
                    PanCamera
                }
                "-qx" - "-qy" - "-qz" - "-qw" {
                    set q [ViewToQuaternion]
                    $_arcball quaternion $q
                    EventuallyRotate $q
                }
                "-zoom" {
                    SendCmd "camera zoom $_view($what)"
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
            append bytes "$contents\n"
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
    DebugTrace "Enter"
    # Parse style string.
    set tag $dataobj-$comp
    array set style {
        -color                  BCGYR
        -cutplaneedges          0
        -cutplanelighting       1
        -cutplaneopacity        1.0
        -cutplanepreinterp      1
        -cutplanesvisible       0
        -cutplanewireframe      0
        -edgecolor              black
        -edges                  0
        -isosurfacevisible      1
        -levels                 10
        -lighting               1
        -linewidth              1.0
        -opacity                0.6
        -outline                0
        -wireframe              0
        -xcutplaneposition      50
        -xcutplanevisible       1
        -ycutplaneposition      50
        -ycutplanevisible       1
        -zcutplaneposition      50
        -zcutplanevisible       1
    }
    if { $style(-levels) == 1 } {
        set style(-opacity) 1.0
    }
    array set style [$dataobj style $comp]
    #DebugTrace [array get style]

    # This is too complicated.  We want to set the colormap, number of
    # isolines and opacity for the dataset.  They can be the default values,
    # the style hints loaded with the dataset, or set by user controls.  As
    # datasets get loaded, they first use the defaults that are overidden
    # by the style hints.  If the user changes the global controls, then that
    # overrides everything else.  I don't know what it means when global
    # controls are specified as style hints by each dataset.  It complicates
    # the code to handle aberrant cases.

    if { $_changed(-isosurfaceedges) } {
        set style(-edges) $_settings(-isosurfaceedges)
    }
    if { $_changed(-isosurfacelighting) } {
        set style(-lighting) $_settings(-isosurfacelighting)
    }
    if { $_changed(-isosurfaceopacity) } {
        set style(-opacity) $_settings(-isosurfaceopacity)
    }
    if { $_changed(-isosurfacewireframe) } {
        set style(-wireframe) $_settings(-isosurfacewireframe)
    }
    if { $_changed(-numcontours) } {
        set style(-levels) $_settings(-numcontours)
    }
    if { $_changed(-colormap) } {
        set style(-color) $_settings(-colormap)
    }
    if { $_currentColormap == "" } {
        SetCurrentColormap $style(-color)
        $itk_component(colormap) value $style(-color)
    }
    if { $_contourList(numLevels) != $style(-levels) } {
        if { [llength $style(-levels)] > 1 } {
            set _contourList(reqValues) [lsort -real $style(-levels)]
        } else {
            set _settings(-numcontours) $style(-levels)
            $itk_component(numcontours) value $style(-levels)
            set _contourList(numLevels) $style(-levels)
        }
        EventuallyChangeContourLevels
    }
    foreach setting {-outline -isosurfacevisible -cutplanesvisible \
                     -xcutplanevisible -ycutplanevisible -zcutplanevisible \
                     -xcutplaneposition -ycutplaneposition -zcutplaneposition \
                     -cutplaneedges -cutplanelighting -cutplaneopacity \
                     -cutplanepreinterp -cutplanewireframe} {
        if {$_changed($setting)} {
            # User-modified UI setting overrides style
            set style($setting) $_settings($setting)
        } else {
            # Set UI control to style setting (tool provided or default)
            set _settings($setting) $style($setting)
        }
    }

    SendCmd "cutplane add $tag"
    SendCmd "cutplane color [Color2RGB $itk_option(-plotforeground)] $tag"
    foreach axis {x y z} {
        set pos [expr $style(-${axis}cutplaneposition) * 0.01]
        set visible $style(-${axis}cutplanevisible)
        SendCmd "cutplane slice $axis $pos $tag"
        SendCmd "cutplane axis $axis $visible $tag"
    }
    SendCmd "cutplane edges $style(-cutplaneedges) $tag"
    SendCmd "cutplane lighting $style(-cutplanelighting) $tag"
    SendCmd "cutplane opacity $style(-cutplaneopacity) $tag"
    set _widget(-cutplaneopacity) [expr $style(-cutplaneopacity) * 100]
    SendCmd "cutplane preinterp $style(-cutplanepreinterp) $tag"
    SendCmd "cutplane wireframe $style(-cutplanewireframe) $tag"
    SendCmd "cutplane visible $style(-cutplanesvisible) $tag"

    SendCmd "outline add $tag"
    SendCmd "outline color [Color2RGB $itk_option(-plotforeground)] $tag"
    SendCmd "outline visible $style(-outline) $tag"

    GenerateContourList
    SendCmd [list contour3d add contourlist $_contourList(values) $tag]
    SendCmd "contour3d visible $style(-isosurfacevisible) $tag"
    SendCmd "contour3d edges $style(-edges) $tag"
    set _settings(-isosurfaceedges) $style(-edges)
    #SendCmd "contour3d color [Color2RGB $style(-color)] $tag"
    SendCmd "contour3d lighting $style(-lighting) $tag"
    set _settings(-isosurfacelighting) $style(-lighting)
    SendCmd "contour3d linecolor [Color2RGB $style(-edgecolor)] $tag"
    SendCmd "contour3d linewidth $style(-linewidth) $tag"
    SendCmd "contour3d opacity $style(-opacity) $tag"
    set _settings(-isosurfaceopacity) $style(-opacity)
    set _widget(-isosurfaceopacity) [expr $style(-opacity) * 100]
    SetCurrentColormap $style(-color)
    SendCmd "contour3d wireframe $style(-wireframe) $tag"
    set _settings(-isosurfacewireframe) $style(-wireframe)
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

# ----------------------------------------------------------------------
# USAGE: LegendB1Motion press <x> <y>
# USAGE: LegendB1Motion motion <x> <y>
# USAGE: LegendB1Motion release <x> <y>
#
# Manage actions for Button 1 presses that happen over the legend.
# Pressing mouse Button 1 on the legend sends a command to the
# visualization server to show a specific isosurface.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendB1Motion { status x y } {

    switch -- $status {
        "press" {
            DisableMouseRotationBindings
            LegendProbeSingleContour $x $y
        }
        "motion" {
            DisableMouseRotationBindings
            LegendProbeSingleContour $x $y
        }
        "release" {
            AdjustSetting -range
            SetupMouseRotationBindings
        }
        default {
            error "bad option \"$option\": should be one of press, motion, release."
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: LegendPointToValue <x> <y>
#
# Convert an x,y point on the legend to a numeric field value.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendPointToValue { x y } {
    set fname $_curFldName

    set font "Arial 8"
    set lineht [font metrics $font -linespace]

    set ih [image height $_image(legend)]
    set iy [expr $y - ($lineht + 2)]

    # Compute the value of the point
    if { [info exists _limits($fname)] } {
        if { $_settings(-customrange) } {
            set vmin [$itk_component(min) value]
            set vmax [$itk_component(max) value]
        } else {
            foreach { vmin vmax } $_limits($fname) break
        }
        set t [expr 1.0 - (double($iy) / double($ih-1))]
        set value [expr $t * ($vmax - $vmin) + $vmin]
    } else {
        set value 0.0
    }
    return $value
}

# ----------------------------------------------------------------------
# USAGE: LegendProbeSingleContour <x> <y>
#
# Calculate the isosurface value for the x,y point and send a commands
# to the visualization server to show that isosurface.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendProbeSingleContour { x y } {

    set value [LegendPointToValue $x $y]
    SendCmd [list contour3d contourlist $value]
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

    if { [string match "component*" $fname] } {
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
    set value [LegendPointToValue $x $y]

    # Setup the location of the tooltip
    set tx [expr $x + 15]
    set ty [expr $y - 5]

    # Setup the text for the tooltip
    if { [info exists _isolines($y)] } {
        Rappture::Tooltip::text $c [format "$title %g (isosurface)" $_isolines($y)]
    } else {
        Rappture::Tooltip::text $c [format "$title %g" $value]
    }

    # Show the tooltip
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
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
#
itcl::body Rappture::VtkIsosurfaceViewer::ReceiveLegend { colormap title min max size } {
    DebugTrace "Enter"
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
# Draws the legend on the canvas on the right side of the plot area.
#
itcl::body Rappture::VtkIsosurfaceViewer::DrawLegend {} {
    set fname $_curFldName
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]

    if { [string match "component*" $fname] } {
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
    if { !$_settings(-legendvisible) } {
        $c delete legend
        return
    }
    if { [$c find withtag "legend"] == "" } {
        set y 2
        # If there's a legend title, create a text item for the title.
        $c create text $x $y \
            -anchor ne \
            -fill $itk_option(-plotforeground) -tags "title legend" \
            -font $font
        if { $title != "" } {
            incr y $lineht
        }
        $c create text $x $y \
            -anchor ne \
            -fill $itk_option(-plotforeground) -tags "vmax legend" \
            -font $font
        incr y $lineht
        $c create image $x $y \
            -anchor ne \
            -image $_image(legend) -tags "colormap legend"
        $c create rectangle $x $y 1 1 \
            -fill "" -outline "" -tags "sensor legend"
        $c create text $x [expr {$h-2}] \
            -anchor se \
            -fill $itk_option(-plotforeground) -tags "vmin legend" \
            -font $font
        $c bind sensor <Enter> [itcl::code $this EnterLegend %x %y]
        $c bind sensor <Leave> [itcl::code $this LeaveLegend]
        $c bind sensor <Motion> [itcl::code $this MotionLegend %x %y]
#        $c bind sensor <ButtonPress-1>   [itcl::code $this LegendB1Motion press %x %y]
#        $c bind sensor <B1-Motion>       [itcl::code $this LegendB1Motion motion %x %y]
#        $c bind sensor <ButtonRelease-1> [itcl::code $this LegendB1Motion release %x %y]
    }
    $c delete isoline
    set x2 $x
    set iw [image width $_image(legend)]
    set ih [image height $_image(legend)]
    set x1 [expr $x2 - ($iw*12)/10]
    set color $_settings(-isolinecolor)
    # Draw the isolines on the legend.
    array unset _isolines
    if { $color != "none"  && [info exists _limits($_curFldName)] &&
         $_settings(-numcontours) > 0 } {

        if { $_settings(-customrange) } {
            set vmin [$itk_component(min) value]
            set vmax [$itk_component(max) value]
        } else {
            foreach { vmin vmax } $_limits($_curFldName) break
        }
        set range [expr double($vmax - $vmin)]
        if { $range <= 0.0 } {
            set range 1.0;              # Min is greater or equal to max.
        }
        set tags "isoline legend"
        set offset [expr 2 + $lineht]
        if { $title != "" } {
            incr offset $lineht
        }
        foreach value $_contourList(values) {
            set norm [expr 1.0 - (($value - $vmin) / $range)]
            set y1 [expr int(round(($norm * $ih) + $offset))]
            for { set off 0 } { $off < 3 } { incr off } {
                set _isolines([expr $y1 + $off]) $value
                set _isolines([expr $y1 - $off]) $value
            }
            $c create line $x1 $y1 $x2 $y1 -fill $color -tags $tags
        }
    }

    $c bind title <ButtonPress> [itcl::code $this LegendTitleAction post]
    $c bind title <Enter> [itcl::code $this LegendTitleAction enter]
    $c bind title <Leave> [itcl::code $this LegendTitleAction leave]
    # Reset the item coordinates according the current size of the plot.
    $c itemconfigure title -text $title
    if { [info exists _limits($_curFldName)] } {
        if { $_settings(-customrange) } {
            set vmin [$itk_component(min) value]
            set vmax [$itk_component(max) value]
        } else {
            foreach { vmin vmax } $_limits($_curFldName) break
        }
        $c itemconfigure vmin -text [format %g $vmin]
        $c itemconfigure vmax -text [format %g $vmax]
    }
    set y 2
    # If there's a legend title, move the title to the correct position
    if { $title != "" } {
        $c itemconfigure title -text $title
        $c coords title $x $y
        incr y $lineht
        $c raise title
    }
    $c coords vmax $x $y
    incr y $lineht
    $c coords colormap $x $y
    $c coords sensor [expr $x - $iw] $y $x [expr $y + $ih]
    $c raise sensor
    $c coords vmin $x [expr {$h - 2}]

    $c bind vmin <ButtonPress> [itcl::code $this LegendRangeAction popup vmin]
    $c bind vmin <Enter> [itcl::code $this LegendRangeAction enter vmin]
    $c bind vmin <Leave> [itcl::code $this LegendRangeAction leave vmin]

    $c bind vmax <ButtonPress> [itcl::code $this LegendRangeAction popup vmax]
    $c bind vmax <Enter> [itcl::code $this LegendRangeAction enter vmax]
    $c bind vmax <Leave> [itcl::code $this LegendRangeAction leave vmax]
}

# ----------------------------------------------------------------------
# USAGE: LegendTitleAction post
# USAGE: LegendTitleAction enter
# USAGE: LegendTitleAction leave
# USAGE: LegendTitleAction save
#
# Used internally to handle the dropdown list for the fields menu combobox.
# The post option is invoked when the field title is pressed to launch the
# dropdown.  The enter option is invoked when the user mouses over the field
# title. The leave option is invoked when the user moves the mouse away
# from the field title.  The save option is invoked whenever there is a
# selection from the list, to alert the visualization server.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendTitleAction {option} {
    set c $itk_component(view)
    switch -- $option {
        post {
            foreach { x1 y1 x2 y2 } [$c bbox title] break
            set cw [winfo width $itk_component(view)]
            set mw [winfo reqwidth $itk_component(fieldmenu)]
            set x1 [expr $cw - $mw]
            set x [expr $x1 + [winfo rootx $itk_component(view)]]
            set y [expr $y2 + [winfo rooty $itk_component(view)]]
            tk_popup $itk_component(fieldmenu) $x $y
        }
        enter {
            $c itemconfigure title -fill red
        }
        leave {
            $c itemconfigure title -fill $itk_option(-plotforeground)
        }
        save {
            $itk_component(field) value $_curFldLabel
            AdjustSetting -field
        }
        default {
            error "bad option \"$option\": should be post, enter, leave or save"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: LegendRangeValidate <widget> <which> <value>
#
# Used internally to validate a legend range min/max value.
# Returns a boolean value telling if <value> was accepted (1) or rejected (0)
# If the value is rejected, a tooltip/warning message is popped up
# near the widget that asked for the validation, specified by <widget>
#
# <widget> is the widget where a tooltip/warning message should show up on
# <which> is either "vmin" or "vmax".
# <value> is the value to be validated.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendRangeValidate {widget which value} {
    #check for a valid value
    if {[string is double $value] != 1} {
        set msg "should be valid number"
        if {$widget != ""} {
            Rappture::Tooltip::cue $widget $msg
        } else {
            # error "bad value \"$value\": $msg"
            error $msg
        }
        return 0
    }

    switch -- $which {
        vmin {
            # check for min > max
            if {$value > [$itk_component(max) value]} {
                set msg "min > max, change max first"
                if {$widget != ""} {
                    Rappture::Tooltip::cue $widget $msg
                } else {
                    # error "bad value \"$value\": $msg"
                    error $msg
                }
                return 0
            }
        }
        vmax {
            # check for max < min
            if {$value < [$itk_component(min) value]} {
                set msg "max < min, change min first"
                if {$widget != ""} {
                    Rappture::Tooltip::cue $widget $msg
                } else {
                    # error "bad value \"$value\": $msg"
                    error $msg
                }
                return 0
            }
        }
        default {
            error "bad option \"$which\": should be vmin, vmax"
        }
    }
}

itcl::body Rappture::VtkIsosurfaceViewer::MouseOver2Which {} {
    switch -- $_mouseOver {
        vmin {
            set which min
        }
        vmax {
            set which max
        }
        default {
            error "bad _mouseOver \"$_mouseOver\": should be vmin, vmax"
        }
    }
    return $which
}

# ----------------------------------------------------------------------
# USAGE: LegendRangeAction enter <which>
# USAGE: LegendRangeAction leave <which>
#
# USAGE: LegendRangeAction popup <which>
# USAGE: LegendRangeAction activate
# USAGE: LegendRangeAction validate <value>
# USAGE: LegendRangeAction apply <value>
#
# Used internally to handle the mouseover and popup entry for the field range
# inputs.  The enter option is invoked when the user moves the mouse over the
# min or max field range. The leave option is invoked when the user moves the
# mouse away from the min or max field range. The popup option is invoked when
# the user click's on a field range. The popup option stores internally which
# widget is requesting a popup ( in the _mouseOver variable) and calls the
# activate command of the widget. The widget's activate command calls back to
# this method to get the xywh dimensions of the popup editor. After the user
# changes focus or sets the value in the editor, the editor calls this methods
# validate and apply options to set the value.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::LegendRangeAction {option args} {
    set c $itk_component(view)

    switch -- $option {
        enter {
            set which [lindex $args 0]
            $c itemconfigure $which -fill red
        }
        leave {
            set which [lindex $args 0]
            $c itemconfigure $which -fill $itk_option(-plotforeground)
        }
        popup {
            DisableMouseRotationBindings
            set which [lindex $args 0]
            set _mouseOver $which
            $itk_component(editor) activate
        }
        activate {
            foreach { x1 y1 x2 y2 } [$c bbox $_mouseOver] break
            set which [MouseOver2Which]
            set info(text) [$itk_component($which) value]
            set info(x) [expr $x1 + [winfo rootx $c]]
            set info(y) [expr $y1 + [winfo rooty $c]]
            set info(w) [expr $x2 - $x1]
            set info(h) [expr $y2 - $y1]
            return [array get info]
        }
        validate {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"editor validate value\""
            }

            set value [lindex $args 0]
            if {[LegendRangeValidate $itk_component(editor) $_mouseOver $value] == 0} {
                return 0
            }

            # value was good, apply it
            # reset the mouse rotation bindings
            SetupMouseRotationBindings
        }
        apply {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"editor apply value\""
            }
            set value [string trim [lindex $args 0]]

            set which [MouseOver2Which]

            # only set custom range if value changed
            if {[$itk_component($which) value] != $value} {
                # set the flag stating the custom range came from the legend
                # change the value in the gauge
                # turn on crange to enable the labels and gauges
                # call AdjustSetting -range (inside ToggleCustomRange)
                # to update drawing and legend
                set _customRangeClick 0
                $itk_component($which) value $value
                $itk_component(crange) select
                ToggleCustomRange
            }
        }
        default {
            error "bad option \"$option\": should be enter, leave, activate, validate, apply"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: ToggleCustomRange
#
# Called whenever the custom range is turned on or off. Used to save
# the custom min and custom max set by the user. When the -customrange
# setting is turned on, the range min and range max gauges are set
# with the last value set by the user, or the default range if no
# previous min and max were set.
#
# When the custom range is turned on, we check how it was turned on
# by querying _customRangeClick. If the variable is 1, this means
# the user clicked the crange checkbutton and we should pull the
# custom range values from our backup variables. If the variable is 0,
# the custom range was enabled through the user manipulating the
# min and max value in the legend.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::ToggleCustomRange {args} {
    if { ! $_settings(-customrange) } {
        # custom range was turned off

        # disable the min/max labels and gauge widgets
        $itk_component(l_min) configure -state disabled
        $itk_component(min) configure -state disabled
        $itk_component(l_max) configure -state disabled
        $itk_component(max) configure -state disabled

        # backup the custom range
        set _settings(-customrangemin) [$itk_component(min) value]
        set _settings(-customrangemax) [$itk_component(max) value]

        # set the gauges to dataset's min and max
        foreach { vmin vmax } $_limits($_curFldName) break
        SetMinMaxGauges $vmin $vmax
    } else {
        # custom range was turned on

        # enable the min/max labels and gauge widgets
        $itk_component(l_min) configure -state normal
        $itk_component(min) configure -state normal
        $itk_component(l_max) configure -state normal
        $itk_component(max) configure -state normal

        # if the custom range is being turned on by clicking the
        # checkbox, restore the min and max gauges from the backup
        # variables. otherwise, new values for the min and max
        # widgets will be set later from the legend's editor.
        if { $_customRangeClick } {
            SetMinMaxGauges $_settings(-customrangemin) $_settings(-customrangemax)
        }

        # reset the click flag
        set _customRangeClick 1
    }
    AdjustSetting -range
}

# ----------------------------------------------------------------------
# USAGE: SetMinMaxGauges <min> <max>
#
# Set the min and max gauges in the correct order, avoiding the
# error where you try to set the min > max before updating the max or
# set the max < min before updating the min.
#
# There are five range cases to consider with our current range validation.
# For example:
# [2,3] -> [0,1]       : update min first, max last
# [2,3] -> [4,5]       : update max first, min last
# [2,3] -> [0,2.5]     : update min or max first
# [2,3] -> [2.5,5]     : update min or max first
# [2,3] -> [2.25,2.75] : update min or max first
#
# In 4 of the cases we can update min first and max last, so we only
# need to check the case where old max < new min, where we update
# max first and min last.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkIsosurfaceViewer::SetMinMaxGauges {min max} {

    if { [$itk_component(max) value] < $min} {
        # old max < new min
        # shift range toward right
        # extend max first, then update min
        $itk_component(max) value $max
        $itk_component(min) value $min
    } else {
        # extend min first, then update max
        $itk_component(min) value $min
        $itk_component(max) value $max
    }
}

#
# SetCurrentColormap --
#
itcl::body Rappture::VtkIsosurfaceViewer::SetCurrentColormap { name } {
    # Keep track of the colormaps that we build.
    if { ![info exists _colormaps($name)] } {
        BuildColormap $name
        set _colormaps($name) 1
    }
    set _currentColormap $name
    SendCmd "contour3d colormap $_currentColormap"
    SendCmd "cutplane colormap $_currentColormap"
}

#
# BuildColormap --
#
# Build the designated colormap on the server.
#
itcl::body Rappture::VtkIsosurfaceViewer::BuildColormap { name } {
    set cmap [ColorsToColormap $name]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    set amap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $amap }"
}

itcl::body Rappture::VtkIsosurfaceViewer::SetOrientation { side } {
    array set positions {
        front "1 0 0 0"
        back  "0 0 1 0"
        left  "0.707107 0 -0.707107 0"
        right "0.707107 0 0.707107 0"
        top   "0.707107 -0.707107 0 0"
        bottom "0.707107 0.707107 0 0"
    }
    foreach name { -qw -qx -qy -qz } value $positions($side) {
        set _view($name) $value
    }
    set q [ViewToQuaternion]
    $_arcball quaternion $q
    SendCmd "camera orient $q"
    SendCmd "camera reset"
    set _view(-xpan) 0
    set _view(-ypan) 0
    set _view(-zoom) 1.0
}

itcl::body Rappture::VtkIsosurfaceViewer::GenerateContourList {} {
    if { ![info exists _limits($_curFldName)] } {
        puts stderr "no _curFldName"
        return ""
    }
    if { $_contourList(numLevels) < 1 } {
        # There are tools that set 0 levels to get cutplanes only
        #puts stderr "numLevels < 1"
        return ""
    }
    if { [llength $_contourList(reqValues)] > 1 } {
        set values $_contourList(reqValues)
    } else {
        # if custom range has been set, use the custom min and max
        # to generate contour list values
        if { $_settings(-customrange) } {
            set vmin [$itk_component(min) value]
            set vmax [$itk_component(max) value]
        } else {
            # use the field limits to calculate the contour list values
            foreach { vmin vmax } $_limits($_curFldName) break
        }

        set v [blt::vector create \#auto]
        $v seq $vmin $vmax [expr $_contourList(numLevels)+2]
        $v delete end 0
        set values [$v range 0 end]
        blt::vector destroy $v
    }
    set _contourList(values) $values
}

itcl::body Rappture::VtkIsosurfaceViewer::SetCurrentFieldName { dataobj } {
    set _first $dataobj
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
                -command [itcl::code $this LegendTitleAction save]
            set _fields($fname) [list $label $units $components]
            if { $_curFldName == "" } {
                set _curFldName $fname
                set _curFldLabel $label
            }
        }
    }
    $itk_component(field) value $_curFldLabel
    if { $_settings(-customrange) } {
        set limits [list [$itk_component(min) value] [$itk_component(max) value]]
        SendCmd "dataset maprange explicit $limits $_curFldName"
        if { $limits != $_currentLimits } {
            set _currentLimits $limits
            EventuallyChangeContourLevels
        }
    } else {
        if { ![info exists _limits($_curFldName)] } {
            SendCmd "dataset maprange all"
        } else {
            set limits $_limits($_curFldName)
            SendCmd "dataset maprange explicit $limits $_curFldName"
            if { $limits != $_currentLimits } {
                set _currentLimits $limits
                EventuallyChangeContourLevels
            }
        }
    }
}
