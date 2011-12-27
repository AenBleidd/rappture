
# ----------------------------------------------------------------------
#  COMPONENT: vtkviewerobsolete - contour plot in a ResultSet
#
#  This widget is a contour plot for 2D meshes with a scalar value.
#  It is normally used in the ResultViewer to show results from the
#  run of a Rappture tool.  Use the "add" and "delete" methods to
#  control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require vtk
package require vtkinteraction
package require BLT
#package require Img

option add *VtkViewerObsolete.width 4i widgetDefault
option add *VtkViewerObsolete.height 4i widgetDefault
option add *VtkViewerObsolete.foreground black widgetDefault
option add *VtkViewerObsolete.controlBackground gray widgetDefault
option add *VtkViewerObsolete.controlDarkBackground #999999 widgetDefault
option add *VtkViewerObsolete.plotBackground black widgetDefault
option add *VtkViewerObsolete.plotForeground white widgetDefault
option add *VtkViewerObsolete.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VtkViewerObsolete {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""

    private variable _dlist ""     ;# list of data objects
    private variable _dims ""      ;# dimensionality of data objects
    private variable _obj2color    ;# maps dataobj => plotting color
    private variable _obj2width    ;# maps dataobj => line width
    private variable _obj2raise    ;# maps dataobj => raise flag 0/1
    private variable _dataobj2vtk  ;# maps dataobj => vtk objects
    private variable _actors       ;# array of actors for each dataobj.
    private variable _lights       ;# list of lights for each renderer
    private variable _click        ;# info used for _move operations
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view
    private variable _download ""  ;# snapshot for download

    private variable _renderer "";
    private variable _window "";
    private variable _interactor "";
    private variable _style "";
    private variable _light "";
    private variable _cubeAxesActor ""
    private variable _axesActor ""
    private variable _axesWidget "";
    private variable _settings
    constructor {args} { 
        # defined below 
    }
    destructor { 
        # defined below 
    }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { 
        # do nothing 
    }
    public method download {option args}

    protected method Rebuild {}
    protected method Clear {}
    protected method Zoom {option}
    protected method Move {option x y}
    protected method _3dView {theta phi}
    protected method _fixLimits {}
    protected method _color2rgb {color}
    protected method SetActorProperties { actor style } 

    private method ComputeLimits { args }
    private method GetLimits {}
    private method BuildCameraTab {}
    private method UpdateCameraInfo {}
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    protected method FixSettings {what {value ""}}

}

itk::usual VtkViewerObsolete {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no
    set _view(theta) 0
    set _view(phi) 0
    
    array set _limits {
        xMin    0 
        xMax    1
        yMin    0
        yMax    1
        zMin    0
        zMax    1
        vMin    0
        vMax    1
    }


    foreach { key value } {
        edges           1
        axes            1
        smallaxes       0
        wireframe       0
    } {
        set _settings($this-$key) $value
    }
    itk_component add main {
        Rappture::SidebarFrame $itk_interior.main
    }
    pack $itk_component(main) -expand yes -fill both
    set f [$itk_component(main) component frame]
    
    itk_component add controls {
        frame $f.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

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
            -command [itcl::code $this Zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) \
        "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomin] \
            -command [itcl::code $this Zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    pack $itk_component(zoomin) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"
    
    itk_component add zoomout {
        button $itk_component(zoom).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomout] \
            -command [itcl::code $this Zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"
    
    #
    # RENDERING AREA
    #
    itk_component add area {
        frame $f.area
    }
    pack $itk_component(area) -expand yes -fill both
    
    set _renderer [vtkRenderer $this-Renderer]
    set _window [vtkRenderWindow $this-RenderWindow]
    itk_component add plot {
        vtkTkRenderWidget $itk_component(area).plot -rw $_window \
            -width 1 -height 1
    } {
        # empty
    }
    pack $itk_component(plot) -expand yes -fill both
    $_window AddRenderer $_renderer
    $_window LineSmoothingOn
    $_window PolygonSmoothingOn
    
    set _interactor [vtkRenderWindowInteractor $this-Interactor]
    set _style [vtkInteractorStyleTrackballCamera $this-InteractorStyle]
    $_interactor SetRenderWindow $_window
    $_interactor SetInteractorStyle $_style
    $_interactor Initialize
    
    set _cubeAxesActor [vtkCubeAxesActor $this-CubeAxesActor]
    $_cubeAxesActor SetCamera [$_renderer GetActiveCamera]
    $_renderer AddActor $_cubeAxesActor
    
    # Supply small axes guide.
    set _axesActor [vtkAxesActor $this-AxesActor]
    set _axesWidget [vtkOrientationMarkerWidget $this-AxesWidget]
    $_axesWidget SetOrientationMarker $_axesActor
    $_axesWidget SetInteractor $_interactor
    $_axesWidget SetEnabled $_settings($this-smallaxes)
    $_axesWidget SetInteractive 0
    $_axesWidget SetViewport .7 0 1.0 0.3
    
    BuildViewTab
    BuildCameraTab
    
    set v0 0
    set v1 1
    set _lookup [vtkLookupTable $this-Lookup]
    $_lookup SetTableRange $v0 $v1
    $_lookup SetHueRange 0.66667 0.0
    $_lookup Build
    
    set lightKit [vtkLightKit $this-LightKit]
    $lightKit AddLightsToRenderer $_renderer
    
    #
    # Create a picture for download snapshots
    #
    set _download [image create photo]
    
    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::destructor {} {
    Clear
    after cancel [itcl::code $this Rebuild]
    
    foreach c [info commands $this-vtk*] {
        rename $c ""
    }
    image delete $_download
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::add {dataobj {settings ""}} {
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

        set _obj2color($dataobj) $params(-color)
        set _obj2width($dataobj) $params(-width)
        set _obj2raise($dataobj) $params(-raise)
        
        after cancel [itcl::code $this Rebuild]
        after idle [itcl::code $this Rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist
    foreach obj $dlist {
        if {[info exists _obj2raise($obj)] && $_obj2raise($obj)} {
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
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set i [lsearch -exact $_dlist $dataobj]
        if {$i >= 0} {
            set _dlist [lreplace $_dlist $i $i]
            catch {unset _obj2color($dataobj)}
            catch {unset _obj2width($dataobj)}
            catch {unset _obj2raise($dataobj)}
            foreach actor $_actors($dataobj) {
                $_renderer RemoveActor $actor
            }
            array unset _actors $dataobj
            array unset _dataobj2vtk $dataobj-*
            set changed 1
        }
    }
    # If anything changed, then rebuild the plot
    if {$changed} {
        after cancel [itcl::code $this Rebuild]
        after idle [itcl::code $this Rebuild]
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
itcl::body Rappture::VtkViewerObsolete::scale {args} {
    eval ComputeLimits $args 
    _fixLimits
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
itcl::body Rappture::VtkViewerObsolete::download {option args} {
    switch $option {
        coming {
            if {[catch {
                blt::winop snap $itk_component(plotarea) $_download
            }]} {
                $_download configure -width 1 -height 1
                $_download put #000000
            }
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            set writer [vtkJPEGWriter $this-vtkJPEGWriter]
            set large [vtkRenderLargeImage $this-RenderLargeImage]
            $_axesWidget SetEnabled 0
            $large SetInput $_renderer
            $large SetMagnification 4
            $writer SetInputConnection [$large GetOutputPort]

            $writer SetFileName junk.jpg
            $writer Write 
            rename $writer ""
            rename $large ""
            FixSettings smallaxes

            set img [image create photo -file junk.jpg]
            set bytes [$img data -format "jpeg -quality 100"]
            set bytes [Rappture::encoding::decode -as b64 $bytes]
            image delete $img
            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Clear
#
# Used internally to clear the drawing area and tear down all vtk
# objects in the current scene.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::Clear {} {
    # clear out any old constructs
    
    foreach ren [array names _lights] {
        foreach light $_lights($ren) {
            $ren RemoveLight $light
            rename $light ""
        }
        set _lights($ren) ""
    }
    foreach dataobj $_dlist {
        foreach actor $_actors($dataobj) {
            $_renderer RemoveActor $actor
        }
    }
    array unset _actors
    set _dlist ""
    array unset _dataobj2vtk
}

# ----------------------------------------------------------------------
# USAGE: Zoom in
# USAGE: Zoom out
# USAGE: Zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::Zoom {option} {
    set cam [$_renderer GetActiveCamera]
    switch -- $option {
        in {
            $cam Zoom 1.25
            $_window Render
        }
        out {
            $cam Zoom 0.8
            $_window Render
        }
        reset {
            $cam SetViewAngle 30
            $_renderer ResetCamera
            _3dView 90 -90
            array set camera {
                xpos 1.73477e-06 ypos 74.7518 zpos -1.73477e-06
                xviewup 5.38569e-16 yviewup 2.32071e-08 zviewup 1.0
                xfocal 0.0 yfocal 0.0 zfocal 0.0 
                angle 30
            }
            set dataobj [lindex $_dlist end]
            if { $dataobj != "" } {
                array set camera [$dataobj hints camera]
            }
            if { [info exists camera(clipmin)] } {
                $cam SetClippingRange $camera(clipmin) $camera(clipmax)
            }
            if { [info exists camera(parallelscale)] } {
                $cam SetParallelScale $camera(parallelscale) 
            }
            $cam SetViewAngle $camera(angle)
            $cam SetFocalPoint $camera(xfocal) $camera(yfocal) $camera(zfocal)
            $cam SetPosition $camera(xpos) $camera(ypos) $camera(zpos)
            $cam SetViewUp $camera(xviewup) $camera(yviewup) $camera(zviewup)
            foreach key [array names camera] {
                set _settings($this-$key) $camera($key)
            }
            $cam ComputeViewPlaneNormal
            $_window Render
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Move click <x> <y>
# USAGE: Move drag <x> <y>
# USAGE: Move release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::Move {option x y} {
    switch -- $option {
        click {
            blt::busy configure $itk_component(area) -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
        }
        drag {
            if {[array size _click] == 0} {
                Move click $x $y
            } else {
                set w [winfo width $itk_component(plot)]
                set h [winfo height $itk_component(plot)]
                set scalex [expr {$_limits(xMax)-$_limits(xMin)}]
                set scaley [expr {$_limits(yMax)-$_limits(yMin)}]
                set dx [expr {double($x-$_click(x))/$w*$scalex}]
                set dy [expr {double($y-$_click(y))/$h*$scaley}]

                if {$_dims == "2D"} {
                    #
                    # Shift the contour plot in 2D
                    #
                    foreach dataobj $_dlist {
                        foreach actor $_actors($dataobj) {
                            foreach {ax ay az} [$actor GetPosition] break
                            $actor SetPosition [expr {$ax+$dx}] \
                                [expr {$ay-$dy}] 0
                        }
                    }
                    $_window Render
                } elseif {$_dims == "3D"} {
                    #
                    # Rotate the camera in 3D
                    #
                    set theta [expr {$_view(theta) - $dy*180}]
                    if {$theta < 2} { set theta 2 }
                    if {$theta > 178} { set theta 178 }
                    set phi [expr {$_view(phi) - $dx*360}]
                    
                    _3dView $theta $phi
                    $_window Render
                }
                set _click(x) $x
                set _click(y) $y
            }
        }
        release {
            Move drag $x $y
            blt::busy configure $itk_component(area) -cursor left_ptr
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
    UpdateCameraInfo
}


# ----------------------------------------------------------------------
# USAGE: _3dView <theta> <phi>
#
# Used internally to change the position of the camera for 3D data
# sets.  Sets the camera according to the angles <theta> (angle from
# the z-axis) and <phi> (angle from the x-axis in the x-y plane).
# Both angles are in degrees.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::_3dView {theta phi} {
    return
    set deg2rad 0.0174532927778
    set xn [expr {sin($theta*$deg2rad)*cos($phi*$deg2rad)}]
    set yn [expr {sin($theta*$deg2rad)*sin($phi*$deg2rad)}]
    set zn [expr {cos($theta*$deg2rad)}]
    
    set xm [expr {0.5*($_limits(xMax)+$_limits(xMin))}]
    set ym [expr {0.5*($_limits(yMax)+$_limits(yMin))}]
    set zm [expr {0.5*($_limits(zMax)+$_limits(zMin))}]
    
    set cam [$_renderer GetActiveCamera]
    set zoom [$cam GetViewAngle]
    $cam SetViewAngle 30
    $cam SetFocalPoint $xm $ym $zm
    $cam SetPosition [expr {$xm-$xn}] [expr {$ym-$yn}] [expr {$zm+$zn}]
    $cam ComputeViewPlaneNormal
    $cam SetViewUp 0 0 1  ;# z-dir is up
    $cam OrthogonalizeViewUp
    $_renderer ResetCamera
    $cam SetViewAngle $zoom
    
    set _view(theta) $theta
    set _view(phi) $phi
}

# ----------------------------------------------------------------------
# USAGE: _fixLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::_fixLimits {} {
    $_renderer ResetCamera
    set camera [$_renderer GetActiveCamera]
    $camera Zoom 1.5
    $_window Render
    if 0 {
        $this-vtkRenderWindow2 Render
    }
}

# ----------------------------------------------------------------------
# USAGE: _color2rgb <color>
#
# Used internally to convert a color name to a set of {r g b} values
# needed for vtk.  Each r/g/b component is scaled in the range 0-1.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::_color2rgb {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] break
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewerObsolete::plotbackground {
    foreach {r g b} [_color2rgb $itk_option(-plotbackground)] break
    $_renderer SetBackground $r $g $b
    $_window Render
    if 0 {
        $this-vtkRenderer2 SetBackground $r $g $b
        $this-vtkRenderWindow2 Render
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewerObsolete::plotforeground {
    after cancel [itcl::code $this Rebuild]
    after idle [itcl::code $this Rebuild]
}

itcl::body Rappture::VtkViewerObsolete::SetActorProperties { actor style } {
    array set props {
        -color \#6666FF
        -edgevisibility yes
        -edgecolor black
        -linewidth 1.0
        -opacity 1.0
    }
    # Parse style string.
    array set props $style
    set prop [$actor GetProperty]
    eval $prop SetColor [_color2rgb $props(-color)]
    if { $props(-edgevisibility) } {
        $prop EdgeVisibilityOn
    } else {
        $prop EdgeVisibilityOff
    }
    set _settings($this-edges) $props(-edgevisibility)
    eval $prop SetEdgeColor [_color2rgb $props(-edgecolor)]
    $prop SetLineWidth $props(-linewidth)
    $prop SetOpacity $props(-opacity)
    set _settings($this-opacity) [expr $props(-opacity) * 100.0]
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::Rebuild {} {
    set id 0
    
    # determine the dimensionality from the topmost (raised) object
    set dlist [get]
    set dataobj [lindex $dlist end]
    if {$dataobj != ""} {
        set _dims [lindex [lsort [$dataobj components -dimensions]] end]
    } else {
        set _dims "0D"
    }
    ComputeLimits
    $_cubeAxesActor SetCamera [$_renderer GetActiveCamera]
    eval $_cubeAxesActor SetBounds [GetLimits]
    
    if 1 {
        #
        # LOOKUP TABLE FOR COLOR CONTOURS
        #
        # Use vmin/vmax if possible, otherwise get from data
        if {$_limits(vMin) == "" || $_limits(vMax) == ""} {
            set v0 0
            set v1 1
            if { [info exists _dataobj2vtk($dataobj)] } {
                set pd [lindex $_dataobj2vtk($dataobj) 0]
                if {"" != $pd} {
                    foreach {v0 v1} [$pd GetScalarRange] break
                }
            }
        } else {
            set v0 $_limits(vMin)
            set v1 $_limits(vMax)
        }
    }   
    # scan through all data objects and build the contours
    set firstobj 1
    foreach dataobj $_dlist {
        foreach comp [$dataobj components] {
            set tag $dataobj-$comp
            if { ![info exists _dataobj2vtk($tag)] } {
                set actor [$dataobj values $comp]
                set style [$dataobj style $comp]
                set _dataobj2vtk($tag) $actor
                lappend _actors($dataobj) $actor
                $_renderer AddActor $actor
                SetActorProperties $actor $style
                incr id
            }
        }
        set firstobj 0
    }
    set top [lindex [get] end]
    if { $top != "" } {
        foreach axis { x y z } {
            set title [$top hints ${axis}label]
            set units [$top hints ${axis}units]
            set method Set[string toupper $axis]Title
            set label "$title"
            if { $units != "" } {
                append label " ($units)"
            }       
            $_cubeAxesActor $method $label
        }
    }
    if 1 {
        _fixLimits
        Zoom reset

    }
    $_interactor Start
    $_window Render
    return

    #
    # HACK ALERT!  A single ResetCamera doesn't seem to work for
    #   some contour data.  You have to do it multiple times to
    #   get to the right zoom factor on data.  I hope 20 times is
    #   enough.  I hate Vtk sometimes...
    #
    for {set i 0} {$i < 20} {incr i} {
        $_renderer ResetCamera
        [$_renderer GetActiveCamera] Zoom 1.5
    }
    # prevent interactions -- use our own
    blt::busy hold $itk_component(area) -cursor left_ptr
    bind $itk_component(area)_Busy <ButtonPress> \
        [itcl::code $this Move click %x %y]
    bind $itk_component(area)_Busy <B1-Motion> \
        [itcl::code $this Move drag %x %y]
    bind $itk_component(area)_Busy <ButtonRelease> \
        [itcl::code $this Move release %x %y]
}

itcl::body Rappture::VtkViewerObsolete::BuildViewTab {} {

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set tab [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    set inner $tab
    if 0 {
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    pack $tab.ss -fill both -expand yes 
    blt::tk::scrollbar $tab.ss.xs               
    blt::tk::scrollbar $tab.ss.ys               
    set inner [blt::tk::frame $tab.ss.frame]
    $inner configure -borderwidth 4
    }
    set ::Rappture::VtkViewerObsolete::_settings($this-isosurface) 0
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

    checkbutton $inner.edges \
        -text "Edges" \
        -variable [itcl::scope _settings($this-edges)] \
        -command [itcl::code $this FixSettings edges] \
        -font "Arial 9"

    checkbutton $inner.smallaxes \
        -text "Small Axes" \
        -variable [itcl::scope _settings($this-smallaxes)] \
        -command [itcl::code $this FixSettings smallaxes] \
        -font "Arial 9"

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings($this-wireframe)] \
        -command [itcl::code $this FixSettings wireframe] \
        -font "Arial 9"

    blt::table $inner \
        0,0 $inner.axes  -columnspan 2 -anchor w \
        1,0 $inner.edges  -columnspan 2 -anchor w \
        2,0 $inner.wireframe  -columnspan 2 -anchor w \
        3,0 $inner.smallaxes  -columnspan 2 -anchor w 

    blt::table configure $inner r* -resize none
    blt::table configure $inner r5 -resize expand
}

itcl::body Rappture::VtkViewerObsolete::BuildVolumeTab {} {
    foreach { key value } {
        light           40
        transp          50
        opacity         1000
    } {
        set _settings($this-$key) $value
    }

    set tab [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    set inner $tab
    if 0 {
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    pack $tab.ss -fill both -expand yes 
    blt::tk::scrollbar $tab.ss.xs               
    blt::tk::scrollbar $tab.ss.ys               
    set inner [blt::tk::frame $tab.ss.frame]
    $inner configure -borderwidth 4
    }
    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    checkbutton $inner.vol -text "Show volume" -font $fg \
        -variable [itcl::scope _settings($this-volume)] \
        -command [itcl::code $this FixSettings volume]
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
        4,3 $inner.opaque -anchor w -pady 2 

    blt::table configure $inner c0 c1 c3 r* -resize none
    blt::table configure $inner r6 -resize expand
}

itcl::body Rappture::VtkViewerObsolete::UpdateCameraInfo {} {
    set cam [$_renderer GetActiveCamera]
    foreach key { x y z } \
            pt  [$cam GetFocalPoint] \
            up  [$cam GetViewUp] \
            pos [$cam GetPosition] {
        set _settings($this-${key}focal) $pt
        set _settings($this-${key}up) $up
        set _settings($this-${key}pos) $pos
    }
    foreach {min max} [$cam GetClippingRange] break
    set _settings($this-clipmin) $min
    set _settings($this-clipmax) $max
    set _settings($this-parallelscale) [$cam GetParallelScale]
    set _settings($this-angle) [$cam GetViewAngle]
    foreach key { xpos ypos zpos xviewup yviewup zviewup 
        xfocal yfocal zfocal angle clipmin clipmax parallelscale
    } {
        set out($key) $_settings($this-$key)
    }
    puts \"[array get out]\"
}

itcl::body Rappture::VtkViewerObsolete::BuildCameraTab {} {
    set inner [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]] 
    $inner configure -borderwidth 4
    bind $inner <Map> [itcl::code $this UpdateCameraInfo]

    label $inner.xposl -text "Position"
    entry $inner.xpos -bg white \
        -textvariable [itcl::scope _settings($this-xpos)]
    entry $inner.ypos -bg white \
        -textvariable [itcl::scope _settings($this-ypos)]
    entry $inner.zpos -bg white \
        -textvariable [itcl::scope _settings($this-zpos)]
    label $inner.xviewupl -text "View Up"
    entry $inner.xviewup -bg white \
        -textvariable [itcl::scope _settings($this-xviewup)]
    entry $inner.yviewup -bg white \
        -textvariable [itcl::scope _settings($this-yviewup)]
    entry $inner.zviewup -bg white \
        -textvariable [itcl::scope _settings($this-zviewup)]
    label $inner.xfocall -text "Focal Point"
    entry $inner.xfocal -bg white \
        -textvariable [itcl::scope _settings($this-xfocal)]
    entry $inner.yfocal -bg white \
        -textvariable [itcl::scope _settings($this-yfocal)]
    entry $inner.zfocal -bg white \
        -textvariable [itcl::scope _settings($this-zfocal)]
    label $inner.anglel -text "View Angle"
    entry $inner.angle -bg white \
        -textvariable [itcl::scope _settings($this-angle)]
    label $inner.clipl -text "Clipping Range"
    entry $inner.clipmin -bg white \
        -textvariable [itcl::scope _settings($this-clipmin)]
    entry $inner.clipmax -bg white \
        -textvariable [itcl::scope _settings($this-clipmax)]
    label $inner.pscalel -text "Parallel Scale"
    entry $inner.pscale -bg white \
        -textvariable [itcl::scope _settings($this-parallelscale)]

    button $inner.refresh -text "Refresh" \
        -command [itcl::code $this UpdateCameraInfo]
    blt::table $inner \
        0,0 $inner.xposl -anchor w -pady 2 \
        1,0 $inner.xpos -pady 2 -fill x\
        2,0 $inner.ypos -pady 2 -fill x\
        3,0 $inner.zpos -pady 2 -fill x\
        4,0 $inner.xviewupl -anchor w -pady 2 \
        5,0 $inner.xviewup -pady 2 -fill x \
        6,0 $inner.yviewup -pady 2 -fill x \
        7,0 $inner.zviewup -pady 2 -fill x \
        8,0 $inner.xfocall -anchor w -pady 2 \
        9,0 $inner.xfocal -pady 2 -fill x \
        10,0 $inner.yfocal -pady 2 -fill x \
        11,0 $inner.zfocal -pady 2 -fill x \
        16,0 $inner.anglel -anchor w -pady 2 \
        17,0 $inner.angle -pady 2 -fill x \
        18,0 $inner.clipl -anchor w -pady 2 \
        19,0 $inner.clipmin -pady 2 -fill x \
        20,0 $inner.clipmax -pady 2 -fill x \
        21,0 $inner.pscalel -anchor w -pady 2 \
        22,0 $inner.pscale -pady 2 -fill x \
        23,0 $inner.refresh 

    blt::table configure $inner r* c* -resize none
    blt::table configure $inner c0 -resize expand
    blt::table configure $inner r24 -resize expand
}

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewerObsolete::FixSettings {what {value ""}} {
    switch -- $what {
        light {
        }
        transp {
        }
        opacity {
            set new [expr $_settings($this-opacity) * 0.01]
            foreach dataobj [get] {
                foreach comp [$dataobj components] {
                    set actor [$dataobj values $comp]
                    set prop [$actor GetProperty]
                    $prop SetOpacity $new
                }
            }
            $_window Render
        }

        "wireframe" {
            foreach dataobj [get] {
                foreach comp [$dataobj components] {
                    set actor [$dataobj values $comp]
                    set prop [$actor GetProperty]
                    if { $_settings($this-wireframe) } {
                        $prop SetRepresentationToWireframe
                    } else {
                        $prop SetRepresentationToSurface
                    }
                }
            }
            $_window Render
        }
        "isosurface" {
        }
        "edges" {
            foreach dataobj [get] {
                foreach comp [$dataobj components] {
                    set actor [$dataobj values $comp]
                    set prop [$actor GetProperty]
                    if { $_settings($this-edges) } {
                        $prop EdgeVisibilityOn
                    } else {
                        $prop EdgeVisibilityOff
                    }
                }
            }
            $_window Render
        }
        "axes" {
            if { $_settings($this-axes) } {
                $_cubeAxesActor VisibilityOn
            } else {
                $_cubeAxesActor VisibilityOff
            }
            $_window Render
        }
        "smallaxes" {
            $_axesWidget SetEnabled $_settings($this-smallaxes)
            $_window Render
        }
        default {
            error "don't know how to fix $what"
        }
    }
}

itcl::body Rappture::VtkViewerObsolete::GetLimits {} {
    return [list $_limits(xMin) $_limits(xMax) \
                $_limits(yMin) $_limits(yMax) \
                $_limits(zMin) $_limits(zMax)]
}

itcl::body Rappture::VtkViewerObsolete::ComputeLimits { args } {
    array set _limits {
        xMin 0 
        xMax 1
        yMin 0
        yMax 1
        zMin 0
        zMax 1
        vMin 0
        vMax 1
    }
    set actors {}
    if { [llength $args] > 0 } {
        foreach dataobj $args {
            foreach comp [$dataobj components] {
                lappend actors [$dataobj values $comp]
            } 
        }
    } else {
        foreach dataobj [get] {
            foreach comp [$dataobj components] {
                lappend actors [$dataobj values $comp]
            } 
        }
    }
    if { [llength $actors] == 0 } {
        return
    }
    set actor [lindex $actors 0]
    foreach key { xMin xMax yMin yMax zMin zMax} value [$actor GetBounds] {
        set _limits($key) $value
    }
    foreach actor [lrange $actors 1 end] {
        foreach { xMin xMax yMin yMax zMin zMax} [$actor GetBounds] break
        if { $xMin < $_limits(xMin) } {
            set _limits(xMin) $xMin
        } 
        if { $xMax > $_limits(xMax) } {
            set _limits(xMax) $xMax
        } 
        if { $yMin < $_limits(yMin) } {
            set _limits(yMin) $yMin
        } 
        if { $yMax > $_limits(yMax) } {
            set _limits(yMax) $yMax
        } 
        if { $zMin < $_limits(zMin) } {
            set _limits(zMin) $zMin
        } 
        if { $zMax > $_limits(zMax) } {
            set _limits(zMax) $zMax
        } 
    }
    set _limits(vMin) $_limits(zMin)
    set _limits(vMax) $_limits(zMax)
}
