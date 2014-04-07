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

    private variable _layersFrame "";	# Name of layers frame widget
    private variable _mapsettings;      # Global map settings

    constructor { hostlist args } {
        Rappture::VisViewer::constructor $hostlist
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
    public method limits { colormap }
    public method parameters {title args} { 
        # do nothing 
    }
    public method scale {args}

    protected method AdjustSetting {what {value ""}}
    protected method Connect {}
    protected method CurrentLayers {args}
    protected method Disconnect {}
    protected method DoResize {}
    protected method DoRotate {}
    protected method FixSettings { args  }
    protected method KeyPress { key }
    protected method KeyRelease { key }
    protected method MouseClick { button x y }
    protected method MouseDoubleClick { button x y }
    protected method MouseDrag { button x y }
    protected method MouseMotion {}
    protected method MouseRelease { button x y }
    protected method MouseScroll { direction }
    protected method Pan {option x y}
    protected method Rebuild {}
    protected method ReceiveMapInfo { args }
    protected method ReceiveImage { args }
    protected method Rotate {option x y}
    protected method Zoom {option {x 0} {y 0}}

    # The following methods are only used by this class.
    private method BuildCameraTab {}
    private method BuildDownloadPopup { widget command } 
    private method BuildLayerTab {}
    private method BuildTerrainTab {}
    private method ChangeLayerVisibility { dataobj layer }
    private method EventuallyHandleMotionEvent { x y } 
    private method EventuallyResize { w h } 
    private method EventuallyRotate { dx dy } 
    private method GetImage { args } 
    private method GetNormalizedMouse { x y }
    private method MapIsGeocentric {}
    private method SetLayerStyle { dataobj layer }
    private method SetTerrainStyle { style }
    private method SetOpacity { dataset }
    private method UpdateLayerControls {}
    private method EarthFile {}

    private variable _dlist "";		# list of data objects
    private variable _obj2datasets
    private variable _obj2ovride;	# maps dataobj => style override
    private variable _layers;		# Contains the names of all the 
                                   	# layer in the server.
    private variable _click;            # info used for rotate operations
    private variable _view;             # view params for 3D view
    private variable _settings
    private variable _visibility
    private variable _style;            # Array of current component styles.
    private variable _initialStyle;     # Array of initial component styles.
    private variable _reset 1;          # Indicates that server was reset and
                                        # needs to be reinitialized.
    private variable _initCamera 1;
    private variable _haveTerrain 0

    private variable _first ""     ;# This is the topmost dataset.
    private variable _start 0
    private variable _title ""

    common _downloadPopup          ;# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _rotatePending 0
    private variable _rotateDelay 150
    private variable _motion 
    private variable _sendEarthFile 0
    private variable _useServerManip 0
}

itk::usual MapViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::constructor {hostlist args} {
    set _serverType "geovis"

    if { [catch {
	
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # <Motion> event
    $_dispatcher register !motion
    $_dispatcher dispatch $this !motion "[itcl::code $this MouseMotion]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image    [itcl::code $this ReceiveImage]
    $_parser alias map      [itcl::code $this ReceiveMapInfo]
    $_parser alias camera   [itcl::code $this camera]

    # Settings for mouse motion events: these are required
    # to update the Lat/Long coordinate display
    array set _motion {
        x               0
        y               0
        pending         0
        delay           100
        compress        0
    }
    # This array holds the Viewpoint parameters that the
    # server sends on "camera get".
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

    # Note: grid types are "geodetic", "utm" and "mgrs"
    # Currently only work in geocentric maps
    array set _settings [subst {
        camera-throw           0
        grid                   0
        grid-type              "geodetic"
        legend                 1
        terrain-edges          0
        terrain-lighting       0
        terrain-vertscale      1.0
        terrain-wireframe      0
    }]
    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }

    set c $itk_component(view)
    #bind $c <KeyPress-Left>  [list %W xview scroll 10 units]
    #bind $c <KeyPress-Right> [list %W xview scroll -10 units]
    #bind $c <KeyPress-Up>    [list %W yview scroll 10 units]
    #bind $c <KeyPress-Down>  [list %W yview scroll -10 units]
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
            -command [itcl::code $this camera reset]
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

    BuildLayerTab
    BuildTerrainTab
    BuildCameraTab

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

    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    if {$_useServerManip} {
        # Bindings for keyboard events
        bind $itk_component(view) <KeyPress> \
            [itcl::code $this KeyPress %N]
        bind $itk_component(view) <KeyRelease> \
            [itcl::code $this KeyRelease %N]

        # Bindings for rotation via mouse
        bind $itk_component(view) <ButtonPress-1> \
            [itcl::code $this MouseClick 1 %x %y]
        bind $itk_component(view) <Double-1> \
            [itcl::code $this MouseDoubleClick 1 %x %y]
        bind $itk_component(view) <B1-Motion> \
            [itcl::code $this MouseDrag 1 %x %y]
        bind $itk_component(view) <ButtonRelease-1> \
            [itcl::code $this MouseRelease 1 %x %y]

        # Bindings for panning via mouse
        bind $itk_component(view) <ButtonPress-2> \
            [itcl::code $this MouseClick 2 %x %y]
        bind $itk_component(view) <Double-2> \
            [itcl::code $this MouseDoubleClick 2 %x %y]
        bind $itk_component(view) <B2-Motion> \
            [itcl::code $this MouseDrag 2 %x %y]
        bind $itk_component(view) <ButtonRelease-2> \
            [itcl::code $this MouseRelease 2 %x %y]

        bind $itk_component(view) <ButtonPress-3> \
            [itcl::code $this MouseClick 3 %x %y]
        bind $itk_component(view) <Double-3> \
            [itcl::code $this MouseDoubleClick 3 %x %y]
        bind $itk_component(view) <B3-Motion> \
            [itcl::code $this MouseDrag 3 %x %y]
        bind $itk_component(view) <ButtonRelease-3> \
            [itcl::code $this MouseRelease 3 %x %y]

        bind $itk_component(view) <Motion> \
            [itcl::code $this EventuallyHandleMotionEvent %x %y]
    } else {
        # Bindings for panning via mouse
        bind $itk_component(view) <ButtonPress-1> \
            [itcl::code $this Pan click %x %y]
        bind $itk_component(view) <B1-Motion> \
            [itcl::code $this Pan drag %x %y]
        bind $itk_component(view) <ButtonRelease-1> \
            [itcl::code $this Pan release %x %y]
        bind $itk_component(view) <Double-1> \
            [itcl::code $this camera go %x %y 0.4]

        # Bindings for rotation via mouse
        bind $itk_component(view) <ButtonPress-2> \
            [itcl::code $this Rotate click %x %y]
        bind $itk_component(view) <B2-Motion> \
            [itcl::code $this Rotate drag %x %y]
        bind $itk_component(view) <ButtonRelease-2> \
            [itcl::code $this Rotate release %x %y]

        # Bindings for zoom via mouse
        bind $itk_component(view) <ButtonPress-3> \
            [itcl::code $this Zoom click %x %y]
        bind $itk_component(view) <B3-Motion> \
            [itcl::code $this Zoom drag %x %y]
        bind $itk_component(view) <ButtonRelease-3> \
            [itcl::code $this Zoom release %x %y]
        bind $itk_component(view) <Double-3> \
            [itcl::code $this camera go %x %y 2.5]

        # Bindings for panning via keyboard
        bind $itk_component(view) <KeyPress-Left> \
            [itcl::code $this Pan set 10 0]
        bind $itk_component(view) <KeyPress-Right> \
            [itcl::code $this Pan set -10 0]
        bind $itk_component(view) <KeyPress-Up> \
            [itcl::code $this Pan set 0 -10]
        bind $itk_component(view) <KeyPress-Down> \
            [itcl::code $this Pan set 0 10]

        # Send (compressed) motion events to update Lat/Long
        set _motion(compress) 1
        bind $itk_component(view) <Motion> \
            [itcl::code $this EventuallyHandleMotionEvent %x %y]
    }

    bind $itk_component(view) <Shift-KeyPress-Left> \
        [itcl::code $this Pan set 2 0]
    bind $itk_component(view) <Shift-KeyPress-Right> \
        [itcl::code $this Pan set -2 0]
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
    set _start [clock clicks -milliseconds]
    if {$sendResize} {
        SendCmd "screen size $_width $_height"
    }
    set _resizePending 0
}

itcl::body Rappture::MapViewer::DoRotate {} {
    SendCmd "camera rotate $_view(azimuth) $_view(elevation)"
    set _rotatePending 0
}

itcl::body Rappture::MapViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

itcl::body Rappture::MapViewer::EventuallyRotate { dx dy } {
    set _view(azimuth) $dx
    set _view(elevation) $dy
    if { !$_rotatePending } {
        set _rotatePending 1
        $_dispatcher event -after $_rotateDelay !rotate
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::add {dataobj {settings ""}} {
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
        #if {[llength $_dlist] > 0} {
        #    error "Can't add more than 1 map to mapviewer"
        #}
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
itcl::body Rappture::MapViewer::delete {args} {
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
        # When a map is marked deleted, we hide its layers.
        foreach layer [$dataobj layers] {
            SendCmd "map layer visible 0 $layer"
            set _visibility($layer) 0
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
    array unset _mapsettings
    set _haveTerrain 0

    # Verify that all the maps have the same global settings. For example,
    # you can't have one map type "geocentric" and the other "projected".

    foreach dataobj $args {
        if { ![$dataobj isvalid] } {
            continue
        }
        array unset hints 
        array set hints [$dataobj hints]
        if { ![info exists _mapsettings(label)] } {
            set _mapsettings(label) $hints(label)
        }
        if { ![info exists _mapsettings(style)] } {
            set _mapsettings(style) $hints(style)
        }
        if { ![info exists _mapsettings(type)] } {
            set _mapsettings(type) $hints(type)
        } elseif { $hints(type) != $_mapsettings(type) } {
            error "maps \"$hints(label)\" have differing types"
        }
        if { ![info exists _mapsettings(projection)] } {
            set _mapsettings(projection) $hints(projection)
        } elseif { $hints(projection) != $_mapsettings(projection) } {
            error "maps \"$hints(label)\" have differing projections"
        }
        if { $hints(extents) != "" } {
            if { ![info exists _mapsettings(extents)] } {
                set _mapsettings(extents) $hints(extents)
            }
            foreach {x1 y1 x2 y2} $hints(extents) break
            if { ![info exists _mapsettings(x1)] || $x1 < $_mapsettings(x1) } {
                set _mapsettings(x1) $x1
            }
            if { ![info exists _mapsettings(y1)] || $y1 < $_mapsettings(y1) } {
                set _mapsettings(y1) $y1
            }
            if { ![info exists _mapsettings(x2)] || $x2 > $_mapsettings(x2) } {
                set _mapsettings(x2) $x2
            }
            if { ![info exists _mapsettings(y2)] || $y2 > $_mapsettings(y2) } {
                set _mapsettings(y2) $y2
            }
        }
        foreach layer [$dataobj layers] {
            if { [$dataobj type $layer] == "elevation" } {
                set _haveTerrain 1
                break
            } 
        }
    }
    if { $_haveTerrain } {
        if { [$itk_component(main) exists "Terrain Settings"] } {
            # TODO: Enable controls like vertical scale that only have
            # an effect when terrain is present
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
#       Indicates if we are currently connected to the visualization server.
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
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::MapViewer::Disconnect {} {
    VisViewer::Disconnect

    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    $_dispatcher cancel !motion
    # disconnected -- no more data sitting on server
    array unset _layers
    array unset _layersFrame 
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
itcl::body Rappture::MapViewer::ReceiveImage { args } {
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
        $_image(plot) configure -data $bytes
    } elseif { $info(type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
}

#
# ReceiveMapInfo --
#
itcl::body Rappture::MapViewer::ReceiveMapInfo { args } {
    if { ![isconnected] } {
        return
    }
    set option [lindex $args 0]
    switch -- $option {
        "coords" {
            set len [llength $args]
            if {$len < 4} {
                puts stderr "Coords out of range"
            } elseif {$len < 6} {
                foreach { x y z } [lrange $args 1 end] break
                puts stderr "Coords: $x $y $z"
            } else {
                foreach { x y z pxX pxY } [lrange $args 1 end] break
                puts stderr "Coords($pxX,$pxY): $x $y $z"
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
            if {$_sendEarthFile} {
                set bytes [EarthFile]
                if {0} {
                    set f [open "/tmp/map.earth" "w"]
                    puts $f $bytes
                    close $f
                }
                set length [string length $bytes]
                SendCmd "map load data follows $length"
                append _outbuf $bytes
            } else {
                if { $_mapsettings(type) == "geocentric" } {
                    SendCmd "map reset geocentric"
                }  else {
                    set proj $_mapsettings(projection)
                    if { $proj == "" } { 
                        SendCmd "map reset projected global-mercator"
                    } elseif { ![info exists _mapsettings(extents)] || $_mapsettings(extents) == "" } {
                        SendCmd [list map reset "projected" $proj]
                    } else {
                        #foreach {x1 y1 x2 y2} $_mapsettings(extents) break
                        foreach key "x1 y1 x2 y2" {
                            set $key $_mapsettings($key)
                        }
                        SendCmd [list map reset "projected" $proj $x1 $y1 $x2 $y2] 
                    }
                }
                # XXX: Remove these after implementing batch load of layers with reset
                SendCmd "map layer delete base"
            }

            # Most terrain settings are global to the map and apply even
            # if there is no elevation layer.  The exception is the 
            # vertical scale, which only applies if there is an elevation
            # layer
            if { [info exists _mapsettings(style)] } {
                SetTerrainStyle $_mapsettings(style)
            } else {
                FixSettings terrain-edges terrain-lighting \
                    terrain-vertscale terrain-wireframe
            }
        } else {
            error "No map settings on reset"
        }
    }

    set _first ""
    set count 0

    foreach dataobj [get -objects] {
        set _obj2datasets($dataobj) ""
        foreach layer [$dataobj layers] {
	    array unset info
	    array set info [$dataobj layer $layer]
            if { ![info exists _layers($layer)] } {
		if { ![info exists info(url)] }  {
		    continue
		}
                if { $_reportClientInfo }  {
                    set cinfo {}
                    lappend cinfo "tool_id"       [$dataobj hints toolId]
                    lappend cinfo "tool_name"     [$dataobj hints toolName]
                    lappend cinfo "tool_version"  [$dataobj hints toolRevision]
                    lappend cinfo "tool_title"    [$dataobj hints toolTitle]
                    lappend cinfo "dataset_label" [$dataobj hints label]
                    lappend cinfo "dataset_tag"   $layer
                    SendCmd [list "clientinfo" $cinfo]
                }
                set _layers($layer) 1
                SetLayerStyle $dataobj $layer
            }
            lappend _obj2datasets($dataobj) $layer
            # FIXME: This is overriding all layers' initial visibility setting
            if { [info exists _obj2ovride($dataobj-raise)] } {
                SendCmd "map layer visible 1 $layer"
                set _visibility($layer) 1
                #SetLayerOpacity $layer
            }
        }
    }

    if {$_reset} {
        if {$_initCamera} {
            # If this is the first Rebuild, we need to
            # set up the initial view settings if there
            # are any
            if { [info exists _mapsettings(camera)] } {
                set location $_mapsettings(camera)
                if { $location != "" } {
                    array set _view $location
                    camera set all
                }
            }
            set _initCamera 0
        } else {
            # Restore view from before reconnect
            camera set all
        }
    }

    UpdateLayerControls
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
#       This routine compresses (no button press) motion events.  It
#       delivers a server mouse command once every 100 milliseconds (if a
#       motion event is pending).
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
                    SendCmd "camera rotate $dx $dy"
                    #EventuallyRotate $dx $dy
                }
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
            set dx [expr ($x - $_click(x))/double($w)]
            set dy [expr ($_click(y) - $y)/double($h)]
            set _click(x) $x
            set _click(y) $y
            if {[expr (abs($dx) > 0.0 || abs($dy) > 0.0)]} {
                SendCmd "camera pan $dx $dy"
            }
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
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::FixSettings { args } {
    foreach setting $args {
        AdjustSetting $setting
    }
}

#
# AdjustSetting --
#
#       Changes/updates a specific setting in the widget.  There are
#       usually user-setable option.  Commands are sent to the render
#       server.
#
itcl::body Rappture::MapViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "grid" - "grid-type" {
            set bool $_settings(grid)
            set gridType $_settings(grid-type)
            SendCmd "map grid $bool $gridType"
        }
        "camera-throw" {
            set bool $_settings(camera-throw)
            SendCmd "camera throw $bool"
        }
        "terrain-edges" {
            set bool $_settings(terrain-edges)
            SendCmd "map terrain edges $bool"
        }
        "terrain-lighting" {
            set bool $_settings(terrain-lighting)
            SendCmd "map terrain lighting $bool"
        }
        "terrain-palette" {
            set cmap [$itk_component(terrainpalette) value]
            #SendCmd "map terrain colormap $cmap"
        }
        "terrain-vertscale" {
            set val $_settings(terrain-vertscale)
            SendCmd "map terrain vertscale $val"
        }
        "terrain-wireframe" {
            set bool $_settings(terrain-wireframe)
            SendCmd "map terrain wireframe $bool"
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
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::MapViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::MapViewer::limits { dataobj } {
    error "no limits"
}

itcl::body Rappture::MapViewer::BuildTerrainTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Terrain Settings" \
        -icon [Rappture::icon surface]]
    $inner configure -borderwidth 4

    checkbutton $inner.grid \
        -text "Show Graticule" \
        -variable [itcl::scope _settings(grid)] \
        -command [itcl::code $this AdjustSetting grid] \
        -font "Arial 9" -anchor w 

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(terrain-wireframe)] \
        -command [itcl::code $this AdjustSetting terrain-wireframe] \
        -font "Arial 9" -anchor w 

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(terrain-lighting)] \
        -command [itcl::code $this AdjustSetting terrain-lighting] \
        -font "Arial 9" -anchor w

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(terrain-edges)] \
        -command [itcl::code $this AdjustSetting terrain-edges] \
        -font "Arial 9" -anchor w

    label $inner.palette_l -text "Palette" -font "Arial 9" -anchor w 
    itk_component add terrainpalette {
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

    $itk_component(terrainpalette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting terrain-palette]

    label $inner.vscale_l -text "Vertical Scale" -font "Arial 9" -anchor w 
    ::scale $inner.vscale -from 0 -to 10 -orient horizontal \
        -variable [itcl::scope _settings(terrain-vertscale)] \
        -width 10 \
        -resolution 0.1 \
        -showvalue on \
        -command [itcl::code $this AdjustSetting terrain-vertscale]
    $inner.vscale set $_settings(terrain-vertscale)

    blt::table $inner \
        0,0 $inner.grid      -cspan 2  -anchor w -pady 2 \
        1,0 $inner.wireframe -cspan 2  -anchor w -pady 2 \
        2,0 $inner.lighting  -cspan 2  -anchor w -pady 2 \
        3,0 $inner.edges     -cspan 2  -anchor w -pady 2 \
        4,0 $inner.vscale_l  -anchor w -pady 2 \
        4,1 $inner.vscale    -fill x   -pady 2 \
        5,0 $inner.palette_l -anchor w -pady 2 \
        5,1 $inner.palette   -fill x   -pady 2  

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c1 -resize expand
}

itcl::body Rappture::MapViewer::BuildLayerTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Layers" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4
    set f [frame $inner.layers]
    blt::table $inner \
        0,0 $f -fill both 
    set _layersFrame $inner
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
            [itcl::code $this camera set ${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this camera set ${tag}]
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
            [itcl::code $this camera set ${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this camera set ${tag}]
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
        -command [itcl::code $this camera set all]
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

    blt::table configure $inner c* r* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

#
#  camera -- 
#
# USAGE: camera get
#        This is called by the server to transfer the
#        current Viewpoint settings
# USAGE: camera reset
#        Reset the camera to the default view
#
itcl::body Rappture::MapViewer::camera {option args} {
    switch -- $option { 
        "get" {
            # We got the camera settings from the server
            foreach name {x y z heading pitch distance srs verticalDatum} value $args {
                set _view($name) $value
            }
            puts stderr "view: $_view(x), $_view(y), $_view(z), $_view(heading), $_view(pitch), $_view(distance), $_view(srs), $_view(verticalDatum)"
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
                set location $_mapsettings(camera)
                if { $location != "" } {
                    array set _view $location
                    set duration 0.0
                    SendCmd [list camera set $_view(x) $_view(y) $_view(z) $_view(heading) $_view(pitch) $_view(distance) $duration $_view(srs) $_view(verticalDatum)]
                } else {
                    SendCmd "camera reset"
                    # Retrieve the settings
                    #SendCmd "camera get"
                }
            } else {
                SendCmd "camera reset"
                # Retrieve the settings
               # SendCmd "camera get"
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

itcl::body Rappture::MapViewer::SetTerrainStyle { style } {
    array set settings {
        -color white
        -edgecolor black
        -edges 0
        -lighting 0
        -linewidth 1.0
        -vertscale 1.0
        -wireframe 0
    }
    array set settings $style

    SendCmd "map terrain edges $settings(-edges)"
    set _settings(terrain-edges) $settings(-edges)
    #SendCmd "map terrain color [Color2RGB $settings(-color)]"
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

itcl::body Rappture::MapViewer::SetLayerStyle { dataobj layer } {
    array set info [$dataobj layer $layer]
    set _visibility($layer) 1

    switch -- $info(type) {
        "image" {
            array set settings {
                -min_level 0
                -max_level 23
                -opacity 1.0
            }
            if { [info exists info(style)] } {
                array set settings $info(style)
            }
            if { [info exists info(opacity)] } {
                set settings(-opacity) $info(opacity)
            }
            if {!$_sendEarthFile} {
                SendCmd [list map layer add image gdal $info(url) $layer]
            }
            SendCmd "map layer opacity $settings(-opacity) $layer"
        }
        "elevation" {
            array set settings {
                -min_level 0
                -max_level 23
            }
            if { [info exists info(style)] } {
                array set settings $info(style)
            }
            if {!$_sendEarthFile} {
                SendCmd [list map layer add elevation gdal $info(url) $layer]
            }
        }
        "line" {
            array set settings {
                -color black
                -minbias 1000
                -opacity 1.0
                -width 1
            }
            if { [info exists info(style)] } {
                array set settings $info(style)
            }
            if { [info exists info(opacity)] } {
                set settings(-opacity) $info(opacity)
            }
            SendCmd [list map layer add line $info(url) $layer]
            SendCmd "map layer opacity $settings(-opacity) $layer"
        }
        "polygon" {
            array set settings {
                -color white
                -minbias 1000
                -opacity 1.0
            }
            if { [info exists info(style)] } {
                array set settings $info(style)
            }
            if { [info exists info(opacity)] } {
                set settings(-opacity) $info(opacity)
            }
            SendCmd [list map layer add polygon $info(url) $layer]
            SendCmd "map layer opacity $settings(-opacity) $layer"
        }
        "label" {
            array set settings {
                -align "center-center"
                -color black
                -declutter 1
                -font Arial
                -fontsize 16.0
                -halocolor white
                -halowidth 2.0
                -layout "ltr"
                -minbias 1000
                -opacity 1.0
                -removedupe 1
            }
            if { [info exists info(style)] } {
                array set settings $info(style)
            }
            if { [info exists info(opacity)] } {
                set settings(-opacity) $info(opacity)
            }
            set contentExpr $info(content)
            if {[info exists info(priority)]} {
                set priorityExpr $info(priority)
            } else {
                set priorityExpr ""
            }
            SendCmd [list map layer add text $info(url) $contentExpr $priorityExpr $layer]
            SendCmd "map layer opacity $settings(-opacity) $layer"
        }
    }

    if { [info exists info(visible)] } {
        if { !$info(visible) } {
            set _visibility($layer) 0
            SendCmd "map layer visible 0 $layer"
        }
    }
}

itcl::body Rappture::MapViewer::SetOpacity { dataset } { 
    foreach {dataobj layer} [split $dataset -] break
    set type [$dataobj type $layer]
    set val $_settings(-opacity)
    set sval [expr { 0.01 * double($val) }]
    if { !$_obj2ovride($dataobj-raise) } {
        # This is wrong.  Need to figure out why raise isn't set with 1
        #set sval [expr $sval * .6]
    }
    SendCmd "$type opacity $sval $dataset"
}

itcl::body Rappture::MapViewer::ChangeLayerVisibility { dataobj layer } { 
    set bool $_visibility($layer)
    SendCmd "map layer visible $bool $layer"
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
    foreach dataobj [get -objects] {
        foreach layer [$dataobj layers] {
	    array unset info
	    array set info [$dataobj layer $layer]
            checkbutton $f.$layer \
                -text $info(title) \
                -variable [itcl::scope _visibility($layer)] \
                -command [itcl::code $this \
                              ChangeLayerVisibility $dataobj $layer] \
		    -font "Arial 9" -anchor w 
            blt::table $f $row,0 $f.$layer -anchor w -pady 2 
            Rappture::Tooltip::for $f.$layer $info(description)
            incr row
	}
    }
    if { $row > 0 } {
        blt::table configure $f r* c* -resize none
        blt::table configure $f r$row c1 -resize expand
    }
}

#
# Generate an OSG Earth file to send to server.  This is inteneded
# as a stopgap and testing tool until the protocol is fleshed out.
# 
# Note that the lighting settings are required to be "hard-coded"
# as below for the runtime control to work.  Don't make those user
# configurable.
#
# Also note: Use "true"/"false" for boolean settings.  Not sure if
# the parser in OSG Earth accepts all of Tcl's forms of boolean vals.
#
itcl::body Rappture::MapViewer::EarthFile {} { 
    append out "<map"
    append out " name=\"$_mapsettings(label)\""
    append out " type=\"$_mapsettings(type)\""
    append out " version=\"2\""
    append out ">\n"
    append out " <options lighting=\"true\">\n"
    # FIXME: convert color setting to hex
    # array set style $_mapsettings(style)
    # if {[info exists style(-color)]} {
    #     set color "?"
    # }
    set color "#ffffffff"
    append out "  <terrain lighting=\"false\" color=\"$color\"/>\n"
    if { [info exists _mapsettings(projection)] } {
        append out "  <profile"
        append out " srs=\"$_mapsettings(projection)\""
        if { [info exists _mapsettings(extents)] } {
            append out " xmin=\"$_mapsettings(x1)\""
            append out " ymin=\"$_mapsettings(y1)\""
            append out " xmax=\"$_mapsettings(x2)\""
            append out " ymax=\"$_mapsettings(y2)\""
        }
        append out "/>\n"
    }
    append out " </options>\n"

    foreach dataobj [get -objects] {
        foreach layer [$dataobj layers] {
            set _layers($layer) 1
            array unset info
            array set info [$dataobj layer $layer]
            switch -- $info(type) {
                "image" {
                    append out " <image"
                    append out " name=\"$layer\""
                    append out " driver=\"gdal\""
                    if { [info exists info(opacity)] } {
                        append out " opacity=\"$info(opacity)\""
                    }
                    if { $info(visible) } {
                        append out " visible=\"true\""
                    } else {
                        append out " visible=\"false\""
                    }
                    append out ">\n"
                    append out "  <url>$info(url)</url>\n"
                    append out " </image>\n"
                }
                "elevation" {
                    append out " <elevation"
                    append out " name=\"$layer\""
                    append out " driver=\"gdal\""
                    if { $info(visible) } {
                        append out " visible=\"true\""
                    } else {
                        append out " visible=\"false\""
                    }
                    append out ">\n"
                    append out "  <url>$info(url)</url>\n"
                    append out " </elevation>\n"
                }
                default {
                    puts stderr "Type $info(type) not implemented in MapViewer::EarthFile"
                }
            }
        }
    }
    append out "</map>\n"
    return $out
} 
