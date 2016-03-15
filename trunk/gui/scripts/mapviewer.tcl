# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: mapviewer - Map object viewer
#
#  It connects to the GeoVis server running on a rendering farm,
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

option add *MapViewer.width 4i widgetDefault
option add *MapViewer*cursor crosshair widgetDefault
option add *MapViewer.height 4i widgetDefault
option add *MapViewer.foreground black widgetDefault
option add *MapViewer.controlBackground gray widgetDefault
option add *MapViewer.controlDarkBackground #999999 widgetDefault
option add *MapViewer.plotBackground black widgetDefault
option add *MapViewer.plotForeground white widgetDefault
option add *MapViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc MapViewer_init_resources {} {
    Rappture::resources::register \
        geovis_server Rappture::MapViewer::SetServerList
}

itcl::class Rappture::MapViewer {
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
        Rappture::VisViewer::SetServerList "geovis" $namelist
    }
    public method add {dataobj {settings ""}}
    public method camera {option args}
    public method delete {args}
    public method disconnect {}
    public method download {option args}
    public method get {args}
    public method isconnected {}
    public method parameters {title args} { # do nothing }
    public method scale {args}
    public method select {option {args ""}}
    public method setSelectCallback {cmd}

    private method KeyPress { key }
    private method KeyRelease { key }
    private method MouseClick { button x y }
    private method MouseDoubleClick { button x y }
    private method MouseDrag { button x y }
    private method MouseMotion {}
    private method MouseRelease { button x y }
    private method MouseScroll { direction }

    # The following methods are only used by this class.
    private method AdjustSetting {what {value ""}}
    private method BuildCameraTab {}
    private method BuildDownloadPopup { widget command }
    private method BuildHelpTab {}
    private method BuildLayerTab {}
    private method BuildMapTab {}
    private method BuildTerrainTab {}
    private method BuildViewpointsTab {}
    private method Camera {option args}
    private method Connect {}
    private method CurrentLayers {args}
    private method DisablePanningMouseBindings {}
    private method DisableRotationMouseBindings {}
    private method DisableZoomMouseBindings {}
    private method Disconnect {}
    private method DoPan {}
    private method DoResize {}
    private method DoRotate {}
    private method DoSelect {}
    private method DoSelectCallback {option {args ""}}
    private method DrawLegend { colormap min max }
    private method EnablePanningMouseBindings {}
    private method EnableRotationMouseBindings {}
    private method EnableZoomMouseBindings {}
    private method EventuallyHandleMotionEvent { x y }
    private method EventuallyPan { dx dy }
    private method EventuallyResize { w h }
    private method EventuallyRotate { dx dy }
    private method EventuallySelect { x y }
    private method GetImage { args }
    private method GetNormalizedMouse { x y }
    private method GoToViewpoint { dataobj viewpoint }
    private method InitSettings { args  }
    private method MapIsGeocentric {}
    private method Pan {option x y}
    private method Pin {option x y}
    private method Rebuild {}
    private method ReceiveImage { args }
    private method ReceiveLegend { args }
    private method ReceiveMapInfo { args }
    private method ReceiveScreenInfo { args }
    private method ReceiveSelect { option {args ""} }
    private method RequestLegend { colormap w h }
    private method Rotate {option x y}
    private method Select {option x y}
    private method SendFiles { path }
    private method SendStylesheetFiles { stylesheet }
    private method SetHeading { {value 0} }
    private method SetLayerOpacity { dataobj layer {value 100} }
    private method SetLayerStyle { dataobj layer }
    private method SetLayerVisibility { dataobj layer }
    private method SetPitch { {value -89.999} }
    private method SetTerrainStyle { style }
    private method ToggleGrid {}
    private method ToggleLighting {}
    private method ToggleWireframe {}
    private method UpdateLayerControls {}
    private method UpdateViewpointControls {}
    private method Zoom {option {x 0} {y 0}}

    private variable _layersFrame "";     # Name of layers frame widget
    private variable _viewpointsFrame ""; # Name of viewpoints frame widget
    private variable _mapsettings;        # Global map settings

    private variable _dlist "";         # list of data objects
    private variable _hidden "";        # list of hidden data objects
    private variable _obj2ovride;       # maps dataobj => style override
    private variable _layers;           # Contains the names of all the
                                        # layers in the server.
    private variable _viewpoints;
    private variable _selectCallback "";
    private variable _click;            # info used for rotate operations
    private variable _view;             # view params for 3D view
    private variable _pan;
    private variable _rotate;
    private variable _select;
    private variable _motion;
    private variable _settings
    private variable _opacity
    private variable _visibility
    private variable _reset 1;          # Indicates that server was reset and
                                        # needs to be reinitialized.
    private variable _initCamera 1;

    private variable _first "";         # This is the topmost dataset.
    private variable _start 0
    private variable _title ""

    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _useSidebar 1
    private variable _useServerManip 0
    private variable _labelCount 0
    private variable _b1mode "pan"

    private common _downloadPopup;      # download options from popup
    private common _hardcopy
}

itk::usual MapViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::constructor {args} {
    set _serverType "geovis"
    #DebugOn

    if { [catch {

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Pan event
    $_dispatcher register !pan
    $_dispatcher dispatch $this !pan "[itcl::code $this DoPan]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Select event
    $_dispatcher register !select
    $_dispatcher dispatch $this !select "[itcl::code $this DoSelect]; list"

    # <Motion> event
    $_dispatcher register !motion
    $_dispatcher dispatch $this !motion "[itcl::code $this MouseMotion]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image    [itcl::code $this ReceiveImage]
    $_parser alias legend   [itcl::code $this ReceiveLegend]
    $_parser alias map      [itcl::code $this ReceiveMapInfo]
    $_parser alias camera   [itcl::code $this Camera]
    $_parser alias screen   [itcl::code $this ReceiveScreenInfo]
    $_parser alias select   [itcl::code $this ReceiveSelect]

    # Millisecond delay before animated wait dialog appears
    set _waitTimeout 900

    # Settings for mouse motion events: these are required
    # to update the Lat/Long coordinate display
    array set _motion {
        compress        1
        delay           100
        enable          1
        pending         0
        x               0
        y               0
    }
    array set _pan {
        compress        1
        delay           100
        pending         0
        x               0
        y               0
    }
    array set _rotate {
        azimuth         0
        compress        1
        delay           100
        elevation       0
        pending         0
    }
    array set _select {
        compress        1
        delay           100
        pending         0
        x               0
        y               0
    }
    # This array holds the Viewpoint parameters that the
    # server sends on "camera get".
    array set _view {
        distance        1.0
        heading         0.0
        pitch           -89.9
        srs             ""
        verticalDatum   ""
        x               0.0
        y               0.0
        z               0.0
    }

    # Note: grid types are "shader", "geodetic", "utm" and "mgrs"
    # Currently only work in geocentric maps
    array set _settings [subst {
        camera-throw           0
        coords-precision       5
        coords-units           "latlong_decimal_degrees"
        coords-visible         1
        grid                   0
        grid-type              "shader"
        terrain-ambient        0.03
        terrain-edges          0
        terrain-lighting       0
        terrain-vertscale      1.0
        terrain-wireframe      0
        time                   12
    }]

    set _settings(time) [clock format [clock seconds] -format %k -gmt 1]

    if {!$_useSidebar} {
        destroy $itk_component(main)
        itk_component add main {
            frame $itk_interior.main
        }
        pack $itk_component(main) -expand yes -fill both
        itk_component add plotarea {
            frame $itk_component(main).plotarea -highlightthickness 0 -background black
        } {
            ignore -background
        }
        pack $itk_component(plotarea) -expand yes -fill both
    }

    DebugTrace "main: [winfo class $itk_component(main)]"
    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }

    set c $itk_component(view)
    bind $c <Enter> "focus %W"
    bind $c <Control-F1> [itcl::code $this ToggleConsole]

    # Fix the scrollregion in case we go off screen
    $c configure -scrollregion [$c bbox all]

    set _map(id) [$c create image 0 0 -anchor nw -image $_image(plot)]
    set _map(cwidth) -1
    set _map(cheight) -1
    set _map(zoom) 1.0
    set _map(original) ""

    if {$_useSidebar} {
        set f [$itk_component(main) component controls]
        itk_component add reset {
            button $f.reset -borderwidth 1 -padx 1 -pady 1 \
                -highlightthickness 0 \
                -image [Rappture::icon reset-view] \
                -command [itcl::code $this Camera reset]
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
    }

    if {$_useSidebar} {
        BuildLayerTab
        BuildViewpointsTab
        BuildMapTab
        BuildTerrainTab
        BuildCameraTab
        BuildHelpTab
    }

    # Hack around the Tk panewindow.  The problem is that the requested
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth 10000
    blt::table configure $itk_component(plotarea) c1 -resize none

    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    set _useServerManip 1
    EnablePanningMouseBindings
    EnableRotationMouseBindings
    EnableZoomMouseBindings
    set _useServerManip 0

    if {$_useServerManip} {
        # Bindings for keyboard events
        bind $itk_component(view) <KeyPress> \
            [itcl::code $this KeyPress %N]
        bind $itk_component(view) <KeyRelease> \
            [itcl::code $this KeyRelease %N]

        # Zoom to point
        bind $itk_component(view) <Double-1> \
            [itcl::code $this MouseDoubleClick 1 %x %y]
        bind $itk_component(view) <Double-3> \
            [itcl::code $this MouseDoubleClick 3 %x %y]

        # Unused
        bind $itk_component(view) <Double-2> \
            [itcl::code $this MouseDoubleClick 2 %x %y]

        # Binding for mouse motion events
        if {$_motion(enable)} {
            bind $itk_component(view) <Motion> \
                [itcl::code $this EventuallyHandleMotionEvent %x %y]
        }
    } else {
        # Zoom to point
        bind $itk_component(view) <Double-1> \
            [itcl::code $this Camera go %x %y 0.4]
        # Travel to point (no zoom)
        bind $itk_component(view) <Shift-Double-1> \
            [itcl::code $this Camera go %x %y 1.0]
        # Zoom out centered on point
        bind $itk_component(view) <Double-3> \
            [itcl::code $this Camera go %x %y 2.5]

        # Pin placemark annotations
        bind $itk_component(view) <Control-ButtonPress-1> \
            [itcl::code $this Pin add %x %y]
        bind $itk_component(view) <Control-ButtonPress-3> \
            [itcl::code $this Pin delete %x %y]

        # Draw selection rectangle
        bind $itk_component(view) <Shift-ButtonPress-1> \
            [itcl::code $this Select click %x %y]
        bind $itk_component(view) <B1-Motion> \
            +[itcl::code $this Select drag %x %y]
        bind $itk_component(view) <Shift-ButtonRelease-1> \
            [itcl::code $this Select release %x %y]

        # Update coordinate readout
        bind $itk_component(view) <ButtonPress-1> \
            +[itcl::code $this SendCmd "map setpos %x %y"]
        bind $itk_component(view) <Double-3> \
            +[itcl::code $this SendCmd "map setpos %x %y"]

        # Bindings for panning via keyboard
        bind $itk_component(view) <KeyPress-Left> \
            [itcl::code $this Pan set 10 0]
        bind $itk_component(view) <KeyPress-Right> \
            [itcl::code $this Pan set -10 0]
        bind $itk_component(view) <KeyPress-Up> \
            [itcl::code $this Pan set 0 -10]
        bind $itk_component(view) <KeyPress-Down> \
            [itcl::code $this Pan set 0 10]

        bind $itk_component(view) <Shift-KeyPress-Left> \
            [itcl::code $this Pan set 2 0]
        bind $itk_component(view) <Shift-KeyPress-Right> \
            [itcl::code $this Pan set -2 0]
        bind $itk_component(view) <Shift-KeyPress-Up> \
            [itcl::code $this Pan set 0 -2]
        bind $itk_component(view) <Shift-KeyPress-Down> \
            [itcl::code $this Pan set 0 2]

        # Bindings for rotation via keyboard
        bind $itk_component(view) <Control-Left> \
            [itcl::code $this Rotate set 10 0]
        bind $itk_component(view) <Control-Right> \
            [itcl::code $this Rotate set -10 0]
        bind $itk_component(view) <Control-Up> \
            [itcl::code $this Rotate set 0 -10]
        bind $itk_component(view) <Control-Down> \
            [itcl::code $this Rotate set 0 10]

        bind $itk_component(view) <Control-Shift-Left> \
            [itcl::code $this Rotate set 2 0]
        bind $itk_component(view) <Control-Shift-Right> \
            [itcl::code $this Rotate set -2 0]
        bind $itk_component(view) <Control-Shift-Up> \
            [itcl::code $this Rotate set 0 -2]
        bind $itk_component(view) <Control-Shift-Down> \
            [itcl::code $this Rotate set 0 2]

        # Bindings for zoom via keyboard
        bind $itk_component(view) <KeyPress-Prior> \
            [itcl::code $this Zoom out]
        bind $itk_component(view) <KeyPress-Next> \
            [itcl::code $this Zoom in]
        bind $itk_component(view) <KeyPress-Home> \
            [itcl::code $this Camera reset]

        # Keyboard shortcuts
        # Reset heading to North
        bind $itk_component(view) <n> \
            [itcl::code $this SetHeading]
        # Reset pitch to top-down (2D) view
        bind $itk_component(view) <p> \
            [itcl::code $this SetPitch]
        bind $itk_component(view) <g> \
            [itcl::code $this ToggleGrid]
        bind $itk_component(view) <l> \
            [itcl::code $this ToggleLighting]
        bind $itk_component(view) <w> \
            [itcl::code $this ToggleWireframe]

        # Binding for mouse motion events
        set _motion(compress) 1
        if {$_motion(enable)} {
            bind $itk_component(view) <Motion> \
                [itcl::code $this EventuallyHandleMotionEvent %x %y]
        }
        #bind $itk_component(view) <Motion> \
        #    +[itcl::code $this SendCmd "map pin hover %x %y"]
    }

    bind $itk_component(view) <Enter> "focus $itk_component(view)"

    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        if {$_useServerManip} {
            bind $itk_component(view) <4> [itcl::code $this MouseScroll up]
            bind $itk_component(view) <5> [itcl::code $this MouseScroll down]
        } else {
            bind $itk_component(view) <4> [itcl::code $this Zoom out]
            bind $itk_component(view) <5> [itcl::code $this Zoom in]
        }
    }

    set _image(download) [image create photo]

    eval itk_initialize $args
    Connect
} errs] != 0 } {
        puts stderr errs=$errs
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::destructor {} {
    Disconnect
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    image delete $_image(plot)
    image delete $_image(download)
}

itcl::body Rappture::MapViewer::DoResize {} {
    set sendResize 1
    if { $_width < 2 } {
        set _width 500
        set sendResize 0
    }
    if { $_height < 2 } {
        set _height 500
        set sendResize 0
    }
    #set _start [clock clicks -milliseconds]
    if {$sendResize} {
        SendCmd "screen size $_width $_height"
    }
    set _resizePending 0
}

itcl::body Rappture::MapViewer::DoRotate {} {
    SendCmd "camera rotate $_rotate(azimuth) $_rotate(elevation)"
    set _rotate(azimuth) 0
    set _rotate(elevation) 0
    set _rotate(pending) 0
}

itcl::body Rappture::MapViewer::DoSelect {} {
    SendCmd "map box update $_select(x) $_select(y)"
    set _select(x) 0
    set _select(y) 0
    set _select(pending) 0
}

itcl::body Rappture::MapViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

itcl::body Rappture::MapViewer::DoPan {} {
    SendCmd "camera pan $_pan(x) $_pan(y)"
    set _pan(x) 0
    set _pan(y) 0
    set _pan(pending) 0
}

itcl::body Rappture::MapViewer::EventuallyPan { dx dy } {
    set _pan(x) [expr $_pan(x) + $dx]
    set _pan(y) [expr $_pan(y) + $dy]
    if { !$_pan(compress) } {
        DoPan
        return
    }
    if { !$_pan(pending) } {
        set _pan(pending) 1
        $_dispatcher event -after $_pan(delay) !pan
    }
}

itcl::body Rappture::MapViewer::EventuallyRotate { dx dy } {
    set _rotate(azimuth) [expr $_rotate(azimuth) + $dx]
    set _rotate(elevation) [expr $_rotate(elevation) + $dy]
    if { !$_rotate(compress) } {
        DoRotate
        return
    }
    if { !$_rotate(pending) } {
        set _rotate(pending) 1
        $_dispatcher event -after $_rotate(delay) !rotate
    }
}

itcl::body Rappture::MapViewer::EventuallySelect { x y } {
    set _select(x) $x
    set _select(y) $y
    if { !$_select(compress) } {
        DoSelect
        return
    }
    if { !$_select(pending) } {
        set _select(pending) 1
        $_dispatcher event -after $_select(delay) !select
    }
}

itcl::body Rappture::MapViewer::DrawLegend { colormap min max } {
    if { [info exists itk_component(legend-$colormap) ] } {
        $itk_component(legend-$colormap-min) configure -text $min
        $itk_component(legend-$colormap-max) configure -text $max
        $itk_component(legend-$colormap) configure -image $_image(legend-$colormap)
    }
}

itcl::body Rappture::MapViewer::RequestLegend { colormap w h } {
    SendCmd "legend $colormap $w $h 0 [Color2RGB #d9d9d9]"
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::add {dataobj {settings ""}} {
    DebugTrace "Enter"
    array set params {
        -brightness 0
        -color auto
        -description ""
        -linestyle solid
        -param ""
        -raise 0
        -simulation 0
        -type ""
        -width 1
    }
    array set params $settings
    set params(-description) ""
    set params(-param) ""
    array set params $settings

    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
    }
    # Add to display list
    set pos [lsearch -exact $_dlist $dataobj]
    if {$pos < 0} {
        #if {[llength $_dlist] > 0} {
        #    error "Can't add more than 1 map to mapviewer"
        #}
        lappend _dlist $dataobj
    }
    # Remove from hidden list
    set pos [lsearch -exact $_hidden $dataobj]
    if {$pos >= 0} {
        set _hidden [lreplace $_hidden $pos $pos]
    }
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
itcl::body Rappture::MapViewer::delete {args} {
    DebugTrace "Enter"
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
        # Add to hidden list
        set pos [lsearch -exact $_hidden $dataobj]
        if {$pos < 0} {
            lappend _hidden $dataobj
        }
        array unset _obj2ovride $dataobj-*
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
# USAGE: get ?-image legend <dataobj> <layer>?
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.  The optional "-image"
# flag can also request the internal images being shown.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::get {args} {
    if {[llength $args] == 0} {
        set args "-objects"
    }

    set op [lindex $args 0]
    switch -- $op {
        "-objects" {
            # put the dataobj list in order according to -raise options
            set dlist {}
            foreach dataobj $_dlist {
                if { ![$dataobj isvalid] } {
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
        "-hidden" {
            set dlist {}
            foreach dataobj $_hidden {
                if { [info commands $dataobj] != $dataobj } {
                    # dataobj was deleted, remove from list
                    set pos [lsearch -exact $_hidden $dataobj]
                    if {$pos >= 0} {
                        set _hidden [lreplace $_hidden $pos $pos]
                    }
                    continue
                }
                if { ![$dataobj isvalid] } {
                    continue
                }
                if { [info exists _obj2ovride($dataobj-raise)] } {
                    puts stderr "ERROR: object on hidden list is visible"
                }
                lappend dlist $dataobj
            }
            return $dlist
        }
        "-visible" {
            set dlist {}
            foreach dataobj $_dlist {
                if { ![$dataobj isvalid] } {
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
            if {[llength $args] < 2} {
                error "wrong # args: should be \"get -image view|legend\""
            }
            switch -- [lindex $args 1] {
                legend {
                    if {[llength $args] < 4} {
                        error "wrong # args: should be \"get -image legend <dataobj> <layer>\""
                    }
                    set dataobj [lindex $args 2]
                    set layer [lindex $args 3]
                    set colormap $layer
                    if {![$dataobj layer $layer shared]} {
                        set colormap $dataobj-$layer
                        set colormap "[regsub -all {::} ${colormap} {}]"
                    }
                    return $_image(legend-$colormap)
                }
                view {
                    return $_image(plot)
                }
                default {
                    error "bad image name \"[lindex $args 1]\": should be view|legend"
                }
            }
        }
        default {
            error "bad option \"$op\": should be -objects, -hidden, -visible or -image"
        }
    }
}

itcl::body Rappture::MapViewer::MapIsGeocentric {} {
    if { [info exists _mapsettings(type)] } {
        return [expr {$_mapsettings(type) eq "geocentric"}]
    } else {
        return 0
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
itcl::body Rappture::MapViewer::scale {args} {
    DebugTrace "Enter"
    array unset _mapsettings

    foreach dataobj $args {
        if { ![$dataobj isvalid] } {
            continue
        }
        # Global map view settings are taken from the first dataobj
        array unset settings
        array set settings [$dataobj hints]
        if { [array size _mapsettings] == 0 } {
            set _mapsettings(label) $settings(label)
            set _mapsettings(style) $settings(style)
            set _mapsettings(type) $settings(type)
            set _mapsettings(projection) $settings(projection)
            set _mapsettings(extents) $settings(extents)
            set _mapsettings(camera) $settings(camera)
        }
        # If dataobj has the same type and projection as view, expand extents
        if { $settings(extents) != "" &&
             $settings(type) == $_mapsettings(type) &&
             $settings(projection) == $_mapsettings(projection)} {
            foreach {xmin ymin xmax ymax} $settings(extents) break
            if { $_mapsettings(extents) == $settings(extents) } {
                set _mapsettings(xmin) $xmin
                set _mapsettings(ymin) $ymin
                set _mapsettings(xmax) $xmax
                set _mapsettings(ymax) $ymax
            } else {
                if { $xmin < $_mapsettings(xmin) } {
                    set _mapsettings(xmin) $xmin
                    #set _reset 1
                }
                if { $ymin < $_mapsettings(ymin) } {
                    set _mapsettings(ymin) $ymin
                    #set _reset 1
                }
                if { $xmax > $_mapsettings(xmax) } {
                    set _mapsettings(xmax) $xmax
                    #set _reset 1
                }
                if { $ymax > $_mapsettings(ymax) } {
                    set _mapsettings(ymax) $ymax
                    #set _reset 1
                }
            }
        }
        foreach viewpoint [$dataobj viewpoints] {
            set _viewpoints($viewpoint) [$dataobj viewpoint $viewpoint]
            if {$_debug} {
                array set vp $_viewpoints($viewpoint)
                foreach key { label description x y z distance heading pitch srs verticalDatum } {
                    if { [info exists vp($key)] } {
                        DebugTrace "vp: $viewpoint $key $vp($key)"
                    }
                }
            }
        }
    }
    #if { $_reset } {
    #    $_dispatcher event -idle !rebuild
    #}
}

itcl::body Rappture::MapViewer::setSelectCallback {cmd} {
    set _selectCallback $cmd
}

itcl::body Rappture::MapViewer::DoSelectCallback {option {args ""}} {
    if { $_selectCallback != "" } {
        set cmd [concat $_selectCallback $option $args]
        uplevel #0 $cmd
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveSelect clear
# USAGE: ReceiveSelect feature <args...>
# USAGE: ReceiveSelect annotation <args...>
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::ReceiveSelect {option {args ""}} {
    DebugTrace "Enter"
    eval DoSelectCallback $option $args
}

# ----------------------------------------------------------------------
# USAGE: select clear
# USAGE: select feature <args...>
# USAGE: select annotation <args...>
#
# Clients use this method to notify the map widget of a selection event
# originating from outside the map
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::select {option {args ""}} {
    switch $option {
        "annotation" {
            SendCmd "select annotation $args"
        }
        "clear" {
            SendCmd "select clear"
        }
        "feature" {
            SendCmd "select feature $args"
        }
        default {
            puts stderr "Unknown select option \"$option\""
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
itcl::body Rappture::MapViewer::download {option args} {
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
            set popup .mapviewerdownload
            if { ![winfo exists .mapviewerdownload] } {
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
            set popup .mapviewerdownload
            if {[winfo exists .mapviewerdownload]} {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                "image" {
                    return [$this GetImage [lindex $args 0]]
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
itcl::body Rappture::MapViewer::Connect {} {
    global readyForNextFrame
    set readyForNextFrame 1
    set _reset 1
    set _hosts [GetServerList "geovis"]
    if { "" == $_hosts } {
        return 0
    }
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
            lappend info "client" "mapviewer"
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
#   Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::MapViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::MapViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#   Clients use this method to disconnect from the current rendering
#   server.
#
itcl::body Rappture::MapViewer::Disconnect {} {
    VisViewer::Disconnect

    $_dispatcher cancel !pan
    $_dispatcher cancel !motion
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    # disconnected -- no more data sitting on server
    array unset _layers
    array unset _layersFrame
    global readyForNextFrame
    set readyForNextFrame 1
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -type <type> -token <token> -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::ReceiveImage { args } {
    global readyForNextFrame
    set readyForNextFrame 1
    array set info {
        -bytes 0
        -token "???"
        -type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    if { $info(-type) == "image" } {
        $_image(plot) configure -data $bytes
    } elseif { $info(type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
    set _waitTimeout 0
}

#
# ReceiveLegend
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
#
itcl::body Rappture::MapViewer::ReceiveLegend { colormap min max size } {
    DebugTrace "ReceiveLegend colormap=$colormap range=$min,$max size=$size"
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend-$colormap)] } {
            set _image(legend-$colormap) [image create photo]
        }
        if 0 {
            set f [open "/tmp/legend-${colormap}.ppm" "w"]
            fconfigure $f -translation binary -encoding binary
            puts $f $bytes
            close $f
        }
        $_image(legend-$colormap) configure -data $bytes
        #puts stderr "read $size bytes for [image width $_image(legend-$colormap)]x[image height $_image(legend-$colormap)] legend>"
        if { [catch {DrawLegend $colormap $min $max} errs] != 0 } {
            global errorInfo
            puts stderr "errs=$errs errorInfo=$errorInfo"
        }
    }
}

#
# ReceiveMapInfo --
#
itcl::body Rappture::MapViewer::ReceiveMapInfo { args } {
    if { ![isconnected] } {
        return
    }
    set timeReceived [clock clicks -milliseconds]
    set elapsed [expr $timeReceived - $_start]
    set option [lindex $args 0]
    switch -- $option {
        "coords" {
            set len [llength $args]
            if {$len < 3} {
                error "Bad map coords response"
            } else {
                set token [lindex $args 1]
            }
            foreach { x y z } [lindex $args 2] {
                puts stderr "\[$token\] Map coords: $x $y $z"
            }
            if {$len > 3} {
                set srs [lindex $args 3]
                set vert [lindex $args 4]
                puts stderr "\[$token\] {$srs} {$vert}"
            }
        }
        "names" {
            foreach { name } [lindex $args 1] {
                puts stderr "layer: $name"
            }
        }
        default {
            error "unknown map option \"$option\" from server"
        }
    }
}

#
# ReceiveScreenInfo --
#
itcl::body Rappture::MapViewer::ReceiveScreenInfo { args } {
    if { ![isconnected] } {
        return
    }
    set option [lindex $args 0]
    switch -- $option {
        "coords" {
            set len [llength $args]
            if {$len < 3} {
                error "Bad screen coords response"
            } else {
                set token [lindex $args 1]
            }
            foreach { x y z } [lindex $args 2] {
                puts stderr "\[$token\] Screen coords: $x $y $z"
            }
        }
        default {
            error "unknown screen option \"$option\" from server"
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
itcl::body Rappture::MapViewer::Rebuild {} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $w < 2 || $h < 2 } {
        update idletasks
        $_dispatcher event -idle !rebuild
        return
    }

    # Turn on buffering of commands to the server.  We don't want to be
    # preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).
    StartBufferingCommands

    if { $_reset } {
        set _width $w
        set _height $h
        DoResize

        if { [info exists _mapsettings(type)] } {
            # The map must be reset once before any layers are added This
            # should not be done more than once as it is very expensive.
            if { [info exists _mapsettings(style)] } {
                array set settings {
                    -color white
                }
                array set settings $_mapsettings(style)
            }
            set bgcolor [Color2RGB $settings(-color)]
            if { $_mapsettings(type) == "geocentric" } {
                if { [info exists itk_component(grid)] } {
                    $itk_component(grid) configure -state normal
                }
                if { [info exists itk_component(time)] } {
                    $itk_component(time_l) configure -state normal
                    $itk_component(time) configure -state normal
                }
                if { [info exists itk_component(pitch_slider)] } {
                    $itk_component(pitch_slider_l) configure -state normal
                    $itk_component(pitch_slider) configure -state normal
                }
                EnableRotationMouseBindings
                SendCmd "map reset geocentric $bgcolor"
            }  else {
                if { [info exists itk_component(grid)] } {
                    $itk_component(grid) configure -state disabled
                }
                if { [info exists itk_component(time)] } {
                    $itk_component(time_l) configure -state disabled
                    $itk_component(time) configure -state disabled
                }
                if { [info exists itk_component(pitch_slider)] } {
                    $itk_component(pitch_slider_l) configure -state disabled
                    $itk_component(pitch_slider) configure -state disabled
                }
                DisableRotationMouseBindings
                set proj $_mapsettings(projection)
                SendCmd "screen bgcolor $bgcolor"
                if { $proj == "" } {
                    SendCmd "map reset projected $bgcolor global-mercator"
                } elseif { ![info exists _mapsettings(extents)] ||
                           $_mapsettings(extents) == "" } {
                    SendCmd "map reset projected $bgcolor [list $proj]"
                } else {
                    foreach key "xmin ymin xmax ymax" {
                        set $key $_mapsettings($key)
                    }
                    SendCmd "map reset projected $bgcolor [list $proj] $xmin $ymin $xmax $ymax"
                }
            }
            # XXX: Remove after implementing batch load of layers on reset
            SendCmd "map layer delete base"

            # Most terrain settings are global to the map and apply even
            # if there is no elevation layer.  The exception is the
            # vertical scale, which only applies if there is an elevation
            # layer
            if { [info exists _mapsettings(style)] } {
                SetTerrainStyle $_mapsettings(style)
            } else {
                InitSettings terrain-ambient terrain-edges terrain-lighting \
                    terrain-vertscale terrain-wireframe
            }
            InitSettings coords-visible
        } else {
            error "No map settings on reset"
        }
    }

    set _first ""
    set haveTerrain 0
    foreach dataobj [get -hidden] {
        foreach layer [$dataobj layers] {
            if { ![$dataobj layer $layer shared] } {
                set tag $dataobj-$layer
                SendCmd "map layer visible 0 $tag"
            }
        }
    }
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        foreach layer [$dataobj layers] {
            array unset info
            array set info [$dataobj layer $layer]
            set tag $layer
            if { !$info(shared) } {
                set tag $dataobj-$layer
            }
            if { ![info exists _layers($tag)] } {
                if { $_reportClientInfo }  {
                    set cinfo {}
                    lappend cinfo "tool_id"       [$dataobj hints toolid]
                    lappend cinfo "tool_name"     [$dataobj hints toolname]
                    lappend cinfo "tool_title"    [$dataobj hints tooltitle]
                    lappend cinfo "tool_command"  [$dataobj hints toolcommand]
                    lappend cinfo "tool_revision" [$dataobj hints toolrevision]
                    lappend cinfo "dataset_label" [encoding convertto utf-8 $info(label)]
                    lappend cinfo "dataset_tag"   $tag
                    SendCmd "clientinfo [list $cinfo]"
                }
                set _layers($tag) 1
                SetLayerStyle $dataobj $layer
            }
            # Don't change visibility of shared/base layers
            if { !$info(shared) } {
                # FIXME: This is overriding data layers' initial visibility
                if { [info exists _obj2ovride($dataobj-raise)] } {
                    SendCmd "map layer visible 1 $tag"
                    set _visibility($tag) 1
                } else {
                    SendCmd "map layer visible 0 $tag"
                    set _visibility($tag) 0
                }
            }
            if {$info(type) == "elevation"} {
                set haveTerrain 1
            }
        }
        # Search our layer list for data layers removed from map object
        foreach tag [array names _layers -glob $dataobj-*] {
            set layer [string range $tag [string length "$dataobj-"] end]
            if {![$dataobj hasLayer $layer]} {
                DebugTrace "Delete layer: tag: $tag layer: $layer"
                SendCmd "map layer delete $tag"
                array unset _layers $tag
                array unset _opacity $tag
                array unset _visibility $tag
            }
        }
    }

    if {$_reset} {
        if {$_initCamera} {
            # If this is the first Rebuild, we need to
            # set up the initial view settings if there
            # are any
            Camera reset
            set _initCamera 0
        } else {
            # Restore view from before reconnect
            Camera set all
        }
    }

    if {$_useSidebar} {
        if ($haveTerrain) {
            if { [info exists itk_component(vscale)] } {
                $itk_component(vscale_l) configure -state normal
                $itk_component(vscale) configure -state normal
            }
        } else {
            if { [info exists itk_component(vscale)] } {
                $itk_component(vscale_l) configure -state disabled
                $itk_component(vscale) configure -state disabled
            }
        }
        UpdateLayerControls
        UpdateViewpointControls
    }

    set _reset 0
    global readyForNextFrame
    set readyForNextFrame 0;            # Don't advance to the next frame
                                        # until we get an image.

    # Actually write the commands to the server socket.  If it fails, we
    # don't care.  We're finished here.
    blt::busy hold $itk_component(hull)
    StopBufferingCommands
    blt::busy release $itk_component(hull)
}

itcl::body Rappture::MapViewer::EnablePanningMouseBindings {} {
    if {1 || $_useServerManip} {
        bind $itk_component(view) <ButtonPress-1> \
            [itcl::code $this MouseClick 1 %x %y]
        bind $itk_component(view) <B1-Motion> \
            [itcl::code $this MouseDrag 1 %x %y]
        bind $itk_component(view) <ButtonRelease-1> \
            [itcl::code $this MouseRelease 1 %x %y]
    } else {
        bind $itk_component(view) <ButtonPress-1> \
            [itcl::code $this Pan click %x %y]
        bind $itk_component(view) <B1-Motion> \
            [itcl::code $this Pan drag %x %y]
        bind $itk_component(view) <ButtonRelease-1> \
            [itcl::code $this Pan release %x %y]
    }
}

itcl::body Rappture::MapViewer::DisablePanningMouseBindings {} {
    bind $itk_component(view) <ButtonPress-1> {}
    bind $itk_component(view) <B1-Motion> {}
    bind $itk_component(view) <ButtonRelease-1> {}
}

itcl::body Rappture::MapViewer::EnableRotationMouseBindings {} {
    if {1 || $_useServerManip} {
        # Bindings for rotation via mouse
        bind $itk_component(view) <ButtonPress-2> \
            [itcl::code $this MouseClick 2 %x %y]
        bind $itk_component(view) <B2-Motion> \
            [itcl::code $this MouseDrag 2 %x %y]
        bind $itk_component(view) <ButtonRelease-2> \
            [itcl::code $this MouseRelease 2 %x %y]
    } else {
        bind $itk_component(view) <ButtonPress-2> \
            [itcl::code $this Rotate click %x %y]
        bind $itk_component(view) <B2-Motion> \
            [itcl::code $this Rotate drag %x %y]
        bind $itk_component(view) <ButtonRelease-2> \
            [itcl::code $this Rotate release %x %y]
    }
}

itcl::body Rappture::MapViewer::DisableRotationMouseBindings {} {
    bind $itk_component(view) <ButtonPress-2> {}
    bind $itk_component(view) <B2-Motion> {}
    bind $itk_component(view) <ButtonRelease-2> {}
}

itcl::body Rappture::MapViewer::EnableZoomMouseBindings {} {
    if {1 || $_useServerManip} {
        bind $itk_component(view) <ButtonPress-3> \
            [itcl::code $this MouseClick 3 %x %y]
        bind $itk_component(view) <B3-Motion> \
            [itcl::code $this MouseDrag 3 %x %y]
        bind $itk_component(view) <ButtonRelease-3> \
            [itcl::code $this MouseRelease 3 %x %y]
    } else {
        bind $itk_component(view) <ButtonPress-3> \
            [itcl::code $this Zoom click %x %y]
        bind $itk_component(view) <B3-Motion> \
            [itcl::code $this Zoom drag %x %y]
        bind $itk_component(view) <ButtonRelease-3> \
            [itcl::code $this Zoom release %x %y]
    }
}

itcl::body Rappture::MapViewer::DisableZoomMouseBindings {} {
    bind $itk_component(view) <ButtonPress-3> {}
    bind $itk_component(view) <B3-Motion> {}
    bind $itk_component(view) <ButtonRelease-3> {}
}
# ----------------------------------------------------------------------
# USAGE: CurrentLayers ?-all -visible? ?dataobjs?
#
# Returns a list of server IDs for the current datasets being displayed.
# This is normally a single ID, but it might be a list of IDs if the
# current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::CurrentLayers {args} {
    set flag [lindex $args 0]
    switch -- $flag {
        "-all" {
            if { [llength $args] > 1 } {
                error "CurrentLayers: can't specify dataobj after \"-all\""
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
        foreach layer [$dataobj layers] {
            if { [info exists _layers($layer)] && $_layers($layer) } {
                lappend rlist $layer
            }
        }
    }
    return $rlist
}

itcl::body Rappture::MapViewer::KeyPress {k} {
    SendCmd "key press $k"
}

itcl::body Rappture::MapViewer::KeyRelease {k} {
    SendCmd "key release $k"
}

itcl::body Rappture::MapViewer::GetNormalizedMouse {x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    return [list $x $y]
}

itcl::body Rappture::MapViewer::MouseClick {button x y} {
    SendCmd "mouse click $button $x $y"
}

itcl::body Rappture::MapViewer::MouseDoubleClick {button x y} {
    SendCmd "mouse dblclick $button $x $y"
}

itcl::body Rappture::MapViewer::MouseDrag {button x y} {
    SendCmd "mouse drag $button $x $y"
}

itcl::body Rappture::MapViewer::MouseRelease {button x y} {
    SendCmd "mouse release $button $x $y"
}

itcl::body Rappture::MapViewer::MouseMotion {} {
    SendCmd "mouse motion $_motion(x) $_motion(y)"
    #SendCmd "map pin hover $_motion(x) $_motion(y)"
    set _motion(pending) 0
}

itcl::body Rappture::MapViewer::MouseScroll {direction} {
    switch -- $direction {
        "up" {
            SendCmd "mouse scroll 1"
        }
        "down" {
            SendCmd "mouse scroll -1"
        }
    }
}

#
# EventuallyHandleMotionEvent --
#
#   This routine compresses (no button press) motion events.  It
#   delivers a server mouse command once every 100 milliseconds (if a
#   motion event is pending).
#
itcl::body Rappture::MapViewer::EventuallyHandleMotionEvent {x y} {
    set _motion(x) $x
    set _motion(y) $y
    if { !$_motion(compress) } {
        MouseMotion
        return
    }
    if { !$_motion(pending) } {
        set _motion(pending) 1
        $_dispatcher event -after $_motion(delay) !motion
    }
}

# ----------------------------------------------------------------------
# USAGE: Zoom in
# USAGE: Zoom out
# USAGE: Zoom reset
#        $this Zoom click x y
#        $this Zoom drag x y
#        $this Zoom release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# Also implements mouse zoom.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::Zoom {option {x 0} {y 0}} {
    switch -- $option {
        "in" {
            # z here is normalized mouse Y delta
            set z -0.25
            SendCmd "camera zoom $z"
        }
        "out" {
            # z here is normalized mouse Y delta
            set z 0.25
            SendCmd "camera zoom $z"
        }
        "reset" {
            SendCmd "camera dist $_view(distance)"
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
            set h [winfo height $itk_component(view)]
            set dy [expr ($_click(y) - $y)/double($h)]
            set _click(x) $x
            set _click(y) $y
            if {[expr (abs($dy) > 0.0)]} {
                SendCmd "camera zoom $dy"
            }
        }
        "release" {
            Zoom drag $x $y
            $itk_component(view) configure -cursor ""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rotate click <x> <y>
# USAGE: Rotate drag <x> <y>
# USAGE: Rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::Rotate {option x y} {
    switch -- $option {
        "click" {
            $itk_component(view) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _rotate(azimuth) 0
            set _rotate(elevation) 0
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
                set dx [expr ($x - $_click(x))/double($w)]
                set dy [expr ($_click(y) - $y)/double($h)]
                set _click(x) $x
                set _click(y) $y
                if {[expr (abs($dx) > 0.0 || abs($dy) > 0.0)]} {
                    #SendCmd "camera rotate $dx $dy"
                    EventuallyRotate $dx $dy
                }
            }
        }
        "release" {
            Rotate drag $x $y
            $itk_component(view) configure -cursor ""
            catch {unset _click}
        }
        "set" {
            set w [winfo width $itk_component(view)]
            set h [winfo height $itk_component(view)]
            set dx [expr $x / double($w)]
            set dy [expr $y / double($h)]
            if {[expr (abs($dx) > 0.0 || abs($dy) > 0.0)]} {
                EventuallyRotate $dx $dy
            }
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

itcl::body Rappture::MapViewer::Select {option x y} {
    switch -- $option {
        "click" {
            set _click(x) $x
            set _click(y) $y
            set _b1mode "select"
            SendCmd "map box init $x $y"
        }
        "drag" {
            if {$_b1mode == "select"} {
                EventuallySelect $x $y
            }
        }
        "release" {
            set _b1mode ""
            if {$_click(x) == $x &&
                $_click(y) == $y} {
                SendCmd "map box clear"
            }
        }
    }
}

itcl::body Rappture::MapViewer::Pin {option x y} {
    set _click(x) $x
    set _click(y) $y
    switch -- $option {
        "add" {
            incr _labelCount
            set label "Label $_labelCount"
            SendCmd [list "map" "pin" "add" $x $y [encoding convertto utf-8 $label]]
        }
        "delete" {
            SendCmd "map pin delete $x $y"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: $this Pan set x y
#        $this Pan click x y
#        $this Pan drag x y
#        $this Pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::Pan {option x y} {
    switch -- $option {
        "click" {
            set _click(x) $x
            set _click(y) $y
            set _pan(x) 0
            set _pan(y) 0
            $itk_component(view) configure -cursor hand1
            set _b1mode "pan"
        }
        "drag" {
            if {$_b1mode != "pan"} {
                return
            }
            if { ![info exists _click(x)] } {
                set _click(x) $x
            }
            if { ![info exists _click(y)] } {
                set _click(y) $y
            }
            set w [winfo width $itk_component(view)]
            set h [winfo height $itk_component(view)]
            set dx [expr ($x - $_click(x))/double($w)]
            set dy [expr ($_click(y) - $y)/double($h)]
            set _click(x) $x
            set _click(y) $y
            if {[expr (abs($dx) > 0.0 || abs($dy) > 0.0)]} {
                EventuallyPan $dx $dy
                #SendCmd "camera pan $dx $dy"
            }
        }
        "release" {
            Pan drag $x $y
            $itk_component(view) configure -cursor ""
            set _b1mode ""
        }
        "set" {
            set w [winfo width $itk_component(view)]
            set h [winfo height $itk_component(view)]
            set x [expr $x / double($w)]
            set y [expr $y / double($h)]
            if {[expr (abs($x) > 0.0 || abs($y) > 0.0)]} {
                SendCmd "camera pan $x $y"
            }
            return
        }
        default {
            error "unknown option \"$option\": should set, click, drag, or release"
        }
    }
}

itcl::body Rappture::MapViewer::SetHeading { {value 0} } {
    set _view(heading) $value
    Camera set heading
}

itcl::body Rappture::MapViewer::SetPitch { {value -89.999} } {
    set _view(pitch) $value
    Camera set pitch
}

# ----------------------------------------------------------------------
# USAGE: InitSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::InitSettings { args } {
    foreach setting $args {
        AdjustSetting $setting
    }
}

#
# AdjustSetting --
#
#   Changes/updates a specific setting in the widget.  There are
#   usually user-setable option.  Commands are sent to the render
#   server.
#
itcl::body Rappture::MapViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "coords-visible" - "coords-precision" - "coords-units" {
            set bool $_settings(coords-visible)
            set units $_settings(coords-units)
            set precision $_settings(coords-precision)
            SendCmd "map posdisp $bool $units $precision"
        }
        "grid" - "grid-type" {
            set bool $_settings(grid)
            set gridType $_settings(grid-type)
            SendCmd "map grid $bool $gridType"
        }
        "camera-throw" {
            set bool $_settings($what)
            SendCmd "camera throw $bool"
        }
        "terrain-ambient" {
            set val $_settings($what)
            SendCmd "map terrain ambient $val"
        }
        "terrain-edges" {
            set bool $_settings($what)
            SendCmd "map terrain edges $bool"
        }
        "terrain-lighting" {
            set bool $_settings($what)
            SendCmd "map terrain lighting $bool"
        }
        "terrain-palette" {
            set cmap [$itk_component(terrainpalette) value]
            #SendCmd "map terrain colormap $cmap"
        }
        "terrain-vertscale" {
            set val $_settings($what)
            SendCmd "map terrain vertscale $val"
        }
        "terrain-wireframe" {
            set bool $_settings($what)
            SendCmd "map terrain wireframe $bool"
        }
        "time" {
            set val $_settings($what)
            SendCmd "map time $val"
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::MapViewer::plotbackground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotbackground)]
        SendCmd "screen bgcolor $rgb"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::MapViewer::plotforeground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotforeground)]
        # FIXME: Set font foreground colors
    }
}

itcl::body Rappture::MapViewer::BuildMapTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Map Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    checkbutton $inner.posdisp \
        -text "Show Coordinate Readout" \
        -variable [itcl::scope _settings(coords-visible)] \
        -command [itcl::code $this AdjustSetting coords-visible] \
        -font "Arial 9" -anchor w

    itk_component add grid {
        checkbutton $inner.grid \
        -text "Show Graticule" \
        -variable [itcl::scope _settings(grid)] \
        -command [itcl::code $this AdjustSetting grid] \
        -font "Arial 9" -anchor w
    } {
        ignore -font
    }
    Rappture::Tooltip::for $inner.grid "Toggle graticule (grid) display <g>"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(terrain-wireframe)] \
        -command [itcl::code $this AdjustSetting terrain-wireframe] \
        -font "Arial 9" -anchor w
    Rappture::Tooltip::for $inner.wireframe "Toggle wireframe rendering of terrain geometry <w>"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(terrain-lighting)] \
        -command [itcl::code $this AdjustSetting terrain-lighting] \
        -font "Arial 9" -anchor w
    Rappture::Tooltip::for $inner.lighting "Toggle sky lighting of terrain <l>"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(terrain-edges)] \
        -command [itcl::code $this AdjustSetting terrain-edges] \
        -font "Arial 9" -anchor w

    itk_component add time_l {
        label $inner.time_l -text "Time (UTC)" -font "Arial 9"
    } {
        ignore -font
    }
    itk_component add time {
        ::scale $inner.time -from 0 -to 23.9 -orient horizontal \
            -resolution 0.1 \
            -variable [itcl::scope _settings(time)] \
            -width 10 \
            -showvalue on \
            -command [itcl::code $this AdjustSetting time]
    }

    itk_component add ambient_l {
        label $inner.ambient_l -text "Ambient min." -font "Arial 9"
    } {
        ignore -font
    }
    itk_component add ambient {
        ::scale $inner.ambient -from 0 -to 1.0 -orient horizontal \
            -resolution 0.01 \
            -variable [itcl::scope _settings(terrain-ambient)] \
            -width 10 \
            -showvalue on \
            -command [itcl::code $this AdjustSetting terrain-ambient]
    }

    blt::table $inner \
        0,0 $inner.posdisp   -cspan 2 -anchor w -pady 2 \
        1,0 $inner.grid      -cspan 2 -anchor w -pady 2 \
        2,0 $inner.wireframe -cspan 2 -anchor w -pady 2 \
        3,0 $inner.lighting  -cspan 2 -anchor w -pady 2 \
        4,0 $inner.time_l    -cspan 2 -anchor w -pady 2 \
        4,1 $inner.time      -cspan 2 -fill x   -pady 2 \
        5,0 $inner.ambient_l -cspan 2 -anchor w -pady 2 \
        5,1 $inner.ambient   -cspan 2 -fill x   -pady 2
#        4,0 $inner.edges     -cspan 2  -anchor w -pady 2

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r6 c1 -resize expand
}

itcl::body Rappture::MapViewer::BuildTerrainTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Terrain Settings" \
        -icon [Rappture::icon terrain]]
    $inner configure -borderwidth 4

    label $inner.palette_l -text "Palette" -font "Arial 9" -anchor w
    itk_component add terrainpalette {
        Rappture::Combobox $inner.palette -width 10 -editable no
    }
    $inner.palette choices insert end [GetColormapList]

    $itk_component(terrainpalette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting terrain-palette]

    itk_component add vscale_l {
        label $inner.vscale_l -text "Vertical Scale" -font "Arial 9" -anchor w
    }
    itk_component add vscale {
        ::scale $inner.vscale -from 0 -to 10 -orient horizontal \
            -variable [itcl::scope _settings(terrain-vertscale)] \
            -width 10 \
            -resolution 0.1 \
            -showvalue on \
            -command [itcl::code $this AdjustSetting terrain-vertscale]
    }
    $inner.vscale set $_settings(terrain-vertscale)

    blt::table $inner \
        0,0 $inner.vscale_l  -anchor w -pady 2 \
        0,1 $inner.vscale    -fill x   -pady 2
#        1,0 $inner.palette_l -anchor w -pady 2 \
#        1,1 $inner.palette   -fill x   -pady 2

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r3 c1 -resize expand
}

itcl::body Rappture::MapViewer::BuildLayerTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Layers" \
        -icon [Rappture::icon layers]]
    $inner configure -borderwidth 4
    set f [frame $inner.layers]
    blt::table $inner \
        0,0 $f -fill both
    set _layersFrame $inner
}

itcl::body Rappture::MapViewer::BuildViewpointsTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Places" \
        -icon [Rappture::icon placemark16]]
    $inner configure -borderwidth 4
    set f [frame $inner.viewpoints]
    blt::table $inner \
        0,0 $f -fill both
    set _viewpointsFrame $inner
}

itcl::body Rappture::MapViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set row 0

    set labels { x y z heading pitch distance }
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _view($tag)]
        bind $inner.${tag} <KeyPress-Return> \
            [itcl::code $this Camera set ${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this Camera set ${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }
    set labels { srs verticalDatum }
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _view($tag)]
        bind $inner.${tag} <KeyPress-Return> \
            [itcl::code $this Camera set ${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this Camera set ${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }

    if {0} {
    button $inner.get \
        -text "Get Camera Settings" \
        -font "Arial 9" \
        -command [itcl::code $this SendCmd "camera get"]
    blt::table $inner \
        $row,0 $inner.get -anchor w -pady 2 -cspan 2
    blt::table configure $inner r$row -resize none
    incr row

    button $inner.set \
        -text "Apply Camera Settings" \
        -font "Arial 9" \
        -command [itcl::code $this Camera set all]
    blt::table $inner \
        $row,0 $inner.set -anchor w -pady 2 -cspan 2
    blt::table configure $inner r$row -resize none
    incr row
    }

    if {$_useServerManip} {
        checkbutton $inner.throw \
            -text "Enable Throw" \
            -font "Arial 9" \
            -variable [itcl::scope _settings(camera-throw)] \
            -command [itcl::code $this AdjustSetting camera-throw]
        blt::table $inner \
            $row,0 $inner.throw -anchor w -pady 2 -cspan 2
        blt::table configure $inner r$row -resize none
        incr row
    }

    label $inner.heading_slider_l -text "Heading" -font "Arial 9"
    ::scale $inner.heading_slider -font "Arial 9" \
        -from -180 -to 180 -orient horizontal \
        -variable [itcl::scope _view(heading)] \
        -width 10 \
        -showvalue on \
        -command [itcl::code $this Camera set heading]

    blt::table $inner \
            $row,0 $inner.heading_slider_l -anchor w -pady 2
    blt::table $inner \
            $row,1 $inner.heading_slider -fill x -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    itk_component add pitch_slider_l {
        label $inner.pitch_slider_l -text "Pitch" -font "Arial 9"
    }
    itk_component add pitch_slider {
        ::scale $inner.pitch_slider -font "Arial 9" \
            -from -10 -to -90 -orient horizontal \
            -variable [itcl::scope _view(pitch)] \
            -width 10 \
            -showvalue on \
            -command [itcl::code $this Camera set pitch]
    }

    blt::table $inner \
            $row,0 $inner.pitch_slider_l -anchor w -pady 2
    blt::table $inner \
            $row,1 $inner.pitch_slider -fill x -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c* r* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::MapViewer::BuildHelpTab {} {
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Help" \
        -icon [Rappture::icon question_mark12]]
    $inner configure -borderwidth 4

    set helptext {*************************
Mouse bindings:
*************************
  Left - Panning
  Middle - Rotation
  Right - Zoom

Zoom/travel:
  Left double-click:
    Zoom to point
  Left shift-double:
    Travel to point
  Right double-click:
    Zoom out from point

Pins:
  Ctl-Left: Drop pin
  Ctl-Right: Delete pin

Select:
  Shift-Left click-drag

*************************
Keyboard bindings:
*************************
  g - Toggle graticule
  l - Toggle lighting
  n - Set North up
  p - Reset pitch
  w - Toggle wireframe
  arrows - panning
  Shift-arrows - fine pan
  Ctl-arrows - rotation
  Ctl-Shift-arrows:
    fine rotation
  PgUp/PgDown - zoom
  Home - Reset camera
*************************}

    text $inner.info -width 25 -bg white
    $inner.info insert end $helptext
    $inner.info configure -state disabled
    blt::table $inner \
        0,0 $inner.info -fill both
}

#
# camera
#
# This is the public camera API
#
itcl::body Rappture::MapViewer::camera {option args} {
    switch -- $option {
        "reset" {
            Camera reset
        }
        "zoom" {
            if {[llength $args] < 1} {
                error "wrong # of args to camera zoom"
            }
            set zoomopt [lindex $args 0]
            switch -- $zoomopt {
                "extent" {
                    if {[llength $args] < 5} {
                        error "wrong # of args to camera zoom extent"
                    }
                    foreach {xmin ymin xmax ymax duration srs} [lrange $args 1 end] break
                    foreach key {xmin ymin xmax ymax} {
                        if {![string is double -strict [set $key]]} {
                            error "Invalid extent: $key=[set $key]"
                        }
                    }
                    if {$duration == ""} {
                        set duration 0.0
                    } elseif {![string is double $duration]} {
                        error "Invalid duration \"$duration\", should be a double"
                    }
                    SendCmd "camera extent $xmin $ymin $xmax $ymax $duration $srs"
                }
                "layer" {
                    if {[llength $args] < 3} {
                        error "wrong # of args to camera zoom layer"
                    }
                    foreach {dataobj layer duration} [lrange $args 1 end] break
                    set tag $layer
                    if {![$dataobj layer $layer shared]} {
                        set tag $dataobj-$layer
                    }
                    if {![info exists _layers($tag)]} {
                        error "Unknown layer $layer"
                    }
                    if {$duration == ""} {
                        set duration 0.0
                    } elseif {![string is double $duration]} {
                        error "Invalid duration \"$duration\", should be a double"
                    }
                    SendCmd "camera lextent $tag $duration"
                }
                default {
                    error "Unknown camera zoom option \"$zoomopt\""
                }
            }
        }
        default {
            error "Unknown camera option \"$option\""
        }
    }
}

#
#  Camera --
#
# USAGE: Camera get
#        This is called by the server to transfer the
#        current Viewpoint settings
# USAGE: Camera reset
#        Reset the camera to the default view
#
itcl::body Rappture::MapViewer::Camera {option args} {
    switch -- $option {
        "get" {
            # We got the camera settings from the server
            foreach name {x y z heading pitch distance srs verticalDatum} value $args {
                set _view($name) $value
            }
            #DebugTrace "view: $_view(x), $_view(y), $_view(z), $_view(heading), $_view(pitch), $_view(distance), {$_view(srs)}, {$_view(verticalDatum)}"
        }
        "go" {
            SendCmd "camera go $args"
        }
        "reset" {
            array set _view {
                x               0.0
                y               0.0
                z               0.0
                heading         0.0
                pitch           -89.9
                distance        1.0
                srs             ""
                verticalDatum   ""
            }
            if { [info exists _mapsettings(camera)] } {
                # Check if the tool specified a default
                set view $_mapsettings(camera)
                if { $view != "" } {
                    array set cam $view
                    set duration 0.0
                    if {[info exists cam(xmin)] && [info exists cam(ymin)] &&
                        [info exists cam(xmax)] && [info exists cam(ymax)]} {
                        set srs ""
                        if {[info exists cam(srs)]} {
                            set srs $cam(srs)
                        }
                        SendCmd [list camera extent $cam(xmin) $cam(ymin) $cam(xmax) $cam(ymax) $duration $srs]
                    } else {
                        array set _view $view
                        SendCmd [list camera set $_view(x) $_view(y) $_view(z) $_view(heading) $_view(pitch) $_view(distance) $duration $_view(srs) $_view(verticalDatum)]
                    }
                } else {
                    SendCmd "camera reset"
                    # Retrieve the settings
                    #SendCmd "camera get"
                }
            } else {
                SendCmd "camera reset"
                # Retrieve the settings
                #SendCmd "camera get"
            }
        }
        "set" {
            set who [lindex $args 0]
            if {$who != "all" && $who != "srs" && $who != "verticalDatum"} {
                set val $_view($who)
                set code [catch { string is double $val } result]
                if { $code != 0 || !$result } {
                    return
                }
            }
            switch -- $who {
                "distance" {
                    SendCmd [list camera dist $_view(distance)]
                }
                "all" - "x" - "y" - "z" - "heading" - "pitch" - "srs" - "verticalDatum" {
                    set duration 0.0
                    SendCmd [list camera set $_view(x) $_view(y) $_view(z) $_view(heading) $_view(pitch) $_view(distance) $duration $_view(srs) $_view(verticalDatum)]
                }
            }
        }
    }
}

itcl::body Rappture::MapViewer::GoToViewpoint { dataobj viewpoint } {
    array set view [subst {
        x 0
        y 0
        z 0
        heading 0
        pitch -90
        distance $_view(distance)
        srs "$_view(srs)"
        verticalDatum "$_view(verticalDatum)"
    }]
    array set view [$dataobj viewpoint $viewpoint]
    foreach key {x y z heading pitch distance srs verticalDatum} {
        if { [info exists view($key)] } {
            set _view($key) $view($key)
        }
    }
    # If map is projected, ignore pitch
    if {![MapIsGeocentric]} {
        set _view(pitch) -90
    }
    set duration 2.0
    SendCmd [list camera set $_view(x) $_view(y) $_view(z) $_view(heading) $_view(pitch) $_view(distance) $duration $_view(srs) $_view(verticalDatum)]
}

itcl::body Rappture::MapViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 &&
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::MapViewer::BuildDownloadPopup { popup command } {
    Rappture::Balloon $popup \
        -title "[Rappture::filexfer::label downloadWord] as..."
    set inner [$popup component inner]
    label $inner.summary -text "" -anchor w

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
        2,0 $inner.image_button -anchor w -cspan 2 -padx { 4 0 } \
        4,1 $inner.cancel -width .9i -fill y \
        4,0 $inner.ok -padx 2 -width .9i -fill y
    blt::table configure $inner r3 -height 4
    blt::table configure $inner r4 -pady 4
    raise $inner.image_button
    $inner.image_button invoke
    return $inner
}

itcl::body Rappture::MapViewer::ToggleGrid {} {
    set _settings(grid) [expr !$_settings(grid)]
    AdjustSetting grid
}

itcl::body Rappture::MapViewer::ToggleLighting {} {
    set _settings(terrain-lighting) [expr !$_settings(terrain-lighting)]
    AdjustSetting terrain-lighting
}

itcl::body Rappture::MapViewer::ToggleWireframe {} {
    set _settings(terrain-wireframe) [expr !$_settings(terrain-wireframe)]
    AdjustSetting terrain-wireframe
}

itcl::body Rappture::MapViewer::SetTerrainStyle { style } {
    array set settings {
        -ambient 0.03
        -color white
        -edgecolor black
        -edges 0
        -lighting 1
        -linewidth 1.0
        -vertscale 1.0
        -wireframe 0
    }
    array set settings $style

    SendCmd "map terrain ambient $settings(-ambient)"
    set _settings(terrain-ambient) $settings(-ambient)
    SendCmd "map terrain edges $settings(-edges)"
    set _settings(terrain-edges) $settings(-edges)
    SendCmd "map terrain color [Color2RGB $settings(-color)]"
    #SendCmd "map terrain colormode constant"
    SendCmd "map terrain lighting $settings(-lighting)"
    set _settings(terrain-lighting) $settings(-lighting)
    SendCmd "map terrain linecolor [Color2RGB $settings(-edgecolor)]"
    #SendCmd "map terrain linewidth $settings(-linewidth)"
    SendCmd "map terrain vertscale $settings(-vertscale)"
    set _settings(terrain-vertscale) $settings(-vertscale)
    SendCmd "map terrain wireframe $settings(-wireframe)"
    set _settings(terrain-wireframe) $settings(-wireframe)
}

itcl::body Rappture::MapViewer::SendStylesheetFiles { stylesheet } {
    set files [Rappture::Map::getFilesFromStylesheet $stylesheet]
    foreach file $files {
        SendFiles $file
    }
}

itcl::body Rappture::MapViewer::SendFiles { path } {
    set isRelative [expr {[string first "://" $path] < 0 &&
                          [string index $path 0] != "/"}]
    if {[string range $path 0 7] != "local://" &&
        !$isRelative} {
        return
    }
    DebugTrace "Local path: $path"
    if {!$isRelative} {
        set path [string range $path 8 end]
    }
    set basename [file rootname $path]
    if {[catch {set files [glob -path $basename .*]} err] != 0} {
        puts stderr "File not found: $path"
        return
    }
    foreach file $files {
        set name $file
        set type [file type $file]
        set size [file size $file]
        set f [open $file "r"]
        fconfigure $f -translation binary -encoding binary
        set data [read $f]
        close $f
        SendCmd [list file put $name $type $size]
        SendData $data
    }
}

itcl::body Rappture::MapViewer::SetLayerStyle { dataobj layer } {
    array set info [$dataobj layer $layer]
    set tag $layer
    if { !$info(shared) } {
        set tag $dataobj-$layer
    }
    if { [info exists info(visible)] &&
         !$info(visible) } {
        set _visibility($tag) 0
    } else {
        set _visibility($tag) 1
    }

    switch -- $info(type) {
        "image" {
            array set style {
                -minlevel 0
                -maxlevel 23
                -opacity 1.0
            }
            if { [info exists info(style)] } {
                DebugTrace "layer style: $info(style)"
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
                set _opacity($tag) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            set coverage 0
            if { [info exists info(coverage)] } {
                set coverage $info(coverage)
            }
            switch -- $info(driver) {
                "arcgis" {
                    SendCmd [list map layer add $tag image arcgis \
                                 $info(arcgis.url) $info(cache) $coverage $info(arcgis.token)]
                }
                "colorramp" {
                    set cmapName "[regsub -all {::} ${tag} {}]"
                    SendFiles $info(colorramp.url)
                    SendCmd [list colormap define $cmapName $info(colorramp.colormap)]
                    SendCmd [list map layer add $tag image colorramp \
                                 $info(colorramp.url) $info(cache) $coverage $info(colorramp.elevdriver) $info(profile)  \
                                 $cmapName]
                }
                "debug" {
                    SendCmd [list map layer add $tag image debug]
                }
                "gdal" {
                    SendFiles $info(gdal.url)
                    SendCmd [list map layer add $tag image gdal \
                                 $info(gdal.url) $info(cache) $coverage]
                }
                "tms" {
                    SendCmd [list map layer add $tag image tms \
                                 $info(tms.url) $info(cache) $coverage]
                }
                "wms" {
                    SendCmd [list map layer add $tag image wms \
                                 $info(wms.url) $info(cache) $coverage \
                                 $info(wms.layers) \
                                 $info(wms.format) \
                                 $info(wms.transparent)]
                }
                "xyz" {
                    SendCmd [list map layer add $tag image xyz \
                                 $info(xyz.url) $info(cache) $coverage]
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "elevation" {
            array set style {
                -minlevel 0
                -maxlevel 23
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            switch -- $info(driver)  {
                "gdal" {
                    SendFiles $info(gdal.url)
                    SendCmd [list map layer add $tag elevation gdal \
                                 $info(gdal.url) $info(cache)]
                }
                "tms" {
                    SendCmd [list map layer add $tag elevation tms \
                                 $info(tms.url) $info(cache)]
                }
                "wcs" {
                    SendCmd [list map layer add $tag elevation wcs \
                                 $info(wcs.url) $info(cache) $info(wcs.identifier)]
                }
            }
        }
        "feature" {
            array set style {
                -opacity 1.0
            }
            if { [info exists info(style)] } {
                DebugTrace "layer style: $info(style)"
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            DebugTrace "stylesheet: $info(stylesheet)"
            set script ""
            if { [info exists info(script)] } {
                set script $info(script)
                DebugTrace "script: $script"
            }
            set selectors [list]
            foreach selector [$dataobj selectors $layer] {
                array set sinfo [$dataobj selector $layer $selector]
                DebugTrace "$selector: [array get sinfo]"
                lappend selectors [array get sinfo]
                if {[info exists sinfo(styleExpression)]} {
                    DebugTrace "$selector: $sinfo(styleExpression)"
                } elseif {[info exists sinfo(query)]} {
                    if {[info exists sinfo(queryBounds)]} {
                        foreach {x1 y1 x2 y2} $sinfo(queryBounds) break
                        DebugTrace "queryBounds: xmin $x1 ymin $y1 xmax $x2 ymax $y2"
                    }
                }
            }
            set format ""
            set wfsType ""
            SendStylesheetFiles $info(stylesheet)
            if { [info exists info(ogr.connection)] } {
                set cmd [list map layer add $tag feature db $format $info(ogr.layer) $info(ogr.connection) $info(cache) $info(stylesheet) $script $selectors]
                if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                    lappend cmd $style(-minrange) $style(-maxrange)
                }
            } else {
                set cmd [list map layer add $tag feature $info(driver) $format $wfsType $info(ogr.url) $info(cache) $info(stylesheet) $script $selectors]
                if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                    lappend cmd $style(-minrange) $style(-maxrange)
                }
                SendFiles $info(ogr.url)
            }
            SendCmd $cmd
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "line" {
            array set style {
                -cap "flat"
                -clamping terrain
                -clamptechnique gpu
                -color black
                -join "mitre"
                -minbias 1000
                -opacity 1.0
                -stipplepattern 0
                -stipplefactor 1
                -width 1
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            foreach {r g b} [Color2RGB $style(-color)] {}
            switch -- $info(driver)  {
                "ogr" {
                    SendFiles $info(ogr.url)
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag line ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag line ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique)]
                    }
                }
                "tfs" {
                    set format "json"
                    if {[info exists info(tfs.format)]} {
                        set format $info(tfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag line tfs $format {} $info(tfs.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag line tfs $format {} $info(tfs.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique)]
                    }
                }
                "wfs" {
                    set format "json"
                    if {[info exists info(wfs.format)]} {
                        set format $info(wfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag line wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag line wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $r $g $b $style(-width) $style(-cap) $style(-join) $style(-stipplepattern) $style(-stipplefactor) $style(-clamping) $style(-clamptechnique)]
                    }
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "point" {
            array set style {
                -color black
                -minbias 1000
                -opacity 1.0
                -size 1
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            foreach {r g b} [Color2RGB $style(-color)] {}
            switch -- $info(driver)  {
                "ogr" {
                    SendFiles $info(ogr.url)
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag point ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-size) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag point ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-size)]
                    }
                }
                "tfs" {
                    set format "json"
                    if {[info exists info(tfs.format)]} {
                        set format $info(tfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag point tfs $format {} $info(ogr.url) $info(cache) $r $g $b $style(-size) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag point tfs $format {} $info(ogr.url) $info(cache) $r $g $b $style(-size)]
                    }
                }
                "wfs" {
                    set format "json"
                    if {[info exists info(wfs.format)]} {
                        set format $info(wfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag point wfs $format $info(wfs.typename) $info(ogr.url) $info(cache) $r $g $b $style(-size) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag point wfs $format $info(wfs.typename) $info(ogr.url) $info(cache) $r $g $b $style(-size)]
                    }
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "icon" {
            array set style {
                -align "center_bottom"
                -declutter 1
                -heading {}
                -icon pin
                -minbias 1000
                -opacity 1.0
                -placement "vertex"
                -scale {}
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            switch -- $info(driver)  {
                "ogr" {
                    SendFiles $info(ogr.url)
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag icon ogr {} {} $info(ogr.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag icon ogr {} {} $info(ogr.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align)]
                    }
                }
                "tfs" {
                    set format "json"
                    if {[info exists info(tfs.format)]} {
                        set format $info(tfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag icon tfs $format {} $info(tfs.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag icon tfs $format {} $info(tfs.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align)]
                    }
                }
                "wfs" {
                    set format "json"
                    if {[info exists info(wfs.format)]} {
                        set format $info(wfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag icon wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag icon wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $style(-icon) $style(-scale) $style(-heading) $style(-declutter) $style(-placement) $style(-align)]
                    }
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "polygon" {
            array set style {
                -clamping terrain
                -clamptechnique drape
                -color white
                -minbias 1000
                -opacity 1.0
                -strokecolor black
                -strokewidth 0.0
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            foreach {r g b} [Color2RGB $style(-color)] {}
            foreach {strokeR strokeG strokeB} [Color2RGB $style(-strokecolor)] {}
            switch -- $info(driver)  {
                "ogr" {
                    SendFiles $info(ogr.url)
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag polygon ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag polygon ogr {} {} $info(ogr.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique)]
                    }
                }
                "tfs" {
                    set format "json"
                    if {[info exists info(tfs.format)]} {
                        set format $info(tfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag polygon tfs $format {} $info(tfs.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag polygon tfs $format {} $info(tfs.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique)]
                    }
                }
                "wfs" {
                    set format "json"
                    if {[info exists info(wfs.format)]} {
                        set format $info(wfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag polygon wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag polygon wfs $format $info(wfs.typename) $info(wfs.url) $info(cache) $r $g $b $style(-strokewidth) $strokeR $strokeG $strokeB $style(-clamping) $style(-clamptechnique)]
                    }
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
        "label" {
            array set style {
                -align "left_baseline"
                -color black
                -declutter 1
                -font Arial
                -fontsize 16.0
                -halocolor white
                -halowidth 2.0
                -layout "left_to_right"
                -minbias 1000
                -opacity 1.0
                -removedupes 1
                -xoffset 0
                -yoffset 0
            }
            if { [info exists info(style)] } {
                array set style $info(style)
            }
            if { [info exists info(opacity)] } {
                set style(-opacity) $info(opacity)
            }
            set _opacity($tag) [expr $style(-opacity) * 100]
            set contentExpr $info(content)
            if {[info exists info(priority)]} {
                set priorityExpr $info(priority)
            } else {
                set priorityExpr ""
            }
            foreach {fgR fgG fgB} [Color2RGB $style(-color)] {}
            foreach {bgR bgG bgB} [Color2RGB $style(-halocolor)] {}
            switch -- $info(driver)  {
                "ogr" {
                    SendFiles $info(ogr.url)
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag text ogr {} {} $info(ogr.url) $info(cache) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag text ogr {} {} $info(ogr.url) $info(cache) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset)]
                    }
                }
                "tfs" {
                    set format "json"
                    if {[info exists info(tfs.format)]} {
                        set format $info(tfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag text tfs $format {} $info(tfs.url) $info(cache) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag text tfs $format {} $info(tfs.url) $info(cache) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset)]
                    }
                }
                "wfs" {
                    set format "json"
                    if {[info exists info(wfs.format)]} {
                        set format $info(wfs.format)
                    }
                    if {[info exists style(-minrange)] && [info exists style(-maxrange)]} {
                        SendCmd [list map layer add $tag text wfs $format $info(wfs.typename) $info(wfs.url) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset) $style(-minrange) $style(-maxrange)]
                    } else {
                        SendCmd [list map layer add $tag text wfs $format $info(wfs.typename) $info(wfs.url) $contentExpr $priorityExpr $fgR $fgG $fgB $bgR $bgG $bgB $style(-halowidth) $style(-fontsize) $style(-removedupes) $style(-declutter) $style(-align) $style(-xoffset) $style(-yoffset)]
                    }
                }
            }
            SendCmd "map layer opacity $style(-opacity) $tag"
        }
    }

    if {[info exists info(placard)]} {
        if {$info(type) == "image" || $info(type) == "elevation"} {
            error "Placard not supported on image or elevation layers"
        }
        array set placard [$dataobj getPlacardConfig $layer]
        SendCmd [list placard config $placard(attrlist) $placard(style) $placard(padding) $tag]
    }

    SendCmd "map layer visible $_visibility($tag) $tag"
}

itcl::body Rappture::MapViewer::SetLayerOpacity { dataobj layer {value 100}} {
    set tag $layer
    if {![$dataobj layer $layer shared]} {
        set tag $dataobj-$layer
    }
    set val $_opacity($tag)
    set sval [expr { 0.01 * double($val) }]
    SendCmd "map layer opacity $sval $tag"
}

itcl::body Rappture::MapViewer::SetLayerVisibility { dataobj layer } {
    set tag $layer
    if {![$dataobj layer $layer shared]} {
        set tag $dataobj-$layer
    }
    set bool $_visibility($tag)
    SendCmd "map layer visible $bool $tag"
}

itcl::body Rappture::MapViewer::UpdateLayerControls {} {
    set row 0
    set inner $_layersFrame
    if { [winfo exists $inner.layers] } {
        foreach w [winfo children $inner.layers] {
            destroy $w
        }
    }
    set f $inner.layers
    set attrib [list]
    set imgIdx 0
    # FIXME: This order may not match stacking order in server
    foreach dataobj [get -objects] {
        foreach layer [$dataobj layers] {
            array unset info
            array set info [$dataobj layer $layer]
            set tag $layer
            set ctlname $layer
            if {!$info(shared)} {
                set tag $dataobj-$layer
                set ctlname "[regsub -all {::} ${tag} {}]"
            }
            checkbutton $f.${ctlname}_visible \
                -text $info(label) \
                -font "Arial 9" -anchor w \
                -variable [itcl::scope _visibility($tag)] \
                -command [itcl::code $this \
                              SetLayerVisibility $dataobj $layer]
            blt::table $f $row,0 $f.${ctlname}_visible -anchor w -pady 2 -cspan 2
            incr row
            if { $info(type) == "image" } {
                incr imgIdx
                if { $info(driver) == "colorramp" } {
                    set colormap $ctlname
                    if { ![info exists _image(legend-$colormap)] } {
                        set _image(legend-$colormap) [image create photo]
                    }
                    itk_component add legend-$colormap-min {
                        label $f.legend-$colormap-min -text 0
                    }
                    itk_component add legend-$colormap-max {
                        label $f.legend-$colormap-max -text 1
                    }
                    itk_component add legend-$colormap {
                        label $f.legend-$colormap -image $_image(legend-$colormap)
                    }
                    blt::table $f $row,0 $f.legend-$colormap-min -anchor w -pady 0
                    blt::table $f $row,1 $f.legend-$colormap-max -anchor e -pady 0
                    incr row
                    blt::table $f $row,0 $f.legend-$colormap -anchor w -pady 2 -cspan 2
                    incr row
                    RequestLegend $colormap 256 16
                }
            }
            if { $info(type) != "elevation" &&
                ($info(type) != "image" || $imgIdx > 1) } {
                label $f.${ctlname}_opacity_l -text "Opacity" -font "Arial 9"
                ::scale $f.${ctlname}_opacity -from 0 -to 100 \
                    -orient horizontal -showvalue off \
                    -variable [itcl::scope _opacity($tag)] \
                    -width 10 \
                    -command [itcl::code $this \
                                  SetLayerOpacity $dataobj $layer]
                Rappture::Tooltip::for $f.${ctlname}_opacity "Set opacity of $info(label) layer"
                blt::table $f $row,0 $f.${ctlname}_opacity_l -anchor w -pady 2
                blt::table $f $row,1 $f.${ctlname}_opacity -anchor w -pady 2
                incr row
            }
            set tooltip [list $info(description)]
            if { [info exists info(attribution)] &&
                 $info(attribution) != ""} {
                lappend tooltip $info(attribution)
            }
            Rappture::Tooltip::for $f.${ctlname}_visible [join $tooltip \n]
        }
        set mapAttrib [$dataobj hints "attribution"]
        if { $mapAttrib != "" } {
            lappend attrib $mapAttrib
        }
    }
    SendCmd "[list map attrib [encoding convertto utf-8 [join $attrib ,]]]"
    label $f.map_attrib -text [join $attrib \n] -font "Arial 9"
    blt::table $f $row,0 $f.map_attrib -anchor sw -pady 2 -cspan 2
    #incr row
    if { $row > 0 } {
        blt::table configure $f r* c* -resize none
        blt::table configure $f r$row c1 -resize expand
    }
}

itcl::body Rappture::MapViewer::UpdateViewpointControls {} {
    set row 0
    set inner $_viewpointsFrame
    if { [winfo exists $inner.viewpoints] } {
        foreach w [winfo children $inner.viewpoints] {
            destroy $w
        }
    }
    set f $inner.viewpoints
    foreach dataobj [get -objects] {
        foreach viewpoint [$dataobj viewpoints] {
            array unset info
            array set info [$dataobj viewpoint $viewpoint]
            button $f.${viewpoint}_go \
                -relief flat -compound left \
                -image [Rappture::icon placemark16] \
                -text $info(label) \
                -font "Arial 9" -anchor w \
                -command [itcl::code $this \
                              GoToViewpoint $dataobj $viewpoint]
            label $f.${viewpoint}_label \
                -text $info(label) \
                -font "Arial 9" -anchor w
            blt::table $f $row,0 $f.${viewpoint}_go -anchor w -pady 2 -cspan 2
            #blt::table $f $row,1 $f.${viewpoint}_label -anchor w -pady 2
            Rappture::Tooltip::for $f.${viewpoint}_go $info(description)
            incr row
        }
    }
    if { $row > 0 } {
        blt::table configure $f r* c* -resize none
        blt::table configure $f r$row c1 -resize expand
    }
}
