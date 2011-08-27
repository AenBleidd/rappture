
# ----------------------------------------------------------------------
#  COMPONENT: vtkviewer - Vtk drawing object viewer
#
#  It connects to the Vtk server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
#package require Img

option add *VtkViewer.width 4i widgetDefault
option add *VtkViewer*cursor crosshair widgetDefault
option add *VtkViewer.height 4i widgetDefault
option add *VtkViewer.foreground black widgetDefault
option add *VtkViewer.controlBackground gray widgetDefault
option add *VtkViewer.controlDarkBackground #999999 widgetDefault
option add *VtkViewer.plotBackground black widgetDefault
option add *VtkViewer.plotForeground white widgetDefault
option add *VtkViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkViewer::SetServerList
}

itcl::class Rappture::VtkViewer {
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
        Rappture::VisViewer::SetServerList "vtkvis" $namelist
    }
    public method add {dataobj {settings ""}}
    public method camera {option args}
    public method delete {args}
    public method disconnect {}
    public method download {option args}
    public method get {args}
    public method isconnected {}
    public method limits { colormap }
    public method sendto { string }
    public method parameters {title args} { 
        # do nothing 
    }
    public method scale {args}

    protected method Connect {}
    protected method CurrentDatasets {args}
    protected method Disconnect {}
    protected method DoResize {}
    protected method FixSettings {what {value ""}}
    protected method Pan {option x y}
    protected method Pick {x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method Rotate {option x y}
    protected method SendCmd {string}
    protected method Zoom {option}

    # The following methods are only used by this class.
    private method BuildCameraTab {}
    private method BuildViewTab {}
    private method BuildAxisTab {}
    private method BuildColormap { colormap dataobj comp }
    private method EventuallyResize { w h } 
    private method SetStyles { dataobj comp }
    private method PanCamera {}
    private method ConvertToVtkData { dataobj comp } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method BuildDownloadPopup { widget command } 
    private method SetObjectStyle { dataobj comp } 
    private method IsValidObject { dataobj } 

    private variable _arcball ""
    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _allDataObjs
    private variable _obj2datasets
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _datasets     ;# contains all the dataobj-component 
                                   ;# datasets in the server
    private variable _colormaps    ;# contains all the colormaps
                                   ;# in the server.
    private variable _dataset2style    ;# maps dataobj-component to transfunc
    private variable _style2datasets   ;# maps tf back to list of 
                                    # dataobj-components using the tf.

    private variable _click        ;# info used for rotate operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private common   _settings
    private variable _reset 1	   ;# indicates if camera needs to be reset
                                    # to starting position.

    # Array of transfer functions in server.  If 0 the transfer has been
    # defined but not loaded.  If 1 the transfer function has been named
    # and loaded.
    private variable _first ""     ;# This is the topmost dataset.
    private variable _start 0
    private variable _buffering 0
    
    # This indicates which isomarkers and transfer function to use when
    # changing markers, opacity, or thickness.
    common _downloadPopup          ;# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _outline
}

itk::usual VtkViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::constructor {hostlist args} {
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    set _outbuf ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias dataset [itcl::code $this ReceiveDataset]

    array set _outline {
	id -1
	afterId -1
	x1 -1
	y1 -1
	x2 -1
	y2 -1
    }
    # Initialize the view to some default parameters.
    array set _view {
	qx		0
	qy		0
	qz		0
	qw		1
        zoom		1.0 
        pan-x		0
        pan-y		0
        ortho		0
    }
    set _arcball [blt::arcball create 100 100]
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q

    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

    array set _settings [subst {
        $this-axes		1
        $this-edges		1
        $this-lighting		1
        $this-opacity		100
        $this-volume		1
        $this-wireframe		0
        $this-grid-x		0
        $this-grid-y		0
        $this-grid-z		0
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

    BuildViewTab
    BuildAxisTab
    BuildCameraTab

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
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

    if 0 {
    bind $itk_component(view) <Configure> \
        [itcl::code $this EventuallyResize %w %h]
    }
    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

    bind $itk_component(view) <ButtonRelease-3> \
        [itcl::code $this Pick %x %y]

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
itcl::body Rappture::VtkViewer::destructor {} {
    Disconnect
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    image delete $_image(plot)
    image delete $_image(download)
    array unset _settings $this-*
    catch { blt::arcball destroy $_arcball}
}

itcl::body Rappture::VtkViewer::DoResize {} {
    if { $_width < 2 } {
	set _width 500
    }
    if { $_height < 2 } {
	set _height 500
    }
    #puts stderr "DoResize screen size $_width $_height"
    set _start [clock clicks -milliseconds]
    SendCmd "screen size $_width $_height"

    # Must reset camera to have object scaling to take effect.
    #SendCmd "camera reset"
    #SendCmd "camera zoom $_view(zoom)"
    set _resizePending 0
}

itcl::body Rappture::VtkViewer::EventuallyResize { w h } {
    #puts stderr "EventuallyResize $w $h"
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 100 !resize
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::add {dataobj {settings ""}} {
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
        set params(-color) black
    }
    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
	lappend _dlist $dataobj
    }
    set _allDataObjs($dataobj) 1
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
itcl::body Rappture::VtkViewer::delete {args} {
    if { [llength $args] == 0} {
        set args $_dlist
    }
    # Delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
	if { $pos < 0 } {
	    continue;			# Don't know anything about it.
	}
	# Remove it from the dataobj list.
	set _dlist [lreplace $_dlist $pos $pos]
	foreach comp [$dataobj components] {
	    SendCmd "dataset visible 0 $dataobj-$comp"
	}
	array unset _obj2ovride $dataobj-*
	# Append to the end of the dataobj list.
	lappend _dlist $dataobj
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
itcl::body Rappture::VtkViewer::get {args} {
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
		    # No setting indicates that the object isn't invisible.
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
itcl::body Rappture::VtkViewer::scale {args} {
    array unset _limits
    foreach dataobj $args {
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
itcl::body Rappture::VtkViewer::download {option args} {
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
            update idletasks		;# Fix initial sizes
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
itcl::body Rappture::VtkViewer::Connect {} {
    #puts stderr "Enter Connect: [info level -1]"
    set _hosts [GetServerList "vtkvis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
    if { $result } {
	#puts stderr "Connected to $_hostname sid=$_sid"
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
itcl::body Rappture::VtkViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkViewer::disconnect {} {
    Disconnect
    set _reset 1
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _datasets 
    array unset _data 
    array unset _colormaps 
}

#
# sendto --
#
itcl::body Rappture::VtkViewer::sendto { bytes } {
    SendBytes "$bytes\n"
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::VtkViewer::SendCmd {string} {
    if { $_buffering } {
        append _outbuf $string "\n"
    } else {
        SendBytes "$string\n"
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size> -type <type> -token <token>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::ReceiveImage { args } {
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
itcl::body Rappture::VtkViewer::ReceiveDataset { args } {
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
itcl::body Rappture::VtkViewer::Rebuild {} {

    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $w < 2 || $h < 2 } {
	$_dispatcher event -idle !rebuild
	return
    }

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    set _width $w
    set _height $h
    $_arcball resize $w $h
    DoResize
    
    set _limits(vmin) ""
    set _limits(vmax) ""
    set _first ""
    foreach dataobj [get -objects] {
	if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
	    set _first $dataobj
	}
	set _obj2datasets($dataobj) ""
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj data $comp]
                set length [string length $bytes]
                append _outbuf "dataset add $tag data follows $length\n"
                append _outbuf $bytes
		append _outbuf "polydata add $tag\n"
                set _datasets($tag) 1
	    }
	    lappend _obj2datasets($dataobj) $tag
	    SetObjectStyle $dataobj $comp
	    if { [info exists _obj2ovride($dataobj-raise)] } {
		SendCmd "dataset visible 1 $tag"
	    } else {
		SendCmd "dataset visible 0 $tag"
	    }
        }
    }
    #
    # Reset the camera and other view parameters
    #
    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
    $_arcball quaternion $q
    if {$_view(ortho)} {
        SendCmd "camera mode ortho"
    } else {
        SendCmd "camera mode persp"
    }
    SendCmd "camera orient $q" 
    PanCamera
    if { $_reset || $_first == "" } {
	Zoom reset
	set _reset 0
    }
    FixSettings opacity
    FixSettings grid-x
    FixSettings grid-y
    FixSettings grid-z
    FixSettings volume
    FixSettings lighting
    FixSettings wireframe
    FixSettings axes
    FixSettings edges
    FixSettings axismode

    if {"" != $_first} {
        set location [$_first hints camera]
        if { $location != "" } {
            array set view $location
        }
    }

    set _buffering 0;                        # Turn off buffering.

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf;                        
    blt::busy release $itk_component(hull)
    set _outbuf "";                        # Clear the buffer.                
}

# ----------------------------------------------------------------------
# USAGE: CurrentDatasets ?-all -visible? ?dataobjs?
#
# Returns a list of server IDs for the current datasets being displayed.  This
# is normally a single ID, but it might be a list of IDs if the current data
# object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::CurrentDatasets {args} {
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
itcl::body Rappture::VtkViewer::Zoom {option} {
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
                qx	0
                qy      0
                qz      0
                qw      1
                zoom    1.0
                pan-x   0
                pan-y   0
            }
            SendCmd "camera reset all"
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
	    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
	    $_arcball quaternion $q
        }
    }
}

itcl::body Rappture::VtkViewer::PanCamera {} {
    set x $_view(pan-x)
    set y $_view(pan-y)
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
itcl::body Rappture::VtkViewer::Rotate {option x y} {
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
		foreach { _view(qw) _view(qx) _view(qy) _view(qz) } $q break
                SendCmd "camera orient $q" 
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

itcl::body Rappture::VtkViewer::Pick {x y} {
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
itcl::body Rappture::VtkViewer::Pan {option x y} {
    switch -- $option {
	"set" {
	    set w [winfo width $itk_component(view)]
	    set h [winfo height $itk_component(view)]
	    set x [expr $x / double($w)]
	    set y [expr $y / double($h)]
	    set _view(pan-x) [expr $_view(pan-x) + $x]
	    set _view(pan-y) [expr $_view(pan-y) + $y]
	    PanCamera
	    return
	}
	"click" {
	    set _click(x) $x
	    set _click(y) $y
	    $itk_component(view) configure -cursor hand1
	}
	"drag" {
	    set w [winfo width $itk_component(view)]
	    set h [winfo height $itk_component(view)]
	    set dx [expr ($_click(x) - $x)/double($w)]
	    set dy [expr ($_click(y) - $y)/double($h)]
	    set _click(x) $x
	    set _click(y) $y
	    set _view(pan-x) [expr $_view(pan-x) - $dx]
	    set _view(pan-y) [expr $_view(pan-y) - $dy]
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
itcl::body Rappture::VtkViewer::FixSettings {what {value ""}} {
    switch -- $what {
        "opacity" {
            if {[isconnected]} {
                set val $_settings($this-opacity)
                set sval [expr { 0.01 * double($val) }]
		foreach dataset [CurrentDatasets -visible $_first] {
		    SendCmd "polydata opacity $sval $dataset"
		}
            }
        }
        "wireframe" {
            if {[isconnected]} {
		set bool $_settings($this-wireframe)
		foreach dataset [CurrentDatasets -visible $_first] {
		    SendCmd "polydata wireframe $bool $dataset"
		}
            }
        }
        "volume" {
            if {[isconnected]} {
		set bool $_settings($this-volume)
		foreach dataset [CurrentDatasets -visible $_first] {
		    SendCmd "polydata visible $bool $dataset"
		}
            }
        }
        "lighting" {
            if {[isconnected]} {
		set bool $_settings($this-lighting)
		foreach dataset [CurrentDatasets -visible $_first] {
		    SendCmd "polydata lighting $bool $dataset"
		}
            }
        }
        "grid-x" {
            if {[isconnected]} {
		set bool $_settings($this-grid-x)
		SendCmd "axis grid x $bool"
            }
        }
        "grid-y" {
            if {[isconnected]} {
		set bool $_settings($this-grid-y)
		SendCmd "axis grid y $bool"
            }
        }
        "grid-z" {
            if {[isconnected]} {
		set bool $_settings($this-grid-z)
		SendCmd "axis grid z $bool"
            }
        }
        "edges" {
            if {[isconnected]} {
		set bool $_settings($this-edges)
		foreach dataset [CurrentDatasets -visible $_first] {
		    SendCmd "polydata edges $bool $dataset"
		}
            }
        }
        "axes" {
            if { [isconnected] } {
		set bool $_settings($this-axes)
                SendCmd "axis visible all $bool"
            }
        }
        "axismode" {
            if { [isconnected] } {
		set mode [$itk_component(axismode) value]
		set mode [$itk_component(axismode) translate $mode]
                SendCmd "axis flymode $mode"
            }
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

#
# SetStyles --
#
itcl::body Rappture::VtkViewer::SetStyles { dataobj comp } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
    }
    set tag $dataobj-$comp
    array set style [lindex [$dataobj components -style $comp] 0]
    set colormap "$style(-color):$style(-levels):$style(-opacity)"
    if { [info exists _colormaps($colormap)] } {
	puts stderr "Colormap $colormap already built"
    }
    if { ![info exists _dataset2style($tag)] } {
	set _dataset2style($tag) $colormap
	lappend _style2datasets($colormap) $tag
    }
    if { ![info exists _colormaps($colormap)] } {
	# Build the pseudo colormap if it doesn't exist.
	BuildColormap $colormap $dataobj $comp
	set _colormaps($colormap) 1
    }
    #SendCmd "polydata add $style(-levels) $tag\n"
    SendCmd "pseudocolor colormap $colormap $tag"
    return $colormap
}

#
# BuildColormap --
#
itcl::body Rappture::VtkViewer::BuildColormap { colormap dataobj comp } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
    }
    array set style [lindex [$dataobj components -style $comp] 0]

    if {$style(-color) == "rainbow"} {
        set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    set clist [split $style(-color) :]
    set cmap {}
    for {set i 0} {$i < [llength $clist]} {incr i} {
        set x [expr {double($i)/([llength $clist]-1)}]
        set color [lindex $clist $i]
        append cmap "$x [Color2RGB $color] "
    }
    if { [llength $cmap] == 0 } {
	set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    set tag $this-$colormap
    if { ![info exists _settings($tag-opacity)] } {
        set _settings($tag-opacity) $style(-opacity)
    }
    set max $_settings($tag-opacity)

    set wmap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $colormap { $cmap } { $wmap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::VtkViewer::limits { dataobj } {

    array unset _limits $dataobj-*
    foreach comp [$dataobj components] {
	set tag $dataobj-$comp
	if { ![info exists _limits($tag)] } {
	    set data [$dataobj data $comp]
	    set arr [vtkCharArray $tag-xvtkCharArray]
	    $arr SetArray $data [string length $data] 1
	    set reader [vtkDataSetReader $tag-xvtkDataSetReader]
	    $reader SetInputArray $arr
	    $reader ReadFromInputStringOn
	    set _limits($tag) [[$reader GetOutput] GetBounds]
	    rename $reader ""
	    rename $arr ""
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


itcl::body Rappture::VtkViewer::BuildViewTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings($this-wireframe)] \
        -command [itcl::code $this FixSettings wireframe] \
        -font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
        -font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Lighting" \
        -variable [itcl::scope _settings($this-lighting)] \
        -command [itcl::code $this FixSettings lighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings($this-edges)] \
        -command [itcl::code $this FixSettings edges] \
        -font "Arial 9"

    label $inner.clear -text "Clear" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-opacity)] \
        -width 10 \
        -showvalue off -command [itcl::code $this FixSettings opacity]
    label $inner.opaque -text "Opaque" -font "Arial 9"

    blt::table $inner \
        0,0 $inner.axes -columnspan 4 -anchor w -pady 2 \
        1,0 $inner.volume -columnspan 4 -anchor w -pady 2 \
        2,0 $inner.wireframe -columnspan 4 -anchor w -pady 2 \
        3,0 $inner.lighting  -columnspan 4 -anchor w \
        4,0 $inner.edges -columnspan 4 -anchor w -pady 2 \
        6,0 $inner.clear -anchor e -pady 2 \
        6,1 $inner.opacity -columnspan 2 -pady 2 -fill x\
        6,3 $inner.opaque -anchor w -pady 2 

    blt::table configure $inner r* -resize none
    blt::table configure $inner r7 -resize expand
}

itcl::body Rappture::VtkViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon cog]]
    $inner configure -borderwidth 4

    checkbutton $inner.axes \
        -text "Visible" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
        -font "Arial 9"

    label $inner.grid -text "Grid" -font "Arial 9"
    set f [frame $inner.gridf]
    checkbutton $f.x \
        -text "X" \
        -variable [itcl::scope _settings($this-grid-x)] \
        -command [itcl::code $this FixSettings grid-x] \
        -font "Arial 9"
    checkbutton $f.y \
        -text "Y" \
        -variable [itcl::scope _settings($this-grid-y)] \
        -command [itcl::code $this FixSettings grid-y] \
        -font "Arial 9"
    checkbutton $f.z \
        -text "Z" \
        -variable [itcl::scope _settings($this-grid-z)] \
        -command [itcl::code $this FixSettings grid-z] \
        -font "Arial 9"
    pack $f.x $f.y $f.z -side left 

    label $inner.axismode -text "Mode" \
        -font "Arial 9" 

    itk_component add axismode {
	Rappture::Combobox $inner.axismode_combo -width 10 -editable no
    }
    $inner.axismode_combo choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "furthest" \
        "outer_edges"     "outer"         
    $itk_component(axismode) value "outer"
    bind $inner.axismode_combo <<Value>> [itcl::code $this FixSettings axismode]

    blt::table $inner \
        0,0 $inner.axes -columnspan 4 -anchor w -pady 2 \
        1,0 $inner.axismode -anchor w -pady 2 \
        1,1 $inner.axismode_combo -cspan 3 -anchor w -pady 2 \
        2,0 $inner.grid -anchor w -pady 2 \
        2,1 $inner.gridf -anchor w -cspan 3 -fill x 

    blt::table configure $inner r* -resize none
    blt::table configure $inner r3 -resize expand
}

itcl::body Rappture::VtkViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set labels { qx qy qz qw pan-x pan-y zoom }
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
        -text "Orthogrpahic" \
        -variable [itcl::scope _view(ortho)] \
        -command [itcl::code $this camera set ortho] \
        -font "Arial 9"
    blt::table $inner \
            $row,0 $inner.ortho -columnspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}


#
#  camera -- 
#
itcl::body Rappture::VtkViewer::camera {option args} {
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
                "pan-x" - "pan-y" {
                    PanCamera
                }
                "qx" - "qy" - "qz" - "qw" {
                    set q [list $_view(qw) $_view(qx) $_view(qy) $_view(qz)]
		    $_arcball quaternion $q
                    SendCmd "camera orient $q"
                }
                "zoom" {
                    SendCmd "camera zoom $_view(zoom)"
                }
            }
        }
    }
}

itcl::body Rappture::VtkViewer::ConvertToVtkData { dataobj comp } {
    foreach { x1 x2 xN y1 y2 yN } [$dataobj mesh $comp] break
    set values [$dataobj values $comp]
    append out "# vtk DataFile Version 2.0 \n"
    append out "Test data \n"
    append out "ASCII \n"
    append out "DATASET STRUCTURED_POINTS \n"
    append out "DIMENSIONS $xN $yN 1 \n"
    append out "ORIGIN 0 0 0 \n"
    append out "SPACING 1 1 1 \n"
    append out "POINT_DATA [expr $xN * $yN] \n"
    append out "SCALARS field float 1 \n"
    append out "LOOKUP_TABLE default \n"
    append out [join $values "\n"]
    append out "\n"
    return $out
}


itcl::body Rappture::VtkViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
	    set contents [ConvertToVtkData $dataobj $comp]
	    append bytes "$contents\n\n"
        }
    }
    return [list .txt $bytes]
}

itcl::body Rappture::VtkViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
	 [image height $_image(download)] > 0 } {
	set bytes [$_image(download) data -format "jpeg -quality 100"]
	set bytes [Rappture::encoding::decode -as b64 $bytes]
	return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkViewer::BuildDownloadPopup { popup command } {
    Rappture::Balloon $popup \
        -title "[Rappture::filexfer::label downloadWord] as..."
    set inner [$popup component inner]
    label $inner.summary -text "" -anchor w 
    radiobutton $inner.vtk_button -text "VTK data file" \
        -variable [itcl::scope _downloadPopup(format)] \
        -font "Helvetica 9 " \
        -value vtk  
    Rappture::Tooltip::for $inner.vtk_button "Save as VTK data file."
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

itcl::body Rappture::VtkViewer::SetObjectStyle { dataobj comp } {
    array set props {
        -color \#6666FF
        -edgevisibility 1
        -edgecolor black
        -linewidth 1.0
        -opacity 1.0
	-wireframe 0
	-lighting 1
    }
    # Parse style string.
    set style [$dataobj style $comp]
    set tag $dataobj-$comp
    array set props $style
    SendCmd "polydata color [Color2RGB $props(-color)] $tag"
    SendCmd "polydata edges $props(-edgevisibility) $tag"
    SendCmd "polydata lighting $props(-lighting) $tag"
    SendCmd "polydata linecolor [Color2RGB $props(-edgecolor)] $tag"
    SendCmd "polydata linewidth $props(-linewidth) $tag"
    SendCmd "polydata opacity $props(-opacity) $tag"
    if { $dataobj != $_first } {
	set props(-wireframe) 1
    }
    SendCmd "polydata wireframe $props(-wireframe) $tag"
    set _settings($this-opacity) [expr $props(-opacity) * 100.0]
}

itcl::body Rappture::VtkViewer::IsValidObject { dataobj } {
    if {[catch {$dataobj isa Rappture::Drawing} valid] != 0 || !$valid} {
	return 0
    }
    return 1
}
