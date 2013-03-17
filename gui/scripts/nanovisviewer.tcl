# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer - 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
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
package require Img

#
# FIXME:
#       Need to Add DX readers this client to examine the data before 
#       it's sent to the server.  This will eliminate 90% of the insanity in
#       computing the limits of all the volumes.  I can rip out all the 
#       "receive data" "send transfer function" event crap.
#
#       This means we can compute the transfer function (relative values) and
#       draw the legend min/max values without waiting for the information to
#       come from the server.  This will also prevent the flashing that occurs
#       when a new volume is drawn (using the default transfer function) and
#       then when the correct transfer function has been sent and linked to
#       the volume.  
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
    public method parameters {title args} { 
        # do nothing 
    }
    public method rmdupmarker { m x }
    public method scale {args}
    public method updatetransferfuncs {}

    protected method Connect {}
    protected method CurrentDatasets {{what -all}}
    protected method Disconnect {}
    protected method DoResize {}
    protected method FixLegend {}
    protected method AdjustSetting {what {value ""}}
    protected method InitSettings { args }
    protected method Pan {option x y}
    protected method Rebuild {}
    protected method ReceiveData { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { tf vmin vmax size }
    protected method Rotate {option x y}
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
    private method ResetColormap { color }
    private method ComputeTransferFunc { tf }
    private method EventuallyResize { w h } 
    private method EventuallyResizeLegend { } 
    private method NameTransferFunc { dataobj comp }
    private method PanCamera {}
    private method ParseLevelsOption { tf levels }
    private method ParseMarkersOption { tf markers }
    private method volume { tag name }
    private method GetVolumeInfo { w }
    private method SetOrientation { side }

    private variable _arcball ""

    private variable _dlist ""     ;# list of data objects
    private variable _allDataObjs
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _serverDatasets   ;# contains all the dataobj-component 
                                   ;# to volumes in the server
    private variable _serverTfs    ;# contains all the transfer functions 
                                   ;# in the server.
    private variable _recvdDatasets    ;# list of data objs to send to server
    private variable _dataset2style    ;# maps dataobj-component to transfunc
    private variable _style2datasets   ;# maps tf back to list of 
                                    # dataobj-components using the tf.

    private variable _reset 1;		# Connection to server has been reset 
    private variable _click        ;# info used for rotate operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private variable _isomarkers    ;# array of isosurface level values 0..1
    private variable  _settings
    # Array of transfer functions in server.  If 0 the transfer has been
    # defined but not loaded.  If 1 the transfer function has been named
    # and loaded.
    private variable _activeTfs
    private variable _first ""     ;# This is the topmost volume.

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
    set _serverType "nanovis"

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

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]

    # Initialize the view to some default parameters.
    array set _view {
        qw      0.853553
        qx      -0.353553
        qy      0.353553
        qz      0.146447
        zoom    1.0
        xpan   0
        ypan   0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(vmin) 0.0
    set _limits(vmax) 1.0
    set _reset 1

    array set _settings [subst {
        $this-qw                $_view(qw)
        $this-qx                $_view(qx)
        $this-qy                $_view(qy)
        $this-qz                $_view(qz)
        $this-zoom              $_view(zoom)    
        $this-xpan             $_view(xpan)
        $this-ypan             $_view(ypan)
        $this-volume            1
        $this-xcutplane         0
        $this-xcutposition      0
        $this-ycutplane         0
        $this-ycutposition      0
        $this-zcutplane         0
        $this-zcutposition      0
    }]

    itk_component add 3dview {
        label $itk_component(plotarea).view -image $_image(plot) \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }
    bind $itk_component(3dview) <Control-F1> [itcl::code $this ToggleConsole]

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
            -command [itcl::code $this AdjustSetting volume] \
            -variable [itcl::scope _settings($this-volume)]
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    pack $itk_component(volume) -padx 2 -pady 2

    if { [catch {
        BuildViewTab
        BuildVolumeTab
        BuildCutplanesTab
        BuildCameraTab
    } errs] != 0 } {
	global errorInfo
        puts stderr "errs=$errs errorInfo=$errorInfo"
    }

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
    catch { blt::arcball destroy $_arcball }
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
            array unset _dataset2style $dataobj-*
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
            if { [image width $_image(plot)] > 0 && 
                 [image height $_image(plot)] > 0 } {
                set bytes [$_image(plot) data -format "jpeg -quality 100"]
                set bytes [Rappture::encoding::decode -as b64 $bytes]
                return [list .jpg $bytes]
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
itcl::body Rappture::NanovisViewer::Connect {} {
    set _hosts [GetServerList "nanovis"]
    if { "" == $_hosts } {
        return 0
    }
    set _reset 1 
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
    array unset _serverDatasets
}

# ----------------------------------------------------------------------
# USAGE: SendTransferFuncs
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SendTransferFuncs {} {
    if { $_first == "" } {
        puts stderr "first not set"
        return
    }
    # Ensure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update
    # the values in the _settings varible.
    set opacity [expr { double($_settings($this-opacity)) * 0.01 }]
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($_settings($this-thickness)) * 0.0001}]

    foreach tag [CurrentDatasets] {
        if { ![info exists _serverDatasets($tag)] || !$_serverDatasets($tag) } {
            # The volume hasn't reached the server yet.  How did we get 
            # here?
            puts stderr "Don't have $tag in _serverDatasets"
            continue
        }
        if { ![info exists _dataset2style($tag)] } {
            puts stderr "Don't have style for volume $tag"
            continue;                        # How does this happen?
        }
        set tf $_dataset2style($tag)
        set _settings($this-$tf-opacity) $opacity
        set _settings($this-$tf-thickness) $thickness
        ComputeTransferFunc $tf
        # FIXME: Need to the send information as to what transfer functions
        #        to update so that we only update the transfer function 
        #        as necessary.  Right now, all transfer functions are 
        #        updated. This makes moving the isomarker slider chunky.
        if { ![info exists _activeTfs($tf)] || !$_activeTfs($tf) } {
            set _activeTfs($tf) 1
        }
        SendCmd "volume shading transfunc $tf $tag"
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

    # The colormap may have changed. Resync the slicers with the colormap.
    set datasets [CurrentDatasets -cutplanes]
    SendCmd "volume data state $_settings($this-volume) $datasets"

    # Adjust the cutplane for only the first component in the topmost volume
    # (i.e. the first volume designated in the field).
    set tag [lindex $datasets 0]
    foreach axis {x y z} {
        # Turn off cutplanes for all volumes
        SendCmd "cutplane state 0 $axis"
        if { $_settings($this-${axis}cutplane) } {
            # Turn on cutplane for this particular volume and set the position
            SendCmd "cutplane state 1 $axis $tag"
            set pos [expr {0.01*$_settings($this-${axis}cutposition)}]
            SendCmd "cutplane position $pos $axis $tag"
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
#       the data we've sent (tracked by _recvdDatasets) we can then determine
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
    set _serverDatasets($tag) 1
    if { $_settings($this-volume) && $dataobj == $_first } {
        SendCmd "volume state 1 $tag"
    }
    set _limits($tag-min) $info(min);  # Minimum value of the volume.
    set _limits($tag-max) $info(max);  # Maximum value of the volume.
    set _limits(vmin)      $info(vmin); # Overall minimum value.
    set _limits(vmax)      $info(vmax); # Overall maximum value.

    unset _recvdDatasets($tag)
    if { [array size _recvdDatasets] == 0 } {
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
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $w < 2 || $h < 2 } {
        $_dispatcher event -idle !rebuild
        return
    }

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    StartBufferingCommands

    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names _isomarkers] {
        foreach m $_isomarkers($tf) {
            $m visible no
        }
    }

    if { $_width != $w || $_height != $h || $_reset } {
        set _width $w
        set _height $h
        $_arcball resize $w $h
        DoResize
    }
    if { $_reset } {
        if { $_reportClientInfo }  {
            # Tell the server the name of the tool, the version, and
            # dataset that we are rendering.  Have to do it here because
            # we don't know what data objects are using the renderer until
            # be get here.
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
            lappend info "client" "nanovisviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }
    }
    foreach dataobj [get] {
        foreach cname [$dataobj components] {
            set tag $dataobj-$cname
            if { ![info exists _serverDatasets($tag)] } {
                # Send the data as one huge base64-encoded mess -- yuck!
                set data [$dataobj values $cname]
                set nbytes [string length $data]
                if { $_reportClientInfo }  {
                    set info {}
                    lappend info "tool_id"       [$dataobj hints toolId]
                    lappend info "tool_name"     [$dataobj hints toolName]
                    lappend info "tool_version"  [$dataobj hints toolRevision]
                    lappend info "tool_title"    [$dataobj hints toolTitle]
                    lappend info "dataset_label" [$dataobj hints label]
                    lappend info "dataset_size"  $nbytes
                    lappend info "dataset_tag"   $tag
                    SendCmd "clientinfo [list $info]"
                }
                SendCmd "volume data follows $nbytes $tag"
                append _outbuf $data
                set _recvdDatasets($tag) 1
                set _serverDatasets($tag) 0
            }
            NameTransferFunc $dataobj $cname
        }
    }
    set _first [lindex [get] 0]
    if { $_reset } {
	#
	# Reset the camera and other view parameters
	#
        set _settings($this-qw)    $_view(qw)
        set _settings($this-qx)    $_view(qx)
        set _settings($this-qy)    $_view(qy)
        set _settings($this-qz)    $_view(qz)
	set _settings($this-xpan) $_view(xpan)
	set _settings($this-ypan) $_view(ypan)
	set _settings($this-zoom)  $_view(zoom)

        set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
        $_arcball quaternion $q
        SendCmd "camera orient $q"
        SendCmd "camera reset"
	PanCamera
	SendCmd "camera zoom $_view(zoom)"
        InitSettings light2side light transp isosurface grid axes 
	
        foreach axis {x y z} {
            # Turn off cutplanes for all volumes
            SendCmd "cutplane state 0 $axis"
        }
	if {"" != $_first} {
	    set axis [$_first hints updir]
	    if { "" != $axis } {
		SendCmd "up $axis"
	    }
	    set location [$_first hints camera]
	    if { $location != "" } {
		array set _view $location
	    }
	}
    }
    # Outline seems to need to be reset every update.
    InitSettings outline
    # nothing to send -- activate the proper ivol
    SendCmd "volume state 0"
    if {"" != $_first} {
        set datasets [array names _serverDatasets $_first-*] 
        if { $datasets != "" } {
            SendCmd "volume state 1 $datasets"
        }
        # If the first volume already exists on the server, then make sure
        # we display the proper transfer function in the legend.
        set cname [lindex [$_first components] 0]
        if { [info exists _serverDatasets($_first-$cname)] } {
            updatetransferfuncs
        }
    }
    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    StopBufferingCommands
    blt::busy release $itk_component(hull)
    set _reset 0
}

# ----------------------------------------------------------------------
# USAGE: CurrentDatasets ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::CurrentDatasets {{what -all}} {
    set rlist ""
    if { $_first == "" } {
        return
    }
    foreach cname [$_first components] {
        set tag $_first-$cname
        if { [info exists _serverDatasets($tag)] && $_serverDatasets($tag) } {
            array set style {
                -cutplanes 1
            }
            array set style [lindex [$_first components -style $cname] 0]
            if { $what != "-cutplanes" || $style(-cutplanes) } {
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
itcl::body Rappture::NanovisViewer::Zoom {option} {
    switch -- $option {
        "in" {
            set _view(zoom) [expr {$_view(zoom)*1.25}]
            set _settings($this-zoom) $_view(zoom)
            SendCmd "camera zoom $_view(zoom)"
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
            set _settings($this-zoom) $_view(zoom)
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
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
            $_arcball quaternion $q
            SendCmd "camera orient $q"
            SendCmd "camera reset"
            set _settings($this-qw)    $_view(qw)
            set _settings($this-qx)    $_view(qx)
            set _settings($this-qy)    $_view(qy)
            set _settings($this-qz)    $_view(qz)
            set _settings($this-xpan) $_view(xpan)
            set _settings($this-ypan) $_view(ypan)
            set _settings($this-zoom)  $_view(zoom)
            $itk_component(orientation) value "default" 
        }
    }
}

itcl::body Rappture::NanovisViewer::PanCamera {} {
    #set x [expr ($_view(xpan)) / $_limits(xrange)]
    #set y [expr ($_view(ypan)) / $_limits(yrange)]
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
itcl::body Rappture::NanovisViewer::Rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
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

                set q [$_arcball rotate $x $y $_click(x) $_click(y)]
                foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
                set _settings($this-qw) $_view(qw)
                set _settings($this-qx) $_view(qx)
                set _settings($this-qy) $_view(qy)
                set _settings($this-qz) $_view(qz)
                SendCmd "camera orient $q"

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
        set _view(xpan) [expr $_view(xpan) + $x]
        set _view(ypan) [expr $_view(ypan) + $y]
        PanCamera
        set _settings($this-xpan) $_view(xpan)
        set _settings($this-ypan) $_view(ypan)
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
        set _view(xpan) [expr $_view(xpan) - $dx]
        set _view(ypan) [expr $_view(ypan) - $dy]
        PanCamera
        set _settings($this-xpan) $_view(xpan)
        set _settings($this-ypan) $_view(ypan)
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: InitSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::InitSettings { args } {
    foreach arg $args {
        AdjustSetting $arg
    }
}

# ----------------------------------------------------------------------
# USAGE: AdjustSetting <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::AdjustSetting {what {value ""}} {
    if {![isconnected]} {
        return
    }
    switch -- $what {
        light {
            set val $_settings($this-light)
            set diffuse [expr {0.01*$val}]
            set ambient [expr {1.0-$diffuse}]
            set specularLevel 0.3
            set specularExp 90.0
            SendCmd "volume shading ambient $ambient"
            SendCmd "volume shading diffuse $diffuse"
            SendCmd "volume shading specularLevel $specularLevel"
            SendCmd "volume shading specularExp $specularExp"
        }
        light2side {
            set val $_settings($this-light2side)
            SendCmd "volume shading light2side $val"
        }
        transp {
            set val $_settings($this-transp)
            set sval [expr { 0.01 * double($val) }]
            SendCmd "volume shading opacity $sval"
        }
        opacity {
            set val $_settings($this-opacity)
            set sval [expr { 0.01 * double($val) }]
            foreach tf [array names _activeTfs] {
                set _settings($this-$tf-opacity) $sval
                set _activeTfs($tf) 0
            }
            updatetransferfuncs
        }
        thickness {
            if { [array names _activeTfs] > 0 } {
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
            SendCmd "volume outline state $_settings($this-outline)"
        }
        "isosurface" {
            SendCmd "volume shading isosurface $_settings($this-isosurface)"
        }
        "colormap" {
            set color [$itk_component(colormap) value]
            set _settings(colormap) $color
            # Only set the colormap on the first volume. Ignore the others.
            #ResetColormap $color
        }
        "grid" {
            SendCmd "grid visible $_settings($this-grid)"
        }
        "axes" {
            SendCmd "axis visible $_settings($this-axes)"
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
            set datasets [CurrentDatasets -cutplanes] 
            SendCmd "volume data state $_settings($this-volume) $datasets"
        }
        "xcutplane" - "ycutplane" - "zcutplane" {
            set axis [string range $what 0 0]
            set bool $_settings($this-$what)
            set datasets [CurrentDatasets -cutplanes] 
            set tag [lindex $datasets 0]
            SendCmd "cutplane state $bool $axis $tag"
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
        set tag [lindex [CurrentDatasets] 0]
        if { [info exists _dataset2style($tag)] } {
            SendCmd "legend $_dataset2style($tag) $w $h"
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
itcl::body Rappture::NanovisViewer::NameTransferFunc { dataobj cname } {
    array set style {
        -color BCGYR
        -levels 6
        -opacity 1.0
        -markers ""
    }
    set tag $dataobj-$cname
    array set style [lindex [$dataobj components -style $cname] 0]
    set tf "$style(-color):$style(-levels):$style(-opacity)"
    set _dataset2style($tag) $tf
    lappend _style2datasets($tf) $tag
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
        -color BCGYR
        -levels 6
        -opacity 1.0
        -markers ""
    }
    foreach {dataobj cname} [split [lindex $_style2datasets($tf) 0] -] break
    array set style [lindex [$dataobj components -style $cname] 0]

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
        if { [info exists style(-markers)] &&
             [llength $style(-markers)] > 0 } {
            ParseMarkersOption $tf $style(-markers)
        } else {
            ParseLevelsOption $tf $style(-levels)
        }
    }
    set cmap [ColorsToColormap $style(-color)]
    set tag $this-$tf
    if { ![info exists _settings($tag-opacity)] } {
        set _settings($tag-opacity) $style(-opacity)
    }
    set max 1.0 ;#$_settings($tag-opacity)

    set isovalues {}
    foreach m $_isomarkers($tf) {
        lappend isovalues [$m relval]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists _settings($tag-thickness)]} {
        set _settings($tag-thickness) 0.005
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
    set tag [lindex [CurrentDatasets] 0]
    set tf $_dataset2style($tag)
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
    if { ![info exists _style2datasets($tf)] } {
        return [array get _limits]
    }
    set min ""; set max ""
    foreach tag $_style2datasets($tf) {
        if { ![info exists _serverDatasets($tag)] } {
            continue
        }
        if { ![info exists _limits($tag-min)] } {
            continue
        }
        if { $min == "" || $min > $_limits($tag-min) } {
            set min $_limits($tag-min)
        }
        if { $max == "" || $max < $_limits($tag-max) } {
            set max $_limits($tag-max)
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
        grid            0
        axes            1
        outline         0
        volume          1
        legend          1
        particles       1
        lic             1
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
        -command [itcl::code $this AdjustSetting isosurface] \
        -font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this AdjustSetting axes] \
        -font "Arial 9"

    checkbutton $inner.grid \
        -text "Grid" \
        -variable [itcl::scope _settings($this-grid)] \
        -command [itcl::code $this AdjustSetting grid] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings($this-outline)] \
        -command [itcl::code $this AdjustSetting outline] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings($this-legend)] \
        -command [itcl::code $this AdjustSetting legend] \
        -font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this AdjustSetting volume] \
        -font "Arial 9"

    blt::table $inner \
        0,0 $inner.axes  -cspan 2 -anchor w \
        1,0 $inner.grid  -cspan 2 -anchor w \
        2,0 $inner.outline  -cspan 2 -anchor w \
        3,0 $inner.volume  -cspan 2 -anchor w \
        4,0 $inner.legend  -cspan 2 -anchor w 

    if 0 {
    bind $inner <Map> [itcl::code $this GetVolumeInfo $inner]
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r5 -resize expand
}

itcl::body Rappture::NanovisViewer::BuildVolumeTab {} {
    foreach { key value } {
        light2side      0
        light           40
        transp          50
        opacity         100
        thickness       350
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
        -command [itcl::code $this AdjustSetting volume]
    label $inner.shading -text "Shading:" -font $fg

    checkbutton $inner.light2side -text "Two-sided lighting" -font $fg \
        -variable [itcl::scope _settings($this-light2side)] \
        -command [itcl::code $this AdjustSetting light2side]

    label $inner.dim -text "Glow" -font $fg
    ::scale $inner.light -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-light)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting light]
    label $inner.bright -text "Surface" -font $fg

    label $inner.fog -text "Clear" -font $fg
    ::scale $inner.transp -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-transp)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting transp]
    label $inner.plastic -text "Opaque" -font $fg

    label $inner.clear -text "Clear" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-opacity)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting opacity]
    label $inner.opaque -text "Opaque" -font $fg

    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings($this-thickness)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting thickness]
    label $inner.thick -text "Thick" -font $fg

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

    blt::table $inner \
        0,0 $inner.vol -cspan 4 -anchor w -pady 2 \
        1,0 $inner.shading -cspan 4 -anchor w -pady {10 2} \
        2,0 $inner.light2side -cspan 4 -anchor w -pady 2 \
        3,0 $inner.dim -anchor e -pady 2 \
        3,1 $inner.light -cspan 2 -pady 2 -fill x \
        3,3 $inner.bright -anchor w -pady 2 \
        4,0 $inner.fog -anchor e -pady 2 \
        4,1 $inner.transp -cspan 2 -pady 2 -fill x \
        4,3 $inner.plastic -anchor w -pady 2 \
        5,0 $inner.thin -anchor e -pady 2 \
        5,1 $inner.thickness -cspan 2 -pady 2 -fill x\
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
            -command [itcl::code $this AdjustSetting xcutplane] \
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
            -command [itcl::code $this AdjustSetting ycutplane] \
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
            -command [itcl::code $this AdjustSetting zcutplane] \
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
        0,3 $itk_component(zCutScale) 

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

    set row 1
    set labels { qw qx qy qz xpan ypan zoom }
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _settings($this-$tag)]
        bind $inner.${tag} <Return> \
            [itcl::code $this camera set ${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this camera set ${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }

    blt::table configure $inner c* r* -resize none
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
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane position $newpos $axis $tag"
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
    $_arcball resize $_width $_height
    SendCmd "screen size $_width $_height"
    set _resizePending 0
}

itcl::body Rappture::NanovisViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
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
                "xpan" - "ypan" {
                    set _view($who) $_settings($this-$who)
                    PanCamera
                }
                "qx" - "qy" - "qz" - "qw" {
                    set _view($who) $_settings($this-$who)
                    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
                    $_arcball quaternion $q
                    SendCmd "camera orient $q"
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
        5,0 $inner -fill both -cspan 2 -anchor nw
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

itcl::body Rappture::NanovisViewer::SetOrientation { side } { 
    array set positions {
        front "1 0 0 0"
        back  "0 0 1 0"
        left  "0.707107 0 -0.707107 0"
        right "0.707107 0 0.707107 0"
        top   "0.707107 -0.707107 0 0"
        bottom "0.707107 0.707107 0 0"
    }
    foreach name { qw qx qy qz } value $positions($side) {
        set _view($name) $value
    } 
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q
    SendCmd "camera orient $q" 
}

