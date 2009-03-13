
# ----------------------------------------------------------------------
#  COMPONENT: flowvisviewer - 3D vector field rendering
#
#  This widget performs vector field rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2005-2009  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require Itk
package require BLT
package require Img

option add *FlowvisViewer.width 4i widgetDefault
option add *FlowvisViewer.height 4i widgetDefault
option add *FlowvisViewer.foreground black widgetDefault
option add *FlowvisViewer.controlBackground gray widgetDefault
option add *FlowvisViewer.controlDarkBackground #999999 widgetDefault
option add *FlowvisViewer.plotBackground black widgetDefault
option add *FlowvisViewer.plotForeground white widgetDefault
option add *FlowvisViewer.plotOutline white widgetDefault
option add *FlowvisViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *FlowvisViewer.boldFont \
    -*-helvetica-bold-r-normal-*-12-* widgetDefault

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
    public method get {args}
    public method delete {args}
    public method scale {args}
    public method download {option args}
    public method parameters {title args} {
        # do nothing
    }
    public method isconnected {}

    protected method Connect {}
    protected method Disconnect {}

    protected method _send {string}
    protected method _send_dataobjs {}
    protected method ReceiveImage {option size}
    protected method ReceiveFile {option size}
    private method ReceiveLegend {tf vmin vmax size}
    protected method _receive_echo {channel {data ""}}

    protected method _rebuild {}
    protected method _currentVolumeIds {{what -all}}
    protected method _zoom {option}
    protected method _pan {option x y}
    protected method _rotate {option x y}
    protected method _slice {option args}
    protected method _slicertip {axis}

    protected method _flow {option args}
    protected method _play {}
    protected method _pause {}

    protected method _state {comp}
    protected method _fixSettings {what {value ""}}
    protected method _getTransfuncData {dataobj comp}


    private variable outbuf_       ;# buffer for outgoing commands

    private variable dlist_ ""     ;# list of data objects
    private variable obj2style_    ;# maps dataobj => style settings
    private variable obj2ovride_   ;# maps dataobj => style override
    private variable obj2id_       ;# maps dataobj => heightmap ID in server
    private variable id2obj_       ;# maps heightmap ID => dataobj in server
    private variable sendobjs_ ""  ;# list of data objs to send to server
    private variable receiveIds_   ;# list of data responses from the server
    private variable click_        ;# info used for _rotate operations
    private variable limits_       ;# autoscale min/max for all axes
    private variable view_         ;# view params for 3D view

    private common settings_       ;# Array used for checkbuttons and radiobuttons
    private variable activeTf_ ""  ;# The currently active transfer function.

    common _downloadPopup          ;# download options from popup

    private variable _afterId ""   ;# current "after" event for play op
    private common _play           ;# options for "play" operation
    set _play(speed) 10
}

itk::usual FlowvisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::constructor {hostlist args} {
    # Send dataobjs event
    $_dispatcher register !send_dataobjs
    $_dispatcher dispatch $this !send_dataobjs \
        "[itcl::code $this _send_dataobjs]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    array set _downloadPopup {
        format jpg
    }

    set outbuf_ ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias file [itcl::code $this ReceiveFile]

    # Initialize the view to some default parameters.
    array set view_ {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        dx      0
        dy      0
    }
    set obj2id_(count) 0

    itk_component add zoom {
        frame $itk_component(controls).zoom
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(zoom) -side top

    itk_component add reset {
        button $itk_component(zoom).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon reset-view] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -side left -padx {4 1} -pady 2
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-in] \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -side left -padx 1 -pady 2
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(zoom).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -image [Rappture::icon zoom-out] \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -side left -padx {1 4} -pady 2
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    #
    # Create slicer controls...
    #
    itk_component add slicers {
        frame $itk_component(controls).slicers
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(slicers) -side bottom -padx 4 -pady 2
    grid rowconfigure $itk_component(slicers) 1 -weight 1
    #
    # X-value slicer...
    #
    itk_component add xslice {
        label $itk_component(slicers).xslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon x-cutplane]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(xslice) <ButtonPress> \
        [itcl::code $this _slice axis x toggle]
    Rappture::Tooltip::for $itk_component(xslice) \
        "Toggle the X cut plane on/off"
    grid $itk_component(xslice) -row 1 -column 0 -sticky ew -padx 1

    itk_component add xslicer {
        ::scale $itk_component(slicers).xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move x]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(xslicer) set 50
    $itk_component(xslicer) configure -state disabled
    grid $itk_component(xslicer) -row 2 -column 0 -padx 1
    Rappture::Tooltip::for $itk_component(xslicer) \
        "@[itcl::code $this _slicertip x]"

    #
    # Y-value slicer...
    #
    itk_component add yslice {
        label $itk_component(slicers).yslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon y-cutplane]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(yslice) <ButtonPress> \
        [itcl::code $this _slice axis y toggle]
    Rappture::Tooltip::for $itk_component(yslice) \
        "Toggle the Y cut plane on/off"
    grid $itk_component(yslice) -row 1 -column 1 -sticky ew -padx 1

    itk_component add yslicer {
        ::scale $itk_component(slicers).yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move y]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(yslicer) set 50
    $itk_component(yslicer) configure -state disabled
    grid $itk_component(yslicer) -row 2 -column 1 -padx 1
    Rappture::Tooltip::for $itk_component(yslicer) \
        "@[itcl::code $this _slicertip y]"

    #
    # Z-value slicer...
    #
    itk_component add zslice {
        label $itk_component(slicers).zslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon z-cutplane]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    grid $itk_component(zslice) -row 1 -column 2 -sticky ew -padx 1
    bind $itk_component(zslice) <ButtonPress> \
        [itcl::code $this _slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zslice) \
        "Toggle the Z cut plane on/off"

    itk_component add zslicer {
        ::scale $itk_component(slicers).zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move z]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    $itk_component(zslicer) set 50
    $itk_component(zslicer) configure -state disabled
    grid $itk_component(zslicer) -row 2 -column 2 -padx 1
    Rappture::Tooltip::for $itk_component(zslicer) \
        "@[itcl::code $this _slicertip z]"

    #
    # Volume toggle...
    #
    itk_component add volume {
        label $itk_component(slicers).volume \
            -borderwidth 1 -relief sunken -padx 1 -pady 1 \
            -text "Volume"
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(volume) <ButtonPress> \
        [itcl::code $this _slice volume toggle]
    Rappture::Tooltip::for $itk_component(volume) \
        "Toggle the volume cloud on/off"
    grid $itk_component(volume) -row 0 -column 0 -columnspan 3 \
        -sticky ew -padx 1 -pady 2

    #
    # Settings panel...
    #
    itk_component add settings {
        button $itk_component(controls).settings -text "Settings..." \
            -borderwidth 1 -relief flat -overrelief raised \
            -padx 2 -pady 1 \
            -command [list $itk_component(controls).panel activate $itk_component(controls).settings left]
    } {
        usual
        ignore -borderwidth
        rename -background -controlbackground controlBackground Background
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(settings) -side top 

    Rappture::Balloon $itk_component(controls).panel -title "Settings"
    set inner [$itk_component(controls).panel component inner]
    frame $inner.scales
    pack $inner.scales -side top -fill x
    grid columnconfigure $inner.scales 1 -weight 1
    set fg [option get $itk_component(hull) font Font]
    set bfg [option get $itk_component(hull) boldFont Font]

    label $inner.scales.diml -text "Dim" -font $fg
    grid $inner.scales.diml -row 0 -column 0 -sticky e
    ::scale $inner.scales.light -from 0 -to 100 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings light]
    grid $inner.scales.light -row 0 -column 1 -sticky ew
    label $inner.scales.brightl -text "Bright" -font $fg
    grid $inner.scales.brightl -row 0 -column 2 -sticky w
    $inner.scales.light set 20

    label $inner.scales.fogl -text "Fog" -font $fg
    grid $inner.scales.fogl -row 1 -column 0 -sticky e
    ::scale $inner.scales.transp -from 0 -to 100 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings transp]
    grid $inner.scales.transp -row 1 -column 1 -sticky ew
    label $inner.scales.plasticl -text "Plastic" -font $fg
    grid $inner.scales.plasticl -row 1 -column 2 -sticky w
    $inner.scales.transp set 20

    label $inner.scales.zerol -text "Clear" -font $fg
    grid $inner.scales.zerol -row 2 -column 0 -sticky e
    ::scale $inner.scales.opacity -from 0 -to 100 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings opacity]
    grid $inner.scales.opacity -row 2 -column 1 -sticky ew
    label $inner.scales.onel -text "Opaque" -font $fg
    grid $inner.scales.onel -row 2 -column 2 -sticky w
    $inner.scales.opacity set 100

    label $inner.scales.thinl -text "Thin" -font $fg
    grid $inner.scales.thinl -row 3 -column 0 -sticky e
    ::scale $inner.scales.thickness -from 0 -to 1000 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings thickness]
    grid $inner.scales.thickness -row 3 -column 1 -sticky ew
    label $inner.scales.thickl -text "Thick" -font $fg
    grid $inner.scales.thickl -row 3 -column 2 -sticky w
    $inner.scales.thickness set 350

    set ::Rappture::FlowvisViewer::settings_($this-isosurface) 0
    ::checkbutton $inner.scales.isosurface \
        -text "Isosurface shading" \
        -variable ::Rappture::FlowvisViewer::settings_($this-isosurface) \
        -command [itcl::code $this _fixSettings isosurface]
    grid $inner.scales.isosurface -row 4 -column 0 -columnspan 2 -sticky w

    set ::Rappture::FlowvisViewer::settings_($this-axes) 1
    ::checkbutton $inner.scales.axes \
        -text "Axes" \
        -variable ::Rappture::FlowvisViewer::settings_($this-axes) \
        -command [itcl::code $this _fixSettings axes]
    grid $inner.scales.axes -row 5 -column 0 -columnspan 2 -sticky w

    set ::Rappture::FlowvisViewer::settings_($this-grid) 0
    ::checkbutton $inner.scales.grid \
        -text "Grid" \
        -variable ::Rappture::FlowvisViewer::settings_($this-grid) \
        -command [itcl::code $this _fixSettings grid]
    grid $inner.scales.grid -row 6 -column 0 -columnspan 2 -sticky w

    set ::Rappture::FlowvisViewer::settings_($this-outline) 1
    ::checkbutton $inner.scales.outline \
        -text "Outline" \
        -variable ::Rappture::FlowvisViewer::settings_($this-outline) \
        -command [itcl::code $this _fixSettings outline]
    grid $inner.scales.outline -row 7 -column 0 -columnspan 2 -sticky w

    set ::Rappture::FlowvisViewer::settings_($this-particlevis) 1
    ::checkbutton $inner.scales.particlevis \
        -text "Particle Visible" \
        -variable ::Rappture::FlowvisViewer::settings_($this-particlevis) \
        -command [itcl::code $this _fixSettings particlevis]
    grid $inner.scales.particlevis -row 8 -column 0 -columnspan 2 -sticky w

    set ::Rappture::FlowvisViewer::settings_($this-lic) 1
    ::checkbutton $inner.scales.lic \
        -text "Lic" \
        -variable ::Rappture::FlowvisViewer::settings_($this-lic) \
        -command [itcl::code $this _fixSettings lic]
    grid $inner.scales.lic -row 9 -column 0 -columnspan 2 -sticky w

# how do we know when to make something an itk component?

    label $inner.scales.framecntlabel -text "# Frames" -font $fg
    grid $inner.scales.framecntlabel -row 10 -column 0 -sticky e
    Rappture::Spinint $inner.scales.framecnt
    $inner.scales.framecnt value 100
    grid $inner.scales.framecnt -row 10 -column 1 -sticky w

    label $inner.scales.speedl -text "Flow Speed:" -font $bfg
    grid $inner.scales.speedl -row 11 -column 0 -sticky e
    frame $inner.scales.speed
    grid $inner.scales.speed -row 11 -column 1 -sticky ew
    label $inner.scales.speed.slowl -text "Slower" -font $fg
    pack $inner.scales.speed.slowl -side left
    ::scale $inner.scales.speed.value -from 100 -to 1 \
        -showvalue 0 -orient horizontal \
        -variable ::Rappture::FlowvisViewer::_play(speed)
    pack $inner.scales.speed.value -side left
    label $inner.scales.speed.fastl -text "Faster" -font $fg
    pack $inner.scales.speed.fastl -side left

    #
    # Create flow controls...
    #
    itk_component add flowctrl {
        frame $itk_component(controls).flowctrl
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(flowctrl) -side top -padx 4 -pady 0
    grid rowconfigure $itk_component(flowctrl) 1 -weight 1

    #
    # flow record button...
    #
    itk_component add record {
        label $itk_component(flowctrl).record \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon playback-record]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(record) <ButtonPress> \
        [itcl::code $this _flow movie record toggle]
    Rappture::Tooltip::for $itk_component(record) \
        "Record flow visualization"
    grid $itk_component(record) -row 1 -column 0 -padx 1

    #
    # flow stop button...
    #
    itk_component add stop {
        label $itk_component(flowctrl).stop \
            -borderwidth 1 -relief sunken -padx 1 -pady 1 \
            -image [Rappture::icon playback-stop]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(stop) <ButtonPress> \
        [itcl::code $this _flow movie stop toggle]
    Rappture::Tooltip::for $itk_component(stop) \
        "Stop flow visualization"
    grid $itk_component(stop) -row 1 -column 1 -padx 1

    #
    # flow play/pause button...
    #
    itk_component add play {
        label $itk_component(flowctrl).play \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -image [Rappture::icon playback-start]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(play) <ButtonPress> \
        [itcl::code $this _flow movie play toggle]
    Rappture::Tooltip::for $itk_component(play) \
        "Play/Pause flow visualization"
    grid $itk_component(play) -row 1 -column 2 -padx 1

    # Bindings for rotation via mouse
    bind $itk_component(3dview) <ButtonPress-1> \
        [itcl::code $this _rotate click %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _rotate drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-1> \
        [itcl::code $this _rotate release %x %y]
    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _send "screen %w %h"]

    # Bindings for panning via mouse
    bind $itk_component(3dview) <ButtonPress-2> \
        [itcl::code $this _pan click %x %y]
    bind $itk_component(3dview) <B2-Motion> \
        [itcl::code $this _pan drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-2> \
        [itcl::code $this _pan release %x %y]

    # Bindings for panning via keyboard
    bind $itk_component(3dview) <KeyPress-Left> \
        [itcl::code $this _pan set -10 0]
    bind $itk_component(3dview) <KeyPress-Right> \
        [itcl::code $this _pan set 10 0]
    bind $itk_component(3dview) <KeyPress-Up> \
        [itcl::code $this _pan set 0 -10]
    bind $itk_component(3dview) <KeyPress-Down> \
        [itcl::code $this _pan set 0 10]
    bind $itk_component(3dview) <Shift-KeyPress-Left> \
        [itcl::code $this _pan set -2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Right> \
        [itcl::code $this _pan set 2 0]
    bind $itk_component(3dview) <Shift-KeyPress-Up> \
        [itcl::code $this _pan set 0 -2]
    bind $itk_component(3dview) <Shift-KeyPress-Down> \
        [itcl::code $this _pan set 0 2]

    # Bindings for zoom via keyboard
    bind $itk_component(3dview) <KeyPress-Prior> \
        [itcl::code $this _zoom out]
    bind $itk_component(3dview) <KeyPress-Next> \
        [itcl::code $this _zoom in]

    bind $itk_component(3dview) <Enter> "focus $itk_component(3dview)"

    if {[string equal "x11" [tk windowingsystem]]} {
        # Bindings for zoom via mouse
        bind $itk_component(3dview) <4> [itcl::code $this _zoom out]
        bind $itk_component(3dview) <5> [itcl::code $this _zoom in]
    }

    set _image(download) [image create photo]

    eval itk_initialize $args

    Connect
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::destructor {} {
    set sendobjs_ ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    image delete $_image(plot)
    image delete $_image(download)
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

    set pos [lsearch -exact $dataobj $dlist_]
    if {$pos < 0} {
        lappend dlist_ $dataobj
        set obj2ovride_($dataobj-color) $params(-color)
        set obj2ovride_($dataobj-width) $params(-width)
        set obj2ovride_($dataobj-raise) $params(-raise)
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
        set dlist $dlist_
        foreach obj $dlist {
            if { [info exists obj2ovride_($obj-raise)] && 
                 $obj2ovride_($obj-raise)} {
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
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $dlist_
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $dlist_ $dataobj]
        if {$pos >= 0} {
            set dlist_ [lreplace $dlist_ $pos $pos]
            foreach key [array names obj2ovride_ $dataobj-*] {
                unset obj2ovride_($key)
            }
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
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
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set limits_($val) ""
    }
    foreach obj $args {
        foreach axis {x y z v} {
            foreach {min max} [$obj limits $axis] break
            if {"" != $min && "" != $max} {
                if {"" == $limits_(${axis}min)} {
                    set limits_(${axis}min) $min
                    set limits_(${axis}max) $max
                } else {
                    if {$min < $limits_(${axis}min)} {
                        set limits_(${axis}min) $min
                    }
                    if {$max > $limits_(${axis}max)} {
                        set limits_(${axis}max) $max
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
    switch $option {
        coming {
            if {[catch {
                blt::winop snap $itk_component(area) $_image(download)
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
            switch -- $_downloadPopup(format) {
                jpg {
                    #
                    # Hack alert!  Need data in binary format,
                    # so we'll save to a file and read it back.
                    #
                    set tmpfile /tmp/image[pid].jpg
                    $_image(download) write $tmpfile -format jpeg
                    set fid [open $tmpfile r]
                    fconfigure $fid -encoding binary -translation binary
                    set bytes [read $fid]
                    close $fid
                    file delete -force $tmpfile

                    return [list .jpg $bytes]
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
    Disconnect
    set _hosts [GetServerList "nanovis"]
    if { "" == $_hosts } {
        return 0
    }
    set result [VisViewer::Connect $_hosts]
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

# ----------------------------------------------------------------------
# USAGE: Disconnect
#
# Clients use this method to disconnect from the current rendering
# server.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::Disconnect {} {
    VisViewer::Disconnect

    set outbuf_ ""
    # disconnected -- no more data sitting on server
    catch {unset obj2id_}
    array unset id2obj_
    set obj2id_(count) 0
    set id2obj_(cound) 0
    set sendobjs_ ""
}

#
# _send
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::FlowvisViewer::_send {string} {
    if {[llength $sendobjs_] > 0} {
        append outbuf_ $string "\n"
    } else {
        if {[SendBytes $string]} {
            foreach line [split $string \n] {
                SendEcho >>line $line
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _send_dataobjs
#
# Used internally to send a series of volume objects off to the
# server.  Sends each object, a little at a time, with updates in
# between so the interface doesn't lock up.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_send_dataobjs {} {
    blt::busy hold $itk_component(hull); update idletasks

    # Reset the overall limits
    if { $sendobjs_ != "" } {
        set limits_(vmin) ""
        set limits_(vmax) ""
    }
    foreach dataobj $sendobjs_ {
        foreach comp [$dataobj components] {
            set data [$dataobj values $comp]

            # tell the engine to expect some data
            set nbytes [string length $data]
            if { ![SendBytes "flow data follows $nbytes"] } {
                return
            }
            if { ![SendBytes $data] } {
                return
            }
            set id $obj2id_(count)
            incr obj2id_(count)
            set id2obj_($id) [list $dataobj $comp]
            set obj2id_($dataobj-$comp) $id
            set receiveIds_($id) 1
        }
    }
    set sendobjs_ ""
    blt::busy release $itk_component(hull)

    # activate the proper volume
    set first [lindex [get] 0]
    if {"" != $first} {
        set axis [$first hints updir]
        if {"" != $axis} {
            _send "up $axis"
        }
    }

# FIXME: does vectorid command turn off other vectors?
    foreach key [array names obj2id_ *-*] {
        _send "flow vectorid $obj2id_($key)"
    }

    # sync the state of slicers
    set vols [_currentVolumeIds -cutplanes]
    foreach axis {x y z} {
        _send "cutplane state [_state ${axis}slice] $axis $vols"
        set pos [expr {0.01*[$itk_component(${axis}slicer) get]}]
        _send "cutplane position $pos $axis $vols"
    }

    # if there are any commands in the buffer, send them now that we're done
    SendBytes $outbuf_
    set outbuf_ ""
}

# ----------------------------------------------------------------------
# USAGE: ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::ReceiveImage {option size} {
    if {[IsConnected]} {
        set bytes [ReceiveBytes $size]
        $_image(plot) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(plot)]x[image height $_image(plot)] image>"
    }
}

# ----------------------------------------------------------------------
# USAGE: ReceiveFile -bytes <size>
#
# Invoked automatically whenever the "file" command comes in from
# the rendering server.  Indicates that binary file data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::ReceiveFile {option size} {
    if {[IsConnected]} {
        set bytes [ReceiveBytes $size]
        ReceiveEcho <<line "<read $size bytes for file>"

        # raise the record button back up.
        $itk_component(record) configure -relief raised 

        # FIXME: manually download file???
        set mesg [Rappture::filexfer::download $bytes "flowmovie.mpeg"]
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_rebuild {} {
    # in the midst of sending data? then bail out
    if {[llength $sendobjs_] > 0} {
        return
    }
    # Find any new data that needs to be sent to the server.  Queue this up on
    # the sendobjs_ list, and send it out a little at a time.  Do this first,
    # before we rebuild the rest.
    foreach dataobj [get] {
        set comp [lindex [$dataobj components] 0]
        if {![info exists obj2id_($dataobj-$comp)]} {
            set i [lsearch -exact $sendobjs_ $dataobj]
            if {$i < 0} {
                lappend sendobjs_ $dataobj
            }
        }
    }
    if {[llength $sendobjs_] > 0} {
        # Send off new data objects
        $_dispatcher event -idle !send_dataobjs
    } else {
        # Nothing to send -- activate the proper volume
        set first [lindex [get] 0]
        if {"" != $first} {
            set axis [$first hints updir]
            if {"" != $axis} {
                _send "up $axis"
            }
        }
        foreach key [array names obj2id_ *-*] {
            set state [string match $first-* $key]
            _send "flow vectorid $obj2id_($key)"
        }
    }

    # Reset the screen size.
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    _send "screen $w $h"

    # Reset the camera and other view parameters
    set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
    _send "camera angle $xyz"
    _send "camera zoom $view_(zoom)"

    # _fixSettings wireframe
    _fixSettings light
    _fixSettings transp
    _fixSettings isosurface
    _fixSettings grid
    _fixSettings axes
    _fixSettings outline
    # _fixSettings contourlines
}

# ----------------------------------------------------------------------
# USAGE: _currentVolumeIds ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_currentVolumeIds {{what -all}} {
    set rlist ""

    set first [lindex [get] 0]
    foreach key [array names _obj2id *-*] {
        if {[string match $first-* $key]} {
            array set style {
                -cutplanes 1
            }
            foreach {dataobj comp} [split $key -] break
            array set style [lindex [$dataobj components -style $comp] 0]

            if {$what != "-cutplanes" || $style(-cutplanes)} {
                lappend rlist $_obj2id($key)
            }
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_zoom {option} {
    switch -- $option {
        "in" {
            set view_(zoom) [expr {$view_(zoom)*1.25}]
        }
        "out" {
            set view_(zoom) [expr {$view_(zoom)*0.8}]
        }
        "reset" {
            array set view_ {
                theta   45
                phi     45
                psi     0
                zoom    1.0
                dx      0
                dy      0
            }
            set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
            _send "camera angle $xyz"
            _send "camera pan $view_(dx) $view_(dy)"
        }
    }
    _send "camera zoom $view_(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: $this _pan click x y
#        $this _pan drag x y
#        $this _pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_pan {option x y} {
    # Experimental stuff
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    if { $option == "set" } {
        set x [expr $x / double($w)]
        set y [expr $y / double($h)]
        set view_(dx) [expr $view_(dx) + $x]
        set view_(dy) [expr $view_(dy) + $y]
        _send "camera pan $view_(dx) $view_(dy)"
        return
    }
    if { $option == "click" } {
        set click_(x) $x
        set click_(y) $y
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
        set dx [expr ($click_(x) - $x)/double($w)]
        set dy [expr ($click_(y) - $y)/double($h)]
        set click_(x) $x
        set click_(y) $y
        set view_(dx) [expr $view_(dx) - $dx]
        set view_(dy) [expr $view_(dy) - $dy]
        _send "camera pan $view_(dx) $view_(dy)"
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _rotate click <x> <y>
# USAGE: _rotate drag <x> <y>
# USAGE: _rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            array set click_ [subst {
                x       $x
                y       $y
                theta   $view_(theta)
                phi     $view_(phi)
            }]
        }
        drag {
            if {[array size click_] == 0} {
                _rotate click $x $y
            } else {
                set w [winfo width $itk_component(3dview)]
                set h [winfo height $itk_component(3dview)]
                if {$w <= 0 || $h <= 0} {
                    return
                }

                if {[catch {
                    # this fails sometimes for no apparent reason
                    set dx [expr {double($x-$click_(x))/$w}]
                    set dy [expr {double($y-$click_(y))/$h}]
                }] != 0 } {
                    return
                }

                #
                # Rotate the camera in 3D
                #
                if {$view_(psi) > 90 || $view_(psi) < -90} {
                    # when psi is flipped around, theta moves backwards
                    set dy [expr {-$dy}]
                }
                set theta [expr {$view_(theta) - $dy*180}]
                while {$theta < 0} { set theta [expr {$theta+180}] }
                while {$theta > 180} { set theta [expr {$theta-180}] }

                if {abs($theta) >= 30 && abs($theta) <= 160} {
                    set phi [expr {$view_(phi) - $dx*360}]
                    while {$phi < 0} { set phi [expr {$phi+360}] }
                    while {$phi > 360} { set phi [expr {$phi-360}] }
                    set psi $view_(psi)
                } else {
                    set phi $view_(phi)
                    set psi [expr {$view_(psi) - $dx*360}]
                    while {$psi < -180} { set psi [expr {$psi+360}] }
                    while {$psi > 180} { set psi [expr {$psi-360}] }
                }

                set view_(theta)        $theta
                set view_(phi)          $phi
                set view_(psi)          $psi
                set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
                _send "camera angle $xyz"
                set click_(x) $x
                set click_(y) $y
            }
        }
        release {
            _rotate drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset click_}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _slice axis x|y|z ?on|off|toggle?
# USAGE: _slice move x|y|z <newval>
# USAGE: _slice volume ?on|off|toggle?
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_slice {option args} {
    switch -- $option {
        axis {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"_slice axis x|y|z ?on|off|toggle?\""
            }
            set axis [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            set current [_state ${axis}slice]
            if {$op == "toggle"} {
                if {$current == "on"} { set op "off" } else { set op "on" }
            }
            if {$op} {
                $itk_component(${axis}slicer) configure -state normal
                _send "cutplane state 1 $axis [_currentVolumeIds -cutplanes]"
                $itk_component(${axis}slice) configure -relief sunken
            } else {
                $itk_component(${axis}slicer) configure -state disabled
                _send "cutplane state 0 $axis [_currentVolumeIds -cutplanes]"
                $itk_component(${axis}slice) configure -relief raised
            }
        }
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set newpos [expr {0.01*$newval}]
#            set newval [expr {0.01*($newval-50)
#                *($limits_(${axis}max)-$limits_(${axis}min))
#                  + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]

            # show the current value in the readout
            #puts "readout: $axis = $newval"

            set ids [_currentVolumeIds -cutplanes]
            _send "cutplane position $newpos $axis $ids"
        }
        volume {
            if {[llength $args] > 1} {
                error "wrong # args: should be \"_slice volume ?on|off|toggle?\""
            }
            set op [lindex $args 0]
            if {$op == ""} { set op "on" }

            set current [_state volume]
            if {$op == "toggle"} {
                if {$current == "on"} { set op "off" } else { set op "on" }
            }

            if {$op} {
                _send "volume data state on [_currentVolumeIds]"
                $itk_component(volume) configure -relief sunken
            } else {
                _send "volume data state off [_currentVolumeIds]"
                $itk_component(volume) configure -relief raised
            }
        }
        default {
            error "bad option \"$option\": should be axis, move, or volume"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _slicertip <axis>
#
# Used internally to generate a tooltip for the x/y/z slicer controls.
# Returns a message that includes the current slicer value.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_slicertip {axis} {
    set val [$itk_component(${axis}slicer) get]
#    set val [expr {0.01*($val-50)
#        *($limits_(${axis}max)-$limits_(${axis}min))
#          + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}

# ----------------------------------------------------------------------
# USAGE: _flow movie record|stop|play ?on|off|toggle?
#
# Called when the user clicks on the record, stop or play buttons
# for flow visualization.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_flow {option args} {
    switch -- $option {
        movie {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"_flow movie record|stop|play ?on|off|toggle?\""
            }
            set action [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            set current [_state $action]
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
		    if { [$itk_component(record) cget -relief] != "sunken" } {
			$itk_component(record) configure -relief sunken 
			$itk_component(stop) configure -relief raised 
			$itk_component(play) configure -relief raised 
			set inner \
			    [$itk_component(controls).panel component inner]
			set frames [$inner.scales.framecnt value]
			set cmds "flow capture $frames"
			_send $cmds
		    }
                }
                stop {
		    if { [$itk_component(stop) cget -relief] != "sunken" } {
			$itk_component(record) configure -relief raised 
			$itk_component(stop) configure -relief sunken 
			$itk_component(play) configure -relief raised 
			_pause
			set cmds "flow reset"
			_send $cmds
		    }
                }
                play {
		    if { [$itk_component(play) cget -relief] != "sunken" } {
			$itk_component(record) configure -relief raised
			$itk_component(stop) configure -relief raised 
			$itk_component(play) configure \
			    -image [Rappture::icon playback-pause] \
			    -relief sunken 
			bind $itk_component(play) <ButtonPress> \
			    [itcl::code $this _pause]
			_play
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
# USAGE: _play
#
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_play {} {
    set cmds {flow play}
    _send $cmds
    set delay [expr {int(ceil(pow($_play(speed)/10.0+2,2.0)*15))}]
    set _afterId [after $delay [itcl::code $this _play]]
}

# ----------------------------------------------------------------------
# USAGE: _pause
#
# Invoked when the user hits the "pause" button to stop playing the
# current sequence of frames as a movie.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_pause {} {
    if {"" != $_afterId} {
        catch {after cancel $_afterId}
        set _afterId ""
    }

    # toggle the button to "play" mode
    $itk_component(play) configure \
        -image [Rappture::icon playback-start] \
        -relief raised 
    bind $itk_component(play) <ButtonPress> \
        [itcl::code $this _flow movie play toggle]

}

# ----------------------------------------------------------------------
# USAGE: _state <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_state {comp} {
    if {[$itk_component($comp) cget -relief] == "sunken"} {
        return "on"
    }
    return "off"
}

# ----------------------------------------------------------------------
# USAGE: _fixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_fixSettings { what {value ""} } {
    set inner [$itk_component(controls).panel component inner]
    switch -- $what {
        light {
            if {[isconnected]} {
                set val [$inner.scales.light get]
                set sval [expr {0.1*$val}]
                _send "volume shading diffuse $sval"

                set sval [expr {sqrt($val+1.0)}]
                _send "volume shading specular $sval"
            }
        }
        transp {
            if {[isconnected]} {
                set val [$inner.scales.transp get]
                set sval [expr {0.2*$val+1}]
                _send "volume shading opacity $sval"
            }
        }
        opacity {
            if {[isconnected] && $activeTf_ != "" } {
                set val [$inner.scales.opacity get]
                set sval [expr { 0.01 * double($val) }]
                set tf $activeTf_
                set settings_($this-$tf-opacity) $sval
                UpdateTransferFunctions
            }
        }

        thickness {
            if {[isconnected] && $activeTf_ != "" } {
                set val [$inner.scales.thickness get]
                # Scale values between 0.00001 and 0.01000
                set sval [expr {0.0001*double($val)}]
                set tf $activeTf_
                set settings_($this-$tf-thickness) $sval
                UpdateTransferFunctions
            }
        }
        "outline" {
            if {[isconnected]} {
                _send "volume outline state $settings_($this-outline)"
            }
        }
        "isosurface" {
            if {[isconnected]} {
                _send "volume shading isosurface $settings_($this-isosurface)"
            }
        }
        "grid" {
            if { [IsConnected] } {
                _send "grid visible $settings_($this-grid)"
            }
        }
        "particlevis" {
            if { [IsConnected] } {
                _send "flow particle visible $settings_($this-particlevis)"
            }
        }
        "lic" {
            if { [IsConnected] } {
                _send "flow lic $settings_($this-lic)"
            }
        }
        "axes" {
            if { [IsConnected] } {
                _send "axis visible $settings_($this-axes)"
            }
        }
        default {
            error "don't know how to fix $what: should be grid, axes, contourlines, or legend"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _getTransfuncData <dataobj> <comp>
#
# Used internally to compute the colormap and alpha map used to define
# a transfer function for the specified component in a data object.
# Returns: name {v r g b ...} {v w ...}
# ----------------------------------------------------------------------
itcl::body Rappture::FlowvisViewer::_getTransfuncData {dataobj comp} {
    array set style {
        -color rainbow
        -levels 6
        -opacity 0.5
    }
    array set style [lindex [$dataobj components -style $comp] 0]
    set sname "$style(-color):$style(-levels):$style(-opacity)"

    if {$style(-color) == "rainbow"} {
        set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    set clist [split $style(-color) :]
    set color white
    set cmap "0.0 [Color2RGB $color] "
    set range [expr $limits_(vmax) - $limits_(vmin)]
    for {set i 0} {$i < [llength $clist]} {incr i} {
        set xval [expr {double($i+1)/([llength $clist]+1)}]
        set color [lindex $clist $i]
        append cmap "$xval [Color2RGB $color] "
    }
    append cmap "1.0 [Color2RGB $color] "

    set opacity $style(-opacity)
    set levels $style(-levels)
    set wmap {}
    if {[string is int $levels]} {
        lappend wmap 0.0 0.0
        set delta [expr {0.125/($levels+1)}]
        for {set i 1} {$i <= $levels} {incr i} {
            # add spikes in the middle
            set xval [expr {double($i)/($levels+1)}]
            lappend wmap [expr {$xval-$delta-0.01}] 0.0
            lappend wmap [expr {$xval-$delta}] $opacity 
            lappend wmap [expr {$xval+$delta}] $opacity
            lappend wmap [expr {$xval+$delta+0.01}] 0.0
        }
        lappend wmap 1.0 0.0
    } else {
        lappend wmap 0.0 0.0
        set delta 0.05
        foreach xval [split $levels ,] {
            lappend wmap [expr {$xval-$delta}] 0.0
            lappend $xval $opacity
            lappend [expr {$xval+$delta}] 0.0
        }
        lappend wmap 1.0 0.0
    }
    return [list $sname $cmap $wmap]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotbackground {
    foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
    #fix this!
    #_send "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotforeground {
    foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
    #fix this!
    #_send "color background $r $g $b"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotoutline
# ----------------------------------------------------------------------
itcl::configbody Rappture::FlowvisViewer::plotoutline {
    if {[IsConnected]} {
        _send "grid linecolor [Color2RGB $itk_option(-plotoutline)]"
    }
}
