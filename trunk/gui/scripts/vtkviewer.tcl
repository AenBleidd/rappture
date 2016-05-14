# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: vtkviewer - Vtk drawing object viewer
#
#  It connects to the Vtkvis server running on a rendering farm,
#  transmits data, and displays the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2016  HUBzero Foundation, LLC
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

    constructor { args } {
        Rappture::VisViewer::constructor
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
    public method limits { dataobj }
    public method parameters {title args} {
        # do nothing
    }
    public method scale {args}

    # The following methods are only used by this class.
    private method AdjustSetting {what {value ""}}
    private method BuildAxisTab {}
    private method BuildCameraTab {}
    private method BuildColormap { name }
    private method BuildCutawayTab {}
    private method BuildDownloadPopup { widget command }
    private method BuildGlyphsTab {}
    private method BuildMoleculeTab {}
    private method BuildPolydataTab {}
    private method ChangeColormap { dataobj comp color }
    private method Connect {}
    private method CurrentDatasets {args}
    private method Disconnect {}
    private method DoResize {}
    private method DoRotate {}
    private method DrawLegend {}
    private method EnterLegend { x y }
    private method EventuallyResize { w h }
    private method EventuallyRotate { q }
    private method EventuallySetAtomScale { args }
    private method EventuallySetBondScale { args }
    private method EventuallySetGlyphsOpacity { args }
    private method EventuallySetMoleculeOpacity { args }
    private method EventuallySetMoleculeQuality { args }
    private method EventuallySetPolydataOpacity { args }
    private method GetImage { args }
    private method GetVtkData { args }
    private method InitSettings { args }
    private method IsValidObject { dataobj }
    private method LeaveLegend {}
    private method MotionLegend { x y }
    private method Pan {option x y}
    private method PanCamera {}
    private method Pick {x y}
    private method QuaternionToView { q } {
        foreach { _view(-qw) _view(-qx) _view(-qy) _view(-qz) } $q break
    }
    private method Rebuild {}
    private method ReceiveDataset { args }
    private method ReceiveImage { args }
    private method ReceiveLegend { colormap title min max size }
    private method RequestLegend {}
    private method Rotate {option x y}
    private method SetAtomScale {}
    private method SetBondScale {}
    private method SetColormap { dataobj comp }
    private method SetGlyphsOpacity {}
    private method SetLegendTip { x y }
    private method SetMoleculeOpacity {}
    private method SetMoleculeQuality {}
    private method SetObjectStyle { dataobj comp }
    private method SetOpacity { dataset }
    private method SetOrientation { side }
    private method SetPolydataOpacity {}
    private method Slice {option args}
    private method ViewToQuaternion {} {
        return [list $_view(-qw) $_view(-qx) $_view(-qy) $_view(-qz)]
    }
    private method Zoom {option}

    private variable _arcball ""
    private variable _dlist "";         # list of data objects
    private variable _obj2ovride;       # maps dataobj => style override
    private variable _datasets;         # contains all the dataobj-component
                                        # datasets in the server
    private variable _colormaps;        # contains all the colormaps
                                        # in the server.
    private variable _dataset2style;    # maps dataobj-component to transfunc
    private variable _click;            # info used for rotate operations
    private variable _limits;           # autoscale min/max for all axes
    private variable _view;             # view params for 3D view
    private variable _settings
    private variable _style;            # Array of current component styles.
    private variable _initialStyle;     # Array of initial component styles.
    private variable _axis
    private variable _reset 1;          # Connection to server has been reset.
    private variable _haveGlyphs 0
    private variable _haveMolecules 0
    private variable _havePolydata 0

    private variable _first "";         # This is the topmost dataset.
    private variable _start 0
    private variable _title ""
    private variable _width 0
    private variable _height 0
    private variable _resizePending 0
    private variable _rotatePending 0
    private variable _atomScalePending 0
    private variable _bondScalePending 0
    private variable _moleculeOpacityPending 0
    private variable _moleculeQualityPending 0
    private variable _polydataOpacityPending 0
    private variable _glyphsOpacityPending 0
    private variable _rotateDelay 150
    private variable _scaleDelay 100

    private common _downloadPopup;      # download options from popup
    private common _hardcopy
}

itk::usual VtkViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::constructor {args} {
    package require vtk
    set _serverType "vtkvis"

    #DebugOn
    EnableWaitDialog 900

    # Rebuild event
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    # Resize event
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this DoResize]; list"

    # Rotate event
    $_dispatcher register !rotate
    $_dispatcher dispatch $this !rotate "[itcl::code $this DoRotate]; list"

    # Atom scale event
    $_dispatcher register !atomscale
    $_dispatcher dispatch $this !atomscale \
        "[itcl::code $this SetAtomScale]; list"

    # Bond scale event
    $_dispatcher register !bondscale
    $_dispatcher dispatch $this !bondscale \
        "[itcl::code $this SetBondScale]; list"

    # Molecule opacity event
    $_dispatcher register !moleculeOpacity
    $_dispatcher dispatch $this !moleculeOpacity \
        "[itcl::code $this SetMoleculeOpacity]; list"

    # Molecule quality event
    $_dispatcher register !moleculeQuality
    $_dispatcher dispatch $this !moleculeQuality \
        "[itcl::code $this SetMoleculeQuality]; list"

    # Polydata opacity event
    $_dispatcher register !polydataOpacity
    $_dispatcher dispatch $this !polydataOpacity \
        "[itcl::code $this SetPolydataOpacity]; list"

    # Glyphs opacity event
    $_dispatcher register !glyphsOpacity
    $_dispatcher dispatch $this !glyphsOpacity \
        "[itcl::code $this SetGlyphsOpacity]; list"

    #
    # Populate parser with commands handle incoming requests
    #
    $_parser alias image [itcl::code $this ReceiveImage]
    $_parser alias legend [itcl::code $this ReceiveLegend]
    $_parser alias dataset [itcl::code $this ReceiveDataset]

    # Initialize the view to some default parameters.
    array set _view {
        -ortho    0
        -qw       0.853553
        -qx       -0.353553
        -qy       0.353553
        -qz       0.146447
        -xpan     0
        -ypan     0
        -zoom     1.0
    }
    set _arcball [blt::arcball create 100 100]
    $_arcball quaternion [ViewToQuaternion]

    set _limits(zmin) 0.0
    set _limits(zmax) 1.0

    array set _axis [subst {
        labels          1
        minorticks      1
        visible         1
        xgrid           0
        ygrid           0
        zgrid           0
        xcutaway        0
        ycutaway        0
        zcutaway        0
        xposition       0
        yposition       0
        zposition       0
        xdirection      -1
        ydirection      -1
        zdirection      -1
    }]
    array set _settings [subst {
        glyphs-edges            0
        glyphs-lighting         1
        glyphs-opacity          100
        glyphs-outline          0
        glyphs-palette          BCGYR
        glyphs-visible          1
        glyphs-wireframe        0
        legend                  1
        molecule-atoms-visible  1
        molecule-atomscale      0.3
        molecule-bonds-visible  1
        molecule-bondscale      0.075
        molecule-bondstyle      "cylinder"
        molecule-edges          0
        molecule-labels         0
        molecule-lighting       1
        molecule-opacity        100
        molecule-outline        0
        molecule-palette        elementDefault
        molecule-quality        1.0
        molecule-representation "Ball and Stick"
        molecule-rscale         "covalent"
        molecule-visible        1
        molecule-wireframe      0
        polydata-edges          0
        polydata-lighting       1
        polydata-opacity        100
        polydata-outline        0
        polydata-palette        BCGYR
        polydata-visible        1
        polydata-wireframe      0
    }]
    itk_component add view {
        canvas $itk_component(plotarea).view \
            -highlightthickness 0 -borderwidth 0
    } {
        usual
        ignore -highlightthickness -borderwidth -background
    }

    set c $itk_component(view)
    bind $c <Configure> [itcl::code $this EventuallyResize %w %h]
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

    if { [catch {
        BuildAxisTab
        #BuildCutawayTab
        BuildCameraTab
    } errs] != 0 } {
        puts stderr errs=$errs
    }

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
        [itcl::code $this Rotate click %x %y]
    bind $itk_component(view) <B1-Motion> \
        [itcl::code $this Rotate drag %x %y]
    bind $itk_component(view) <ButtonRelease-1> \
        [itcl::code $this Rotate release %x %y]

    # Bindings for panning via mouse
    bind $itk_component(view) <ButtonPress-2> \
        [itcl::code $this Pan click %x %y]
    bind $itk_component(view) <B2-Motion> \
        [itcl::code $this Pan drag %x %y]
    bind $itk_component(view) <ButtonRelease-2> \
        [itcl::code $this Pan release %x %y]

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
    $_dispatcher cancel !rotate
    image delete $_image(plot)
    image delete $_image(download)
    catch { blt::arcball destroy $_arcball }
}

itcl::body Rappture::VtkViewer::DoResize {} {
    if { $_width < 2 } {
        set _width 500
    }
    if { $_height < 2 } {
        set _height 500
    }
    set _start [clock clicks -milliseconds]
    SendCmd "screen size $_width $_height"

    set _resizePending 0
}

itcl::body Rappture::VtkViewer::DoRotate {} {
    SendCmd "camera orient [ViewToQuaternion]"
    set _rotatePending 0
}

itcl::body Rappture::VtkViewer::EventuallyResize { w h } {
    set _width $w
    set _height $h
    $_arcball resize $w $h
    if { !$_resizePending } {
        set _resizePending 1
        $_dispatcher event -after 200 !resize
    }
}

itcl::body Rappture::VtkViewer::EventuallyRotate { q } {
    QuaternionToView $q
    if { !$_rotatePending } {
        set _rotatePending 1
        $_dispatcher event -after $_rotateDelay !rotate
    }
}

itcl::body Rappture::VtkViewer::SetAtomScale {} {
    SendCmd "molecule ascale $_settings(molecule-atomscale)"
    set _atomScalePending 0
}

itcl::body Rappture::VtkViewer::SetBondScale {} {
    SendCmd "molecule bscale $_settings(molecule-bondscale)"
    set _bondScalePending 0
}

itcl::body Rappture::VtkViewer::SetMoleculeOpacity {} {
    set _moleculeOpacityPending 0
    foreach dataset [CurrentDatasets -visible $_first] {
        foreach { dataobj comp } [split $dataset -] break
        if { [$dataobj type $comp] == "molecule" } {
            SetOpacity $dataset
        }
    }
}

itcl::body Rappture::VtkViewer::SetMoleculeQuality {} {
    SendCmd [subst {molecule aquality $_settings(molecule-quality)
molecule bquality $_settings(molecule-quality)}]
    set _moleculeQualityPending 0
}

itcl::body Rappture::VtkViewer::SetGlyphsOpacity {} {
    set _glyphsOpacityPending 0
    foreach dataset [CurrentDatasets -visible $_first] {
        foreach { dataobj comp } [split $dataset -] break
        if { [$dataobj type $comp] == "glyphs" } {
            SetOpacity $dataset
        }
    }
}

itcl::body Rappture::VtkViewer::SetPolydataOpacity {} {
    set _polydataOpacityPending 0
    foreach dataset [CurrentDatasets -visible $_first] {
        foreach { dataobj comp } [split $dataset -] break
        if { [$dataobj type $comp] == "polydata" } {
            SetOpacity $dataset
        }
    }
}

itcl::body Rappture::VtkViewer::EventuallySetAtomScale { args } {
    if { !$_atomScalePending } {
        set _atomScalePending 1
        $_dispatcher event -after $_scaleDelay !atomscale
    }
}

itcl::body Rappture::VtkViewer::EventuallySetBondScale { args } {
    if { !$_bondScalePending } {
        set _bondScalePending 1
        $_dispatcher event -after $_scaleDelay !bondscale
    }
}

itcl::body Rappture::VtkViewer::EventuallySetMoleculeOpacity { args } {
    if { !$_moleculeOpacityPending } {
        set _moleculeOpacityPending 1
        $_dispatcher event -after $_scaleDelay !moleculeOpacity
    }
}

itcl::body Rappture::VtkViewer::EventuallySetMoleculeQuality { args } {
    if { !$_moleculeQualityPending } {
        set _moleculeQualityPending 1
        $_dispatcher event -after $_scaleDelay !moleculeQuality
    }
}

itcl::body Rappture::VtkViewer::EventuallySetPolydataOpacity { args } {
    if { !$_polydataOpacityPending } {
        set _polydataOpacityPending 1
        $_dispatcher event -after $_scaleDelay !polydataOpacity
    }
}

itcl::body Rappture::VtkViewer::EventuallySetGlyphsOpacity { args } {
    if { !$_glyphsOpacityPending } {
        set _glyphsOpacityPending 1
        $_dispatcher event -after $_scaleDelay !glyphsOpacity
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
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.  No data objects are
# deleted.  They are only removed from the display list.
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
        "-image" {
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
            error "bad option \"$op\": should be -objects, -visible or -image"
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
    foreach dataobj $args {
        foreach comp [$dataobj components] {
            set type [$dataobj type $comp]
            switch -- $type {
                "polydata" {
                    set _havePolydata 1
                }
                "glyphs" {
                    set _haveGlyphs 1
                }
                "molecule" {
                    set _haveMolecules 1
                }
            }
        }
        array set bounds [limits $dataobj]
        if {[info exists bounds(xmin)] && (![info exists _limits(xmin)] || $_limits(xmin) > $bounds(xmin))} {
            set _limits(xmin) $bounds(xmin)
        }
        if {[info exists bounds(xmax)] && (![info exists _limits(xmax)] || $_limits(xmax) < $bounds(xmax))} {
            set _limits(xmax) $bounds(xmax)
        }

        if {[info exists bounds(ymin)] && (![info exists _limits(ymin)] || $_limits(ymin) > $bounds(ymin))} {
            set _limits(ymin) $bounds(ymin)
        }
        if {[info exists bounds(ymax)] && (![info exists _limits(ymax)] || $_limits(ymax) < $bounds(ymax))} {
            set _limits(ymax) $bounds(ymax)
        }

        if {[info exists bounds(zmin)] && (![info exists _limits(zmin)] || $_limits(zmin) > $bounds(zmin))} {
            set _limits(zmin) $bounds(zmin)
        }
        if {[info exists bounds(zmax)] && (![info exists _limits(zmax)] || $_limits(zmax) < $bounds(zmax))} {
            set _limits(zmax) $bounds(zmax)
        }
    }
    if { $_haveGlyphs } {
        if { ![$itk_component(main) exists "Glyphs Settings"] } {
            if { [catch { BuildGlyphsTab } errs ]  != 0 } {
                puts stderr "errs=$errs"
            }
        }
    }
    if { $_havePolydata } {
        if { ![$itk_component(main) exists "Mesh Settings"] } {
            if { [catch { BuildPolydataTab } errs ]  != 0 } {
                puts stderr "errs=$errs"
            }
        }
    }
    if { $_haveMolecules } {
        if { ![$itk_component(main) exists "Molecule Settings"]} {
            if { [catch { BuildMoleculeTab } errs ]  != 0 } {
                global errorInfo
                puts stderr "errs=$errs\nerrorInfo=$errorInfo"
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
            update idletasks            ;# Fix initial sizes
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
    global readyForNextFrame
    set readyForNextFrame 1
    set _hosts [GetServerList "vtkvis"]
    if { "" == $_hosts } {
        return 0
    }
    set _reset 1
    set result [VisViewer::Connect $_hosts]
    if { $result } {
        if { $_reportClientInfo }  {
            # Tell the server the viewer, hub, user and session.
            # Do this immediately on connect before buffering any commands
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
            lappend info "version" "$Rappture::version"
            lappend info "build" "$Rappture::build"
            lappend info "svnurl" "$Rappture::svnurl"
            lappend info "installdir" "$Rappture::installdir"
            lappend info "hub" [exec hostname]
            lappend info "client" "vtkviewer"
            lappend info "user" $user
            lappend info "session" $session
            SendCmd "clientinfo [list $info]"
        }

        set w [winfo width $itk_component(view)]
        set h [winfo height $itk_component(view)]
        EventuallyResize $w $h
    }
    return $result
}

#
# isconnected --
#
# Indicates if we are currently connected to the visualization server.
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
# Clients use this method to disconnect from the current rendering server.
#
itcl::body Rappture::VtkViewer::Disconnect {} {
    VisViewer::Disconnect

    # disconnected -- no more data sitting on server
    array unset _datasets
    array unset _colormaps
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
itcl::body Rappture::VtkViewer::ReceiveImage { args } {
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
        update
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
        InitSettings axis-xgrid axis-ygrid axis-zgrid axis-mode \
            axis-visible axis-labels axis-minorticks

        StopBufferingCommands
        SendCmd "imgflush"
        StartBufferingCommands
    }

    set _limits(zmin) ""
    set _limits(zmax) ""
    set _first ""
    SendCmd "dataset visible 0"
    set count 0
    foreach dataobj [get -objects] {
        if { [info exists _obj2ovride($dataobj-raise)] &&  $_first == "" } {
            set _first $dataobj
        }
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _datasets($tag)] } {
                set bytes [$dataobj data $comp]
                if { $bytes == "" } {
                    continue
                }
                if 0 {
                    set f [open /tmp/vtkviewer.vtk "w"]
                    fconfigure $f -translation binary -encoding binary
                    puts -nonewline $f $bytes
                    close $f
                }
                set length [string length $bytes]
                if { $_reportClientInfo }  {
                    set info {}
                    lappend info "tool_id"       [$dataobj hints toolid]
                    lappend info "tool_name"     [$dataobj hints toolname]
                    lappend info "tool_title"    [$dataobj hints tooltitle]
                    lappend info "tool_command"  [$dataobj hints toolcommand]
                    lappend info "tool_revision" [$dataobj hints toolrevision]
                    lappend info "dataset_label" [$dataobj hints label]
                    lappend info "dataset_size"  $length
                    lappend info "dataset_tag"   $tag
                    SendCmd "clientinfo [list $info]"
                }
                SendCmd "dataset add $tag data follows $length"
                SendData $bytes
                set _datasets($tag) 1
                SetObjectStyle $dataobj $comp
            }
            set type [$dataobj type $comp]
            if { [info exists _obj2ovride($dataobj-raise)] } {
                SendCmd "$type visible 1 $tag"
                SetOpacity $tag
            }
        }
    }
    if {"" != $_first} {
        foreach axis { x y z } {
            set label [$_first hints ${axis}label]
            if { $label != "" } {
                SendCmd [list axis name $axis $label]
            }
            set units [$_first hints ${axis}units]
            if { $units != "" } {
                SendCmd [list axis units $axis $units]
            }
        }
    }
    if { $_haveGlyphs } {
        InitSettings glyphs-outline
    }
    if { $_haveMolecules } {
        InitSettings molecule-outline
    }
    if { $_havePolydata } {
        InitSettings polydata-outline
    }
    if { $_reset } {
        if { $_haveGlyphs } {
            InitSettings glyphs-edges glyphs-lighting glyphs-opacity \
                glyphs-visible glyphs-wireframe
        }
        if { $_havePolydata } {
            InitSettings polydata-edges polydata-lighting polydata-opacity \
                polydata-visible polydata-wireframe
        }
        if { $_haveMolecules } {
            InitSettings molecule-edges molecule-lighting molecule-opacity \
                molecule-visible molecule-wireframe molecule-labels
        }

        $_arcball quaternion [ViewToQuaternion]
        SendCmd "camera reset"
        if { $_view(-ortho)} {
            SendCmd "camera mode ortho"
        } else {
            SendCmd "camera mode persp"
        }
        DoRotate
        PanCamera
        Zoom reset
    }

    if { $_haveMolecules } {
        #InitSettings molecule-representation
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
            set _view(-zoom) [expr {$_view(-zoom)*1.25}]
            SendCmd "camera zoom $_view(-zoom)"
        }
        "out" {
            set _view(-zoom) [expr {$_view(-zoom)*0.8}]
            SendCmd "camera zoom $_view(-zoom)"
        }
        "reset" {
            array set _view {
                -qw      0.853553
                -qx      -0.353553
                -qy      0.353553
                -qz      0.146447
                -xpan    0
                -ypan    0
                -zoom    1.0
            }
            if { $_first != "" } {
                set location [$_first hints camera]
                if { $location != "" } {
                    array set _view $location
                }
            }
            $_arcball quaternion [ViewToQuaternion]
            DoRotate
            SendCmd "camera reset"
        }
    }
}

itcl::body Rappture::VtkViewer::PanCamera {} {
    set x $_view(-xpan)
    set y $_view(-ypan)
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
            set _view(-xpan) [expr $_view(-xpan) + $x]
            set _view(-ypan) [expr $_view(-ypan) + $y]
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
            set _view(-xpan) [expr $_view(-xpan) - $dx]
            set _view(-ypan) [expr $_view(-ypan) - $dy]
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
# USAGE: InitSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::InitSettings { args } {
    foreach setting $args {
        AdjustSetting $setting
    }
}

#
# AdjustSetting --
#
# Changes/updates a specific setting in the widget.  There are
# usually user-setable option.  Commands are sent to the render
# server.
#
itcl::body Rappture::VtkViewer::AdjustSetting {what {value ""}} {
    if { ![isconnected] } {
        return
    }
    switch -- $what {
        "glyphs-opacity" {
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                if { [$dataobj type $comp] == "glyphs" } {
                    SetOpacity $dataset
                }
            }
        }
        "glyphs-outline" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    SendCmd "outline visible $bool $dataset"
                }
            }
        }
        "glyphs-wireframe" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    SendCmd "$type wireframe $bool $dataset"
                }
            }
        }
        "glyphs-visible" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    SendCmd "$type visible $bool $dataset"
                }
            }
        }
        "glyphs-lighting" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    SendCmd "$type lighting $bool $dataset"
                }
            }
        }
        "glyphs-edges" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    SendCmd "$type edges $bool $dataset"
                }
            }
        }
        "glyphs-palette" {
            set palette [$itk_component(glyphspalette) value]
            set _settings($what) $palette
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "glyphs" } {
                    ChangeColormap $dataobj $comp $palette
                    # FIXME: fill in current selected fieldname
                    #SendCmd "glyphs colormode scalar {} $dataset"
                }
            }
            set _legendPending 1
        }
        "polydata-opacity" {
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                if { [$dataobj type $comp] == "polydata" } {
                    SetOpacity $dataset
                }
            }
        }
        "polydata-outline" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "outline visible $bool $dataset"
                }
            }
        }
        "polydata-wireframe" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type wireframe $bool $dataset"
                }
            }
        }
        "polydata-visible" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type visible $bool $dataset"
                }
            }
        }
        "polydata-lighting" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type lighting $bool $dataset"
                }
            }
        }
        "polydata-edges" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    SendCmd "$type edges $bool $dataset"
                }
            }
        }
        "polydata-palette" {
            set palette [$itk_component(meshpalette) value]
            set _settings($what) $palette
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "polydata" } {
                    ChangeColormap $dataobj $comp $palette
                    # FIXME: fill in current selected fieldname
                    #SendCmd "polydata colormode scalar {} $dataset"
                }
            }
            set _legendPending 1
        }
        "molecule-opacity" {
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                if { [$dataobj type $comp] == "molecule" } {
                    SetOpacity $dataset
                }
            }
        }
        "molecule-outline" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd "outline visible $bool $dataset"
                }
            }
        }
        "molecule-wireframe" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd "molecule wireframe $bool $dataset"
                }
            }
        }
        "molecule-visible" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd "molecule visible $bool $dataset"
                }
            }
        }
        "molecule-lighting" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd "molecule lighting $bool $dataset"
                }
            }
        }
        "molecule-edges" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach { dataobj comp } [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd "molecule edges $bool $dataset"
                }
            }
        }
        "molecule-palette" {
            set palette [$itk_component(moleculepalette) value]
            set _settings($what) $palette
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    ChangeColormap $dataobj $comp $palette
                    if { $palette == "elementDefault" } {
                        SendCmd "molecule colormode by_elements element $dataset"
                    } else {
                        # FIXME: Set the chosen scalar field name here
                        SendCmd "molecule colormode scalar {} $dataset"
                    }
                }
            }
            set _legendPending 1
        }
        "molecule-representation" {
            set value [$itk_component(representation) value]
            set value [$itk_component(representation) translate $value]
            switch -- $value {
                "ballandstick" {
                    set _settings(molecule-rscale) covalent
                    set _settings(molecule-atoms-visible) 1
                    set _settings(molecule-bonds-visible) 1
                    set _settings(molecule-bondstyle) cylinder
                    set _settings(molecule-atomscale) 0.3
                    set _settings(molecule-bondscale) 0.075
                }
                "balls" - "spheres" {
                    set _settings(molecule-rscale) covalent
                    set _settings(molecule-atoms-visible) 1
                    set _settings(molecule-bonds-visible) 0
                    set _settings(molecule-bondstyle) cylinder
                    set _settings(molecule-atomscale) 0.3
                    set _settings(molecule-bondscale) 0.075
                }
                "sticks" {
                    set _settings(molecule-rscale) none
                    set _settings(molecule-atoms-visible) 1
                    set _settings(molecule-bonds-visible) 1
                    set _settings(molecule-bondstyle) cylinder
                    set _settings(molecule-atomscale) 0.075
                    set _settings(molecule-bondscale) 0.075
                }
                "spacefilling" {
                    set _settings(molecule-rscale) van_der_waals
                    set _settings(molecule-atoms-visible) 1
                    set _settings(molecule-bonds-visible) 0
                    set _settings(molecule-bondstyle) cylinder
                    set _settings(molecule-atomscale) 1.0
                    set _settings(molecule-bondscale) 0.075
                }
                "rods"  {
                    set _settings(molecule-rscale) none
                    set _settings(molecule-atoms-visible) 1
                    set _settings(molecule-bonds-visible) 1
                    set _settings(molecule-bondstyle) cylinder
                    set _settings(molecule-atomscale) 0.1
                    set _settings(molecule-bondscale) 0.1
                }
                "wireframe" - "lines" {
                    set _settings(molecule-rscale) none
                    set _settings(molecule-atoms-visible) 0
                    set _settings(molecule-bonds-visible) 1
                    set _settings(molecule-bondstyle) line
                    set _settings(molecule-atomscale) 1.0
                    set _settings(molecule-bondscale) 1.0
                }
                default {
                    error "unknown representation $value"
                }
            }
            $itk_component(rscale) value [$itk_component(rscale) label $_settings(molecule-rscale)]
            switch -- $value {
                "ballandstick" - "balls" - "spheres" {
                    $itk_component(rscale) configure -state normal
                }
                default {
                    $itk_component(rscale) configure -state disabled
                }
            }
            foreach dataset [CurrentDatasets -all] {
                foreach {dataobj comp} [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    StartBufferingCommands
                    SendCmd [subst {molecule rscale $_settings(molecule-rscale) $dataset}]
                    SendCmd [subst {molecule ascale $_settings(molecule-atomscale) $dataset}]
                    SendCmd [subst {molecule bscale $_settings(molecule-bondscale) $dataset}]
                    SendCmd [subst {molecule bstyle $_settings(molecule-bondstyle) $dataset}]
                    SendCmd [subst {molecule atoms $_settings(molecule-atoms-visible) $dataset}]
                    SendCmd [subst {molecule bonds $_settings(molecule-bonds-visible) $dataset}]
                    StopBufferingCommands
                }
            }
        }
        "molecule-rscale" {
            set value [$itk_component(rscale) value]
            set value [$itk_component(rscale) translate $value]
            set _settings($what) $value
            foreach dataset [CurrentDatasets -visible $_first] {
                foreach {dataobj comp} [split $dataset -] break
                set type [$dataobj type $comp]
                if { $type == "molecule" } {
                    SendCmd [subst {molecule rscale $_settings($what) $dataset}]
                }
            }
        }
        "molecule-labels" {
            set bool $_settings($what)
            foreach dataset [CurrentDatasets -visible $_first] {
               foreach { dataobj comp } [split $dataset -] break
               set type [$dataobj type $comp]
               if { $type == "molecule" } {
                   SendCmd "molecule labels $bool $dataset"
               }
            }
        }
        "axis-visible" {
            set bool $_axis(visible)
            SendCmd "axis visible all $bool"
        }
        "axis-labels" {
            set bool $_axis(labels)
            SendCmd "axis labels all $bool"
        }
        "axis-minorticks" {
            set bool $_axis(minorticks)
            SendCmd "axis minticks all $bool"
        }
        "axis-xgrid" {
            set bool $_axis(xgrid)
            SendCmd "axis grid x $bool"
        }
        "axis-ygrid" {
            set bool $_axis(ygrid)
            SendCmd "axis grid y $bool"
        }
        "axis-zgrid" {
            set bool $_axis(zgrid)
            SendCmd "axis grid z $bool"
        }
        "axis-mode" {
            set mode [$itk_component(axismode) value]
            set mode [$itk_component(axismode) translate $mode]
            SendCmd "axis flymode $mode"
        }
        "axis-xcutaway" - "axis-ycutaway" - "axis-zcutaway" {
            set axis [string range $what 5 5]
            set bool $_axis(${axis}cutaway)
            if { $bool } {
                set pos [expr $_axis(${axis}position) * 0.01]
                set dir $_axis(${axis}direction)
                $itk_component(${axis}CutScale) configure -state normal \
                    -troughcolor white
                SendCmd "renderer clipplane $axis $pos $dir"
            } else {
                $itk_component(${axis}CutScale) configure -state disabled \
                    -troughcolor grey82
                SendCmd "renderer clipplane $axis 1 -1"
            }
        }
        "axis-xposition" - "axis-yposition" - "axis-zposition" -
        "axis-xdirection" - "axis-ydirection" - "axis-zdirection" {
            set axis [string range $what 5 5]
            #set dir $_axis(${axis}direction)
            set pos [expr $_axis(${axis}position) * 0.01]
            SendCmd "renderer clipplane ${axis} $pos -1"
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

#
# RequestLegend --
#
# Request a new legend from the server.  The size of the legend
# is determined from the height of the canvas.  It will be rotated
# to be vertical when drawn.
#
itcl::body Rappture::VtkViewer::RequestLegend {} {
    set font "Arial 8"
    set lineht [font metrics $font -linespace]
    set w 12
    set h [expr {$_height - 2 * ($lineht + 2)}]
    if { $h < 1 } {
        return
    }
    # Set the legend on the first dataset.
    foreach dataset [CurrentDatasets -visible] {
        foreach {dataobj comp} [split $dataset -] break
        if { [info exists _dataset2style($dataset)] } {
            #SendCmd "legend $_dataset2style($dataset) vmag {} {} $w $h 0"
            SendCmd "legend2 $_dataset2style($dataset) $w $h"
            break;
        }
    }
}

#
# ChangeColormap --
#
itcl::body Rappture::VtkViewer::ChangeColormap {dataobj comp color} {
    set tag $dataobj-$comp
    if { ![info exist _style($tag)] } {
        error "no initial colormap"
    }
    array set style $_style($tag)
    set style(-color) $color
    set _style($tag) [array get style]
    SetColormap $dataobj $comp
}

#
# SetColormap --
#
itcl::body Rappture::VtkViewer::SetColormap { dataobj comp } {
    array set style {
        -color BCGYR
    }
    if {[$dataobj type $comp] == "molecule"} {
        set style(-color) elementDefault
    }
    set tag $dataobj-$comp
    if { ![info exists _initialStyle($tag)] } {
        # Save the initial component style.
        set _initialStyle($tag) [$dataobj style $comp]
    }

    # Override defaults with initial style defined in xml.
    array set style $_initialStyle($tag)

    if { ![info exists _style($tag)] } {
        set _style($tag) [array get style]
    }
    # Override initial style with current style.
    array set style $_style($tag)

    set name "$style(-color)"
    if { ![info exists _colormaps($name)] } {
        BuildColormap $name
        set _colormaps($name) 1
    }
    if { ![info exists _dataset2style($tag)] ||
         $_dataset2style($tag) != $name } {
        set _dataset2style($tag) $name
        switch -- [$dataobj type $comp] {
            "polydata" {
                SendCmd "polydata colormap $name $tag"
            }
            "glyphs" {
                SendCmd "glyphs colormap $name $tag"
            }
            "molecule" {
                SendCmd "molecule colormap $name $tag"
            }
        }
    }
}

#
# BuildColormap --
#
itcl::body Rappture::VtkViewer::BuildColormap { name } {
    if { $name == "elementDefault" } {
        return
    }
    set cmap [ColorsToColormap $name]
    if { [llength $cmap] == 0 } {
        set cmap "0.0 0.0 0.0 0.0 1.0 1.0 1.0 1.0"
    }
    set amap "0.0 1.0 1.0 1.0"
    SendCmd "colormap add $name { $cmap } { $amap }"
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewer::plotbackground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotbackground)]
        SendCmd "screen bgcolor $rgb"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewer::plotforeground {
    if { [isconnected] } {
        set rgb [Color2RGB $itk_option(-plotforeground)]
        SendCmd "axis color all $rgb"
        SendCmd "outline color $rgb"
    }
}

itcl::body Rappture::VtkViewer::limits { dataobj } {
    foreach comp [$dataobj components] {
        set tag $dataobj-$comp
        if { ![info exists _limits($tag)] } {
            set data [$dataobj data $comp]
            if { $data == "" } {
                continue
            }
            set tmpfile file[pid].vtk
            set f [open "$tmpfile" "w"]
            fconfigure $f -translation binary -encoding binary
            puts $f $data
            close $f
            set reader [vtkDataSetReader $tag-xvtkDataSetReader]
            $reader SetFileName $tmpfile
set debug 0
            if {$debug} {
                # Only needed for debug output below
                $reader ReadAllNormalsOn
                $reader ReadAllTCoordsOn
                $reader ReadAllScalarsOn
                $reader ReadAllColorScalarsOn
                $reader ReadAllVectorsOn
                $reader ReadAllTensorsOn
                $reader ReadAllFieldsOn
            }
            $reader Update
            file delete $tmpfile
            set output [$reader GetOutput]
            if { $output == "" } {
                # Invalid VTK file -- loader failed to parse
                continue
            }
            set _limits($tag) [$output GetBounds]
            if {$debug} {
                puts stderr "\#scalars=[$reader GetNumberOfScalarsInFile]"
                puts stderr "\#vectors=[$reader GetNumberOfVectorsInFile]"
                puts stderr "\#tensors=[$reader GetNumberOfTensorsInFile]"
                puts stderr "\#normals=[$reader GetNumberOfNormalsInFile]"
                puts stderr "\#tcoords=[$reader GetNumberOfTCoordsInFile]"
                puts stderr "\#fielddata=[$reader GetNumberOfFieldDataInFile]"
                puts stderr "fielddataname=[$reader GetFieldDataNameInFile 0]"
                set pointData [$output GetPointData]
                if { $pointData != ""} {
                    puts stderr "point \#arrays=[$pointData GetNumberOfArrays]"
                    puts stderr "point \#components=[$pointData GetNumberOfComponents]"
                    puts stderr "point \#tuples=[$pointData GetNumberOfTuples]"
                    puts stderr "point scalars=[$pointData GetScalars]"
                    puts stderr "point vectors=[$pointData GetVectors]"
                }
                set cellData [$output GetCellData]
                if { $cellData != ""} {
                    puts stderr "cell \#arrays=[$cellData GetNumberOfArrays]"
                    puts stderr "cell \#components=[$cellData GetNumberOfComponents]"
                    puts stderr "cell \#tuples=[$cellData GetNumberOfTuples]"
                    puts stderr "cell scalars=[$cellData GetScalars]"
                    puts stderr "cell vectors=[$cellData GetVectors]"
                }
                set fieldData [$output GetFieldData]
                if { $fieldData != ""} {
                    puts stderr "field \#arrays=[$fieldData GetNumberOfArrays]"
                    puts stderr "field \#components=[$fieldData GetNumberOfComponents]"
                    puts stderr "field \#tuples=[$fieldData GetNumberOfTuples]"
                }
            }
            rename $output ""
            rename $reader ""
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

itcl::body Rappture::VtkViewer::BuildGlyphsTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert 0 \
        -title "Glyph Settings" \
        -icon [Rappture::icon volume-on]]
    $inner configure -borderwidth 4

    checkbutton $inner.glyphs \
        -text "Show Glyphs" \
        -variable [itcl::scope _settings(glyphs-visible)] \
        -command [itcl::code $this AdjustSetting glyphs-visible] \
        -font "Arial 9" -anchor w

    checkbutton $inner.outline \
        -text "Show Outline" \
        -variable [itcl::scope _settings(glyphs-outline)] \
        -command [itcl::code $this AdjustSetting glyphs-outline] \
        -font "Arial 9" -anchor w

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(glyphs-wireframe)] \
        -command [itcl::code $this AdjustSetting glyphs-wireframe] \
        -font "Arial 9" -anchor w

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(glyphs-lighting)] \
        -command [itcl::code $this AdjustSetting glyphs-lighting] \
        -font "Arial 9" -anchor w

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(glyphs-edges)] \
        -command [itcl::code $this AdjustSetting glyphs-edges] \
        -font "Arial 9" -anchor w

    label $inner.palette_l -text "Palette" -font "Arial 9" -anchor w
    itk_component add glyphspalette {
        Rappture::Combobox $inner.palette -width 10 -editable 0
    }
    $inner.palette choices insert end [GetColormapList]
    $itk_component(glyphspalette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting glyphs-palette]

    label $inner.opacity_l -text "Opacity" -font "Arial 9" -anchor w
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(glyphs-opacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this EventuallySetGlyphsOpacity]
    $inner.opacity set $_settings(glyphs-opacity)

    blt::table $inner \
        0,0 $inner.glyphs    -cspan 2  -anchor w -pady 2 \
        1,0 $inner.outline   -cspan 2  -anchor w -pady 2 \
        2,0 $inner.wireframe -cspan 2  -anchor w -pady 2 \
        3,0 $inner.lighting  -cspan 2  -anchor w -pady 2 \
        4,0 $inner.edges     -cspan 2  -anchor w -pady 2 \
        5,0 $inner.opacity_l -anchor w -pady 2 \
        5,1 $inner.opacity   -fill x   -pady 2 \
        6,0 $inner.palette_l -anchor w -pady 2 \
        6,1 $inner.palette   -fill x   -pady 2

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r8 c1 -resize expand
}

itcl::body Rappture::VtkViewer::BuildPolydataTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert 0 \
        -title "Mesh Settings" \
        -icon [Rappture::icon mesh]]
    $inner configure -borderwidth 4

    checkbutton $inner.mesh \
        -text "Show Mesh" \
        -variable [itcl::scope _settings(polydata-visible)] \
        -command [itcl::code $this AdjustSetting polydata-visible] \
        -font "Arial 9" -anchor w

    checkbutton $inner.outline \
        -text "Show Outline" \
        -variable [itcl::scope _settings(polydata-outline)] \
        -command [itcl::code $this AdjustSetting polydata-outline] \
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
        Rappture::Combobox $inner.palette -width 10 -editable 0
    }
    $inner.palette choices insert end [GetColormapList]
    $itk_component(meshpalette) value "BCGYR"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting polydata-palette]

    label $inner.opacity_l -text "Opacity" -font "Arial 9" -anchor w
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(polydata-opacity)] \
        -width 10 \
        -showvalue off \
        -command [itcl::code $this EventuallySetPolydataOpacity]
    $inner.opacity set $_settings(polydata-opacity)

    blt::table $inner \
        0,0 $inner.mesh      -cspan 2  -anchor w -pady 2 \
        1,0 $inner.outline   -cspan 2  -anchor w -pady 2 \
        2,0 $inner.wireframe -cspan 2  -anchor w -pady 2 \
        3,0 $inner.lighting  -cspan 2  -anchor w -pady 2 \
        4,0 $inner.edges     -cspan 2  -anchor w -pady 2 \
        5,0 $inner.opacity_l -anchor w -pady 2 \
        5,1 $inner.opacity   -fill x   -pady 2 \
        6,0 $inner.palette_l -anchor w -pady 2 \
        6,1 $inner.palette   -fill x   -pady 2

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r8 c1 -resize expand
}

itcl::body Rappture::VtkViewer::BuildAxisTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set inner [$itk_component(main) insert end \
        -title "Axis Settings" \
        -icon [Rappture::icon axis2]]
    $inner configure -borderwidth 4

    checkbutton $inner.visible \
        -text "Axes" \
        -variable [itcl::scope _axis(visible)] \
        -command [itcl::code $this AdjustSetting axis-visible] \
        -font "Arial 9"

    checkbutton $inner.labels \
        -text "Axis Labels" \
        -variable [itcl::scope _axis(labels)] \
        -command [itcl::code $this AdjustSetting axis-labels] \
        -font "Arial 9"
    label $inner.grid_l -text "Grid" -font "Arial 9"
    checkbutton $inner.xgrid \
        -text "X" \
        -variable [itcl::scope _axis(xgrid)] \
        -command [itcl::code $this AdjustSetting axis-xgrid] \
        -font "Arial 9"
    checkbutton $inner.ygrid \
        -text "Y" \
        -variable [itcl::scope _axis(ygrid)] \
        -command [itcl::code $this AdjustSetting axis-ygrid] \
        -font "Arial 9"
    checkbutton $inner.zgrid \
        -text "Z" \
        -variable [itcl::scope _axis(zgrid)] \
        -command [itcl::code $this AdjustSetting axis-zgrid] \
        -font "Arial 9"
    checkbutton $inner.minorticks \
        -text "Minor Ticks" \
        -variable [itcl::scope _axis(minorticks)] \
        -command [itcl::code $this AdjustSetting axis-minorticks] \
        -font "Arial 9"

    label $inner.mode_l -text "Mode" -font "Arial 9"

    itk_component add axismode {
        Rappture::Combobox $inner.mode -width 10 -editable 0
    }
    $inner.mode choices insert end \
        "static_triad"    "static" \
        "closest_triad"   "closest" \
        "furthest_triad"  "farthest" \
        "outer_edges"     "outer"
    $itk_component(axismode) value "static"
    bind $inner.mode <<Value>> [itcl::code $this AdjustSetting axis-mode]

    blt::table $inner \
        0,0 $inner.visible -anchor w -cspan 4 \
        1,0 $inner.labels  -anchor w -cspan 4 \
        2,0 $inner.minorticks  -anchor w -cspan 4 \
        4,0 $inner.grid_l  -anchor w \
        4,1 $inner.xgrid   -anchor w \
        4,2 $inner.ygrid   -anchor w \
        4,3 $inner.zgrid   -anchor w \
        5,0 $inner.mode_l  -anchor w -padx { 2 0 } \
        5,1 $inner.mode    -fill x   -cspan 3

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r7 c6 -resize expand
    blt::table configure $inner r3 -height 0.125i
}

itcl::body Rappture::VtkViewer::BuildCameraTab {} {
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
    blt::table configure $inner r0 -resize none

    set labels { qx qy qz qw xpan ypan zoom }
    set row 1
    foreach tag $labels {
        label $inner.${tag}label -text $tag -font "Arial 9"
        entry $inner.${tag} -font "Arial 9"  -bg white \
            -textvariable [itcl::scope _view(-$tag)]
        bind $inner.${tag} <Return> \
            [itcl::code $this camera set -${tag}]
        bind $inner.${tag} <KP_Enter> \
            [itcl::code $this camera set -${tag}]
        blt::table $inner \
            $row,0 $inner.${tag}label -anchor e -pady 2 \
            $row,1 $inner.${tag} -anchor w -pady 2
        blt::table configure $inner r$row -resize none
        incr row
    }
    checkbutton $inner.ortho \
        -text "Orthographic Projection" \
        -variable [itcl::scope _view(-ortho)] \
        -command [itcl::code $this camera set -ortho] \
        -font "Arial 9"
    blt::table $inner \
            $row,0 $inner.ortho -cspan 2 -anchor w -pady 2
    blt::table configure $inner r$row -resize none
    incr row

    blt::table configure $inner c* -resize none
    blt::table configure $inner c2 -resize expand
    blt::table configure $inner r$row -resize expand
}

itcl::body Rappture::VtkViewer::BuildCutawayTab {} {

    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert end \
        -title "Cutaway Along Axis" \
        -icon [Rappture::icon cutbutton]]

    $inner configure -borderwidth 4

    # X-value slicer...
    itk_component add xCutButton {
        Rappture::PushButton $inner.xbutton \
            -onimage [Rappture::icon x-cutplane] \
            -offimage [Rappture::icon x-cutplane] \
            -command [itcl::code $this AdjustSetting axis-xcutaway] \
            -variable [itcl::scope _axis(xcutaway)]
    }
    Rappture::Tooltip::for $itk_component(xCutButton) \
        "Toggle the X-axis cutaway on/off"

    itk_component add xCutScale {
        ::scale $inner.xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x] \
            -variable [itcl::scope _axis(xposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    # Set the default cutaway value before disabling the scale.
    $itk_component(xCutScale) set 100
    $itk_component(xCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(xCutScale) \
        "@[itcl::code $this Slice tooltip x]"

    itk_component add xDirButton {
        Rappture::PushButton $inner.xdir \
            -onimage [Rappture::icon arrow-down] \
            -onvalue -1 \
            -offimage [Rappture::icon arrow-up] \
            -offvalue 1 \
            -command [itcl::code $this AdjustSetting axis-xdirection] \
            -variable [itcl::scope _axis(xdirection)]
    }
    set _axis(xdirection) -1
    Rappture::Tooltip::for $itk_component(xDirButton) \
        "Toggle the direction of the X-axis cutaway"

    # Y-value slicer...
    itk_component add yCutButton {
        Rappture::PushButton $inner.ybutton \
            -onimage [Rappture::icon y-cutplane] \
            -offimage [Rappture::icon y-cutplane] \
            -command [itcl::code $this AdjustSetting axis-ycutaway] \
            -variable [itcl::scope _axis(ycutaway)]
    }
    Rappture::Tooltip::for $itk_component(yCutButton) \
        "Toggle the Y-axis cutaway on/off"

    itk_component add yCutScale {
        ::scale $inner.yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y] \
            -variable [itcl::scope _axis(yposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    Rappture::Tooltip::for $itk_component(yCutScale) \
        "@[itcl::code $this Slice tooltip y]"
    # Set the default cutaway value before disabling the scale.
    $itk_component(yCutScale) set 100
    $itk_component(yCutScale) configure -state disabled

    itk_component add yDirButton {
        Rappture::PushButton $inner.ydir \
            -onimage [Rappture::icon arrow-down] \
            -onvalue -1 \
            -offimage [Rappture::icon arrow-up] \
            -offvalue 1 \
            -command [itcl::code $this AdjustSetting axis-ydirection] \
            -variable [itcl::scope _axis(ydirection)]
    }
    Rappture::Tooltip::for $itk_component(yDirButton) \
        "Toggle the direction of the Y-axis cutaway"
    set _axis(ydirection) -1

    # Z-value slicer...
    itk_component add zCutButton {
        Rappture::PushButton $inner.zbutton \
            -onimage [Rappture::icon z-cutplane] \
            -offimage [Rappture::icon z-cutplane] \
            -command [itcl::code $this AdjustSetting axis-zcutaway] \
            -variable [itcl::scope _axis(zcutaway)]
    }
    Rappture::Tooltip::for $itk_component(zCutButton) \
        "Toggle the Z-axis cutaway on/off"

    itk_component add zCutScale {
        ::scale $inner.zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue 1 \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z] \
            -variable [itcl::scope _axis(zposition)]
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    $itk_component(zCutScale) set 100
    $itk_component(zCutScale) configure -state disabled
    Rappture::Tooltip::for $itk_component(zCutScale) \
        "@[itcl::code $this Slice tooltip z]"

    itk_component add zDirButton {
        Rappture::PushButton $inner.zdir \
            -onimage [Rappture::icon arrow-down] \
            -onvalue -1 \
            -offimage [Rappture::icon arrow-up] \
            -offvalue 1 \
            -command [itcl::code $this AdjustSetting axis-zdirection] \
            -variable [itcl::scope _axis(zdirection)]
    }
    set _axis(zdirection) -1
    Rappture::Tooltip::for $itk_component(zDirButton) \
        "Toggle the direction of the Z-axis cutaway"

    blt::table $inner \
        0,0 $itk_component(xCutButton)  -anchor e -padx 2 -pady 2 \
        1,0 $itk_component(xCutScale)   -fill y \
        0,1 $itk_component(yCutButton)  -anchor e -padx 2 -pady 2 \
        1,1 $itk_component(yCutScale)   -fill y \
        0,2 $itk_component(zCutButton)  -anchor e -padx 2 -pady 2 \
        1,2 $itk_component(zCutScale)   -fill y \

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner r1 c3 -resize expand
}

itcl::body Rappture::VtkViewer::BuildMoleculeTab {} {
    set fg [option get $itk_component(hull) font Font]

    set inner [$itk_component(main) insert 0 \
        -title "Molecule Settings" \
        -icon [Rappture::icon molecule]]
    $inner configure -borderwidth 4

    checkbutton $inner.molecule \
        -text "Show Molecule" \
        -variable [itcl::scope _settings(molecule-visible)] \
        -command [itcl::code $this AdjustSetting molecule-visible] \
        -font "Arial 9"

    checkbutton $inner.outline \
        -text "Show Outline" \
        -variable [itcl::scope _settings(molecule-outline)] \
        -command [itcl::code $this AdjustSetting molecule-outline] \
        -font "Arial 9"

    checkbutton $inner.label \
        -text "Show Atom Labels" \
        -variable [itcl::scope _settings(molecule-labels)] \
        -command [itcl::code $this AdjustSetting molecule-labels] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Show Wireframe" \
        -variable [itcl::scope _settings(molecule-wireframe)] \
        -command [itcl::code $this AdjustSetting molecule-wireframe] \
        -font "Arial 9"

    checkbutton $inner.lighting \
        -text "Enable Lighting" \
        -variable [itcl::scope _settings(molecule-lighting)] \
        -command [itcl::code $this AdjustSetting molecule-lighting] \
        -font "Arial 9"

    checkbutton $inner.edges \
        -text "Show Edges" \
        -variable [itcl::scope _settings(molecule-edges)] \
        -command [itcl::code $this AdjustSetting molecule-edges] \
        -font "Arial 9"

    label $inner.rep_l -text "Molecule Representation" \
        -font "Arial 9"

    itk_component add representation {
        Rappture::Combobox $inner.rep -width 20 -editable 0
    }
    $inner.rep choices insert end \
        "ballandstick"  "Ball and Stick" \
        "spheres"       "Spheres"        \
        "sticks"        "Sticks"         \
        "rods"          "Rods"           \
        "wireframe"     "Wireframe"      \
        "spacefilling"  "Space Filling"

    bind $inner.rep <<Value>> \
        [itcl::code $this AdjustSetting molecule-representation]
    $inner.rep value "Ball and Stick"

    label $inner.rscale_l -text "Atom Radii" \
        -font "Arial 9"

    itk_component add rscale {
        Rappture::Combobox $inner.rscale -width 20 -editable 0
    }
    $inner.rscale choices insert end \
        "atomic"        "Atomic"   \
        "covalent"      "Covalent" \
        "van_der_waals" "VDW"      \
        "none"          "Constant"

    bind $inner.rscale <<Value>> \
        [itcl::code $this AdjustSetting molecule-rscale]
    $inner.rscale value "Covalent"

    label $inner.palette_l -text "Palette" -font "Arial 9"
    itk_component add moleculepalette {
        Rappture::Combobox $inner.palette -width 10 -editable 0
    }
    $inner.palette choices insert end [GetColormapList -includeElementDefault]
    $itk_component(moleculepalette) value "elementDefault"
    bind $inner.palette <<Value>> \
        [itcl::code $this AdjustSetting molecule-palette]

    label $inner.atomscale_l -text "Atom Scale" -font "Arial 9"
    ::scale $inner.atomscale -width 15 -font "Arial 7" \
        -from 0.025 -to 2.0 -resolution 0.025 -label "" \
        -showvalue true -orient horizontal \
        -command [itcl::code $this EventuallySetAtomScale] \
        -variable [itcl::scope _settings(molecule-atomscale)]
    $inner.atomscale set $_settings(molecule-atomscale)
    Rappture::Tooltip::for $inner.atomscale \
        "Adjust relative scale of atoms (spheres or balls)."

    label $inner.bondscale_l -text "Bond Scale" -font "Arial 9"
    ::scale $inner.bondscale -width 15 -font "Arial 7" \
        -from 0.005 -to 0.3 -resolution 0.005 -label "" \
        -showvalue true -orient horizontal \
        -command [itcl::code $this EventuallySetBondScale] \
        -variable [itcl::scope _settings(molecule-bondscale)]
    Rappture::Tooltip::for $inner.bondscale \
        "Adjust scale of bonds (sticks)."
    $inner.bondscale set $_settings(molecule-bondscale)

    label $inner.opacity_l -text "Opacity" -font "Arial 9"
    ::scale $inner.opacity -from 0 -to 100 -orient horizontal \
        -variable [itcl::scope _settings(molecule-opacity)] \
        -width 15 -font "Arial 7" \
        -showvalue on \
        -command [itcl::code $this EventuallySetMoleculeOpacity]

    label $inner.quality_l -text "Quality" -font "Arial 9"
    ::scale $inner.quality -width 15 -font "Arial 7" \
        -from 0.0 -to 10.0 -resolution 0.1 -label "" \
        -showvalue true -orient horizontal \
        -command [itcl::code $this EventuallySetMoleculeQuality] \
        -variable [itcl::scope _settings(molecule-quality)]
    Rappture::Tooltip::for $inner.quality \
        "Adjust tesselation quality"
    $inner.quality set $_settings(molecule-quality)

    blt::table $inner \
        0,0 $inner.molecule     -anchor w -pady {1 0} \
        1,0 $inner.outline      -anchor w -pady {1 0} \
        2,0 $inner.label        -anchor w -pady {1 0} \
        3,0 $inner.edges        -anchor w -pady {1 0} \
        4,0 $inner.rep_l        -anchor w -pady { 2 0 } \
        5,0 $inner.rep          -fill x    -pady 2 \
        6,0 $inner.rscale_l     -anchor w -pady { 2 0 } \
        7,0 $inner.rscale       -fill x    -pady 2 \
        8,0 $inner.palette_l    -anchor w  -pady 0 \
        9,0 $inner.palette      -fill x    -padx 2 \
        10,0 $inner.atomscale_l  -anchor w -pady {3 0} \
        11,0 $inner.atomscale   -fill x    -padx 2 \
        12,0 $inner.bondscale_l -anchor w -pady {3 0} \
        13,0 $inner.bondscale   -fill x   -padx 2 \
        14,0 $inner.opacity_l   -anchor w -pady {3 0} \
        15,0 $inner.opacity     -fill x    -padx 2 \
        16,0 $inner.quality_l   -anchor w -pady {3 0} \
        17,0 $inner.quality     -fill x    -padx 2

    blt::table configure $inner r* -resize none
    blt::table configure $inner r18 -resize expand
}

#
# camera --
#
itcl::body Rappture::VtkViewer::camera {option args} {
    switch -- $option {
        "show" {
            puts [array get _view]
        }
        "set" {
            set what [lindex $args 0]
            set x $_view($what)
            set code [catch { string is double $x } result]
            if { $code != 0 || !$result } {
                return
            }
            switch -- $what {
                "-ortho" {
                    if {$_view($what)} {
                        SendCmd "camera mode ortho"
                    } else {
                        SendCmd "camera mode persp"
                    }
                }
                "-xpan" - "-ypan" {
                    PanCamera
                }
                "-qx" - "-qy" - "-qz" - "-qw" {
                    set q [ViewToQuaternion]
                    $_arcball quaternion $q
                    EventuallyRotate $q
                }
                "-zoom" {
                    SendCmd "camera zoom $_view($what)"
                }
            }
        }
    }
}

itcl::body Rappture::VtkViewer::GetVtkData { args } {
    set bytes ""
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            set contents [$dataobj data $comp]
            append bytes "$contents\n"
        }
    }
    return [list .vtk $bytes]
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
        -font "Arial 9" \
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
    # Parse style string.
    set tag $dataobj-$comp
    set type [$dataobj type $comp]
    set style [$dataobj style $comp]
    switch -- $type {
        "glyphs" {
            array set settings {
                -color white
                -edgecolor black
                -edges 0
                -gscale 1
                -lighting 1
                -linewidth 1.0
                -normscale 0
                -opacity 1.0
                -orientglyphs 0
                -outline 0
                -ptsize 1.0
                -quality 1
                -scalemode "vcomp"
                -shape "sphere"
                -visible 1
                -wireframe 0
            }
            array set settings $style
            set shape [$dataobj shape $comp]
            if {$shape != ""} {
                set settings(-shape) $shape
            }
            # Backwards compat with camel case style option
            if { [info exists settings(-orientGlyphs)] } {
                set settings(-orientglyphs) $settings(-orientGlyphs)
                array unset settings -orientGlyphs
            }
            # Backwards compat with camel case style option
            if { [info exists settings(-scaleMode)] } {
                set settings(-scalemode) $settings(-scaleMode)
                array unset settings -scaleMode
            }
            SendCmd "outline add $tag"
            SendCmd "outline color [Color2RGB $settings(-color)] $tag"
            SendCmd "outline visible $settings(-outline) $tag"
            set _settings(glyphs-outline) $settings(-outline)

            SendCmd "glyphs add $settings(-shape) $tag"
            SendCmd "glyphs normscale $settings(-normscale) $tag"
            SendCmd "glyphs gscale $settings(-gscale) $tag"
            SendCmd "glyphs wireframe $settings(-wireframe) $tag"
            SendCmd "glyphs color [Color2RGB $settings(-color)] $tag"
            #SendCmd "glyphs colormode constant {} $tag"
            # Omitting field name for gorient and smode commands
            # defaults to active scalars or vectors depending on mode
            SendCmd "glyphs gorient $settings(-orientglyphs) {} $tag"
            SendCmd "glyphs smode $settings(-scalemode) {} $tag"
            SendCmd "glyphs edges $settings(-edges) $tag"
            SendCmd "glyphs linecolor [Color2RGB $settings(-edgecolor)] $tag"
            SendCmd "glyphs linewidth $settings(-linewidth) $tag"
            SendCmd "glyphs ptsize $settings(-ptsize) $tag"
            SendCmd "glyphs quality $settings(-quality) $tag"
            SendCmd "glyphs lighting $settings(-lighting) $tag"
            SendCmd "glyphs opacity $settings(-opacity) $tag"
            set _settings(glyphs-opacity) [expr 100.0 * $settings(-opacity)]
            SendCmd "glyphs visible $settings(-visible) $tag"
            set _settings(glyphs-wireframe) $settings(-wireframe)
        }
        "molecule" {
            array set settings {
                -atomscale 0.3
                -atomsvisible 1
                -bondscale 0.075
                -bondstyle "cylinder"
                -bondsvisible 1
                -color "elementDefault"
                -edgecolor black
                -edges 0
                -labels 0
                -lighting 1
                -linewidth 1.0
                -opacity 1.0
                -outline 0
                -quality 1.0
                -representation ""
                -rscale "covalent"
                -visible 1
                -wireframe 0
            }
            array set settings $style

            SendCmd "outline add $tag"
            SendCmd "outline color [Color2RGB white] $tag"
            SendCmd "outline visible $settings(-outline) $tag"
            set _settings(molecule-outline) $settings(-outline)

            SendCmd "molecule add $tag"
            if {$settings(-representation) != ""} {
                switch -- $settings(-representation) {
                    "ballandstick" {
                        set _settings(molecule-rscale) covalent
                        set _settings(molecule-atoms-visible) 1
                        set _settings(molecule-bonds-visible) 1
                        set _settings(molecule-bondstyle) cylinder
                        set _settings(molecule-atomscale) 0.3
                        set _settings(molecule-bondscale) 0.075
                    }
                    "balls" - "spheres" {
                        set _settings(molecule-rscale) covalent
                        set _settings(molecule-atoms-visible) 1
                        set _settings(molecule-bonds-visible) 0
                        set _settings(molecule-bondstyle) cylinder
                        set _settings(molecule-atomscale) 0.3
                        set _settings(molecule-bondscale) 0.075
                    }
                    "sticks" {
                        set _settings(molecule-rscale) none
                        set _settings(molecule-atoms-visible) 1
                        set _settings(molecule-bonds-visible) 1
                        set _settings(molecule-bondstyle) cylinder
                        set _settings(molecule-atomscale) 0.075
                        set _settings(molecule-bondscale) 0.075
                    }
                    "spacefilling" {
                        set _settings(molecule-rscale) van_der_waals
                        set _settings(molecule-atoms-visible) 1
                        set _settings(molecule-bonds-visible) 0
                        set _settings(molecule-bondstyle) cylinder
                        set _settings(molecule-atomscale) 1.0
                        set _settings(molecule-bondscale) 0.075
                    }
                    "rods"  {
                        set _settings(molecule-rscale) none
                        set _settings(molecule-atoms-visible) 1
                        set _settings(molecule-bonds-visible) 1
                        set _settings(molecule-bondstyle) cylinder
                        set _settings(molecule-atomscale) 0.1
                        set _settings(molecule-bondscale) 0.1
                    }
                    "wireframe" - "lines" {
                        set _settings(molecule-rscale) none
                        set _settings(molecule-atoms-visible) 0
                        set _settings(molecule-bonds-visible) 1
                        set _settings(molecule-bondstyle) line
                        set _settings(molecule-atomscale) 1.0
                        set _settings(molecule-bondscale) 1.0
                    }
                    default {
                        error "unknown representation $value"
                    }
                }
                SendCmd "molecule rscale $_settings(molecule-rscale) $tag"
                SendCmd "molecule atoms $_settings(molecule-atoms-visible) $tag"
                SendCmd "molecule bonds $_settings(molecule-bonds-visible) $tag"
                SendCmd "molecule bstyle $_settings(molecule-bondstyle) $tag"
                SendCmd "molecule ascale $_settings(molecule-atomscale) $tag"
                SendCmd "molecule bscale $_settings(molecule-bondscale) $tag"
                $itk_component(representation) value [$itk_component(representation) label $settings(-representation)]
                $itk_component(rscale) value [$itk_component(rscale) label $_settings(molecule-rscale)]
                switch -- $settings(-representation) {
                    "ballandstick" - "balls" - "spheres" {
                        $itk_component(rscale) configure -state normal
                    }
                    default {
                        $itk_component(rscale) configure -state disabled
                    }
                }
            } else {
                SendCmd "molecule rscale $settings(-rscale) $tag"
                set _settings(molecule-rscale) $settings(-rscale)
                SendCmd "molecule atoms $settings(-atomsvisible) $tag"
                set _settings(molecule-atoms-visible) $settings(-atomsvisible)
                SendCmd "molecule bonds $settings(-bondsvisible) $tag"
                set _settings(molecule-bonds-visible) $settings(-bondsvisible)
                SendCmd "molecule bstyle $settings(-bondstyle) $tag"
                set _settings(molecule-bondstyle) $settings(-bondstyle)
                SendCmd "molecule ascale $settings(-atomscale) $tag"
                set _settings(molecule-atomscale) $settings(-atomscale)
                SendCmd "molecule bscale $settings(-bondscale) $tag"
                set _settings(molecule-bondscale) $settings(-bondscale)
            }
            SendCmd "molecule labels $settings(-labels) $tag"
            set _settings(molecule-labels) $settings(-labels)
            SendCmd "molecule linecolor [Color2RGB $settings(-edgecolor)] $tag"
            SendCmd "molecule linewidth $settings(-linewidth) $tag"
            SendCmd "molecule edges $settings(-edges) $tag"
            set _settings(molecule-edges) $settings(-edges)
            SendCmd "molecule lighting $settings(-lighting) $tag"
            set _settings(molecule-lighting) $settings(-lighting)
            SendCmd "molecule aquality $settings(-quality) $tag"
            SendCmd "molecule bquality $settings(-quality) $tag"
            set _settings(molecule-quality) $settings(-quality)
            SendCmd "molecule visible $settings(-visible) $tag"
            set _settings(molecule-visible) $settings(-visible)
            set _haveMolecules 1
        }
        "polydata" {
            array set settings {
                -cloudstyle "mesh"
                -color white
                -edgecolor black
                -edges 1
                -lighting 1
                -linewidth 1.0
                -opacity 1.0
                -outline 0
                -visible 1
                -wireframe 0
            }
            array set settings $style

            SendCmd "outline add $tag"
            SendCmd "outline color [Color2RGB $settings(-color)] $tag"
            SendCmd "outline visible $settings(-outline) $tag"
            set _settings(polydata-outline) $settings(-outline)

            SendCmd "polydata add $tag"
            SendCmd "polydata visible $settings(-visible) $tag"
            set _settings(polydata-visible) $settings(-visible)
            SendCmd "polydata edges $settings(-edges) $tag"
            set _settings(polydata-edges) $settings(-edges)
            SendCmd "polydata cloudstyle $settings(-cloudstyle) $tag"
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

itcl::body Rappture::VtkViewer::IsValidObject { dataobj } {
    if {[catch {$dataobj isa Rappture::Drawing} valid] != 0 || !$valid} {
        return 0
    }
    return 1
}

# ----------------------------------------------------------------------
# USAGE: ReceiveLegend <colormap> <title> <min> <max> <size>
#
# Invoked automatically whenever the "legend" command comes in from
# the rendering server.  Indicates that binary image data with the
# specified <size> will follow.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::ReceiveLegend { colormap title min max size } {
    set _limits(vmin) $min
    set _limits(vmax) $max
    set _title $title
    if { [IsConnected] } {
        set bytes [ReceiveBytes $size]
        if { ![info exists _image(legend)] } {
            set _image(legend) [image create photo]
        }
        $_image(legend) configure -data $bytes
        DrawLegend
    }
}

#
# DrawLegend --
#
# Draws the legend in it's own canvas which resides to the right
# of the contour plot area.
#
itcl::body Rappture::VtkViewer::DrawLegend {} {
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]

    if { $_settings(legend) } {
        set x [expr $w - 2]
        if { [$c find withtag "legend"] == "" } {
            $c create image $x [expr {$lineht+2}] \
                -anchor ne \
                -image $_image(legend) -tags "colormap legend"
            $c create text $x 2 \
                -anchor ne \
                -fill $itk_option(-plotforeground) -tags "vmax legend" \
                -font $font
            $c create text $x [expr {$h-2}] \
                -anchor se \
                -fill $itk_option(-plotforeground) -tags "vmin legend" \
                -font $font
            #$c bind colormap <Enter> [itcl::code $this EnterLegend %x %y]
            $c bind colormap <Leave> [itcl::code $this LeaveLegend]
            $c bind colormap <Motion> [itcl::code $this MotionLegend %x %y]
        }
        # Reset the item coordinates according the current size of the plot.
        $c coords colormap $x [expr {$lineht+2}]
        if { $_limits(vmin) != "" } {
            $c itemconfigure vmin -text [format %g $_limits(vmin)]
        }
        if { $_limits(vmax) != "" } {
            $c itemconfigure vmax -text [format %g $_limits(vmax)]
        }
        $c coords vmin $x [expr {$h-2}]
        $c coords vmax $x 2
    }
}

#
# EnterLegend --
#
itcl::body Rappture::VtkViewer::EnterLegend { x y } {
    SetLegendTip $x $y
}

#
# MotionLegend --
#
itcl::body Rappture::VtkViewer::MotionLegend { x y } {
    Rappture::Tooltip::tooltip cancel
    set c $itk_component(view)
    SetLegendTip $x $y
}

#
# LeaveLegend --
#
itcl::body Rappture::VtkViewer::LeaveLegend { } {
    Rappture::Tooltip::tooltip cancel
    .rappturetooltip configure -icon ""
}

#
# SetLegendTip --
#
itcl::body Rappture::VtkViewer::SetLegendTip { x y } {
    set c $itk_component(view)
    set w [winfo width $c]
    set h [winfo height $c]
    set font "Arial 8"
    set lineht [font metrics $font -linespace]

    set imgHeight [image height $_image(legend)]
    set coords [$c coords colormap]
    set imgX [expr $w - [image width $_image(legend)] - 2]
    set imgY [expr $y - $lineht - 2]

    # Make a swatch of the selected color
    if { [catch { $_image(legend) get 10 $imgY } pixel] != 0 } {
        return
    }
    if { ![info exists _image(swatch)] } {
        set _image(swatch) [image create photo -width 24 -height 24]
    }
    set color [eval format "\#%02x%02x%02x" $pixel]
    $_image(swatch) put black  -to 0 0 23 23
    $_image(swatch) put $color -to 1 1 22 22
    .rappturetooltip configure -icon $_image(swatch)

    # Compute the value of the point
    set t [expr 1.0 - (double($imgY) / double($imgHeight-1))]
    set value [expr $t * ($_limits(vmax) - $_limits(vmin)) + $_limits(vmin)]
    set tipx [expr $x + 15]
    set tipy [expr $y - 5]
    Rappture::Tooltip::text $c "$_title $value"
    Rappture::Tooltip::tooltip show $c +$tipx,+$tipy
}

# ----------------------------------------------------------------------
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::Slice {option args} {
    switch -- $option {
        "move" {
            set axis [lindex $args 0]
            set newval [lindex $args 1]
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set newpos [expr {0.01*$newval}]
            SendCmd "renderer clipplane $axis $newpos -1"
        }
        "tooltip" {
            set axis [lindex $args 0]
            set val [$itk_component(${axis}CutScale) get]
            return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val%"
        }
        default {
            error "bad option \"$option\": should be axis, move, or tooltip"
        }
    }
}

itcl::body Rappture::VtkViewer::SetOrientation { side } {
    array set positions {
        front "1 0 0 0"
        back  "0 0 1 0"
        left  "0.707107 0 -0.707107 0"
        right "0.707107 0 0.707107 0"
        top   "0.707107 -0.707107 0 0"
        bottom "0.707107 0.707107 0 0"
    }
    foreach name { -qw -qx -qy -qz } value $positions($side) {
        set _view($name) $value
    }
    set q [ViewToQuaternion]
    $_arcball quaternion $q
    SendCmd "camera orient $q"
    SendCmd "camera reset"
    set _view(-xpan) 0
    set _view(-ypan) 0
    set _view(-zoom) 1.0
}

itcl::body Rappture::VtkViewer::SetOpacity { dataset } {
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
