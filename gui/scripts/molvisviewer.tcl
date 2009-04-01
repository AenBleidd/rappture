
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
    public method projection {option}
    public method rock {option}
    public method representation {option {model "all"} }
    public method atomscale {option {model "all"} }
    public method bondthickness {option {model "all"} }
    public method ResetView {} 
    public method tab {what who}

    protected method _send {args}
    protected method _update { args }
    protected method _rebuild { }
    protected method _zoom {option {factor 10}}
    protected method _pan {option x y}
    protected method _rotate {option x y}
    protected method _configure {w h}
    protected method _unmap {}
    protected method _map {}
    protected method _vmouse2 {option b m x y}
    protected method _vmouse  {option b m x y}
    private method _ReceiveImage { size cacheid frame rock }
    private method _BuildViewTab {}
    private method GetPngImage { widget width height }
    private method WaitIcon { option widget }
    private variable icon_ 0
    private variable _inrebuild 0

    private variable _mevent       ;# info used for mouse event operations
    private variable _rocker       ;# info used for rock operations
    private variable _dlist ""    ;# list of dataobj objects
    private variable _dataobjs     ;# data objects on server
    private variable _dobj2transparency  ;# maps dataobj => transparency
    private variable _dobj2raise  ;# maps dataobj => raise flag 0/1
    private variable _dobj2ghost

    private variable view_
    private variable click_

    private variable _model
    private variable _mlist
    private variable _mrepresentation "ballnstick"

    private variable _imagecache
    private variable _state
    private variable _labels  "default"
    private variable _cacheid ""
    private variable _cacheimage ""

    private variable delta1_ 10
    private variable delta2_ 2

    private common _settings  ;# array of settings for all known widgets
    private variable initialized_

    common _downloadPopup           ;# download options from popup
    private variable _pdbdata       ;# pdb data from run file sent to pymol
    private common hardcopy_
    private variable nextToken_ 0
    private variable headings_
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
    $_dispatcher register !pngtimeout
    $_dispatcher register !waiticon

    array set _downloadPopup {
	format draft
    }

    # Populate the slave interpreter with commands to handle responses from
    # the visualization server.
    $_parser alias image [itcl::code $this _ReceiveImage]

    set _rocker(dir) 1
    set _rocker(client) 0
    set _rocker(server) 0
    set _rocker(on) 0
    set _state(server) 1
    set _state(client) 1
    set _hostlist $hostlist

    array set view_ {
	theta   45
	phi     45
	psi     0
	vx 0
	vy 0
	vz 0
	zoom 0
	mx 0
	my 0
	mz 0
	x  0
	y  0
	z  0
	width 0
	height 0
    }

    # Setup default settings for widget.
    array set _settings [subst {
	$this-model     ballnstick
	$this-modelimg  [Rappture::icon ballnstick]
	$this-emblems   no
	$this-rock      no
	$this-ortho     no
	$this-atomscale 0.25
	$this-bondthickness 0.15
    }]

    #
    # Set up the widgets in the main body
    #
    itk_component add reset {
	button $itk_component(controls).reset \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -image [Rappture::icon reset-view] \
	    -command [itcl::code $this ResetView]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(reset) -padx 1 -pady 2
    Rappture::Tooltip::for $itk_component(reset) \
	"Reset the view to the default zoom level"

    itk_component add zoomin {
	button $itk_component(controls).zin \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -image [Rappture::icon zoom-in] \
	    -command [itcl::code $this _zoom in]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground \
	    controlBackground Background
    }
    pack $itk_component(zoomin) -padx 2 -pady { 0 2 }
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
	button $itk_component(controls).zout \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -image [Rappture::icon zoom-out] \
	    -command [itcl::code $this _zoom out]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(zoomout) -padx 2 -pady { 0 2 }
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

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
	    -relief "raised" -image [Rappture::icon atom-label]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(labels) -padx 2 -pady { 0 2} -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(labels) \
	"Show/hide the labels on atoms"
    bind $itk_component(labels) <ButtonPress> \
	[itcl::code $this emblems toggle]

    itk_component add rock {
	label $itk_component(shortcuts).rock \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon rock-view]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(rock) -padx 2 -pady { 0 2 } -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(rock) "Rock model back and forth"

    bind $itk_component(rock) <ButtonPress> \
	[itcl::code $this rock toggle]


    itk_component add ortho {
	label $itk_component(shortcuts).ortho \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon 3dpers]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }
    pack $itk_component(ortho) -padx 2 -pady { 0 2 } -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(ortho) \
	"Change to orthoscopic projection"

    bind $itk_component(ortho) <ButtonPress> \
	[itcl::code $this projection toggle]
    $this projection perspective

    itk_component add configure_button {
	label $itk_component(controls).configbutton \
	    -borderwidth 1 -padx 1 -pady 1 \
	    -relief "raised" -image [Rappture::icon wrench]
    } {
	usual
	ignore -borderwidth
	rename -highlightbackground -controlbackground controlBackground \
	    Background
    }

    _BuildViewTab

    # Hack around the Tk panewindow.  The problem is that the requested 
    # size of the 3d view isn't set until an image is retrieved from
    # the server.  So the panewindow uses the tiny size.
    set w [expr [winfo reqwidth $itk_component(hull)] - 80]
    blt::table $itk_component(plotarea) \
	0,0 $itk_component(3dview) -fill both -reqwidth $w 

    #
    # RENDERING AREA
    #

    set _image(id) ""

    # set up bindings for rotation
    if 0 {
	bind $itk_component(3dview) <ButtonPress-1> \
	    [itcl::code $this _rotate click %x %y]
	bind $itk_component(3dview) <B1-Motion> \
	    [itcl::code $this _rotate drag %x %y]
	bind $itk_component(3dview) <ButtonRelease-1> \
	    [itcl::code $this _rotate release %x %y]
    } else {
	bind $itk_component(3dview) <ButtonPress-1> \
	    [itcl::code $this _vmouse click %b %s %x %y]
	bind $itk_component(3dview) <B1-Motion> \
	    [itcl::code $this _vmouse drag 1 %s %x %y]
	bind $itk_component(3dview) <ButtonRelease-1> \
	    [itcl::code $this _vmouse release %b %s %x %y]
    }

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
	[itcl::code $this _pan set -50 0]
    bind $itk_component(3dview) <Shift-KeyPress-Right> \
	[itcl::code $this _pan set 50 0]
    bind $itk_component(3dview) <Shift-KeyPress-Up> \
	[itcl::code $this _pan set 0 -50]
    bind $itk_component(3dview) <Shift-KeyPress-Down> \
	[itcl::code $this _pan set 0 50]
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

    $itk_component(scroller) contents $itk_component(view_canvas)
    $itk_component(title) configure -text "$headings_(view)"

    eval itk_initialize $args
    Connect
}

itcl::body Rappture::MolvisViewer::_BuildViewTab {} {

    itk_component add view_canvas {
	canvas $itk_component(scroller).viewcanvas -highlightthickness 0
    } {
	ignore -highlightthickness
    }
    $itk_component(sidebar) insert end "view" \
	-image [Rappture::icon wrench] -text ""  -padx 0 -pady 0 \
	-command [itcl::code $this tab select "view"]
    set headings_(view) "View Settings"

    itk_component add view_frame {
	frame $itk_component(view_canvas).frame -bg white
    } 
    $itk_component(view_canvas) create window 0 0 \
	-anchor nw -window $itk_component(view_frame)
    bind $itk_component(view_frame) <Configure> \
	[itcl::code $this tab resize view]

    set fg [option get $itk_component(hull) font Font]

    set inner $itk_component(view_frame)
    label $inner.drawinglabel -text "Drawing Method" -font "Arial 9 bold"

    label $inner.pict -image $_settings($this-modelimg)
    radiobutton $inner.bstick -text "balls and sticks" \
	-command [itcl::code $this representation ballnstick all] \
	-variable Rappture::MolvisViewer::_settings($this-model) \
	-value ballnstick -font "Arial 9" -pady 0 
    radiobutton $inner.spheres -text "spheres" \
	-command [itcl::code $this representation spheres all] \
	-variable Rappture::MolvisViewer::_settings($this-model) \
	-value spheres -font "Arial 9" -pady 0
    radiobutton $inner.lines -text "lines" \
	-command [itcl::code $this representation lines all] \
	-variable Rappture::MolvisViewer::_settings($this-model) \
	-value lines -font "Arial 9" -pady 0

    scale $inner.atomscale -width 10 -font "Arial 9 bold" \
	-from 0.1 -to 2.0 -resolution 0.05 -label "Atom Scale" \
	-showvalue true -orient horizontal \
	-command [itcl::code $this atomscale] \
	-variable Rappture::MolvisViewer::_settings($this-atomscale)
    $inner.atomscale set $_settings($this-atomscale)

    scale $inner.bondthickness -width 10 -font "Arial 9 bold" \
	-from 0.1 -to 1.0 -resolution 0.025 -label "Bond Thickness" \
	-showvalue true -orient horizontal \
	-command [itcl::code $this bondthickness] \
	-variable Rappture::MolvisViewer::_settings($this-bondthickness)
    $inner.bondthickness set $_settings($this-bondthickness)

    checkbutton $inner.labels -text "Show labels on atoms" \
	-command [itcl::code $this emblems update] \
	-variable Rappture::MolvisViewer::_settings($this-emblems) \
	-font "Arial 9 bold"
    checkbutton $inner.rock -text "Rock model back and forth" \
	-command [itcl::code $this rock toggle] \
	-variable Rappture::MolvisViewer::_settings($this-rock) \
	-font "Arial 9 bold"
    checkbutton $inner.ortho -text "Orthoscopic projection" \
	-command [itcl::code $this projection update] \
	-variable Rappture::MolvisViewer::_settings($this-ortho) \
	 -font "Arial 9 bold"
    blt::table $inner \
	0,0 $inner.drawinglabel -anchor w -columnspan 4 \
	1,1 $inner.pict -anchor w -rowspan 3 \
	1,2 $inner.spheres -anchor w -columnspan 2 \
	2,2 $inner.lines -anchor w -columnspan 2 \
	3,2 $inner.bstick -anchor w -columnspan 2 \
	4,0 $inner.labels -anchor w -columnspan 4 \
	5,0 $inner.rock -anchor w -columnspan 4 \
	6,0 $inner.ortho -anchor w -columnspan 4 \
	8,1 $inner.atomscale -anchor w -columnspan 2 \
	10,1 $inner.bondthickness -anchor w -columnspan 2 

    blt::table configure $inner c0 -resize expand -width 2
    blt::table configure $inner c1 c2 -resize none
    blt::table configure $inner c3 -resize expand
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
	controls {
	    set popup .molvisviewerdownload
	    if {![winfo exists .molvisviewerdownload]} {
		# if we haven't created the popup yet, do it now
		Rappture::Balloon $popup \
		    -title "[Rappture::filexfer::label downloadWord] as..."
		set inner [$popup component inner]
		label $inner.summary -text "" -anchor w
		pack $inner.summary -side top
		set img $_image(plot)
		set res "[image width $img]x[image height $img]"
		radiobutton $inner.draft -text "Image (draft $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value draft
		pack $inner.draft -anchor w

		set res "1200x1200"
		radiobutton $inner.medium -text "Image (standard $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value $res
		pack $inner.medium -anchor w

		set res "2400x2400"
		radiobutton $inner.high -text "Image (high quality $res)" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value $res
		pack $inner.high -anchor w

		radiobutton $inner.pdb -text "PDB File" \
		    -variable Rappture::MolvisViewer::_downloadPopup(format) \
		    -value pdb
		pack $inner.pdb -anchor w
		button $inner.go -text [Rappture::filexfer::label download] \
		    -command [lindex $args 0]
		pack $inner.go -pady 4
	    } else {
		set inner [$popup component inner]
	    }
	    set num [llength [get]]
	    set num [expr {($num == 1) ? "1 result" : "$num results"}]
	    $inner.summary configure -text "[Rappture::filexfer::label downloadWord] $num in the following format:"
	    update idletasks ;# fix initial sizes
	    return $popup
	}
	now {

	    set popup .molvisviewerdownload
	    if {[winfo exists .molvisviewerdownload]} {
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
		"2400x2400" {
		    return [$this GetPngImage [lindex $args 0] 2400 2400]
		}
		"1200x1200" {
		    return [$this GetPngImage [lindex $args 0] 1200 1200]
		}
		pdb {
		    return [list .pdb $_pdbdata]
		}
	    }
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
    if { [isconnected] } {
	return 1
    }
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
    debug "_send $args"
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
# _ReceiveImage -bytes <size>
#
#     Invoked automatically whenever the "image" command comes in from
#     the rendering server.  Indicates that binary image data with the
#     specified <size> will follow.
#
set count 0
itcl::body Rappture::MolvisViewer::_ReceiveImage { size cacheid frame rock } {
    set tag "$frame,$rock"
    global count
    incr count
    debug "$count: cacheid=$cacheid frame=$frame\n"
    if { $cacheid != $_cacheid } {
	array unset _imagecache 
	set _cacheid $cacheid
    }
    #debug "reading $size bytes from proxy\n"
    set data [ReceiveBytes $size]
    #debug "success: reading $size bytes from proxy\n"
    if { $cacheid == "print" } {
	puts stderr "got hardcopy token"
	# $frame is the token that we sent to the proxy.
	set hardcopy_($this-$frame) $data
    } else {
	set _imagecache($tag) $data
	#debug "CACHED: $tag,$cacheid"
	$_image(plot) configure -data $data
	set _image(id) $tag
    }
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
    debug "in rebuild"
    #set _inrebuild 1
    set changed 0

    $itk_component(3dview) configure -cursor watch

    # refresh GUI (primarily to make pending cursor changes visible)
    #update idletasks
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
	    set serial    1

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
		set _pdbdata $data1
		_send "loadpdb -defer \"$data1\" $model $state"
		set _dataobjs($model-$state)  1
	    }
	    # note that pdb files always overwrite xyz files
	    if {"" != $data2} {
		set _pdbdata $data2
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
	_send "frame 1"
    } elseif { ![info exists _imagecache($state,$_rocker(client))] } {
	set _state(server) $state
	set _state(client) $state
	_send "frame $state"
    } else {
	set _state(client) $state
	_update
    }
    # Reset viewing parameters
    set w  [winfo width $itk_component(3dview)] 
    set h  [winfo height $itk_component(3dview)] 
    _send [subst { 
	reset
	screen $w $h
	rotate $view_(mx) $view_(my) $view_(mz)
	pan $view_(x) $view_(y)
	zoom $view_(zoom)
    }]
    debug "rebuild: rotate $view_(mx) $view_(my) $view_(mz)"

    $this projection update
    $this atomscale update
    $this bondthickness update
    $this emblems update

    set _inrebuild 0
    $itk_component(3dview) configure -cursor ""
    debug "exiting rebuild"
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
    debug "in _configure $w $h"
    $_image(plot) configure -width $w -height $h
    # immediately invalidate cache, defer update until mapped
    array unset _imagecache 
    _send "screen $w $h"
}

# ----------------------------------------------------------------------
# USAGE: $this _pan click x y
#        $this _pan drag x y
#        $this _pan release x y
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
	 if {$diff < 25 && $option == "drag" } { # 75ms between motion updates
	     set _mevent(afterid) [after [expr 25 - $diff] [itcl::code $this _vmouse drag $b $m $x $y]]
	     return
	 }
	set w [winfo width $itk_component(3dview)]
	set h [winfo height $itk_component(3dview)]
	if {$w <= 0 || $h <= 0} {
	    return
	}
	set x1 [expr double($w) / 3]
	set x2 [expr $x1 * 2]
	set y1 [expr double($h) / 3]
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
	debug "_vmmouse: rotate $view_(mx) $view_(my) $view_(mz)"
    }
    set _mevent(x) $x
    set _mevent(y) $y
    set _mevent(time) $now
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
itcl::body Rappture::MolvisViewer::_rotate {option x y} {
    set now  [clock clicks -milliseconds]
    update idletasks
    # cancel any pending delayed dragging events
    if { [info exists _mevent(afterid)] } {
	after cancel $_mevent(afterid)
	unset _mevent(afterid)
    }
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
#         set diff 0
#          catch { set diff [expr $now - $_mevent(time) ] }
#          if {$diff < 175 && $option == "drag" } { # 75ms between motion updates
#              set _mevent(afterid) [after [expr 175 - $diff] [itcl::code $this _rotate drag $x $y]]
#              return
#          }

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
		foreach { vx vy vz } [Euler2XYZ $theta $phi $psi] break
		set a [expr $vx - $view_(vx)]
		set a [expr -$a]
		set b [expr $vy - $view_(vy)]
		set c [expr $vz - $view_(vz)]
		array set view_ [subst {
		    vx $vx
		    vy $vy
		    vz $vz
		}]
		_send "rotate $a $b $c"
		debug "_rotate $x $y: rotate $view_(vx) $view_(vy) $view_(vz)"
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
    set _mevent(time) $now
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
    #@set inner [$itk_component(controls).panel component inner]
    set inner $itk_component(settings_frame)
    $inner.pict configure -image $_settings($this-modelimg)

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
	_send "$option -model $model"
	#$_dispatcher event -idle !rebuild
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
# USAGE: projection on|off|toggle
# USAGE: projection update
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::projection {option} {
    switch -- $option {
	"orthoscopic" {
	    set ortho 1
	}
	"perspective" {
	    set ortho 0
	}
	"toggle" {
	    set ortho [expr {$_settings($this-ortho) == 0}]
	}
	"update" {
	    set ortho $_settings($this-ortho)
	}
	default {
	    error "bad option \"$option\": should be on, off, toggle, or update"
	}
    }
    if { $ortho == $_settings($this-ortho) && $option != "update"} {
	# nothing to do
	return
    }
    if { $ortho } {
	$itk_component(ortho) configure -image [Rappture::icon 3dorth]
	Rappture::Tooltip::for $itk_component(ortho) \
	    "Change to perspective projection"
	set _settings($this-ortho) 1
	_send "orthoscopic on"
    } else {
	$itk_component(ortho) configure -image [Rappture::icon 3dpers]
	Rappture::Tooltip::for $itk_component(ortho) \
	    "Change to orthoscopic projection"
	set _settings($this-ortho) 0
	_send "orthoscopic off"
    }
}

# ----------------------------------------------------------------------
# USAGE: atomscale scale ?model?
#        atomscale update
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::atomscale { option {model "all"} } {
    if { $option == "update" } {
	set scale $_settings($this-atomscale)
    } elseif { [string is double $option] } {
	set scale $option
	if { ($scale < 0.1) || ($scale > 2.0) } {
	    error "bad atom size \"$scale\""
	}
    } else {
	error "bad option \"$option\""
    }
    set _settings($this-atomscale) $scale
    if { [isconnected] } {
	_send "atomscale -model $model $scale"
    }
}

# ----------------------------------------------------------------------
# USAGE: bondthickness scale ?model?
#        bondthickness update
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::bondthickness { option {model "all"} } {
    if { $option == "update" } {
	set scale $_settings($this-bondthickness)
    } elseif { [string is double $option] } {
	set scale $option
	if { ($scale < 0.1) || ($scale > 2.0) } {
	    error "bad bind thickness \"$scale\""
	}
    } else {
	error "bad option \"$option\""
    }
    set _settings($this-bondthickness) $scale
    if { [isconnected] } {
	_send "bondthickness -model $model $scale"
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
	theta   45
	phi     45
	psi     0
	mx      0
	my      0
	mz      0
	x       0
	y       0
	z       0
	zoom    0
	width   0
	height  0
    }
    _send "reset"
    _send "rotate $view_(mx) $view_(my) $view_(mz)"
    debug "ResetView: rotate $view_(mx) $view_(my) $view_(mz)"
    _send "pan $view_(x) $view_(y)"
    _send "zoom $view_(zoom)"
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



itcl::body Rappture::MolvisViewer::WaitIcon  { option widget } {
    switch -- $option {
	"start" {
	    $_dispatcher dispatch $this !waiticon \
		"[itcl::code $this WaitIcon "next" $widget] ; list"
	    set icon_ 0
	    $widget configure -image [Rappture::icon bigroller${icon_}]
	    $_dispatcher event -after 100 !waiticon
	}
	"next" {
	    incr icon_
	    if { $icon_ >= 8 } {
		set icon_ 0
	    }
	    $widget configure -image [Rappture::icon bigroller${icon_}]
	    $_dispatcher event -after 100 !waiticon
	}
	"stop" {
	    $_dispatcher cancel !waiticon
	}
    }
}
	    
itcl::body Rappture::MolvisViewer::GetPngImage  { widget width height } {
    set token "print[incr nextToken_]"
    set var ::Rappture::MolvisViewer::hardcopy_($this-$token)
    set $var ""

    # Setup an automatic timeout procedure.
    $_dispatcher dispatch $this !pngtimeout "set $var {} ; list"

    set popup [Rappture::Balloon .print -title "Generating file..."]
    set inner [$popup component inner]
    label $inner.title -text "Generating Hardcopy" -font "Arial 10 bold"
    label $inner.please -text "This may take a minute." -font "Arial 10"
    label $inner.icon -image [Rappture::icon bigroller0]
    button $inner.cancel -text "Cancel" -font "Arial 10 bold" \
	-command [list set $var ""]
    $_dispatcher event -after 60000 !pngtimeout
    WaitIcon start $inner.icon
    bind $inner.cancel <KeyPress-Return> [list $inner.cancel invoke]
    
    blt::table $inner \
	0,0 $inner.title -columnspan 2 \
	1,0 $inner.please -anchor w \
	1,1 $inner.icon -anchor e  \
	2,0 $inner.cancel -columnspan 2 
    blt::table configure $inner r0 -pady 4 
    blt::table configure $inner r2 -pady 4 
    grab set -local $inner
    focus $inner.cancel

    _send "print $token $width $height"

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
    destroy $popup
    update

    if { $hardcopy_($this-$token) != "" } {
	return [list .png $hardcopy_($this-$token)]
    }
    return ""
}

itcl::body Rappture::MolvisViewer::tab { what who } {
    switch -- ${what} {
	"select" {
	    $itk_component(scroller) contents $itk_component(${who}_canvas)
	    after idle [list focus $itk_component(${who}_canvas)]
	    $itk_component(title) configure -text "$headings_($who)"
	    drawer open
	}
	"deselect" {
	    drawer close 
	}
	"resize" {
	    set bbox [$itk_component(${who}_canvas) bbox all]
	    set wid [winfo width $itk_component(${who}_frame)]
	    $itk_component(${who}_canvas) configure -width $wid \
		-scrollregion $bbox -yscrollincrement 0.1i
	}
    }
}
