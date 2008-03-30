
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
    public method GetLimits { {ivol ""} } {
	if { $ivol != "" } {
	    set _limits(min) $_limits($ivol-min)
	    set _limits(max) $_limits($ivol-max)
	}
	return [array get _limits] 
    }
    public method download {option args}
    public method parameters {title args} { 
        # do nothing 
    }
    public method isconnected {}
    public method UpdateTransferFunction { ivol }
    public method RemoveDuplicateIsoMarker { m x }
    public method OverIsoMarker { m x }

    protected method Connect {}
    protected method Disconnect {}

    protected method _send {string}
    protected method _send_dataobjs {}
    protected method _send_transfunc {}

    protected method _ReceiveImage {option size}
    protected method _ReceiveLegend { ivol vmin vmax size }
    protected method _ReceiveData { args }

    protected method _rebuild {}
    protected method _currentVolumeIds {{what -all}}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _slice {option args}
    protected method _slicertip {axis}
    protected method _probe {option args}
    protected method _marker {index option args}

    protected method _state {comp}
    protected method _fixSettings {what {value ""}}
    protected method _fixLegend {}

    # The following methods are only used by this class. 
    private method _DefineTransferFunction { ivol }
    private method _AddIsoMarker { ivol x y }
    private method _InitIsoMarkers { ivol }
    private method _HideIsoMarkers { ivol }
    private method _ShowIsoMarkers { ivol }
    private method _ParseMarkersOption { ivol markers }
    private method _ParseLevelsOption { ivol levels }

    private variable _outbuf       ;# buffer for outgoing commands

    private variable _dlist ""     ;# list of data objects
    private variable _all_data_objs
    private variable _dims ""      ;# dimensionality of data objects
    private variable _id2style     ;# maps id => style settings
    private variable _obj2ovride   ;# maps dataobj => style override
    private variable _obj2id       ;# maps dataobj => volume ID in server
    private variable _id2obj       ;# maps dataobj => volume ID in server
    private variable _sendobjs ""  ;# list of data objs to send to server
    private variable _sendobjs2 ""  ;# list of data objs to send to server
    private variable _receiveids   ;# list of data objs to send to server

    private variable _click        ;# info used for _move operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
        
    private variable _isomarkers    ;# array of isosurface level values 0..1
    private variable _styles
    private common   _settings
    private variable _currentVolId ""
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
        "[itcl::code $this _send_dataobjs]; list"
    # Send transfer functions event
    $_dispatcher register !send_transfunc
    $_dispatcher dispatch $this !send_transfunc \
        "[itcl::code $this _send_transfunc]; list"
    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    set _outbuf ""

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this _ReceiveImage]
    $_parser alias legend [itcl::code $this _ReceiveLegend]
    $_parser alias data [itcl::code $this _ReceiveData]

    # Initialize the view to some default parameters.
    array set _view {
        theta   45
        phi     45
        psi     0
        zoom    1.0
        xfocus  0
        yfocus  0
        zfocus  0
    }
    set _obj2id(count) 0
    set _id2obj(count) 0
    set _limits(vmin) 0.0
    set _limits(vmax) 1.0

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
    $inner.scales.thickness set 500

    set ::Rappture::NanovisViewer::_settings($this-isosurface) 0
    ::checkbutton $inner.scales.isosurface \
        -text "Isosurface shading" \
        -variable ::Rappture::NanovisViewer::_settings($this-isosurface) \
        -command [itcl::code $this _fixSettings isosurface]
    grid $inner.scales.isosurface -row 4 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::_settings($this-axes) 1
    ::checkbutton $inner.scales.axes \
        -text "Axes" \
        -variable ::Rappture::NanovisViewer::_settings($this-axes) \
        -command [itcl::code $this _fixSettings axes]
    grid $inner.scales.axes -row 5 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::_settings($this-grid) 0
    ::checkbutton $inner.scales.grid \
        -text "Grid" \
        -variable ::Rappture::NanovisViewer::_settings($this-grid) \
        -command [itcl::code $this _fixSettings grid]
    grid $inner.scales.grid -row 6 -column 0 -columnspan 2 -sticky w

    set ::Rappture::NanovisViewer::_settings($this-outline) 1
    ::checkbutton $inner.scales.outline \
        -text "Outline" \
        -variable ::Rappture::NanovisViewer::_settings($this-outline) \
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

    # set up bindings for rotation
    bind $itk_component(3dview) <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(3dview) <ButtonRelease> \
        [itcl::code $this _move release %x %y]
    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _send "screen %w %h"]

    set _image(download) [image create photo]

    eval itk_initialize $args

    Connect
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::destructor {} {
    set _sendobjs ""  ;# stop any send in progress
    set _sendobjs2 ""  ;# stop any send in progress
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
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            foreach key [array names _obj2ovride $dataobj-*] {
                unset _obj2ovride($key)
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
itcl::body Rappture::NanovisViewer::scale {args} {
    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }
    foreach obj $args {
        foreach axis {x y z v} {
            foreach {min max} [$obj limits $axis] break
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
    set _outbuf ""
    catch {unset _obj2id}
    array unset _id2obj
    set _obj2id(count) 0
    set _id2obj(count) 0
    set _sendobjs ""
    set _sendobjs2 ""
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
        append _outbuf $string "\n"
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
itcl::body Rappture::NanovisViewer::_send_dataobjs {} {
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
    }
    foreach key [array names _obj2id *-*] {
        set state [string match $first-* $key]
	set ivol $_obj2id($key)
        _send "volume state $state $ivol"
        if {[info exists _id2style($ivol)]} {
            _send "volume shading transfunc $_id2style($ivol) $ivol"
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

    # if there are any commands in the buffer, send them now that we're done
    SendBytes $_outbuf
    set _outbuf ""

    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: _send_transfunc
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_send_transfunc {} {
    if { ![_DefineTransferFunction $_currentVolId] } {
	return
    }
    _fixLegend
    $_dispatcher event -idle !legend
}

# ----------------------------------------------------------------------
# USAGE: _ReceiveImage -bytes <size>
#
# Invoked automatically whenever the "image" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_ReceiveImage {option size} {
    if { [isconnected] } {
        set bytes [ReceiveBytes $size]
        $_image(plot) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(plot)]x[image height $_image(plot)] image>"
    }
}

# ----------------------------------------------------------------------
# USAGE: _ReceiveLegend <volume> <vmin> <vmax> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_ReceiveLegend { ivol vmin vmax size } {
    if { [isconnected] } {
        set bytes [ReceiveBytes $size]
        $_image(legend) configure -data $bytes
        ReceiveEcho <<line "<read $size bytes for [image width $_image(legend)]x[image height $_image(legend)] legend>"

        set c $itk_component(legend)
        set w [winfo width $c]
        set h [winfo height $c]
        foreach { dataobj comp } $_id2obj($ivol) break
        if {"" == [$c find withtag transfunc]} {
            $c create image 10 10 -anchor nw \
                 -image $_image(legend) -tags transfunc
            $c create text 10 [expr {$h-8}] -anchor sw \
                 -fill $itk_option(-plotforeground) -tags vmin
            $c create text [expr {$w-10}] [expr {$h-8}] -anchor se \
                 -fill $itk_option(-plotforeground) -tags vmax
            $c lower transfunc
            $c bind transfunc <ButtonRelease-1> \
                [itcl::code $this _AddIsoMarker $ivol %x %y]
        }
        $c itemconfigure vmin -text $_limits($ivol-min)
        $c coords vmin 10 [expr {$h-8}]

        $c itemconfigure vmax -text  $_limits($ivol-max)
        $c coords vmax [expr {$w-10}] [expr {$h-8}]

        _ShowIsoMarkers $ivol
    }
}

# ----------------------------------------------------------------------
# USAGE: _ReceiveData <volume> <vmin> <vmax> 
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_ReceiveData { args } {
    if { [isconnected] } {
        array set info $args
        set ivol $info(id)
	set _limits($ivol-min) $info(min)
	set _limits($ivol-max) $info(max)
        foreach { dataobj comp } $_id2obj($ivol) break

        if { ![info exists _limits($dataobj-vmin)] } {
            set _limits($dataobj-vmin) $info(min)
            set _limits($dataobj-vmax) $info(max)
        } else {
            if { $_limits($dataobj-vmin) > $info(min) } {
                set _limits($dataobj-vmin) $info(min)
            } 
            if { $_limits($dataobj-vmax) < $info(max) } {
                set _limits($dataobj-vmax) $info(max)
            }
        }
        set _limits(vmin) $info(vmin)
        set _limits(vmax) $info(vmax)
        lappend _sendobjs2 $dataobj
        unset _receiveids($info(id))

	if { ![info exists _isomarkers($ivol)] } {
	    _InitIsoMarkers $ivol
	} else {
	    _HideIsoMarkers $ivol
	}
	# We can update the transfer function now that we know the data limits.
	if { ![_DefineTransferFunction $ivol] } {
	    return
	}
	_fixLegend
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
    if {[llength $_sendobjs] > 0} {
        # send off new data objects
        $_dispatcher event -idle !send_dataobjs
    } else {
        set w [winfo width $itk_component(3dview)]
        set h [winfo height $itk_component(3dview)]
        _send "screen $w $h"
        # nothing to send -- activate the proper ivol
        set first [lindex [get] 0]
        if {"" != $first} {
            set axis [$first hints updir]
            if {"" != $axis} {
                _send "up $axis"
            }
        }
	foreach key [array names _obj2id *-*] {
	    set state [string match $first-* $key]
	    _send "volume state $state $_obj2id($key)"
	}
	set comp [lindex [$dataobj components] 0]
	set ivol $_obj2id($first-$comp) 

        _ShowIsoMarkers $ivol 

	if { [info exists _settings($this-$ivol-opacity)] } {
	    set inner [$itk_component(controls).panel component inner]
	    set sval $_settings($this-$ivol-opacity)
	    set sval [expr {round($sval * 100)}]
	    $inner.scales.opacity set $sval
	}
	if { [info exists _settings($this-$ivol-thickness)] } {
	    set inner [$itk_component(controls).panel component inner]
	    set sval $_settings($this-$ivol-thickness)
	    set sval [expr {round($sval * 10000.0)}]
	    $inner.scales.thickness set $sval
	}
        UpdateTransferFunction $ivol

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

    #
    # Reset the camera and other view parameters
    #
    _send "camera angle [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]"
    _send "camera zoom $_view(zoom)"

    _fixSettings light
    _fixSettings transp
    _fixSettings isosurface 
    _fixSettings opacity
    _fixSettings thickness
    _fixSettings grid
    _fixSettings axes
    _fixSettings outline


#     if {"" == $itk_option(-plotoutline)} {
#         _send "volume outline state off"
#     } else {
#         _send "volume outline state on"
#         set rgb [Color2RGB $itk_option(-plotoutline)]
#         _send "volume outline color $rgb"
#     }
    _send "volume axis label x \"\""
    _send "volume axis label y \"\""
    _send "volume axis label z \"\""
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
            set _view(zoom) [expr {$_view(zoom)* 1.25}]
        }
        "out" {
            set _view(zoom) [expr {$_view(zoom)* 0.8}]
        }
        "reset" {
            array set _view {
                theta 45
                phi 45
                psi 0
                zoom 1.0
            }
            set xyz [Euler2XYZ $_view(theta) $_view(phi) $_view(psi)]
            _send "camera angle $xyz"
        }
    }
    _send "camera zoom $_view(zoom)"
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
# USAGE: _move drag <x> <y>
# USAGE: _move release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_move {option x y} {
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
                _move click $x $y
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

                array set _view [subst {
                    theta $theta
                    phi $phi
                    psi $psi
                }]
                set xyz [Euler2XYZ $theta $phi $psi]
                _send "camera angle $xyz"
                set _click(x) $x
                set _click(y) $y
            }
        }
        release {
            _move drag $x $y
            $itk_component(3dview) configure -cursor ""
            catch {unset _click}
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
#                *($_limits(${axis}max)-$_limits(${axis}min))
#                  + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]

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
#        *($_limits(${axis}max)-$_limits(${axis}min))
#          + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]
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
            if {[isconnected]} {
                set dataobj [lindex [get] 0]
                if {$dataobj != 0} {
		    set val [$inner.scales.opacity get]
		    set sval [expr { 0.01 * double($val) }]
		    # FIXME: This will change when we can select the current 
		    #        volume.
		    set comp [lindex [$dataobj components] 0]
		    set tag $dataobj-$comp
		    if { [info exists _obj2id($tag)] } {
			set ivol $_obj2id($tag)
			set _settings($this-$ivol-opacity) $sval
			UpdateTransferFunction $ivol
		    }
                }
            }
        }

        thickness {
            if {[isconnected]} {
                set dataobj [lindex [get] 0]
                if {$dataobj != 0} {
                    set val [$inner.scales.thickness get]
                    # Scale values between 0.00001 and 0.01000
                    set sval [expr {0.0001*double($val)}]
		    # FIXME: This will change when we can select the current 
		    #        volume.
		    set comp [lindex [$dataobj components] 0]
		    set tag $dataobj-$comp
		    if { [info exists _obj2id($tag)] } {
			set ivol $_obj2id($tag)
			set _settings($this-$ivol-thickness) $sval
			UpdateTransferFunction $ivol
		    }
                }
            }
        }
        "outline" {
            if {[isconnected]} {
                _send "volume outline state $_settings($this-outline)"
            }
        }           
        "isosurface" {
            if {[isconnected]} {
                _send "volume shading isosurface $_settings($this-isosurface)"
            }
        }           
        "grid" {
            if { [isconnected] } {
                _send "grid visible $_settings($this-grid)"
            }
        }
        "axes" {
            if { [isconnected] } {
                _send "axis visible $_settings($this-axes)"
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
    set ivol ""
    if { $ivol == "" } {
	set dataobj [lindex [get] 0]
	if {"" != $dataobj} {
	    set comp [lindex [$dataobj components] 0]
	    if {[info exists _obj2id($dataobj-$comp)]} {
		set ivol $_obj2id($dataobj-$comp)
	    }
	}
    }
    if {$w > 0 && $h > 0 && "" != $ivol} {
        _send "legend $ivol $w $h"
    } else {
        #$itk_component(legend) delete all
    }
}

# ----------------------------------------------------------------------
# USAGE: _DefineTransferFunction <ivol>
#
# Used internally to compute the colormap and alpha map used to define
# a transfer function for the specified component in a data object.
# Returns: name {v r g b ...} {v w ...}
# ----------------------------------------------------------------------
itcl::body Rappture::NanovisViewer::_DefineTransferFunction { ivol } {
    array set style {
        -color rainbow
        -levels 6
        -opacity 1.0
    }
    foreach {dataobj comp} $_id2obj($ivol) break
    array set style [lindex [$dataobj components -style $comp] 0]
    set sname "$style(-color):$style(-levels):$style(-opacity)"
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

    set tag $this-$ivol
    if { ![info exists _settings($tag-opacity)] } {
	set _settings($tag-opacity) $style(-opacity)
    }
    set max $_settings($tag-opacity)

    set isovalues {}
    foreach m $_isomarkers($ivol) {
        lappend isovalues [$m GetRelativeValue]
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
        }
        if { $x2 < 0.0 } {
            set x2 0.0
        }
        if { $x3 > 1.0 } {
            set x3 1.0 
        }
        if { $x4 > 1.0 } {
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
    set _id2style($ivol) $sname
    set _styles($sname) 1
    set cmds [subst {
	transfunc define $sname { $cmap } { $wmap }
	volume shading transfunc $sname $ivol
    }]
    return [SendBytes $cmds]
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

itcl::body Rappture::NanovisViewer::_HideIsoMarkers { ivol } {
    if { [info exists _isomarkers($ivol)] } {
        foreach m $_isomarkers($ivol) {
            $m Hide
        }
    }
}

itcl::body Rappture::NanovisViewer::_ShowIsoMarkers { ivol } {
    # Clear all markers
    foreach dataobj [array names _all_data_objs] {
        foreach comp [$dataobj components] {
	    _HideIsoMarkers $_obj2id($dataobj-$comp)
	}
    }
    if { ![info exists _isomarkers($ivol)] } {
        return
    }
    foreach m $_isomarkers($ivol) {
        $m Show
    }
}

#
# The -levels option takes a single value that represents the number
# of evenly distributed markers based on the current data range. Each
# marker is a relative value from 0.0 to 1.0.
#
itcl::body Rappture::NanovisViewer::_ParseLevelsOption { ivol levels } {
    set c $itk_component(legend)
    regsub -all "," $levels " " levels
    if {[string is int $levels]} {
	for {set i 1} { $i <= $levels } {incr i} {
	    set x [expr {double($i)/($levels+1)}]
	    set m [IsoMarker \#auto $c $this $ivol]
	    $m SetRelativeValue $x
	    lappend _isomarkers($ivol) $m 
	}
    } else {
        foreach x $levels {
	    set m [IsoMarker \#auto $c $this $ivol]
	    $m SetRelativeValue $x
	    lappend _isomarkers($ivol) $m 
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
itcl::body Rappture::NanovisViewer::_ParseMarkersOption { ivol markers } {
    set c $itk_component(legend)
    regsub -all "," $markers " " markers
    foreach marker $markers {
        set n [scan $marker "%g%s" value suffix]
        if { $n == 2 && $suffix == "%" } {
            # ${n}% : Set relative value. 
            set value [expr {$value * 0.01}]
            set m [IsoMarker \#auto $c $this $ivol]
            $m SetRelativeValue $value
            lappend _isomarkers($ivol) $m
        } else {
            # ${n} : Set absolute value.
            set m [IsoMarker \#auto $c $this $ivol]
            $m SetAbsoluteValue $value
            lappend _isomarkers($ivol) $m
        }
    }
}

itcl::body Rappture::NanovisViewer::_InitIsoMarkers { ivol } {
    array set style {
        -levels 6
    }
    set _isomarkers($ivol) ""
    foreach {dataobj comp} $_id2obj($ivol) break
    array set style [lindex [$dataobj components -style $comp] 0]
    if { [info exists style(-markers)] } {
	_ParseMarkersOption $ivol $style(-markers)
    } else {
	_ParseLevelsOption $ivol $style(-levels)
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
itcl::body Rappture::NanovisViewer::UpdateTransferFunction { ivol } {
    if {![info exists _id2style($ivol)]} {
        return
    }
    set _currentVolId $ivol
    $_dispatcher event -idle !send_transfunc
}

itcl::body Rappture::NanovisViewer::_AddIsoMarker { ivol x y } {
    set c $itk_component(legend)
    set m [IsoMarker \#auto $c $this $ivol]
    set w [winfo width $c]
    $m SetRelativeValue [expr {double($x-10)/($w-20)}]
    $m Show
    lappend _isomarkers($ivol) $m
    UpdateTransferFunction $ivol
    return 1
}

itcl::body Rappture::NanovisViewer::RemoveDuplicateIsoMarker { marker x } {
    set ivol [$marker GetVolume]
    set bool 0
    if { [info exists _isomarkers($ivol)] } {
        set list {}
        set marker [namespace tail $marker]
        foreach m $_isomarkers($ivol) {
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
        set _isomarkers($ivol) $list
        UpdateTransferFunction $ivol
    }
    return $bool
}

itcl::body Rappture::NanovisViewer::OverIsoMarker { marker x } {
    set ivol [$marker GetVolume]
    if { [info exists _isomarkers($ivol)] } {
        set marker [namespace tail $marker]
        foreach m $_isomarkers($ivol) {
            set sx [$m GetScreenPosition]
            if { $m != $marker } {
                set bool [expr { $x >= ($sx-3) && $x <= ($sx+3) }]
                $m Activate $bool
            }
        }
    }
    return ""
}
