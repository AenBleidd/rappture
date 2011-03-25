
# ----------------------------------------------------------------------
#  COMPONENT: vtkcontourviewer - Contour plot viewer
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
package require Img
                                        
option add *VtkContourViewer.width 4i widgetDefault
option add *VtkContourViewer*cursor crosshair widgetDefault
option add *VtkContourViewer.height 4i widgetDefault
option add *VtkContourViewer.foreground black widgetDefault
option add *VtkContourViewer.controlBackground gray widgetDefault
option add *VtkContourViewer.controlDarkBackground #999999 widgetDefault
option add *VtkContourViewer.plotBackground black widgetDefault
option add *VtkContourViewer.plotForeground white widgetDefault
option add *VtkContourViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc VtkContourViewer_init_resources {} {
    Rappture::resources::register \
        vtkvis_server Rappture::VtkContourViewer::SetServerList
}

itcl::class Rappture::VtkContourViewer {
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
    protected method CurrentDatasets {{what -all}}
    protected method Disconnect {}
    protected method DoResize {}
    protected method FixLegend {}
    protected method FixSettings {what {value ""}}
    protected method Pan {option x y}
    protected method Rebuild {}
    protected method ReceiveDataset { args }
    protected method ReceiveImage { args }
    protected method DrawLegend {}
    protected method ReceiveLegend { colormap size }
    protected method Rotate {option x y}
    protected method SendCmd {string}
    protected method Zoom {option}

    private method AdjustSelectionRectangle { x y }
    private method BeginSelectionRectangle { x y }
    private method EndSelectionRectangle { x y }
    private method KillSelectionRectangle {}
    private method MarchingAnts {}

    # The following methods are only used by this class.
    private method BuildCameraTab {}
    private method BuildViewTab {}
    private method BuildColormap { colormap dataobj comp }
    private method EventuallyResize { w h } 
    private method EventuallyResizeLegend { } 
    private method SetStyles { dataobj comp }
    private method PanCamera {}
    private method ConvertToVtkData { dataobj comp } 
    private method GetImage { args } 
    private method GetVtkData { args } 
    private method BuildDownloadPopup { widget command } 

    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _allDataObjs
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
    # Array of transfer functions in server.  If 0 the transfer has been
    # defined but not loaded.  If 1 the transfer function has been named
    # and loaded.
    private variable _first ""     ;# This is the topmost dataset.
    private variable _buffering 0

    # This indicates which isomarkers and transfer function to use when
    # changing markers, opacity, or thickness.
    common _downloadPopup          ;# download options from popup
    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0
    private variable _outline
}

itk::usual VtkContourViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::constructor {hostlist args} {
    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this FixLegend]; list"

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
    $_parser alias legend [itcl::code $this ReceiveLegend]
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
        theta		45
        phi		45
        psi		0 
        zoom		1.0 
        pan-x		0
        pan-y		0
	zoom-x		1.0
	zoom-y		1.0
    }
    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

    array set _settings [subst {
        $this-pan-x		$_view(pan-x)
        $this-pan-y		$_view(pan-y)
        $this-phi		$_view(phi)
        $this-psi		$_view(psi)
        $this-theta		$_view(theta)
        $this-opacity		1
        $this-axes		1
        $this-legend		1
        $this-colormap		1
        $this-edges		0
        $this-isolines		1
        $this-zoom		$_view(zoom)
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

    bind $c <ButtonPress-1> [itcl::code $this BeginSelectionRectangle %x %y]
    bind $c <B1-Motion> [itcl::code $this AdjustSelectionRectangle %x %y]
    bind $c <ButtonRelease-1> [itcl::code $this EndSelectionRectangle %x %y]
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
    bind $itk_component(legend) <Configure> \
        [itcl::code $this EventuallyResizeLegend]

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(view)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w \
        1,0 $itk_component(legend) -fill x 
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(view) -fill both -reqwidth $w \
        0,1 $itk_component(legend) -fill y 
    blt::table configure $itk_component(plotarea) c1 -resize none

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
    puts stderr waiting
    #after 30000
    puts stderr continuing
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::destructor {} {
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !resize
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
    array unset _settings $this-*
}

itcl::body Rappture::VtkContourViewer::DoResize {} {
    if { $_width < 2 } {
	set _width 500
    }
    if { $_height < 2 } {
	set _height 500
    }
    SendCmd "screen size $_width $_height"
    if { $_settings($this-legend) } {
	EventuallyResizeLegend
    }
    set _resizePending 0
}

itcl::body Rappture::VtkContourViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        $_dispatcher event -after 100 !resize
        set _resizePending 1
    }
}

itcl::body Rappture::VtkContourViewer::EventuallyResizeLegend {} {
    if { !$_resizeLegendPending } {
        $_dispatcher event -after 100 !legend
        set _resizeLegendPending 1
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::add {dataobj {settings ""}} {
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
# USAGE: get ?-image view|legend?
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.  The optional "-image"
# flag can also request the internal images being shown.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::get {args} {
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
#       Clients use this to delete a dataobj from the plot.  If no dataobjs
#       are specified, then all dataobjs are deleted.  No data objects are
#       deleted.  They are only removed from the display list.
#
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::delete {args} {
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
itcl::body Rappture::VtkContourViewer::scale {args} {
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
itcl::body Rappture::VtkContourViewer::download {option args} {
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
            set popup .vtkcontourviewerdownload
            if { ![winfo exists .vtkcontourviewerdownload] } {
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
            set popup .vtkcontourviewerdownload
            if {[winfo exists .vtkcontourviewerdownload]} {
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
itcl::body Rappture::VtkContourViewer::Connect {} {
    set _hosts [GetServerList "vtkvis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
    if { $result } {
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
itcl::body Rappture::VtkContourViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# disconnect --
#
itcl::body Rappture::VtkContourViewer::disconnect {} {
    Disconnect
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::VtkContourViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _datasets 
    array unset _colormaps 
}

#
# sendto --
#
itcl::body Rappture::VtkContourViewer::sendto { bytes } {
    SendBytes "$bytes\n"
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::VtkContourViewer::SendCmd {string} {
    if { $_buffering } {
        append _outbuf $string "\n"
    } else {
        foreach line [split $string \n] {
            SendEcho >>line $line
        }
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
itcl::body Rappture::VtkContourViewer::ReceiveImage { args } {
    array set info {
        -token "???"
        -bytes 0
        -type image
    }
    array set info $args
    set bytes [ReceiveBytes $info(-bytes)]
    ReceiveEcho <<line "<read $info(-bytes) bytes"
    if { $info(-type) == "image" } {
        $_image(plot) configure -data $bytes
        #puts stderr "received image [image width $_image(plot)]x[image height $_image(plot)] image>"        
    } elseif { $info(type) == "print" } {
        set tag $this-print-$info(-token)
        set _hardcopy($tag) $bytes
    }
}

# ----------------------------------------------------------------------
# USAGE: FixLegend
#
# Used internally to update the legend area whenever it changes size
# or when the field changes.  Asks the server to send a new legend
# for the current field.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::FixLegend {} {
    puts stderr "FixLegend _first=$_first"
    set _resizeLegendPending 0
    set lineht [font metrics $itk_option(-font) -linespace]
    set c $itk_component(legend)
    set w [expr {[winfo height $itk_component(view)]-20}]
    set h 45
    puts stderr "in fixlegend w=$w h=$h"
    if {$w > 0 && $h > 0 && $_first != "" } {
        set tag [lindex [CurrentDatasets] 0]
	puts stderr "tag=$tag [info exists _dataset2style($tag)]"
        if { [info exists _dataset2style($tag)] } {
            SendCmd "legend $_dataset2style($tag) {} $w $h"
        }
    } else {
        #$itk_component(legend) delete all
    }
}

#
# DrawLegend --
#
#	Draws the legend in it's own canvas which resides to the right
#	of the contour plot area.
#
itcl::body Rappture::VtkContourViewer::DrawLegend {} {
    set c $itk_component(legend)
    set w [winfo width $c]
    set h [winfo height $c]
    puts stderr "DrawLegend w=$w h=$h"
    set lineht [font metrics $itk_option(-font) -linespace]
    
    if { $_settings($this-legend) } {
	if { [$c find withtag "legend"] == "" } {
	    $c create image [expr {$w-2}] [expr {$lineht+2}] -anchor ne \
		-image $_image(legend) -tags "transfunc legend"
	    $c create text [expr {$w-2}] 2 -anchor ne \
		-fill $itk_option(-plotforeground) -tags "vmax legend" \
		-font "Arial 6"
	    $c create text [expr {$w-2}] [expr {$h-2}] -anchor se \
		-fill $itk_option(-plotforeground) -tags "vmin legend" \
		-font "Arial 6"
	}
	# Reset the item coordinates according the current size of the plot.
	$c coords transfunc [expr {$w-2}] [expr {$lineht+2}]
	if { $_limits(vmin) != "" } {
	    $c itemconfigure vmin -text [format %g $_limits(vmin)]
	}
	if { $_limits(vmax) != "" } {
	    $c itemconfigure vmax -text [format %g $_limits(vmax)]
	}
	$c coords vmin [expr {$w-2}] [expr {$h-2}]
	$c coords vmax [expr {$w-2}] 2
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveLegend <tf> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::ReceiveLegend { colormap size} {
    puts stderr "ReceiveLegend colormap=$colormap size=$size"
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend)] } {
            set _image(legend) [image create photo]
        }
        puts stderr "read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
        set src [image create photo -data $bytes]
        blt::winop image rotate $src $_image(legend) 90
        set dst $_image(legend)
	DrawLegend
    }
}

#
# ReceiveDataset --
#
itcl::body Rappture::VtkContourViewer::ReceiveDataset { args } {
    if { ![isconnected] } {
        return
    }
    set option [lindex $args 0]
    switch -- $option {
	"value" {
	    set option [lindex $args 1]
	    switch -- $option {
		"world" {
		    foreach { x y z value } [lrange $args 2 end] break
		    puts stderr "world x=$x y=$y z=$z value=$value"
		}
		"pixel" {
		    foreach { x y value } [lrange $args 2 end] break
		    puts stderr "pixel x=$x y=$y value=$value"
		}
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
itcl::body Rappture::VtkContourViewer::Rebuild {} {

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    EventuallyResize $w $h
    
    set _limits(vmin) ""
    set _limits(vmax) ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [ConvertToVtkData $dataobj $comp]
                set length [string length $bytes]
                append _outbuf "dataset add $tag data follows $length\n"
                append _outbuf $bytes
		append _outbuf "pseudocolor add $tag\n"
		append _outbuf "pseudocolor colormap default $tag\n"
		SetStyles $dataobj $comp
                set _datasets($tag) 1
		foreach {min max} [$dataobj limits z] break
		if { ($_limits(vmin) == "") || ($min < $_limits(vmin)) } {
		    set _limits(vmin) $min
		}
		if { ($_limits(vmax) == "") || ($max < $_limits(vmax)) } {
		    set _limits(vmax) $max
		}
	    }
        }
    }
    #
    # Reset the camera and other view parameters
    #

    set _settings($this-theta) $_view(theta)
    set _settings($this-phi)   $_view(phi)
    set _settings($this-psi)   $_view(psi)
    set _settings($this-pan-x) $_view(pan-x)
    set _settings($this-pan-y) $_view(pan-y)
    set _settings($this-zoom)  $_view(zoom)

    set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
    #SendCmd "camera rotate $xyz"
    PanCamera
    SendCmd "camera mode image"
    SendCmd "camera zoom $_view(zoom)"
    FixSettings opacity
    FixSettings isolines
    FixSettings colormap
    FixSettings axes
    FixSettings edges

    # Nothing to send -- activate the proper ivol
    foreach tag [CurrentDatasets] {
	SendCmd "dataset visible 0 $tag"
    }
    set _first [lindex [get] 0]
    if {"" != $_first} {
        set location [$_first hints camera]
        if { $location != "" } {
            array set view $location
        }
        foreach tag [array names _datasets $_first-*]  {
            SendCmd "dataset visible 1 $tag"
            SendCmd "pseudocolor opacity 1.0 $tag"
        }
    }

    FixLegend 

    # If the first dataset already exists on the server, then make sure we
    # display the proper transfer function in the legend.
    set comp [lindex [$_first components] 0]
    set _buffering 0;                        # Turn off buffering.

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf;                        
    blt::busy release $itk_component(hull)
    set _outbuf "";                        # Clear the buffer.                
}

# ----------------------------------------------------------------------
# USAGE: CurrentDatasets ?-cutplanes?
#
# Returns a list of server IDs for the current datasets being displayed.  This
# is normally a single ID, but it might be a list of IDs if the current data
# object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::CurrentDatasets {{what -all}} {
    set rlist ""
    if { $_first == "" } {
        return
    }
    foreach comp [$_first components] {
        set tag $_first-$comp
        if { [info exists _datasets($tag)] && $_datasets($tag) } {
	    lappend rlist $tag
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
itcl::body Rappture::VtkContourViewer::Zoom {option} {
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
                theta   45
                phi     45
                psi     0
                zoom    1.0
                pan-x   0
                pan-y   0
		zoom-x  1.0
		zoom-y  1.0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
            #SendCmd "camera rotate $xyz"
            PanCamera
            set _settings($this-theta) $_view(theta)
            set _settings($this-phi)   $_view(phi)
            set _settings($this-psi)   $_view(psi)
            set _settings($this-pan-x) $_view(pan-x)
            set _settings($this-pan-y) $_view(pan-y)
            set _settings($this-zoom)  $_view(zoom)
            SendCmd "camera reset all"
        }
    }
}

itcl::body Rappture::VtkContourViewer::PanCamera {} {
    #set x [expr ($_view(pan-x)) / $_limits(xrange)]
    #set y [expr ($_view(pan-y)) / $_limits(yrange)]
    set x $_view(pan-x)
    set y $_view(pan-y)
    #SendCmd "camera pan $x $y"
}


# ----------------------------------------------------------------------
# USAGE: Rotate click <x> <y>
# USAGE: Rotate drag <x> <y>
# USAGE: Rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::Rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(view) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
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

                #
                # Rotate the camera in 3D
                #
                if {$_view(psi) > 90 || $_view(psi) < -90} {
                    # when psi is flipped around, theta moves backwards
                    set dy [expr {-$dy}]
                }
                set theta [expr {$_view(theta) - $dy*180}]
                while {$theta < 0} { set theta [expr {$theta+180}] }
                while {$theta > 180} { set theta [expr {$theta-180}] }

                if {abs($theta) >= 30 && abs($theta) <= 160} {
                    set phi [expr {$_view(phi) - $dx*360}]
                    while {$phi < 0} { set phi [expr {$phi+360}] }
                    while {$phi > 360} { set phi [expr {$phi-360}] }
                    set psi $_view(psi)
                } else {
                    set phi $_view(phi)
                    set psi [expr {$_view(psi) - $dx*360}]
                    while {$psi < -180} { set psi [expr {$psi+360}] }
                    while {$psi > 180} { set psi [expr {$psi-360}] }
                }

                set _view(theta)        $theta
                set _view(phi)          $phi
                set _view(psi)          $psi
                set xyz [Euler2XYZ $theta $phi $psi]
                set _settings($this-theta) $_view(theta)
                set _settings($this-phi)   $_view(phi)
                set _settings($this-psi)   $_view(psi)
                #SendCmd "camera rotate $xyz"
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
itcl::body Rappture::VtkContourViewer::Pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(view)]
    set h [winfo height $itk_component(view)]
    if { $option == "set" } {
        set x [expr (($w - $x) / double($w))]
        set y [expr (($h - $y) / double($h))]
        set _view(pan-x) [expr $_view(pan-x) + $x]
        set _view(pan-y) [expr $_view(pan-y) + $y]
        PanCamera
        set _settings($this-pan-x) $_view(pan-x)
        set _settings($this-pan-y) $_view(pan-y)
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
        set _view(pan-x) [expr $_view(pan-x) - $dx]
        set _view(pan-y) [expr $_view(pan-y) - $dy]
        PanCamera
        set _settings($this-pan-x) $_view(pan-x)
        set _settings($this-pan-y) $_view(pan-y)
    }
    if { $option == "release" } {
        $itk_component(view) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkContourViewer::FixSettings {what {value ""}} {
    switch -- $what {
        "opacity" {
            if {[isconnected]} {
                set val $_settings($this-opacity)
                set sval [expr { 0.01 * double($val) }]
		foreach dataset [CurrentDatasets] {
		    SendCmd "pseudocolor opacity $sval $dataset"
		}
            }
        }
        "isolines" {
            if {[isconnected]} {
		set bool $_settings($this-isolines)
		foreach dataset [CurrentDatasets] {
		    SendCmd "contour2d visible $bool $dataset"
		}
            }
        }
        "edges" {
            if {[isconnected]} {
		set bool $_settings($this-edges)
		foreach dataset [CurrentDatasets] {
		    SendCmd "pseudocolor edges $bool $dataset"
		}
            }
        }
        "colormap" {
            if {[isconnected]} {
		set bool $_settings($this-colormap)
		if { $bool } {
		    set linecolor "0.0 0.0 0.0"
		    set opacity 0.0
		} else {
		    set linecolor "1.0 1.0 1.0"
		    set opacity 1.0
		}
		foreach dataset [CurrentDatasets] {
		    SendCmd "pseudocolor visible $bool $dataset"
		    SendCmd "contour2d linecolor $linecolor $dataset"
		    SendCmd "pseudocolor linecolor $linecolor $dataset"
		}
            }
        }
        "axes" {
            if { [isconnected] } {
		set bool $_settings($this-axes)
                SendCmd "axis visible all $bool"
            }
        }
        "legend" {
            if { $_settings($this-legend) } {
                blt::table $itk_component(plotarea) \
                    0,0 $itk_component(view) -fill both \
                    0,1 $itk_component(legend) -fill y 
                blt::table configure $itk_component(plotarea) c1 -resize none
            } else {
                blt::table forget $itk_component(legend)
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
itcl::body Rappture::VtkContourViewer::SetStyles { dataobj comp } {
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
    SendCmd "contour2d add numcontours $style(-levels) $tag\n"
    SendCmd "pseudocolor colormap $colormap $tag"
    return $colormap
}

#
# BuildColormap --
#
itcl::body Rappture::VtkContourViewer::BuildColormap { colormap dataobj comp } {
    puts stderr "BuildColormap $colormap"
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
itcl::configbody Rappture::VtkContourViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        SendCmd "screen bgcolor $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkContourViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

itcl::body Rappture::VtkContourViewer::limits { colormap } {
    set _limits(min) 0.0
    set _limits(max) 1.0
    if { ![info exists _style2datasets($colormap)] } {
        return [array get _limits]
    }
    set min ""; set max ""
    foreach tag $_style2datasets($colormap) {
        if { ![info exists _datasets($tag)] } {
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


itcl::body Rappture::VtkContourViewer::BuildViewTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    checkbutton $inner.isolines \
        -text "Isolines" \
        -variable [itcl::scope _settings($this-isolines)] \
        -command [itcl::code $this FixSettings isolines] \
        -font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
        -font "Arial 9"

    checkbutton $inner.colormap \
        -text "Colormap" \
        -variable [itcl::scope _settings($this-colormap)] \
        -command [itcl::code $this FixSettings colormap] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings($this-legend)] \
        -command [itcl::code $this FixSettings legend] \
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
        1,0 $inner.colormap -columnspan 4 -anchor w -pady 2 \
        3,0 $inner.isolines -columnspan 4 -anchor w -pady 2 \
        4,0 $inner.legend  -columnspan 4 -anchor w \
        6,0 $inner.edges -columnspan 4 -anchor w -pady 2 \
        7,0 $inner.clear -anchor e -pady 2 \
        7,1 $inner.opacity -columnspan 2 -pady 2 -fill x\
        7,3 $inner.opaque -anchor w -pady 2 

    blt::table configure $inner r* -resize none
    blt::table configure $inner r8 -resize expand
}

itcl::body Rappture::VtkContourViewer::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    $inner configure -borderwidth 4

    set labels { phi theta psi pan-x pan-y zoom }
    set row 0
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _settings($this-$tag)]
        bind $inner.${tag} <KeyPress-Return> \
            [itcl::code $this camera set ${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }
    blt::table configure $inner c0 c1 -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}


#
#  camera -- 
#
itcl::body Rappture::VtkContourViewer::camera {option args} {
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
                "pan-x" - "pan-y" {
                    set _view($who) $_settings($this-$who)
                    PanCamera
                }
                "phi" - "theta" - "psi" {
                    set _view($who) $_settings($this-$who)
                    set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
                    #SendCmd "camera rotate $xyz"
                }
                "zoom" {
                    set _view($who) $_settings($this-$who)
                    #SendCmd "camera zoom $_view(zoom)"
                }
            }
        }
    }
}

itcl::body Rappture::VtkContourViewer::ConvertToVtkData { dataobj comp } {
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

#
# MarchingAnts --
#
#	Called from "after" timer events.  This routine is changes
#	the dash offset of the selection outline rectangle to simulate
#	marching ants.
#
itcl::body Rappture::VtkContourViewer::MarchingAnts {} {
    set c $itk_component(view) 
    if { [winfo exists $c] && ![winfo viewable $c] } { 
	KillSelectionRectangle
	return
    }
    $c itemconfigure $_outline(id) -dashoffset $_outline(offset)
    incr _outline(offset) 
    set _outline(afterId) [after 100 [itcl::code $this MarchingAnts]]
}

#
# BeginSelectionRectangle --
#
#	Called from ButtonPress-1 events.  This routine creates the
#	the selection outline rectangle.  It also clears the selection 
#	in the sensor tree.
#
itcl::body Rappture::VtkContourViewer::BeginSelectionRectangle { x y } {
    set c $itk_component(view)
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    if { $_outline(id) >= 0 } {
	after cancel $_outline(afterId)
	set _outline(afterId) -1
	$c delete $_outline(id)
	set _outline(id) -1
    }
    set _outline(offset) 0
    set _outline(x1) $x
    set _outline(y1) $y
    set _outline(id) \
	[$c create line $x $y $x $y -dash "4 4" -width 2 -fill black]
    MarchingAnts 
}

#
# AdjustSelectionRectangle --
#
#	Called from B1-Motion events.  This routine redraws the selection
#	outline rectangle and redraws the sensors using the "selected"
#	icon.
#
itcl::body Rappture::VtkContourViewer::AdjustSelectionRectangle { x y } {
    set c $itk_component(view)
    set x [$c canvasx $x]
    set y [$c canvasy $y]
    $c coords $_outline(id) $_outline(x1) $_outline(y1) $_outline(x1) $y \
	$x $y $x $_outline(y1) $_outline(x1) $_outline(y1)
    set _outline(x2) $x
    set _outline(y2) $y
}

#
# KillSelectionRectangle --
#
#	Removes the outline rectangle and adjusts the sensor icon
#	according to the sensor's selected/deselected status.
#
itcl::body Rappture::VtkContourViewer::KillSelectionRectangle { } {
    after cancel $_outline(afterId)
    set _outline(afterId) -1
    set c $itk_component(view) 
    $c delete $_outline(id)
    set _outline(id) -1
}

#
# EndSelectionRectangle --
#
#	Called from the ButtonRelease event to finish the outline of the
#	selection rectangle.  If the outline is too small (less than 10
#	pixels in length or width) the outline is removed and the selection
#	is ignored.  Otherwise a popup menu is automatically generated and
#	displayed.
#
itcl::body Rappture::VtkContourViewer::EndSelectionRectangle { x y } {
    set c $itk_component(view)
    AdjustSelectionRectangle $x $y
    set cx [$c canvasx $x]
    set cy [$c canvasy $y]
    set cw [winfo width $c]
    set ch [winfo height $c]
    if { abs($_outline(x1) - $cx) < 10 && abs($_outline(y1) - $cy) < 10 }  {
	KillSelectionRectangle
    } else {
	set w [expr $_outline(x2) - $_outline(x1)]
	set h [expr $_outline(y2) - $_outline(y1)]
	# Convert the zoom 
	set _view(zoom-x) [expr $cw / $w * $_view(zoom-x)]
	set _view(zoom-y) [expr $ch / $h * $_view(zoom-y)]
	set w [expr $w * $_view(zoom-x)]
	set h [expr $h * $_view(zoom-y)]
	SendCmd "camera ortho $_outline(x1) $_outline(y1) $w $h"
	KillSelectionRectangle
    }
}

itcl::body Rappture::VtkContourViewer::GetVtkData { args } {
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

itcl::body Rappture::VtkContourViewer::GetImage { args } {
    if { [image width $_image(download)] > 0 && 
	 [image height $_image(download)] > 0 } {
	set bytes [$_image(download) data -format "jpeg -quality 100"]
	set bytes [Rappture::encoding::decode -as b64 $bytes]
	return [list .jpg $bytes]
    }
    return ""
}

itcl::body Rappture::VtkContourViewer::BuildDownloadPopup { popup command } {
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

