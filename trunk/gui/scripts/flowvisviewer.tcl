# ----------------------------------------------------------------------
#  COMPONENT: flowvisviewer - 3D flow rendering
#
#
# This widget performs volume and flow rendering on 3D scalar/vector datasets.
# It connects to the Flowvis server running on a rendering farm, transmits
# data, and displays the results.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
    public method flow {option}
    public method get {args}
    public method isconnected {}
    public method limits { tf }
    public method overmarker { m x }
    public method parameters {title args} { 
        # do nothing 
    }
    public method rmdupmarker { m x }
    public method scale {args}
    public method sendto { cmd }
    public method updatetransferfuncs {}

    protected method Connect {}
    protected method CurrentVolumeIds {{what -all}}
    protected method Disconnect {}
    protected method Resize {}
    protected method ResizeLegend {}
    protected method FixSettings {what {value ""}}
    protected method Pan {option x y}
    protected method Rebuild {}
    protected method ReceiveData { args }
    protected method ReceiveImage { args }
    protected method ReceiveLegend { tf vmin vmax size }
    protected method Rotate {option x y}
    protected method SendCmd {string}
    protected method SendDataObjs {}
    protected method SendTransferFuncs {}
    protected method Slice {option args}
    protected method SlicerTip {axis}
    protected method Zoom {option}

    # soon to be removed.
    protected method Flow {option args}
    protected method Play {}
    protected method Pause {}


    # The following methods are only used by this class.

    private method AddIsoMarker { x y }
    private method BuildCameraTab {}
    private method BuildCutplanesTab {}
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    private method ComputeTransferFunc { tf }
    private method EventuallyResize { w h } 
    private method EventuallyGoto { nSteps } 
    private method EventuallyResizeLegend { } 
    private method FlowCmd { dataobj comp nbytes extents }
    private method GetMovie { widget width height }
    private method GetPngImage { widget width height }
    private method NameTransferFunc { dataobj comp }
    private method PanCamera {}
    private method ParseLevelsOption { tf levels }
    private method ParseMarkersOption { tf markers }
    private method WaitIcon { option widget }
    private method str2millisecs { value } 
    private method millisecs2str { value } 
    private method GetFlowInfo { widget }
    private method particles { tag name } 
    private method box { tag name } 
    private method streams { tag name } 
    private method arrows { tag name } 

    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _allDataObjs
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _serverObjs   ;# maps dataobj-component to volume ID 
                                    # in the server
    private variable _sendobjs ""  ;# list of data objs to send to server
    private variable _recvObjs  ;# list of data objs to send to server
    private variable _obj2style    ;# maps dataobj-component to transfunc
    private variable _style2objs   ;# maps tf back to list of 
                                    # dataobj-components using the tf.
    private variable _obj2flow;		# Maps dataobj-component to a flow.

    private variable _click        ;# info used for rotate operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private variable _isomarkers    ;# array of isosurface level values 0..1
    private common   _settings
    private variable _activeTf ""  ;# The currently active transfer function.
    private variable _first ""     ;# This is the topmost volume.
    private variable _buffering 0
    private variable _nextToken 0
    private variable _icon 0
    private variable _flow
    # This 
    # indicates which isomarkers and transfer
    # function to use when changing markers,
    # opacity, or thickness.
    private common _downloadPopup          ;# download options from popup

    private common _hardcopy
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _resizeLegendPending 0
    private variable _gotoPending 0
}

itk::usual FlowvisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::constructor { hostlist args } {
    set _serverType "nanovis"

    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this ResizeLegend]; list"

    # Send dataobjs event
    $_dispatcher register !send_dataobjs
    $_dispatcher dispatch $this !send_dataobjs \
        "[itcl::code $this SendDataObjs]; list"

    # Send transferfunctions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
        "[itcl::code $this SendTransferFuncs]; list"

    # Rebuild event.
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event.
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this Resize]; list"

    $_dispatcher register !play
    $_dispatcher dispatch $this !play "[itcl::code $this flow next]; list"
    
    # Draw legend event
    $_dispatcher register !goto
    $_dispatcher dispatch $this !goto "[itcl::code $this flow goto2]; list"

    $_dispatcher register !movietimeout
    $_dispatcher register !waiticon

    set _flow(state) 0

    set _outbuf ""

    array set _downloadPopup {
        format draft
    }
    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias data [itcl::code $this ReceiveData]

    # Initialize the view to some default parameters.
    array set _view {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        pan-x	0
        pan-y	0
    }
    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

    array set _settings [subst {
        $this-arrows		0
        $this-currenttime	0
        $this-duration		1:00
        $this-loop		0
        $this-pan-x		$_view(pan-x)
        $this-pan-y		$_view(pan-y)
        $this-phi		$_view(phi)
        $this-play		0
        $this-psi		$_view(psi)
        $this-speed		500
        $this-step		0
        $this-streams		0
        $this-theta		$_view(theta)
        $this-volume		1
        $this-xcutplane		0
        $this-xcutposition	0
        $this-ycutplane		0
        $this-ycutposition	0
        $this-zcutplane		0
        $this-zcutposition	0
        $this-zoom		$_view(zoom)
    }]

    itk_component add 3dview {
        label $itk_component(plotarea).view -image $_image(plot) \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth  -background
    }
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
            -command [itcl::code $this FixSettings volume] \
            -variable [itcl::scope _settings($this-volume)]
    }
    $itk_component(volume) select
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    pack $itk_component(volume) -padx 2 -pady 2

    BuildViewTab
    BuildVolumeTab
    BuildCutplanesTab
    BuildCameraTab

    bind $itk_component(3dview) <Configure> \
        [itcl::code $this EventuallyResize %w %h]

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
        [itcl::code $this EventuallyResizeLegend]

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w 10000
    pack forget $itk_component(3dview)
    blt::table $itk_component(plotarea) \
        0,0 $itk_component(3dview) -fill both -reqwidth $w \
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
            -variable [itcl::scope _settings($this-play)] \
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
            -variable [itcl::scope _settings($this-loop)]
    }
    Rappture::Tooltip::for $itk_component(loop) \
        "Play continuously"

    itk_component add dial {
        Rappture::Flowdial $itk_component(flowcontrols).dial \
            -length 10 -valuewidth 0 -valuepadding 0 -padding 6 \
            -linecolor "" -activelinecolor "" \
            -min 0.0 -max 1.0 \
            -variable [itcl::scope _settings($this-currenttime)] \
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
            -textvariable [itcl::scope _settings($this-duration)] \
            -bg white -width 6 -font "arial 9"
    } {
        usual
        ignore -highlightthickness -background
    }
    bind $itk_component(duration) <Return> [itcl::code $this flow duration]
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
itcl::body Rappture::FlowvisViewer::destructor {} {
    set _sendobjs ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    $_dispatcher cancel !send_transfunc
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
    array unset _settings $this-*
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -width 1
        -linestyle solid
        -brightness 0
        -raise 0
        -description ""
        -param ""
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
    foreach comp [$dataobj components] {
        set flowobj [$dataobj flowhints $comp]
        if { $flowobj == "" } {
            puts stderr "no flowhints $dataobj-$comp"
            continue
        }
        set _obj2flow($dataobj-$comp) $flowobj
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
# USAGE: get ?-image 3dview|legend?
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
            foreach comp [$dataobj components] {
                array unset _limits $dataobj-$comp-*
            }
            set _dlist [lreplace $_dlist $pos $pos]
            array unset _obj2ovride $dataobj-*
            array unset _obj2flow $dataobj-*
            array unset _serverObjs $dataobj-*
            array unset _obj2style $dataobj-*
            set changed 1
        }
    }
    # If anything changed, then rebuild the plot
    if {$changed} {
        # Repair the reverse lookup 
        foreach tf [array names _style2objs] {
            set list {}
            foreach {dataobj comp} $_style2objs($tf) break
            if { [info exists _serverObjs($dataobj-$comp)] } {
                lappend list $dataobj $comp
            }
            if { $list == "" } {
                array unset _style2objs $tf
            } else {
                set _style2objs($tf) $list
            }
        }
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
itcl::body Rappture::FlowvisViewer::download {option args} {
    set popup .flowvisviewerdownload
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
            if {![winfo exists $popup]} {
                # if we haven't created the popup yet, do it now
                Rappture::Balloon $popup \
                    -title "[Rappture::filexfer::label downloadWord] as..."
                set inner [$popup component inner]
                label $inner.summary -text "" -anchor w
                pack $inner.summary -side top
                set img $_image(plot)
                set res "[image width $img]x[image height $img]"
                radiobutton $inner.draft -text "Image (draft $res)" \
                    -variable Rappture::FlowvisViewer::_downloadPopup(format) \
                    -value draft
                pack $inner.draft -anchor w

                set res "640x480"
                radiobutton $inner.medium -text "Movie (standard $res)" \
                    -variable Rappture::FlowvisViewer::_downloadPopup(format) \
                    -value $res
                pack $inner.medium -anchor w

                set res "1024x768"
                radiobutton $inner.high -text "Movie (high quality $res)" \
                    -variable Rappture::FlowvisViewer::_downloadPopup(format) \
                    -value $res
                pack $inner.high -anchor w
                button $inner.go -text [Rappture::filexfer::label download] \
                    -command [lindex $args 0]
                pack $inner.go -pady 4
                $inner.draft select
            } else {
                set inner [$popup component inner]
            }
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            set word [Rappture::filexfer::label downloadWord]
            $inner.summary configure -text "$word $num in the following format:"
            update idletasks ;# fix initial sizes
            return $popup
        }
        now {
            if { [winfo exists $popup] } {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                draft {
                    # Get the image data (as base64) and decode it back to
                    # binary.  This is better than writing to temporary
                    # files.  When we switch to the BLT picture image it
                    # won't be necessary to decode the image data.
                    set bytes [$_image(plot) data -format "jpeg -quality 100"]
                    set bytes [Rappture::encoding::decode -as b64 $bytes]
                    return [list .jpg $bytes]
                }
                "640x480" {
                    return [$this GetMovie [lindex $args 0] 640 480]
                }
                "1024x768" {
                    return [$this GetMovie [lindex $args 0] 1024 768]
                }
                default {
                    error "bad download format $_downloadPopup(format)"
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
    set result [VisViewer::Connect $_hosts]
    if { $result } {
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
# sendto --
#
itcl::body Rappture::FlowvisViewer::sendto { bytes } {
    SendBytes "$bytes\n"
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::FlowvisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set _outbuf ""
    array unset _serverObjs
    set _sendobjs ""
}

#
# SendCmd
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::FlowvisViewer::SendCmd { string } {
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
# USAGE: SendDataObjs
#
# Used internally to send a series of volume objects off to the
# server.  Sends each object, a little at a time, with updates in
# between so the interface doesn't lock up.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::SendDataObjs {} {
    blt::busy hold $itk_component(hull)
    foreach dataobj $_sendobjs {
        foreach comp [$dataobj components] {
            # Send the data as one huge base64-encoded mess -- yuck!
            set data [$dataobj blob $comp]
            set nbytes [string length $data]
            set extents [$dataobj extents $comp]

            # I have a field. Is a vector field or a volume field?
            if { $extents == 1 } {
                set cmd "volume data follows $nbytes $dataobj-$comp\n"
            } else {
                set cmd [FlowCmd $dataobj $comp $nbytes $extents]
                if { $cmd == "" } {
                    puts stderr "no command"
                    continue
                }
            }
            f { ![SendBytes $cmd] } {
                puts stderr "can't send"
                return
            }
            if { ![SendBytes $data] } {
                puts stderr "can't send"
                return
            }
            NameTransferFunc $dataobj $comp
            set _recvObjs($dataobj-$comp) 1
        }
    }
    set _sendobjs ""
    blt::busy release $itk_component(hull)

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    # activate the proper volume
    set _first [lindex [get] 0]
    if { "" != $_first } {
        set axis [$_first hints updir]
        if {"" != $axis} {
            SendCmd "up $axis"
        }

        if 0 {
        set location [$_first hints camera]
        if { $location != "" } {
            array set _view $location
        }
        set _settings($this-theta) $_view(theta)
        set _settings($this-phi)   $_view(phi)
        set _settings($this-psi)   $_view(psi)
        set _settings($this-pan-x) $_view(pan-x)
        set _settings($this-pan-y) $_view(pan-y)
        set _settings($this-zoom)  $_view(zoom)
        set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
        SendCmd "camera angle $xyz"
        PanCamera
        SendCmd "camera zoom $_view(zoom)"
        }
        # The active transfer function is by default the first component of
        # the first data object.  This assumes that the data is always
        # successfully transferred.
        set comp [lindex [$_first components] 0]
        set _activeTf [lindex $_obj2style($_first-$comp) 0]
    }

    SendCmd "flow reset"

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    SendBytes $_outbuf;			
    set _buffering 0;			# Turn off buffering.
    set _outbuf "";			# Clear the buffer.		
}

# ----------------------------------------------------------------------
# USAGE: SendTransferFuncs
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::SendTransferFuncs {} {
    if { $_activeTf == "" } {
        puts stderr "no active tf"
        return
    }
    set tf $_activeTf
    if { $_first == "" } {
        puts stderr "no first"
        return
    }

    # Insure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the active transfer-function.  Update the
    # values in the _settings varible.
    set value $_settings($this-opacity)
    set opacity [expr { double($value) * 0.01 }]
    set _settings($this-$tf-opacity) $opacity
    set value $_settings($this-thickness)
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($value) * 0.0001}]
    set _settings($this-$tf-thickness) $thickness

    foreach key [array names _obj2style $_first-*] {
        if { [info exists _obj2style($key)] } {
            foreach tf $_obj2style($key) {
                ComputeTransferFunc $tf
            }
        }
    }
    EventuallyResizeLegend
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes $size -type $type -token $token
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
    ReceiveEcho <<line "<read $info(-bytes) bytes"
    switch -- $info(-type)  {
        "image" {
            $_image(plot) configure -data $bytes
            #puts stderr "image received [image width $_image(plot)] by [image height $_image(plot)]"
        } 
        "print" {
            set tag $this-$info(-token)
            set _hardcopy($tag) $bytes
        } 
        "movie" {
            puts stderr "read [string length $bytes] bytes type=$info(-type) token=$info(-token)"
            set tag $this-$info(-token)
            set _hardcopy($tag) $bytes
        }
        default {
            puts stderr "unknown download type $info(-type)"
        }
    }
}

#
# ReceiveLegend --
#
#       The procedure is the response from the render server to each "legend"
#       command.  The server sends back a "legend" command invoked our
#       the slave interpreter.  The purpose is to collect data of the image 
#       representing the legend in the canvas.  In addition, the isomarkers
#       of the active transfer function are displayed.
#
#       I don't know is this is the right place to display the isomarkers.
#       I don't know all the different paths used to draw the plot. There's 
#       "Rebuild", "add", etc.
#
itcl::body Rappture::FlowvisViewer::ReceiveLegend { tag vmin vmax size } {
    if { ![isconnected] } {
        return
    }
    #puts stderr "receive legend $tag $vmin $vmax $size"
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

    set c $itk_component(legend)
    set w [winfo width $c]
    set h [winfo height $c]
    set lx 10
    set ly [expr {$h - 1}]
    if {"" == [$c find withtag transfunc]} {
        $c create image 10 10 -anchor nw \
            -image $_image(legend) -tags transfunc
        $c create text $lx $ly -anchor sw \
            -fill $itk_option(-plotforeground) -tags "limits vmin"
        $c create text [expr {$w-$lx}] $ly -anchor se \
            -fill $itk_option(-plotforeground) -tags "limits vmax"
        $c lower transfunc
        $c bind transfunc <ButtonRelease-1> \
            [itcl::code $this AddIsoMarker %x %y]
    }
    # Display the markers used by the active transfer function.
    set tf $_obj2style($tag)
    array set limits [limits $tf]
    $c itemconfigure vmin -text [format %.2g $limits(vmin)]
    $c coords vmin $lx $ly

    $c itemconfigure vmax -text [format %.2g $limits(vmax)]
    $c coords vmax [expr {$w-$lx}] $ly

    if { [info exists _isomarkers($tf)] } {
        foreach m $_isomarkers($tf) {
            $m visible yes
        }
    }
}

#
# ReceiveData --
#
#	The procedure is the response from the render server to each "data
#	follows" command.  The server sends back a "data" command invoked our
#	the slave interpreter.  The purpose is to collect the min/max of the
#	volume sent to the render server.  Since the client (flowvisviewer)
#	doesn't parse 3D data formats, we rely on the server (flowvis) to
#	tell us what the limits are.  Once we've received the limits to all
#	the data we've sent (tracked by _recvObjs) we can then determine
#	what the transfer functions are for these # volumes.
#
#       Note: There is a considerable tradeoff in having the server report
#             back what the data limits are.  It means that much of the code
#             having to do with transfer-functions has to wait for the data
#             to come back, since the isomarkers are calculated based upon
#             the data limits.  The client code is much messier because of
#             this.  The alternative is to parse any of the 3D formats on the
#             client side.
#
itcl::body Rappture::FlowvisViewer::ReceiveData { args } {
    if { ![isconnected] } {
        return
    }
    # Arguments from server are name value pairs. Stuff them in an array.
    array set values $args
    set tag $values(tag)
    set parts [split $tag -]
    set dataobj [lindex $parts 0]
    set _serverObjs($tag) 0
    set _limits($tag-min)  $values(min);  # Minimum value of the volume.
    set _limits($tag-max)  $values(max);  # Maximum value of the volume.
    unset _recvObjs($tag)
    if { [array size _recvObjs] == 0 } {
        updatetransferfuncs
    }
}

#
# Rebuild --
#
# Called automatically whenever something changes that affects the data
# in the widget.  Clears any existing data and rebuilds the widget to
# display new data.  
#
itcl::body Rappture::FlowvisViewer::Rebuild {} {

    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names _isomarkers] {
        foreach m $_isomarkers($tf) {
            $m visible no
        }
    }

    set _first ""
    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (which automatically
    # generates a new call to Rebuild).   
    set _buffering 1

    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    EventuallyResize $w $h

    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            # Send the data as one huge base64-encoded mess -- yuck!
            set data [$dataobj blob $comp]
            set nbytes [string length $data]
            set extents [$dataobj extents $comp]
            # I have a field. Is a vector field or a volume field?
            if { $extents == 1 } {
                set cmd "volume data follows $nbytes $dataobj-$comp\n"
            } else {
                set cmd [FlowCmd $dataobj $comp $nbytes $extents]
                if { $cmd == "" } {
                    puts stderr "no command"
                    continue
                }
            }
            append _outbuf $cmd
            append _outbuf $data
            NameTransferFunc $dataobj $comp
            set _recvObjs($dataobj-$comp) 1
        }
    }

    set _first [lindex [get] 0]

    # Reset the camera and other view parameters
    FixSettings isosurface
    FixSettings grid
    FixSettings axes
    FixSettings volume
    FixSettings outline
    FixSettings light
    FixSettings transp
    
    # nothing to send -- activate the proper volume
    if {"" != $_first} {
        FixSettings light
        FixSettings transp
        set axis [$_first hints updir]
        if {"" != $axis} {
            SendCmd "up $axis"
        }
        set location [$_first hints camera]
        if { $location != "" } {
            array set _view $location
        }
    }
    set _settings($this-theta) $_view(theta)
    set _settings($this-phi)   $_view(phi)
    set _settings($this-psi)   $_view(psi)
    set _settings($this-pan-x) $_view(pan-x)
    set _settings($this-pan-y) $_view(pan-y)
    set _settings($this-zoom)  $_view(zoom)

    set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
    SendCmd "camera angle $xyz"
    PanCamera
    SendCmd "camera zoom $_view(zoom)"

    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            NameTransferFunc $dataobj $comp
        }
    }

    # nothing to send -- activate the proper ivol
    set _first [lindex [get] 0]
    if {"" != $_first} {
        set axis [$_first hints updir]
        if {"" != $axis} {
            SendCmd "up $axis"
        }
        set location [$_first hints camera]
        if { $location != "" } {
            array set _view $location
        }
        set comp [lindex [$_first components] 0]
        set _activeTf [lindex $_obj2style($_first-$comp) 0]
    }


    # sync the state of slicers
    set vols [CurrentVolumeIds -cutplanes]
    foreach axis {x y z} {
        SendCmd "cutplane state $_settings($this-${axis}cutplane) $axis $vols"
        set pos [expr {0.01*$_settings($this-${axis}cutposition)}]
        SendCmd "cutplane position $pos $axis $vols"
    }
    SendCmd "volume data state $_settings($this-volume)"
    EventuallyResizeLegend

    # Actually write the commands to the server socket.  If it fails, we don't
    # care.  We're finished here.
    blt::busy hold $itk_component(hull)
    SendBytes $_outbuf
    blt::busy release $itk_component(hull)
    set _buffering 0;			# Turn off buffering.
    set _outbuf "";			# Clear the buffer.		
}

# ----------------------------------------------------------------------
# USAGE: CurrentVolumeIds ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::CurrentVolumeIds {{what -all}} {
    return ""
    if { $_first == "" } {
        return
    }
    foreach key [array names _serverObjs *-*] {
        if {[string match $_first-* $key]} {
            array set style {
                -cutplanes 1
            }
            foreach {dataobj comp} [split $key -] break
            array set style [lindex [$dataobj components -style $comp] 0]
            if {$what != "-cutplanes" || $style(-cutplanes)} {
                lappend rlist $_serverObjs($key)
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
            set _view(zoom) [expr {$_view(zoom)*1.25}]
            set _settings($this-zoom) $_view(zoom)
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)*0.8}]
            set _settings($this-zoom) $_view(zoom)
        }
        "reset" {
            array set _view {
                theta   45
                phi     45
                psi     0
                zoom	1.0
                pan-x	0
                pan-y	0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
            SendCmd "camera angle $xyz"
            PanCamera
            set _settings($this-theta) $_view(theta)
            set _settings($this-phi)   $_view(phi)
            set _settings($this-psi)   $_view(psi)
            set _settings($this-pan-x) $_view(pan-x)
            set _settings($this-pan-y) $_view(pan-y)
            set _settings($this-zoom)  $_view(zoom)
        }
    }
    SendCmd "camera zoom $_view(zoom)"
}

itcl::body Rappture::FlowvisViewer::PanCamera {} {
    #set x [expr ($_view(pan-x)) / $_limits(xrange)]
    #set y [expr ($_view(pan-y)) / $_limits(yrange)]
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
itcl::body Rappture::FlowvisViewer::Rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
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
                SendCmd "camera angle $xyz"
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
itcl::body Rappture::FlowvisViewer::Pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
        set x [expr $x / double($w)]
        set y [expr $y / double($h)]
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
        $itk_component(3dview) configure -cursor hand1
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
        $itk_component(3dview) configure -cursor ""
    }
}


# ----------------------------------------------------------------------
# USAGE: Flow movie record|stop|play ?on|off|toggle?
#
# Called when the user clicks on the record, stop or play buttons
# for flow visualization.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Flow {option args} {
    switch -- $option {
        movie {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"Flow movie record|stop|play ?on|off|toggle?\""
            }
            set action [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            set current [State $action]
            if {$op == "toggle"} {
                if {$current == "on"} {
                    set op "off"
                } else {
                    set op "on"
                }
            }
            set cmds ""
            switch -- $action {
                record {
                    if { [$itk_component(rewind) cget -relief] != "sunken" } {
                        $itk_component(rewind) configure -relief sunken 
                        $itk_component(stop) configure -relief raised 
                        $itk_component(play) configure -relief raised 
                        set inner $itk_component(settingsFrame)
                        set frames [$inner.framecnt value]
                        set _settings(nsteps) $frames
                        set cmds "flow capture $frames"
                        SendCmd $cmds
                    }
                }
                stop {
                    if { [$itk_component(stop) cget -relief] != "sunken" } {
                        $itk_component(rewind) configure -relief raised 
                        $itk_component(stop) configure -relief sunken 
                        $itk_component(play) configure -relief raised 
                        _pause
                        set cmds "flow reset"
                        SendCmd $cmds
                    }
                }
                play {
                    if { [$itk_component(play) cget -relief] != "sunken" } {
                        $itk_component(rewind) configure -relief raised
                        $itk_component(stop) configure -relief raised 
                        $itk_component(play) configure \
                            -image [Rappture::icon flow-pause] \
                            -relief sunken 
                        bind $itk_component(play) <ButtonPress> \
                            [itcl::code $this _pause]
                        flow next
                    }
                }
                default {
                    error "bad option \"$option\": should be one of record|stop|play"
                }

            }
        }
        default {
            error "bad option \"$option\": should be movie"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Play
#
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Play {} {
    SendCmd "flow next"
    set delay [expr {int(ceil(pow($_settings(speed)/10.0+2,2.0)*15))}]
    $_dispatcher event -after $delay !play
}

# ----------------------------------------------------------------------
# USAGE: Pause
#
# Invoked when the user hits the "pause" button to stop playing the
# current sequence of frames as a movie.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Pause {} {
    $_dispatcher cancel !play

    # Toggle the button to "play" mode
    $itk_component(play) configure \
        -image [Rappture::icon flow-start] \
        -relief raised 
    bind $itk_component(play) <ButtonPress> \
        [itcl::code $this Flow movie play toggle]
}

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::FixSettings {what {value ""}} {
    switch -- $what {
        light {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set diffuse [expr {0.1*$_settings($this-light)}]
                set specular [expr {sqrt($_settings($this-light)+1.0)}]
                SendCmd "$tag configure -diffuse $diffuse -specular $specular"
            }
        }
        transp {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                set opacity [expr {0.2*$_settings($this-transp)+1}]
                SendCmd "$tag configure -opacity $opacity"
            }
        }
        opacity {
            if { $_first != "" && $_activeTf != "" } {
                set opacity [expr { 0.01 * double($_settings($this-opacity)) }]
                set tf $_activeTf
                set _settings($this-$tf-opacity) $opacity
                updatetransferfuncs
            }
        }

        thickness {
            if { $_first != "" && $_activeTf != "" } {
                set val $_settings($this-thickness)
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                set tf $_activeTf
                set _settings($this-$tf-thickness) $sval
                updatetransferfuncs
            }
        }
        "outline" {
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                SendCmd "$tag configure -outline $_settings($this-outline)"
            }
        }
        "isosurface" {
            if { [isconnected] } {
                SendCmd "volume shading isosurface $_settings($this-isosurface)"
            }
        }
        "grid" {
            if { [isconnected] } {
                SendCmd "grid visible $_settings($this-grid)"
            }
        }
        "axes" {
            if { [isconnected] } {
                SendCmd "axis visible $_settings($this-axes)"
            }
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
            if { $_first != "" } {
                set comp [lindex [$_first components] 0]
                set tag $_first-$comp
                SendCmd "$tag configure -volume $_settings($this-volume)"
            }
        }
        "xcutplane" - "ycutplane" - "zcutplane" {
            set axis [string range $what 0 0]
            set bool $_settings($this-$what)
            if { [isconnected] } {
                set vols [CurrentVolumeIds -cutplanes] 
                SendCmd "cutplane state $bool $axis $vols"
            }
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
# USAGE: ResizeLegend
#
# Used internally to update the legend area whenever it changes size
# or when the field changes.  Asks the server to send a new legend
# for the current field.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::ResizeLegend {} {
    set _resizeLegendPending 0
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {$_width-20}]
    set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]

    if { $_first == "" } {
        return
    }
    set comp [lindex [$_first components] 0]
    set tag $_first-$comp
    #set _activeTf [lindex $_obj2style($tag) 0]
    if {$w > 0 && $h > 0 && "" != $_activeTf} {
        #SendCmd "legend $_activeTf $w $h"
        SendCmd "$tag legend $w $h"
    } else {
    # Can't do this as this will remove the items associated with the
    # isomarkers.

    #$itk_component(legend) delete all
    }
}

#
# NameTransferFunc --
#
#       Creates a transfer function name based on the <style> settings in the
#       library run.xml file. This placeholder will be used later to create
#       and send the actual transfer function once the data info has been sent
#       to us by the render server. [We won't know the volume limits until the
#       server parses the 3D data and sends back the limits via ReceiveData.]
#
#       FIXME: The current way we generate transfer-function names completely
#              ignores the -markers option.  The problem is that we are forced
#              to compute the name from an increasing complex set of values:
#              color, levels, marker, opacity.  I think we're stuck doing it
#              now.
#
itcl::body Rappture::FlowvisViewer::NameTransferFunc { dataobj comp } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
        -light 40
        -transp 50
    }
    array set style [lindex [$dataobj components -style $comp] 0]
    set _settings($this-light) $style(-light)
    set _settings($this-transp) $style(-transp)
    set _settings($this-opacity) [expr $style(-opacity) * 100]
    set tf "$style(-color):$style(-levels):$style(-opacity)"
    set _obj2style($dataobj-$comp) $tf
    lappend _style2objs($tf) $dataobj $comp
    return $tf
}

#
# ComputeTransferFunc --
#
#   Computes and sends the transfer function to the render server.  It's
#   assumed that the volume data limits are known and that the global
#   transfer-functions slider values have be setup.  Both parts are
#   needed to compute the relative value (location) of the marker, and
#   the alpha map of the transfer function.
#
itcl::body Rappture::FlowvisViewer::ComputeTransferFunc { tf } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
        -light 40
        -transp 50
    }
    set dataobj ""; set comp ""
    foreach {dataobj comp} $_style2objs($tf) break
    if { $dataobj == "" } {
        return 0
    }
    array set style [lindex [$dataobj components -style $comp] 0]


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
    #        color, levels, marker, opacity.  I think the cow's out of the
    #        barn on this one.

    if { ![info exists _isomarkers($tf)] } {
        # Have to defer creation of isomarkers until we have data limits
        if { [info exists style(-markers)] } {
            ParseMarkersOption $tf $style(-markers)
        } else {
            ParseLevelsOption $tf $style(-levels)
        }
    }
    if {$style(-color) == "rainbow"} {
        set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    if { [info exists style(-nonuniformcolors)] } {
        foreach { value color } $style(-nonuniformcolors) {
            append cmap "$value [Color2RGB $color] "
        }
    } else {
        set clist [split $style(-color) :]
        set cmap "0.0 [Color2RGB white] "
        for {set i 0} {$i < [llength $clist]} {incr i} {
            set x [expr {double($i+1)/([llength $clist]+1)}]
            set color [lindex $clist $i]
            append cmap "$x [Color2RGB $color] "
        }
        append cmap "1.0 [Color2RGB $color]"
    }
    set tag $this-$tf
    if { ![info exists _settings($tag-opacity)] } {
        set _settings($tag-opacity) $style(-opacity)
    }
    set max $_settings($tag-opacity)
    
    set isovalues {}
    foreach m $_isomarkers($tf) {
        lappend isovalues [$m relval]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists _settings($tag-thickness)]} {
        set _settings($tag-thickness) 0.05
    }
    set delta $_settings($tag-thickness)

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
    SendCmd "transfunc define $tf { $cmap } { $wmap }\n"
    return [SendCmd "$dataobj-$comp configure -transferfunction $tf\n"]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #SendCmd "color background $r $g $b"
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
            $m relval $x
            lappend _isomarkers($tf) $m 
        }
    } else {
        foreach x $levels {
            set m [Rappture::IsoMarker \#auto $c $this $tf]
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
            $m relval $value
            lappend _isomarkers($tf) $m
        } else {
            # ${n} : Set absolute value.
            set m [Rappture::IsoMarker \#auto $c $this $tf]
            $m absval $value
            lappend _isomarkers($tf) $m
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: UndateTransferFuncs 
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::updatetransferfuncs {} {
    $_dispatcher event -after 100 !send_transfunc
}

itcl::body Rappture::FlowvisViewer::AddIsoMarker { x y } {
    if { $_activeTf == "" } {
        error "active transfer function isn't set"
    }
    set tf $_activeTf 
    set c $itk_component(legend)
    set m [Rappture::IsoMarker \#auto $c $this $tf]
    set w [winfo width $c]
    $m relval [expr {double($x-10)/($w-20)}]
    lappend _isomarkers($tf) $m
    updatetransferfuncs
    return 1
}

itcl::body Rappture::FlowvisViewer::rmdupmarker { marker x } {
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
        updatetransferfuncs
    }
    return $bool
}

itcl::body Rappture::FlowvisViewer::overmarker { marker x } {
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

itcl::body Rappture::FlowvisViewer::limits { tf } {
    set _limits(vmin) 0.0
    set _limits(vmax) 1.0
    if { ![info exists _style2objs($tf)] } {
        puts stderr "no style2objs for $tf tf=($tf)"
        return [array get _limits]
    }
    set min ""; set max ""
    foreach {dataobj comp} $_style2objs($tf) {
        set tag $dataobj-$comp
        if { ![info exists _serverObjs($tag)] } {
            puts stderr "$tag not in serverObjs?"
            continue
        }
        if { ![info exists _limits($tag-min)] } {
            puts stderr "$tag no min?"
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
        set _limits(vmin) $min
    } 
    if { $max != "" } {
        set _limits(vmax) $max
    }
    return [array get _limits]
}



itcl::body Rappture::FlowvisViewer::BuildViewTab {} {
    foreach { key value } {
        grid		0
        axes		0
        outline		1
        volume		1
        legend		1
        particles	1
        lic		1
    } {
        set _settings($this-$key) $value
    }

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    set ::Rappture::FlowvisViewer::_settings($this-isosurface) 0
    checkbutton $inner.isosurface \
        -text "Isosurface shading" \
        -variable [itcl::scope _settings($this-isosurface)] \
        -command [itcl::code $this FixSettings isosurface] \
        -font "Arial 9"

    checkbutton $inner.axes \
        -text "Axes" \
        -variable [itcl::scope _settings($this-axes)] \
        -command [itcl::code $this FixSettings axes] \
        -font "Arial 9"

    checkbutton $inner.grid \
        -text "Grid" \
        -variable [itcl::scope _settings($this-grid)] \
        -command [itcl::code $this FixSettings grid] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Outline" \
        -variable [itcl::scope _settings($this-outline)] \
        -command [itcl::code $this FixSettings outline] \
        -font "Arial 9"

    checkbutton $inner.legend \
        -text "Legend" \
        -variable [itcl::scope _settings($this-legend)] \
        -command [itcl::code $this FixSettings legend] \
        -font "Arial 9"

    checkbutton $inner.volume \
        -text "Volume" \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume] \
        -font "Arial 9"

    checkbutton $inner.particles \
        -text "Particles" \
        -variable [itcl::scope _settings($this-particles)] \
        -command [itcl::code $this FixSettings particles] \
        -font "Arial 9"

    checkbutton $inner.lic \
        -text "Lic" \
        -variable [itcl::scope _settings($this-lic)] \
        -command [itcl::code $this FixSettings lic] \
        -font "Arial 9"

    frame $inner.frame

    blt::table $inner \
        0,0 $inner.axes  -columnspan 2 -anchor w \
        1,0 $inner.grid  -columnspan 2 -anchor w \
        2,0 $inner.outline  -columnspan 2 -anchor w \
        3,0 $inner.volume  -columnspan 2 -anchor w \
        4,0 $inner.legend  -columnspan 2 -anchor w 

    bind $inner <Map> [itcl::code $this GetFlowInfo $inner]

    blt::table configure $inner r* -resize none
    blt::table configure $inner r5 -resize expand
}

itcl::body Rappture::FlowvisViewer::BuildVolumeTab {} {
    foreach { key value } {
        light		40
        transp		50
        opacity		100
        thickness	350
    } {
        set _settings($this-$key) $value
    }

    set inner [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    checkbutton $inner.vol -text "Show volume" -font $fg \
        -text "Volume" \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume] \
        -font "Arial 9"

    label $inner.shading -text "Shading:" -font $fg

    label $inner.dim -text "Dim" -font $fg
    ::scale $inner.light -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-light)] \
        -width 10 \
        -showvalue off -command [itcl::code $this FixSettings light]
    label $inner.bright -text "Bright" -font $fg

    label $inner.fog -text "Fog" -font $fg
    ::scale $inner.transp -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-transp)] \
        -width 10 \
        -showvalue off -command [itcl::code $this FixSettings transp]
    label $inner.plastic -text "Plastic" -font $fg

    label $inner.clear -text "Clear" -font $fg
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings($this-opacity)] \
        -width 10 \
        -showvalue off -command [itcl::code $this FixSettings opacity]
    label $inner.opaque -text "Opaque" -font $fg

    label $inner.thin -text "Thin" -font $fg
    ::scale $inner.thickness -from 0 -to 1000 -orient horizontal \
        -variable [itcl::scope _settings($this-thickness)] \
        -width 10 \
        -showvalue off -command [itcl::code $this FixSettings thickness]
    label $inner.thick -text "Thick" -font $fg

    blt::table $inner \
        0,0 $inner.vol -columnspan 4 -anchor w -pady 2 \
        1,0 $inner.shading -columnspan 4 -anchor w -pady {10 2} \
        2,0 $inner.dim -anchor e -pady 2 \
        2,1 $inner.light -columnspan 2 -pady 2 -fill x \
        2,3 $inner.bright -anchor w -pady 2 \
        3,0 $inner.fog -anchor e -pady 2 \
        3,1 $inner.transp -columnspan 2 -pady 2 -fill x \
        3,3 $inner.plastic -anchor w -pady 2 \
        4,0 $inner.clear -anchor e -pady 2 \
        4,1 $inner.opacity -columnspan 2 -pady 2 -fill x\
        4,3 $inner.opaque -anchor w -pady 2 \
        5,0 $inner.thin -anchor e -pady 2 \
        5,1 $inner.thickness -columnspan 2 -pady 2 -fill x\
        5,3 $inner.thick -anchor w -pady 2

    if 0 {
        0,0 $inner.dim  -anchor e -pady 2 \
        0,1 $inner.light -columnspan 2 -pady 2 \
        0,3 $inner.bright -anchor w -pady 2 \
        1,0 $inner.fog -anchor e -pady 2 \
        1,1 $inner.transp -columnspan 2 -pady 2 \
        1,3 $inner.plastic -anchor w -pady 2 \
        2,0 $inner.clear -anchor e -pady 2 \
        2,1 $inner.opacity -columnspan 2 -pady 2 \
        2,3 $inner.opaque -anchor w -pady 2 \
        3,0 $inner.thin -anchor e -pady 2 \
        3,1 $inner.thickness -columnspan 2 -pady 2 \
        3,3 $inner.thick -anchor w -pady 2
    }
    blt::table configure $inner c0 c1 c3 r* -resize none
    blt::table configure $inner r6 -resize expand
}

itcl::body Rappture::FlowvisViewer::BuildCutplanesTab {} {
    set inner [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]
    $inner configure -borderwidth 4

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane] \
            -offimage [Rappture::icon x-cutplane] \
            -command [itcl::code $this FixSettings xcutplane] \
            -variable [itcl::scope _settings($this-xcutplane)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X cut plane on/off"

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
            -command [itcl::code $this FixSettings ycutplane] \
            -variable [itcl::scope _settings($this-ycutplane)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y cut plane on/off"

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
            -command [itcl::code $this FixSettings zcutplane] \
            -variable [itcl::scope _settings($this-zcutplane)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z cut plane on/off"

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
    #$itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this SlicerTip z]"

    blt::table $inner \
        1,1 $itk_component(xCutButton) \
        1,2 $itk_component(yCutButton) \
        1,3 $itk_component(zCutButton) \
        0,1 $itk_component(xCutScale) \
        0,2 $itk_component(yCutScale) \
        0,3 $itk_component(zCutScale) \

    blt::table configure $inner r0 r1 c* -resize none
    blt::table configure $inner r2 c4 -resize expand
    blt::table configure $inner c0 -width 2
    blt::table configure $inner c1 c2 c3 -padx 2
}

itcl::body Rappture::FlowvisViewer::BuildCameraTab {} {
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

itcl::body Rappture::FlowvisViewer::GetFlowInfo { w } {
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
        5,0 $inner -fill both -columnspan 2 -anchor nw
    array set hints [$flowobj hints]
    checkbutton $inner.showstreams -text "Streams Plane" \
        -variable [itcl::scope _settings($this-streams)] \
        -command  [itcl::code $this streams $key $hints(name)]  \
        -font "Arial 9"
    Rappture::Tooltip::for $inner.showstreams $hints(description)

    checkbutton $inner.showarrows -text "Arrows" \
        -variable [itcl::scope _settings($this-arrows)] \
        -command  [itcl::code $this arrows $key $hints(name)]  \
        -font "Arial 9"

    label $inner.particles -text "Particles"         -font "Arial 9 bold"
    label $inner.boxes -text "Boxes"         -font "Arial 9 bold"

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
        if { ![info exists _settings($this-particles-$name)] } {
            set _settings($this-particles-$name) $info(hide)
        }
        checkbutton $inner.part$row -text $info(label) \
            -variable [itcl::scope _settings($this-particles-$name)] \
            -onvalue 0 -offvalue 1 \
            -command [itcl::code $this particles $key $name] \
            -font "Arial 9"
        Rappture::Tooltip::for $inner.part$row $info(description)
        blt::table $inner $row,0 $inner.part$row -anchor w 
        if { !$_settings($this-particles-$name) } {
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
        if { ![info exists _settings($this-box-$name)] } {
            set _settings($this-box-$name) $info(hide)
        }
        checkbutton $inner.box$row -text $info(label) \
            -variable [itcl::scope _settings($this-box-$name)] \
            -onvalue 0 -offvalue 1 \
            -command [itcl::code $this box $key $name] \
            -font "Arial 9"
        Rappture::Tooltip::for $inner.box$row $info(description)
        blt::table $inner $row,0 $inner.box$row -anchor w
        if { !$_settings($this-box-$name) } {
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
    set bool $_settings($this-particles-$name)
    SendCmd "$tag particles configure {$name} -hide $bool"
}

itcl::body Rappture::FlowvisViewer::box { tag name } {
    set bool $_settings($this-box-$name)
    SendCmd "$tag box configure {$name} -hide $bool"
}

itcl::body Rappture::FlowvisViewer::streams { tag name } {
    set bool $_settings($this-streams)
    SendCmd "$tag configure -slice $bool"
}

itcl::body Rappture::FlowvisViewer::arrows { tag name } {
    set bool $_settings($this-arrows)
    SendCmd "$tag configure -arrows no"
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

            # show the current value in the readout

            set ids [CurrentVolumeIds -cutplanes]
            SendCmd "cutplane position $newpos $axis $ids"
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
#    set val [expr {0.01*($val-50)
#        *($_limits(${axis}max)-$_limits(${axis}min))
#          + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}


itcl::body Rappture::FlowvisViewer::Resize {} {
    SendCmd "screen $_width $_height"
    set _resizePending 0
}

itcl::body Rappture::FlowvisViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        $_dispatcher event -after 200 !resize
        set _resizePending 1
    }
}

itcl::body Rappture::FlowvisViewer::EventuallyResizeLegend {} {
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
itcl::body Rappture::FlowvisViewer::camera {option args} {
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
                    SendCmd "camera angle $xyz"
                }
                "zoom" {
                    set _view($who) $_settings($this-$who)
                    SendCmd "camera zoom $_view(zoom)"
                }
            }
        }
    }
}

itcl::body Rappture::FlowvisViewer::FlowCmd { dataobj comp nbytes extents } {
    set tag "$dataobj-$comp"
    if { ![info exists _obj2flow($tag)] } {
        append cmd "flow add $tag\n"
        append cmd "$tag data follows $nbytes $extents\n"
        return $cmd
    }
    set flowobj $_obj2flow($tag)
    if { $flowobj == "" } {
        puts stderr "no flowobj"
        return ""
    }
    set cmd {}
    append cmd "if {\[flow exists $tag\]} {flow delete $tag}\n"
    array set info  [$flowobj hints]
    set _settings($this-volume) $info(volume)
    set _settings($this-outline) $info(outline)
    set _settings($this-arrows) $info(arrows)
    set _settings($this-duration) $info(duration)
    $itk_component(speed) value $info(speed)
    append cmd "flow add $tag"
    append cmd " -position $info(position)"
    append cmd " -axis $info(axis)"
    append cmd " -volume $info(volume)"
    append cmd " -outline $info(outline)"
    append cmd " -slice $info(streams)"
    append cmd " -arrows $info(arrows)\n"
    foreach part [$flowobj particles] {
        array unset info
        array set info $part
        set color [Color2RGB $info(color)]
        append cmd "$tag particles add $info(name)"
        append cmd " -position $info(position)"
        append cmd " -hide $info(hide)"
        append cmd " -axis $info(axis)"
        append cmd " -color {$color}"
        append cmd " -size $info(size)\n"
    }
    foreach box [$flowobj boxes] {
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
        append cmd " -corner2 {$info(corner2)}\n"
    }    
    append cmd "$tag data follows $nbytes $extents\n"
    return $cmd
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
            puts stderr "flow goto to $_settings($this-currenttime)"
            # Figure out how many steps to the current time based upon
            # the speed and duration.
            set current $_settings($this-currenttime)
            set speed [$itk_component(speed) value]
            set time [str2millisecs $_settings($this-duration)]
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
            set max [str2millisecs $_settings($this-duration)]
            if { $max < 0 } {
                bell
                return
            }
            set _flow(duration) $max
            set _settings($this-duration) [millisecs2str $max]
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
            set _settings($this-currenttime) 0
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
                set _settings($this-currenttime) \
                    [expr {$_settings($this-currenttime) + $_flow(delay)}]
                if { $_settings($this-currenttime) >= $_flow(duration) } {
                    set _settings($this-step) 1
                    SendCmd "flow reset"
                } 
                flow next
            }
        }
        "toggle" {
            if { $_settings($this-play) } {
                flow play
            } else { 
                flow pause
            }
        }
        "reset" {
            set _settings($this-currenttime) 0
            SendCmd "flow reset"
            if { !$_flow(state) } {
                SendCmd "flow next"
            }
        }
        "next" {
            if { ![winfo viewable $itk_component(3dview)] } {
                flow stop
                return
            }
            set _settings($this-currenttime) \
                [expr {$_settings($this-currenttime) + $_flow(delay)}]
            if { $_settings($this-currenttime) >= $_flow(duration) } {
                if { !$_settings($this-loop) } {
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

itcl::body Rappture::FlowvisViewer::WaitIcon  { option widget } {
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

itcl::body Rappture::FlowvisViewer::GetPngImage  { widget width height } {
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
            0,0 $inner.title -columnspan 2 \
            1,0 $inner.please -anchor w \
            1,1 $inner.icon -anchor e  \
            2,0 $inner.cancel -columnspan 2 
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
            0,0 $inner.title -columnspan 2 \
            1,0 $inner.please -anchor w \
            1,1 $inner.icon -anchor e  \
            2,0 $inner.cancel -columnspan 2 
        blt::table configure $inner r0 -pady 4 
        blt::table configure $inner r2 -pady 4 
        bind $inner.cancel <KeyPress-Return> [list $inner.cancel invoke]
    } else {
        set inner [$popup component inner]
    }
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

