
# ----------------------------------------------------------------------
#  COMPONENT: molvisviewer - view a molecule in 3D
#
#  This widget brings up a 3D representation of a molecule
#  It connects to the Molvis server running on a rendering farm,
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

option add *MolvisViewer.width 4i widgetDefault
option add *MolvisViewer.height 4i widgetDefault
option add *MolvisViewer.foreground black widgetDefault
option add *MolvisViewer.controlBackground gray widgetDefault
option add *MolvisViewer.controlDarkBackground #999999 widgetDefault
option add *MolvisViewer.font -*-helvetica-medium-r-normal-*-12-* widgetDefault

# must use this name -- plugs into Rappture::resources::load
proc MolvisViewer_init_resources {} {
    Rappture::resources::register \
        molvis_server Rappture::MolvisViewer::SetServerList
}

set debug 0
proc debug { args } {
    global debug
    if { $debug } {
        puts stderr "[info level -1]: $args"
    }
}

itcl::class Rappture::MolvisViewer {
    inherit Rappture::VisViewer

    itk_option define -device device Device ""

    constructor { hostlist args } { 
        Rappture::VisViewer::constructor $hostlist 
    } { 
        # defined below 
    }
    destructor { 
        # defined below 
    }
    public proc SetServerList { namelist } {
	Rappture::VisViewer::SetServerList "pymol" $namelist
    }
    public method Connect {}
    public method Disconnect {}
    public method isconnected {}
    public method download {option args}

    public method add {dataobj {options ""}}
    public method get {}
    public method delete {args}
    public method parameters {title args} { # do nothing }

    public method emblems {option}
    public method rock {option}
    public method representation {option {model "all"} }
    public method ResetView {} 

    protected method _send {args}
    protected method _update { args }
    protected method _rebuild { }
    protected method _zoom {option {factor 10}}
    protected method _pan {option x y}
    protected method _configure {w h}
    protected method _unmap {}
    protected method _map {}
    protected method _vmouse2 {option b m x y}
    protected method _vmouse  {option b m x y}
    private method _receive_image { size cacheid frame rock } 

    private variable _inrebuild 0

    private variable _mevent       ;# info used for mouse event operations
    private variable _rocker       ;# info used for rock operations
    private variable _dlist ""    ;# list of dataobj objects
    private variable _dataobjs     ;# data objects on server
    private variable _dobj2transparency  ;# maps dataobj => transparency
    private variable _dobj2raise  ;# maps dataobj => raise flag 0/1
    private variable _dobj2ghost

    private variable view_

    private variable _model
    private variable _mlist
    private variable _mrepresentation "ballnstick"

    private variable _imagecache
    private variable _state
    private variable _labels  "default"
    private variable _cacheid ""
    private variable _cacheimage ""
    private variable _busy 0

    private variable delta1_ 10
    private variable delta2_ 2

    private common _settings  ;# array of settings for all known widgets
}

itk::usual MolvisViewer {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::constructor {hostlist args} {
    # Register events to the dispatcher.  Base class expects !rebuild 
    # event to be registered.

    # Rebuild
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"
    # Rocker
    $_dispatcher register !rocker
    $_dispatcher dispatch $this !rocker "[itcl::code $this rock step]; list"
    # Mouse Event
    $_dispatcher register !mevent
    $_dispatcher dispatch $this !mevent "[itcl::code $this _mevent]; list"

    # Populate the slave interpreter with commands to handle responses from
    # the visualization server.
    $_parser alias image [itcl::code $this _receive_image]

    set _rocker(dir) 1
    set _rocker(client) 0
    set _rocker(server) 0
    set _rocker(on) 0
    set _state(server) 1
    set _state(client) 1
    set _hostlist $hostlist

    array set view_ {
	zoom 0
	mx 0
	my 0
	mz 0
	x  0
	y  0
	z  0
    }

    array set _settings [subst {
        $this-model $_mrepresentation 
        $this-modelimg [Rappture::icon ballnstick]
        $this-emblems 0 
        $this-rock 0 
    }]

    #
    # Set up the widgets in the main body
    #
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
            -command [itcl::code $this ResetView]
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
    pack $itk_component(settings) -side top -pady {8 2}

    Rappture::Balloon $itk_component(controls).panel -title "Rendering Options"
    set inner [$itk_component(controls).panel component inner]
    frame $inner.model
    pack $inner.model -side top -fill x
    set fg [option get $itk_component(hull) font Font]

    label $inner.model.pict -image $_settings($this-modelimg)
    pack $inner.model.pict -side left -anchor n
    label $inner.model.heading -text "Method for drawing atoms:"
    pack $inner.model.heading -side top -anchor w
    radiobutton $inner.model.bstick -text "Balls and sticks" \
        -command [itcl::code $this representation ballnstick all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value ballnstick
    pack $inner.model.bstick -side top -anchor w
    radiobutton $inner.model.spheres -text "Spheres" \
        -command [itcl::code $this representation spheres all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value spheres
    pack $inner.model.spheres -side top -anchor w
    radiobutton $inner.model.lines -text "Lines" \
        -command [itcl::code $this representation lines all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value lines
    pack $inner.model.lines -side top -anchor w

    checkbutton $inner.labels -text "Show labels on atoms" \
        -command [itcl::code $this emblems update] \
        -variable Rappture::MolvisViewer::_settings($this-emblems)
    pack $inner.labels -side top -anchor w -pady {4 1}

    checkbutton $inner.rock -text "Rock model back and forth" \
        -command [itcl::code $this rock toggle] \
        -variable Rappture::MolvisViewer::_settings($this-rock)
    pack $inner.rock -side top -anchor w -pady {1 4}

    #
    # Shortcuts
    #
    itk_component add shortcuts {
        frame $itk_component(controls).shortcuts
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(shortcuts) -side top

    itk_component add labels {
        label $itk_component(shortcuts).labels \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -bitmap [Rappture::icon atoms]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(labels) -side left -padx {4 1} -pady 4 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(labels) "Show/hide the labels on atoms"
    bind $itk_component(labels) <ButtonPress> \
        [itcl::code $this emblems toggle]

    itk_component add rock {
        label $itk_component(shortcuts).rock \
            -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -bitmap [Rappture::icon rocker]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(rock) -side left -padx 1 -pady 4 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(rock) "Rock model back and forth"

    bind $itk_component(rock) <ButtonPress> \
        [itcl::code $this rock toggle]

    #
    # RENDERING AREA
    #

    set _image(id) ""

    # set up bindings for rotation
    bind $itk_component(3dview) <ButtonPress-1> \
        [itcl::code $this _vmouse click %b %s %x %y]
    bind $itk_component(3dview) <B1-Motion> \
        [itcl::code $this _vmouse drag 1 %s %x %y]
    bind $itk_component(3dview) <ButtonRelease-1> \
        [itcl::code $this _vmouse release %b %s %x %y]

    bind $itk_component(3dview) <ButtonPress-2> \
        [itcl::code $this _pan click %x %y]
    bind $itk_component(3dview) <B2-Motion> \
        [itcl::code $this _pan drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-2> \
        [itcl::code $this _pan release %x %y]

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
    bind $itk_component(3dview) <KeyPress-Prior> \
	[itcl::code $this _zoom out 2]
    bind $itk_component(3dview) <KeyPress-Next> \
	[itcl::code $this _zoom in 2]

    bind $itk_component(3dview) <Enter> "focus $itk_component(3dview)"


    if {[string equal "x11" [tk windowingsystem]]} {
	bind $itk_component(3dview) <4> [itcl::code $this _zoom out 2]
	bind $itk_component(3dview) <5> [itcl::code $this _zoom in 2]
    }

    # set up bindings to bridge mouse events to server
    #bind $itk_component(3dview) <ButtonPress> \
    #   [itcl::code $this _vmouse2 click %b %s %x %y]
    #bind $itk_component(3dview) <ButtonRelease> \
    #    [itcl::code $this _vmouse2 release %b %s %x %y]
    #bind $itk_component(3dview) <B1-Motion> \
    #    [itcl::code $this _vmouse2 drag 1 %s %x %y]
    #bind $itk_component(3dview) <B2-Motion> \
    #    [itcl::code $this _vmouse2 drag 2 %s %x %y]
    #bind $itk_component(3dview) <B3-Motion> \
    #    [itcl::code $this _vmouse2 drag 3 %s %x %y]
    #bind $itk_component(3dview) <Motion> \
    #    [itcl::code $this _vmouse2 move 0 %s %x %y]

    bind $itk_component(3dview) <Configure> \
        [itcl::code $this _configure %w %h]
    bind $itk_component(3dview) <Unmap> \
        [itcl::code $this _unmap]
    bind $itk_component(3dview) <Map> \
        [itcl::code $this _map]

    eval itk_initialize $args
    Connect 
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::destructor {} {
    VisViewer::Disconnect

    image delete $_image(plot)
    array unset _settings $this-*
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
itcl::body Rappture::MolvisViewer::download {option args} {
    switch $option {
        coming {}
        controls {}
        now {
            return [list .jpg [Rappture::encoding::decode -as b64 [$_image(plot) data -format jpeg]]]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

#
# isconnected --
#
#       Indicates if we are currently connected to the visualization server.
#
itcl::body Rappture::MolvisViewer::isconnected {} {
    return [VisViewer::IsConnected]
}


#
# Connect --
#
#       Establishes a connection to a new visualization server.
#
itcl::body Rappture::MolvisViewer::Connect {} {
    #$_image(plot) blank
    set hosts [GetServerList "pymol"]
    if { "" == $hosts } {
        return 0
    }
    set result [VisViewer::Connect $hosts]
    if { $result } {
        set _rocker(server) 0
        set _cacheid 0
        _send "raw -defer {set auto_color,0}"
        _send "raw -defer {set auto_show_lines,0}"
    }
    return $result
}

#
# Disconnect --
#
#       Clients use this method to disconnect from the current rendering
#       server.
#
itcl::body Rappture::MolvisViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    catch { after cancel $_rocker(afterid) }
    catch { after cancel $_mevent(afterid) }
    array unset _dataobjs
    array unset _model
    array unset _mlist
    array unset _imagecache

    set _state(server) 1
    set _state(client) 1
    set _outbuf ""
}

itcl::body Rappture::MolvisViewer::_send { args } {
    if { $_state(server) != $_state(client) } {
        if { ![SendBytes "frame -defer $_state(client)"] } {
            set _state(server) $_state(client)
        }
    }

    if { $_rocker(server) != $_rocker(client) } {
        if { ![SendBytes "rock -defer $_rocker(client)"] } {
            set _rocker(server) $_rocker(client)
        }
    }
    eval SendBytes $args
}

#
# _receive_image -bytes <size>
#
#     Invoked automatically whenever the "image" command comes in from
#     the rendering server.  Indicates that binary image data with the
#     specified <size> will follow.
#
itcl::body Rappture::MolvisViewer::_receive_image { size cacheid frame rock } {
    set tag "$frame,$rock"
    if { $cacheid != $_cacheid } {
        array unset _imagecache 
        set _cacheid $cacheid
    }
    set _imagecache($tag) [ReceiveBytes $size]
 
    #puts stderr "CACHED: $tag,$cacheid"
    $_image(plot) configure -data $_imagecache($tag)
    set _image(id) $tag
}


# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_rebuild {} {
    if { $_inrebuild } { 
        # don't allow overlapping rebuild calls
        return
    }

    #set _inrebuild 1
    set changed 0
    set _busy 1

    $itk_component(3dview) configure -cursor watch

    # refresh GUI (primarily to make pending cursor changes visible)
    update idletasks 
    set dlist [get]
    foreach dev $dlist {
        set model [$dev get components.molecule.model]
        set state [$dev get components.molecule.state]
        
        if {"" == $model } { 
            set model "molecule"
            scan $dev "::libraryObj%d" suffix
            set model $model$suffix
        }

        if {"" == $state} { set state $_state(server) }

        if { ![info exists _mlist($model)] } { # new, turn on
            set _mlist($model) 2
        } elseif { $_mlist($model) == 1 } { # on, leave on
            set _mlist($model) 3 
        } elseif { $_mlist($model) == 0 } { # off, turn on
            set _mlist($model) 2
        }
        if { ![info exists _dataobjs($model-$state)] } {
            set data1      ""
            set serial   0

            foreach _atom [$dev children -type atom components.molecule] {
                set symbol [$dev get components.molecule.$_atom.symbol]
                set xyz [$dev get components.molecule.$_atom.xyz]
                regsub {,} $xyz {} xyz
                scan $xyz "%f %f %f" x y z
                set recname  "ATOM  "
                set altLoc   ""
                set resName  ""
                set chainID  ""
                set Seqno    ""
                set occupancy  1
                set tempFactor 0
                set recID      ""
                set segID      ""
                set element    ""
                set charge     ""
                set atom $symbol
                set line [format "%6s%5d %4s%1s%3s %1s%5s   %8.3f%8.3f%8.3f%6.2f%6.2f%8s\n" $recname $serial $atom $altLoc $resName $chainID $Seqno $x $y $z $occupancy $tempFactor $recID]
                append data1 $line
                incr serial
            }
            set data2 [$dev get components.molecule.pdb]
            if {"" != $data1} {
                _send "loadpdb -defer \"$data1\" $model $state"
                set _dataobjs($model-$state)  1
            }
            if {"" != $data2} {
                _send "loadpdb -defer \"$data2\" $model $state"
                set _dataobjs($model-$state)  1
            }
        }
        if { ![info exists _model($model-transparency)] } {
            set _model($model-transparency) "undefined"
        }
        if { ![info exists _model($model-representation)] } {
            set _model($model-representation) "undefined"
            set _model($model-newrepresentation) $_mrepresentation
        }
        if { $_model($model-transparency) != $_dobj2transparency($dev) } {
            set _model($model-newtransparency) $_dobj2transparency($dev)
        } 
    }

    # enable/disable models as required (0=off->off, 1=on->off, 2=off->on,
    # 3=on->on)

    foreach obj [array names _mlist] {
        if { $_mlist($obj) == 1 } {
            _send "disable -defer $obj"
            set _mlist($obj) 0
            set changed 1
        } elseif { $_mlist($obj) == 2 } {
            set _mlist($obj) 1
            _send "enable -defer $obj"
            if { $_labels } {
                _send "label -defer on"
            } else {
                _send "label -defer off"
            }
            set changed 1
        } elseif { $_mlist($obj) == 3 } {
            set _mlist($obj) 1
        }

        if { $_mlist($obj) == 1 } {
            if {  [info exists _model($obj-newtransparency)] || 
                  [info exists _model($obj-newrepresentation)] } {
                if { ![info exists _model($obj-newrepresentation)] } {
                    set _model($obj-newrepresentation) $_model($obj-representation)
                }
                if { ![info exists _model($obj-newtransparency)] } {
                    set _model($obj-newtransparency) $_model($obj-transparency)
                }
                set rep $_model($obj-newrepresentation)
                set transp $_model($obj-newtransparency)
                _send "$_model($obj-newrepresentation) -defer -model $obj -$_model($obj-newtransparency)"
                set changed 1
                set _model($obj-transparency) $_model($obj-newtransparency)
                set _model($obj-representation) $_model($obj-newrepresentation)
                catch {
                    unset _model($obj-newtransparency)
                    unset _model($obj-newrepresentation)
                }
            }
        }

    }

    if { $changed } {
        array unset _imagecache
    }
    if { $dlist == "" } {
        set _state(server) 1
        set _state(client) 1
        _send "frame -push 1"
    } elseif { ![info exists _imagecache($state,$_rocker(client))] } {
        set _state(server) $state
        set _state(client) $state
        _send "frame -push $state"
    } else {
        set _state(client) $state
        _update
    }
    # Reset screen size
    set w  [winfo width $itk_component(3dview)] 
    set h  [winfo height $itk_component(3dview)] 
    _send "screen $w $h"
    # Reset viewing parameters
    _send "reset"
    _send "rotate $view_(mx) $view_(my) $view_(mz)"
    _send "pan $view_(x) $view_(y)"
    _send "zoom $view_(zoom)"

    set _inrebuild 0
    $itk_component(3dview) configure -cursor ""
}

itcl::body Rappture::MolvisViewer::_unmap { } {
    #pause rocking loop while unmapped (saves CPU time)
    rock pause

    # Blank image, mark current image dirty
    # This will force reload from cache, or remain blank if cache is cleared
    # This prevents old image from briefly appearing when a new result is added
    # by result viewer

    #$_image(plot) blank
    set _image(id) ""
}

itcl::body Rappture::MolvisViewer::_map { } {
    if { [isconnected] } {
        # resume rocking loop if it was on
        rock unpause
        # rebuild image if modified, or redisplay cached image if not
        $_dispatcher event -idle !rebuild
    }
}

itcl::body Rappture::MolvisViewer::_configure { w h } {
    $_image(plot) configure -width $w -height $h
    # immediately invalidate cache, defer update until mapped
    array unset _imagecache 
    if { [isconnected] } {
        if { [winfo ismapped $itk_component(3dview)] } {
            _send "screen $w $h"
	    # Why do a reset?
            #_send "reset -push"
        } else {
            _send "screen -defer $w $h"
	    # Why do a reset?
            #_send "reset -push"
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
itcl::body Rappture::MolvisViewer::_pan {option x y} {
    if { $option == "set" } {
        set dx $x
        set dy $y
	set view_(x) [expr $view_(x) + $dx]
	set view_(y) [expr $view_(y) + $dy]
        _send "pan $dx $dy"
	return
    }
    if { ![info exists _mevent(x)] } {
        set option "click"
    }
    if { $option == "click" } { 
        $itk_component(3dview) configure -cursor hand1
    }
    if { $option == "drag" || $option == "release" } {
        set dx [expr $x - $_mevent(x)]
        set dy [expr $y - $_mevent(y)]
	set view_(x) [expr $view_(x) + $dx]
	set view_(y) [expr $view_(y) + $dy]
        _send "pan $dx $dy"
    }
    set _mevent(x) $x
    set _mevent(y) $y
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::_zoom {option {factor 10}} {
    switch -- $option {
        "in" {
	    set view_(zoom) [expr $view_(zoom) + $factor]
            _send "zoom $factor"
        }
        "out" {
	    set view_(zoom) [expr $view_(zoom) - $factor]
            _send "zoom -$factor"
        }
        "reset" {
	    set view_(zoom) 0
            _send "reset"
        }
    }
}

itcl::body Rappture::MolvisViewer::_update { args } {
    set tag "$_state(client),$_rocker(client)"
    if { $_image(id) != "$tag" } {
        if { [info exists _imagecache($tag)] } {
            #puts stderr "DISPLAYING CACHED IMAGE"
            $_image(plot) configure -data $_imagecache($tag)
            set _image(id) "$tag"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: rock on|off|toggle
# USAGE: rock pause|unpause|step
#
# Used to control the "rocking" model for the molecule being displayed.
# Clients should use only the on/off/toggle options; the rest are for
# internal control of the rocking motion.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::rock { option } {
    # cancel any pending rocks
    if { [info exists _rocker(afterid)] } { 
        after cancel $_rocker(afterid)
        unset _rocker(afterid)
    }

    if { $option == "toggle" } {
        if { $_rocker(on) } {
            set option "off"
        } else {
            set option "on"
        }
    }
    if { $option == "on" || ($option == "toggle" && !$_rocker(on)) } {
        set _rocker(on) 1
        set _settings($this-rock) 1
        $itk_component(rock) configure -relief sunken
    } elseif { $option == "off" || ($option == "toggle" && $_rocker(on)) } {
        set _rocker(on) 0
        set _settings($this-rock) 0
        $itk_component(rock) configure -relief raised
    } elseif { $option == "step"} {
        if { $_rocker(client) >= 10 } {
            set _rocker(dir) -1
        } elseif { $_rocker(client) <= -10 } {
            set _rocker(dir) 1
        }
        set _rocker(client) [expr {$_rocker(client) + $_rocker(dir)}]
        if { ![info exists _imagecache($_state(server),$_rocker(client))] } {
            set _rocker(server) $_rocker(client)
            _send "rock $_rocker(client)"
        }
        _update
    }
    if { $_rocker(on) && $option != "pause" } {
         set _rocker(afterid) [after 200 [itcl::code $this rock step]]
    }
}

itcl::body Rappture::MolvisViewer::_vmouse2 {option b m x y} {
    set now [clock clicks -milliseconds]
    set vButton [expr $b - 1]
    set vModifier 0
    set vState 1

    if { $m & 1 }      { set vModifier [expr $vModifier | 1 ] }
    if { $m & 4 }      { set vModifier [expr $vModifier | 2 ] }
    if { $m & 131072 } { set vModifier [expr $vModifier | 4 ] }

    if { $option == "click"   } { set vState 0 }
    if { $option == "release" } { set vState 1 }
    if { $option == "drag"    } { set vState 2 }
    if { $option == "move"    } { set vState 3 }

    if { $vState == 2 || $vState == 3} {
        set diff 0

        catch { set diff [expr $now - $_mevent(time)] } 
        if {$diff < 75} { # 75ms between motion updates
            return
        }
    }
    _send "vmouse $vButton $vModifier $vState $x $y"
    set _mevent(time) $now
}

itcl::body Rappture::MolvisViewer::_vmouse {option b m x y} {
    set now  [clock clicks -milliseconds]
    # cancel any pending delayed dragging events
    if { [info exists _mevent(afterid)] } { 
        after cancel $_mevent(afterid)
        unset _mevent(afterid)
    }

    if { ![info exists _mevent(x)] } {
        set option "click"
    }
    if { $option == "click" } { 
        $itk_component(3dview) configure -cursor fleur
    }
    if { $option == "drag" || $option == "release" } {
        set diff 0
        catch { set diff [expr $now - $_mevent(time) ] }
        if {$diff < 75 && $option == "drag" } { # 75ms between motion updates
            set _mevent(afterid) [after [expr 75 - $diff] [itcl::code $this _vmouse drag $b $m $x $y]]
            return
        }
        set w [winfo width $itk_component(3dview)]
        set h [winfo height $itk_component(3dview)]
        if {$w <= 0 || $h <= 0} {
            return
        }
        set x1 [expr $w / 3]
        set x2 [expr $x1 * 2]
        set y1 [expr $h / 3]
        set y2 [expr $y1 * 2]
        set dx [expr $x - $_mevent(x)]
        set dy [expr $y - $_mevent(y)]
        set mx 0
        set my 0
        set mz 0

        if { $_mevent(x) < $x1 } {
            set mz $dy
        } elseif { $_mevent(x) < $x2 } {
            set mx $dy
        } else {
            set mz [expr -$dy]
        }

        if { $_mevent(y) < $y1 } {
            set mz [expr -$dx]
        } elseif { $_mevent(y) < $y2 } {
            set my $dx
        } else {
            set mz $dx
        }
	# Accumlate movements
	set view_(mx) [expr {$view_(mx) + $mx}]
	set view_(my) [expr {$view_(my) + $my}]
	set view_(mz) [expr {$view_(mz) + $mz}]
        _send "rotate $mx $my $mz"
    }
    set _mevent(x) $x
    set _mevent(y) $y
    set _mevent(time) $now
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: representation spheres
# USAGE: representation ballnstick
# USAGE: representation lines
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::representation {option {model "all"} } {
    if { $option == $_mrepresentation } { 
        return 
    }
    set _settings($this-modelimg) [Rappture::icon $option]
    set inner [$itk_component(controls).panel component inner]
    $inner.model.pict configure -image $_settings($this-modelimg)

    # Save the current option to set all radiobuttons -- just in case.
    # This method gets called without the user clicking on a radiobutton.
    set _settings($this-model) $option
    set _mrepresentation $option

    if { $model == "all" } {
        set models [array names _mlist]
    } else {
        set models $model
    }
    foreach obj $models {
        if { [info exists _model($obj-representation)] } {
            if { $_model($obj-representation) != $option } {
                set _model($obj-newrepresentation) $option
            } else {
                catch { unset _model($obj-newrepresentation) }
            }
        }
    }
    if { [isconnected] } {
        $_dispatcher event -idle !rebuild
    }
}

# ----------------------------------------------------------------------
# USAGE: emblems on|off|toggle
# USAGE: emblems update
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::emblems {option} {
    switch -- $option {
        on {
            set emblem 1
        }
        off {
            set emblem 0
        }
        toggle {
            if {$_settings($this-emblems)} {
                set emblem 0
            } else {
                set emblem 1
            }
        }
        update {
            set emblem $_settings($this-emblems)
        }
        default {
            error "bad option \"$option\": should be on, off, toggle, or update"
        }
    }
    set _labels $emblem
    if {$emblem == $_settings($this-emblems) && $option != "update"} {
        # nothing to do
        return
    }

    if {$emblem} {
        $itk_component(labels) configure -relief sunken
        set _settings($this-emblems) 1
        _send "label on"
    } else {
        $itk_component(labels) configure -relief raised
        set _settings($this-emblems) 0
        _send "label off"
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise. Only
# -brightness and -raise do anything.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::add { dataobj {options ""}} {
    array set params {
        -color          auto
        -brightness     0
        -width          1
        -raise          0
        -linestyle      solid
        -description    ""
        -param          ""
    }

    foreach {opt val} $options {
        if {![info exists params($opt)]} {
            error "bad settings \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }
 
    set pos [lsearch -exact $dataobj $_dlist]

    if {$pos < 0} {
        if {![Rappture::library isvalid $dataobj]} {
            error "bad value \"$dataobj\": should be Rappture::library object"
        }
    
        if { $_labels == "default" } {
            set emblem [$dataobj get components.molecule.about.emblems]

            if {$emblem == "" || ![string is boolean $emblem] || !$emblem} {
                emblems off
            } else {
                emblems on
            }
        }

        lappend _dlist $dataobj
        if { $params(-brightness) >= 0.5 } {
            set _dobj2transparency($dataobj) "ghost"
        } else {
            set _dobj2transparency($dataobj) "normal"
        }
        set _dobj2raise($dataobj) $params(-raise)

        if { [isconnected] } {
            $_dispatcher event -idle !rebuild
        }
    }
}

#
# ResetView
#
itcl::body Rappture::MolvisViewer::ResetView {} {
    array set view_ {
	mx 0 
	my 0
	mz 0
	x 0
	y 0
	z 0
	zoom 0
    }
    _send "reset"
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist
    foreach obj $dlist {
        if {[info exists _dobj2raise($obj)] && $_dobj2raise($obj)} {
            set i [lsearch -exact $dlist $obj]
            if {$i >= 0} {
                set dlist [lreplace $dlist $i $i]
                lappend dlist $obj
            }
        }
    }
    return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj> <dataobj> ...?
#
# Clients use this to delete a dataobj from the plot. If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _dobj2transparency($dataobj)}
            catch {unset _dobj2color($dataobj)}
            catch {unset _dobj2width($dataobj)}
            catch {unset _dobj2dashes($dataobj)}
            catch {unset _dobj2raise($dataobj)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        if { [isconnected] } {
            $_dispatcher event -idle !rebuild
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -device
# ----------------------------------------------------------------------
itcl::configbody Rappture::MolvisViewer::device {
    if {$itk_option(-device) != "" } {

        if {![Rappture::library isvalid $itk_option(-device)]} {
            error "bad value \"$itk_option(-device)\": should be Rappture::library object"
        }
        $this delete
        $this add $itk_option(-device)
    } else {
        $this delete
    }

    if { [isconnected] } {
        $_dispatcher event -idle !rebuild
    }
}

