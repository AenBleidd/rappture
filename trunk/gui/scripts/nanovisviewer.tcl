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
    public method parameters {title args} { 
        # do nothing 
    }
    public method scale {args}
    public method updateTransferFunctions {}


    # The following methods are only used by this class.
    private method AdjustSetting {what {value ""}}
    private method BuildCameraTab {}
    private method BuildCutplanesTab {}
    private method BuildViewTab {}
    private method BuildVolumeComponents {}
    private method BuildVolumeTab {}
    private method ComputeAlphamap { cname } 
    private method ComputeTransferFunction { cname }
    private method Connect {}
    private method CurrentDatasets {{what -all}}
    private method Disconnect {}
    private method DoResize {}
    private method DrawLegend { cname }
    private method EventuallyRedrawLegend { } 
    private method EventuallyResize { w h } 
    private method FixLegend {}
    private method GetAlphamap { cname color } 
    private method GetColormap { cname color } 
    private method GetDatasetsWithComponent { cname } 
    private method GetVolumeInfo { w }
    private method HideAllMarkers {} 
    private method AddNewMarker { x y } 
    private method InitComponentSettings { cname } 
    private method InitSettings { args }
    private method NameToAlphamap { name } 
    private method NameTransferFunction { dataobj comp }
    private method Pan {option x y}
    private method PanCamera {}
    private method ParseLevelsOption { cname levels }
    private method ParseMarkersOption { cname markers }
    private method Rebuild {}
    private method ReceiveData { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { tf vmin vmax size }
    private method ResetColormap { cname color }
    private method Rotate {option x y}
    private method SendTransferFunctions {}
    private method SetOrientation { side }
    private method Slice {option args}
    private method SlicerTip {axis}
    private method SwitchComponent { cname } 
    private method Zoom {option}
    private method ToggleVolume { tag name }
    private method RemoveMarker { x y }

    private variable _arcball ""

    private variable _dlist ""     ;# list of data objects
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _serverDatasets   ;# contains all the dataobj-component 
                                   ;# to volumes in the server
    private variable _recvdDatasets;    # list of data objs to send to server
    private variable _dataset2style;    # maps dataobj-component to transfunc
    private variable _style2datasets;   # maps tf back to list of 
                                        # dataobj-components using the tf.

    private variable _reset 1;		# Connection to server has been reset.
    private variable _click;            # Info used for rotate operations.
    private variable _limits;           # Autoscale min/max for all axes
    private variable _view;             # View params for 3D view
    private variable _parsedFunction
    private variable _transferFunctionEditors;# Array of isosurface level values 0..1
    private variable  _settings
    private variable _first "" ;        # This is the topmost volume.
    private variable _current "";       # Currently selected component 
    private variable _volcomponents   ; # Array of components found 
    private variable _componentsList   ; # Array of components found 
    private variable _cname2style
    private variable _cname2transferFunction
    private variable _cname2defaultcolormap
    private variable _cname2defaultalphamap

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
        "[itcl::code $this SendTransferFunctions]; list"

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
        xpan    0
        ypan    0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(v) [list 0.0 1.0]
    set _reset 1

    array set _settings [subst {
        $this-ambient           60
        $this-colormap          default
        $this-cutplaneVisible   0
        $this-diffuse           40
        $this-light2side        1
        $this-opacity           100
        $this-qw                $_view(qw)
        $this-qx                $_view(qx)
        $this-qy                $_view(qy)
        $this-qz                $_view(qz)
        $this-specularExponent  90
        $this-specularLevel     30
        $this-thickness         350
        $this-transp            50
        $this-volume            1
        $this-volumeVisible     1
        $this-xcutplane         1
        $this-xcutposition      0
        $this-xpan              $_view(xpan)
        $this-ycutplane         1
        $this-ycutposition      0
        $this-ypan              $_view(ypan)
        $this-zcutplane         1
        $this-zcutposition      0
        $this-zoom              $_view(zoom)    
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

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings($this-cutplaneVisible)] \
            -command [itcl::code $this AdjustSetting cutplaneVisible] 
    }
    Rappture::Tooltip::for $itk_component(cutplane) \
        "Show/Hide cutplanes"
    pack $itk_component(cutplane) -padx 2 -pady 2

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
        [itcl::code $this EventuallyRedrawLegend]
    bind $itk_component(legend) <KeyPress-Delete> \
        [itcl::code $this RemoveMarker %x %y]
    bind $itk_component(legend) <Enter> \
        [list focus $itk_component(legend)]

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
    foreach name [array names _transferFunctionEditors] {
        itcl::delete object $_transferFunctionEditors($cname)
    }
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
    }
    array set params $settings

    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
    }
    set pos [lsearch -exact $_dlist $dataobj]
    if {$pos < 0} {
        lappend _dlist $dataobj
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
    array set style {
        -color BCGYR
        -levels 6
        -opacity 1.0
        -markers ""
    }
    array unset _limits 
    array unset _volcomponents 
    foreach dataobj $args {
        if { ![$dataobj isvalid] } {
            continue;                     # Object doesn't contain valid data.
        }
        foreach cname [$dataobj components] {
            if { ![info exists _volcomponents($cname)] } {
                lappend _componentsList $cname
                array set style [lindex [$dataobj components -style $cname] 0]
                set cmap [ColorsToColormap $style(-color)]
                set _cname2defaultcolormap($cname) $cmap
                set _settings($cname-colormap) $style(-color)
            }
            lappend _volcomponents($cname) $dataobj-$cname
            array unset limits
            array set limits [$dataobj valueLimits $cname]
            set _limits($cname) $limits(v)
        }
        foreach axis {x y z v} {
            foreach { min max } [$dataobj limits $axis] break
            if {"" != $min && "" != $max} {
                if { ![info exists _limits($axis)] } {
                    set _limits($axis) [list $min $max]
                } else {
                    foreach {amin amax} $_limits($axis) break
                    if {$min < $amin} {
                        set amin $min
                    }
                    if {$max > $amax} {
                        set amax $max
                    }
                    set _limits($axis) [list $amin $amax]
                }
            }
        }
    }
    BuildVolumeComponents
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
            lappend info "hub" [exec hostname]
            lappend info "client" "nanovisviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }

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
# USAGE: SendTransferFunctions
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::SendTransferFunctions {} {
    if 0 {
    if { $_first == "" } {
        puts stderr "first not set"
        return
    }

    foreach tag [CurrentDatasets] {
        if { ![info exists _serverDatasets($tag)] || !$_serverDatasets($tag) } {
            # The volume hasn't reached the server yet.  How did we get 
            # here?
            puts stderr "Don't have $tag in _serverDatasets"
            continue
        }
        if { ![info exists _dataset2style($tag)] } {
            puts stderr "don't have style for volume $tag"
            continue;                        # How does this happen?
        }
        foreach {dataobj cname} [split $tag -] break
        set cname $_dataset2style($tag)

        ComputeTransferFunction $cname
        SendCmd "volume shading transfunc $cname $tag"
    }
    }
    foreach cname [array names _volcomponents] {
        ComputeTransferFunction $cname
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
    } elseif { $info(-type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
}

#
# DrawLegend --
#
itcl::body Rappture::NanovisViewer::DrawLegend { cname } {
    set c $itk_component(legend)
    set w [winfo width $c]
    set h [winfo height $c]
    set lx 10
    set ly [expr {$h - 1}]
    if {"" == [$c find withtag colorbar]} {
        $c create image 10 10 -anchor nw \
            -image $_image(legend) -tags colorbar
        $c create text $lx $ly -anchor sw \
            -fill $itk_option(-plotforeground) -tags "limits text vmin"
        $c create text [expr {$w-$lx}] $ly -anchor se \
            -fill $itk_option(-plotforeground) -tags "limits text vmax"
        $c create text [expr {$w/2}] $ly -anchor s \
            -fill $itk_option(-plotforeground) -tags "limits text title"
        $c lower colorbar
        $c bind colorbar <ButtonRelease-1> [itcl::code $this AddNewMarker %x %y]
    }

    # Display the markers used by the current transfer function.
    HideAllMarkers
    $_transferFunctionEditors($cname) showMarkers $_limits($cname)

    foreach {min max} $_limits($cname) break
    $c itemconfigure vmin -text [format %.2g $min]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %.2g $max]
    $c coords vmax [expr {$w-$lx}] $ly

    set title [$_first hints label]
    set units [$_first hints units]
    if { $units != "" } {
        set title "$title ($units)"
    }
    $c itemconfigure title -text $title
    $c coords title [expr {$w/2}] $ly

    # The colormap may have changed. Resync the slicers with the colormap.
    set datasets [CurrentDatasets -cutplanes]

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
#
# ReceiveLegend --
#
#       The procedure is the response from the render server to each "legend"
#       command.  The server sends back a "legend" command invoked our
#       the slave interpreter.  The purpose is to collect data of the image 
#       representing the legend in the canvas.  In addition, the
#       active transfer function is displayed.
#
#
itcl::body Rappture::NanovisViewer::ReceiveLegend { cname vmin vmax size } {
    if { ![isconnected] } {
        return
    }
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

    DrawLegend $_current
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
    set _limits($tag) [list $info(min)  $info(max)]
    set _limits(v)    [list $info(vmin) $info(vmax)]

    unset _recvdDatasets($tag)
    if { [array size _recvdDatasets] == 0 } {
        updateTransferFunctions
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

    if { $_width != $w || $_height != $h || $_reset } {
        set _width $w
        set _height $h
        $_arcball resize $w $h
        DoResize
    }
    foreach dataobj [get] {
        foreach cname [$dataobj components] {
            set tag $dataobj-$cname
            if { ![info exists _serverDatasets($tag)] } {
                # Send the data as one huge base64-encoded mess -- yuck!
                if { [$dataobj type] == "dx" } {
                    if { ![$dataobj isvalid] } {
                        puts stderr "??? $dataobj is invalid"
                    }
                    set data [$dataobj blob $cname]
                    if 0 {
                        set f [open "/tmp/values-$cname.txt" "w"]
                        puts $f [$dataobj values $cname]
                        close $f
                    }
                } else {
                    set data [$dataobj vtkdata $cname]
                    if 0 {
                        set f [open "/tmp/volume.vtk" "w"]
                        puts $f $data
                        close $f
                    }
                }
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
            NameTransferFunction $dataobj $cname
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
	set _settings($this-xpan)  $_view(xpan)
	set _settings($this-ypan)  $_view(ypan)
	set _settings($this-zoom)  $_view(zoom)

        set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
        $_arcball quaternion $q
        SendCmd "camera orient $q"
        SendCmd "camera reset"
	PanCamera
	SendCmd "camera zoom $_view(zoom)"

        foreach axis {x y z} {
            # Turn off cutplanes for all volumes
            SendCmd "cutplane state 0 $axis"
        }

        InitSettings light2side ambient diffuse specularLevel specularExponent \
            transp isosurface grid axes \
            xcutplane ycutplane zcutplane current

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
    InitSettings outline cutplaneVisible 
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
            updateTransferFunctions
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
            set _settings($this-xpan)  $_view(xpan)
            set _settings($this-ypan)  $_view(ypan)
            set _settings($this-zoom)  $_view(zoom)
        }
    }
}

itcl::body Rappture::NanovisViewer::PanCamera {} {
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
        "current" {
            set cname [$itk_component(volcomponents) value]
            SwitchComponent $cname
        }
        ambient {
            set val $_settings($this-ambient)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading ambient $val $tag"
            }
        }
        diffuse {
            set val $_settings($this-diffuse)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading diffuse $val $tag"
            }
        }
        specularLevel {
            set val $_settings($this-specularLevel)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading specularLevel $val $tag"
            }
        }
        specularExponent {
            set val $_settings($this-specularExponent)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading specularExp $val $tag"
            }
        }
        light2side {
            set _settings($_current-light2side) $_settings($this-light2side)
            set val $_settings($this-light2side)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading light2side $val $tag"
            }
        }
        transp {
            set _settings($_current-transp) $_settings($this-transp)
            set val $_settings($this-transp)
            set sval [expr { 0.01 * double($val) }]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading opacity $sval $tag"
            }
        }
        opacity {
            error "this should not be called"
            set _settings($_current-opacity) $_settings($this-opacity)
            updateTransferFunctions
        }
        thickness {
            set val $_settings($this-thickness)
            set _settings($_current-thickness) $val
            updateTransferFunctions
        }
        "outline" {
            SendCmd "volume outline state $_settings($this-outline)"
        }
        "isosurface" {
            SendCmd "volume shading isosurface $_settings($this-isosurface)"
        }
        "colormap" {
            set color [$itk_component(colormap) value]
            set _settings($this-colormap) $color
            set _settings($_current-colormap) $color
            ResetColormap $_current $color
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
            # This is the global volume visibility control.  It controls the
            # visibility of all the all volumes.  Whenever it's changed, you
            # have to synchronize each of the local controls (see below) with
            # this.
            set datasets [CurrentDatasets] 
            set bool $_settings($this-volume)
            SendCmd "volume data state $bool $datasets"
            foreach cname $_componentsList {
                set _settings($cname-volumeVisible) $bool
            }
            set _settings($this-volumeVisible) $bool
        }
        "volumeVisible" {
            # This is the component specific control.  It changes the 
            # visibility of only the current component.
            set _settings($_current-volumeVisible) \
                $_settings($this-volumeVisible)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume data state $_settings($this-volumeVisible) $tag"
            }
        }
        "cutplaneVisible" {
            set bool $_settings($this-$what)
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane visible $bool $tag"
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
    if {$w > 0 && $h > 0 && $_first != "" } {
        if { [info exists _cname2transferFunction($_current)] } {
            SendCmd "legend $_current $w $h"
        }
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
itcl::body Rappture::NanovisViewer::NameTransferFunction { dataobj cname } {
    array set style {
        -color BCGYR
        -levels 6
        -opacity 1.0
        -markers ""
    }
    set tag $dataobj-$cname
    array set style [lindex [$dataobj components -style $cname] 0]
    if { ![info exists _cname2transferFunction($cname)] } {
        # Get the colormap right now, since it doesn't change with marker
        # changes.
        set cmap [ColorsToColormap $style(-color)]
        set wmap [list 0.0 0.0 1.0 1.0]
        set _cname2transferFunction($cname) [list $cmap $wmap]
        SendCmd [list transfunc define $cname $cmap $wmap]
    }
    SendCmd "volume shading transfunc $cname $tag"
    if { ![info exists _transferFunctionEditors($cname)] } {
        set _transferFunctionEditors($cname) \
            [Rappture::TransferFunctionEditor ::\#auto $itk_component(legend) \
                 $cname \
                 -command [itcl::code $this updateTransferFunctions]]
    }
    set _dataset2style($tag) $cname
    lappend _style2datasets($cname) $tag
    return $cname
}

#
# ComputeTransferFunction --
#
#       Computes and sends the transfer function to the render server.  It's
#       assumed that the volume data limits are known and that the global
#       transfer-functions slider values have been set up.  Both parts are
#       needed to compute the relative value (location) of the marker, and
#       the alpha map of the transfer function.
#
itcl::body Rappture::NanovisViewer::ComputeTransferFunction { cname } {
    foreach {cmap wmap} $_cname2transferFunction($cname) break

    # We have to parse the style attributes for a volume using this
    # transfer-function *once*.  This sets up the initial isomarkers for the
    # transfer function.  The user may add/delete markers, so we have to
    # maintain a list of markers for each transfer-function.  We use the one
    # of the volumes (the first in the list) using the transfer-function as a
    # reference.
    if { ![info exists _parsedFunction($cname)] } {
        array set style {
            -color BCGYR
            -levels 6
            -opacity 1.0
            -markers ""
        }
        # Accumulate the style from all the datasets using it.
        foreach tag [GetDatasetsWithComponent $cname] {
            foreach {dataobj cname} [split [lindex $tag 0] -] break
            array set style [lindex [$dataobj components -style $cname] 0]
        }
        eval $_transferFunctionEditors($cname) limits $_limits($cname)
        # Have to defer creation of isomarkers until we have data limits
        if { [info exists style(-markers)] &&
             [llength $style(-markers)] > 0 } {
            ParseMarkersOption $cname $style(-markers)
        } else {
            ParseLevelsOption $cname $style(-levels)
        }
        
    }
    set wmap [ComputeAlphamap $cname]
    set _cname2transferFunction($cname) [list $cmap $wmap]
    SendCmd [list transfunc define $cname $cmap $wmap]
}

itcl::body Rappture::NanovisViewer::AddNewMarker { x y } {
    if { ![info exists _transferFunctionEditors($_current)] } {
        continue
    }
    # Add a new marker to the current transfer function
    $_transferFunctionEditors($_current) newMarker $x $y normal
}

itcl::body Rappture::NanovisViewer::RemoveMarker { x y } {
    if { ![info exists _transferFunctionEditors($_current)] } {
        continue
    }
    # Add a new marker to the current transfer function
    $_transferFunctionEditors($_current) deleteMarker $x $y 
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
itcl::body Rappture::NanovisViewer::ParseLevelsOption { cname levels } {
    set c $itk_component(legend)
    set list {}
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
        for {set i 1} { $i <= $levels } {incr i} {
            lappend list [expr {double($i)/($levels+1)}]
        }
    } else {
        foreach x $levels {
            lappend list $x
        }
    }
    set _parsedFunction($cname) 1
    $_transferFunctionEditors($cname) addMarkers $list
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
itcl::body Rappture::NanovisViewer::ParseMarkersOption { cname markers } {
    set c $itk_component(legend)
    set list {}
    foreach { min max } $_limits($cname) break
    regsub -all "," $markers " " markers
    foreach marker $markers {
        set n [scan $marker "%g%s" value suffix]
        if { $n == 2 && $suffix == "%" } {
            # $n% : Set relative value (0..1). 
            lappend list [expr {$value * 0.01}]
        } else {
            # $n : absolute value, compute relative
            lappend list  [expr {(double($value)-$min)/($max-$min)]}
        }
    }
    set _parsedFunction($cname) 1
    $_transferFunctionEditors($cname) addMarkers $list
}

# ----------------------------------------------------------------------
# USAGE: UndateTransferFuncs 
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::updateTransferFunctions {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::limits { cname } {
    set _limits(min) 0.0
    set _limits(max) 1.0
    if { ![info exists _style2datasets($cname)] } {
        return [array get _limits]
    }
    set min ""; set max ""
    foreach tag [GetDatasetsWithComponent $cname] {
        if { ![info exists _limits($tag)] } {
            continue
        }
        foreach {amin amax} $_limits($tag) break
        if { $min == "" || $min > $amin } {
            set min $amin
        }
        if { $max == "" || $max < $amax } {
            set max $amax
        }
    }
    if { $min != "" } {
        set _limits(min) $min
    } 
    if { $max != "" } {
        set _limits(max) $max
    }
    return [list $_limits(min) $_limits(max)]
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
    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    set font [option get $itk_component(hull) font Font]
    #set bfont [option get $itk_component(hull) boldFont Font]

    label $inner.lighting_l \
        -text "Lighting / Material Properties" \
        -font "Arial 9 bold" 

    checkbutton $inner.light2side \
        -text "Two-sided lighting" \
        -font $font \
        -variable [itcl::scope _settings($this-light2side)] \
        -command [itcl::code $this AdjustSetting light2side]

    checkbutton $inner.visibility \
        -text "Visible" \
        -font $font \
        -variable [itcl::scope _settings($this-volumeVisible)] \
        -command [itcl::code $this AdjustSetting volumeVisible] \

    label $inner.ambient_l \
        -text "Ambient" \
        -font $font
    ::scale $inner.ambient -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-ambient)] \
        -showvalue off -command [itcl::code $this AdjustSetting ambient] \
        -troughcolor grey92

    label $inner.diffuse_l -text "Diffuse" -font $font
    ::scale $inner.diffuse -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-diffuse)] \
        -showvalue off -command [itcl::code $this AdjustSetting diffuse] \
        -troughcolor grey92

    label $inner.specularLevel_l -text "Specular" -font $font
    ::scale $inner.specularLevel -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-specularLevel)] \
        -showvalue off -command [itcl::code $this AdjustSetting specularLevel] \
        -troughcolor grey92

    label $inner.specularExponent_l -text "Shininess" -font $font
    ::scale $inner.specularExponent -from 10 -to 128 -orient horizontal \
        -variable [itcl::scope _settings($this-specularExponent)] \
        -showvalue off \
        -command [itcl::code $this AdjustSetting specularExponent] \
        -troughcolor grey92

    label $inner.transp_l -text "Opacity" -font $font
    ::scale $inner.transp -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-transp)] \
        -showvalue off -command [itcl::code $this AdjustSetting transp] \
        -troughcolor grey92

    label $inner.transferfunction_l \
        -text "Transfer Function" -font "Arial 9 bold" 

    label $inner.thin -text "Thin" -font $font
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings($this-thickness)] \
        -showvalue off -command [itcl::code $this AdjustSetting thickness] \
        -troughcolor grey92

    label $inner.thick -text "Thick" -font $font

    label $inner.colormap_l -text "Colormap" -font $font
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable no
    }

    $inner.colormap choices insert end \
        "default"            "default"            \
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

    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting colormap]
    $itk_component(colormap) value "default"
    set _settings($this-colormap) "default"

    label $inner.volcomponents_l -text "Component" -font $font
    itk_component add volcomponents {
        Rappture::Combobox $inner.volcomponents -editable no
    }
    $itk_component(volcomponents) value "BCGYR"
    bind $inner.volcomponents <<Value>> \
        [itcl::code $this AdjustSetting current]

    blt::table $inner \
        0,0 $inner.volcomponents_l -anchor e -cspan 2 \
        0,2 $inner.volcomponents             -cspan 3 -fill x \
        1,1 $inner.lighting_l -anchor w -cspan 4 \
        2,1 $inner.ambient_l       -anchor e -pady 2 \
        2,2 $inner.ambient                   -cspan 3 -fill x \
        3,1 $inner.diffuse_l       -anchor e -pady 2 \
        3,2 $inner.diffuse                   -cspan 3 -fill x \
        4,1 $inner.specularLevel_l -anchor e -pady 2 \
        4,2 $inner.specularLevel             -cspan 3 -fill x \
        5,1 $inner.specularExponent_l -anchor e -pady 2 \
        5,2 $inner.specularExponent          -cspan 3 -fill x \
        6,1 $inner.light2side -cspan 3 -anchor w \
        7,1 $inner.visibility -cspan 3 -anchor w \
        8,1 $inner.transferfunction_l -anchor w              -cspan 4 \
        9,1 $inner.transp_l -anchor e -pady 2 \
        9,2 $inner.transp                    -cspan 3 -fill x \
        10,1 $inner.colormap_l       -anchor e \
        10,2 $inner.colormap                  -padx 2 -cspan 3 -fill x \
        11,1 $inner.thin             -anchor e \
        11,2 $inner.thickness                 -cspan 2 -fill x \
        11,4 $inner.thick -anchor w  

    blt::table configure $inner c* r* -resize none
    blt::table configure $inner r* -pady { 2 0 }
    blt::table configure $inner c2 c3 r12 -resize expand
    blt::table configure $inner c0 -width .1i
}

itcl::body Rappture::NanovisViewer::BuildCutplanesTab {} {
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Show Cutplanes" \
        -variable [itcl::scope _settings($this-cutplaneVisible)] \
        -command [itcl::code $this AdjustSetting cutplaneVisible] \
        -font "Arial 9"

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
    $itk_component(xCutButton) select

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
    $itk_component(yCutButton) select

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
    $itk_component(zCutButton) select

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
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this SlicerTip z]"

    blt::table $inner \
        0,1 $inner.visible              -anchor w -pady 2 -cspan 4 \
        1,1 $itk_component(xCutScale) \
        1,2 $itk_component(yCutScale) \
        1,3 $itk_component(zCutScale) \
        2,1 $itk_component(xCutButton) \
        2,2 $itk_component(yCutButton) \
        2,3 $itk_component(zCutButton)

    blt::table configure $inner r0 r1 r2 c* -resize none
    blt::table configure $inner r3 c4 -resize expand
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

itcl::body Rappture::NanovisViewer::EventuallyRedrawLegend {} {
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
            -command [itcl::code $this ToggleVolume $key $name] \
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

itcl::body Rappture::NanovisViewer::ToggleVolume { tag name } {
    set bool $_settings($this-volume-$name)
    SendCmd "volume state $bool $name"
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
    SendCmd "camera reset"
    set _view(xpan) 0
    set _view(ypan) 0
    set _view(zoom) 1.0
    set _settings($this-xpan) $_view(xpan)
    set _settings($this-ypan) $_view(ypan)
    set _settings($this-zoom) $_view(zoom)
}


#
# InitComponentSettings --
#
#       Initializes the volume settings for a specific component. This
#       should match what's used as global settings above. This
#       is called the first time we try to switch to a given component
#       in SwitchComponent below.
#
itcl::body Rappture::NanovisViewer::InitComponentSettings { cname } { 
    array set _settings [subst {
        $cname-ambient           60
        $cname-colormap          default
        $cname-diffuse           40
        $cname-light2side        1
        $cname-opacity           100
        $cname-outline           0
        $cname-specularExponent  90
        $cname-specularLevel     30
        $cname-thickness         350
        $cname-transp            50
        $cname-volumeVisible     1
    }]
}

#
# SwitchComponent --
#
#       This is called when the current component is changed by the
#       dropdown menu in the volume tab.  It synchronizes the global
#       volume settings with the settings of the new current component.
#
itcl::body Rappture::NanovisViewer::SwitchComponent { cname } { 
    if { ![info exists _settings($cname-ambient)] } {
        InitComponentSettings $cname
    }
    # _settings variables change widgets, except for colormap
    set _settings($this-ambient)          $_settings($cname-ambient)
    set _settings($this-colormap)         $_settings($cname-colormap)
    set _settings($this-diffuse)          $_settings($cname-diffuse)
    set _settings($this-light2side)       $_settings($cname-light2side)
    set _settings($this-opacity)          $_settings($cname-opacity)
    set _settings($this-outline)          $_settings($cname-outline)
    set _settings($this-specularExponent) $_settings($cname-specularExponent)
    set _settings($this-specularLevel)    $_settings($cname-specularLevel)
    set _settings($this-thickness)        $_settings($cname-thickness)
    set _settings($this-transp)           $_settings($cname-transp)
    set _settings($this-volumeVisible)    $_settings($cname-volumeVisible)
    $itk_component(colormap) value        $_settings($cname-colormap)
    set _current $cname;                # Reset the current component
}

#
# BuildVolumeComponents --
#
#       This is called from the "scale" method which is called when a
#       new dataset is added or deleted.  It repopulates the dropdown
#       menu of volume component names.  It sets the current component
#       to the first component in the list (of components found).
#       Finally, if there is only one component, don't display the
#       label or the combobox in the volume settings tab.
#
itcl::body Rappture::NanovisViewer::BuildVolumeComponents {} { 
    $itk_component(volcomponents) choices delete 0 end
    foreach name $_componentsList {
        $itk_component(volcomponents) choices insert end $name $name
    }
    set _current [lindex $_componentsList 0]
    $itk_component(volcomponents) value $_current
    set parent [winfo parent $itk_component(volcomponents)]
    if { [llength $_componentsList] <= 1 } {
        # Unpack the components label and dropdown if there's only one
        # component. 
        blt::table forget $parent.volcomponents_l $parent.volcomponents
    } else {
        # Pack the components label and dropdown into the table there's 
        # more than one component to select. 
        blt::table $parent \
            0,0 $parent.volcomponents_l -anchor e -cspan 2 \
            0,2 $parent.volcomponents -cspan 3 -fill x 
    }
}

#
# GetDatasetsWithComponents --
#
#       Returns a list of all the datasets (known by the combination of
#       their data object and component name) that match the given 
#       component name.  For example, this is used where we want to change 
#       the settings of volumes that have the current component.
#
itcl::body Rappture::NanovisViewer::GetDatasetsWithComponent { cname } { 
    if { ![info exists _volcomponents($cname)] } {
        return ""
    }
    set list ""
    foreach tag $_volcomponents($cname) {
        if { ![info exists _serverDatasets($tag)] } {
            continue
        }
        lappend list $tag
    }
    return $list
}

#
# HideAllMarkers --
#
#       Hide all the markers in all the transfer functions.  Can't simply 
#       delete and recreate markers from the <style> since the user may 
#       have create, deleted, or moved markers.
#
itcl::body Rappture::NanovisViewer::HideAllMarkers {} { 
    foreach cname [array names _transferFunctionEditors] {
        $_transferFunctionEditors($cname) hideMarkers 
    }
}

itcl::body Rappture::NanovisViewer::GetColormap { cname color } { 
    if { $color == "default" } {
        return $_cname2defaultcolormap($cname)
    }
    return [ColorsToColormap $color]
}

itcl::body Rappture::NanovisViewer::GetAlphamap { cname name } { 
    if { $name == "default" } {
        return $_cname2defaultalphamap($cname)
    }
    return [NameToAlphamap $name]
}

itcl::body Rappture::NanovisViewer::ResetColormap { cname color } { 
    # Get the current transfer function
    if { ![info exists _cname2transferFunction($cname)] } {
        return
    }
    foreach { cmap wmap } $_cname2transferFunction($cname) break
    set cmap [GetColormap $cname $color]
    set _cname2transferFunction($cname) [list $cmap $wmap]
    SendCmd [list transfunc define $cname $cmap $wmap]
    EventuallyRedrawLegend
}

itcl::body Rappture::NanovisViewer::ComputeAlphamap { cname } { 
    if { ![info exists _transferFunctionEditors($cname)] } {
        return [list 0.0 0.0 1.0 1.0]
    }
    if { ![info exists _settings($cname-ambient)] } {
        InitComponentSettings $cname
    }
    set max 1.0 ;                       #$_settings($tag-opacity)

    set isovalues [$_transferFunctionEditors($cname) values]

    # Ensure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update
    # the values in the _settings varible.
    set opacity [expr { double($_settings($cname-opacity)) * 0.01 }]

    # Scale values between 0.00001 and 0.01000
    set delta [expr {double($_settings($cname-thickness)) * 0.0001}]
    
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
    return $wmap
}


itcl::body Rappture::NanovisViewer::NameToAlphamap { name } {
    switch -- $name {
        "ramp-up" {
            set wmap { 
                0.0 0.0 
                1.0 1.0 
            }
        }
        "ramp-down" {
            set wmap { 
                0.0 1.0 
                1.0 0.0 
            }
        }
        "vee" {
            set wmap { 
                0.0 1.0 
                0.5 0.0 
                1.0 1.0 
            }
        }
        "tent-1" {
            set wmap { 
                0.0 0.0 
                0.5 1.0 
                1.0 0.0 
            }
        }
        "tent-2" {
            set wmap { 
                0.0 0.0 
                0.25 1.0 
                0.5 0.0 
                0.75 1.0 
                1.0 0.0 
            }
        }
        "tent-3" {
            set wmap { 
                0.0 0.0 
                0.16666 1.0
                0.33333 0.0
                0.5     1.0
                0.66666 0.0
                0.83333 1.0
                1.0 0.0 
            }
        }
        "tent-4" {
            set wmap { 
                0.0     0.0 
                0.125   1.0
                0.25    0.0
                0.375   1.0
                0.5     0.0        
                0.625   1.0
                0.75    0.0
                0.875   1.0
                1.0     0.0 
            }
        }
        "sinusoid-1" {
            set wmap {
                0.0                     0.000 0.600 0.800 
                0.14285714285714285     0.400 0.900 1.000 
                0.2857142857142857      0.600 1.000 1.000 
                0.42857142857142855     0.800 1.000 1.000 
                0.5714285714285714      0.900 0.900 0.900 
                0.7142857142857143      0.600 0.600 0.600 
                0.8571428571428571      0.400 0.400 0.400 
                1.0                     0.200 0.200 0.200
            }
        }
        "sinusoid-2" {
            set wmap { 
                0.0                     0.900 1.000 1.000 
                0.1111111111111111      0.800 0.983 1.000 
                0.2222222222222222      0.700 0.950 1.000 
                0.3333333333333333      0.600 0.900 1.000 
                0.4444444444444444      0.500 0.833 1.000 
                0.5555555555555556      0.400 0.750 1.000 
                0.6666666666666666      0.300 0.650 1.000 
                0.7777777777777778      0.200 0.533 1.000 
                0.8888888888888888      0.100 0.400 1.000 
                1.0                     0.000 0.250 1.000
            }
        }
        "sinusoid-6" {
            set wmap {
                0.0                             0.200   0.100   0.000 
                0.09090909090909091             0.400   0.187   0.000 
                0.18181818181818182             0.600   0.379   0.210 
                0.2727272727272727              0.800   0.608   0.480 
                0.36363636363636365             0.850   0.688   0.595 
                0.45454545454545453             0.950   0.855   0.808 
                0.5454545454545454              0.800   0.993   1.000 
                0.6363636363636364              0.600   0.973   1.000 
                0.7272727272727273              0.400   0.940   1.000 
                0.8181818181818182              0.200   0.893   1.000 
                0.9090909090909091              0.000   0.667   0.800 
                1.0                             0.000   0.480   0.600 
            }
        }
        "sinusoid-10" {
            set wmap {
                0.0                             0.000   0.480   0.600 
                0.09090909090909091             0.000   0.667   0.800 
                0.18181818181818182             0.200   0.893   1.000 
                0.2727272727272727              0.400   0.940   1.000 
                0.36363636363636365             0.600   0.973   1.000 
                0.45454545454545453             0.800   0.993   1.000 
                0.5454545454545454              0.950   0.855   0.808 
                0.6363636363636364              0.850   0.688   0.595 
                0.7272727272727273              0.800   0.608   0.480 
                0.8181818181818182              0.600   0.379   0.210 
                0.9090909090909091              0.400   0.187   0.000 
                1.0                             0.200   0.100   0.000 
            }
        }
        "step-2" {
            set wmap {
                0.0                             0.000   0.167   1.000
                0.09090909090909091             0.100   0.400   1.000
                0.18181818181818182             0.200   0.600   1.000
                0.2727272727272727              0.400   0.800   1.000
                0.36363636363636365             0.600   0.933   1.000
                0.45454545454545453             0.800   1.000   1.000
                0.5454545454545454              1.000   1.000   0.800
                0.6363636363636364              1.000   0.933   0.600
                0.7272727272727273              1.000   0.800   0.400
                0.8181818181818182              1.000   0.600   0.200
                0.9090909090909091              1.000   0.400   0.100
                1.0                             1.000   0.167   0.000
            }
        }
        "step-5" {
            set wmap {
                0.0                             1.000   0.167   0.000
                0.09090909090909091             1.000   0.400   0.100
                0.18181818181818182             1.000   0.600   0.200
                0.2727272727272727              1.000   0.800   0.400
                0.36363636363636365             1.000   0.933   0.600
                0.45454545454545453             1.000   1.000   0.800
                0.5454545454545454              0.800   1.000   1.000
                0.6363636363636364              0.600   0.933   1.000
                0.7272727272727273              0.400   0.800   1.000
                0.8181818181818182              0.200   0.600   1.000
                0.9090909090909091              0.100   0.400   1.000
                1.0                             0.000   0.167   1.000
            }
        }
        "step-12" {
            set wmap {
                "#EE82EE"
                "#4B0082" 
                "blue" 
                "#008000" 
                "yellow" 
                "#FFA500" 
                "red" 
            }
        }
        default {
        }
    }
    # Multiply each component by the global opacity value.
    return ""
}


