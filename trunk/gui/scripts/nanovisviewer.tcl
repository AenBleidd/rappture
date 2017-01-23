# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer - 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
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

    constructor { args } {
        Rappture::VisViewer::constructor
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
    public method overMarker { m x }
    public method parameters {title args} {
        # do nothing
    }
    public method removeDuplicateMarker { m x }
    public method scale {args}
    public method updateTransferFunctions {}

    # The following methods are only used by this class.
    private method AddIsoMarker { x y }
    private method AdjustSetting {what {value ""}}
    private method BuildCameraTab {}
    private method BuildCutplanesTab {}
    private method BuildDownloadPopup { widget command }
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    private method ComputeTransferFunction { tf }
    private method Connect {}
    private method CurrentDatasets {{what -all}}
    private method Disconnect {}
    private method DoResize {}
    private method DrawLegend { tf }
    private method EventuallyRedrawLegend { }
    private method EventuallyResize { w h }
    private method FixLegend {}
    private method GetImage { args }
    private method GetVtkData { args }
    private method InitSettings { args }
    private method NameTransferFunction { dataobj comp }
    private method Pan {option x y}
    private method PanCamera {}
    private method ParseLevelsOption { tf levels }
    private method ParseMarkersOption { tf markers }
    private method QuaternionToView { q } {
        foreach { _view(-qw) _view(-qx) _view(-qy) _view(-qz) } $q break
    }
    private method Rebuild {}
    private method ReceiveData { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { tf vmin vmax size }
    private method ResetColormap { color }
    private method Rotate {option x y}
    private method SendTransferFunctions {}
    private method SetOrientation { side }
    private method Slice {option args}
    private method SlicerTip {axis}
    private method ViewToQuaternion {} {
        return [list $_view(-qw) $_view(-qx) $_view(-qy) $_view(-qz)]
    }
    private method Zoom {option}

    private variable _arcball ""
    private variable _dlist "";         # list of data objects
    private variable _obj2ovride;       # maps dataobj => style override
    private variable _serverDatasets;   # contains all the dataobj-component
                                        # to volumes in the server
    private variable _recvdDatasets;    # list of data objs to send to server
    private variable _dataset2style;    # maps dataobj-component to transfunc
    private variable _style2datasets;   # maps tf back to list of
                                        # dataobj-components using the tf.

    private variable _reset 1;          # Connection to server has been reset.
    private variable _click;            # Info used for rotate operations.
    private variable _limits;           # Autoscale min/max for all axes
    private variable _view;             # View params for 3D view
    private variable _isomarkers;       # array of isosurface level values 0..1
    # Array of transfer functions in server.  If 0 the transfer has been
    # defined but not loaded.  If 1 the transfer function has been named
    # and loaded.
    private variable _activeTfs
    private variable _settings
    private variable _first "";         # This is the topmost volume.
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0

    private common _downloadPopup;      # download options from popup
    private common _hardcopy
}

itk::usual NanovisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::constructor {args} {
    set _serverType "nanovis"

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
    $_dispatcher dispatch $this !legend "[itcl::code $this FixLegend]; list"

    # Send transfer functions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
        "[itcl::code $this SendTransferFunctions]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]

    # Initialize the view to some default parameters.
    array set _view {
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

    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

    array set _settings [subst {
        -axesvisible            1
        -background             black
        -colormap               "BCGYR"
        -cutplanesvisible       0
        -gridvisible            0
        -isosurfaceshading      0
        -legendvisible          1
        -light                  40
        -light2side             1
        -opacity                50
        -outlinevisible         0
        -qw                     $_view(-qw)
        -qx                     $_view(-qx)
        -qy                     $_view(-qy)
        -qz                     $_view(-qz)
        -thickness              350
        -volume                 1
        -xcutplaneposition      50
        -xcutplanevisible       0
        -xpan                   $_view(-xpan)
        -ycutplaneposition      50
        -ycutplanevisible       0
        -ypan                   $_view(-ypan)
        -zcutplaneposition      50
        -zcutplanevisible       0
        -zoom                   $_view(-zoom)
    }]

    itk_component add view {
        label $itk_component(plotarea).view -image $_image(plot) \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth -background
    }
    bind $itk_component(view) <Control-F1> [itcl::code $this ToggleConsole]

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
            -variable [itcl::scope _settings(-volume)] \
            -command [itcl::code $this AdjustSetting -volume]
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
    #pack $itk_component(cutplane) -padx 2 -pady 2

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

    # Hack around the Tk panewindow.  The problem is that the requested
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w \
        1,0 $itk_component(legend) -fill x
    blt::table configure $itk_component(plotarea) r1 -resize none

    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> \
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

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
itcl::body Rappture::NanovisViewer::destructor {} {
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_transfunc
    $_dispatcher cancel !resize
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
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
# USAGE: get ?-image view|legend?
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
        "-objects" {
            # put the dataobj list in order according to -raise options
            set dlist $_dlist
            foreach dataobj $dlist {
                if {[info exists _obj2ovride($dataobj-raise)] &&
                    $_obj2ovride($dataobj-raise)} {
                    set i [lsearch -exact $dlist $dataobj]
                    if {$i >= 0} {
                        set dlist [lreplace $dlist $i $i]
                        lappend dlist $dataobj
                    }
                }
            }
            return $dlist
        }
        "-image" {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"get -image view|legend\""
            }
            switch -- [lindex $args end] {
                view {
                    return $_image(plot)
                }
                legend {
                    return $_image(legend)
                }
                default {
                    error "bad image name \"[lindex $args end]\": should be view or legend"
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
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.  No data objects are
# deleted.  They are only removed from the display list.
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
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }
    foreach dataobj $args {
        if { ![$dataobj isvalid] } {
            continue;                     # Object doesn't contain valid data.
        }
        foreach axis {x y z v} {
            foreach { min max } [$dataobj limits $axis] break
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
            set popup .nanovisviewerdownload
            if { ![winfo exists $popup] } {
                set inner [BuildDownloadPopup $popup [lindex $args 0]]
            } else {
                set inner [$popup component inner]
            }
            # FIXME: we only support download of current active component
            #set num [llength [get]]
            #set num [expr {($num == 1) ? "1 result" : "$num results"}]
            set num "current field component"
            set word [Rappture::filexfer::label downloadWord]
            $inner.summary configure -text "$word $num in the following format:"
            update idletasks            ;# Fix initial sizes
            return $popup
        }
        now {
            set popup .nanovisviewerdownload
            if { [winfo exists $popup] } {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                "image" {
                    return [$this GetImage [lindex $args 0]]
                }
                "vtk" {
                    return [$this GetVtkData [lindex $args 0]]
                }
                default {
                    error "bad download format \"$_downloadPopup(format)\""
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
# Clients use this method to disconnect from the current rendering server.
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
    if { $_first == "" } {
        puts stderr "first not set"
        return
    }
    # Ensure that the global thickness setting (in the slider
    # settings widget) is used for the active transfer-function.  Update
    # the value in the _settings variable.
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($_settings(-thickness)) * 0.0001}]

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
        set tf $_dataset2style($tag)
        set _settings($tf-thickness) $thickness
        ComputeTransferFunction $tf
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
    switch -- $info(-type) {
        "image" {
            #puts stderr "received image [image width $_image(plot)]x[image height $_image(plot)]"
            $_image(plot) configure -data $bytes
        }
        "print" {
            set tag $this-print-$info(-token)
            set _hardcopy($tag) $bytes
        }
        default {
            puts stderr "unknown image type $info(-type)"
        }
    }
}

#
# DrawLegend --
#
itcl::body Rappture::NanovisViewer::DrawLegend { tf } {
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
            -fill $itk_option(-plotforeground) -tags "title text"
        $c lower colorbar
        $c bind colorbar <ButtonRelease-1> [itcl::code $this AddIsoMarker %x %y]
    }

    # Display the markers used by the current transfer function.
    array set limits [limits $tf]
    $c itemconfigure vmin -text [format %g $limits(min)]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %g $limits(max)]
    $c coords vmax [expr {$w-$lx}] $ly

    if { $_first == "" } {
        return
    }
    set title [$_first hints label]
    set units [$_first hints units]
    if { $units != "" } {
        set title "$title ($units)"
    }
    $c itemconfigure title -text $title
    $c coords title [expr {$w/2}] $ly

    if { [info exists _isomarkers($tf)] } {
        # The "visible" isomarker method below implicitly calls "absval".
        # It uses the screen position of the marker to compute the absolute
        # value.  So make sure the window size has been computed before
        # calling "visible".
        update idletasks
        update 
        foreach m $_isomarkers($tf) {
            $m visible yes
        }
    }

    # The colormap may have changed. Resync the slicers with the colormap.
    set datasets [CurrentDatasets -cutplanes]
    SendCmd "volume data state $_settings(-volume) $datasets"

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
# ReceiveLegend --
#
# The procedure is the response from the render server to each "legend"
# command.  The server sends back a "legend" command invoked our
# the slave interpreter.  The purpose is to collect data of the image
# representing the legend in the canvas.  In addition, the
# active transfer function is displayed.
#
itcl::body Rappture::NanovisViewer::ReceiveLegend { tf vmin vmax size } {
    if { ![isconnected] } {
        return
    }
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

    DrawLegend $tf
}

#
# ReceiveData --
#
# The procedure is the response from the render server to each "data
# follows" command.  The server sends back a "data" command invoked our
# the slave interpreter.  The purpose is to collect the min/max of the
# volume sent to the render server.  Since the client (nanovisviewer)
# doesn't parse 3D data formats, we rely on the server (nanovis) to
# tell us what the limits are.  Once we've received the limits to all
# the data we've sent (tracked by _recvdDatasets) we can then determine
# what the transfer functions are for these volumes.
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
    if { $_settings(-volume) && $dataobj == $_first } {
        SendCmd "volume state 1 $tag"
    }
    set _limits($tag-min)  $info(min);  # Minimum value of the volume.
    set _limits($tag-max)  $info(max);  # Maximum value of the volume.
    set _limits(vmin)      $info(vmin); # Overall minimum value.
    set _limits(vmax)      $info(vmax); # Overall maximum value.

    unset _recvdDatasets($tag)
    if { [array size _recvdDatasets] == 0 } {
        # The active transfer function is by default the first component of
        # the first data object.  This assumes that the data is always
        # successfully transferred.
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

    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    # The "visible" isomarker method below implicitly calls "absval".  It
    # uses the screen position of the marker to compute the absolute value.
    # So make sure the window size has been computed before calling
    # "visible".
    update idletasks
    update 
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
        InitSettings -background -axesvisible -gridvisible
    }
    foreach dataobj [get] {
        foreach cname [$dataobj components] {
            set tag $dataobj-$cname
            if { ![info exists _serverDatasets($tag)] } {
                # Send the data as one huge base64-encoded mess -- yuck!
                if { [$dataobj type] == "dx" } {
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
    # Outline seems to need to be reset every update.
    InitSettings -outlinevisible ;#-cutplanesvisible
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

        InitSettings -opacity -light2side -isosurfaceshading \
            -light \
            -xcutplanevisible -ycutplanevisible -zcutplanevisible

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
        set _reset 0
    }

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
            $itk_component(view) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
        }
        drag {
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
            $itk_component(view) configure -cursor ""
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
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
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
        $itk_component(view) configure -cursor hand1
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
        $itk_component(view) configure -cursor ""
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
            #DrawLegend $_current
        }
        "-colormap" {
            set color [$itk_component(colormap) value]
            set _settings($what) $color
            # Only set the colormap on the first volume. Ignore the others.
            #ResetColormap $color
        }
        "-cutplanesvisible" {
            set bool $_settings($what)
            # We only set cutplanes on the first dataset.
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane visible $bool $tag"
        }
        "-gridvisible" {
            SendCmd "grid visible $_settings($what)"
        }
        "-isosurfaceshading" {
            set val $_settings($what)
            SendCmd "volume shading isosurface $val"
        }
        "-legendvisible" {
            if { $_settings($what) } {
                blt::table $itk_component(plotarea) \
                    0,0 $itk_component(view) -fill both \
                    1,0 $itk_component(legend) -fill x
                blt::table configure $itk_component(plotarea) r1 -resize none
            } else {
                blt::table forget $itk_component(legend)
            }
        }
        "-light" {
            set val $_settings($what)
            set diffuse [expr {0.01*$val}]
            set ambient [expr {1.0-$diffuse}]
            set specularLevel 0.3
            set specularExp 90.0
            SendCmd "volume shading ambient $ambient"
            SendCmd "volume shading diffuse $diffuse"
            SendCmd "volume shading specularLevel $specularLevel"
            SendCmd "volume shading specularExp $specularExp"
        }
        "-light2side" {
            set val $_settings($what)
            SendCmd "volume shading light2side $val"
        }
        "-opacity" {
            set val $_settings($what)
            set sval [expr { 0.01 * double($val) }]
            SendCmd "volume shading opacity $sval"
        }
        "-outlinevisible" {
            SendCmd "volume outline state $_settings($what)"
        }
        "-thickness" {
            if { [array names _activeTfs] > 0 } {
                set val $_settings($what)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                foreach tf [array names _activeTfs] {
                    set _settings($tf${what}) $sval
                    set _activeTfs($tf) 0
                }
                updateTransferFunctions
            }
        }
        "-volume" {
            set datasets [CurrentDatasets -cutplanes]
            SendCmd "volume data state $_settings($what) $datasets"
        }
        "-xcutplaneposition" - "-ycutplaneposition" - "-zcutplaneposition" {
            set axis [string range $what 1 1]
            set pos [expr $_settings($what) * 0.01]
            # We only set cutplanes on the first dataset.
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane position $pos $axis $tag"
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
    if {$w > 0 && $h > 0 && [array names _activeTfs] > 0 && $_first != "" } {
        set tag [lindex [CurrentDatasets] 0]
        if { [info exists _dataset2style($tag)] } {
            SendCmd "legend $_dataset2style($tag) $w $h"
        }
    }
}

#
# NameTransferFunction --
#
# Creates a transfer function name based on the <style> settings in the
# library run.xml file. This placeholder will be used later to create
# and send the actual transfer function once the data info has been sent
# to us by the render server. [We won't know the volume limits until the
# server parses the 3D data and sends back the limits via ReceiveData.]
#
#       FIXME: The current way we generate transfer-function names completely
#              ignores the -markers option.  The problem is that we are forced
#              to compute the name from an increasing complex set of values:
#              color, levels, markers.
#
itcl::body Rappture::NanovisViewer::NameTransferFunction { dataobj cname } {
    array set style {
        -color BCGYR
        -levels 6
    }
    set tag $dataobj-$cname
    array set style [lindex [$dataobj components -style $cname] 0]
    set tf "$style(-color):$style(-levels)"
    set _dataset2style($tag) $tf
    lappend _style2datasets($tf) $tag
    return $tf
}

#
# ComputeTransferFunction --
#
# Computes and sends the transfer function to the render server.  It's
# assumed that the volume data limits are known and that the global
# transfer-functions slider values have been set up.  Both parts are
# needed to compute the relative value (location) of the marker, and
# the alpha map of the transfer function.
#
itcl::body Rappture::NanovisViewer::ComputeTransferFunction { tf } {
    array set style {
        -color BCGYR
        -levels 6
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
    #        color, levels, markers.
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

    # Transfer function should be normalized with [0,1] range
    # The volume shading opacity setting is used to scale opacity
    # in the volume shader.
    set max 1.0

    set isovalues {}
    foreach m $_isomarkers($tf) {
        lappend isovalues [$m relval]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists _settings($tf-thickness)]} {
        set _settings($tf-thickness) 0.005
    }
    set delta $_settings($tf-thickness)

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
    SendCmd "transfunc define $tf { $cmap } { $amap }"
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
        $itk_component(legend) itemconfigure text -fill $color
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
            $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
            $m relval $x
            lappend _isomarkers($tf) $m
        }
    } else {
        foreach x $levels {
            set m [Rappture::IsoMarker \#auto $c $this $tf]
            $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
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
            $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
            $m relval $value
            lappend _isomarkers($tf) $m
        } else {
            # ${n} : Set absolute value.
            set m [Rappture::IsoMarker \#auto $c $this $tf]
            $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
            $m absval $value
            lappend _isomarkers($tf) $m
        }
    }
}

itcl::body Rappture::NanovisViewer::updateTransferFunctions {} {
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
    $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
    set w [winfo width $c]
    $m relval [expr {double($x-10)/($w-20)}]
    lappend _isomarkers($tf) $m
    updateTransferFunctions
    return 1
}

itcl::body Rappture::NanovisViewer::removeDuplicateMarker { marker x } {
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
        updateTransferFunctions
    }
    return $bool
}

itcl::body Rappture::NanovisViewer::overMarker { marker x } {
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
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

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
        "black" "black" \
        "white" "white" \
        "grey"  "grey"

    $itk_component(background) value $_settings(-background)
    bind $inner.background <<Value>> \
        [itcl::code $this AdjustSetting -background]

    blt::table $inner \
        0,0 $inner.axes -cspan 2 -anchor w \
        1,0 $inner.grid -cspan 2 -anchor w \
        2,0 $inner.outline -cspan 2 -anchor w \
        3,0 $inner.volume -cspan 2 -anchor w \
        4,0 $inner.legend -cspan 2 -anchor w \
        5,0 $inner.background_l -anchor e -pady 2 \
        5,1 $inner.background -fill x

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

    checkbutton $inner.vol -text "Show volume" -font $fg \
        -variable [itcl::scope _settings(-volume)] \
        -command [itcl::code $this AdjustSetting -volume]
    label $inner.shading -text "Shading:" -font $fg

    checkbutton $inner.isosurface -text "Isosurface shading" -font $fg \
        -variable [itcl::scope _settings(-isosurfaceshading)] \
        -command [itcl::code $this AdjustSetting -isosurfaceshading]

    checkbutton $inner.light2side -text "Two-sided lighting" -font $fg \
        -variable [itcl::scope _settings(-light2side)] \
        -command [itcl::code $this AdjustSetting -light2side]

    label $inner.dim -text "Glow" -font $fg
    ::scale $inner.light -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-light)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -light]
    label $inner.bright -text "Surface" -font $fg

    # Opacity
    label $inner.clear -text "Clear" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-opacity)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -opacity]
    label $inner.opaque -text "Opaque" -font $fg

    # Tooth thickness
    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings(-thickness)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -thickness]
    label $inner.thick -text "Thick" -font $fg

    # Colormap
    label $inner.colormap_l -text "Colormap" -font "Arial 9"
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable no
    }

    $inner.colormap choices insert end [GetColormapList -includeNone]
    $itk_component(colormap) value "BCGYR"
    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting -colormap]

    blt::table $inner \
        0,0 $inner.vol -cspan 4 -anchor w -pady 2 \
        1,0 $inner.shading -cspan 4 -anchor w -pady {10 2} \
        2,0 $inner.light2side -cspan 4 -anchor w -pady 2 \
        3,0 $inner.dim -anchor e -pady 2 \
        3,1 $inner.light -cspan 2 -pady 2 -fill x \
        3,3 $inner.bright -anchor w -pady 2 \
        4,0 $inner.clear -anchor e -pady 2 \
        4,1 $inner.opacity -cspan 2 -pady 2 -fill x \
        4,3 $inner.opaque -anchor w -pady 2 \
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
    #$itk_component(xCutButton) select

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
    #$itk_component(yCutButton) select

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
    #$itk_component(zCutButton) select

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
        0,1 $itk_component(xCutScale) \
        0,2 $itk_component(yCutScale) \
        0,3 $itk_component(zCutScale) \
        1,1 $itk_component(xCutButton) \
        1,2 $itk_component(yCutButton) \
        1,3 $itk_component(zCutButton)

    #    0,1 $inner.visible -anchor w -pady 2 -cspan 4 \

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

itcl::body Rappture::NanovisViewer::GetVtkData { args } {
    # FIXME: We can only put one component of one dataset in a single
    # VTK file.  To download all components/results, we would need
    # to put them in an archive (e.g. zip or tar file)
    if { $_first != ""} {
        set cname [lindex [$_first components] 0]
        set bytes [$_first vtkdata $cname]
        return [list .vtk $bytes]
    }
    puts stderr "Failed to get vtkdata"
    return ""
}

itcl::body Rappture::NanovisViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 &&
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::NanovisViewer::BuildDownloadPopup { popup command } {
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
        -font "Arial 9" \
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
