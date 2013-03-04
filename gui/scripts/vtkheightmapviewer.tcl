# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: vtkheightmapviewer - Vtk heightmap viewer
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

option add *VtkHeightmapViewer.width 4i widgetDefault
option add *VtkHeightmapViewer*cursor crosshair widgetDefault
option add *VtkHeightmapViewer.height 4i widgetDefault
option add *VtkHeightmapViewer.foreground black widgetDefault
option add *VtkHeightmapViewer.controlBackground gray widgetDefault
option add *VtkHeightmapViewer.controlDarkBackground #999999 widgetDefault
option add *VtkHeightmapViewer.plotBackground black widgetDefault
option add *VtkHeightmapViewer.plotForeground white widgetDefault
option add *VtkHeightmapViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkHeightmapViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkHeightmapViewer::SetServerList
}

itcl::class Rappture::VtkHeightmapViewer {
    inherit Rappture::VisViewer

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""
    itk_option define -mode mode Mode "contour"

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
    public method limits3 { dataobj }
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
    protected method AdjustMode {}
    protected method InitSettings { args  }
    protected method Pan {option x y}
    protected method Pick {x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { colormap title min max size }
    protected method Rotate {option x y}
    protected method Zoom {option}
    
    # The following methods are only used by this class.
    private method BuildAxisTab {}
    private method BuildCameraTab {}
    private method BuildColormap { name colors }
    private method ResetColormap { color }
    private method BuildContourTab {}
    private method BuildDownloadPopup { widget command } 
    private method Combo { option }
    private method ConvertToVtkData { dataobj comp } 
    private method DrawLegend {}
    private method EnterLegend { x y } 
    private method EventuallyRequestLegend {} 
    private method EventuallyResize { w h } 
    private method EventuallyRotate { q } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method IsValidObject { dataobj } 
    private method LeaveLegend {}
    private method MotionLegend { x y } 
    private method PanCamera {}
    private method RequestLegend {}
    private method SetCurrentColormap { stylelist }
    private method SetLegendTip { x y }
    private method SetObjectStyle { dataobj comp } 
    private method GetHeightmapScale {} 
    private method ResetAxes {}

    private variable _arcball ""
    private variable _dlist ""     ;    # list of data objects
    private variable _obj2datasets
    private variable _obj2ovride   ;    # maps dataobj => style override
    private variable _comp2scale;	# maps dataset to the heightmap scale.
    private variable _datasets     ;    # contains all the dataobj-component 
                                   ;    # datasets in the server
    private variable _colormaps    ;    # contains all the colormaps
                                   ;    # in the server.

    # The name of the current colormap used.  The colormap is global to all
    # heightmaps displayed.
    private variable _currentColormap "" ;    

    private variable _click        ;    # info used for rotate operations
    private variable _limits       ;    # Holds overall limits for all dataobjs 
                                        # using the viewer.
    private variable _view         ;    # view params for 3D view
    private variable _settings
    private variable _initialStyle "";  # First found style in dataobjects.
    private variable _reset 1;          # Indicates if camera needs to be reset
                                        # to starting position.
    private variable _beforeConnect 1;  # Indicates if camera needs to be reset
                                        # to starting position.

    private variable _first ""     ;    # This is the topmost dataset.
    private variable _start 0
    private variable _isolines

    common _downloadPopup;              # download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _legendWidth 0
    private variable _legendHeight 0
    private variable _resizePending 0
    private variable _numIsolinesPending 0
    private variable _rotatePending 0
    private variable _legendPending 0
    private variable _fieldNames {} 
    private variable _fields 
    private variable _curFldName ""
    private variable _curFldLabel ""
    private variable _colorMode "vmag";#  Mode of colormap (vmag or scalar)
}

itk::usual VtkHeightmapViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground -mode
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkHeightmapViewer::constructor {hostlist args} {
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

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias dataset [itcl::code $this ReceiveDataset]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias viserror [itcl::code $this ReceiveError]

    # Initialize the view to some default parameters.
    array set _view {
        qw      0.36
        qx      0.25
        qy      0.50
        qz      0.70
        zoom	1.0 
        xpan	0
        ypan	0
        ortho	1
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    array set _settings [subst {
	axisFlymode		"static"
	axisMinorTicks		1
	stretchToFit		0
        axisLabels              1
        axisVisible		1
        axisXGrid		0
        axisYGrid		0
        axisZGrid		0
        colormapVisible         1
        edges			0
        field			"Default"
        heightmapScale		50
        isHeightmap		0
        isolineColor            black
        isolinesVisible         1
        legendVisible           1
        lighting		1
        saveLighting		1
        numIsolines		10
        opacity                 100
        outline			0
        wireframe		0
        saveOpacity		100
    }]

    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth -background 
    }

    itk_component add fieldmenu {
        menu $itk_component(plotarea).menu \
	    -relief flat \
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

    itk_component add mode {
        Rappture::PushButton $f.mode \
            -onimage [Rappture::icon surface] \
            -offimage [Rappture::icon surface] \
            -variable [itcl::scope _settings(isHeightmap)] \
            -command [itcl::code $this AdjustSetting isHeightmap] \
    }
    Rappture::Tooltip::for $itk_component(mode) \
        "Toggle the surface/contour on/off"
    pack $itk_component(mode) -padx 2 -pady 2

    if { [catch {
        BuildContourTab
        BuildAxisTab
        BuildCameraTab
    } errs] != 0 } {
	global errorInfo
        puts stderr "errs=$errs errorInfo=$errorInfo"
    }
    set _image(legend) [image create photo]

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w 
    blt::table configure $itk_component(plotarea) c1 -resize none

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
    set _beforeConnect 0
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkHeightmapViewer::destructor {} {
    Disconnect
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::VtkHeightmapViewer::DoResize {} {
    if { $_width < 2 } {
        set _width 500
    }
    if { $_height < 2 } {
        set _height 500
    }
    set _start [clock clicks -milliseconds]
    SendCmd "screen size [expr $_width - 20] $_height"

    set font "Arial 8"
    set lh [font metrics $font -linespace]
    set h [expr {$_height - 2 * ($lh + 2)}]
    if { $h != $_legendHeight } {
        EventuallyRequestLegend
    } else {
        DrawLegend
    }
    set _resizePending 0
}

itcl::body Rappture::VtkHeightmapViewer::DoRotate {} {
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::VtkHeightmapViewer::EventuallyRequestLegend {} {
    if { !$_legendPending } {
        set _legendPending 1
        $_dispatcher event -idle !legend
    }
}

itcl::body Rappture::VtkHeightmapViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 250 !resize
    }
}

set rotate_delay 100

itcl::body Rappture::VtkHeightmapViewer::EventuallyRotate { q } {
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
    if { !$_rotatePending } {
        set _rotatePending 1
        global rotate_delay 
        $_dispatcher event -after $rotate_delay !rotate
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkHeightmapViewer::add {dataobj {settings ""}} {
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
        set params(-color) white
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
itcl::body Rappture::VtkHeightmapViewer::delete {args} {
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
        # Append to the end of the dataobj list.
        #lappend _dlist $dataobj
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
itcl::body Rappture::VtkHeightmapViewer::get {args} {
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

# 
# scale  --
#
#       This gets called either incrementally as new simulations are
#       added or all at once as a sequence of heightmaps.
#       This  accounts for all objects--even those not showing on the 
#       screen.  Because of this, the limits are appropriate for all 
#       objects as the user scans through data in the ResultSet viewer.
#
itcl::body Rappture::VtkHeightmapViewer::scale {args} {
    foreach dataobj $args {
        foreach axis { x y } {
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
itcl::body Rappture::VtkHeightmapViewer::download {option args} {
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
itcl::body Rappture::VtkHeightmapViewer::Connect {} {
    global readyForNextFrame
    set readyForNextFrame 1
    set _reset 1
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
itcl::body Rappture::VtkHeightmapViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkHeightmapViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkHeightmapViewer::Disconnect {} {
    VisViewer::Disconnect

    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    $_dispatcher cancel !legend
    # disconnected -- no more data sitting on server
    array unset _datasets 
    array unset _data 
    array unset _colormaps 
    array unset _obj2datasets 
    global readyForNextFrame
    set readyForNextFrame 1
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkHeightmapViewer::ReceiveImage { args } {
    global readyForNextFrame
    set readyForNextFrame 1
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
itcl::body Rappture::VtkHeightmapViewer::ReceiveDataset { args } {
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
itcl::body Rappture::VtkHeightmapViewer::Rebuild {} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $w < 2 || $h < 2 } {
        $_dispatcher event -idle !rebuild
        return
    }

    StartBufferingCommands
    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    if { $_width != $w || $_height != $h || $_reset } {
	set _width $w
	set _height $h
	$_arcball resize $w $h
	DoResize
	if { $_settings(stretchToFit) } {
	    AdjustSetting stretchToFit
	}
    }
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
            lappend info "client" "vtkheightmapviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }
        InitSettings isHeightmap background
	#
	# Reset the camera and other view parameters
	#
	SendCmd "axis color all [Color2RGB $itk_option(-plotforeground)]"
	SendCmd "axis lrot z 90"
	set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
	$_arcball quaternion $q 
	if {$_settings(isHeightmap) } {
	    if { $_view(ortho)} {
		SendCmd "camera mode ortho"
	    } else {
		SendCmd "camera mode persp"
	    }
	    SendCmd "camera reset"
            DoRotate
	} 	    
	PanCamera
        SendCmd "imgflush"
    }

    set _first ""
    # Start off with no datasets are visible.
    SendCmd "dataset visible 0"
    set scale [GetHeightmapScale]
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
                    set f [open /tmp/vtkheightmap.vtk "w"]
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
                    SendCmd [list "clientinfo" $info]
                }
                SendCmd "dataset add $tag data follows $length"
                append _outbuf $bytes
                set _datasets($tag) 1
                SetObjectStyle $dataobj $comp
            }
            lappend _obj2datasets($dataobj) $tag
            if { [info exists _obj2ovride($dataobj-raise)] } {
                # Setting dataset visible enables outline (if enabled)
                # and heightmap
                SendCmd "dataset visible 1 $tag"
            }
	    if { ![info exists _comp2scale($tag)] || 
		 $_comp2scale($tag) != $scale } {
		SendCmd "heightmap heightscale $scale $tag"
		set _comp2scale($tag) $scale
	    }
        }
    }
    if { $_first != ""  } {
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
    InitSettings stretchToFit

    if { $_reset } {
	SendCmd "axis tickpos outside"
	foreach axis { x y z } {
	    SendCmd "axis lformat $axis %g"
	}
        
	foreach axis { x y z } {
	    set label [$_first hints ${axis}label]
	    if { $label == "" } {
		if {$axis == "z"} {
                    if { [string match "component*" $_curFldName] } {
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

	    if {$axis == "z" && [$_first hints ${axis}units] == ""} {
		set units [lindex $_fields($_curFldName) 1]
	    } else {
		set units [$_first hints ${axis}units]
	    }
	    if { $units != "" } {
		# May be a space in the axis units.
		SendCmd [list axis units $axis $units]
	    }
	}
	#
	# Reset the camera and other view parameters
	#
	SendCmd "axis color all [Color2RGB $itk_option(-plotforeground)]"
	SendCmd "dataset color [Color2RGB $itk_option(-plotforeground)]"
	ResetAxes
	set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
	$_arcball quaternion $q 
	if {$_settings(isHeightmap) } {
	    if { $_view(ortho)} {
		SendCmd "camera mode ortho"
	    } else {
		SendCmd "camera mode persp"
	    }
	    SendCmd "camera reset"
            DoRotate
	}
	PanCamera
	InitSettings axisXGrid axisYGrid axisZGrid \
	    axisVisible axisLabels 
        InitSettings opacity heightmapScale lighting edges wireframe \
            colormap field outline isHeightmap
        if { [array size _fields] < 2 } {
            blt::table forget $itk_component(field) $itk_component(field_l)
        }
        RequestLegend
        set _reset 0
    }
    global readyForNextFrame
    set readyForNextFrame 0;		# Don't advance to the next frame

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
itcl::body Rappture::VtkHeightmapViewer::CurrentDatasets {args} {
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
itcl::body Rappture::VtkHeightmapViewer::Zoom {option} {
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
                qw      0.36
                qx      0.25
                qy      0.50
                qz      0.70
                zoom    1.0
                xpan    0
                ypan    0 
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
            if {$_settings(isHeightmap) } {
                DoRotate
            }
        }
    }
}

itcl::body Rappture::VtkHeightmapViewer::PanCamera {} {
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
itcl::body Rappture::VtkHeightmapViewer::Rotate {option x y} {
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

itcl::body Rappture::VtkHeightmapViewer::Pick {x y} {
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
itcl::body Rappture::VtkHeightmapViewer::Pan {option x y} {
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
itcl::body Rappture::VtkHeightmapViewer::InitSettings { args } {
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
itcl::body Rappture::VtkHeightmapViewer::AdjustSetting {what {value ""}} {
    if { $_beforeConnect } {
        return
    }
    switch -- $what {
        "axisFlymode" {
            set mode [$itk_component(axisflymode) value]
            set mode [$itk_component(axisflymode) translate $mode]
            set _settings($what) $mode
            SendCmd "axis flymode $mode"
        }
        "axisLabels" {
            set bool $_settings(axisLabels)
            SendCmd "axis labels all $bool"
        }
        "axisMinorTicks" {
            set bool $_settings(axisMinorTicks)
	    foreach axis { x y z } { 
		SendCmd "axis minticks ${axis} $bool"
	    }
        }
        "axisVisible" {
            set bool $_settings(axisVisible)
            SendCmd "axis visible all $bool"
        }
        "axisXGrid" - "axisYGrid" - "axisZGrid" {
            set axis [string tolower [string range $what 4 4]]
            set bool $_settings($what)
            SendCmd "axis grid $axis $bool"
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
        "colormap" {
            StartBufferingCommands
            set color [$itk_component(colormap) value]
            set _settings(colormap) $color
	    if { $color == "none" } {
		if { $_settings(colormapVisible) } {
		    SendCmd "heightmap surface 0"
		    set _settings(colormapVisible) 0
		}
	    } else {
		if { !$_settings(colormapVisible) } {
		    SendCmd "heightmap surface 1"
		    set _settings(colormapVisible) 1
		}
		ResetColormap $color
		SendCmd "heightmap colormap $_currentColormap"
	    }
            #SendCmd "heightmap colormode scalar $_curFldName"
            #SendCmd "dataset scalar $_curFldName"
            StopBufferingCommands
	    EventuallyRequestLegend
        }
        "colormapVisible" {
            set bool $_settings($what)
            SendCmd "heightmap surface $bool"
        }
        "edges" {
            set bool $_settings(edges)
            SendCmd "heightmap edges $bool"
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
	    set label [$_first hints zlabel]
	    if { $label == "" } {
                if { [string match "component*" $_curFldName] } {
                    set label Z
                } else {
                    set label $_curFldLabel
                }
            } else {
                set label Z
	    }
	    # May be a space in the axis label.
	    SendCmd [list axis name z $label]

	    if { [$_first hints zunits] == "" } {
		set units [lindex $_fields($_curFldName) 1]
	    } else {
		set units [$_first hints zunits]
	    }
	    if { $units != "" } {
		# May be a space in the axis units.
		SendCmd [list axis units $axis $units]
	    }
            # Get the new limits because the field changed.
            ResetAxes
            SendCmd "dataset scalar $_curFldName"
            SendCmd "heightmap colormode scalar $_curFldName"
            SendCmd "camera reset"
            DrawLegend
        }
        "heightmapScale" {
	    if { $_settings(isHeightmap) } {
		set scale [GetHeightmapScale]
		foreach dataset [CurrentDatasets -all] {
		    SendCmd "heightmap heightscale $scale $dataset"
		    set _comp2scale($dataset) $scale
		}
		ResetAxes
	    }
        }
        "isHeightmap" {
	    set bool $_settings(isHeightmap)
            set c $itk_component(view)
            StartBufferingCommands
            # Fix heightmap scale: 0 for contours, 1 for heightmaps.
            if { $bool } {
                set _settings(heightmapScale) 50
                set _settings(opacity) $_settings(saveOpacity)
                set _settings(lighting) $_settings(saveLighting)
            } else {
                set _settings(heightmapScale) 0
                set _settings(lighting) 0
                set _settings(opacity) 100
            }
            AdjustSetting lighting
            AdjustSetting opacity
            set scale [GetHeightmapScale]
            foreach dataset [CurrentDatasets -all] {
                SendCmd "heightmap heightscale $scale $dataset"
                set _comp2scale($dataset) $scale
            }
	    if { $bool } {
		$itk_component(lighting) configure -state normal
		$itk_component(opacity) configure -state normal
		$itk_component(scale) configure -state normal
		$itk_component(opacity_l) configure -state normal
		$itk_component(scale_l) configure -state normal
                if {$_view(ortho)} {
                    SendCmd "camera mode ortho"
                } else {
                    SendCmd "camera mode persp"
                }
	    } else {
		$itk_component(lighting) configure -state disabled
		$itk_component(opacity) configure -state disabled
		$itk_component(scale) configure -state disabled
		$itk_component(opacity_l) configure -state disabled
		$itk_component(scale_l) configure -state disabled
                SendCmd "camera mode image"
	    }
            if {$_settings(stretchToFit)} {
                if {$scale == 0} {
                    SendCmd "camera aspect window"
                } else {
                    SendCmd "camera aspect square"
                }
            }
            ResetAxes
            SendCmd "camera reset"
            if { $bool } {
                set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
                $_arcball quaternion $q
                SendCmd "camera orient $q" 
            } else {
                bind $c <ButtonPress-1> {}
                bind $c <B1-Motion> {}
                bind $c <ButtonRelease-1> {}
            }
            # Fix the mouse bindings for rotation/panning and the 
            # camera mode. Ideally we'd create a bindtag for these.
            if { $bool } {
                # Bindings for rotation via mouse
                bind $c <ButtonPress-1> \
                    [itcl::code $this Rotate click %x %y]
                bind $c <B1-Motion> \
                    [itcl::code $this Rotate drag %x %y]
                bind $c <ButtonRelease-1> \
                    [itcl::code $this Rotate release %x %y]
            }
            StopBufferingCommands
        }
        "isolineColor" {
            set color [$itk_component(isolinecolor) value]
	    if { $color == "none" } {
		if { $_settings(isolinesVisible) } {
		    SendCmd "heightmap isolines 0"
		    set _settings(isolinesVisible) 0
		}
	    } else {
		if { !$_settings(isolinesVisible) } {
		    SendCmd "heightmap isolines 1"
		    set _settings(isolinesVisible) 1
		}
		SendCmd "heightmap isolinecolor [Color2RGB $color]"
	    }
	    DrawLegend
        }
        "isolinesVisible" {
	    set bool $_settings($what)
            SendCmd "heightmap isolines $bool"
	    DrawLegend
        }
        "legendVisible" {
            if { !$_settings($what) } {
		$itk_component(view) delete legend
	    }
	    DrawLegend
        }
        "lighting" {
	    if { $_settings(isHeightmap) } {
                set _settings(saveLighting) $_settings(lighting)
		set bool $_settings($what)
		SendCmd "heightmap lighting $bool"
	    } else {
		SendCmd "heightmap lighting 0"
	    }
        }
        "numIsolines" {
            set _settings(numIsolines) [$itk_component(numisolines) value]
            SendCmd "heightmap numcontours $_settings(numIsolines)"
	    DrawLegend
        }
        "opacity" {
	    if { $_settings(isHeightmap) } {
                set _settings(saveOpacity) $_settings(opacity)
                set val $_settings(opacity)
                set sval [expr { 0.01 * double($val) }]
                SendCmd "heightmap opacity $sval"
            } else {
		SendCmd "heightmap opacity 1"
            }
        }
        "outline" {
	    set bool $_settings($what)
	    SendCmd "dataset outline $bool"
	}
        "stretchToFit" {
	    set bool $_settings($what)
	    if { $bool } {
		set heightScale [GetHeightmapScale]
		if {$heightScale == 0} {
		    SendCmd "camera aspect window"
		} else {
		    SendCmd "camera aspect square"
		}
	    } else {
		SendCmd "camera aspect native"
	    }
	}
        "wireframe" {
            set bool $_settings($what)
            SendCmd "heightmap wireframe $bool"
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
itcl::body Rappture::VtkHeightmapViewer::RequestLegend {} {
    set _legendPending 0
    set font "Arial 8"
    set w 12
    set lineht [font metrics $font -linespace]
    # color ramp height = (canvas height) - (min and max value lines) - 2 
    set h [expr {$_height - 2 * ($lineht + 2)}]
    set _legendHeight $h

    set fname $_curFldName
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
    # If there's a title too, substract one more line
    if { $title != "" } {
        incr h -$lineht 
    }
    if { $h < 1} {
        return
    }
    # Set the legend on the first heightmap dataset.
    if { $_currentColormap != ""  } {
	set cmap $_currentColormap
	SendCmd "legend $cmap scalar $_curFldName {} $w $h 0"
	#SendCmd "heightmap colormode scalar $_curFldName"
	#SendCmd "dataset scalar $_curFldName"
    }
}

#
# ResetAxes --
#
#       Set axis z bounds and range
#
itcl::body Rappture::VtkHeightmapViewer::ResetAxes {} {
    if { ![info exists _limits($_curFldName)]} {
        SendCmd "dataset maprange all"
        SendCmd "axis autorange z on"
        SendCmd "axis autobounds z on"
        return
    }
    foreach { xmin xmax } $_limits(x) break
    foreach { ymin ymax } $_limits(y) break
    foreach { vmin vmax } $_limits($_curFldName) break

    global tcl_precision
    set tcl_precision 17
    set xr [expr $xmax - $xmin]
    set yr [expr $ymax - $ymin]
    set vr [expr $vmax - $vmin]
    set r  [expr ($yr > $xr) ? $yr : $xr]
    if { $vr < 1.0e-17 } {
        set dataScale 1.0
    } else {
        set dataScale [expr $r / $vr]
    }
    set heightScale [GetHeightmapScale]
    set bmin [expr $heightScale * $dataScale * $vmin]
    set bmax [expr $heightScale * $dataScale * $vmax]
    SendCmd "dataset maprange explicit $_limits($_curFldName) $_curFldName"
    SendCmd "axis bounds z $bmin $bmax"
    SendCmd "axis range z $_limits($_curFldName)"
}

#
# SetCurrentColormap --
#
itcl::body Rappture::VtkHeightmapViewer::SetCurrentColormap { stylelist } {
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
itcl::body Rappture::VtkHeightmapViewer::BuildColormap { name stylelist } {
    array set style $stylelist
    set cmap [ColorsToColormap $style(-color)]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    if { ![info exists _settings(opacity)] } {
        set _settings(opacity) $style(-opacity)
    }
    set max $_settings(opacity)

    set wmap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $wmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -mode
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkHeightmapViewer::mode {
    switch -- $itk_option(-mode) {
	"heightmap" {
	    set _settings(isHeightmap) 1
	}
	"contour" {
	    set _settings(isHeightmap) 0
	} 
	default {
	    error "unknown mode settings \"$itk_option(-mode)\""
	}
    }
    AdjustSetting isHeightmap
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkHeightmapViewer::plotbackground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotbackground)]
        SendCmd "screen bgcolor $rgb"
	$itk_component(view) configure -background $itk_option(-plotbackground)
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkHeightmapViewer::plotforeground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotforeground)]
        SendCmd "dataset color $rgb"
	SendCmd "axis color all $rgb"
    }
}

itcl::body Rappture::VtkHeightmapViewer::limits3 { dataobj } {
    lappend limits x [$dataobj limits x]
    lappend limits y [$dataobj limits y] 
    if { [catch { $dataobj limits $_curFldName } vlim] != 0 } {
        set vlim [$dataobj limits v] 
    }
    lappend limits v $vlim
    return $limits
}

itcl::body Rappture::VtkHeightmapViewer::BuildContourTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Contour/Surface Settings" \
        -icon [Rappture::icon contour]]
    $inner configure -borderwidth 4

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings(legendVisible)] \
        -command [itcl::code $this AdjustSetting legendVisible] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings(wireframe)] \
        -command [itcl::code $this AdjustSetting wireframe] \
        -font "Arial 9"

    itk_component add lighting {
	checkbutton $inner.lighting \
	    -text "Enable Lighting" \
	    -variable [itcl::scope _settings(lighting)] \
	    -command [itcl::code $this AdjustSetting lighting] \
	    -font "Arial 9"
    } {
	ignore -font
    }
    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings(edges)] \
        -command [itcl::code $this AdjustSetting edges] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings(outline)] \
        -command [itcl::code $this AdjustSetting outline] \
        -font "Arial 9"

    checkbutton $inner.stretch \
        -text "Stretch to fit" \
        -variable [itcl::scope _settings(stretchToFit)] \
        -command [itcl::code $this AdjustSetting stretchToFit] \
        -font "Arial 9"

    checkbutton $inner.isolines \
        -text "Isolines" \
        -variable [itcl::scope _settings(isolinesVisible)] \
        -command [itcl::code $this AdjustSetting isolinesVisible] \
        -font "Arial 9"

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

    label $inner.colormap_l -text "Colormap" -font "Arial 9" 
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable no
    }
    $inner.colormap choices insert end \
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
        "orange-to-blue"     "orange-to-blue"   \
	"none"		     "none"

    $itk_component(colormap) value "BCGYR"
    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting colormap]

    label $inner.isolinecolor_l -text "Isolines Color" -font "Arial 9" 
    itk_component add isolinecolor {
        Rappture::Combobox $inner.isolinecolor -width 10 -editable no
    }
    $inner.isolinecolor choices insert end \
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

    $itk_component(isolinecolor) value "black"
    bind $inner.isolinecolor <<Value>> \
	[itcl::code $this AdjustSetting isolineColor]

    label $inner.background_l -text "Background Color" -font "Arial 9" 
    itk_component add background {
        Rappture::Combobox $inner.background -width 10 -editable no
    }
    $inner.background choices insert end \
        "black"              "black"            \
        "white"              "white"            \
        "grey"               "grey"             

    $itk_component(background) value "white"
    bind $inner.background <<Value>> [itcl::code $this AdjustSetting background]

    itk_component add opacity_l {
        label $inner.opacity_l -text "Opacity" -font "Arial 9"
    } {
        ignore -font
    }
    itk_component add opacity {
        ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
            -variable [itcl::scope _settings(opacity)] \
            -showvalue off \
            -command [itcl::code $this AdjustSetting opacity]
    }
    itk_component add scale_l {
        label $inner.scale_l -text "Scale" -font "Arial 9"
    } {
        ignore -font
    }
    itk_component add scale {
        ::scale $inner.scale -from 0 -to 100 -orient horizontal \
            -variable [itcl::scope _settings(heightmapScale)] \
            -showvalue off \
            -command [itcl::code $this AdjustSetting heightmapScale]
    }
    label $inner.numisolines_l -text "Number of Isolines" -font "Arial 9"
    itk_component add numisolines {
        Rappture::Spinint $inner.numisolines \
            -min 2 -max 50 -font "arial 9"
    }
    $itk_component(numisolines) value $_settings(numIsolines)
    bind $itk_component(numisolines) <<Value>> \
        [itcl::code $this AdjustSetting numIsolines]

    frame $inner.separator -height 2 -relief sunken -bd 1
    blt::table $inner \
        0,0 $inner.field_l -anchor w -pady 2 \
        0,1 $inner.field -anchor w -pady 2 -fill x \
        1,0 $inner.colormap_l -anchor w -pady 2  \
        1,1 $inner.colormap   -anchor w -pady 2 -fill x  \
        2,0 $inner.isolinecolor_l  -anchor w -pady 2  \
        2,1 $inner.isolinecolor    -anchor w -pady 2 -fill x  \
	3,0 $inner.background_l -anchor w -pady 2 \
	3,1 $inner.background -anchor w -pady 2  -fill x \
        4,0 $inner.numisolines_l -anchor w -pady 2 \
        4,1 $inner.numisolines -anchor w -pady 2 \
        5,0 $inner.stretch    -anchor w -pady 2 -cspan 2 \
        6,0 $inner.edges      -anchor w -pady 2 -cspan 2 \
        7,0 $inner.legend     -anchor w -pady 2 -cspan 2 \
        8,0 $inner.wireframe  -anchor w -pady 2 -cspan 2\
        9,0 $inner.outline    -anchor w -pady 2 -cspan 2 \
        10,0 $inner.isolines   -anchor w -pady 2 -cspan 2 \
        11,0 $inner.separator -padx 2 -fill x -cspan 2 \
        12,0 $inner.lighting   -anchor w -pady 2 -cspan 2 \
        13,0 $inner.opacity_l -anchor w -pady 2 \
        13,1 $inner.opacity   -fill x   -pady 2 \
        14,0 $inner.scale_l   -anchor w -pady 2 -cspan 2 \
        14,1 $inner.scale     -fill x   -pady 2 -cspan 2 \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r15 c1 -resize expand
}

itcl::body Rappture::VtkHeightmapViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon axis1]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Axes" \
        -variable [itcl::scope _settings(axisVisible)] \
        -command [itcl::code $this AdjustSetting axisVisible] \
        -font "Arial 9"
    checkbutton $inner.labels \
        -text "Axis Labels" \
        -variable [itcl::scope _settings(axisLabels)] \
        -command [itcl::code $this AdjustSetting axisLabels] \
        -font "Arial 9"
    label $inner.grid_l -text "Grid" -font "Arial 9" 
    checkbutton $inner.xgrid \
        -text "X" \
        -variable [itcl::scope _settings(axisXGrid)] \
        -command [itcl::code $this AdjustSetting axisXGrid] \
        -font "Arial 9"
    checkbutton $inner.ygrid \
        -text "Y" \
        -variable [itcl::scope _settings(axisYGrid)] \
        -command [itcl::code $this AdjustSetting axisYGrid] \
        -font "Arial 9"
    checkbutton $inner.zgrid \
        -text "Z" \
        -variable [itcl::scope _settings(axisZGrid)] \
        -command [itcl::code $this AdjustSetting axisZGrid] \
        -font "Arial 9"
    checkbutton $inner.minorticks \
        -text "Minor Ticks" \
        -variable [itcl::scope _settings(axisMinorTicks)] \
        -command [itcl::code $this AdjustSetting axisMinorTicks] \
        -font "Arial 9"


    label $inner.mode_l -text "Mode" -font "Arial 9" 

    itk_component add axisflymode {
        Rappture::Combobox $inner.mode -width 10 -editable no
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "furthest" \
        "outer_edges"     "outer"         
    $itk_component(axisflymode) value "static"
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting axisFlymode]

    blt::table $inner \
        0,0 $inner.visible -anchor w -cspan 4 \
        1,0 $inner.labels  -anchor w -cspan 4 \
        2,0 $inner.minorticks  -anchor w -cspan 4 \
	4,0 $inner.grid_l  -anchor w \
        4,1 $inner.xgrid   -anchor w \
        4,2 $inner.ygrid   -anchor w \
        4,3 $inner.zgrid   -anchor w \
        5,0 $inner.mode_l  -anchor w -padx { 2 0 } \
        5,1 $inner.mode    -fill x -cspan 3 

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c6 -resize expand
    blt::table configure $inner r3 -height 0.125i
}


itcl::body Rappture::VtkHeightmapViewer::BuildCameraTab {} {
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

#
#  camera -- 
#
itcl::body Rappture::VtkHeightmapViewer::camera {option args} {
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

itcl::body Rappture::VtkHeightmapViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            #set contents [ConvertToVtkData $dataobj $comp]
            set contents [$dataobj vtkdata $comp]
            append bytes "$contents\n\n"
        }
    }
    return [list .vtk $bytes]
}

itcl::body Rappture::VtkHeightmapViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkHeightmapViewer::BuildDownloadPopup { popup command } {
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

itcl::body Rappture::VtkHeightmapViewer::SetObjectStyle { dataobj comp } {
    # Parse style string.
    set tag $dataobj-$comp
    array set style {
        -color BCGYR
        -edges 0
        -edgecolor black
        -linewidth 1.0
        -levels 10
        -visible 1
    }
    if { $_currentColormap == "" } {
	set stylelist [$dataobj style $comp]
	if { $stylelist != "" } {
	    array set style $stylelist
	    set stylelist [array get style]
	    SetCurrentColormap $stylelist
	}
	$itk_component(colormap) value $style(-color)
    }
    SendCmd "dataset outline $_settings(outline) $tag"
    SendCmd "dataset color [Color2RGB $itk_option(-plotforeground)] $tag"
    set _settings(numIsolines) $style(-levels)
    set scale [GetHeightmapScale]
    SendCmd "heightmap add numcontours $_settings(numIsolines) $scale $tag"
    set _comp2scale($tag) $_settings(heightmapScale)
    SendCmd "heightmap edges $_settings(edges) $tag"
    SendCmd "heightmap wireframe $_settings(wireframe) $tag"
    SendCmd "heightmap colormap $_currentColormap $tag"
    set color [$itk_component(isolinecolor) value]
    SendCmd "heightmap isolinecolor [Color2RGB $color] $tag"
    SendCmd "heightmap lighting $_settings(isHeightmap) $tag"
}

itcl::body Rappture::VtkHeightmapViewer::IsValidObject { dataobj } {
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
itcl::body Rappture::VtkHeightmapViewer::ReceiveLegend { colormap title min max size } {
    #puts stderr "ReceiveLegend colormap=$colormap title=$title range=$min,$max size=$size"
    if { [isconnected] } {
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
itcl::body Rappture::VtkHeightmapViewer::DrawLegend {} {
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
    if { !$_settings(legendVisible) } {
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
	    incr y $lineht
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
    }
    $c delete isoline
    set x2 $x
    set iw [image width $_image(legend)]
    set ih [image height $_image(legend)]
    set x1 [expr $x2 - ($iw*12)/10]
    set color [$itk_component(isolinecolor) value]

    # Draw the isolines on the legend.
    if { $color != "none"  && [info exists _limits($_curFldName)] && 
         $_settings(isolinesVisible) && $_settings(numIsolines) > 0 } {
	set pixels [blt::vector create \#auto]
	set values [blt::vector create \#auto]
	set range [image height $_image(legend)]
	# Order of pixels is max to min (max is at top of legend).
	$pixels seq $ih 0 $_settings(numIsolines)

	set offset [expr 2 + $lineht]
	# If there's a legend title, increase the offset by the line height.
	if { $title != "" } {
	    incr offset $lineht
	}
	$pixels expr {round($pixels + $offset)}
	# Order of values is min to max.
        foreach { vmin vmax } $_limits($_curFldName) break
	$values seq $vmin $vmax $_settings(numIsolines)
	set tags "isoline legend"
        array unset _isolines
	foreach pos [$pixels range 0 end] value [$values range end 0] {
	    set y1 [expr int($pos)]
            for { set off 0 } { $off < 3 } { incr off } {
                set _isolines([expr $y1 + $off]) $value
                set _isolines([expr $y1 - $off]) $value
            }
	    set id [$c create line $x1 $y1 $x2 $y1 -fill $color -tags $tags]
	    #$c bind $id <B1-Enter> [itcl::code $this EnterIsoline %x %y $value]
	    #$c bind $id <Leave> [itcl::code $this LeaveIsoline]
	}
	blt::vector destroy $pixels $values
    }

    $c bind title <ButtonPress> [itcl::code $this Combo post]
    $c bind title <Enter> [itcl::code $this Combo activate]
    $c bind title <Leave> [itcl::code $this Combo deactivate]
    # Reset the item coordinates according the current size of the plot.
    if { [info exists _limits($_curFldName)] } {
        foreach { vmin vmax } $_limits($_curFldName) break
	$c itemconfigure vmin -text [format %g $vmin]
	$c itemconfigure vmax -text [format %g $vmax]
    }
    set y 2
    # If there's a legend title, move the title to the correct position
    if { $title != "" } {
        $c itemconfigure title -text $title
	$c coords title $x $y
	incr y $lineht
    }
    $c coords vmax $x $y
    incr y $lineht
    $c coords colormap $x $y
    set ix [image width $_image(legend)]
    set ih [image height $_image(legend)]
    $c coords sensor [expr $x - $iw] $y $x [expr $y + $ih]
    $c raise sensor
    $c coords vmin $x [expr {$h - 2}]
}

#
# EnterLegend --
#
itcl::body Rappture::VtkHeightmapViewer::EnterLegend { x y } {
    SetLegendTip $x $y
}

#
# MotionLegend --
#
itcl::body Rappture::VtkHeightmapViewer::MotionLegend { x y } {
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
itcl::body Rappture::VtkHeightmapViewer::LeaveLegend { } {
    Rappture::Tooltip::tooltip cancel
    .rappturetooltip configure -icon ""
}

#
# SetLegendTip --
#
itcl::body Rappture::VtkHeightmapViewer::SetLegendTip { x y } {
    set fname $_curFldName
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    
    set ih [image height $_image(legend)]
    # Subtract off the offset of the color ramp from the top of the canvas
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

    # Compute the value of the point
    if { [info exists _limits($fname)] } {
        foreach { vmin vmax } $_limits($fname) break
        set t [expr 1.0 - (double($iy) / double($ih-1))]
        set value [expr $t * ($vmax - $vmin) + $vmin]
    } else {
        set value 0.0
    }
    set tipx [expr $x + 15] 
    set tipy [expr $y - 5]
    .rappturetooltip configure -icon $_image(swatch)
    if { [info exists _isolines($y)] } {
        Rappture::Tooltip::text $c [format "$title %g (isoline)" $_isolines($y)]
    } else {
        Rappture::Tooltip::text $c [format "$title %g" $value]
    }
    Rappture::Tooltip::tooltip show $c +$tipx,+$tipy    
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
itcl::body Rappture::VtkHeightmapViewer::Combo {option} {
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

itcl::body Rappture::VtkHeightmapViewer::GetHeightmapScale {} {
    if {  $_settings(isHeightmap) } {
	set val $_settings(heightmapScale)
	set sval [expr { $val >= 50 ? double($val)/50.0 : 1.0/(2.0-(double($val)/50.0)) }]
	return $sval
    }
    set sval 0 
}

itcl::body Rappture::VtkHeightmapViewer::ResetColormap { color } {
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

