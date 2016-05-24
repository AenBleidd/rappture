# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: vtkstreamlinesviewer - Vtk streamlines object viewer
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
package require Img

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
    private method BuildStreamsTab {}
    private method BuildSurfaceTab {}
    private method DrawLegend {}
    private method Connect {}
    private method CurrentDatasets {args}
    private method Disconnect {}
    private method DoRescale {}
    private method DoReseed {}
    private method DoResize {}
    private method DoRotate {}
    private method EnterLegend { x y }
    private method EventuallyRescale { length }
    private method EventuallyReseed { numPoints }
    private method EventuallyResize { w h }
    private method EventuallyRotate { q }
    private method EventuallySetCutplane { axis args }
    private method GetImage { args }
    private method GetVtkData { args }
    private method InitSettings { args }
    private method IsValidObject { dataobj }
    private method LeaveLegend {}
    private method LegendTitleAction { option }
    private method MotionLegend { x y }
    private method Pan {option x y}
    private method PanCamera {}
    private method Pick {x y}
    private method QuaternionToView { q } {
        foreach { _view(-qw) _view(-qx) _view(-qy) _view(-qz) } $q break
    }
    private method Rebuild {}
    private method ReceiveDataset { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { colormap title min max size }
    private method RequestLegend {}
    private method Rotate {option x y}
    private method SetCurrentColormap { color }
    private method SetCurrentFieldName { dataobj }
    private method SetLegendTip { x y }
    private method SetObjectStyle { dataobj comp }
    private method Slice {option args}
    private method SetOrientation { side }
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
    private variable _currentColormap ""

    private variable _click;            # info used for rotate operations
    private variable _limits;           # autoscale min/max for all axes
    private variable _view;             # view params for 3D view
    private variable _settings
    private variable _reset 1;          # Connection to server has been reset.

    private variable _first "";         # This is the topmost dataset.
    private variable _start 0
    private variable _title ""
    private variable _seeds
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _legendPending 0
    private variable _rescalePending 0
    private variable _reseedPending 0
    private variable _rotatePending 0
    private variable _cutplanePending 0
    private variable _fields
    private variable _curFldName ""
    private variable _curFldLabel ""
    private variable _colorMode "vmag"; # Mode of colormap (vmag or scalar)
    private variable _streamlinesLength 0
    private variable _numSeeds 200

    private common _downloadPopup;      # download options from popup
    private common _hardcopy
}

itk::usual VtkStreamlinesViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::constructor {args} {
    set _serverType "vtkvis"

    #DebugOn
    EnableWaitDialog 2000

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this RequestLegend]; list"

    # Rescale streamlines event
    $_dispatcher register !rescale
    $_dispatcher dispatch $this !rescale "[itcl::code $this DoRescale]; list"

    # Reseed event
    $_dispatcher register !reseed
    $_dispatcher dispatch $this !reseed "[itcl::code $this DoReseed]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

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

    array set _settings {
        -axesvisible              1
        -axislabelsvisible        1
        -axisminorticks           1
        -axismode                 "static"
        -cutplaneedges            0
        -cutplanelighting         1
        -cutplaneopacity          100
        -cutplanesvisible         0
        -cutplanewireframe        0
        -legendvisible            1
        -streamlineslighting      1
        -streamlinesmode          lines
        -streamlinesnumseeds      200
        -streamlinesopacity       100
        -streamlineslength        70
        -streamlinesseedsvisible  0
        -streamlinesvisible       1
        -surfaceedges             0
        -surfacelighting          1
        -surfaceopacity           40
        -surfacevisible           1
        -surfacewireframe         0
        -xcutplaneposition        50
        -xcutplanevisible         1
        -xgrid                    0
        -ycutplaneposition        50
        -ycutplanevisible         1
        -ygrid                    0
        -zcutplaneposition        50
        -zcutplanevisible         1
        -zgrid                    0
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

    itk_component add surface {
        Rappture::PushButton $f.surface \
            -onimage [Rappture::icon volume-on] \
            -offimage [Rappture::icon volume-off] \
            -variable [itcl::scope _settings(-surfacevisible)] \
            -command [itcl::code $this AdjustSetting -surfacevisible]
    }
    $itk_component(surface) select
    Rappture::Tooltip::for $itk_component(surface) \
        "Show/Hide the boundary surface"
    pack $itk_component(surface) -padx 2 -pady 2

    itk_component add streamlines {
        Rappture::PushButton $f.streamlines \
            -onimage [Rappture::icon streamlines-on] \
            -offimage [Rappture::icon streamlines-off] \
            -variable [itcl::scope _settings(-streamlinesvisible)] \
            -command [itcl::code $this AdjustSetting -streamlinesvisible] \
    }
    $itk_component(streamlines) select
    Rappture::Tooltip::for $itk_component(streamlines) \
        "Show/Hide the streamlines"
    pack $itk_component(streamlines) -padx 2 -pady 2

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings(-cutplanesvisible)] \
            -command [itcl::code $this AdjustSetting -cutplanesvisible]
    }
    Rappture::Tooltip::for $itk_component(cutplane) \
        "Show/Hide the cutplanes"
    pack $itk_component(cutplane) -padx 2 -pady 2

    if { [catch {
        BuildSurfaceTab
        BuildStreamsTab
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

    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> \
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]

    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

    #bind $itk_component(view) <ButtonRelease-3> \
    #    [itcl::code $this Pick %x %y]

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
    set _start [clock clicks -milliseconds]
    SendCmd "screen size $_width $_height"

    set _legendPending 1
    set _resizePending 0
}

itcl::body Rappture::VtkStreamlinesViewer::DoRotate {} {
    SendCmd "camera orient [ViewToQuaternion]"
    set _rotatePending 0
}

itcl::body Rappture::VtkStreamlinesViewer::DoRescale {} {
    foreach dataset [CurrentDatasets -visible] {
        SendCmd "streamlines length $_streamlinesLength $dataset"
    }
    set _rescalePending 0
}

itcl::body Rappture::VtkStreamlinesViewer::DoReseed {} {
    foreach dataset [CurrentDatasets -visible] {
        # This command works for either random or fmesh seeds
        SendCmd "streamlines seed numpts $_numSeeds $dataset"
    }
    set _reseedPending 0
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 400 !resize
    }
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallyRescale { length } {
    set _streamlinesLength $length
    if { !$_rescalePending } {
        set _rescalePending 1
        $_dispatcher event -after 600 !rescale
    }
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallyReseed { numPoints } {
    set _numSeeds $numPoints
    if { !$_reseedPending } {
        set _reseedPending 1
        $_dispatcher event -after 600 !reseed
    }
}

itcl::body Rappture::VtkStreamlinesViewer::EventuallyRotate { q } {
    QuaternionToView $q
    if { !$_rotatePending } {
        set _rotatePending 1
        $_dispatcher event -after 100 !rotate
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
itcl::body Rappture::VtkStreamlinesViewer::delete {args} {
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
        "-image" {
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
            error "bad option \"$op\": should be -objects, -visible or -image"
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
itcl::body Rappture::VtkStreamlinesViewer::Connect {} {
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
            lappend info "client" "vtkstreamlinesviewer"
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
# Clients use this method to disconnect from the current rendering server.
#
itcl::body Rappture::VtkStreamlinesViewer::Disconnect {} {
    VisViewer::Disconnect

    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !reseed
    $_dispatcher cancel !rotate
    $_dispatcher cancel !xcutplane
    $_dispatcher cancel !ycutplane
    $_dispatcher cancel !zcutplane
    $_dispatcher cancel !legend
    # disconnected -- no more data sitting on server
    array unset _datasets
    array unset _colormaps
    array unset _seeds
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
            fconfigure $f -encoding binary
            puts -nonewline $f $bytes
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
    if { $_legendPending } {
        RequestLegend
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
    set _legendPending 1

    set _first ""
    if { $_reset } {
        set _width $w
        set _height $h
        $_arcball resize $w $h
        DoResize
        InitSettings -xgrid -ygrid -zgrid -axismode \
            -axesvisible -axislabelsvisible -axisminorticks
        # This "imgflush" is to force an image returned before vtkvis starts
        # reading a (big) dataset.  This will display an empty plot with axes
        # at the correct image size.
        SendCmd "imgflush"
    }

    set _first ""
    SendCmd "dataset visible 0"
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
            SetCurrentFieldName $dataobj
        }
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj vtkdata $comp]
                set length [string length $bytes]
                if 0 {
                    set f [open /tmp/vtkstreamlines.vtk "w"]
                    fconfigure $f -translation binary -encoding binary
                    puts -nonewline $f $bytes
                    close $f
                }
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
                SendCmd "dataset visible 1 $tag"
            }
        }
    }
    if {"" != $_first} {
        foreach axis { x y z } {
            set label [$_first hints ${axis}label]
            if { $label != "" } {
                SendCmd [list axis name $axis $label]
            }
            set units [$_first hints ${axis}units]
            if { $units != "" } {
                SendCmd [list axis units $axis $units]
            }
        }
    }

    if { $_reset } {
        InitSettings -streamlinesseedsvisible -streamlinesopacity \
            -streamlinesvisible \
            -streamlineslighting \
            -streamlinescolormap -field \
            -surfacevisible -surfaceedges -surfacelighting -surfaceopacity \
            -surfacewireframe \
            -cutplanesvisible \
            -xcutplaneposition -ycutplaneposition -zcutplaneposition \
            -xcutplanevisible -ycutplanevisible -zcutplanevisible

        # Reset the camera and other view parameters
        $_arcball quaternion [ViewToQuaternion]
        if {$_view(-ortho)} {
            SendCmd "camera mode ortho"
        } else {
            SendCmd "camera mode persp"
        }
        DoRotate
        PanCamera
        Zoom reset
        SendCmd "camera reset"
        set _reset 0
    }
    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    StopBufferingCommands
    blt::busy release $itk_component(hull)
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

itcl::body Rappture::VtkStreamlinesViewer::PanCamera {} {
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
itcl::body Rappture::VtkStreamlinesViewer::InitSettings { args } {
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
itcl::body Rappture::VtkStreamlinesViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "-axesvisible" {
            set bool $_settings($what)
            SendCmd "axis visible all $bool"
        }
        "-axislabelsvisible" {
            set bool $_settings($what)
            SendCmd "axis labels all $bool"
        }
        "-axisminorticks" {
            set bool $_settings($what)
            SendCmd "axis minticks all $bool"
        }
        "-axismode" {
            set mode [$itk_component(axismode) value]
            set mode [$itk_component(axismode) translate $mode]
            set _settings($what) $mode
            SendCmd "axis flymode $mode"
        }
        "-cutplaneedges" {
            set bool $_settings($what)
            SendCmd "cutplane edges $bool"
        }
        "-cutplanesvisible" {
            set bool $_settings($what)
            SendCmd "cutplane visible $bool"
            if { $bool } {
                Rappture::Tooltip::for $itk_component(cutplane) \
                    "Hide the cutplanes"
            } else {
                Rappture::Tooltip::for $itk_component(cutplane) \
                    "Show the cutplanes"
            }
        }
        "-cutplanewireframe" {
            set bool $_settings($what)
            SendCmd "cutplane wireframe $bool"
        }
        "-cutplanelighting" {
            set bool $_settings($what)
            SendCmd "cutplane lighting $bool"
        }
        "-cutplaneopacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            SendCmd "cutplane opacity $sval"
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
            # Get the new limits because the field changed.
            if { ![info exists _limits($_curFldName)] } {
                SendCmd "dataset maprange all"
            } else {
                SendCmd "dataset maprange explicit $_limits($_curFldName) $_curFldName"
            }
            SendCmd "streamlines colormode $_colorMode $_curFldName"
            SendCmd "cutplane colormode $_colorMode $_curFldName"
            DrawLegend
        }
        "-streamlinesseedsvisible" {
            set bool $_settings($what)
            SendCmd "streamlines seed visible $bool"
        }
        "-streamlinesnumseeds" {
            set density $_settings($what)
            EventuallyReseed $density
        }
        "-streamlinesvisible" {
            set bool $_settings($what)
            SendCmd "streamlines visible $bool"
            if { $bool } {
                Rappture::Tooltip::for $itk_component(streamlines) \
                    "Hide the streamlines"
            } else {
                Rappture::Tooltip::for $itk_component(streamlines) \
                    "Show the streamlines"
            }
        }
        "-streamlinesmode" {
            set mode [$itk_component(streammode) value]
            set _settings($what) $mode
            switch -- $mode {
                "lines" {
                    SendCmd "streamlines lines"
                }
                "ribbons" {
                    SendCmd "streamlines ribbons 3 0"
                }
                "tubes" {
                    SendCmd "streamlines tubes 5 3"
                }
            }
        }
        "-streamlinescolormap" {
            set colormap [$itk_component(colormap) value]
            set _settings($what) $colormap
            SetCurrentColormap $colormap
            set _legendPending 1
        }
        "-streamlinesopacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            SendCmd "streamlines opacity $sval"
        }
        "-streamlineslength" {
            set val $_settings($what)
            set sval [expr { (0.01 * double($val)) / 0.7 }]
            foreach axis {x y z} {
                foreach {min max} $_limits($axis) break
                set ${axis}len [expr double($max) - double($min)]
            }
            set length [expr { $sval * ($xlen + $ylen + $zlen) } ]
            EventuallyRescale $length
        }
        "-streamlineslighting" {
            set bool $_settings($what)
            SendCmd "streamlines lighting $bool"
        }
        "-surfaceopacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            SendCmd "polydata opacity $sval"
        }
        "-surfacewireframe" {
            set bool $_settings($what)
            SendCmd "polydata wireframe $bool"
        }
        "-surfacevisible" {
            set bool $_settings($what)
            SendCmd "polydata visible $bool"
            if { $bool } {
                Rappture::Tooltip::for $itk_component(surface) \
                    "Hide the boundary surface"
            } else {
                Rappture::Tooltip::for $itk_component(surface) \
                    "Show the boundary surface"
            }
        }
        "-surfacelighting" {
            set bool $_settings($what)
            SendCmd "polydata lighting $bool"
        }
        "-surfaceedges" {
            set bool $_settings($what)
            SendCmd "polydata edges $bool"
        }
        "-xcutplanevisible" - "-ycutplanevisible" - "-zcutplanevisible" {
            set axis [string range $what 1 1]
            set bool $_settings($what)
            if { $bool } {
                $itk_component(${axis}CutScale) configure -state normal \
                    -troughcolor white
            } else {
                $itk_component(${axis}CutScale) configure -state disabled \
                    -troughcolor grey82
            }
            SendCmd "cutplane axis $axis $bool"
        }
        "-xcutplaneposition" - "-ycutplaneposition" - "-zcutplaneposition" {
            set axis [string range $what 1 1]
            set pos [expr $_settings($what) * 0.01]
            SendCmd "cutplane slice ${axis} ${pos}"
            set _cutplanePending 0
        }
        "-xgrid" - "-ygrid" - "-zgrid" {
            set axis [string range $what 1 1]
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
# is determined from the height of the canvas.  It will be rotated
# to be vertical when drawn.
#
itcl::body Rappture::VtkStreamlinesViewer::RequestLegend {} {
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set w 12
    set h [expr {$_height - 3 * ($lineht + 2)}]
    if { $h < 1 } {
        return
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

#
# SetCurrentColormap --
#
itcl::body Rappture::VtkStreamlinesViewer::SetCurrentColormap { name } {
    # Keep track of the colormaps that we build.
    if { ![info exists _colormaps($name)] } {
        BuildColormap $name
        set _colormaps($name) 1
    }
    set _currentColormap $name
    SendCmd "streamlines colormap $_currentColormap"
    SendCmd "cutplane colormap $_currentColormap"
}

#
# BuildColormap --
#
itcl::body Rappture::VtkStreamlinesViewer::BuildColormap { name } {
    set cmap [ColorsToColormap $name]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    set amap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $amap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkStreamlinesViewer::plotbackground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotbackground)]
        SendCmd "screen bgcolor $rgb"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkStreamlinesViewer::plotforeground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotforeground)]
        SendCmd "axis color all $rgb"
        SendCmd "outline color $rgb"
        SendCmd "cutplane color $rgb"
    }
}

itcl::body Rappture::VtkStreamlinesViewer::BuildSurfaceTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Boundary Surface Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.surface \
        -text "Show Surface" \
        -variable [itcl::scope _settings(-surfacevisible)] \
        -command [itcl::code $this AdjustSetting -surfacevisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(-surfacewireframe)] \
        -command [itcl::code $this AdjustSetting -surfacewireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(-surfacelighting)] \
        -command [itcl::code $this AdjustSetting -surfacelighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(-surfaceedges)] \
        -command [itcl::code $this AdjustSetting -surfaceedges] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-surfaceopacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -surfaceopacity]

    blt::table $inner \
        0,0 $inner.wireframe -anchor w -pady 2 -cspan 2 \
        1,0 $inner.lighting  -anchor w -pady 2 -cspan 2 \
        2,0 $inner.edges     -anchor w -pady 2 -cspan 3 \
        3,0 $inner.opacity_l -anchor w -pady 2 \
        3,1 $inner.opacity   -fill x   -pady 2

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r4 c1 -resize expand
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
        -variable [itcl::scope _settings(-streamlinesvisible)] \
        -command [itcl::code $this AdjustSetting -streamlinesvisible] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(-streamlineslighting)] \
        -command [itcl::code $this AdjustSetting -streamlineslighting] \
        -font "Arial 9"

    checkbutton $inner.seeds \
        -text "Show Seeds" \
        -variable [itcl::scope _settings(-streamlinesseedsvisible)] \
        -command [itcl::code $this AdjustSetting -streamlinesseedsvisible] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9"
    itk_component add streammode {
        Rappture::Combobox $inner.mode -width 10 -editable 0
    }
    $inner.mode choices insert end \
        "lines"    "lines" \
        "ribbons"   "ribbons" \
        "tubes"     "tubes"
    $itk_component(streammode) value $_settings(-streamlinesmode)
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting -streamlinesmode]

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-streamlinesopacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -streamlinesopacity]

    label $inner.density_l -text "No. Seeds" -font "Arial 9"
    ::scale $inner.density -from 1 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings(-streamlinesnumseeds)] \
        -width 10 \
        -showvalue on \
        -command [itcl::code $this AdjustSetting -streamlinesnumseeds]

    label $inner.scale_l -text "Length" -font "Arial 9"
    ::scale $inner.scale -from 1 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-streamlineslength)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -streamlineslength]

    label $inner.field_l -text "Color by" -font "Arial 9"
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
        [itcl::code $this AdjustSetting -streamlinescolormap]

    blt::table $inner \
        0,0 $inner.field_l     -anchor w -pady 2  \
        0,1 $inner.field       -fill x   -pady 2  \
        1,0 $inner.colormap_l  -anchor w -pady 2  \
        1,1 $inner.colormap    -fill x   -pady 2  \
        2,0 $inner.mode_l      -anchor w -pady 2  \
        2,1 $inner.mode        -fill x   -pady 2  \
        3,0 $inner.scale_l     -anchor w -pady 2  \
        3,1 $inner.scale       -fill x   -pady 2  \
        4,0 $inner.opacity_l   -anchor w -pady 2  \
        4,1 $inner.opacity     -fill x -pady 2 \
        5,0 $inner.lighting    -anchor w -pady 2 -cspan 2 \
        6,0 $inner.seeds       -anchor w -pady 2 -cspan 2 \
        7,0 $inner.density_l   -anchor w -pady 2  \
        7,1 $inner.density     -fill x   -pady 2  \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r8 c1 c2 -resize expand
}

itcl::body Rappture::VtkStreamlinesViewer::BuildAxisTab {} {

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
        -variable [itcl::scope _settings(-axislabelsvisible)] \
        -command [itcl::code $this AdjustSetting -axislabelsvisible] \
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

    itk_component add axismode {
        Rappture::Combobox $inner.mode -width 10 -editable 0
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "farthest" \
        "outer_edges"     "outer"
    $itk_component(axismode) value $_settings(-axismode)
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting -axismode]

    blt::table $inner \
        0,0 $inner.visible -anchor w -cspan 4 \
        1,0 $inner.labels  -anchor w -cspan 4 \
        2,0 $inner.minorticks  -anchor w -cspan 4 \
        4,0 $inner.grid_l  -anchor w \
        4,1 $inner.xgrid   -anchor w \
        4,2 $inner.ygrid   -anchor w \
        4,3 $inner.zgrid   -anchor w \
        5,0 $inner.mode_l  -anchor w -padx { 2 0 } \
        5,1 $inner.mode    -fill x   -cspan 3

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c6 -resize expand
    blt::table configure $inner r3 -height 0.125i
}

itcl::body Rappture::VtkStreamlinesViewer::BuildCameraTab {} {
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
        -command [itcl::code $this camera set -ortho] \
        -font "Arial 9"
    blt::table $inner \
            $row,0 $inner.ortho -cspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::VtkStreamlinesViewer::BuildCutplanesTab {} {

    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]

    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Show Cutplanes" \
        -variable [itcl::scope _settings(-cutplanesvisible)] \
        -command [itcl::code $this AdjustSetting -cutplanesvisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(-cutplanewireframe)] \
        -command [itcl::code $this AdjustSetting -cutplanewireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(-cutplanelighting)] \
        -command [itcl::code $this AdjustSetting -cutplanelighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(-cutplaneedges)] \
        -command [itcl::code $this AdjustSetting -cutplaneedges] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-cutplaneopacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -cutplaneopacity]
    $inner.opacity set $_settings(-cutplaneopacity)

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane-red] \
            -offimage [Rappture::icon x-cutplane-red] \
            -command [itcl::code $this AdjustSetting -xcutplanevisible] \
            -variable [itcl::scope _settings(-xcutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X-axis cutplane on/off"
    $itk_component(xCutButton) select

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane x] \
            -variable [itcl::scope _settings(-xcutplaneposition)] \
            -foreground red3 -font "Arial 9 bold"
    } {
        usual
        ignore -borderwidth -highlightthickness -foreground -font
    }
    # Set the default cutplane value before disabling the scale.
    $itk_component(xCutScale) set 50
    $itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this Slice tooltip x]"

    # Y-value slicer...
    itk_component add yCutButton {
        Rappture::PushButton $inner.ybutton \
            -onimage [Rappture::icon y-cutplane-green] \
            -offimage [Rappture::icon y-cutplane-green] \
            -command [itcl::code $this AdjustSetting -ycutplanevisible] \
            -variable [itcl::scope _settings(-ycutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y-axis cutplane on/off"
    $itk_component(yCutButton) select

    itk_component add yCutScale {
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
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this Slice tooltip y]"
    # Set the default cutplane value before disabling the scale.
    $itk_component(yCutScale) set 50
    $itk_component(yCutScale) configure -state disabled

    # Z-value slicer...
    itk_component add zCutButton {
        Rappture::PushButton $inner.zbutton \
            -onimage [Rappture::icon z-cutplane-blue] \
            -offimage [Rappture::icon z-cutplane-blue] \
            -command [itcl::code $this AdjustSetting -zcutplanevisible] \
            -variable [itcl::scope _settings(-zcutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z-axis cutplane on/off"
    $itk_component(zCutButton) select

    itk_component add zCutScale {
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
    $itk_component(zCutScale) set 50
    $itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this Slice tooltip z]"

    blt::table $inner \
        0,0 $inner.lighting             -anchor w -pady 2 -cspan 4 \
        1,0 $inner.wireframe            -anchor w -pady 2 -cspan 4 \
        2,0 $inner.edges                -anchor w -pady 2 -cspan 4 \
        3,0 $inner.opacity_l            -anchor w -pady 2 -cspan 1 \
        3,1 $inner.opacity              -fill x   -pady 2 -cspan 3 \
        4,0 $itk_component(xCutButton)  -anchor w -padx 2 -pady 2 \
        5,0 $itk_component(yCutButton)  -anchor w -padx 2 -pady 2 \
        6,0 $itk_component(zCutButton)  -anchor w -padx 2 -pady 2 \
        4,1 $itk_component(xCutScale)   -fill y -rspan 4 \
        4,2 $itk_component(yCutScale)   -fill y -rspan 4 \
        4,3 $itk_component(zCutScale)   -fill y -rspan 4 \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c4 -resize expand
}

#
# camera --
#
itcl::body Rappture::VtkStreamlinesViewer::camera {option args} {
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

itcl::body Rappture::VtkStreamlinesViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            set contents [$dataobj vtkdata $comp]
            append bytes "$contents\n"
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
        -font "Arial 9" \
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
    array set style {
        -color BCGYR
        -constcolor white
        -edgecolor black
        -edges 0
        -lighting 1
        -linewidth 1.0
        -mode lines
        -numseeds 200
        -opacity 1.0
        -seeds 1
        -seedcolor white
        -streamlineslength 0.7
        -surfacecolor white
        -surfaceedgecolor black
        -surfaceedges 0
        -surfacelighting 1
        -surfaceopacity 0.4
        -surfacevisible 1
        -surfacewireframe 0
        -visible 1
    }
    array set style [$dataobj style $comp]

    StartBufferingCommands
    SendCmd "streamlines add $tag"
    SendCmd "streamlines color [Color2RGB $style(-constcolor)] $tag"
    SendCmd "streamlines edges $style(-edges) $tag"
    SendCmd "streamlines linecolor [Color2RGB $style(-edgecolor)] $tag"
    SendCmd "streamlines linewidth $style(-linewidth) $tag"
    SendCmd "streamlines lighting $style(-lighting) $tag"
    SendCmd "streamlines opacity $style(-opacity) $tag"
    SendCmd "streamlines seed color [Color2RGB $style(-seedcolor)] $tag"
    SendCmd "streamlines seed visible $style(-seeds) $tag"
    SendCmd "streamlines visible $style(-visible) $tag"
    set seeds [$dataobj hints seeds]
    if { $seeds != "" && ![info exists _seeds($dataobj)] } {
        set length [string length $seeds]
        SendCmd "streamlines seed fmesh $style(-numseeds) data follows $length $tag"
        SendData $seeds
        set _seeds($dataobj) 1
    }
    set _settings(-streamlineslighting) $style(-lighting)
    $itk_component(streammode) value $style(-mode)
    AdjustSetting -streamlinesmode
    set _settings(-streamlinesnumseeds) $style(-numseeds)
    set _settings(-streamlinesopacity) [expr $style(-opacity) * 100.0]
    set _settings(-streamlineslength) [expr $style(-streamlineslength) * 100.0]
    set _settings(-streamlinesseedsvisible) $style(-seeds)
    set _settings(-streamlinesvisible) $style(-visible)

    SendCmd "cutplane add $tag"

    SendCmd "polydata add $tag"
    SendCmd "polydata color [Color2RGB $style(-surfacecolor)] $tag"
    SendCmd "polydata colormode constant {} $tag"
    SendCmd "polydata edges $style(-surfaceedges) $tag"
    SendCmd "polydata linecolor [Color2RGB $style(-surfaceedgecolor)] $tag"
    SendCmd "polydata lighting $style(-surfacelighting) $tag"
    SendCmd "polydata opacity $style(-surfaceopacity) $tag"
    SendCmd "polydata wireframe $style(-surfacewireframe) $tag"
    SendCmd "polydata visible $style(-surfacevisible) $tag"
    set _settings(-surfaceedges) $style(-surfaceedges)
    set _settings(-surfacelighting) $style(-surfacelighting)
    set _settings(-surfaceopacity) [expr $style(-surfaceopacity) * 100.0]
    set _settings(-surfacewireframe) $style(-surfacewireframe)
    set _settings(-surfacevisible) $style(-surfacevisible)
    StopBufferingCommands
    SetCurrentColormap $style(-color)
    $itk_component(colormap) value $style(-color)
}

itcl::body Rappture::VtkStreamlinesViewer::IsValidObject { dataobj } {
    if {[catch {$dataobj isa Rappture::Field} valid] != 0 || !$valid} {
        return 0
    }
    return 1
}

# ----------------------------------------------------------------------
# USAGE: ReceiveLegend <colormap> <title> <min> <max> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkStreamlinesViewer::ReceiveLegend { colormap title min max size } {
    set _legendPending 0
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
            puts stderr errs=$errs
        }
    }
}

#
# DrawLegend --
#
# Draws the legend in it's own canvas which resides to the right
# of the contour plot area.
#
itcl::body Rappture::VtkStreamlinesViewer::DrawLegend {} {
    set fname $_curFldName
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]

    if { [info exists _fields($fname)] } {
        foreach { title units } $_fields($fname) break
        if { $units != "" } {
            set title [format "%s (%s)" $title $units]
        }
    } else {
        set title $fname
    }
    if { $_settings(-legendvisible) } {
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
        $c bind title <ButtonPress> [itcl::code $this LegendTitleAction post]
        $c bind title <Enter> [itcl::code $this LegendTitleAction enter]
        $c bind title <Leave> [itcl::code $this LegendTitleAction leave]
        # Reset the item coordinates according the current size of the plot.
        $c itemconfigure title -text $title
        if { [info exists _limits($_curFldName)] } {
            foreach {vmin vmax} $_limits($_curFldName) break
            $c itemconfigure vmin -text [format %g $vmin]
            $c itemconfigure vmax -text [format %g $vmax]
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

    if { [info exists _fields($_title)] } {
        foreach { title units } $_fields($_title) break
        if { $units != "" } {
            set title [format "%s (%s)" $title $units]
        }
    } else {
        set title $_title
    }
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
    if { [info exists _limits($_curFldName)] } {
        foreach {vmin vmax} $_limits($_curFldName) break
        set t [expr 1.0 - (double($imgY) / double($imgHeight-1))]
        set value [expr $t * ($vmax - $vmin) + $vmin]
    } else {
        set value 0.0
    }
    set tipx [expr $x + 15]
    set tipy [expr $y - 5]
    Rappture::Tooltip::text $c "$title $value"
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
itcl::body Rappture::VtkStreamlinesViewer::LegendTitleAction {option} {
    set c $itk_component(view)
    switch -- $option {
        post {
            foreach { x1 y1 x2 y2 } [$c bbox title] break
            set x1 [expr [winfo width $itk_component(view)] - [winfo reqwidth $itk_component(fieldmenu)]]
            set x [expr $x1 + [winfo rootx $itk_component(view)]]
            set y [expr $y2 + [winfo rooty $itk_component(view)]]
            tk_popup $itk_component(fieldmenu) $x $y
        }
        enter {
            $c itemconfigure title -fill red
        }
        leave {
            $c itemconfigure title -fill white
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

itcl::body Rappture::VtkStreamlinesViewer::SetOrientation { side } {
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

itcl::body Rappture::VtkStreamlinesViewer::SetCurrentFieldName { dataobj } {
    set _first $dataobj
    $itk_component(field) choices delete 0 end
    $itk_component(fieldmenu) delete 0 end
    array unset _fields
    set _curFldName ""
    set _curFldLabel ""
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
            if { $_curFldName == "" && $components == 3 } {
                set _curFldName $fname
                set _curFldLabel $label
            }
        }
    }
    $itk_component(field) value $_curFldLabel
}
