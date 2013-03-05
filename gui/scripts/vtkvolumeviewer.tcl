# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: VtkVolumeViewer - Vtk volume viewer
#
#  It connects to the Vtk server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
#package require Img

option add *VtkVolumeViewer.width 4i widgetDefault
option add *VtkVolumeViewer*cursor crosshair widgetDefault
option add *VtkVolumeViewer.height 4i widgetDefault
option add *VtkVolumeViewer.foreground black widgetDefault
option add *VtkVolumeViewer.controlBackground gray widgetDefault
option add *VtkVolumeViewer.controlDarkBackground #999999 widgetDefault
option add *VtkVolumeViewer.plotBackground black widgetDefault
option add *VtkVolumeViewer.plotForeground white widgetDefault
option add *VtkVolumeViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkVolumeViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkVolumeViewer::SetServerList
}

itcl::class Rappture::VtkVolumeViewer {
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
    public method parameters {title args} { 
        # do nothing 
    }
    public method scale {args}

    protected method Connect {}
    protected method CurrentDatasets {args}
    protected method Disconnect {}
    protected method DoResize {}
    protected method DoReseed {}
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
    private method BuildVolumeTab {}
    private method ConvertToVtkData { dataobj comp } 
    private method DrawLegend {}
    private method Combo { option }
    private method EnterLegend { x y } 
    private method EventuallyResize { w h } 
    private method EventuallyReseed { numPoints } 
    private method EventuallyRotate { q } 
    private method EventuallySetCutplane { axis args } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method IsValidObject { dataobj } 
    private method LeaveLegend {}
    private method MotionLegend { x y } 
    private method PanCamera {}
    private method RequestLegend {}
    private method SetColormap { dataobj comp }
    private method ChangeColormap { dataobj comp color }
    private method SetLegendTip { x y }
    private method SetObjectStyle { dataobj comp } 
    private method Slice {option args} 

    private variable _arcball ""
    private variable _dlist ""     ;    # list of data objects
    private variable _obj2datasets
    private variable _obj2ovride   ;    # maps dataobj => style override
    private variable _datasets     ;    # contains all the dataobj-component 
                                   ;    # datasets in the server
    private variable _colormaps    ;    # contains all the colormaps
                                   ;    # in the server.
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
    private variable _seeds

    common _downloadPopup;              # download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _reseedPending 0
    private variable _rotatePending 0
    private variable _cutplanePending 0
    private variable _legendPending 0
    private variable _outline
    private variable _fields 
    private variable _curFldName ""
    private variable _curFldLabel ""
    private variable _colorMode "vmag";#  Mode of colormap (vmag or scalar)
}

itk::usual VtkVolumeViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkVolumeViewer::constructor {hostlist args} {
    package require vtk
    set _serverType "vtkvis"

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Reseed event
    $_dispatcher register !reseed
    $_dispatcher dispatch $this !reseed "[itcl::code $this DoReseed]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this RequestLegend]; list"

    # X-Cutplane event
    $_dispatcher register !xcutplane
    $_dispatcher dispatch $this !xcutplane \
        "[itcl::code $this AdjustSetting cutplane-xposition]; list"

    # Y-Cutplane event
    $_dispatcher register !ycutplane
    $_dispatcher dispatch $this !ycutplane \
        "[itcl::code $this AdjustSetting cutplane-yposition]; list"

    # Z-Cutplane event
    $_dispatcher register !zcutplane
    $_dispatcher dispatch $this !zcutplane \
        "[itcl::code $this AdjustSetting cutplane-zposition]; list"

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

    array set _settings {
        axis-xgrid              0
        axis-ygrid              0
        axis-zgrid              0
        axis-xcutplane          0
        axis-ycutplane          0
        axis-zcutplane          0
        axis-xposition          0
        axis-yposition          0
        axis-zposition          0
        axesVisible             1
        axisLabels              1
        cutplaneEdges           0
        cutplane-xvisible       0
        cutplane-yvisible       0
        cutplane-zvisible       0
        cutplane-xposition      50
        cutplane-yposition      50
        cutplane-zposition      50
        cutplaneVisible         1
        cutplaneLighting        1
        cutplaneWireframe       0
        cutplane-opacity        100
        volumeLighting          1
        volume-material         80
        volume-opacity          40
        volume-quality          50
        volumeVisible           1
        legendVisible           1
    }

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

    # Fixes the scrollregion in case we go off screen
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
            -variable [itcl::scope _settings(volumeVisible)] \
            -command [itcl::code $this AdjustSetting volumeVisible] 
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Don't display the volume"
    pack $itk_component(volume) -padx 2 -pady 2

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings(cutplaneVisible)] \
            -command [itcl::code $this AdjustSetting cutplaneVisible] 
    }
    $itk_component(cutplane) select
    Rappture::Tooltip::for $itk_component(cutplane) \
        "Show/Hide cutplanes"
    pack $itk_component(cutplane) -padx 2 -pady 2


    if { [catch {
        BuildVolumeTab
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
    update
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkVolumeViewer::destructor {} {
    Disconnect
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::VtkVolumeViewer::DoResize {} {
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

itcl::body Rappture::VtkVolumeViewer::DoRotate {} {
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::VtkVolumeViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 400 !resize
    }
}

itcl::body Rappture::VtkVolumeViewer::EventuallyReseed { numPoints } {
    set _numSeeds $numPoints
    if { !$_reseedPending } {
        set _reseedPending 1
        $_dispatcher event -after 600 !reseed
    }
}

set rotate_delay 100

itcl::body Rappture::VtkVolumeViewer::EventuallyRotate { q } {
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
    if { !$_rotatePending } {
        set _rotatePending 1
        global rotate_delay 
        $_dispatcher event -after $rotate_delay !rotate
    }
}

itcl::body Rappture::VtkVolumeViewer::EventuallySetCutplane { axis args } {
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
itcl::body Rappture::VtkVolumeViewer::add {dataobj {settings ""}} {
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
itcl::body Rappture::VtkVolumeViewer::delete {args} {
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
        SendCmd "dataset visible 0"
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
itcl::body Rappture::VtkVolumeViewer::get {args} {
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
itcl::body Rappture::VtkVolumeViewer::scale {args} {
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
itcl::body Rappture::VtkVolumeViewer::download {option args} {
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
itcl::body Rappture::VtkVolumeViewer::Connect {} {
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
itcl::body Rappture::VtkVolumeViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkVolumeViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkVolumeViewer::Disconnect {} {
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
itcl::body Rappture::VtkVolumeViewer::ReceiveImage { args } {
    array set info {
        -token "???"
        -bytes 0
        -type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    StopWaiting
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
    if { $_legendPending } {
        RequestLegend
    }
}

#
# ReceiveDataset --
#
itcl::body Rappture::VtkVolumeViewer::ReceiveDataset { args } {
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
itcl::body Rappture::VtkVolumeViewer::Rebuild {} {
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

    set _legendPending 1

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
            lappend info "client" "vtkvolumeviewer"
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
        InitSettings axis-xgrid axis-ygrid axis-zgrid axisFlyMode \
            axesVisible axisLabels
        PanCamera
    }
    set _first ""

    SendCmd "imgflush"
    set _first ""

    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        set _obj2datasets($dataobj) ""
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj vtkdata $comp]
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
                    SendCmd [list "clientinfo" $info]
                }
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
            break
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

    InitSettings volume-palette volume-material volume-quality volumeVisible \
        cutplaneVisible \
        cutplane-xposition cutplane-yposition cutplane-zposition \
        cutplane-xvisible cutplane-yvisible cutplane-zvisible

    if { $_reset } {
        InitSettings volumeLighting
        Zoom reset
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
itcl::body Rappture::VtkVolumeViewer::CurrentDatasets {args} {
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
itcl::body Rappture::VtkVolumeViewer::Zoom {option} {
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
                qw      0.853553
                qx      -0.353553
                qy      0.353553
                qz      0.146447
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

itcl::body Rappture::VtkVolumeViewer::PanCamera {} {
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
itcl::body Rappture::VtkVolumeViewer::Rotate {option x y} {
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

itcl::body Rappture::VtkVolumeViewer::Pick {x y} {
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
itcl::body Rappture::VtkVolumeViewer::Pan {option x y} {
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
itcl::body Rappture::VtkVolumeViewer::InitSettings { args } {
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
itcl::body Rappture::VtkVolumeViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "volumeVisible" {
            set bool $_settings(volumeVisible)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "volume visible $bool $dataset"
            }
            if { $bool } {
                Rappture::Tooltip::for $itk_component(volume) \
                    "Hide the volume"
            } else {
                Rappture::Tooltip::for $itk_component(volume) \
                    "Show the volume"
            }
        }
        "volume-material" {
            set val $_settings(volume-material)
            set diffuse [expr {0.01*$val}]
            set specular [expr {0.01*$val}]
            #set power [expr {sqrt(160*$val+1.0)}]
            set power [expr {$val+1.0}]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "volume shading diffuse $diffuse $dataset"
                SendCmd "volume shading specular $specular $power $dataset"
            }
        }
        "volumeLighting" {
            set bool $_settings(volumeLighting)
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "volume lighting $bool $dataset"
            }
        }
        "volume-quality" {
            set val $_settings(volume-quality)
            set val [expr {0.01*$val}]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "volume quality $val $dataset"
            }
        }
        "axesVisible" {
            set bool $_settings(axesVisible)
            SendCmd "axis visible all $bool"
        }
        "axisLabels" {
            set bool $_settings(axisLabels)
            SendCmd "axis labels all $bool"
        }
        "axis-xgrid" - "axis-ygrid" - "axis-zgrid" {
            set axis [string range $what 5 5]
            set bool $_settings($what)
            SendCmd "axis grid $axis $bool"
        }
        "axisFlyMode" {
            set mode [$itk_component(axismode) value]
            set mode [$itk_component(axismode) translate $mode]
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
        "cutplane-opacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane opacity $sval $dataset"
            }
        }
        "cutplane-xvisible" - "cutplane-yvisible" - "cutplane-zvisible" {
            set axis [string range $what 9 9]
            set bool $_settings($what)
            if { $bool } {
                $itk_component(${axis}CutScale) configure -state normal \
                    -troughcolor white
            } else {
                $itk_component(${axis}CutScale) configure -state disabled \
                    -troughcolor grey82
            }
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane axis $axis $bool $dataset"
            }
        }
        "cutplane-xposition" - "cutplane-yposition" - "cutplane-zposition" {
            set axis [string range $what 9 9]
            set pos [expr $_settings($what) * 0.01]
            foreach dataset [CurrentDatasets -visible] {
                SendCmd "cutplane slice ${axis} ${pos} $dataset"
            }
            set _cutplanePending 0
        }
        "volume-palette" {
            set palette [$itk_component(palette) value]
            set _settings(volume-palette) $palette
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                ChangeColormap $dataobj $comp $palette
            }
            set _legendPending 1
        }
        "volume-palette" {
            set palette [$itk_component(palette) value]
            set _settings(volume-palette) $palette
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                ChangeColormap $dataobj $comp $palette
            }
            set _legendPending 1
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
            SendCmd "volume colormode $_colorMode ${name} $dataset"
            SendCmd "cutplane colormode $_colorMode ${name} $dataset"
            SendCmd "camera reset"
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
#       is determined from the height of the canvas.  It will be rotated
#       to be vertical when drawn.
#
itcl::body Rappture::VtkVolumeViewer::RequestLegend {} {
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set c $itk_component(legend)
    set w 12
    set h [expr {$_height - 3 * ($lineht + 2)}]
    if { $h < 1} {
        return
    }
    # Set the legend on the first volume dataset.
    foreach dataset [CurrentDatasets -visible $_first] {
        foreach {dataobj comp} [split $dataset -] break
        if { [info exists _dataset2style($dataset)] } {
            SendCmdNoWait \
                "legend $_dataset2style($dataset) $_colorMode $_curFldName {} $w $h 0"
            break;
        }
    }
}

#
# ChangeColormap --
#
itcl::body Rappture::VtkVolumeViewer::ChangeColormap {dataobj comp color} {
    set tag $dataobj-$comp
    if { ![info exist _style($tag)] } {
        error "no initial colormap"
    }
    array set style $_style($tag)
    set style(-color) $color
    set _style($tag) [array get style]
    SetColormap $dataobj $comp
}

#
# SetColormap --
#
itcl::body Rappture::VtkVolumeViewer::SetColormap { dataobj comp } {
    array set style {
        -color BCGYR
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
        SendCmd "volume colormap $name $tag"
        SendCmd "cutplane colormap $name-opaque $tag"
        set _dataset2style($tag) $name
    }
}

#
# BuildColormap --
#
itcl::body Rappture::VtkVolumeViewer::BuildColormap { name styles } {
    array set style $styles
    set cmap [ColorsToColormap $style(-color)]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    if { ![info exists _settings(volume-opacity)] } {
        set _settings(volume-opacity) $style(-opacity)
    }
    set max $_settings(volume-opacity)

    set opaqueWmap "0.0 1.0 1.0 1.0"
    #set wmap "0.0 0.0 0.1 0.0 0.2 0.8 0.98 0.8 0.99 0.0 1.0 0.0"
    # Approximate cubic opacity curve
    set wmap "0.0 0.0 0.1 0.001 0.2 0.008 0.3 0.027 0.4 0.064 0.5 0.125 0.6 0.216 0.7 0.343 0.8 0.512 0.9 0.729 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $wmap }"
    SendCmd "colormap add $name-opaque { $cmap } { $opaqueWmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkVolumeViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkVolumeViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::VtkVolumeViewer::BuildVolumeTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.volume \
        -text "Show Volume" \
        -variable [itcl::scope _settings(volumeVisible)] \
        -command [itcl::code $this AdjustSetting volumeVisible] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(volumeLighting)] \
        -command [itcl::code $this AdjustSetting volumeLighting] \
        -font "Arial 9"

    label $inner.dim_l -text "Dim" -font "Arial 9"
    ::scale $inner.material -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(volume-material)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting volume-material]
    label $inner.bright_l -text "Bright" -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(volume-opacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting volume-opacity]

    label $inner.quality_l -text "Quality" -font "Arial 9"
    ::scale $inner.quality -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(volume-quality)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting volume-quality]

    itk_component add field_l {
        label $inner.field_l -text "Field" -font "Arial 9" 
    } {
        ignore -font
    }
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
        [itcl::code $this AdjustSetting volume-palette]

    blt::table $inner \
        0,0 $inner.field_l   -anchor w -pady 2  \
        0,1 $inner.field     -anchor w -pady 2 -cspan 2 \
        1,0 $inner.volume    -anchor w -pady 2 -cspan 4 \
        2,0 $inner.lighting  -anchor w -pady 2 -cspan 4 \
        3,0 $inner.dim_l     -anchor e -pady 2 \
        3,1 $inner.material  -fill x   -pady 2 \
        3,2 $inner.bright_l  -anchor w -pady 2 \
        4,0 $inner.quality_l -anchor w -pady 2 -cspan 2 \
        5,0 $inner.quality   -fill x   -pady 2 -cspan 2 \
        7,0 $inner.palette_l -anchor w -pady 2  \
        7,1 $inner.palette   -anchor w -pady 2 -cspan 2 \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r8 -resize expand
}

itcl::body Rappture::VtkVolumeViewer::BuildAxisTab {} {

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
        -variable [itcl::scope _settings(axisLabels)] \
        -command [itcl::code $this AdjustSetting axisLabels] \
        -font "Arial 9"

    checkbutton $inner.gridx \
        -text "Show X Grid" \
        -variable [itcl::scope _settings(axis-xgrid)] \
        -command [itcl::code $this AdjustSetting axis-xgrid] \
        -font "Arial 9"
    checkbutton $inner.gridy \
        -text "Show Y Grid" \
        -variable [itcl::scope _settings(axis-ygrid)] \
        -command [itcl::code $this AdjustSetting axis-ygrid] \
        -font "Arial 9"
    checkbutton $inner.gridz \
        -text "Show Z Grid" \
        -variable [itcl::scope _settings(axis-zgrid)] \
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
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting axisFlyMode]

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


itcl::body Rappture::VtkVolumeViewer::BuildCameraTab {} {
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

itcl::body Rappture::VtkVolumeViewer::BuildCutplaneTab {} {

    set fg [option get $itk_component(hull) font Font]
    
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]] 

    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Show Cutplanes" \
        -variable [itcl::scope _settings(cutplaneVisible)] \
        -command [itcl::code $this AdjustSetting cutplaneVisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(cutplaneWireframe)] \
        -command [itcl::code $this AdjustSetting cutplaneWireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(cutplaneLighting)] \
        -command [itcl::code $this AdjustSetting cutplaneLighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(cutplaneEdges)] \
        -command [itcl::code $this AdjustSetting cutplaneEdges] \
        -font "Arial 9"

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(cutplane-opacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting cutplane-opacity]
    $inner.opacity set $_settings(cutplane-opacity)

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane] \
            -offimage [Rappture::icon x-cutplane] \
            -command [itcl::code $this AdjustSetting cutplane-xvisible] \
            -variable [itcl::scope _settings(cutplane-xvisible)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X-axis cutplane on/off"

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane x] \
            -variable [itcl::scope _settings(cutplane-xposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    # Set the default cutplane value before disabling the scale.
    $itk_component(xCutScale) set 50
    $itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this Slice tooltip x]"

    # Y-value slicer...
    itk_component add yCutButton {
        Rappture::PushButton $inner.ybutton \
            -onimage [Rappture::icon y-cutplane] \
            -offimage [Rappture::icon y-cutplane] \
            -command [itcl::code $this AdjustSetting cutplane-yvisible] \
            -variable [itcl::scope _settings(cutplane-yvisible)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y-axis cutplane on/off"

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane y] \
            -variable [itcl::scope _settings(cutplane-yposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this Slice tooltip y]"
    # Set the default cutplane value before disabling the scale.
    $itk_component(yCutScale) set 50
    $itk_component(yCutScale) configure -state disabled

    # Z-value slicer...
    itk_component add zCutButton {
        Rappture::PushButton $inner.zbutton \
            -onimage [Rappture::icon z-cutplane] \
            -offimage [Rappture::icon z-cutplane] \
            -command [itcl::code $this AdjustSetting cutplane-zvisible] \
            -variable [itcl::scope _settings(cutplane-zvisible)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z-axis cutplane on/off"

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue yes \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this EventuallySetCutplane z] \
            -variable [itcl::scope _settings(cutplane-zposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    $itk_component(zCutScale) set 50
    $itk_component(zCutScale) configure -state disabled
    #$itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this Slice tooltip z]"

    blt::table $inner \
        0,0 $inner.visible              -anchor w -pady 2 -cspan 4 \
        1,0 $inner.lighting             -anchor w -pady 2 -cspan 4 \
        2,0 $inner.wireframe            -anchor w -pady 2 -cspan 4 \
        3,0 $inner.edges                -anchor w -pady 2 -cspan 4 \
        4,0 $inner.opacity_l            -anchor w -pady 2 -cspan 3 \
        5,0 $inner.opacity              -fill x   -pady 2 -cspan 3 \
        6,0 $itk_component(xCutButton)  -anchor e -padx 2 -pady 2 \
        7,0 $itk_component(xCutScale)   -fill y \
        6,1 $itk_component(yCutButton)  -anchor e -padx 2 -pady 2 \
        7,1 $itk_component(yCutScale)   -fill y \
        6,2 $itk_component(zCutButton)  -anchor e -padx 2 -pady 2 \
        7,2 $itk_component(zCutScale)   -fill y \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c3 -resize expand
}

#
#  camera -- 
#
itcl::body Rappture::VtkVolumeViewer::camera {option args} {
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

itcl::body Rappture::VtkVolumeViewer::ConvertToVtkData { dataobj comp } {
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


itcl::body Rappture::VtkVolumeViewer::GetVtkData { args } {
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

itcl::body Rappture::VtkVolumeViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkVolumeViewer::BuildDownloadPopup { popup command } {
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

itcl::body Rappture::VtkVolumeViewer::SetObjectStyle { dataobj comp } {
    # Parse style string.
    set tag $dataobj-$comp
    set style [$dataobj style $comp]
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
    if { $dataobj != $_first } {
        set settings(-opacity) 1
    }
    array set settings $style
    SendCmd "volume add $tag"
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

    SendCmd "volume lighting $settings(-lighting) $tag"
    set _settings(volumeLighting) $settings(-lighting)
    SetColormap $dataobj $comp
}

itcl::body Rappture::VtkVolumeViewer::IsValidObject { dataobj } {
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
itcl::body Rappture::VtkVolumeViewer::ReceiveLegend { colormap title vmin vmax size } {
    set _legendPending 0
    puts stderr "ReceiveLegend colormap=$colormap title=$title range=$vmin,$vmax size=$size"
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
#       Draws the legend in it's own canvas which resides to the right
#       of the contour plot area.
#
itcl::body Rappture::VtkVolumeViewer::DrawLegend { } {
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
    if { $_settings(legendVisible) } {
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
        $c bind title <ButtonPress> [itcl::code $this Combo post]
        $c bind title <Enter> [itcl::code $this Combo activate]
        $c bind title <Leave> [itcl::code $this Combo deactivate]
        # Reset the item coordinates according the current size of the plot.
        $c itemconfigure title -text $title
        if { [info exists _limits($_curFldName)] } {
            foreach { vmin vmax } $_limits($_curFldName) break
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
itcl::body Rappture::VtkVolumeViewer::EnterLegend { x y } {
    SetLegendTip $x $y
}

#
# MotionLegend --
#
itcl::body Rappture::VtkVolumeViewer::MotionLegend { x y } {
    Rappture::Tooltip::tooltip cancel
    set c $itk_component(view)
    SetLegendTip $x $y
}

#
# LeaveLegend --
#
itcl::body Rappture::VtkVolumeViewer::LeaveLegend { } {
    Rappture::Tooltip::tooltip cancel
    .rappturetooltip configure -icon ""
}

#
# SetLegendTip --
#
itcl::body Rappture::VtkVolumeViewer::SetLegendTip { x y } {
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
        foreach { vmin vmax } $_limits($_curFldName) break
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
itcl::body Rappture::VtkVolumeViewer::Slice {option args} {
    switch -- $option {
        "move" {
            set axis [lindex $args 0]
            set oldval $_settings(axis-${axis}position)
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
itcl::body Rappture::VtkVolumeViewer::Combo {option} {
    set c $itk_component(view) 
    switch -- $option {
        post {
            foreach { x1 y1 x2 y2 } [$c bbox title] break
            set x1 [expr [winfo width $itk_component(view)] - [winfo reqwidth $itk_component(fieldmenu)]]
            set x [expr $x1 + [winfo rootx $itk_component(view)]]
            set y [expr $y2 + [winfo rooty $itk_component(view)]]
            puts stderr "combo x=$x y=$y"
            tk_popup $itk_component(fieldmenu) $x $y
        }
        activate {
            $c itemconfigure title -fill red
        }
        deactivate {
            $c itemconfigure title -fill white 
        }
        invoke {
            $itk_component(field) value _curFldLabel
            AdjustSetting field
        }
        default {
            error "bad option \"$option\": should be post, unpost, select"
        }
    }
}

