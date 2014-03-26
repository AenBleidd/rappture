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

    private variable _layers "";	# Name of layers tab widget
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

    protected method Connect {}
    protected method CurrentDatasets {args}
    protected method Disconnect {}
    protected method DoResize {}
    protected method DoRotate {}
    protected method AdjustSetting {what {value ""}}
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
    protected method Pick {x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method Rotate {option x y}
    protected method Zoom {option}

    # The following methods are only used by this class.
    private method UpdateLayerControls {}
    private method ChangeLayerVisibility { dataobj layer }
    private method BuildCameraTab {}
    private method BuildLayerTab {}
    private method BuildDownloadPopup { widget command } 
    private method BuildTerrainTab {}
    private method EventuallyResize { w h } 
    private method EventuallyHandleMotionEvent { x y } 
    private method EventuallyRotate { q } 
    private method GetImage { args } 
    private method PanCamera {}
    private method SetObjectStyle { dataobj layer } 
    private method SetOpacity { dataset }
    private method SetOrientation { side }

    private variable _arcball ""
    private variable _dlist "";		# list of data objects
    private variable _obj2datasets
    private variable _obj2ovride;	# maps dataobj => style override
    private variable _datasets;		# contains all the dataobj-component 
                                   	# datasets in the server
    private variable _click;            # info used for rotate operations
    private variable _limits;           # autoscale min/max for all axes
    private variable _view;             # view params for 3D view
    private variable _settings
    private variable _visibility
    private variable _style;            # Array of current component styles.
    private variable _initialStyle;     # Array of initial component styles.
    private variable _reset 1;          # Indicates that server was reset and
                                        # needs to be reinitialized.
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
    private variable _scaleDelay 100
    private variable _motion 
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
    $_parser alias dataset  [itcl::code $this ReceiveDataset]

    array set _motion {
        x               0
        y               0
        pending         0
        delay           100
        compress        0
    }
    # Initialize the view to some default parameters.
    array set _view {
        qw              1.0
        qx              0.0
        qy              0.0
        qz              0.0
        zoom            1.0
        xpan            0.0
        ypan            0.0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(zmin) 0.0
    set _limits(zmax) 1.0

    array set _settings [subst {
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

    # Bindings for keyboard events
    bind $itk_component(view) <KeyPress> \
        [itcl::code $this KeyPress %N]
    bind $itk_component(view) <KeyRelease> \
        [itcl::code $this KeyRelease %N]

    # Bindings for rotation via mouse
    bind $itk_component(view) <ButtonPress-1> \
        [itcl::code $this MouseClick 1 %x %y]
        #[itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <Double-1> \
        [itcl::code $this MouseDoubleClick 1 %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this MouseDrag 1 %x %y]
        #[itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this MouseRelease 1 %x %y]
        #[itcl::code $this Rotate release %x %y]
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this MouseClick 2 %x %y]
        #[itcl::code $this Pan click %x %y]
    bind $itk_component(view) <Double-2> \
        [itcl::code $this MouseDoubleClick 2 %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this MouseDrag 2 %x %y]
        #[itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this MouseRelease 2 %x %y]
        #[itcl::code $this Pan release %x %y]

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
        #bind $itk_component(view) <4> [itcl::code $this Zoom out]
        #bind $itk_component(view) <5> [itcl::code $this Zoom in]
        bind $itk_component(view) <4> [itcl::code $this MouseScroll up]
        bind $itk_component(view) <5> [itcl::code $this MouseScroll down]
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
    catch { blt::arcball destroy $_arcball }
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
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::MapViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

itcl::body Rappture::MapViewer::EventuallyRotate { q } {
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
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
	    set tag $dataobj-$layer
            SendCmd "map layer visible 0 $tag"
            set _visibility($tag) 0
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

    # Verify that all the maps have the same global settings. For example,
    # you can't have one map type "geocentric" and the other "projected".

    foreach dataobj $args {
        if { ![$dataobj isvalid] } {
            continue
        }
        array unset hints 
        array set hints [$dataobj hints]
        if { ![info exists overall(type)] } {
            set _mapsettings(type) $hints(type)
        } elseif { $hints(type) != $_mapsettings(type) } {
            error "maps \"$hints(label)\" have differing types"
        }
        if { ![info exists _mapsettings(projection)] } {
            set _mapsettings(projection) $hints(projection)
        } elseif { $hints(projection) != $_mapsettings(projection) } {
            error "maps \"$hints(label)\" have differing projections"
        }
        if { ![info exists _mapsettings(extents)] } {
            set _mapsettings(extents) $hints(extents)
        } elseif { $hints(extents) != $_mapsettings(extents) } {
            error "maps \"$hints(label)\" have differing extents"
        }

        foreach layer [$dataobj layers] {
            set type [$dataobj layer $layer]
            switch -- $type {
                "elevation" {
                    set _haveTerrain 1
                }
            }
        }
    }
    if { $_haveTerrain } {
        if { ![$itk_component(main) exists "Terrain Settings"] } {
            if { [catch { BuildTerrainTab } errs ]  != 0 } {
                puts stderr "errs=$errs"
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

    # disconnected -- no more data sitting on server
    array unset _datasets 
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
# ReceiveDataset --
#
itcl::body Rappture::MapViewer::ReceiveDataset { args } {
    if { ![isconnected] } {
        return
    }
    set option [lindex $args 0]
    switch -- $option {
        "coords" {
            foreach { x y z } [lrange $args 1 end] break
            puts stderr "Coords: $x $y $z"
        }
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
        $_arcball resize $w $h
        DoResize

        if { [info exists _mapsettings(type)] } {

            # The map must be reset once before any layers are added This
            # should not be done more than once as it is very expensive.

            if { $_mapsettings(type) == "geocentric" } {
                SendCmd "map reset geocentric"
            } else {
                if { $_mapsettings(extents) == ""} {
                    SendCmd "map reset projected global-mercator"
                } else {
                    SendCmd \
        "map reset projected $_mapsettings(projection) $_mapsettings(extents)"
                }
            }
            if { $_haveTerrain } {
                FixSettings terrain-edges terrain-lighting terrain-vertscale \
                    terrain-wireframe
            }
            SendCmd "imgflush"
        }
    }

    set _limits(zmin) ""
    set _limits(zmax) ""
    set _first ""
    set count 0

    foreach dataobj [get -objects] {
        set _obj2datasets($dataobj) ""
        foreach layer [$dataobj layers] {
	    array unset info
	    array set info [$dataobj layer $layer]
	    set tag $dataobj-$layer
            if { ![info exists _datasets($tag)] } {
		if { ![info exists info(url)] }  {
		    continue
		}
                # FIXME: wms, tms layers have additional options
		switch -- $info(type) {
		    "raster" {
			set type "image"
		    }
		    default {
			set type "$info(type)"
		    }
		}
                if { $_reportClientInfo }  {
                    set cinfo {}
                    lappend cinfo "tool_id"       [$dataobj hints toolId]
                    lappend cinfo "tool_name"     [$dataobj hints toolName]
                    lappend cinfo "tool_version"  [$dataobj hints toolRevision]
                    lappend cinfo "tool_title"    [$dataobj hints toolTitle]
                    lappend cinfo "dataset_label" [$dataobj hints label]
                    lappend cinfo "dataset_tag"   $tag
                    SendCmd [list "clientinfo" $cinfo]
                }
                SendCmd [list map layer add $type $info(url) $tag]
                set _datasets($tag) 1
                SetObjectStyle $dataobj $layer
            }
            lappend _obj2datasets($dataobj) $tag
            if { [info exists _obj2ovride($dataobj-raise)] } {
                SendCmd "map layer visible 1 $tag"
                set _visibility($tag) 1
                #SetLayerOpacity $tag
            }
        }
    }
    if {"" != $_first} {
        set location [$_first hints camera]
        if { $location != "" } {
            array set view $location
        }
    }
    if { $_reset } {
        set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
        $_arcball quaternion $q 
        SendCmd "camera reset"
        DoRotate
        PanCamera
        Zoom reset
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
# USAGE: CurrentDatasets ?-all -visible? ?dataobjs?
#
# Returns a list of server IDs for the current datasets being displayed.
# This is normally a single ID, but it might be a list of IDs if the
# current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::CurrentDatasets {args} {
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
        foreach layer [$dataobj layers] {
            set tag $dataobj-$layer
            if { [info exists _datasets($tag)] && $_datasets($tag) } {
                lappend rlist $tag
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

itcl::body Rappture::MapViewer::MouseClick {button x y} {
    if {0} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    }
    SendCmd "mouse click $button $x $y"
}

itcl::body Rappture::MapViewer::MouseDoubleClick {button x y} {
    if {0} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    }
    SendCmd "mouse dblclick $button $x $y"
}

itcl::body Rappture::MapViewer::MouseDrag {button x y} {
    if {0} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    }
    SendCmd "mouse drag $button $x $y"
}

itcl::body Rappture::MapViewer::MouseRelease {button x y} {
    if {0} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    }
    SendCmd "mouse release $button $x $y"
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

itcl::body Rappture::MapViewer::MouseMotion {} {
    if {0} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($_motion(x))/$w) - 1.0}]
    set y [expr {(2.0 * double($_motion(y))/$h) - 1.0}]
    }
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

# ----------------------------------------------------------------------
# USAGE: Zoom in
# USAGE: Zoom out
# USAGE: Zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::MapViewer::Zoom {option} {
    switch -- $option {
        "in" {
            set _view(zoom) [expr {$_view(zoom)*1.25}]
            #SendCmd "camera zoom $_view(zoom)"
            set z -0.25
            SendCmd "camera zoom $z"
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
            #SendCmd "camera zoom $_view(zoom)"
            set z 0.25
            SendCmd "camera zoom $z"
        }
        "reset" {
            array set _view {
                qw      1.0
                qx      0.0
                qy      0.0
                qz      0.0
                zoom    1.0
                xpan    0.0
                ypan    0.0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
            $_arcball quaternion $q
            DoRotate
            SendCmd "camera reset"
        }
    }
}

itcl::body Rappture::MapViewer::PanCamera {} {
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

itcl::body Rappture::MapViewer::Pick {x y} {
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
itcl::body Rappture::MapViewer::Pan {option x y} {
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
    foreach layer [$dataobj layers] {
        set tag $dataobj-$layer

        foreach { xMin xMax yMin yMax zMin zMax} $_limits($tag) break
        if {![info exists limits(xmin)] || $limits(xmin) > $xMin} {
            set limits(xmin) $xMin
        }
        if {![info exists limits(xmax)] || $limits(xmax) < $xMax} {
            set limits(xmax) $xMax
        }
        if {![info exists limits(ymin)] || $limits(ymin) > $yMin} {
            set limits(ymin) $xMin
        }
        if {![info exists limits(ymax)] || $limits(ymax) < $yMax} {
            set limits(ymax) $yMax
        }
        if {![info exists limits(zmin)] || $limits(zmin) > $zMin} {
            set limits(zmin) $zMin
        }
        if {![info exists limits(zmax)] || $limits(zmax) < $zMax} {
            set limits(zmax) $zMax
        }
    }
    return [array get limits]
}

itcl::body Rappture::MapViewer::BuildTerrainTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Terrain Settings" \
        -icon [Rappture::icon mesh]]
    $inner configure -borderwidth 4

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
    ::scale $inner.vscale -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(terrain-vertscale)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting terrain-vertscale]
    $inner.vscale set $_settings(terrain-vertscale)

    blt::table $inner \
        0,0 $inner.wireframe -cspan 2  -anchor w -pady 2 \
        1,0 $inner.lighting  -cspan 2  -anchor w -pady 2 \
        2,0 $inner.edges     -cspan 2  -anchor w -pady 2 \
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
    set _layers $inner
}

itcl::body Rappture::MapViewer::BuildCameraTab {} {
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

    set labels { qx qy qz qw xpan ypan zoom }
    set row 1
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

    blt::table configure $inner c* r* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

#
#  camera -- 
#
itcl::body Rappture::MapViewer::camera {option args} {
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

itcl::body Rappture::MapViewer::SetObjectStyle { dataobj layer } {
    set tag $dataobj-$layer
    set _visibility($tag) 1

    return 

    # The following code is a place holder for terrain styles.

    set type [$dataobj type $layer]
    set style [$dataobj style $layer]
    if { $dataobj != $_first } {
        set settings(-wireframe) 1
    }
    switch -- $type {
        "elevation" {
            array set settings {
                -edgecolor black
                -edges 0
                -lighting 1
                -linewidth 1.0
                -vertscale 1.0
                -wireframe 0
            }
            array set settings $style
            SendCmd "map terrain edges $settings(-edges) $tag"
            set _settings(terrain-edges) $settings(-edges)
            SendCmd "map terrain color [Color2RGB $settings(-color)] $tag"
            #SendCmd "map terrain colormode constant {} $tag"
            SendCmd "map terrain lighting $settings(-lighting) $tag"
            set _settings(terrain-lighting) $settings(-lighting)
            SendCmd "map terrain linecolor [Color2RGB $settings(-edgecolor)] $tag"
            SendCmd "map terrain linewidth $settings(-linewidth) $tag"
            SendCmd "map terrain wireframe $settings(-wireframe) $tag"
            set _settings(terrain-wireframe) $settings(-wireframe)
        }
    }
    #SetColormap $dataobj $layer
}

itcl::body Rappture::MapViewer::SetOrientation { side } { 
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
    #SendCmd "camera reset"
    set _view(xpan) 0
    set _view(ypan) 0
    set _view(zoom) 1.0
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
    set tag $dataobj-$layer
    set bool $_visibility($tag)
    SendCmd "map layer visible $bool $tag"
}

itcl::body Rappture::MapViewer::UpdateLayerControls {} { 
    set row 0
    set inner $_layers
    if { [winfo exists $inner.layers] } {
        foreach w [winfo children $inner.layers] {
            destroy $w
        }
    }
    set f $inner.layers
    foreach dataobj [get -objects] {
        foreach name [$dataobj layers] {
	    array unset info
	    array set info [$dataobj layer $name]
	    set tag $dataobj-$name
            set w [string range $dataobj$name 2 end]
            checkbutton $f.$w \
                -text $info(title) \
                -variable [itcl::scope _visibility($tag)] \
                -command [itcl::code $this \
                              ChangeLayerVisibility $dataobj $name] \
		    -font "Arial 9" -anchor w 
            blt::table $f $row,0 $f.$w -anchor w -pady 2 
            Rappture::Tooltip::for $f.$w $info(description)
            incr row
	}
    }
    if { $row > 0 } {
        blt::table configure $f r* c* -resize none
        blt::table configure $f r$row c1 -resize expand
    }
}
