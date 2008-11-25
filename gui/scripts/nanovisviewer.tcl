
# ----------------------------------------------------------------------
#  COMPONENT: nanovisviewer - 3D volume rendering
#
#  This widget performs volume rendering on 3D scalar/vector datasets.
#  It connects to the Nanovis server running on a rendering farm,
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
    public method GetLimits { ivol } 
    public method download {option args}
    public method parameters {title args} { 
        # do nothing 
    }
    public method isconnected {}
    public method UpdateTransferFunctions {}
    public method RemoveDuplicateIsoMarker { m x }
    public method OverIsoMarker { m x }

    protected method Connect {}
    protected method Disconnect {}

    protected method _send {string}
    protected method _SendDataObjs {}
    protected method _SendTransferFunctions {}

    protected method _ReceiveImage {option size}
    protected method _ReceiveLegend { ivol vmin vmax size }
    protected method _ReceiveData { args }
    protected method _ReceivePrint { option size }

    protected method _rebuild {}
    protected method _currentVolumeIds {{what -all}}
    protected method _zoom {option}
    protected method _pan {option x y}
    protected method _rotate {option x y}
    protected method _slice {option args}
    protected method _slicertip {axis}
    protected method _probe {option args}
    protected method _marker {index option args}

    protected method _state {comp}
    protected method _fixSettings {what {value ""}}
    protected method _fixLegend {}

    # The following methods are only used by this class. 
    private method _NameTransferFunction { ivol }
    private method _ComputeTransferFunction { tf }
    private method _AddIsoMarker { x y }
    private method _ParseMarkersOption { tf ivol markers }
    private method _ParseLevelsOption { tf ivol levels }

    private variable outbuf_       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _all_data_objs
    private variable _dims ""      ;# dimensionality of data objects
    private variable _id2style     ;# maps id => style settings
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _obj2id       ;# maps dataobj => volume ID in server
    private variable _id2obj       ;# maps dataobj => volume ID in server
    private variable _sendobjs ""  ;# list of data objs to send to server
    private variable _receiveids   ;# list of data objs to send to server
    private variable _obj2styles   ;# maps id => style settings
    private variable _style2ids    ;# maps id => style settings

    private variable click_        ;# info used for _rotate operations
    private variable limits_       ;# autoscale min/max for all axes
    private variable view_         ;# view params for 3D view
    private variable isomarkers_    ;# array of isosurface level values 0..1
    private common   settings_
    private variable activeId_ ""   ;# The currently active volume.  This 
				    # indicates which isomarkers and transfer
				    # function to use when changing markers,
				    # opacity, or thickness.
    #common _downloadPopup          ;# download options from popup
}

itk::usual NanovisViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::constructor {hostlist args} {

    # Draw legend event
    $_dispatcher register !legend
    $_dispatcher dispatch $this !legend "[itcl::code $this _fixLegend]; list"
    # Send dataobjs event
    $_dispatcher register !send_dataobjs
    $_dispatcher dispatch $this !send_dataobjs \
        "[itcl::code $this _SendDataObjs]; list"
    # Send transfer functions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
        "[itcl::code $this _SendTransferFunctions]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    set outbuf_ ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this _ReceiveImage]
    $_parser alias legend [itcl::code $this _ReceiveLegend]
    $_parser alias data [itcl::code $this _ReceiveData]
    #$_parser alias print [itcl::code $this _ReceivePrint]

    # Initialize the view to some default parameters.
    array set view_ {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        dx	0
        dy	0
    }
    set _obj2id(count) 0
    set _id2obj(count) 0
    set limits_(vmin) 0.0
    set limits_(vmax) 1.0

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
            -bitmap [Rappture::icon reset] \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -side left -padx {4 1} -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomin] \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -side left -padx 1 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(zoom).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomout] \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -side left -padx {1 4} -pady 4
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
    pack $itk_component(slicers) -side bottom -padx 4 -pady 4
    grid rowconfigure $itk_component(slicers) 1 -weight 1
    #
    # X-value slicer...
    #
    itk_component add xslice {
        label $itk_component(slicers).xslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap [Rappture::icon x]
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
            -bitmap [Rappture::icon y]
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
            -bitmap [Rappture::icon z]
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
        -sticky ew -padx 1 -pady 3

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
    pack $itk_component(settings) -side top -pady 8

    Rappture::Balloon $itk_component(controls).panel -title "Settings"
    set inner [$itk_component(controls).panel component inner]
    frame $inner.scales
    pack $inner.scales -side top -fill x
    grid columnconfigure $inner.scales 1 -weight 1
    set fg [option get $itk_component(hull) font Font]

    label $inner.scales.diml -text "Dim" -font $fg
    grid $inner.scales.diml -row 0 -column 0 -sticky e
    ::scale $inner.scales.light -from 0 -to 100 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings light]
    grid $inner.scales.light -row 0 -column 1 -sticky ew
    label $inner.scales.brightl -text "Bright" -font $fg
    grid $inner.scales.brightl -row 0 -column 2 -sticky w
    $inner.scales.light set 40

    label $inner.scales.fogl -text "Fog" -font $fg
    grid $inner.scales.fogl -row 1 -column 0 -sticky e
    ::scale $inner.scales.transp -from 0 -to 100 -orient horizontal \
        -showvalue off -command [itcl::code $this _fixSettings transp]
    grid $inner.scales.transp -row 1 -column 1 -sticky ew
    label $inner.scales.plasticl -text "Plastic" -font $fg
    grid $inner.scales.plasticl -row 1 -column 2 -sticky w
    $inner.scales.transp set 50

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

    set ::Rappture::NanovisViewer::settings_($this-isosurface) 0
    ::checkbutton $inner.scales.isosurface \
        -text "Isosurface shading" \
        -variable ::Rappture::NanovisViewer::settings_($this-isosurface) \
        -command [itcl::code $this _fixSettings isosurface]
    grid $inner.scales.isosurface -row 4 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::settings_($this-axes) 1
    ::checkbutton $inner.scales.axes \
        -text "Axes" \
        -variable ::Rappture::NanovisViewer::settings_($this-axes) \
        -command [itcl::code $this _fixSettings axes]
    grid $inner.scales.axes -row 5 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::settings_($this-grid) 0
    ::checkbutton $inner.scales.grid \
        -text "Grid" \
        -variable ::Rappture::NanovisViewer::settings_($this-grid) \
        -command [itcl::code $this _fixSettings grid]
    grid $inner.scales.grid -row 6 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::settings_($this-outline) 1
    ::checkbutton $inner.scales.outline \
        -text "Outline" \
        -variable ::Rappture::NanovisViewer::settings_($this-outline) \
        -command [itcl::code $this _fixSettings outline]
    grid $inner.scales.outline -row 7 -column 0 -columnspan 2 -sticky w

    # Legend

    set _image(legend) [image create photo]
    itk_component add legend {
        canvas $itk_component(area).legend -height 50 -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -plotbackground plotBackground Background
    }
    pack $itk_component(legend) -side bottom -fill x
    bind $itk_component(legend) <Configure> \
        [list $_dispatcher event -idle !legend]

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
itcl::body Rappture::NanovisViewer::destructor {} {
    set _sendobjs ""  ;# stop any send in progress
    $_dispatcher cancel !rebuild
    $_dispatcher cancel !send_dataobjs
    $_dispatcher cancel !send_transfunc
    image delete $_image(plot)
    image delete $_image(legend)
    image delete $_image(download)
    array unset settings_ $this-*
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::add {dataobj {settings ""}} {
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

    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _all_data_objs($dataobj) 1
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
itcl::body Rappture::NanovisViewer::get {args} {
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
#	Clients use this to delete a dataobj from the plot.  If no dataobjs
#	are specified, then all dataobjs are deleted.  No data objects are
#	deleted.  They are only removed from the display list.
#
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
            foreach key [array names _obj2ovride $dataobj-*] {
                unset _obj2ovride($key)
            }
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
        set limits_($val) ""
    }
    foreach obj $args {
        foreach axis {x y z v} {

            foreach { min max } [$obj limits $axis] break

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
itcl::body Rappture::NanovisViewer::download {option args} {
    switch $option {
        coming {
            if {[catch {blt::winop snap $itk_component(area) $_image(download)}]} {
                $_image(download) configure -width 1 -height 1
                $_image(download) put #000000
            }
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            # Doing an image base64 encode/decode has to be better than
            # writing the image to a file and reading it back in.
            set data [$_image(plot) data -format jpeg]
            set data [Rappture::encoding::decode -as b64 $data]
            return [list .jpg $data]
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
    set result [VisViewer::Connect $_hosts]
    if { $result } {
	set w [winfo width $itk_component(3dview)]
	set h [winfo height $itk_component(3dview)]
	_send "screen $w $h"
    }
    return $result
}

#
# isconnected --
#
#       Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::NanovisViewer::isconnected {} {
    return [VisViewer::IsConnected]
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::NanovisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    set outbuf_ ""
    catch {unset _obj2id}
    array unset _id2obj
    set _obj2id(count) 0
    set _id2obj(count) 0
    set _sendobjs ""
}

#
# _send
#
#       Send commands off to the rendering server.  If we're currently
#       sending data objects to the server, buffer the commands to be 
#       sent later.
#
itcl::body Rappture::NanovisViewer::_send {string} {
    if {[llength $_sendobjs] > 0} {
        append outbuf_ $string "\n"
    } else {
	foreach line [split $string \n] {
	    SendEcho >>line $line
	}
        SendBytes $string
    }
}

# ----------------------------------------------------------------------
# USAGE: _SendDataObjs
#
# Used internally to send a series of volume objects off to the
# server.  Sends each object, a little at a time, with updates in
# between so the interface doesn't lock up.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_SendDataObjs {} {
    blt::busy hold $itk_component(hull); update idletasks
    foreach dataobj $_sendobjs {
        foreach comp [$dataobj components] {
            # send the data as one huge base64-encoded mess -- yuck!
            set data [$dataobj values $comp]
            set nbytes [string length $data]
            if { ![SendBytes "volume data follows $nbytes"] } {
                return
            }
            if { ![SendBytes $data] } {
                return
            }
            set ivol $_obj2id(count)
            incr _obj2id(count)

            set _id2obj($ivol) [list $dataobj $comp]
            set _obj2id($dataobj-$comp) $ivol
	    _NameTransferFunction $ivol
            set _receiveids($ivol) 1
        }
    }
    set _sendobjs ""
    blt::busy release $itk_component(hull)

    # activate the proper volume
    set first [lindex [get] 0]
    if {"" != $first} {
        set axis [$first hints updir]
        if {"" != $axis} {
            _send "up $axis"
        }
	# The active volume is by default the first component of the first
	# data object.  This assumes that the data is always successfully
	# transferred.
	set comp [lindex [$first components] 0]
	set activeId_ $_obj2id($first-$comp)
    }
    foreach key [array names _obj2id *-*] {
        set state [string match $first-* $key]
	set ivol $_obj2id($key)
        _send "volume state $state $ivol"
    }

    # sync the state of slicers
    set vols [_currentVolumeIds -cutplanes]
    foreach axis {x y z} {
        _send "cutplane state [_state ${axis}slice] $axis $vols"
        set pos [expr {0.01*[$itk_component(${axis}slicer) get]}]
        _send "cutplane position $pos $axis $vols"
    }
    _send "volume data state [_state volume] $vols"

    if 0 {
	# Add this when we fix grid for volumes
    _send "volume axis label x \"\""
    _send "volume axis label y \"\""
    _send "volume axis label z \"\""
    _send "grid axisname x X eV"
    _send "grid axisname y Y eV"
    _send "grid axisname z Z eV"
    }
    # if there are any commands in the buffer, send them now that we're done
    SendBytes $outbuf_
    set outbuf_ ""

    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: _SendTransferFunctions
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_SendTransferFunctions {} {
    set first [lindex [get] 0] 

    # Insure that the global opacity and thickness settings (in the slider
    # settings widgets) are used for the transfer-function used by the active
    # volume.  Update the values in the settings_ varible.
    set inner [$itk_component(controls).panel component inner]
    set tf $_id2style($activeId_)
    set value [$inner.scales.opacity get]
    set opacity [expr { double($value) * 0.01 }]
    set settings_($this-$tf-opacity) $opacity
    set value [$inner.scales.thickness get]
    # Scale values between 0.00001 and 0.01000
    set thickness [expr {double($value) * 0.0001}]
    set settings_($this-$tf-thickness) $thickness

    if { ![info exists $_obj2styles($first)] } {
	foreach tf $_obj2styles($first) {
	    if { ![_ComputeTransferFunction $tf] } {
		return
	    }
	}
	_fixLegend
    }
}

# ----------------------------------------------------------------------
# USAGE: _ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
set counter 0
itcl::body Rappture::NanovisViewer::_ReceiveImage {option size} {
    if { [isconnected] } {
	global counter
	incr counter
        set bytes [ReceiveBytes $size]
        $_image(plot) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(plot)]x[image height $_image(plot)] image>"
    }
}

#
# _ReceiveLegend --
#
#	The procedure is the response from the render server to each "legend"
#	command.  The server sends back a "legend" command invoked our
#	the slave interpreter.  The purpose is to collect data of the image 
#	representing the legend in the canvas.  In addition, the isomarkers
#	of the active volume are displayed.
#
#	I don't know is this is the right place to display the isomarkers.
#	I don't know all the different paths used to draw the plot. There's 
#	"_rebuild", "add", etc.
#
itcl::body Rappture::NanovisViewer::_ReceiveLegend { ivol vmin vmax size } {
    if { ![isconnected] } {
	return
    }
    set bytes [ReceiveBytes $size]
    $_image(legend) configure -data $bytes
    ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"
    
    set c $itk_component(legend)
    set w [winfo width $c]
    set h [winfo height $c]
    foreach { dataobj comp } $_id2obj($ivol) break
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
	    [itcl::code $this _AddIsoMarker %x %y]
    }
    array set limits [GetLimits $ivol]
    $c itemconfigure vmin -text [format %.2g $limits(min)]
    $c coords vmin $lx $ly
    
    $c itemconfigure vmax -text [format %.2g $limits(max)]
    $c coords vmax [expr {$w-$lx}] $ly

    # Display the markers used by the active volume.
    set tf $_id2style($activeId_)
    if { [info exists isomarkers_($tf)] } {
	foreach m $isomarkers_($tf) {
	    $m Show
	}
    }
}

#
# _ReceiveData --
#
#	The procedure is the response from the render server to each "data 
#	follows" command.  The server sends back a "data" command invoked our
#	the slave interpreter.  The purpose is to collect the min/max of the
#	volume sent to the render server.  Since the client (nanovisviewer)
#	doesn't parse 3D data formats, we rely on the server (nanovis) to
#	tell us what the limits are.  Once we've received the limits to all 
#	the data we've sent (tracked by _receiveids) we can then determine
#	what the transfer functions are for these volumes.
#
#
#	Note: There is a considerable tradeoff in having the server report
#	      back what the data limits are.  It means that much of the code 
#	      having to do with transfer-functions has to wait for the data
#	      to come back, since the isomarkers are calculated based upon
#	      the data limits.  The client code is much messier because of 
#	      this.  The alternative is to parse any of the 3D formats on the 
#	      client side.  
#
itcl::body Rappture::NanovisViewer::_ReceiveData { args } {
    if { ![isconnected] } {
	return
    }
    # Arguments from server are name value pairs. Stuff them in an array.
    array set info $args

    set ivol $info(id);			# Id of volume created by server.

    set limits_($ivol-min) $info(min);	# Minimum value of the volume.
    set limits_($ivol-max) $info(max);	# Maximum value of the volume.
    set limits_(vmin)	   $info(vmin);	# Overall minimum value.
    set limits_(vmax)      $info(vmax);	# Overall maximum value.

    unset _receiveids($ivol)
    if { [array size _receiveids] == 0 } {
	UpdateTransferFunctions
    }
}

# ----------------------------------------------------------------------
# USAGE: _ReceivePrint -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_ReceivePrint {option size} {
    if { [isconnected] } {
        set bytes [ReceiveBytes $size]
	set f [open /tmp/junk "w"]
	puts $f $bytes
	close $f
        $_image(download) configure -data $bytes
	update
        puts stderr "<read $size bytes for [image width $_image(download)]x[image height $_image(download)] image>"
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_rebuild {} {
    # Hide all the isomarkers. Can't remove them. Have to remember the
    # settings since the user may have created/deleted/moved markers.

    foreach tf [array names isomarkers_] {
	foreach m $isomarkers_($tf) {
            $m Hide
        }
    }

    # in the midst of sending data? then bail out
    if {[llength $_sendobjs] > 0} {
        return
    }

    # Find any new data that needs to be sent to the server.  Queue this up on
    # the _sendobjs list, and send it out a little at a time.  Do this first,
    # before we rebuild the rest.  
    foreach dataobj [get] {
        set comp [lindex [$dataobj components] 0]
        if {![info exists _obj2id($dataobj-$comp)]} {
            set i [lsearch -exact $_sendobjs $dataobj]
            if {$i < 0} {
                lappend _sendobjs $dataobj
            }
        }
    }
    set w [winfo width $itk_component(3dview)]
    set h [winfo height $itk_component(3dview)]
    _send "screen $w $h"

    #
    # Reset the camera and other view parameters
    #
    set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
    _send "camera angle $xyz"
    _send "camera zoom $view_(zoom)"
    
    _fixSettings light
    _fixSettings transp
    _fixSettings isosurface 
    _fixSettings grid
    _fixSettings axes
    _fixSettings outline

    if {[llength $_sendobjs] > 0} {
        # send off new data objects
        $_dispatcher event -idle !send_dataobjs
	return
    } 

    # nothing to send -- activate the proper ivol
    set first [lindex [get] 0]
    if {"" != $first} {
	set axis [$first hints updir]
	if {"" != $axis} {
	    _send "up $axis"
	}
	foreach key [array names _obj2id *-*] {
	    set state [string match $first-* $key]
	    _send "volume state $state $_obj2id($key)"
	}
	# 
	# The _obj2id and _id2style arrays may or may not have the right
	# information.  It's possible for the server to know about volumes
	# that the client has assumed it's deleted.  We could add checks.
	# But this problem needs to be fixed not bandaided.  
	set comp [lindex [$first components] 0]
	set ivol $_obj2id($first-$comp) 
	
	set tf _id2style($ivol)
    
	foreach comp [$first components] {
	    foreach ivol $_obj2id($first-$comp) {
		_NameTransferFunction $ivol
	    }
	}
    }

    # sync the state of slicers
    set vols [_currentVolumeIds -cutplanes]
    foreach axis {x y z} {
	_send "cutplane state [_state ${axis}slice] $axis $vols"
	set pos [expr {0.01*[$itk_component(${axis}slicer) get]}]
	_send "cutplane position $pos $axis $vols"
    }
    _send "volume data state [_state volume] $vols"
    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: _currentVolumeIds ?-cutplanes?
#
# Returns a list of volume server IDs for the current volume being
# displayed.  This is normally a single ID, but it might be a list
# of IDs if the current data object has multiple components.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_currentVolumeIds {{what -all}} {
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
itcl::body Rappture::NanovisViewer::_zoom {option} {
    switch -- $option {
        "in" {
            set view_(zoom) [expr {$view_(zoom)* 1.25}]
        }
        "out" {
            set view_(zoom) [expr {$view_(zoom)* 0.8}]
        }
        "reset" {
            array set view_ {
                theta 45
                phi 45
                psi 0
                zoom 1.0
		dx 0
		dy 0
            }
            set xyz [Euler2XYZ $view_(theta) $view_(phi) $view_(psi)]
            _send "camera angle $xyz"
            _send "camera pan $view_(dx) $view_(dy)"
        }
    }
    _send "camera zoom $view_(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: _rotate click <x> <y>
# USAGE: _rotate drag <x> <y>
# USAGE: _rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_rotate {option x y} {
    switch -- $option {
        click {
            $itk_component(3dview) configure -cursor fleur
            set click_(x) $x
            set click_(y) $y
            set click_(theta) $view_(theta)
            set click_(phi) $view_(phi)
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
                }]} {
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

                array set view_ [subst {
                    theta $theta
                    phi $phi
                    psi $psi
                }]
                set xyz [Euler2XYZ $theta $phi $psi]
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
# USAGE: $this _pan click x y
#        $this _pan drag x y
#	 $this _pan release x y
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_pan {option x y} {
    return
    # Experimental stuff
    if { $option == "set" } {
	set view_(dx) $x
	set view_(dy) $y
        _send "camera pan $view_(dx) $view_(dy)"
	return
    }
    if { $option == "click" } { 
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
	set view_(dx) [expr $view_(dx) + $x]
	set view_(dy) [expr $view_(dy) + $y]
	_send "camera pan $view_(dx) $view_(dy)"
    }
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
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
itcl::body Rappture::NanovisViewer::_slice {option args} {
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
itcl::body Rappture::NanovisViewer::_slicertip {axis} {
    set val [$itk_component(${axis}slicer) get]
#    set val [expr {0.01*($val-50)
#        *($limits_(${axis}max)-$limits_(${axis}min))
#          + 0.5*($limits_(${axis}max)+$limits_(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
}

# ----------------------------------------------------------------------
# USAGE: _state <component>
#
# Used internally to determine the state of a toggle button.
# The <component> is the itk component name of the button.
# Returns on/off for the state of the button.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_state {comp} {
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
itcl::body Rappture::NanovisViewer::_fixSettings {what {value ""}} {
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
            if {[isconnected] && $activeId_ != "" } {
		set val [$inner.scales.opacity get]
		set sval [expr { 0.01 * double($val) }]
		set tf $_id2style($activeId_)
		set settings_($this-$tf-opacity) $sval
		UpdateTransferFunctions
	    }
        }

        thickness {
            if {[isconnected] && $activeId_ != "" } {
		set val [$inner.scales.thickness get]
		# Scale values between 0.00001 and 0.01000
		set sval [expr {0.0001*double($val)}]
		set tf $_id2style($activeId_)
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
            if { [isconnected] } {
                _send "grid visible $settings_($this-grid)"
            }
        }
        "axes" {
            if { [isconnected] } {
                _send "axis visible $settings_($this-axes)"
            }
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLegend
#
# Used internally to update the legend area whenever it changes size
# or when the field changes.  Asks the server to send a new legend
# for the current field.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_fixLegend {} {
    set lineht [font metrics $itk_option(-font) -linespace]
    set w [expr {[winfo width $itk_component(legend)]-20}]
    set h [expr {[winfo height $itk_component(legend)]-20-$lineht}]
    set ivol $activeId_
    if {$w > 0 && $h > 0 && "" != $ivol} {
        _send "legend $ivol $w $h"
    } else {
	# Can't do this as this will remove the items associated with the
	# isomarkers.  

	#$itk_component(legend) delete all
    }
}

#
# _NameTransferFunction --
#
#	Creates a transfer function name based on the <style> settings in the
#	library run.xml file. This placeholder will be used later to create
#	and send the actual transfer function once the data info has been sent
#	to us by the render server. [We won't know the volume limits until the
#	server parses the 3D data and sends back the limits via _ReceiveData.]
#
#	FIXME: The current way we generate transfer-function names completely 
#	      ignores the -markers option.  The problem is that we are forced 
#	      to compute the name from an increasing complex set of values: 
#	      color, levels, marker, opacity.  I think we're stuck doing it
#	      now.
#
itcl::body Rappture::NanovisViewer::_NameTransferFunction { ivol } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
    }
    foreach {dataobj comp} $_id2obj($ivol) break
    array set style [lindex [$dataobj components -style $comp] 0]
    set tf "$style(-color):$style(-levels):$style(-opacity)"

    set _id2style($ivol) $tf
    lappend _obj2styles($dataobj) $tf
    lappend _style2ids($tf) $ivol
}

#
# _ComputeTransferFunction --
#
#	Computes and sends the transfer function to the render server.  It's
#	assumed that the volume data limits are known and that the global
#	transfer-functions slider values have be setup.  Both parts are
#	needed to compute the relative value (location) of the marker, and
#	the alpha map of the transfer function.  
#
itcl::body Rappture::NanovisViewer::_ComputeTransferFunction { tf } {
    array set style {
	-color rainbow
	-levels 6
	-opacity 1.0
    }
    set ivol [lindex $_style2ids($tf) 0]
    foreach {dataobj comp} $_id2obj($ivol) break
    array set style [lindex [$dataobj components -style $comp] 0]


    # We have to parse the style attributes for a volume using this
    # transfer-function *once*.  This sets up the initial isomarkers for the
    # transfer function.  The user may add/delete markers, so we have to
    # maintain a list of markers for each transfer-function.  We use the one
    # of the volumes (the first in the list) using the transfer-function as a
    # reference.  
    #
    # FIXME: The current way we generate transfer-function names completely 
    #	     ignores the -markers option.  The problem is that we are forced 
    #        to compute the name from an increasing complex set of values: 
    #        color, levels, marker, opacity.  I think the cow's out of the
    #	     barn on this one.

    if { ![info exists isomarkers_($tf)] } {
	# Have to defer creation of isomarkers until we have data limits
	if { [info exists style(-markers)] } {
	    _ParseMarkersOption $tf $ivol $style(-markers)
	} else {
	    _ParseLevelsOption $tf $ivol $style(-levels)
	}
    } 
    if {$style(-color) == "rainbow"} {
        set style(-color) "white:yellow:green:cyan:blue:magenta"
    }
    set clist [split $style(-color) :]
    set cmap "0.0 [Color2RGB white] "
    for {set i 0} {$i < [llength $clist]} {incr i} {
        set x [expr {double($i+1)/([llength $clist]+1)}]
        set color [lindex $clist $i]
        append cmap "$x [Color2RGB $color] "
    }
    append cmap "1.0 [Color2RGB $color]"

    set tag $this-$tf
    if { ![info exists settings_($tag-opacity)] } {
	set settings_($tag-opacity) $style(-opacity)
    }
    set max $settings_($tag-opacity)

    set isovalues {}
    foreach m $isomarkers_($tf) {
        lappend isovalues [$m GetRelativeValue]
    }
    # Sort the isovalues
    set isovalues [lsort -real $isovalues]

    if { ![info exists settings_($tag-thickness)]} {
	set settings_($tag-thickness) 0.05
    }
    set delta $settings_($tag-thickness)

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
    SendBytes "transfunc define $tf { $cmap } { $wmap }\n"
    return [SendBytes "volume shading transfunc $tf $_style2ids($tf)\n"]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotbackground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotbackground)] break
        #fix this!
        #_send "color background $r $g $b"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::NanovisViewer::plotforeground {
    if { [isconnected] } {
        foreach {r g b} [Color2RGB $itk_option(-plotforeground)] break
        #fix this!
        #_send "color background $r $g $b"
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
            _send "volume outline state off"
        } else {
            _send "volume outline state on"
            _send "volume outline color [Color2RGB $itk_option(-plotoutline)]"
        }
    }
}

#
# The -levels option takes a single value that represents the number
# of evenly distributed markers based on the current data range. Each
# marker is a relative value from 0.0 to 1.0.
#
itcl::body Rappture::NanovisViewer::_ParseLevelsOption { tf ivol levels } {
    set c $itk_component(legend)
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
	for {set i 1} { $i <= $levels } {incr i} {
	    set x [expr {double($i)/($levels+1)}]
	    set m [IsoMarker \#auto $c $this $ivol]
	    $m SetRelativeValue $x
	    lappend isomarkers_($tf) $m 
	}
    } else {
        foreach x $levels {
	    set m [IsoMarker \#auto $c $this $ivol]
	    $m SetRelativeValue $x
	    lappend isomarkers_($tf) $m 
        }
    }
}

#
# The -markers option takes a list of zero or more values (the values
# may be separated either by spaces or commas) that have the following 
# format:
#
# 	N%	Percent of current total data range.  Converted to
#		to a relative value between 0.0 and 1.0.
#	N	Absolute value of marker.  If the marker is outside of
#		the current range, it will be displayed on the outer
#		edge of the legends, but it range it represents will
#		not be seen.
#
itcl::body Rappture::NanovisViewer::_ParseMarkersOption { tf ivol markers } {
    set c $itk_component(legend)
    regsub -all "," $markers " " markers
    foreach marker $markers {
        set n [scan $marker "%g%s" value suffix]
        if { $n == 2 && $suffix == "%" } {
            # ${n}% : Set relative value. 
            set value [expr {$value * 0.01}]
            set m [IsoMarker \#auto $c $this $ivol]
            $m SetRelativeValue $value
            lappend isomarkers_($tf) $m
        } else {
            # ${n} : Set absolute value.
            set m [IsoMarker \#auto $c $this $ivol]
            $m SetAbsoluteValue $value
            lappend isomarkers_($tf) $m
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _marker start <x> <y>
# USAGE: _marker update <x> <y>
# USAGE: _marker end <x> <y>
#
# Used internally to handle the various marker operations performed
# when the user clicks and drags on the legend area.  The marker changes the
# transfer function to highlight the area being selected in the
# legend.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::UpdateTransferFunctions {} {
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::_AddIsoMarker { x y } {
    if { $activeId_ == "" } {
	error "active volume isn't set"
    }
    set tf $_id2style($activeId_)
    set c $itk_component(legend)
    set m [IsoMarker \#auto $c $this $activeId_]
    set w [winfo width $c]
    $m SetRelativeValue [expr {double($x-10)/($w-20)}]
    lappend isomarkers_($tf) $m
    UpdateTransferFunctions
    return 1
}

itcl::body Rappture::NanovisViewer::RemoveDuplicateIsoMarker { marker x } {
    set ivol [$marker GetVolume]
    set tf $_id2style($ivol)
    set bool 0
    if { [info exists isomarkers_($tf)] } {
        set list {}
        set marker [namespace tail $marker]
        foreach m $isomarkers_($tf) {
            set sx [$m GetScreenPosition]
            if { $m != $marker } {
                if { $x >= ($sx-3) && $x <= ($sx+3) } {
                    $marker SetRelativeValue [$m GetRelativeValue]
                    itcl::delete object $m
                    bell
                    set bool 1
                    continue
                }
            }
            lappend list $m
        }
        set isomarkers_($tf) $list
        UpdateTransferFunctions
    }
    return $bool
}

itcl::body Rappture::NanovisViewer::OverIsoMarker { marker x } {
    set ivol [$marker GetVolume]
    if { [info exists isomarkers_($ivol)] } {
        set marker [namespace tail $marker]
        foreach m $isomarkers_($ivol) {
            set sx [$m GetScreenPosition]
            if { $m != $marker } {
                set bool [expr { $x >= ($sx-3) && $x <= ($sx+3) }]
                $m Activate $bool
            }
        }
    }
    return ""
}

itcl::body Rappture::NanovisViewer::GetLimits { ivol } {
    if { ![info exists _id2style($ivol)] } {
	return
    }
    set tf $_id2style($ivol)
    set limits_(min) ""
    set limits_(max) ""
    foreach ivol $_style2ids($tf) {
	if { ![info exists limits_($ivol-min)] } {
	    error "can't find $ivol limits"
	}
	if { $limits_(min) == "" || $limits_(min) > $limits_($ivol-min) } {
	    set limits_(min) $limits_($ivol-min)
	}
	if { $limits_(max) == "" || $limits_(max) < $limits_($ivol-max) } {
	    set limits_(max) $limits_($ivol-max)
	}
    }
    return [array get limits_] 
}
