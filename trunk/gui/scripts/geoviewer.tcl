# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: geoviewer - Map object viewer
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

option add *GeoViewer.width 4i widgetDefault
option add *GeoViewer*cursor crosshair widgetDefault
option add *GeoViewer.height 4i widgetDefault
option add *GeoViewer.foreground black widgetDefault
option add *GeoViewer.controlBackground gray widgetDefault
option add *GeoViewer.controlDarkBackground #999999 widgetDefault
option add *GeoViewer.plotBackground black widgetDefault
option add *GeoViewer.plotForeground white widgetDefault
option add *GeoViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc GeoViewer_init_resources {} {
    Rappture::resources::register \
        geovis_server Rappture::GeoViewer::SetServerList
}

itcl::class Rappture::GeoViewer {
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
    protected method MouseClick { button x y }
    protected method MouseDoubleClick { button x y }
    protected method MouseDrag { button x y }
    protected method MouseMotion { x y }
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
    private method BuildCameraTab {}
    private method BuildDownloadPopup { widget command } 
    private method BuildPolydataTab {}
    private method EventuallySetPolydataOpacity { args } 
    private method EventuallyResize { w h } 
    private method EventuallyRotate { q } 
    private method GetImage { args } 
    private method IsValidObject { dataobj } 
    private method PanCamera {}
    private method SetObjectStyle { dataobj comp } 
    private method SetOpacity { dataset }
    private method SetOrientation { side }
    private method SetPolydataOpacity {}

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
    private variable _style;            # Array of current component styles.
    private variable _initialStyle;     # Array of initial component styles.
    private variable _reset 1;          # Indicates that server was reset and
                                        # needs to be reinitialized.
    private variable _havePolydata 0

    private variable _first ""     ;# This is the topmost dataset.
    private variable _start 0
    private variable _title ""

    common _downloadPopup          ;# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _rotatePending 0
    private variable _polydataOpacityPending 0
    private variable _updatePending 0;
    private variable _rotateDelay 150
    private variable _scaleDelay 100
}

itk::usual GeoViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoViewer::constructor {hostlist args} {
    set _serverType "geovis"

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Update state event
    $_dispatcher register !update
    $_dispatcher dispatch $this !update "[itcl::code $this DoUpdate]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Polydata opacity event
    $_dispatcher register !polydataOpacity
    $_dispatcher dispatch $this !polydataOpacity \
        "[itcl::code $this SetPolydataOpacity]; list"
    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image    [itcl::code $this ReceiveImage]
    $_parser alias dataset  [itcl::code $this ReceiveDataset]

    # Initialize the view to some default parameters.
    array set _view {
        qw              0.853553
        qx              -0.353553
        qy              0.353553
        qz              0.146447
        zoom            1.0 
        xpan            0
        ypan            0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(zmin) 0.0
    set _limits(zmax) 1.0

    array set _settings [subst {
        legend                  1
        polydata-lighting       1
        polydata-opacity        100
        polydata-texture        1
        polydata-visible        1
        polydata-wireframe      0
    }]
    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
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
        [itcl::code $this MouseMotion %x %y]

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
        #bind $itk_component(view) <4> [itcl::code $this Zoom out]
        #bind $itk_component(view) <5> [itcl::code $this Zoom in]
        bind $itk_component(view) <4> [itcl::code $this MouseScroll up]
        bind $itk_component(view) <5> [itcl::code $this MouseScroll down]

    }

    set _image(download) [image create photo]

    eval itk_initialize $args
    Connect
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoViewer::destructor {} {
    Disconnect
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    $_dispatcher cancel !rotate
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::GeoViewer::DoResize {} {
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

itcl::body Rappture::GeoViewer::DoRotate {} {
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    SendCmd "camera orient $q" 
    set _rotatePending 0
}

itcl::body Rappture::GeoViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

itcl::body Rappture::GeoViewer::EventuallyRotate { q } {
    foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
    if { !$_rotatePending } {
        set _rotatePending 1
        $_dispatcher event -after $_rotateDelay !rotate
    }
}

itcl::body Rappture::GeoViewer::SetPolydataOpacity {} {
    set _polydataOpacityPending 0
    foreach dataset [CurrentDatasets -visible $_first] {
        foreach { dataobj comp } [split $dataset -] break
        if { [$dataobj type $comp] == "polydata" } {
            SetOpacity $dataset
        }
    }
}

itcl::body Rappture::GeoViewer::EventuallySetPolydataOpacity { args } {
    if { !$_polydataOpacityPending } {
        set _polydataOpacityPending 1
        $_dispatcher event -after $_scaleDelay !polydataOpacity
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::GeoViewer::add {dataobj {settings ""}} {
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
itcl::body Rappture::GeoViewer::delete {args} {
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
itcl::body Rappture::GeoViewer::get {args} {
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
itcl::body Rappture::GeoViewer::scale {args} {
    foreach dataobj $args {
        foreach comp [$dataobj components] {
            set type [$dataobj type $comp]
            switch -- $type {
                "polydata" {
                    set _havePolydata 1
                }
            }
        }
        array set bounds [limits $dataobj]
        if {![info exists _limits(xmin)] || $_limits(xmin) > $bounds(xmin)} {
            set _limits(xmin) $bounds(xmin)
        }
        if {![info exists _limits(xmax)] || $_limits(xmax) < $bounds(xmax)} {
            set _limits(xmax) $bounds(xmax)
        }

        if {![info exists _limits(ymin)] || $_limits(ymin) > $bounds(ymin)} {
            set _limits(ymin) $bounds(ymin)
        }
        if {![info exists _limits(ymax)] || $_limits(ymax) < $bounds(ymax)} {
            set _limits(ymax) $bounds(ymax)
        }

        if {![info exists _limits(zmin)] || $_limits(zmin) > $bounds(zmin)} {
            set _limits(zmin) $bounds(zmin)
        }
        if {![info exists _limits(zmax)] || $_limits(zmax) < $bounds(zmax)} {
            set _limits(zmax) $bounds(zmax)
        }
    }
    if { $_havePolydata } {
        if { ![$itk_component(main) exists "Mesh Settings"] } {
            if { [catch { BuildPolydataTab } errs ]  != 0 } {
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
itcl::body Rappture::GeoViewer::download {option args} {
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
            set popup .geoviewerdownload
            if { ![winfo exists .geoviewerdownload] } {
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
            set popup .geoviewerdownload
            if {[winfo exists .geoviewerdownload]} {
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
itcl::body Rappture::GeoViewer::Connect {} {
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
            # Do this immediately on connect before buffing any commands
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
            lappend info "client" "geoviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }

        SendCmd "renderer load /usr/share/osgearth/maps/gdal_tiff.earth"

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
itcl::body Rappture::GeoViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::GeoViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::GeoViewer::Disconnect {} {
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
itcl::body Rappture::GeoViewer::ReceiveImage { args } {
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
            fconfigure $f -encoding binary
            puts -nonewline $f $bytes
            close $f
        }
        $_image(plot) configure -data $bytes
        set time [clock seconds]
        set date [clock format $time]
        if { $_start > 0 } {
            set finish [clock clicks -milliseconds]
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
itcl::body Rappture::GeoViewer::ReceiveDataset { args } {
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
itcl::body Rappture::GeoViewer::Rebuild {} {

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

    if { $_reset } {
        set _width $w
        set _height $h
        $_arcball resize $w $h
        DoResize
        #FixSettings ?

        if { $_havePolydata } {
            FixSettings polydata-edges polydata-lighting polydata-opacity \
                polydata-visible polydata-wireframe 
        }
        StopBufferingCommands
        SendCmd "imgflush"
        StartBufferingCommands
    }

    set _limits(zmin) ""
    set _limits(zmax) ""
    set _first ""
    #SendCmd "dataset visible 0"
    set count 0
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        set _obj2datasets($dataobj) ""
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj data $comp]
                if { $bytes == "" } {
                    continue
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
                SendCmd "dataset visible 1 $tag"
                SetOpacity $tag
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

    set _reset 0
    global readyForNextFrame
    set readyForNextFrame 0;            # Don't advance to the next frame
                                        # until we get an image.

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
itcl::body Rappture::GeoViewer::CurrentDatasets {args} {
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

itcl::body Rappture::GeoViewer::MouseClick {button x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    SendCmd "mouse click $button $x $y"
}

itcl::body Rappture::GeoViewer::MouseDoubleClick {button x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    SendCmd "mouse dblclick $button $x $y"
}

itcl::body Rappture::GeoViewer::MouseDrag {button x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    SendCmd "mouse drag $button $x $y"
}

itcl::body Rappture::GeoViewer::MouseRelease {button x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    SendCmd "mouse release $button $x $y"
}

itcl::body Rappture::GeoViewer::MouseMotion {x y} {
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    set x [expr {(2.0 * double($x)/$w) - 1.0}]
    set y [expr {(2.0 * double($y)/$h) - 1.0}]
    SendCmd "mouse motion $x $y"
}

itcl::body Rappture::GeoViewer::MouseScroll {direction} {
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
itcl::body Rappture::GeoViewer::Zoom {option} {
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
                qw      0.853553
                qx      -0.353553
                qy      0.353553
                qz      0.146447
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
            DoRotate
            SendCmd "camera reset"
        }
    }
}

itcl::body Rappture::GeoViewer::PanCamera {} {
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
itcl::body Rappture::GeoViewer::Rotate {option x y} {
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

itcl::body Rappture::GeoViewer::Pick {x y} {
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
itcl::body Rappture::GeoViewer::Pan {option x y} {
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
itcl::body Rappture::GeoViewer::FixSettings { args } {
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
itcl::body Rappture::GeoViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "polydata-opacity" {
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                if { [$dataobj type $comp] == "polydata" } {
                    SetOpacity $dataset
                }
            }
        }
        "polydata-wireframe" {
            set bool $_settings(polydata-wireframe)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type wireframe $bool $dataset"
                }
            }
        }
        "polydata-visible" {
            set bool $_settings(polydata-visible)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type visible $bool $dataset"
                }
            }
        }
        "polydata-lighting" {
            set bool $_settings(polydata-lighting)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type lighting $bool $dataset"
                }
            }
        }
        "polydata-edges" {
            set bool $_settings(polydata-edges)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type edges $bool $dataset"
                }
            }
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::GeoViewer::limits { dataobj } {
    foreach comp [$dataobj components] {
        set tag $dataobj-$comp
        if { ![info exists _limits($tag)] } {
            set data [$dataobj data $comp]
            if { $data == "" } {
                continue
            }
        }

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

itcl::body Rappture::GeoViewer::BuildPolydataTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Mesh Settings" \
        -icon [Rappture::icon mesh]]
    $inner configure -borderwidth 4

    checkbutton $inner.mesh \
        -text "Show Mesh" \
        -variable [itcl::scope _settings(polydata-visible)] \
        -command [itcl::code $this AdjustSetting polydata-visible] \
        -font "Arial 9" -anchor w 

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(polydata-wireframe)] \
        -command [itcl::code $this AdjustSetting polydata-wireframe] \
        -font "Arial 9" -anchor w 

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(polydata-lighting)] \
        -command [itcl::code $this AdjustSetting polydata-lighting] \
        -font "Arial 9" -anchor w

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(polydata-edges)] \
        -command [itcl::code $this AdjustSetting polydata-edges] \
        -font "Arial 9" -anchor w

    label $inner.palette_l -text "Palette" -font "Arial 9" -anchor w 
    itk_component add meshpalette {
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

    $itk_component(meshpalette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting polydata-palette]

    label $inner.opacity_l -text "Opacity" -font "Arial 9" -anchor w 
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(polydata-opacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this AdjustSetting polydata-opacity]
    $inner.opacity set $_settings(polydata-opacity)

    blt::table $inner \
        0,0 $inner.mesh      -cspan 2  -anchor w -pady 2 \
        1,0 $inner.wireframe -cspan 2  -anchor w -pady 2 \
        2,0 $inner.lighting  -cspan 2  -anchor w -pady 2 \
        3,0 $inner.edges     -cspan 2  -anchor w -pady 2 \
        4,0 $inner.opacity_l -anchor w -pady 2 \
        4,1 $inner.opacity   -fill x   -pady 2 \
        5,0 $inner.palette_l -anchor w -pady 2 \
        5,1 $inner.palette   -fill x   -pady 2  

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c1 -resize expand
}

itcl::body Rappture::GeoViewer::BuildCameraTab {} {
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
itcl::body Rappture::GeoViewer::camera {option args} {
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

itcl::body Rappture::GeoViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
         [image height $_image(download)] > 0 } {
        set bytes [$_image(download) data -format "jpeg -quality 100"]
        set bytes [Rappture::encoding::decode -as b64 $bytes]
        return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::GeoViewer::BuildDownloadPopup { popup command } {
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

itcl::body Rappture::GeoViewer::SetObjectStyle { dataobj comp } {
    # Parse style string.
    set tag $dataobj-$comp
    set type [$dataobj type $comp]
    set style [$dataobj style $comp]
    if { $dataobj != $_first } {
        set settings(-wireframe) 1
    }
    switch -- $type {
        "polydata" {
            array set settings {
                -color \#FFFFFF
                -edges 1
                -edgecolor black
                -linewidth 1.0
                -opacity 1.0
                -wireframe 0
                -lighting 1
                -visible 1
            }
            array set settings $style
            SendCmd "polydata add $tag"
            SendCmd "polydata visible $settings(-visible) $tag"
            set _settings(polydata-visible) $settings(-visible)
            SendCmd "polydata edges $settings(-edges) $tag"
            set _settings(polydata-edges) $settings(-edges)
            SendCmd "polydata color [Color2RGB $settings(-color)] $tag"
            #SendCmd "polydata colormode constant {} $tag"
            SendCmd "polydata lighting $settings(-lighting) $tag"
            set _settings(polydata-lighting) $settings(-lighting)
            SendCmd "polydata linecolor [Color2RGB $settings(-edgecolor)] $tag"
            SendCmd "polydata linewidth $settings(-linewidth) $tag"
            SendCmd "polydata opacity $settings(-opacity) $tag"
            set _settings(polydata-opacity) [expr 100.0 * $settings(-opacity)]
            SendCmd "polydata wireframe $settings(-wireframe) $tag"
            set _settings(polydata-wireframe) $settings(-wireframe)
            set havePolyData 1
        }
    }
    SetColormap $dataobj $comp
}

itcl::body Rappture::GeoViewer::IsValidObject { dataobj } {
    return 1
}

itcl::body Rappture::GeoViewer::SetOrientation { side } { 
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

itcl::body Rappture::GeoViewer::SetOpacity { dataset } { 
    foreach {dataobj comp} [split $dataset -] break
    set type [$dataobj type $comp]
    set val $_settings($type-opacity)
    set sval [expr { 0.01 * double($val) }]
    if { !$_obj2ovride($dataobj-raise) } {
        # This is wrong.  Need to figure out why raise isn't set with 1
        #set sval [expr $sval * .6]
    }
    SendCmd "$type opacity $sval $dataset"
}
