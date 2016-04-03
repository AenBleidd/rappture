# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: flowvisviewer - 3D flow rendering
#
#  This widget performs volume and flow rendering on 3D vector datasets.
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

option add *FlowvisViewer.width 5i widgetDefault
option add *FlowvisViewer*cursor crosshair widgetDefault
option add *FlowvisViewer.height 4i widgetDefault
option add *FlowvisViewer.foreground black widgetDefault
option add *FlowvisViewer.controlBackground gray widgetDefault
option add *FlowvisViewer.controlDarkBackground #999999 widgetDefault
option add *FlowvisViewer.plotBackground black widgetDefault
option add *FlowvisViewer.plotForeground white widgetDefault
option add *FlowvisViewer.plotOutline gray widgetDefault
option add *FlowvisViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc FlowvisViewer_init_resources {} {
    Rappture::resources::register \
        nanovis_server Rappture::FlowvisViewer::SetServerList
}

itcl::class Rappture::FlowvisViewer {
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
    public method flow {option}
    public method get {args}
    public method isconnected {}
    public method limits { cname }
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
    private method BuildVolumeComponents {}
    private method BuildVolumeTab {}
    private method ComputeTransferFunction { tf }
    private method Connect {}
    private method CurrentDatasets {{what -all}}
    private method Disconnect {}
    private method DoResize {}
    private method DrawLegend { tf }
    private method EventuallyGoto { nSteps }
    private method EventuallyRedrawLegend { }
    private method EventuallyResize { w h }
    private method FixLegend {}
    private method GetDatasetsWithComponent { cname }
    private method GetFlowInfo { widget }
    private method GetImage { args }
    private method GetMovie { widget width height }
    private method GetPngImage { widget width height }
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
    private method Rotate {option x y}
    private method SendFlowCmd { dataobj comp nbytes numComponents }
    private method SendTransferFunctions {}
    private method SetOrientation { side }
    private method Slice {option args}
    private method SlicerTip {axis}
    private method ViewToQuaternion {} {
        return [list $_view(-qw) $_view(-qx) $_view(-qy) $_view(-qz)]
    }
    private method WaitIcon { option widget }
    private method Zoom {option}
    private method arrows { tag name }
    private method box { tag name }
    private method millisecs2str { value }
    private method particles { tag name }
    private method str2millisecs { value }
    private method streams { tag name }

    private variable _arcball ""
    private variable _dlist ""         ;# list of data objects
    private variable _obj2ovride       ;# maps dataobj => style override
    private variable _datasets         ;# contains all the dataobj-component
                                       ;# to volumes in the server
    private variable _dataset2style    ;# maps dataobj-component to transfunc
    private variable _style2datasets   ;# maps tf back to list of
                                        # dataobj-components using the tf.
    private variable _dataset2flow     ;# Maps dataobj-component to a flow.

    private variable _reset 1          ;# Connection to server has been reset.
    private variable _click            ;# Info used for rotate operations.
    private variable _limits           ;# Autoscale min/max for all axes
    private variable _view             ;# View params for 3D view
    private variable _isomarkers       ;# array of isosurface level values 0..1
    private variable _settings
    private variable _activeTf ""      ;# Currently active transfer function
    private variable _first ""         ;# This is the topmost volume.
    private variable _volcomponents    ;# Maps component name to list of
                                       ;# dataobj-component tags
    private variable _componentsList   ;# List of components found
    private variable _nextToken 0
    private variable _icon 0
    private variable _flow
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0
    private variable _gotoPending 0

    private common _downloadPopup      ;# download options from popup
    private common _hardcopy
}

itk::usual FlowvisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::constructor {args} {
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

    $_dispatcher register !play
    $_dispatcher dispatch $this !play "[itcl::code $this flow next]; list"

    $_dispatcher register !goto
    $_dispatcher dispatch $this !goto "[itcl::code $this flow goto2]; list"

    $_dispatcher register !movietimeout
    $_dispatcher register !waiticon

    set _flow(state) 0

    array set _downloadPopup {
        format vtk
    }
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

    array set _settings [subst {
        -ambient                60
        -arrows                 0
        -axesvisible            0
        -background             black
        -colormap               BCGYR
        -currenttime            0
        -cutplanesvisible       0
        -diffuse                40
        -duration               1:00
        -gridvisible            0
        -isosurfaceshading      0
        -legendvisible          1
        -lic                    1
        -light2side             1
        -loop                   0
        -opacity                50
        -outlinevisible         1
        -particles              1
        -play                   0
        -qw                     $_view(-qw)
        -qx                     $_view(-qx)
        -qy                     $_view(-qy)
        -qz                     $_view(-qz)
        -specularexponent       90
        -specularlevel          30
        -speed                  500
        -step                   0
        -streams                0
        -thickness              350
        -volume                 1
        -xcutplaneposition      50
        -xcutplanevisible       1
        -xpan                   $_view(-xpan)
        -ycutplaneposition      50
        -ycutplanevisible       1
        -ypan                   $_view(-ypan)
        -zcutplaneposition      50
        -zcutplanevisible       1
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

    # Hack around the Tk panewindow.  The problem is that the requested
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w \
        1,0 $itk_component(legend) -fill x
    blt::table configure $itk_component(plotarea) r1 -resize none

    # Create flow controls...
    itk_component add flowcontrols {
        frame $itk_interior.flowcontrols
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack forget $itk_component(main)
    blt::table $itk_interior \
        0,0 $itk_component(main) -fill both  \
        1,0 $itk_component(flowcontrols) -fill x
    blt::table configure $itk_interior r1 -resize none

    # Rewind
    itk_component add rewind {
        button $itk_component(flowcontrols).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-rewind] \
            -command [itcl::code $this flow reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(rewind) \
        "Rewind flow"

    # Stop
    itk_component add stop {
        button $itk_component(flowcontrols).stop \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon flow-stop] \
            -command [itcl::code $this flow stop]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    Rappture::Tooltip::for $itk_component(stop) \
        "Stop flow"

    # Play
    itk_component add play {
        Rappture::PushButton $itk_component(flowcontrols).play \
            -onimage [Rappture::icon flow-pause] \
            -offimage [Rappture::icon flow-play] \
            -variable [itcl::scope _settings(-play)] \
            -command [itcl::code $this flow toggle]
    }
    set fg [option get $itk_component(hull) font Font]
    Rappture::Tooltip::for $itk_component(play) \
        "Play/Pause flow"

    # Loop
    itk_component add loop {
        Rappture::PushButton $itk_component(flowcontrols).loop \
            -onimage [Rappture::icon flow-loop] \
            -offimage [Rappture::icon flow-loop] \
            -variable [itcl::scope _settings(-loop)]
    }
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously"

    itk_component add dial {
        Rappture::Flowdial $itk_component(flowcontrols).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0.0 -max 1.0 \
            -variable [itcl::scope _settings(-currenttime)] \
            -knobimage [Rappture::icon knob2] -knobposition center@middle
    } {
        usual
        ignore -dialprogresscolor
        rename -background -controlbackground controlBackground Background
    }
    $itk_component(dial) current 0.0
    bind $itk_component(dial) <<Value>> [itcl::code $this flow goto]
    # Duration
    itk_component add duration {
        entry $itk_component(flowcontrols).duration \
            -textvariable [itcl::scope _settings(-duration)] \
            -bg white -width 6 -font "arial 9"
    } {
        usual
        ignore -highlightthickness -background
    }
    bind $itk_component(duration) <Return> [itcl::code $this flow duration]
    bind $itk_component(duration) <KP_Enter> [itcl::code $this flow duration]
    bind $itk_component(duration) <Tab> [itcl::code $this flow duration]
    Rappture::Tooltip::for $itk_component(duration) \
        "Set duration of flow (format is min:sec)"

    itk_component add durationlabel {
        label $itk_component(flowcontrols).durationl \
            -text "Duration:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    itk_component add speedlabel {
        label $itk_component(flowcontrols).speedl -text "Speed:" -font $fg \
            -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }

    # Speed
    itk_component add speed {
        Rappture::Flowspeed $itk_component(flowcontrols).speed \
            -min 1 -max 10 -width 3 -font "arial 9"
    } {
        usual
        ignore -highlightthickness
        rename -background -controlbackground controlBackground Background
    }
    Rappture::Tooltip::for $itk_component(speed) \
        "Change speed of flow"

    $itk_component(speed) value 1
    bind $itk_component(speed) <<Value>> [itcl::code $this flow speed]


    blt::table $itk_component(flowcontrols) \
        0,0 $itk_component(rewind) -padx {3 0} \
        0,1 $itk_component(stop) -padx {2 0} \
        0,2 $itk_component(play) -padx {2 0} \
        0,3 $itk_component(loop) -padx {2 0} \
        0,4 $itk_component(dial) -fill x -padx {2 0 } \
        0,5 $itk_component(duration) -padx { 0 0} \
        0,7 $itk_component(speed) -padx {2 3}

#        0,6 $itk_component(speedlabel) -padx {2 0}
    blt::table configure $itk_component(flowcontrols) c* -resize none
    blt::table configure $itk_component(flowcontrols) c4 -resize both
    blt::table configure $itk_component(flowcontrols) r0 -pady 1
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
itcl::body Rappture::FlowvisViewer::destructor {} {
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
itcl::body Rappture::FlowvisViewer::add {dataobj {settings ""}} {
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
    foreach cname [$dataobj components] {
        set flowobj [$dataobj flowhints $cname]
        if { $flowobj == "" } {
            puts stderr "no flowhints $dataobj-$cname"
            continue
        }
        set _dataset2flow($dataobj-$cname) $flowobj
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
itcl::body Rappture::FlowvisViewer::get {args} {
    if {[llength $args] == 0} {
        set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
        -objects {
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
        -image {
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
itcl::body Rappture::FlowvisViewer::delete {args} {
    flow stop
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
itcl::body Rappture::FlowvisViewer::scale {args} {
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
                set _settings($cname-colormap) $style(-color)
            }
            lappend _volcomponents($cname) $dataobj-$cname
            array unset limits
            array set limits [$dataobj valueLimits $cname]
            set _limits($cname) $limits(v)
        }
        foreach axis {x y z} {
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
    #BuildVolumeComponents
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
itcl::body Rappture::FlowvisViewer::download {option args} {
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
            set popup .flowvisviewerdownload
            if { ![winfo exists $popup] } {
                set inner [BuildDownloadPopup $popup [lindex $args 0]]
            } else {
                set inner [$popup component inner]
            }
            # FIXME: we only support download of current active component
            #set num [llength [get]]
            #set num [expr {($num == 1) ? "1 result" : "$num results"}]
            set num "current flow"
            set word [Rappture::filexfer::label downloadWord]
            $inner.summary configure -text "$word $num in the following format:"
            update idletasks            ;# Fix initial sizes
            return $popup
        }
        now {
            set popup .flowvisviewerdownload
            if { [winfo exists $popup] } {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                "640x480" {
                    return [$this GetMovie [lindex $args 0] 640 480]
                }
                "1024x768" {
                    return [$this GetMovie [lindex $args 0] 1024 768]
                }
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
itcl::body Rappture::FlowvisViewer::Connect {} {
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
            lappend info "client" "flowvisviewer"
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
itcl::body Rappture::FlowvisViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::FlowvisViewer::disconnect {} {
    Disconnect
}

#
# Disconnect --
#
# Clients use this method to disconnect from the current rendering server.
#
itcl::body Rappture::FlowvisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    array unset _datasets
}

# ----------------------------------------------------------------------
# USAGE: SendTransferFunctions
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::SendTransferFunctions {} {
    if { $_activeTf == "" } {
        puts stderr "no active tf"
        return
    }
    set tf $_activeTf
    if { $_first == "" } {
        puts stderr "first not set"
        return
    }

    # Ensure that the global thickness setting (in the slider
    # settings widget) is used for the active transfer-function.  Update
    # the value in the _settings variable.

    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($_settings(-thickness)) * 0.0001}]
    set _settings($tf-thickness) $thickness

    foreach tag [array names _dataset2style $_first-*] {
        if { [info exists _dataset2style($tag)] } {
            foreach tf $_dataset2style($tag) {
                ComputeTransferFunction $tf
            }
        }
    }
    EventuallyRedrawLegend
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::ReceiveImage { args } {
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
            set tag $this-$info(-token)
            set _hardcopy($tag) $bytes
        }
        "movie" {
            set tag $this-$info(-token)
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
itcl::body Rappture::FlowvisViewer::DrawLegend { tag } {
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

    if {$tag == "" || ![info exists _dataset2style($tag)]} {
        return
    }
    # Display the markers used by the current transfer function.
    set tf $_dataset2style($tag)
    foreach {min max} [limits $tf] break
    $c itemconfigure vmin -text [format %g $min]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %g $max]
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
        foreach m $_isomarkers($tf) {
            $m visible yes
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
itcl::body Rappture::FlowvisViewer::ReceiveLegend { tag vmin vmax size } {
    if { ![isconnected] } {
        return
    }
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

    DrawLegend $tag
}

#
# ReceiveData --
#
# The procedure is the response from the render server to each "data
# follows" command.  The server sends back a "data" command invoked our
# the slave interpreter.  The purpose was to collect the min/max of the
# volume sent to the render server.  This is no longer needed since we
# already know the limits.
#
itcl::body Rappture::FlowvisViewer::ReceiveData { args } {
    if { ![isconnected] } {
        return
    }

    # Arguments from server are name value pairs. Stuff them in an array.
    array set info $args

    set tag $info(tag)
    set _limits($tag) [list $info(min) $info(max)]
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Rebuild {} {
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

    set _first ""
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        foreach cname [$dataobj components] {
            set tag $dataobj-$cname
            if { ![info exists _datasets($tag)] } {
                if { [$dataobj type] == "dx" } {
                    set data [$dataobj blob $cname]
                } else {
                    set data [$dataobj vtkdata $cname]
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
                set numComponents [$dataobj numComponents $cname]
                # I have a field. Is a vector field or a volume field?
                if { $numComponents == 1 } {
                    SendCmd "volume data follows $nbytes $tag"
                } else {
                    if {[SendFlowCmd $dataobj $cname $nbytes $numComponents] < 0} {
                        continue
                    }
                }
                SendData $data
                set _datasets($tag) 1
                NameTransferFunction $dataobj $cname
            }
        }
    }

    # Turn off cutplanes for all volumes
    foreach axis {x y z} {
        SendCmd "cutplane state 0 $axis"
    }

    InitSettings -volume -outlinevisible -cutplanesvisible \
        -xcutplanevisible -ycutplanevisible -zcutplanevisible \
        -xcutplaneposition -ycutplaneposition -zcutplaneposition

    if { $_reset } {
        InitSettings -axesvisible -gridvisible \
            -opacity -light2side -isosurfaceshading \
            -ambient -diffuse -specularlevel -specularexponent

        #
        # Reset the camera and other view parameters
        #
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
        set _reset 0
    }

    if {"" != $_first} {
        set cname [lindex [$_first components] 0]
        set _activeTf [lindex $_dataset2style($_first-$cname) 0]
        # Make sure we display the proper transfer function in the legend.
        updateTransferFunctions
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
itcl::body Rappture::FlowvisViewer::CurrentDatasets {{what -all}} {
    set rlist ""
    if { $_first == "" } {
        return
    }
    foreach cname [$_first components] {
        set tag $_first-$cname
        if { [info exists _datasets($tag)] && $_datasets($tag) } {
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
itcl::body Rappture::FlowvisViewer::Zoom {option} {
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

itcl::body Rappture::FlowvisViewer::PanCamera {} {
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
itcl::body Rappture::FlowvisViewer::Rotate {option x y} {
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
itcl::body Rappture::FlowvisViewer::Pan {option x y} {
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
itcl::body Rappture::FlowvisViewer::InitSettings { args } {
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
itcl::body Rappture::FlowvisViewer::AdjustSetting {what {value ""}} {
    if {![isconnected]} {
        return
    }
    switch -- $what {
        "-ambient" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                set val [expr {0.01*$val}]
                SendCmd "$tag configure -ambient $val"
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
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                DrawLegend $tag
            }
        }
        "-colormap" {
            set color [$itk_component(colormap) value]
            set _settings($what) $color
            #ResetColormap $color
        }
        "-cutplanesvisible" {
            set bool $_settings($what)
            set datasets [CurrentDatasets -cutplanes]
            set tag [lindex $datasets 0]
            SendCmd "cutplane visible $bool $tag"
        }
        "-diffuse" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                set val [expr {0.01*$val}]
                SendCmd "$tag configure -diffuse $val"
            }
        }
        "-gridvisible" {
            SendCmd "grid visible $_settings($what)"
        }
        "-isosurfaceshading" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                # This flag isn't implemented in the server
                #SendCmd "$tag configure -isosurface $val"
            }
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
        "-light2side" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                SendCmd "$tag configure -light2side $val"
            }
        }
        "-opacity" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set opacity [expr { 0.01 * double($_settings($what)) }]
                SendCmd "$tag configure -opacity $opacity"
            }
        }
        "-outlinevisible" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                SendCmd "$tag configure -outline $_settings($what)"
            }
        }
        "-specularlevel" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                set val [expr {0.01*$val}]
                SendCmd "$tag configure -specularLevel $val"
            }
        }
        "-specularexponent" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set val $_settings($what)
                SendCmd "$tag configure -specularExp $val"
            }
        }
        "-thickness" {
            if { $_first != "" && $_activeTf != "" } {
                set val $_settings($what)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                set tf $_activeTf
                set _settings($tf${what}) $sval
                updateTransferFunctions
            }
        }
        "-volume" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                SendCmd "$tag configure -volume $_settings($what)"
            }
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
itcl::body Rappture::FlowvisViewer::FixLegend {} {
    set _resizeLegendPending 0
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {$_width-20}]
    set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]

    if { $_first == "" } {
        return
    }
    set comp [lindex [$_first components] 0]
    set tag $_first-$comp
    #set _activeTf [lindex $_dataset2style($tag) 0]
    if {$w > 0 && $h > 0 && "" != $_activeTf} {
        #SendCmd "legend $_activeTf $w $h"
        SendCmd "$tag legend $w $h"
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
itcl::body Rappture::FlowvisViewer::NameTransferFunction { dataobj cname } {
    array set style {
        -color BCGYR
        -levels 6
        -opacity 0.5
    }
    array set style [lindex [$dataobj components -style $cname] 0]
    # Some tools erroneously set -opacity to 1 in style, so
    # override the requested opacity for now
    set style(-opacity) 0.5
    set _settings(-opacity) [expr $style(-opacity) * 100]
    set _dataset2style($dataobj-$cname) $cname
    lappend _style2datasets($cname) $dataobj $cname
    return $cname
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
itcl::body Rappture::FlowvisViewer::ComputeTransferFunction { tf } {
    array set style {
        -color BCGYR
        -levels 6
        -opacity 0.5
    }
    set dataobj ""; set cname ""
    foreach {dataobj cname} $_style2datasets($tf) break
    if { $dataobj == "" } {
        return 0
    }
    array set style [lindex [$dataobj components -style $cname] 0]
    # Some tools erroneously set -opacity to 1 in style, so
    # override the requested opacity for now
    set style(-opacity) 0.5

    # We have to parse the style attributes for a volume using this
    # transfer-function *once*.  This sets up the initial isomarkers for the
    # transfer function.  The user may add/delete markers, so we have to
    # maintain a list of markers for each transfer-function.  We use the one
    # of the volumes (the first in the list) using the transfer-function as a
    # reference.

    if { ![info exists _isomarkers($tf)] } {
        # Have to defer creation of isomarkers until we have data limits
        if { [info exists style(-markers)] &&
             [llength $style(-markers)] > 0 } {
            ParseMarkersOption $tf $style(-markers)
        } else {
            ParseLevelsOption $tf $style(-levels)
        }
    }
    if { [info exists style(-nonuniformcolors)] } {
        foreach { value color } $style(-nonuniformcolors) {
            append cmap "$value [Color2RGB $color] "
        }
    } else {
        set cmap [ColorsToColormap $style(-color)]
    }

    if { ![info exists _settings(-opacity)] } {
        set _settings(-opacity) [expr $style(-opacity) * 100]
    }

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

    set tag $tf
    if { ![info exists _settings($tag-thickness)]} {
        set _settings($tag-thickness) 0.005
    }
    set delta $_settings($tag-thickness)

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
    return [SendCmd "$dataobj-$cname configure -transferfunction $tf"]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotbackground {
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
itcl::configbody Rappture::FlowvisViewer::plotforeground {
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
itcl::configbody Rappture::FlowvisViewer::plotoutline {
    # Must check if we are connected because this routine is called from the
    # class body when the -plotoutline itk_option is defined.  At that point
    # the FlowvisViewer class constructor hasn't been called, so we can't
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
itcl::body Rappture::FlowvisViewer::ParseLevelsOption { tf levels } {
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
itcl::body Rappture::FlowvisViewer::ParseMarkersOption { tf markers } {
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

itcl::body Rappture::FlowvisViewer::updateTransferFunctions {} {
    $_dispatcher event -after 100 !send_transfunc
}

itcl::body Rappture::FlowvisViewer::AddIsoMarker { x y } {
    if { $_activeTf == "" } {
        error "active transfer function isn't set"
    }
    set tf $_activeTf
    set c $itk_component(legend)
    set m [Rappture::IsoMarker \#auto $c $this $tf]
    $itk_component(legend) itemconfigure labels -fill $itk_option(-plotforeground)
    set w [winfo width $c]
    $m relval [expr {double($x-10)/($w-20)}]
    lappend _isomarkers($tf) $m
    updateTransferFunctions
    return 1
}

itcl::body Rappture::FlowvisViewer::removeDuplicateMarker { marker x } {
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

itcl::body Rappture::FlowvisViewer::overMarker { marker x } {
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

itcl::body Rappture::FlowvisViewer::limits { cname } {
    if { ![info exists _limits($cname)] } {
        puts stderr "no limits for cname=($cname)"
        return [list 0.0 1.0]
    }
    return $_limits($cname)
}

itcl::body Rappture::FlowvisViewer::BuildViewTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    # General options
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

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings(-legendvisible)] \
        -command [itcl::code $this AdjustSetting -legendvisible] \
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

    # Dataset options
    label $inner.flow_l -text "Flow" -font "Arial 9 bold"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings(-outlinevisible)] \
        -command [itcl::code $this AdjustSetting -outlinevisible] \
        -font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings(-volume)] \
        -command [itcl::code $this AdjustSetting -volume] \
        -font "Arial 9"

    frame $inner.frame

    blt::table $inner \
        0,0 $inner.axes -cspan 2 -anchor w \
        1,0 $inner.grid -cspan 2 -anchor w \
        2,0 $inner.legend -cspan 2 -anchor w \
        3,0 $inner.background_l -anchor e -pady 2 \
        3,1 $inner.background -fill x \
        4,0 $inner.flow_l -anchor w \
        5,0 $inner.outline -cspan 2 -anchor w \
        6,0 $inner.volume -cspan 2 -anchor w \

    bind $inner <Map> [itcl::code $this GetFlowInfo $inner]

    blt::table configure $inner c* r* -resize none
    blt::table configure $inner c2 r7 -resize expand
}

itcl::body Rappture::FlowvisViewer::BuildVolumeTab {} {
    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    checkbutton $inner.vol -text "Show volume" -font $fg \
        -text "Volume" \
        -variable [itcl::scope _settings(-volume)] \
        -command [itcl::code $this AdjustSetting -volume] \
        -font "Arial 9"

    label $inner.lighting_l \
        -text "Lighting / Material Properties" \
        -font "Arial 9 bold"

    checkbutton $inner.isosurface -text "Isosurface shading" -font $fg \
        -variable [itcl::scope _settings(-isosurfaceshading)] \
        -command [itcl::code $this AdjustSetting -isosurfaceshading]

    checkbutton $inner.light2side -text "Two-sided lighting" -font $fg \
        -variable [itcl::scope _settings(-light2side)] \
        -command [itcl::code $this AdjustSetting -light2side]

    label $inner.ambient_l -text "Ambient" -font $fg
    ::scale $inner.ambient -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-ambient)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -ambient]

    label $inner.diffuse_l -text "Diffuse" -font $fg
    ::scale $inner.diffuse -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-diffuse)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -diffuse]

    label $inner.specularLevel_l -text "Specular" -font $fg
    ::scale $inner.specularLevel -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-specularlevel)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -specularlevel]

    label $inner.specularExponent_l -text "Shininess" -font $fg
    ::scale $inner.specularExponent -from 10 -to 128 -orient horizontal \
        -variable [itcl::scope _settings(-specularexponent)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -specularexponent]

    # Opacity
    label $inner.opacity_l -text "Opacity" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(-opacity)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -opacity]

    label $inner.transferfunction_l \
        -text "Transfer Function" -font "Arial 9 bold"

    # Tooth thickness
    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings(-thickness)] \
        -width 10 \
        -showvalue off -command [itcl::code $this AdjustSetting -thickness]
    label $inner.thick -text "Thick" -font $fg

    # Colormap
    label $inner.colormap_l -text "Colormap" -font $fg
    itk_component add colormap {
        Rappture::Combobox $inner.colormap -width 10 -editable no
    }
    $inner.colormap choices insert end [GetColormapList -includeNone]
    bind $inner.colormap <<Value>> \
        [itcl::code $this AdjustSetting -colormap]
    $itk_component(colormap) value "BCGYR"
    set _settings(-colormap) "BCGYR"

    blt::table $inner \
        0,0 $inner.vol -cspan 4 -anchor w -pady 2 \
        1,0 $inner.lighting_l -cspan 4 -anchor w -pady {10 2} \
        2,0 $inner.light2side -cspan 4 -anchor w -pady 2 \
        3,0 $inner.ambient_l -anchor e -pady 2 \
        3,1 $inner.ambient -cspan 3 -pady 2 -fill x \
        4,0 $inner.diffuse_l -anchor e -pady 2 \
        4,1 $inner.diffuse -cspan 3 -pady 2 -fill x \
        5,0 $inner.specularLevel_l -anchor e -pady 2 \
        5,1 $inner.specularLevel -cspan 3 -pady 2 -fill x \
        6,0 $inner.specularExponent_l -anchor e -pady 2 \
        6,1 $inner.specularExponent -cspan 3 -pady 2 -fill x \
        7,0 $inner.opacity_l -anchor e -pady 2 \
        7,1 $inner.opacity -cspan 3 -pady 2 -fill x \
        8,0 $inner.thin -anchor e -pady 2 \
        8,1 $inner.thickness -cspan 2 -pady 2 -fill x \
        8,3 $inner.thick -anchor w -pady 2

    blt::table configure $inner c0 c1 c3 r* -resize none
    blt::table configure $inner r9 -resize expand
}

itcl::body Rappture::FlowvisViewer::BuildCutplanesTab {} {
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
        0,1 $inner.visible -anchor w -pady 2 -cspan 4 \
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

itcl::body Rappture::FlowvisViewer::BuildCameraTab {} {
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

itcl::body Rappture::FlowvisViewer::GetFlowInfo { w } {
    set flowobj ""
    foreach key [array names _dataset2flow] {
        set flowobj $_dataset2flow($key)
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
        7,0 $inner -fill both -cspan 2 -anchor nw
    array set hints [$flowobj hints]
    checkbutton $inner.showstreams -text "Streams Plane" \
        -variable [itcl::scope _settings(-streams)] \
        -command  [itcl::code $this streams $key $hints(name)]  \
        -font "Arial 9"
    Rappture::Tooltip::for $inner.showstreams $hints(description)

    checkbutton $inner.showarrows -text "Arrows" \
        -variable [itcl::scope _settings(-arrows)] \
        -command  [itcl::code $this arrows $key $hints(name)]  \
        -font "Arial 9"

    label $inner.particles -text "Particles" -font "Arial 9 bold"
    label $inner.boxes -text "Boxes" -font "Arial 9 bold"

    blt::table $inner \
        1,0 $inner.showstreams  -anchor w \
        2,0 $inner.showarrows  -anchor w
    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand

    set row 3
    set particles [$flowobj particles]
    if { [llength $particles] > 0 } {
        blt::table $inner $row,0 $inner.particles  -anchor w
        incr row
    }
    foreach part $particles {
        array unset info
        array set info $part
        set name $info(name)
        if { ![info exists _settings(-particles-$name)] } {
            set _settings(-particles-$name) $info(hide)
        }
        checkbutton $inner.part$row -text $info(label) \
            -variable [itcl::scope _settings(-particles-$name)] \
            -onvalue 0 -offvalue 1 \
            -command [itcl::code $this particles $key $name] \
            -font "Arial 9"
        Rappture::Tooltip::for $inner.part$row $info(description)
        blt::table $inner $row,0 $inner.part$row -anchor w
        if { !$_settings(-particles-$name) } {
            $inner.part$row select
        }
        incr row
    }
    set boxes [$flowobj boxes]
    if { [llength $boxes] > 0 } {
        blt::table $inner $row,0 $inner.boxes  -anchor w
        incr row
    }
    foreach box $boxes {
        array unset info
        array set info $box
        set name $info(name)
        if { ![info exists _settings(-box-$name)] } {
            set _settings(-box-$name) $info(hide)
        }
        checkbutton $inner.box$row -text $info(label) \
            -variable [itcl::scope _settings(-box-$name)] \
            -onvalue 0 -offvalue 1 \
            -command [itcl::code $this box $key $name] \
            -font "Arial 9"
        Rappture::Tooltip::for $inner.box$row $info(description)
        blt::table $inner $row,0 $inner.box$row -anchor w
        if { !$_settings(-box-$name) } {
            $inner.box$row select
        }
        incr row
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r$row -resize expand
    blt::table configure $inner c3 -resize expand
    event generate [winfo parent [winfo parent $w]] <Configure>
}

itcl::body Rappture::FlowvisViewer::particles { tag name } {
    set bool $_settings(-particles-$name)
    SendCmd "$tag particles configure {$name} -hide $bool"
}

itcl::body Rappture::FlowvisViewer::box { tag name } {
    set bool $_settings(-box-$name)
    SendCmd "$tag box configure {$name} -hide $bool"
}

itcl::body Rappture::FlowvisViewer::streams { tag name } {
    set bool $_settings(-streams)
    SendCmd "$tag configure -slice $bool"
}

itcl::body Rappture::FlowvisViewer::arrows { tag name } {
    set bool $_settings(-arrows)
    SendCmd "$tag configure -arrows $bool"
}

# ----------------------------------------------------------------------
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Slice {option args} {
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
itcl::body Rappture::FlowvisViewer::SlicerTip {axis} {
    set val [$itk_component(${axis}CutScale) get]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}

itcl::body Rappture::FlowvisViewer::DoResize {} {
    $_arcball resize $_width $_height
    SendCmd "screen size $_width $_height"
    set _resizePending 0
}

itcl::body Rappture::FlowvisViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        $_dispatcher event -after 200 !resize
        set _resizePending 1
    }
}

itcl::body Rappture::FlowvisViewer::EventuallyRedrawLegend {} {
    if { !$_resizeLegendPending } {
        $_dispatcher event -after 100 !legend
        set _resizeLegendPending 1
    }
}

itcl::body Rappture::FlowvisViewer::EventuallyGoto { nSteps } {
    set _flow(goto) $nSteps
    if { !$_gotoPending } {
        $_dispatcher event -after 1000 !goto
        set _gotoPending 1
    }
}

#  camera --
#
itcl::body Rappture::FlowvisViewer::camera {option args} {
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

itcl::body Rappture::FlowvisViewer::SendFlowCmd { dataobj comp nbytes numComponents } {
    set tag "$dataobj-$comp"
    if { ![info exists _dataset2flow($tag)] } {
        SendCmd "flow add $tag"
        SendCmd "$tag data follows $nbytes $numComponents"
        return 0
    }
    set flowobj $_dataset2flow($tag)
    if { $flowobj == "" } {
        puts stderr "no flowobj"
        return -1
    }
    SendCmd "if {\[flow exists $tag\]} {flow delete $tag}"
    array set info [$flowobj hints]
    set _settings(-volume) $info(volume)
    set _settings(-outlinevisible) $info(outline)
    set _settings(-arrows) $info(arrows)
    set _settings(-duration) $info(duration)
    $itk_component(speed) value $info(speed)
    set cmd {}
    append cmd "flow add $tag"
    append cmd " -position $info(position)"
    append cmd " -axis $info(axis)"
    append cmd " -volume $info(volume)"
    append cmd " -outline $info(outline)"
    append cmd " -slice $info(streams)"
    append cmd " -arrows $info(arrows)"
    SendCmd $cmd
    foreach part [$flowobj particles] {
        set cmd {}
        array unset info
        array set info $part
        set color [Color2RGB $info(color)]
        append cmd "$tag particles add $info(name)"
        append cmd " -position $info(position)"
        append cmd " -hide $info(hide)"
        append cmd " -axis $info(axis)"
        append cmd " -color {$color}"
        append cmd " -size $info(size)"
        SendCmd $cmd
    }
    foreach box [$flowobj boxes] {
        set cmd {}
        array unset info
        set info(corner1) ""
        set info(corner2) ""
        array set info $box
        if { $info(corner1) == "" || $info(corner2) == "" } {
            continue
        }
        set color [Color2RGB $info(color)]
        append cmd "$tag box add $info(name)"
        append cmd " -color {$color}"
        append cmd " -hide $info(hide)"
        append cmd " -linewidth $info(linewidth) "
        append cmd " -corner1 {$info(corner1)} "
        append cmd " -corner2 {$info(corner2)}"
        SendCmd $cmd
    }
    SendCmd "$tag data follows $nbytes $numComponents"
    return 0
}

#
# flow --
#
# Called when the user clicks on the stop or play buttons
# for flow visualization.
#
#        $this flow play
#        $this flow stop
#        $this flow toggle
#        $this flow reset
#        $this flow pause
#        $this flow next
#
itcl::body Rappture::FlowvisViewer::flow { args } {
    set option [lindex $args 0]
    switch -- $option {
        "goto2" {
            puts stderr "actually sending \"flow goto $_flow(goto)\""
            SendCmd "flow goto $_flow(goto)"
            set _gotoPending 0
        }
        "goto" {
            puts stderr "flow goto to $_settings(-currenttime)"
            # Figure out how many steps to the current time based upon
            # the speed and duration.
            set current $_settings(-currenttime)
            set speed [$itk_component(speed) value]
            set time [str2millisecs $_settings(-duration)]
            $itk_component(dial) configure -max $time
            set delay [expr int(round(500.0/$speed))]
            set timePerStep [expr {double($time) / $delay}]
            set nSteps [expr {int(ceil($current/$timePerStep))}]
            EventuallyGoto $nSteps
        }
        "speed" {
            set speed [$itk_component(speed) value]
            set _flow(delay) [expr int(round(500.0/$speed))]
        }
        "duration" {
            set max [str2millisecs $_settings(-duration)]
            if { $max < 0 } {
                bell
                return
            }
            set _flow(duration) $max
            set _settings(-duration) [millisecs2str $max]
            $itk_component(dial) configure -max $max
        }
        "off" {
            set _flow(state) 0
            $_dispatcher cancel !play
            $itk_component(play) deselect
        }
        "on" {
            flow speed
            flow duration
            set _flow(state) 1
            set _settings(-currenttime) 0
            $itk_component(play) select
        }
        "stop" {
            if { $_flow(state) } {
                flow off
                flow reset
            }
        }
        "pause" {
            if { $_flow(state) } {
                flow off
            }
        }
        "play" {
            # If the flow is currently off, then restart it.
            if { !$_flow(state) } {
                flow on
                # If we're at the end of the flow, reset the flow.
                set _settings(-currenttime) \
                    [expr {$_settings(-currenttime) + $_flow(delay)}]
                if { $_settings(-currenttime) >= $_flow(duration) } {
                    set _settings(-step) 1
                    SendCmd "flow reset"
                }
                flow next
            }
        }
        "toggle" {
            if { $_settings(-play) } {
                flow play
            } else {
                flow pause
            }
        }
        "reset" {
            set _settings(-currenttime) 0
            SendCmd "flow reset"
        }
        "next" {
            if { ![winfo viewable $itk_component(view)] } {
                flow stop
                return
            }
            set _settings(-currenttime) \
                [expr {$_settings(-currenttime) + $_flow(delay)}]
            if { $_settings(-currenttime) >= $_flow(duration) } {
                if { !$_settings(-loop) } {
                    flow off
                    return
                }
                flow reset
            } else {
                SendCmd "flow next"
            }
            $_dispatcher event -after $_flow(delay) !play
        }
        default {
            error "bad option \"$option\": should be play, stop, toggle, or reset."
        }
    }
}

itcl::body Rappture::FlowvisViewer::WaitIcon { option widget } {
    switch -- $option {
        "start" {
            $_dispatcher dispatch $this !waiticon \
                "[itcl::code $this WaitIcon "next" $widget] ; list"
            set _icon 0
            $widget configure -image [Rappture::icon bigroller${_icon}]
            $_dispatcher event -after 100 !waiticon
        }
        "next" {
            incr _icon
            if { $_icon >= 8 } {
                set _icon 0
            }
            $widget configure -image [Rappture::icon bigroller${_icon}]
            $_dispatcher event -after 100 !waiticon
        }
        "stop" {
            $_dispatcher cancel !waiticon
        }
    }
}

itcl::body Rappture::FlowvisViewer::GetVtkData { args } {
    # FIXME: We can only put one component of one dataset in a single
    # VTK file.  To download all components/results, we would need
    # to put them in an archive (e.g. zip or tar file)
    if { $_first != "" } {
        set cname [lindex [$_first components] 0]
        set bytes [$_first vtkdata $cname]
        return [list .vtk $bytes]
    }
    puts stderr "Failed to get vtkdata"
    return ""
}

itcl::body Rappture::FlowvisViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 &&
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::FlowvisViewer::GetPngImage { widget width height } {
    set token "print[incr _nextToken]"
    set var ::Rappture::FlowvisViewer::_hardcopy($this-$token)
    set $var ""

    # Setup an automatic timeout procedure.
    $_dispatcher dispatch $this !pngtimeout "set $var {} ; list"

    set popup .flowvisviewerprint
    if {![winfo exists $popup]} {
        Rappture::Balloon $popup -title "Generating file..."
        set inner [$popup component inner]
        label $inner.title -text "Generating hardcopy." -font "Arial 10 bold"
        label $inner.please -text "This may take a minute." -font "Arial 10"
        label $inner.icon -image [Rappture::icon bigroller0]
        button $inner.cancel -text "Cancel" -font "Arial 10 bold" \
            -command [list set $var ""]
        blt::table $inner \
            0,0 $inner.title -cspan 2 \
            1,0 $inner.please -anchor w \
            1,1 $inner.icon -anchor e  \
            2,0 $inner.cancel -cspan 2
        blt::table configure $inner r0 -pady 4
        blt::table configure $inner r2 -pady 4
        bind $inner.cancel <KeyPress-Return> [list $inner.cancel invoke]
    } else {
        set inner [$popup component inner]
    }

    $_dispatcher event -after 60000 !pngtimeout
    WaitIcon start $inner.icon
    grab set $inner
    focus $inner.cancel

    SendCmd "print $token $width $height"

    $popup activate $widget below
    update idletasks
    update
    # We wait here for either
    #  1) the png to be delivered or
    #  2) timeout or
    #  3) user cancels the operation.
    tkwait variable $var

    # Clean up.
    $_dispatcher cancel !pngtimeout
    WaitIcon stop $inner.icon
    grab release $inner
    $popup deactivate
    update

    if { $_hardcopy($this-$token) != "" } {
        return [list .png $_hardcopy($this-$token)]
    }
    return ""
}

itcl::body Rappture::FlowvisViewer::GetMovie { widget w h } {
    set token "movie[incr _nextToken]"
    set var ::Rappture::FlowvisViewer::_hardcopy($this-$token)
    set $var ""

    # Setup an automatic timeout procedure.
    $_dispatcher dispatch $this !movietimeout "set $var {} ; list"
    set popup .flowvisviewermovie
    if {![winfo exists $popup]} {
        Rappture::Balloon $popup -title "Generating movie..."
        set inner [$popup component inner]
        label $inner.title -text "Generating movie for download" \
                -font "Arial 10 bold"
        label $inner.please -text "This may take a few minutes." \
                -font "Arial 10"
        label $inner.icon -image [Rappture::icon bigroller0]
        button $inner.cancel -text "Cancel" -font "Arial 10 bold" \
            -command [list set $var ""]
        blt::table $inner \
            0,0 $inner.title -cspan 2 \
            1,0 $inner.please -anchor w \
            1,1 $inner.icon -anchor e  \
            2,0 $inner.cancel -cspan 2
        blt::table configure $inner r0 -pady 4
        blt::table configure $inner r2 -pady 4
        bind $inner.cancel <KeyPress-Return> [list $inner.cancel invoke]
    } else {
        set inner [$popup component inner]
    }
    update
    # Timeout is set to 10 minutes.
    $_dispatcher event -after 600000 !movietimeout
    WaitIcon start $inner.icon
    grab set $inner
    focus $inner.cancel

    flow duration
    flow speed
    set nframes [expr round($_flow(duration) / $_flow(delay))]
    set framerate [expr 1000.0 / $_flow(delay)]

    # These are specific to MPEG1 video generation
    set framerate 25.0
    set bitrate 6.0e+6

    set start [clock seconds]
    SendCmd "flow video $token -width $w -height $h -numframes $nframes "

    $popup activate $widget below
    update idletasks
    update
    # We wait here until
    #  1. the movie is delivered or
    #  2. we've timed out or
    #  3. the user has canceled the operation.b
    tkwait variable $var

    puts stderr "Video generated in [expr [clock seconds] - $start] seconds."

    # Clean up.
    $_dispatcher cancel !movietimeout
    WaitIcon stop $inner.icon
    grab release $inner
    $popup deactivate
    destroy $popup
    update

    # This will both cancel the movie generation (if it hasn't already
    # completed) and reset the flow.
    SendCmd "flow reset"
    if { $_hardcopy($this-$token) != "" } {
        return [list .mpg $_hardcopy($this-$token)]
    }
    return ""
}

itcl::body Rappture::FlowvisViewer::BuildDownloadPopup { popup command } {
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
        -font "Arial 9 " \
        -value image
    Rappture::Tooltip::for $inner.image_button \
        "Save as digital image."

    set res "640x480"
    radiobutton $inner.movie_std -text "Movie (standard $res)" \
        -variable [itcl::scope _downloadPopup(format)] \
        -value $res
    Rappture::Tooltip::for $inner.movie_std \
        "Save as movie file."

    set res "1024x768"
    radiobutton $inner.movie_high -text "Movie (high quality $res)" \
        -variable [itcl::scope _downloadPopup(format)] \
        -value $res
    Rappture::Tooltip::for $inner.movie_high \
        "Save as movie file."

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
        3,0 $inner.movie_std -anchor w -cspan 2 -padx { 4 0 } \
        4,0 $inner.movie_high -anchor w -cspan 2 -padx { 4 0 } \
        6,1 $inner.cancel -width .9i -fill y \
        6,0 $inner.ok -padx 2 -width .9i -fill y
    blt::table configure $inner r5 -height 4
    blt::table configure $inner r6 -pady 4
    raise $inner.image_button
    $inner.vtk_button invoke
    return $inner
}

itcl::body Rappture::FlowvisViewer::str2millisecs { value } {
    set parts [split $value :]
    set secs 0
    set mins 0
    if { [llength $parts] == 1 } {
        scan [lindex $parts 0] "%d" secs
    } else {
        scan [lindex $parts 1] "%d" secs
        scan [lindex $parts 0] "%d" mins
    }
    set ms [expr {(($mins * 60) + $secs) * 1000.0}]
    if { $ms > 600000.0 } {
        set ms 600000.0
    }
    if { $ms == 0.0 } {
        set ms 60000.0
    }
    return $ms
}

itcl::body Rappture::FlowvisViewer::millisecs2str { value } {
    set min [expr floor($value / 60000.0)]
    set sec [expr ($value - ($min*60000.0)) / 1000.0]
    return [format %02d:%02d [expr round($min)] [expr round($sec)]]
}

itcl::body Rappture::FlowvisViewer::SetOrientation { side } {
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
# BuildVolumeComponents --
#
# This is called from the "scale" method which is called when a new
# dataset is added or deleted.  It repopulates the dropdown menu of
# volume component names.  It sets the current component to the first
# component in the list (of components found).  Finally, if there is
# only one component, don't display the label or the combobox in the
# volume settings tab.
#
itcl::body Rappture::FlowvisViewer::BuildVolumeComponents {} {
    $itk_component(volcomponents) choices delete 0 end
    foreach name $_componentsList {
        $itk_component(volcomponents) choices insert end $name $name
    }
    set _current [lindex $_componentsList 0]
    $itk_component(volcomponents) value $_current
}

#
# GetDatasetsWithComponents --
#
# Returns a list of all the datasets (known by the combination of their
# data object and component name) that match the given component name.
# For example, this is used where we want to change the settings of
# volumes that have the current component.
#
itcl::body Rappture::FlowvisViewer::GetDatasetsWithComponent { cname } {
    if { ![info exists _volcomponents($cname)] } {
        return ""
    }
    set list ""
    foreach tag $_volcomponents($cname) {
        if { ![info exists _datasets($tag)] } {
            continue
        }
        lappend list $tag
    }
    return $list
}
