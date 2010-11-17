
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

    private variable _icon 0

    private variable _mevent;		# info used for mouse event operations
    private variable _rocker;		# info used for rock operations
    private variable _dlist "";		# list of dataobj objects
    private variable _dataobjs;		# data objects on server
    private variable _dobj2transparency;# maps dataobj => transparency
    private variable _dobj2raise;	# maps dataobj => raise flag 0/1

    private variable _active;		# array of active models.
    private variable _obj2models;	# array containing list of models 
                                        # for each data object.
    private variable _view
    private variable _click

    private variable _model
    private variable _mlist
    private variable _mrep "ballnstick"

    private variable _imagecache
    private variable _state
    private variable _labels  "default"
    private variable _cacheid ""
    private variable _cacheimage ""

    private variable _delta1 10
    private variable _delta2 2

    private common _settings  ;		# Array of settings for all known 
                                        # widgets
    private variable _initialized

    private common _downloadPopup;	# Download options from popup
    private variable _pdbdata;		# PDB data from run file sent to pymol
    private common _hardcopy
    private variable _nextToken 0
    private variable _outbuf "";
    private variable _buffering 0;
    private variable _resizePending 0;
    private variable _width
    private variable _height
    private variable _restore 1;	# Restore camera settings
    private variable _cell 0;		# Restore camera settings

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
    private method BuildSettingsTab {}
    private method DoResize {} 
    private method EventuallyResize { w h } 
    private method GetImage { widget }
    private method ReceiveImage { size cacheid frame rock }
    private method WaitIcon { option widget }
    private method DownloadPopup { popup command } 
    private method EnableDownload { popup what }

    protected method Map {}
    protected method Pan {option x y}
    protected method Rebuild { }
    protected method Rotate {option x y}
    protected method SendCmd { string }
    protected method Unmap {}
    protected method Update { args }
    protected method Vmouse  {option b m x y}
    protected method Vmouse2 {option b m x y}
    protected method Zoom {option {factor 10}}

    public method Connect {}
    public method Disconnect {}
    public method ResetView {} 
    public method add {dataobj {options ""}}
    public method delete {args}
    public method download {option args}
    public method get {}
    public method isconnected {}
    public method labels {option {model "all"}}
    public method parameters {title args} { 
        # do nothing 
    }
    public method snap { w h }
    private method Opacity {option {models "all"} }
    private method SphereScale {option {models "all"} }
    private method StickRadius {option {models "all"} }
    private method OrthoProjection {option}
    private method Representation {option {model "all"} }
    private method CartoonTrace {option {model "all"}}
    private method ComputeParallelepipedVertices { dataobj }
    private method Cell {option}
    private method Rock {option}
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
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Rocker
    $_dispatcher register !rocker
    $_dispatcher dispatch $this !rocker "[itcl::code $this Rock step]; list"
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
    $_parser alias image [itcl::code $this ReceiveImage]

    set _rocker(dir) 1
    set _rocker(client) 0
    set _rocker(server) 0
    set _rocker(on) 0
    set _state(server) 1
    set _state(client) 1
    set _hostlist $hostlist
    set _restore 1

    array set _view {
        theta   45
        phi     45
        psi     0
        vx	0
        vy	0
        vz	0
        zoom	0
        mx	0
        my	0
        mz	0
        x	0
        y	0
        z	0
        width	0
        height  0
    }

    # Setup default settings for widget.
    array set _settings [subst {
        $this-spherescale 0.25
        $this-stickradius 0.14
        $this-cartoontrace no
        $this-model     ballnstick
        $this-modelimg  [Rappture::icon ballnstick]
        $this-opacity   1.0
        $this-ortho     no
        $this-rock      no
        $this-showlabels no
        $this-showcell	yes
        $this-showlabels-initialized no
    }]
    
    itk_component add 3dview {
        label $itk_component(plotarea).vol -image $_image(plot) \
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
            -command [itcl::code $this ResetView]
    } {
        usual
        ignore -highlightthickness
    }
    pack $itk_component(reset) -padx 1 -pady 2
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
    pack $itk_component(zoomin) -padx 2 -pady 2
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
    pack $itk_component(zoomout) -padx 2 -pady 2
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add labels {
        Rappture::PushButton $f.labels \
            -onimage [Rappture::icon molvis-labels-view] \
            -offimage [Rappture::icon molvis-labels-view] \
            -command [itcl::code $this labels update] \
            -variable [itcl::scope _settings($this-showlabels)]
    }
    $itk_component(labels) deselect
    Rappture::Tooltip::for $itk_component(labels) \
        "Show/hide the labels on atoms"
    pack $itk_component(labels) -padx 2 -pady {6 2} 

    itk_component add rock {
        Rappture::PushButton $f.rock \
            -onimage [Rappture::icon molvis-rock-view] \
            -offimage [Rappture::icon molvis-rock-view] \
            -command [itcl::code $this Rock toggle] \
            -variable [itcl::scope _settings($this-rock)]
    }
    pack $itk_component(rock) -padx 2 -pady 2 
    Rappture::Tooltip::for $itk_component(rock) "Rock model back and forth"

    itk_component add ortho {
        label $f.ortho -borderwidth 1 -padx 1 -pady 1 \
            -relief "raised" -image [Rappture::icon molvis-3dpers]
    }
    pack $itk_component(ortho) -padx 2 -pady 2 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(ortho) \
        "Use orthoscopic projection"

    bind $itk_component(ortho) <ButtonPress> \
        [itcl::code $this OrthoProjection toggle]

    BuildSettingsTab

    # HACK ALERT. Initially force a requested width of the 3dview label. 

    # It's a chicken-and-the-egg problem.  The size of the 3dview label is set
    # from the size of the image retrieved from the server.  But the size of
    # the image is specified by the viewport which is the size of the label.
    # The fly-in-the-ointment is that it takes a non-trival amount of time to
    # get the first image back from the server.  In the meantime the idletasks
    # have already kicked in.  We end up with a 1x1 viewport and image.

    # So the idea is to force a ridiculously big requested width on the label
    # (that's why we're using the blt::table to manage the geometry).  It has
    # to be big, because we don't know how big the user may want to stretch
    # the window.  This at least forces the sidebarframe to give the 3dview
    # the maximum size available, which is perfect for an initially closed
    # sidebar.

    blt::table $itk_component(plotarea) \
        0,0 $itk_component(3dview) -fill both -reqwidth 10000
    #
    # RENDERING AREA
    #

    set _image(id) ""

    # set up bindings for rotation
    if 0 {
        bind $itk_component(3dview) <ButtonPress-1> \
            [itcl::code $this Rotate click %x %y]
        bind $itk_component(3dview) <B1-Motion> \
            [itcl::code $this Rotate drag %x %y]
        bind $itk_component(3dview) <ButtonRelease-1> \
            [itcl::code $this Rotate release %x %y]
    } else {
        bind $itk_component(3dview) <ButtonPress-1> \
            [itcl::code $this Vmouse click %b %s %x %y]
        bind $itk_component(3dview) <B1-Motion> \
            [itcl::code $this Vmouse drag 1 %s %x %y]
        bind $itk_component(3dview) <ButtonRelease-1> \
            [itcl::code $this Vmouse release %b %s %x %y]
    }

    bind $itk_component(3dview) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(3dview) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(3dview) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

    bind $itk_component(3dview) <KeyPress-Left> \
        [itcl::code $this Pan set -10 0]
    bind $itk_component(3dview) <KeyPress-Right> \
        [itcl::code $this Pan set 10 0]
    bind $itk_component(3dview) <KeyPress-Up> \
        [itcl::code $this Pan set 0 -10]
    bind $itk_component(3dview) <KeyPress-Down> \
        [itcl::code $this Pan set 0 10]
    bind $itk_component(3dview) <Shift-KeyPress-Left> \
        [itcl::code $this Pan set -50 0]
    bind $itk_component(3dview) <Shift-KeyPress-Right> \
        [itcl::code $this Pan set 50 0]
    bind $itk_component(3dview) <Shift-KeyPress-Up> \
        [itcl::code $this Pan set 0 -50]
    bind $itk_component(3dview) <Shift-KeyPress-Down> \
        [itcl::code $this Pan set 0 50]
    bind $itk_component(3dview) <KeyPress-Prior> \
        [itcl::code $this Zoom out 2]
    bind $itk_component(3dview) <KeyPress-Next> \
        [itcl::code $this Zoom in 2]

    bind $itk_component(3dview) <Enter> "focus $itk_component(3dview)"


    if {[string equal "x11" [tk windowingsystem]]} {
        bind $itk_component(3dview) <4> [itcl::code $this Zoom out 2]
        bind $itk_component(3dview) <5> [itcl::code $this Zoom in 2]
    }

    # set up bindings to bridge mouse events to server
    #bind $itk_component(3dview) <ButtonPress> \
    #   [itcl::code $this Vmouse2 click %b %s %x %y]
    #bind $itk_component(3dview) <ButtonRelease> \
    #    [itcl::code $this Vmouse2 release %b %s %x %y]
    #bind $itk_component(3dview) <B1-Motion> \
    #    [itcl::code $this Vmouse2 drag 1 %s %x %y]
    #bind $itk_component(3dview) <B2-Motion> \
    #    [itcl::code $this Vmouse2 drag 2 %s %x %y]
    #bind $itk_component(3dview) <B3-Motion> \
    #    [itcl::code $this Vmouse2 drag 3 %s %x %y]
    #bind $itk_component(3dview) <Motion> \
    #    [itcl::code $this Vmouse2 move 0 %s %x %y]

    bind $itk_component(3dview) <Configure> \
        [itcl::code $this EventuallyResize %w %h]
    bind $itk_component(3dview) <Unmap> \
        [itcl::code $this Unmap]
    bind $itk_component(3dview) <Map> \
        [itcl::code $this Map]

    eval itk_initialize $args
    Connect
}

itcl::body Rappture::MolvisViewer::BuildSettingsTab {} {
    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "Settings" \
        -icon [Rappture::icon wrench]]
    $inner configure -borderwidth 4

    label $inner.drawinglabel -text "Molecule Representation" \
        -font "Arial 9 bold"

    label $inner.pict -image $_settings($this-modelimg)

    radiobutton $inner.bstick -text "balls and sticks" \
        -command [itcl::code $this Representation ballnstick all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value ballnstick -font "Arial 9" -pady 0 
    Rappture::Tooltip::for $inner.bstick \
        "Display atoms (balls) and connections (sticks) "

    radiobutton $inner.spheres -text "spheres" \
        -command [itcl::code $this Representation spheres all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value spheres -font "Arial 9" -pady 0
    Rappture::Tooltip::for $inner.spheres \
        "Display atoms as spheres. Do not display bonds."

    radiobutton $inner.sticks -text "sticks" \
        -command [itcl::code $this Representation sticks all] \
        -variable Rappture::MolvisViewer::_settings($this-model) \
        -value sticks -font "Arial 9" -pady 0
    Rappture::Tooltip::for $inner.sticks \
        "Display bonds as sticks. Do not display atoms."

    radiobutton $inner.lines -text "lines" \
        -command [itcl::code $this Representation lines all] \
        -variable [itcl::scope _settings($this-model)] \
        -value lines -font "Arial 9" -pady 0
    Rappture::Tooltip::for $inner.lines \
        "Display bonds as lines. Do not display atoms."

    radiobutton $inner.cartoon -text "cartoon" \
        -command [itcl::code $this Representation cartoon all] \
        -variable [itcl::scope _settings($this-model)] \
        -value cartoon -font "Arial 9" -pady 0
    Rappture::Tooltip::for $inner.cartoon \
        "Display cartoon representation of bonds (sticks)."

    scale $inner.spherescale -width 10 -font "Arial 9 bold" \
        -from 0.1 -to 2.0 -resolution 0.05 -label "Sphere Scale" \
        -showvalue true -orient horizontal \
        -command [itcl::code $this SphereScale] \
        -variable Rappture::MolvisViewer::_settings($this-spherescale)
    $inner.spherescale set $_settings($this-spherescale)
    Rappture::Tooltip::for $inner.spherescale \
        "Adjust scale of atoms (spheres or balls). 1.0 is the full VDW radius."

    scale $inner.stickradius -width 10 -font "Arial 9 bold" \
        -from 0.1 -to 1.0 -resolution 0.025 -label "Stick Radius" \
        -showvalue true -orient horizontal \
        -command [itcl::code $this StickRadius] \
        -variable Rappture::MolvisViewer::_settings($this-stickradius)
    Rappture::Tooltip::for $inner.stickradius \
        "Adjust scale of bonds (sticks)."
    $inner.stickradius set $_settings($this-stickradius)

    checkbutton $inner.labels -text "Show labels on atoms" \
        -command [itcl::code $this labels update] \
        -variable [itcl::scope _settings($this-showlabels)] \
        -font "Arial 9 bold"
    Rappture::Tooltip::for $inner.labels \
        "Display atom symbol and serial number."

    checkbutton $inner.rock -text "Rock model back and forth" \
        -command [itcl::code $this Rock toggle] \
        -variable Rappture::MolvisViewer::_settings($this-rock) \
        -font "Arial 9 bold"
    Rappture::Tooltip::for $inner.rock \
        "Rotate the object back and forth around the y-axis."

    checkbutton $inner.ortho -text "Orthoscopic projection" \
        -command [itcl::code $this OrthoProjection update] \
        -variable Rappture::MolvisViewer::_settings($this-ortho) \
         -font "Arial 9 bold"
    Rappture::Tooltip::for $inner.ortho \
        "Toggle between orthoscopic/perspective projection modes."

    checkbutton $inner.cartoontrace -text "Cartoon Trace" \
        -command [itcl::code $this CartoonTrace update] \
        -variable [itcl::scope _settings($this-cartoontrace)] \
        -font "Arial 9 bold"
    Rappture::Tooltip::for $inner.cartoontrace \
        "Set cartoon representation of bonds (sticks)."

    checkbutton $inner.cell -text "Parallelepiped" \
	-command [itcl::code $this Cell toggle] \
        -font "Arial 9 bold"
    $inner.cell select

    label $inner.spacer
    blt::table $inner \
        0,0 $inner.drawinglabel -anchor w -columnspan 4 \
        1,1 $inner.pict -anchor w -rowspan 5 \
        1,2 $inner.bstick -anchor w -columnspan 2 \
        2,2 $inner.spheres -anchor w -columnspan 2 \
        3,2 $inner.sticks -anchor w -columnspan 2 \
        4,2 $inner.lines -anchor w -columnspan 2 \
        5,2 $inner.cartoon -anchor w -columnspan 2 \
        6,0 $inner.labels -anchor w -columnspan 4 -pady {1 0} \
        7,0 $inner.rock -anchor w -columnspan 4 -pady {1 0} \
        8,0 $inner.ortho -anchor w -columnspan 4 -pady {1 0} \
        9,0 $inner.cartoontrace -anchor w -columnspan 4 -pady {1 0} \
	10,0 $inner.cell -anchor w -columnspan 4 -pady {1 0} \
        11,1 $inner.spherescale -fill x -columnspan 4 -pady {1 0} \
        11,1 $inner.stickradius -fill x -columnspan 4 -pady {1 0} \

    blt::table configure $inner c0 -resize expand -width 2
    blt::table configure $inner c1 c2 -resize none
    blt::table configure $inner c3 -resize expand
    blt::table configure $inner r* -resize none
    blt::table configure $inner r13 -resize expand
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
            if { ![winfo exists .molvisviewerdownload] } {
                set inner [DownloadPopup $popup [lindex $args 0]]
            } else {
                set inner [$popup component inner]
            }
            set _downloadPopup(image_controls) $inner.image_frame
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            set word [Rappture::filexfer::label downloadWord]
            $inner.summary configure -text "$word $num in the following format:"
            update idletasks ;		# Fix initial sizes
            return $popup
        }
        now {

            set popup .molvisviewerdownload
            if {[winfo exists .molvisviewerdownload]} {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                "image" {
                    return [$this GetImage [lindex $args 0]]
                }
                "pdb" {
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
    global readyForNextFrame
    set readyForNextFrame 1
    if { [isconnected] } {
        return 1
    }
    set hosts [GetServerList "pymol"]
    if { "" == $hosts } {
        return 0
    }
    set _restore 1 
    set result [VisViewer::Connect $hosts]
    if { $result } {
        $_dispatcher event -idle !rebuild
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
    global readyForNextFrame
    set readyForNextFrame 1
}

itcl::body Rappture::MolvisViewer::SendCmd { cmd } {
    debug "in SendCmd ($cmd)\n"

    if { $_buffering } {
        # Just buffer the commands. Don't send them yet.
        if { $_state(server) != $_state(client) } {
            append _outbuf "frame -defer $_state(client)\n"
            set _state(server) $_state(client)
        }
        if { $_rocker(server) != $_rocker(client) } {
            append _outbuf "rock -defer $_rocker(client)\n"
            set _rocker(server) $_rocker(client)
        }
        append _outbuf "$cmd\n"
    } else {
        if { $_state(server) != $_state(client) } {
            if { ![SendBytes "frame -defer $_state(client)\n"] } {
                set _state(server) $_state(client)
            }
        }
        if { $_rocker(server) != $_rocker(client) } {
            if { ![SendBytes "rock -defer $_rocker(client)\n"] } {
                set _rocker(server) $_rocker(client)
            }
        }
        SendBytes "$cmd\n"
    }
}

#
# ReceiveImage -bytes <size>
#
#     Invoked automatically whenever the "image" command comes in from
#     the rendering server.  Indicates that binary image data with the
#     specified <size> will follow.
#
set count 0
itcl::body Rappture::MolvisViewer::ReceiveImage { size cacheid frame rock } {
    global readyForNextFrame
    set readyForNextFrame 1
    set tag "$frame,$rock"
    global count
    incr count
    if { $cacheid != $_cacheid } {
        array unset _imagecache 
        set _cacheid $cacheid
    }
    #debug "reading $size bytes from proxy\n"
    set data [ReceiveBytes $size]
    #debug "success: reading $size bytes from proxy\n"
    if { $cacheid == "print" } {
        # $frame is the token that we sent to the proxy.
        set _hardcopy($this-$frame) $data
    } else {
        set _imagecache($tag) $data
        #debug "CACHED: $tag,$cacheid"
        $_image(plot) configure -data $data
        set _image(id) $tag
    }
}


# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::Rebuild {} {
    debug "in rebuild"
    set changed 0

    # Turn on buffering of commands to the server.  We don't want to
    # be preempted by a server disconnect/reconnect (that automatically
    # generates a new call to Rebuild).   
    #blt::bltdebug 100
    set _buffering 1
    set _cell 0

    if { $_restore } {
        set _rocker(server) 0
        set _cacheid 0
        SendCmd "raw -defer {set auto_color,0}"
        SendCmd "raw -defer {set auto_show_lines,0}"
    }
    set dlist [get]
    foreach dataobj $dlist {
        set model [$dataobj get components.molecule.model]
        if {"" == $model } {
            set model "molecule"
            scan $dataobj "::libraryObj%d" suffix
            set model $model$suffix
        }
        lappend _obj2models($dataobj) $model 
        set state [$dataobj get components.molecule.state]
        if {"" == $state} { 
            set state $_state(server) 
        }
        if { ![info exists _mlist($model)] } {  # new, turn on
            set _mlist($model) 2
        } elseif { $_mlist($model) == 1 } {	# on, leave on
            set _mlist($model) 3 
        } elseif { $_mlist($model) == 0 } {	# off, turn on
            set _mlist($model) 2
        }
        if { ![info exists _dataobjs($model-$state)] } {
            set data1      ""
            set serial    1

            foreach _atom [$dataobj children -type atom components.molecule] {
                set symbol [$dataobj get components.molecule.$_atom.symbol]
                set xyz [$dataobj get components.molecule.$_atom.xyz]
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
            if {"" != $data1} {
                # Save the PDB data in case the user wants to later save it.
                set _pdbdata $data1
                set nBytes [string length $data1]

                # We know we're buffered here, so append the "loadpdb" command
                # with the data payload immediately afterwards.
                append _outbuf "loadpdb -defer follows $model $state $nBytes\n"
                append _outbuf $data1
                set _dataobjs($model-$state)  1
            }
            # note that pdb files always overwrite xyz files
            set data2 [$dataobj get components.molecule.pdb]
            if {"" != $data2} {
                # Save the PDB data in case the user wants to later save it.
                set _pdbdata $data2
                set nBytes [string length $data2]

                # We know we're buffered here, so append the "loadpdb" command
                # with the data payload immediately afterwards.
                append _outbuf "loadpdb -defer follows $model $state $nBytes\n"
                append _outbuf $data2
                set _dataobjs($model-$state)  1
            }
            # lammps dump file overwrites pdb file (change this?)
            set lammpstypemap [$dataobj get components.molecule.lammpstypemap]
            set lammpsdata [$dataobj get components.molecule.lammps]
            if {"" != $lammpsdata} {
                set data3 ""
                set modelcount 0
                foreach lammpsline [split $lammpsdata "\n"] {
                    if {[scan $lammpsline "%d %d %f %f %f" id type x y z] == 5} {
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
                        if { "" == $lammpstypemap} {
                            set atom $type 
                        } else {
                            set atom [lindex $lammpstypemap [expr {$type - 1}]]
                            if { "" == $atom} {
                              set atom $type
                            }
                        }
                        set pdbline [format "%6s%5d %4s%1s%3s %1s%5s   %8.3f%8.3f%8.3f%6.2f%6.2f%8s\n" $recname $id $atom $altLoc $resName $chainID $Seqno $x $y $z $occupancy $tempFactor $recID]
                        append data3 $pdbline
                    }
                    # only read first model 
                    if {[regexp "^ITEM: ATOMS" $lammpsline]} {
                      incr modelcount
                      if {$modelcount > 1} {
                        break
                      }
                    }
                }
                if {"" != $data3} {
                    # Save the PDB data in case the user wants to later save it.
                    set _pdbdata $data3
                    set nBytes [string length $data3]

                    # We know we're buffered here, so append the "loadpdb" 
                    # command with the data payload immediately afterwards.
                    append _outbuf \
                        "loadpdb -defer follows $model $state $nBytes\n"
                    append _outbuf $data3
                }
                set _dataobjs($model-$state) 1
	    }
        }
        if { ![info exists _model($model-transparency)] } {
            set _model($model-transparency) ""
        }
        if { ![info exists _model($model-rep)] } {
            set _model($model-rep) ""
            set _model($model-newrep) $_mrep
        }
        if { $_model($model-transparency) != $_dobj2transparency($dataobj) } {
            set _model($model-newtransparency) $_dobj2transparency($dataobj)
        }
        if { $_dobj2transparency($dataobj) == "ghost"} {
            array unset _active $model
        } else {
            set _active($model) $dataobj
        }
        set vector [$dataobj get components.parallelepiped.vector]
	if { $vector != "" } {
	    set vertices [ComputeParallelepipedVertices $dataobj]
	    SendCmd "raw -defer {verts = \[$vertices\]\n}"
	    SendCmd "raw -defer {run \$PYMOL_PATH/rappture/box.py\n}"
	    SendCmd "raw -defer {draw_box(verts)\n}"
	    set _cell 1
	}
    }
	
    # enable/disable models as required (0=off->off, 1=on->off, 2=off->on,
    # 3=on->on)

    foreach model [array names _mlist] {
        if { $_mlist($model) == 1 } {
            SendCmd "disable -defer $model"
            set _mlist($model) 0
            set changed 1
        } elseif { $_mlist($model) == 2 } {
            set _mlist($model) 1
            SendCmd "enable -defer $model"
            set changed 1
        } elseif { $_mlist($model) == 3 } {
            set _mlist($model) 1
        }
        if { $_mlist($model) == 1 } {
            if {  [info exists _model($model-newtransparency)] || 
                  [info exists _model($model-newrep)] } {
                if { ![info exists _model($model-newrep)] } {
                    set _model($model-newrep) $_model($model-rep)
                }
                if { ![info exists _model($model-newtransparency)] } {
                    set _model($model-newtransparency) $_model($model-transparency)
                }
                set rep $_model($model-newrep)
                set transp $_model($model-newtransparency)
                SendCmd "representation -defer -model $model $rep"
                if { $_model($model-newtransparency) == "ghost" } {
                    SendCmd "deactivate -defer -model $model"
                } else {
                    SendCmd "activate -defer -model $model"
                }
                set changed 1
                set _model($model-transparency) $_model($model-newtransparency)
                set _model($model-rep) $_model($model-newrep)
                catch {
                    unset _model($model-newtransparency)
                    unset _model($model-newrep)
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
        SendCmd "frame 1"
        set flush 1
    } elseif { ![info exists _imagecache($state,$_rocker(client))] } {
        set _state(server) $state
        set _state(client) $state
        SendCmd "frame $state"
        set flush 1
    } else {
        set _state(client) $state
        Update
        set flush 0
    }
    if { $_restore } {
        # Set or restore viewing parameters.  We do this for the first
        # model and assume this works for everything else.
        set w  [winfo width $itk_component(3dview)] 
        set h  [winfo height $itk_component(3dview)] 
        SendCmd [subst { 
            reset
            screen $w $h
            rotate $_view(mx) $_view(my) $_view(mz)
            pan $_view(x) $_view(y)
            zoom $_view(zoom)
        }]
        debug "rebuild: rotate $_view(mx) $_view(my) $_view(mz)"

        # Default settings for all models.
        SphereScale update 
        StickRadius update
        labels update 
        Opacity update 
        CartoonTrace update 
        Cell update
        OrthoProjection update 
        Representation update 
        SendCmd "raw -defer {zoom complete=1}"
        set _restore 0
    }
    set inner [$itk_component(main) panel "Settings"]
    if { $_cell } {
	$inner.cell configure -state normal
    } else {
	$inner.cell configure -state disabled
    }
    if { $flush } {
        global readyForNextFrame
        set readyForNextFrame 0;	# Don't advance to the next frame
                                        # until we get an image.
        SendCmd "bmp";			# Flush the results.
    }
    set _buffering 0;			# Turn off buffering.

    blt::busy hold $itk_component(hull)

    # Actually write the commands to the server socket.  
    # If it fails, we don't care.  We're finished here.
    SendBytes $_outbuf;			
    set _outbuf "";			# Clear the buffer.		
    blt::busy release $itk_component(hull)

    debug "exiting rebuild"
}

itcl::body Rappture::MolvisViewer::Unmap { } {
    # Pause rocking loop while unmapped (saves CPU time)
    Rock pause

    # Blank image, mark current image dirty
    # This will force reload from cache, or remain blank if cache is cleared
    # This prevents old image from briefly appearing when a new result is added
    # by result viewer

    #$_image(plot) blank
    set _image(id) ""
}

itcl::body Rappture::MolvisViewer::Map { } {
    if { [isconnected] } {
        # Resume rocking loop if it was on
        Rock unpause
        # Rebuild image if modified, or redisplay cached image if not
        $_dispatcher event -idle !rebuild
    }
}

itcl::body Rappture::MolvisViewer::DoResize { } {
    SendCmd "screen $_width $_height"
    $_image(plot) configure -width $_width -height $_height
    # Immediately invalidate cache, defer update until mapped
    array unset _imagecache 
    set _resizePending 0
}
    
itcl::body Rappture::MolvisViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    if { !$_resizePending } {
        $_dispatcher event -idle !resize
        set _resizePending 1
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
itcl::body Rappture::MolvisViewer::Pan {option x y} {
    if { $option == "set" } {
        set dx $x
        set dy $y
        set _view(x) [expr $_view(x) + $dx]
        set _view(y) [expr $_view(y) + $dy]
        SendCmd "pan $dx $dy"
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
        set _view(x) [expr $_view(x) + $dx]
        set _view(y) [expr $_view(y) + $dy]
        SendCmd "pan $dx $dy"
    }
    set _mevent(x) $x
    set _mevent(y) $y
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
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
itcl::body Rappture::MolvisViewer::Zoom {option {factor 10}} {
    switch -- $option {
        "in" {
            set _view(zoom) [expr $_view(zoom) + $factor]
            SendCmd "zoom $factor"
        }
        "out" {
            set _view(zoom) [expr $_view(zoom) - $factor]
            SendCmd "zoom -$factor"
        }
        "reset" {
            set _view(zoom) 0
            SendCmd "reset"
        }
    }
}

itcl::body Rappture::MolvisViewer::Update { args } {
    set tag "$_state(client),$_rocker(client)"
    if { $_image(id) != "$tag" } {
        if { [info exists _imagecache($tag)] } {
            $_image(plot) configure -data $_imagecache($tag)
            set _image(id) "$tag"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rock on|off|toggle
# USAGE: Rock pause|unpause|step
#
# Used to control the "rocking" model for the molecule being displayed.
# Clients should use only the on/off/toggle options; the rest are for
# internal control of the rocking motion.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::Rock { option } {
    # cancel any pending rocks
    if { [info exists _rocker(afterid)] } {
        after cancel $_rocker(afterid)
        unset _rocker(afterid)
    }
    if { ![winfo viewable $itk_component(3dview)] } {
        return
    }
    set _rocker(on) $_settings($this-rock) 
    if { $option == "step"} {
        if { $_rocker(client) >= 10 } {
            set _rocker(dir) -1
        } elseif { $_rocker(client) <= -10 } {
            set _rocker(dir) 1
        }
        set _rocker(client) [expr {$_rocker(client) + $_rocker(dir)}]
        if { ![info exists _imagecache($_state(server),$_rocker(client))] } {
            set _rocker(server) $_rocker(client)
            SendCmd "rock $_rocker(client)"
        }
        Update
    }
    if { $_rocker(on) && $option != "pause" } {
         set _rocker(afterid) [after 200 [itcl::code $this Rock step]]
    }
}


itcl::body Rappture::MolvisViewer::Vmouse2 {option b m x y} {
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
    SendCmd "vmouse $vButton $vModifier $vState $x $y"
    set _mevent(time) $now
}

itcl::body Rappture::MolvisViewer::Vmouse {option b m x y} {
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
             set _mevent(afterid) [after [expr 25 - $diff] [itcl::code $this Vmouse drag $b $m $x $y]]
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
        set _view(mx) [expr {$_view(mx) + $mx}]
        set _view(my) [expr {$_view(my) + $my}]
        set _view(mz) [expr {$_view(mz) + $mz}]
        SendCmd "rotate $mx $my $mz"
        debug "_vmmouse: rotate $_view(mx) $_view(my) $_view(mz)"
    }
    set _mevent(x) $x
    set _mevent(y) $y
    set _mevent(time) $now
    if { $option == "release" } {
        $itk_component(3dview) configure -cursor ""
    }
}

# ----------------------------------------------------------------------
# USAGE: Rotate click <x> <y>
# USAGE: Rotate drag <x> <y>
# USAGE: Rotate release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::Rotate {option x y} {
    set now  [clock clicks -milliseconds]
    #update idletasks
    # cancel any pending delayed dragging events
    if { [info exists _mevent(afterid)] } {
        after cancel $_mevent(afterid)
        unset _mevent(afterid)
    }
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
#         set diff 0
#          catch { set diff [expr $now - $_mevent(time) ] }
#          if {$diff < 175 && $option == "drag" } { # 75ms between motion updates
#              set _mevent(afterid) [after [expr 175 - $diff] [itcl::code $this Rotate drag $x $y]]
#              return
#          }

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
                foreach { vx vy vz } [Euler2XYZ $theta $phi $psi] break
                set a [expr $vx - $_view(vx)]
                set a [expr -$a]
                set b [expr $vy - $_view(vy)]
                set c [expr $vz - $_view(vz)]
                array set _view [subst {
                    vx $vx
                    vy $vy
                    vz $vz
                }]
                SendCmd "rotate $a $b $c"
                debug "Rotate $x $y: rotate $_view(vx) $_view(vy) $_view(vz)"
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
    set _mevent(time) $now
}

# ----------------------------------------------------------------------
# USAGE: Representation spheres|ballnstick|lines|sticks
#
# Used internally to change the molecular representation used to render
# our scene.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::Representation {option {model "all"} } {
    if { $option == $_mrep } {
        return 
    }
    if { $option == "update" } {
        set option $_settings($this-model)
    }
    if { $option == "sticks" } {
        set _settings($this-modelimg) [Rappture::icon lines]
    }  else {
        set _settings($this-modelimg) [Rappture::icon $option]
    }
    set inner [$itk_component(main) panel "Settings"]
    $inner.pict configure -image $_settings($this-modelimg)

    # Save the current option to set all radiobuttons -- just in case.
    # This method gets called without the user clicking on a radiobutton.
    set _settings($this-model) $option
    set _mrep $option

    if { $model == "all" } {
        set models [array names _mlist]
    } else {
        set models $model
    }

    foreach model $models {
        if { [info exists _model($model-rep)] } {
            if { $_model($model-rep) != $option } {
                set _model($model-newrep) $option
            } else {
                catch { unset _model($model-newrep) }
            }
        }
    }
    if { [isconnected] } {
        SendCmd "representation -model $model $option"
        #$_dispatcher event -idle !rebuild
    }
}


# ----------------------------------------------------------------------
# USAGE: OrthoProjection on|off|toggle
# USAGE: OrthoProjection update
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::OrthoProjection {option} {
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
        $itk_component(ortho) configure -image [Rappture::icon molvis-3dorth]
        Rappture::Tooltip::for $itk_component(ortho) \
            "Use perspective projection"
        set _settings($this-ortho) 1
        SendCmd "orthoscopic on"
    } else {
        $itk_component(ortho) configure -image [Rappture::icon molvis-3dpers]
        Rappture::Tooltip::for $itk_component(ortho) \
            "Use orthoscopic projection"
        set _settings($this-ortho) 0
        SendCmd "orthoscopic off"
    }
}

# ----------------------------------------------------------------------
# USAGE: Cell on|off|toggle
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::Cell {option} {
    switch -- $option {
        "on" - "off" {
            set cell $option
	}
        "toggle" {
            set cell [expr {$_settings($this-showcell) == 0}]
        }
        "update" {
            set cell $_settings($this-showcell)
        }
        default {
            error "bad option \"$option\": should be on, off, toggle, or update"
        }
    }
    if { $cell == $_settings($this-showcell) && $option != "update"} {
        # nothing to do
        return
    }
    if { $cell } {
        Rappture::Tooltip::for $itk_component(ortho) \
            "Hide the cell."
        set _settings($this-showcell) 1
        SendCmd "raw {show everything,unitcell}"
    } else {
        Rappture::Tooltip::for $itk_component(ortho) \
            "Show the cell."
        set _settings($this-showcell) 0
        SendCmd "raw {hide everything,unitcell}"
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

        if { !$_settings($this-showlabels-initialized) } {
            set showlabels [$dataobj get components.molecule.about.emblems]
            if { $showlabels != "" && [string is boolean $showlabels] } {
                set _settings($this-showlabels) $showlabels
            } 
        }

        lappend _dlist $dataobj
        if { $params(-brightness) >= 0.5 } {
            set _dobj2transparency($dataobj) "ghost"
        } else {
            set _dobj2transparency($dataobj) "normal"
        }
        set _dobj2raise($dataobj) $params(-raise)
        debug "setting parameters for $dataobj\n"

        if { [isconnected] } {
            $_dispatcher event -idle !rebuild
        }
    }
}

#
# ResetView
#
itcl::body Rappture::MolvisViewer::ResetView {} {
    array set _view {
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
    SendCmd "reset"
    SendCmd "rotate $_view(mx) $_view(my) $_view(mz)"
    debug "ResetView: rotate $_view(mx) $_view(my) $_view(mz)"
    SendCmd "pan $_view(x) $_view(y)"
    SendCmd "zoom $_view(zoom)"
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
itcl::body Rappture::MolvisViewer::delete { args } {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            if { [info exists _obj2models($dataobj)] } {
                foreach model $_obj2models($dataobj) {
                    array unset _active $model
                }
            }
            array unset _obj2models $dataobj
            array unset _dobj2transparency $dataobj
            array unset _dobj2color $dataobj
            array unset _dobj2width $dataobj
            array unset _dobj2dashes $dataobj
            array unset _dobj2raise $dataobj
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
            
itcl::body Rappture::MolvisViewer::GetImage { widget } {
    set token "print[incr _nextToken]"
    set var ::Rappture::MolvisViewer::_hardcopy($this-$token)
    set $var ""

    set controls $_downloadPopup(image_controls) 
    set combo $controls.size_combo
    set size [$combo translate [$combo value]]
    switch -- $size {
        "standard" {
            set width 1200
            set height 1200
        }
        "highquality" {
            set width 2400
            set height 2400
        }
        "draft" {
            set width 400
            set height 400
        }
        default {
            error "unknown image size [$inner.image_size_combo value]"
        }
    }
    # Setup an automatic timeout procedure.
    $_dispatcher dispatch $this !pngtimeout "set $var {} ; list"
    
    set popup .molvisviewerprint
    if { ![winfo exists $popup] } {
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
    set combo $controls.bgcolor_combo
    set bgcolor [$combo translate [$combo value]]
    
    $_dispatcher event -after 60000 !pngtimeout
    WaitIcon start $inner.icon
    grab set -local $inner
    focus $inner.cancel
    
    SendCmd "print $token $width $height $bgcolor"

    $popup activate $widget below
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
        set combo $controls.type_combo
        set type [$combo translate [$combo value]]
        switch -- $type {
            "jpg" {
                set img [image create photo -data $_hardcopy($this-$token)]
                set bytes [$img data -format "jpeg -quality 100"]
                set bytes [Rappture::encoding::decode -as b64 $bytes]
                return [list .jpg $bytes]
            }
            "gif" {
                set img [image create photo -data $_hardcopy($this-$token)]
                set bytes [$img data -format "gif"]
                set bytes [Rappture::encoding::decode -as b64 $bytes]
                return [list .gif $bytes]
            }
            "png" {
                return [list .png $_hardcopy($this-$token)]
            }
        }
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: SphereScale radius ?model?
#        SphereScale update ?model?
#
# Used internally to change the molecular atom scale used to render
# our scene.  
#
# Note: Only sets the specified radius for active models.  If the model 
#       is inactive, then it overridden with the value "0.1".
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::SphereScale { option {models "all"} } {
    if { $option == "update" } {
        set radius $_settings($this-spherescale)
    } elseif { [string is double $option] } {
        set radius $option
        if { ($radius < 0.1) || ($radius > 2.0) } {
            error "bad atom size \"$radius\""
        }
    } else {
        error "bad option \"$option\""
    }
    set _settings($this-spherescale) $radius
    if { $models == "all" } {
        SendCmd "spherescale -model all $radius"
        return
    }
    set overrideradius [expr $radius * 0.8]
    SendCmd "spherescale -model all $overrideradius"
    foreach model $models {
        if { [info exists _active($model)] } {
            SendCmd "spherescale -model $model $radius"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: StickRadius radius ?models?
#	 StickRadius update ?models?
#
# Used internally to change the stick radius used to render
# our scene.
#
# Note: Only sets the specified radius for active models.  If the model 
#       is inactive, then it overridden with the value "0.25".
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::StickRadius { option {models "all"} } {
    if { $option == "update" } {
        set radius $_settings($this-stickradius)
    } elseif { [string is double $option] } {
        set radius $option
        if { ($radius < 0.1) || ($radius > 2.0) } {
            error "bad stick radius \"$radius\""
        }
    } else {
        error "bad option \"$option\""
    }
    set _settings($this-stickradius) $radius
    if { $models == "all" } {
        SendCmd "stickradius -model all $radius"
        return
    }
    set overrideradius [expr $radius * 0.8]
    SendCmd "stickradius -model all $overrideradius"
    foreach model $models {
        if { [info exists _active($model)] } {
            SendCmd "stickradius -model $model $radius"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Opacity value ?models?
#	 Opacity update ?models?
#
# Used internally to change the opacity (transparency) used to render
# our scene.
#
# Note: Only sets the specified transparency for active models.  If the model 
#       is inactive, then it overridden with the value "0.75".
# ----------------------------------------------------------------------

itcl::body Rappture::MolvisViewer::Opacity { option {models "all"} } {
    if { $option == "update" } {
        set opacity $_settings($this-opacity)
    } elseif { [string is double $option] } {
        set opacity $option
        if { ($opacity < 0.0) || ($opacity > 1.0) } {
            error "bad opacity \"$opacity\""
        }
    } else {
        error "bad option \"$option\""
    }
    set _settings($this-opacity) $opacity
    set transparency [expr 1.0 - $opacity]
    if { $models == "all" } {
        SendCmd "transparency -model all $transparency"
        return
    }
    set overridetransparency 0.60
    SendCmd "transparency -model all $overridetransparency"
    foreach model $models {
        if { [info exists _active($model)] } {
            SendCmd "transparency -model $model $transparency"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: labels on|off|toggle
# USAGE: labels update
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::labels {option {models "all"}} {
    set showlabels $_settings($this-showlabels)
    if { $option == "update" } {
        set showlabels $_settings($this-showlabels)
    } elseif { [string is boolean $option] } {
        set showlabels $option
    } else {
        error "bad option \"$option\""
    }
    set _settings($this-showlabels) $showlabels
    if { $models == "all" } {
        SendCmd "label -model all $showlabels"
        return
    }
    SendCmd "label -model all off"
    if { $showlabels } {
        foreach model $models {
            if { [info exists _active($model)] } {
                SendCmd "label -model $model $showlabels"
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: CartoonTrace on|off|toggle
# USAGE: CartoonTrace update
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MolvisViewer::CartoonTrace {option {models "all"}} {
    set trace $_settings($this-cartoontrace)
    if { $option == "update" } {
        set trace $_settings($this-cartoontrace)
    } elseif { [string is boolean $option] } {
        set trace $option
    } else {
        error "bad option \"$option\""
    }
    set _settings($this-cartoontrace) $trace
    if { $models == "all" } {
        SendCmd "cartoontrace -model all $trace"
        return
    }
    SendCmd "cartoontrace -model all off"
    if { $trace } {
        foreach model $models {
            if { [info exists _active($model)] } {
                SendCmd "cartoontrace -model $model $trace"
            }
        }
    }
}

itcl::body Rappture::MolvisViewer::DownloadPopup { popup command } {
    Rappture::Balloon $popup \
        -title "[Rappture::filexfer::label downloadWord] as..."
    set inner [$popup component inner]
    label $inner.summary -text "" -anchor w -font "Arial 11 bold"
    radiobutton $inner.pdb_button -text "PDB Protein Data Bank Format File" \
        -variable [itcl::scope _downloadPopup(format)] \
        -command [itcl::code $this EnableDownload $popup pdb] \
        -font "Arial 10 " \
        -value pdb  
    Rappture::Tooltip::for $inner.pdb_button \
        "Save as PDB Protein Data Bank format file."
    radiobutton $inner.image_button -text "Image File" \
        -variable [itcl::scope _downloadPopup(format)] \
        -command [itcl::code $this EnableDownload $popup image] \
        -font "Arial 10 " \
        -value image 
    Rappture::Tooltip::for $inner.image_button \
        "Save as digital image."

    set controls [frame $inner.image_frame -bd 2 -relief groove]
    label $controls.size_label -text "Size:" \
        -font "Arial 9" 
    set img $_image(plot)
    set res "[image width $img]x[image height $img]"
    Rappture::Combobox $controls.size_combo -width 20 -editable no
    $controls.size_combo choices insert end \
        "draft"  "Draft (400x400)"         \
        "standard"  "Standard (1200x1200)"          \
        "highquality"  "High Quality (2400x2400)"

    label $controls.bgcolor_label -text "Background:" \
        -font "Arial 9" 
    Rappture::Combobox $controls.bgcolor_combo -width 20 -editable no
    $controls.bgcolor_combo choices insert end \
        "black"  "Black" \
        "white"  "White" \
        "none"  "Transparent (PNG only)"         

    label $controls.type_label -text "Type:" \
        -font "Arial 9" 
    Rappture::Combobox $controls.type_combo -width 20 -editable no
    $controls.type_combo choices insert end \
        "jpg"  "JPEG Joint Photographic Experts Group Format (*.jpg)" \
        "png"  "PNG Portable Network Graphics Format (*.png)"         

    button $inner.go -text [Rappture::filexfer::label download] \
        -command $command

    blt::table $controls \
        1,0 $controls.size_label -anchor e \
        1,1 $controls.size_combo -anchor w -fill x \
        2,0 $controls.bgcolor_label -anchor e \
        2,1 $controls.bgcolor_combo -anchor w -fill x \
        3,0 $controls.type_label -anchor e \
        3,1 $controls.type_combo -anchor w -fill x  
    blt::table configure $controls r0 -height 16 
    blt::table configure $controls -padx 4 -pady {0 6}
    blt::table $inner \
        0,0 $inner.summary -cspan 2 \
        1,0 $inner.pdb_button -cspan 2 -anchor w \
        2,0 $inner.image_button -cspan 2 -rspan 2 -anchor nw -ipadx 2 -ipady 2 \
        3,1 $controls -fill both \
        6,0 $inner.go -cspan 2 -pady 5
    blt::table configure $inner c0 -width 11
    blt::table configure $inner r2 -height 11
    #blt::table configure $inner c1 -width 8
    raise $inner.image_button
    $inner.pdb_button invoke
    $controls.bgcolor_combo value "Black"
    $controls.size_combo value "Draft (400x400)"
    $controls.type_combo value  "PNG Portable Network Graphics Format (*.png)" 
    return $inner
}

itcl::body Rappture::MolvisViewer::EnableDownload { popup what } {
    set inner [$popup component inner]
    switch -- $what {
        "pdb" {
            foreach w [winfo children $inner.image_frame] {
                $w configure -state disabled
            }
        }
        "image" {
            foreach w [winfo children $inner.image_frame] {
                $w configure -state normal
            }
        }
        default {
            error "unknown type of download"
        }
    }
}

itcl::body Rappture::MolvisViewer::snap { w h } {
    if { $w <= 0 || $h <= 0 } {
        set w [image width $_image(plot)]
        set h [image height $_image(plot)]
    } 
    set tag "$_state(client),$_rocker(client)"
    if { $_image(id) != "$tag" } {
        while { ![info exists _imagecache($tag)] } {
            update idletasks
            update
            after 100
        }
        if { [info exists _imagecache($tag)] } {
            $_image(plot) configure -data $_imagecache($tag)
            set _image(id) "$tag"
        }
    }
    set img [image create picture -width $w -height $h]
    $img resample $_image(plot)
    return $img
}

# FIXME: Handle 2D vectors
itcl::body Rappture::MolvisViewer::ComputeParallelepipedVertices { dataobj } {
    # Create a vector for every 3D point
    blt::vector point0(3) point1(3) point2(3) point3(3) point4(3) point5(3) \
	point6(3) point7(3) origin(3) scale(3)

    set count 0
    set parent [$dataobj element -as object "components.parallelepiped"]
    foreach child [$parent children] {
	if { ![string match "vector*" $child] } {
	    continue
	}
	incr count
	set vector  [$parent get $child]
	regexp -all {,} $vector {} vector
	point$count set $vector
    }
    itcl::delete object $parent
    if { $count < 1 || $count > 3 } {
	error "bad number of vectors supplied to parallelepiped"
    }
    set values [$dataobj get components.parallelepiped.scale]
    set n [llength $values]
    scale set { 1.0 1.0 1.0 }
    if { $n == 1 } {
	set scale(0:2) [lindex $values 0]
    } elseif { $n == 2 } {
	set scale(0:1) [lindex $values 0]
    } elseif { $n == 3 } {
	scale set $values
    }
    set values [$dataobj get components.parallelepiped.origin]
    set n [llength $values]
    origin set { 0.0 0.0 0.0 }
    if { $n == 1 } {
	set origin(0) [lindex $values 0]
    } elseif { $n == 2 } {
	set origin(0) [lindex $values 0]
	set origin(1) [lindex $values 1]
    } elseif { $n == 3 } {
	origin set $values
    }
    point0 set { 0.0 0.0 0.0 }
    point4 expr {point2 + point1}
    point5 expr {point4 + point3}
    point6 expr {point2 + point3}
    point7 expr {point1 + point3}

    # Generate vertices as a string for PyMOL
    set vertices ""
    blt::vector x
    foreach n { 0 1 0 2 0 3 1 4 2 4 2 6 1 7 3 7 5 7 4 5 3 6 5 } {
	x expr "(point${n} * scale) + origin"
	set values [x print 0 end]
	append vertices "\[ [join $values {, }] \], \\\n"
    }
    x expr "(point6 * scale) + origin"
    set values [x print 0 end]
    append vertices "\[ [join $values {, }] \]  \\\n"
    blt::vector destroy point0 point1 point2 point3 point4 point5 point6 \
	point7 x origin scale
    return $vertices
}
