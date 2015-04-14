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
    public method parameters {title args} {
        # do nothing
    }
    public method scale {args}
    public method updateTransferFunctions {}

    # The following methods are only used by this class.

    private method AddNewMarker { x y }
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
    private method GetColormap { cname color }
    private method GetDatasetsWithComponent { cname }
    private method GetVolumeInfo { w }
    private method HideAllMarkers {}
    private method InitComponentSettings { cname }
    private method InitSettings { args }
    private method NameTransferFunction { dataobj comp }
    private method Pan {option x y}
    private method PanCamera {}
    private method ParseLevelsOption { cname levels }
    private method ParseMarkersOption { cname markers }
    private method QuaternionToView { q } {
        foreach { _view(-qw) _view(-qx) _view(-qy) _view(-qz) } $q break
    }
    private method Rebuild {}
    private method ReceiveData { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { tf vmin vmax size }
    private method RemoveMarker { x y }
    private method ResetColormap { cname color }
    private method Rotate {option x y}
    private method SendTransferFunctions {}
    private method SetOrientation { side }
    private method Slice {option args}
    private method SlicerTip {axis}
    private method SwitchComponent { cname }
    private method ToggleVolume { tag name }
    private method ViewToQuaternion {} {
        return [list $_view(-qw) $_view(-qx) $_view(-qy) $_view(-qz)]
    }
    private method Zoom {option}

    private variable _arcball ""

    private variable _dlist ""         ;# list of data objects
    private variable _obj2ovride       ;# maps dataobj => style override
    private variable _serverDatasets   ;# contains all the dataobj-component
                                       ;# to volumes in the server
    private variable _recvdDatasets;    # list of data objs to send to server

    private variable _reset 1;          # Connection to server has been reset.
    private variable _click;            # Info used for rotate operations.
    private variable _limits;           # Autoscale min/max for all axes
    private variable _view;             # View params for 3D view
    private variable _parsedFunction
    private variable _transferFunctionEditors
    private variable _settings
    private variable _first "" ;        # This is the topmost volume.
    private variable _current "";       # Currently selected component
    private variable _volcomponents    ;# Array of components found
    private variable _componentsList   ;# Array of components found
    private variable _cname2transferFunction
    private variable _cname2defaultcolormap

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
        -qw      0.853553
        -qx      -0.353553
        -qy      0.353553
        -qz      0.146447
        -xpan    0
        -ypan    0
        -zoom    1.0
    }
    set _arcball [blt::arcball create 100 100]
    $_arcball quaternion [ViewToQuaternion]

    set _limits(v) [list 0.0 1.0]
    set _reset 1

    array set _settings {
        -ambient                60
        -axesvisible            1
        -background             black
        -colormap               "default"
        -cutplanesvisible       0
        -diffuse                40
        -gridvisible            0
        -isosurfaceshading      0
        -legendvisible          1
        -light2side             1
        -opacity                50
        -outlinevisible         0
        -qw                     0.853553
        -qx                     -0.353553
        -qy                     0.353553
        -qz                     0.146447
        -specularexponent       90
        -specularlevel          30
        -thickness              350
        -volume                 1
        -volumevisible          1
        -xcutplaneposition      50
        -xcutplanevisible       1
        -xpan                   0
        -ycutplaneposition      50
        -ycutplanevisible       1
        -ypan                   0
        -zcutplaneposition      50
        -zcutplanevisible       1
        -zoom                   1.0
    }

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

    itk_component add volume {
        Rappture::PushButton $f.volume \
            -onimage [Rappture::icon volume-on] \
            -offimage [Rappture::icon volume-off] \
            -command [itcl::code $this AdjustSetting -volume] \
            -variable [itcl::scope _settings(-volume)]
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    pack $itk_component(volume) -padx 2 -pady 2

    itk_component add cutplane {
        Rappture::PushButton $f.cutplane \
            -onimage [Rappture::icon cutbutton] \
            -offimage [Rappture::icon cutbutton] \
            -variable [itcl::scope _settings(-cutplanesvisible)] \
            -command [itcl::code $this AdjustSetting -cutplanesvisible]
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

    EnableWaitDialog 900
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
    foreach cname [array names _transferFunctionEditors] {
        itcl::delete object $_transferFunctionEditors($cname)
    }
    catch { blt::arcball destroy $_arcball }
    array unset _settings
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
                if {[info exists _obj2ovride($obj-raise)] &&
                    $_obj2ovride($obj-raise)} {
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
        -color    BCGYR
        -levels   6
        -markers  ""
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
                    continue
                }
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
            lappend info "version" "$Rappture::version"
            lappend info "build" "$Rappture::build"
            lappend info "svnurl" "$Rappture::svnurl"
            lappend info "installdir" "$Rappture::installdir"
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
    $c itemconfigure vmin -text [format %g $min]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %g $max]
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
        if { $_settings(-${axis}cutplanevisible) } {
            # Turn on cutplane for this particular volume and set the position
            SendCmd "cutplane state 1 $axis $tag"
            set pos [expr {0.01*$_settings(-${axis}cutplaneposition)}]
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
    if { $_settings(-volumevisible) && $dataobj == $_first } {
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
        update
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
                } else {
                    set data [$dataobj vtkdata $cname]
                    if 0 {
                        set f [open "/tmp/volume.vtk" "w"]
                        fconfigure $f -translation binary -encoding binary
                        puts -nonewline $f $data
                        close $f
                    }
                }
                set nbytes [string length $data]
                if { $_reportClientInfo }  {
                    set info {}
                    lappend info "tool_id"       [$dataobj hints toolid]
                    lappend info "tool_name"     [$dataobj hints toolname]
                    lappend info "tool_title"    [$dataobj hints tooltitle]
                    lappend info "tool_command"  [$dataobj hints toolcommand]
                    lappend info "tool_revision" [$dataobj hints toolrevision]
                    lappend info "dataset_label" [$dataobj hints label]
                    lappend info "dataset_size"  $nbytes
                    lappend info "dataset_tag"   $tag
                    SendCmd "clientinfo [list $info]"
                }
                SendCmd "volume data follows $nbytes $tag"
                SendData $data
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
        set _settings(-qw)    $_view(-qw)
        set _settings(-qx)    $_view(-qx)
        set _settings(-qy)    $_view(-qy)
        set _settings(-qz)    $_view(-qz)
        set _settings(-xpan)  $_view(-xpan)
        set _settings(-ypan)  $_view(-ypan)
        set _settings(-zoom)  $_view(-zoom)

        set q [ViewToQuaternion]
        $_arcball quaternion $q
        SendCmd "camera orient $q"
        SendCmd "camera reset"
        PanCamera
        SendCmd "camera zoom $_view(-zoom)"

        # Turn off cutplanes for all volumes
        foreach axis {x y z} {
            SendCmd "cutplane state 0 $axis"
        }

        InitSettings -light2side -ambient -diffuse -specularlevel \
            -specularexponent -opacity -isosurfaceshading -gridvisible \
            -axesvisible -xcutplanevisible -ycutplanevisible -zcutplanevisible \
            -current

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
    InitSettings -outlinevisible -cutplanesvisible
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
            set _view(-zoom) [expr {$_view(-zoom)*1.25}]
            set _settings(-zoom) $_view(-zoom)
            SendCmd "camera zoom $_view(-zoom)"
        }
        "out" {
            set _view(-zoom) [expr {$_view(-zoom)*0.8}]
            set _settings(-zoom) $_view(-zoom)
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
            set q [ViewToQuaternion]
            $_arcball quaternion $q
            SendCmd "camera orient $q"
            SendCmd "camera reset"
            set _settings(-qw)    $_view(-qw)
            set _settings(-qx)    $_view(-qx)
            set _settings(-qy)    $_view(-qy)
            set _settings(-qz)    $_view(-qz)
            set _settings(-xpan)  $_view(-xpan)
            set _settings(-ypan)  $_view(-ypan)
            set _settings(-zoom)  $_view(-zoom)
        }
    }
}

itcl::body Rappture::NanovisViewer::PanCamera {} {
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
                QuaternionToView $q
                set _settings(-qw) $_view(-qw)
                set _settings(-qx) $_view(-qx)
                set _settings(-qy) $_view(-qy)
                set _settings(-qz) $_view(-qz)
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
        set _view(-xpan) [expr $_view(-xpan) + $x]
        set _view(-ypan) [expr $_view(-ypan) + $y]
        PanCamera
        set _settings(-xpan) $_view(-xpan)
        set _settings(-ypan) $_view(-ypan)
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
        set _view(-xpan) [expr $_view(-xpan) - $dx]
        set _view(-ypan) [expr $_view(-ypan) - $dy]
        PanCamera
        set _settings(-xpan) $_view(-xpan)
        set _settings(-ypan) $_view(-ypan)
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
        "-ambient" {
            # Other parts of the code use the ambient setting to
            # tell if the component settings have been initialized
            if { ![info exists _settings($_current${what})] } {
                InitComponentSettings $_current
            }
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading ambient $val $tag"
            }
        }
        "-axesvisible" {
            SendCmd "axis visible $_settings($what)"
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
            DrawLegend $_current
        }
        "-colormap" {
            set color [$itk_component(colormap) value]
            set _settings($what) $color
            set _settings($_current${what}) $color
            ResetColormap $_current $color
        }
        "-current" {
            set cname [$itk_component(volcomponents) value]
            SwitchComponent $cname
        }
        "-cutplanesvisible" {
            set bool $_settings($what)
            # We only set cutplanes on the first dataset.
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane visible $bool $tag"
        }
        "-diffuse" {
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading diffuse $val $tag"
            }
        }
        "-gridvisible" {
            SendCmd "grid visible $_settings($what)"
        }
        "-isosurfaceshading" {
            SendCmd "volume shading isosurface $_settings($what)"
        }
        "-legendvisible" {
            if { $_settings($what) } {
                blt::table $itk_component(plotarea) \
                    0,0 $itk_component(3dview) -fill both \
                    1,0 $itk_component(legend) -fill x
                blt::table configure $itk_component(plotarea) r1 -resize none
            } else {
                blt::table forget $itk_component(legend)
            }
        }
        "-light2side" {
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading light2side $val $tag"
            }
        }
        "-opacity" {
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading opacity $sval $tag"
            }
        }
        "-outlinevisible" {
            SendCmd "volume outline state $_settings($what)"
        }
        "-outlinecolor" {
            set rgb [Color2RGB $_settings($what)]
            SendCmd "volume outline color $rgb"
        }
        "-specularlevel" {
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            set val [expr {0.01*$val}]
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading specularLevel $val $tag"
            }
        }
        "-specularexponent" {
            set _settings($_current${what}) $_settings($what)
            set val $_settings($what)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume shading specularExp $val $tag"
            }
        }
        "-thickness" {
            set val $_settings($what)
            set _settings($_current${what}) $val
            updateTransferFunctions
        }
        "-volume" {
            # This is the global volume visibility control.  It controls the
            # visibility of all the all volumes.  Whenever it's changed, you
            # have to synchronize each of the local controls (see below) with
            # this.
            set datasets [CurrentDatasets]
            set bool $_settings($what)
            SendCmd "volume data state $bool $datasets"
            foreach cname $_componentsList {
                set _settings($cname-volumevisible) $bool
            }
            set _settings(-volumevisible) $bool
        }
        "-volumevisible" {
            # This is the component specific control.  It changes the
            # visibility of only the current component.
            set _settings($_current${what}) $_settings($what)
            foreach tag [GetDatasetsWithComponent $_current] {
                SendCmd "volume data state $_settings($what) $tag"
            }
        }
        "-xcutplanevisible" - "-ycutplanevisible" - "-zcutplanevisible" {
            set axis [string range $what 1 1]
            set bool $_settings($what)
            # We only set cutplanes on the first dataset.
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
        -markers ""
    }
    set tag $dataobj-$cname
    array set style [lindex [$dataobj components -style $cname] 0]
    if { ![info exists _cname2transferFunction($cname)] } {
        # Get the colormap right now, since it doesn't change with marker
        # changes.
        set cmap [ColorsToColormap $style(-color)]
        set amap [list 0.0 0.0 1.0 1.0]
        set _cname2transferFunction($cname) [list $cmap $amap]
        SendCmd [list transfunc define $cname $cmap $amap]
    }
    SendCmd "volume shading transfunc $cname $tag"
    if { ![info exists _transferFunctionEditors($cname)] } {
        set _transferFunctionEditors($cname) \
            [Rappture::TransferFunctionEditor ::\#auto $itk_component(legend) \
                 $cname \
                 -command [itcl::code $this updateTransferFunctions]]
    }
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
    foreach {cmap amap} $_cname2transferFunction($cname) break

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
    set amap [ComputeAlphamap $cname]
    set _cname2transferFunction($cname) [list $cmap $amap]
    SendCmd [list transfunc define $cname $cmap $amap]
}

itcl::body Rappture::NanovisViewer::AddNewMarker { x y } {
    if { ![info exists _transferFunctionEditors($_current)] } {
        continue
    }
    # Add a new marker to the current transfer function
    $_transferFunctionEditors($_current) newMarker $x $y normal
    $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
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
        set color $itk_option(-plotbackground)
        set rgb [Color2RGB $color]
        SendCmd "screen bgcolor $rgb"
        $itk_component(legend) configure -background $color
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotforeground {
    if { [isconnected] } {
        set color $itk_option(-plotforeground)
        set rgb [Color2RGB $color]
        SendCmd "volume outline color $rgb"
        SendCmd "grid axiscolor $rgb"
        SendCmd "grid linecolor $rgb"
        $itk_component(legend) itemconfigure labels -fill $color
        $itk_component(legend) itemconfigure limits -fill $color
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
    $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
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
    $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
}

# ----------------------------------------------------------------------
# USAGE: UndateTransferFuncs
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::updateTransferFunctions {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::BuildViewTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    set ::Rappture::NanovisViewer::_settings(-isosurfaceshading) 0
    checkbutton $inner.isosurface \
        -text "Isosurface shading" \
        -variable [itcl::scope _settings(-isosurfaceshading)] \
        -command [itcl::code $this AdjustSetting -isosurfaceshading] \
        -font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings(-axesvisible)] \
        -command [itcl::code $this AdjustSetting -axesvisible] \
        -font "Arial 9"

    checkbutton $inner.grid \
        -text "Grid" \
        -variable [itcl::scope _settings(-gridvisible)] \
        -command [itcl::code $this AdjustSetting -gridvisible] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings(-outlinevisible)] \
        -command [itcl::code $this AdjustSetting -outlinevisible] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings(-legendvisible)] \
        -command [itcl::code $this AdjustSetting -legendvisible] \
        -font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings(-volume)] \
        -command [itcl::code $this AdjustSetting -volume] \
        -font "Arial 9"

    label $inner.background_l -text "Background" -font "Arial 9"
    itk_component add background {
        Rappture::Combobox $inner.background -width 10 -editable no
    }
    $inner.background choices insert end \
        "black"              "black"            \
        "white"              "white"            \
        "grey"               "grey"

    $itk_component(background) value $_settings(-background)
    bind $inner.background <<Value>> \
        [itcl::code $this AdjustSetting -background]

    blt::table $inner \
        0,0 $inner.axes  -cspan 2 -anchor w \
        1,0 $inner.grid  -cspan 2 -anchor w \
        2,0 $inner.outline  -cspan 2 -anchor w \
        3,0 $inner.volume  -cspan 2 -anchor w \
        4,0 $inner.legend  -cspan 2 -anchor w \
        5,0 $inner.background_l       -anchor e -pady 2 \
        5,1 $inner.background                   -fill x \

    if 0 {
    bind $inner <Map> [itcl::code $this GetVolumeInfo $inner]
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r6 -resize expand
}

itcl::body Rappture::NanovisViewer::BuildVolumeTab {} {
    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    label $inner.lighting_l \
        -text "Lighting / Material Properties" \
        -font "Arial 9 bold"

    checkbutton $inner.light2side -text "Two-sided lighting" -font $fg \
        -variable [itcl::scope _settings(-light2side)] \
        -command [itcl::code $this AdjustSetting -light2side]

    checkbutton $inner.visibility -text "Visible" -font $fg \
        -variable [itcl::scope _settings(-volumevisible)] \
        -command [itcl::code $this AdjustSetting -volumevisible]

    label $inner.ambient_l -text "Ambient" -font $fg
    ::scale $inner.ambient -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-ambient)] \
        -showvalue off -command [itcl::code $this AdjustSetting -ambient] \
        -troughcolor grey92

    label $inner.diffuse_l -text "Diffuse" -font $fg
    ::scale $inner.diffuse -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-diffuse)] \
        -showvalue off -command [itcl::code $this AdjustSetting -diffuse] \
        -troughcolor grey92

    label $inner.specularLevel_l -text "Specular" -font $fg
    ::scale $inner.specularLevel -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-specularlevel)] \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -specularlevel] \
        -troughcolor grey92

    label $inner.specularExponent_l -text "Shininess" -font $fg
    ::scale $inner.specularExponent -from 10 -to 128 -orient horizontal \
        -variable [itcl::scope _settings(-specularexponent)] \
        -showvalue off \
        -command [itcl::code $this AdjustSetting -specularexponent] \
        -troughcolor grey92

    # Opacity
    label $inner.opacity_l -text "Opacity" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-opacity)] \
        -showvalue off -command [itcl::code $this AdjustSetting -opacity] \
        -troughcolor grey92

    label $inner.transferfunction_l \
        -text "Transfer Function" -font "Arial 9 bold"

    # Tooth thickness
    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings(-thickness)] \
        -showvalue off -command [itcl::code $this AdjustSetting -thickness] \
        -troughcolor grey92

    label $inner.thick -text "Thick" -font $fg

    # Colormap
    label $inner.colormap_l -text "Colormap" -font $fg
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable no
    }

    $inner.colormap choices insert end [GetColormapList -includeDefault -includeNone]
    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting -colormap]
    $itk_component(colormap) value "default"
    set _settings(-colormap) "default"

    # Component
    label $inner.volcomponents_l -text "Component" -font $fg
    itk_component add volcomponents {
        Rappture::Combobox $inner.volcomponents -editable no
    }
    bind $inner.volcomponents <<Value>> \
        [itcl::code $this AdjustSetting -current]

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
        9,1 $inner.opacity_l -anchor e -pady 2 \
        9,2 $inner.opacity                    -cspan 3 -fill x \
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
        -variable [itcl::scope _settings(-cutplanesvisible)] \
        -command [itcl::code $this AdjustSetting -cutplanesvisible] \
        -font "Arial 9"

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane] \
            -offimage [Rappture::icon x-cutplane] \
            -command [itcl::code $this AdjustSetting -xcutplanevisible] \
            -variable [itcl::scope _settings(-xcutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X cut plane on/off"
    $itk_component(xCutButton) select

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x] \
            -variable [itcl::scope _settings(-xcutplaneposition)]
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
            -command [itcl::code $this AdjustSetting -ycutplanevisible] \
            -variable [itcl::scope _settings(-ycutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y cut plane on/off"
    $itk_component(yCutButton) select

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y] \
            -variable [itcl::scope _settings(-ycutplaneposition)]
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
            -command [itcl::code $this AdjustSetting -zcutplanevisible] \
            -variable [itcl::scope _settings(-zcutplanevisible)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z cut plane on/off"
    $itk_component(zCutButton) select

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z] \
            -variable [itcl::scope _settings(-zcutplaneposition)]
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
    blt::table configure $inner r0 -resize none

    set row 1
    set labels { qw qx qy qz xpan ypan zoom }
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _settings(-$tag)]
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

    blt::table configure $inner c* -resize none
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
            set what [lindex $args 0]
            set x $_settings($what)
            set code [catch { string is double $x } result]
            if { $code != 0 || !$result } {
                set _settings($what) $_view($what)
                return
            }
            switch -- $what {
                "-xpan" - "-ypan" {
                    set _view($what) $_settings($what)
                    PanCamera
                }
                "-qx" - "-qy" - "-qz" - "-qw" {
                    set _view($what) $_settings($what)
                    set q [ViewToQuaternion]
                    $_arcball quaternion $q
                    SendCmd "camera orient $q"
                }
                "-zoom" {
                    set _view($what) $_settings($what)
                    SendCmd "camera zoom $_view($what)"
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
        if { ![info exists _settings(-volumevisible-$name)] } {
            set _settings(-volumevisible-$name) $info(hide)
        }
        checkbutton $inner.vol$row -text $info(label) \
            -variable [itcl::scope _settings(-volumevisible-$name)] \
            -onvalue 0 -offvalue 1 \
            -command [itcl::code $this ToggleVolume $key $name] \
            -font "Arial 9"
        Rappture::Tooltip::for $inner.vol$row $info(description)
        blt::table $inner $row,0 $inner.vol$row -anchor w
        if { !$_settings(-volume-$name) } {
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
    set bool $_settings(-volumevisible-$name)
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
    set _settings(-xpan) $_view(-xpan)
    set _settings(-ypan) $_view(-ypan)
    set _settings(-zoom) $_view(-zoom)
}


#
# InitComponentSettings --
#
#    Initializes the volume settings for a specific component. This should
#    match what's used as global settings above. This is called the first
#    time we try to switch to a given component in SwitchComponent below.
#
itcl::body Rappture::NanovisViewer::InitComponentSettings { cname } {
    foreach {key value} {
        -ambient           60
        -colormap          "default"
        -diffuse           40
        -light2side        1
        -opacity           50
        -specularexponent  90
        -specularlevel     30
        -thickness         350
        -volumevisible     1
    } {
        set _settings($cname${key}) $value
    }
}

#
# SwitchComponent --
#
#    This is called when the current component is changed by the dropdown
#    menu in the volume tab.  It synchronizes the global volume settings
#    with the settings of the new current component.
#
itcl::body Rappture::NanovisViewer::SwitchComponent { cname } {
    if { ![info exists _settings($cname-ambient)] } {
        InitComponentSettings $cname
    }
    # _settings variables change widgets, except for colormap
    set _settings(-ambient)          $_settings($cname-ambient)
    set _settings(-colormap)         $_settings($cname-colormap)
    set _settings(-diffuse)          $_settings($cname-diffuse)
    set _settings(-light2side)       $_settings($cname-light2side)
    set _settings(-opacity)          $_settings($cname-opacity)
    set _settings(-specularexponent) $_settings($cname-specularexponent)
    set _settings(-specularlevel)    $_settings($cname-specularlevel)
    set _settings(-thickness)        $_settings($cname-thickness)
    set _settings(-volumevisible)    $_settings($cname-volumevisible)
    $itk_component(colormap) value   $_settings($cname-colormap)
    set _current $cname;                # Reset the current component
}

#
# BuildVolumeComponents --
#
#    This is called from the "scale" method which is called when a new
#    dataset is added or deleted.  It repopulates the dropdown menu of
#    volume component names.  It sets the current component to the first
#    component in the list (of components found).  Finally, if there is
#    only one component, don't display the label or the combobox in the
#    volume settings tab.
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
#    Returns a list of all the datasets (known by the combination of their
#    data object and component name) that match the given component name.
#    For example, this is used where we want to change the settings of
#    volumes that have the current component.
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
#    Hide all the markers in all the transfer functions.  Can't simply
#    delete and recreate markers from the <style> since the user may have
#    created, deleted, or moved markers.
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

itcl::body Rappture::NanovisViewer::ResetColormap { cname color } {
    # Get the current transfer function
    if { ![info exists _cname2transferFunction($cname)] } {
        return
    }
    foreach { cmap amap } $_cname2transferFunction($cname) break
    set cmap [GetColormap $cname $color]
    set _cname2transferFunction($cname) [list $cmap $amap]
    SendCmd [list transfunc define $cname $cmap $amap]
    EventuallyRedrawLegend
}

itcl::body Rappture::NanovisViewer::ComputeAlphamap { cname } {
    if { ![info exists _transferFunctionEditors($cname)] } {
        return [list 0.0 0.0 1.0 1.0]
    }
    if { ![info exists _settings($cname-ambient)] } {
        InitComponentSettings $cname
    }

    set isovalues [$_transferFunctionEditors($cname) values]

    # Transfer function should be normalized with [0,1] range
    # The volume shading opacity setting is used to scale opacity
    # in the volume shader.
    set max 1.0

    # Use the component-wise thickness setting from the slider
    # settings widget
    # Scale values between 0.00001 and 0.01000
    set delta [expr {double($_settings($cname-thickness)) * 0.0001}]

    set first [lindex $isovalues 0]
    set last [lindex $isovalues end]
    set amap ""
    if { $first == "" || $first != 0.0 } {
        lappend amap 0.0 0.0
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
        lappend amap $x1 0.0
        lappend amap $x2 $max
        lappend amap $x3 $max
        lappend amap $x4 0.0
    }
    if { $last == "" || $last != 1.0 } {
        lappend amap 1.0 0.0
    }
    return $amap
}
