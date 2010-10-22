
# ----------------------------------------------------------------------
#  COMPONENT: contourresult - contour plot in a ResultSet
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

option add *VtkViewer.width 4i widgetDefault
option add *VtkViewer.height 4i widgetDefault
option add *VtkViewer.foreground black widgetDefault
option add *VtkViewer.controlBackground gray widgetDefault
option add *VtkViewer.controlDarkBackground #999999 widgetDefault
option add *VtkViewer.plotBackground black widgetDefault
option add *VtkViewer.plotForeground white widgetDefault
option add *VtkViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::VtkViewer {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""

    private variable _dlist ""     ;# list of data objects
    private variable _dims ""      ;# dimensionality of data objects
    private variable _obj2color    ;# maps dataobj => plotting color
    private variable _obj2width    ;# maps dataobj => line width
    private variable _obj2raise    ;# maps dataobj => raise flag 0/1
    private variable _obj2vtk      ;# maps dataobj => vtk objects
    private variable _actors ""    ;# list of actors for each renderer
    private variable _lights       ;# list of lights for each renderer
    private variable _click        ;# info used for _move operations
    private variable _slicer       ;# vtk transform used for 3D slice plane
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
    protected method Slice {option args}
    protected method _3dView {theta phi}
    protected method _fixLimits {}
    protected method Slicertip {axis}
    protected method _color2rgb {color}
    protected method SetActorProperties { actor style } 

    private method ComputeLimits { args }
    private method GetLimits {}
    private method BuildCameraTab {}
    private method BuildCutplanesTab {}
    private method BuildViewTab {}
    private method BuildVolumeTab {}
    protected method FixSettings {what {value ""}}

}

itk::usual VtkViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    set _slicer(xplane) ""
    set _slicer(yplane) ""
    set _slicer(zplane) ""
    set _slicer(xslice) ""
    set _slicer(yslice) ""
    set _slicer(zslice) ""
    set _slicer(readout) ""
    set _view(theta) 0
    set _view(phi) 0

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
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(zoom).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap [Rappture::icon zoomin] \
            -command [itcl::code $this Zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
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
        rename -highlightbackground -controlbackground controlBackground Background
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
    $_axesWidget SetEnabled 1
    $_axesWidget SetInteractive 0
    $_axesWidget SetViewport .7 0 1.0 0.3

    $_cubeAxesActor SetXTitle "X Axis (units)"
    $_cubeAxesActor SetYTitle "Y Axis (units)"

    BuildViewTab
    BuildVolumeTab
    BuildCutplanesTab
    BuildCameraTab

    set v0 0
    set v1 1
    set _lookup [vtkLookupTable $this-Lookup]
    $_lookup SetTableRange $v0 $v1
    $_lookup SetHueRange 0.66667 0.0
    $_lookup Build


    #
    # 3D CUT PLANES
    #
    foreach axis {x y z} norm {{1 0 0} {0 1 0} {0 0 1}} {

        set plane [vtkPlane $this-${axis}cutplane]
        eval $plane SetNormal $norm
        set _slicer(${axis}plane) $plane
        
        set cutter [vtkCutter $this-${axis}cutter]
        $cutter SetCutFunction $plane
        
        set mapper [vtkPolyDataMapper  $this-${axis}cutmapper]
        $mapper SetInput [$cutter GetOutput]
        $mapper SetScalarRange $v0 $v1
        $mapper SetLookupTable $_lookup
        
        #lappend _obj2vtk($dataobj) $cutplane $cutter $mapper
        
        set actor [vtkActor $this-${axis}actor]
        $actor VisibilityOn
        $actor SetMapper $mapper
        $actor SetPosition 0 0 0
        [$actor GetProperty] SetColor 0 0 0
        set _slicer(${axis}slice) $actor
        
        $_renderer AddActor $actor
        lappend _actors $actor
        #lappend _obj2vtk($dataobj) $actor
    }
    
    #
    # CUT PLANE READOUT
    #
    set mapper [vtkTextMapper $this-text]
    set props [$mapper GetTextProperty]
    #eval $props SetColor [_color2rgb $itk_option(-plotforeground)]
    $props SetVerticalJustificationToTop
    set _slicer(readout) $mapper
            
    set actor [vtkActor2D $this-texta]
    $actor SetMapper $mapper
    set coords [$actor GetPositionCoordinate] 
    $coords SetCoordinateSystemToNormalizedDisplay
    $coords SetValue 0.02 0.98
            
    $_renderer AddActor $actor
    lappend _actors $actor
    #lappend _obj2vtk($dataobj) $mapper $actor

    # turn off all slicers by default
    foreach axis {x y z} {
        $itk_component(${axis}CutScale) configure -state normal
        $itk_component(${axis}CutScale) set 50
        Slice move $axis 50
        Slice axis $axis off
    }

    set lightKit [vtkLightKit $this-LightKit]
    $lightKit AddLightsToRenderer $_renderer

    #
    # Create a picture for download snapshots
    #
    set _download [image create picture]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::destructor {} {
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
itcl::body Rappture::VtkViewer::add {dataobj {settings ""}} {
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
itcl::body Rappture::VtkViewer::get {} {
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
itcl::body Rappture::VtkViewer::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _obj2color($dataobj)}
            catch {unset _obj2width($dataobj)}
            catch {unset _obj2raise($dataobj)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
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
itcl::body Rappture::VtkViewer::scale {args} {
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
itcl::body Rappture::VtkViewer::download {option args} {
    switch $option {
        coming {
            if {[catch {$_download snap $itk_component(area)} errs] != 0 } {
                global errorInfo
                puts stderr "$errorInfo"
                $_download blank #000000
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
            $_axesWidget SetEnabled 1

            $_download export jpg -quality 100 -data bytes
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
itcl::body Rappture::VtkViewer::Clear {} {
    # clear out any old constructs
    foreach actor $_actors {
        $_renderer RemoveActor $actor
    }
    set _actors ""
    foreach ren [array names _lights] {
        foreach light $_lights($ren) {
            $ren RemoveLight $light
            rename $light ""
        }
        set _lights($ren) ""
    }
    foreach dataobj [array names _obj2vtk] {
        foreach cmd $_obj2vtk($dataobj) {
            rename $cmd ""
        }
        set _obj2vtk($dataobj) ""
    }
    set _slicer(xplane) ""
    set _slicer(yplane) ""
    set _slicer(zplane) ""
    set _slicer(xslice) ""
    set _slicer(yslice) ""
    set _slicer(zslice) ""
    set _slicer(readout) ""
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
    set camera [$_renderer GetActiveCamera]
    switch -- $option {
        in {
            $camera Zoom 1.25
            $_window Render
        }
        out {
            $camera Zoom 0.8
            $_window Render
        }
        reset {
            $camera SetViewAngle 30
            $_renderer ResetCamera
            _3dView 90 -90
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
itcl::body Rappture::VtkViewer::Move {option x y} {
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
                    foreach actor $_actors {
                        foreach {ax ay az} [$actor GetPosition] break
                        $actor SetPosition [expr {$ax+$dx}] [expr {$ay-$dy}] 0
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
}

# ----------------------------------------------------------------------
# USAGE: Slice axis x|y|z ?on|off|toggle?
# USAGE: Slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::Slice {option args} {
    if {$_slicer(xplane) == ""} {
        # no slicer? then bail out!
        return
    }
    switch -- $option {
        axis {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"Slice axis x|y|z ?on|off|toggle?\""
            }
            set axis [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            set current $_settings($this-${axis}cutplane)
            if {$op == "toggle"} {
                set op [expr $current==0]
            }

            if {$op} {
                $itk_component(${axis}CutScale) configure -state normal
                $_slicer(${axis}slice) VisibilityOn
                $itk_component(${axis}CutButton) select
            } else {
                $itk_component(${axis}CutScale) configure -state disabled
                $_slicer(${axis}slice) VisibilityOff
                $itk_component(${axis}CutButton) deselect
            }
            $_window Render
        }
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"Slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set xm [expr {0.5*($_limits(xMax)+$_limits(xMin))}]
            set ym [expr {0.5*($_limits(yMax)+$_limits(yMin))}]
            set zm [expr {0.5*($_limits(zMax)+$_limits(zMin))}]

            set newval [expr {0.01*($newval-50)
                *($_limits(${axis}Max)-$_limits(${axis}Min))
                  + 0.5*($_limits(${axis}Max)+$_limits(${axis}Min))}]

            # show the current value in the readout
            if {$_slicer(readout) != ""} {
                $_slicer(readout) SetInput "$axis = $newval"
            }

            # keep a little inside the volume, or the slice will disappear!
            if {$newval == $_limits(${axis}Min)} {
                set range [expr {$_limits(${axis}Max)-$_limits(${axis}Min)}]
                set newval [expr {$newval + 1e-6*$range}]
            }

            # xfer new value to the proper dimension and move the cut plane
            set ${axis}m $newval
            $_slicer(${axis}plane) SetOrigin $xm $ym $zm

            $_window Render
        }
        default {
            error "bad option \"$option\": should be axis or move"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _3dView <theta> <phi>
#
# Used internally to change the position of the camera for 3D data
# sets.  Sets the camera according to the angles <theta> (angle from
# the z-axis) and <phi> (angle from the x-axis in the x-y plane).
# Both angles are in degrees.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::_3dView {theta phi} {
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
itcl::body Rappture::VtkViewer::_fixLimits {} {
    $_renderer ResetCamera
    set camera [$_renderer GetActiveCamera]
    $camera Zoom 1.5
    $_window Render
    if 0 {
    $this-vtkRenderWindow2 Render
    }
}

# ----------------------------------------------------------------------
# USAGE: Slicertip <axis>
#
# Used internally to generate a tooltip for the x/y/z slicer controls.
# Returns a message that includes the current slicer value.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::Slicertip {axis} {
    set val [$itk_component(${axis}slicer) get]
    set val [expr {0.01*($val-50)
        *($_limits(${axis}Max)-$_limits(${axis}Min))
          + 0.5*($_limits(${axis}Max)+$_limits(${axis}Min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val"
}

# ----------------------------------------------------------------------
# USAGE: _color2rgb <color>
#
# Used internally to convert a color name to a set of {r g b} values
# needed for vtk.  Each r/g/b component is scaled in the range 0-1.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::_color2rgb {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] break
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::VtkViewer::plotbackground {
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
itcl::configbody Rappture::VtkViewer::plotforeground {
    after cancel [itcl::code $this Rebuild]
    after idle [itcl::code $this Rebuild]
}

itcl::body Rappture::VtkViewer::SetActorProperties { actor style } {
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
itcl::body Rappture::VtkViewer::Rebuild {} {
    Clear
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
    puts stderr limits=[GetLimits]
    eval $_cubeAxesActor SetBounds [GetLimits]

    #
    # LOOKUP TABLE FOR COLOR CONTOURS
    #
    # use vmin/vmax if possible, otherwise get from data
    if {$_limits(vMin) == "" || $_limits(vMax) == ""} {
        set v0 0
        set v1 1
        if {[info exists _obj2vtk($dataobj)]} {
            set pd [lindex $_obj2vtk($dataobj) 0]
            if {"" != $pd} {
                foreach {v0 v1} [$pd GetScalarRange] break
            }
        }
    } else {
        set v0 $_limits(vMin)
        set v1 $_limits(vMax)
    }

    if 0 {
        set lu $this-vtkLookup
        vtkLookupTable $lu
        $lu SetTableRange $v0 $v1
        $lu SetHueRange 0.66667 0.0
        $lu Build
        
        lappend _obj2vtk($dataobj) $lu
        
        #
        # 3D LIGHTS (on both sides of all three axes)
        #
        set x0 $_limits(xMin)
        set x1 $_limits(xMax)
        set xm [expr {0.5*($x0+$x1)}]
        set y0 $_limits(yMin)
        set y1 $_limits(yMax)
        set ym [expr {0.5*($y0+$y1)}]
        set z0 $_limits(zMin)
        set z1 $_limits(zMax)
        set zm [expr {0.5*($z0+$z1)}]
        set xr [expr {$x1-$x0}]
        set yr [expr {$y1-$y0}]
        set zr [expr {$z1-$z0}]

        set light [vtkLight $this-vtkLight]
        $light SetColor 1 1 1
        $light SetAttenuationValues 0 0 0
        $light SetFocalPoint $xm $ym $zm
        $light SetLightTypeToHeadlight
        $_renderer AddLight $light
        lappend _lights($_renderer) $light
    }
    # scan through all data objects and build the contours
    set firstobj 1
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set actor [$dataobj values $comp]
            set style [$dataobj style $comp]

            $_renderer AddActor $actor
            SetActorProperties $actor $style
            set mapper [$dataobj data $comp]
            $this-xcutter SetInput [$mapper GetInput]
            #$_cubeAxesActor SetCamera [$_renderer GetActiveCamera]
            #eval $_cubeAxesActor SetBounds [$actor GetBounds]


            if 0 {
            #
            # Add color contours.
            #
            if {$firstobj} {
                if {$_dims == "3D"} {
                    pack $itk_component(slicers) -side bottom -padx 4 -pady 4
                    pack $itk_component(reset) -side left
                    pack $itk_component(zoomin) -side left
                    pack $itk_component(zoomout) -side left

                    #
                    # 3D DATA SET
                    #
                    set mesh [$dataobj mesh $comp]
                    if {"" == $mesh} {
                        set x [expr {[winfo rootx $itk_component(area)]+10}]
                        set y [expr {[winfo rooty $itk_component(area)]+10}]
                        Rappture::Tooltip::cue @$x,$y "This data requires the visualization server, and that appears to be down.  Please try your simulation again later."
                        return
                    }
                    switch -- [$mesh GetClassName] {
                      vtkPoints {
                        # handle cloud of 3D points
                        set pd $this-polydata
                        vtkPolyData $pd
                        $pd SetPoints $mesh
                        [$pd GetPointData] SetScalars [$dataobj values $comp]

                        set tr $this-triangles
                        vtkDelaunay3D $tr
                        $tr SetInput $pd
                        $tr SetTolerance 0.0000000000001
                        set source [$tr GetOutput]

                        set mp $this-mapper
                        vtkPolyDataMapper $mp

                        lappend _obj2vtk($dataobj) $pd $tr $mp
                      }
                      vtkUnstructuredGrid {
                        # handle 3D grid with connectivity
                        set gr $this-grdata
                        vtkUnstructuredGrid $gr
                        $gr ShallowCopy $mesh
                        [$gr GetPointData] SetScalars [$dataobj values $comp]
                        set source $gr

                        lappend _obj2vtk($dataobj) $gr
                      }
                      vtkRectilinearGrid {
                        # handle 3D grid with connectivity
                        set gr $this-grdata
                        vtkRectilinearGrid $gr
                        $gr ShallowCopy $mesh
                        [$gr GetPointData] SetScalars [$dataobj values $comp]
                        set source $gr

                        lappend _obj2vtk($dataobj) $gr
                      }
                      default {
                        error "don't know how to handle [$mesh GetClassName] data"
                      }
                    }

                    #
                    # 3D ISOSURFACES
                    #
                    set iso $this-iso
                    vtkContourFilter $iso
                      $iso SetInput $source

                    set mp $this-isomap
                    vtkPolyDataMapper $mp
                      $mp SetInput [$iso GetOutput]

                    set ac $this-isoactor
                    vtkActor $ac
                      $ac SetMapper $mp
                      [$ac GetProperty] SetOpacity 0.3
                      [$ac GetProperty] SetDiffuse 0.5
                      [$ac GetProperty] SetAmbient 0.7
                      [$ac GetProperty] SetSpecular 10.0
                      [$ac GetProperty] SetSpecularPower 200.0
                    $_renderer AddActor $ac

                    lappend _obj2vtk($dataobj) $iso $mp $ac
                    lappend _actors $ac

                    catch {unset style}
                    array set style [lindex [$dataobj components -style $comp] 0]
                    if {[info exists style(-color)]} {
                        $mp ScalarVisibilityOff  ;# take color from actor
                        eval [$ac GetProperty] SetColor [_color2rgb $style(-color)]
                    }

                    if {[info exists style(-opacity)]} {
                        [$ac GetProperty] SetOpacity $style(-opacity)
                    }

                    set levels 5
                    if {[info exists style(-levels)]} {
                        set levels $style(-levels)
                    }
                    if {$levels == 1} {
                        $iso SetValue 0 [expr {0.5*($v1-$v0)+$v0}]
                    } else {
                        $iso GenerateValues [expr {$levels+2}] $v0 $v1
                    }

                    #
                    # 3D CUT PLANES
                    #
                    if {$id == 0} {
                        foreach axis {x y z} norm {{1 0 0} {0 1 0} {0 0 1}} {
                            set pl $this-${axis}cutplane$id
                            vtkPlane $pl
                            eval $pl SetNormal $norm
                            set _slicer(${axis}plane) $pl

                            set ct $this-${axis}cutter$id
                            vtkCutter $ct
                            $ct SetInput $source
                            $ct SetCutFunction $pl

                            set mp $this-${axis}cutmapper$id
                            vtkPolyDataMapper $mp
                            $mp SetInput [$ct GetOutput]
                            $mp SetScalarRange $v0 $v1
                            $mp SetLookupTable $lu

                            lappend _obj2vtk($dataobj) $pl $ct $mp

                            set ac $this-${axis}actor$id
                            vtkActor $ac
                            $ac VisibilityOff
                            $ac SetMapper $mp
                            $ac SetPosition 0 0 0
                            [$ac GetProperty] SetColor 0 0 0
                            set _slicer(${axis}slice) $ac

                            $_renderer AddActor $ac
                            lappend _actors $ac
                            lappend _obj2vtk($dataobj) $ac
                        }

                        #
                        # CUT PLANE READOUT
                        #
                        set tx $this-text$id
                        vtkTextMapper $tx
                        set tp [$tx GetTextProperty]
                        eval $tp SetColor [_color2rgb $itk_option(-plotforeground)]
                        $tp SetVerticalJustificationToTop
                        set _slicer(readout) $tx

                        set txa $this-texta$id
                        vtkActor2D $txa
                        $txa SetMapper $tx
                        [$txa GetPositionCoordinate] \
                            SetCoordinateSystemToNormalizedDisplay
                        [$txa GetPositionCoordinate] SetValue 0.02 0.98

                        $_renderer AddActor $txa
                        lappend _actors $txa

                        lappend _obj2vtk($dataobj) $tx $txa

                        # turn off all slicers by default
                        foreach axis {x y z} {
                            $itk_component(${axis}slicer) configure -state normal
                            $itk_component(${axis}slicer) set 50
                            Slice move $axis 50
                            Slice axis $axis off
                        }
                    }

                } else {
                    pack forget $itk_component(slicers)
                    pack $itk_component(reset) -side top
                    pack $itk_component(zoomin) -side top
                    pack $itk_component(zoomout) -side top

                    set pd $this-polydata$id
                    vtkPolyData $pd
                    $pd SetPoints [$dataobj mesh $comp]
                    [$pd GetPointData] SetScalars [$dataobj values $comp]

                    set tr $this-triangles$id
                    vtkDelaunay2D $tr
                    $tr SetInput $pd
                    $tr SetTolerance 0.0000000000001
                    set source [$tr GetOutput]

                    set mp $this-mapper$id
                    vtkPolyDataMapper $mp
                    $mp SetInput $source
                    $mp SetScalarRange $v0 $v1
                    $mp SetLookupTable $lu

                    set ac $this-actor$id
                    vtkActor $ac
                    $ac SetMapper $mp
                    $ac SetPosition 0 0 0
                    [$ac GetProperty] SetColor 0 0 0
                    $_renderer AddActor $ac
                    lappend _actors $ac

                    lappend _obj2vtk($dataobj) $pd $tr $mp $ac
                }
            } else {
                #
                # Add color lines
                #
                set cf $this-clfilter$id
                vtkContourFilter $cf
                $cf SetInput $source
                $cf GenerateValues 20 $v0 $v1

                set mp $this-clmapper$id
                vtkPolyDataMapper $mp
                $mp SetInput [$cf GetOutput]
                $mp SetScalarRange $v0 $v1
                $mp SetLookupTable $lu

                set ac $this-clactor$id
                vtkActor $ac
                $ac SetMapper $mp
                [$ac GetProperty] SetColor 1 1 1
                $ac SetPosition 0 0 0
                $_renderer AddActor $ac
                lappend _actors $ac

                lappend _obj2vtk($dataobj) $cf $mp $ac
            }

            #
            # Add an outline around the data
            #
            if {$id == 0} {
                set olf $this-olfilter$id
                vtkOutlineFilter $olf
                $olf SetInput $source

                set olm $this-olmapper$id
                vtkPolyDataMapper $olm
                $olm SetInput [$olf GetOutput]

                set ola $this-olactor$id
                vtkActor $ola
                $ola SetMapper $olm
                eval [$ola GetProperty] SetColor [_color2rgb $itk_option(-plotforeground)]
                $_renderer AddActor $ola
                lappend _actors $ola

                lappend _obj2vtk($dataobj) $olf $olm $ola

                if {$_dims == "3D"} {
                    # pick a good scale factor for text
                    if {$xr < $yr} {
                        set tscale [expr {0.1*$xr}]
                    } else {
                        set tscale [expr {0.1*$yr}]
                    }

                    foreach {i axis px py pz rx ry rz} {
                        0  x   $xm   0   0   90   0   0
                        1  y     0 $ym   0   90 -90   0
                        2  z   $x1   0 $zm   90   0 -45
                    } {
                        set length "[expr {[set ${axis}1]-[set ${axis}0]}]"

                        set vtx $this-${axis}label$id
                        vtkVectorText $vtx
                        $vtx SetText "$axis"

                        set vmp $this-${axis}lmap$id
                        vtkPolyDataMapper $vmp
                        $vmp SetInput [$vtx GetOutput]

                        set vac $this-${axis}lact$id
                        vtkActor $vac
                        $vac SetMapper $vmp
                        $vac SetPosition [expr $px] [expr $py] [expr $pz]
                        $vac SetOrientation $rx $ry $rz
                        $vac SetScale $tscale
                        $_renderer AddActor $vac

                        lappend _obj2vtk($dataobj) $vtx $vmp $vac
                        lappend _actors $vac

                        $vmp Update
                        foreach {xx0 xx1 yy0 yy1 zz0 zz1} [$vac GetBounds] break
                        switch -- $axis {
                          x {
                            set dx [expr {-0.5*($xx1-$xx0)}]
                            set dy 0
                            set dz [expr {1.3*($zz0-$zz1)}]
                          }
                          y {
                            set dx 0
                            set dy [expr {0.5*($yy1-$yy0)}]
                            set dz [expr {$zz0-$zz1}]
                          }
                          z {
                            set dx [expr {0.2*$tscale}]
                            set dy $dx
                            set dz [expr {-0.5*($zz1-$zz0)}]
                          }
                        }
                        $vac AddPosition $dx $dy $dz
                    }
                }
            }

            #
            # Add a legend with the scale.
            #
            if {$id == 0} {
                set lg $this-legend$id
                vtkScalarBarActor $lg
                $lg SetLookupTable $lu
                [$lg GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
                [$lg GetPositionCoordinate] SetValue 0.1 0.1
                $lg SetOrientationToHorizontal
                $lg SetWidth 0.8
                $lg SetHeight 1.0

                set tp [$lg GetLabelTextProperty]
                eval $tp SetColor [_color2rgb $itk_option(-plotforeground)]
                $tp BoldOff
                $tp ItalicOff
                $tp ShadowOff
                #eval $tp SetShadowColor [_color2rgb gray]

                if 0 {
                $this-vtkRenderer2 AddActor2D $lg
                lappend _actors $lg
                lappend _obj2vtk($dataobj) $lg
                }
            }
            }
            incr id
        }
        set firstobj 0
    }
    _fixLimits
    Zoom reset
    $_interactor Start
    $_window Render

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

itcl::body Rappture::VtkViewer::BuildViewTab {} {
    foreach { key value } {
        edges		1
        axes		1
        wireframe	0
        volume		1
        legend		1
        particles	1
        lic		1
    } {
        set _settings($this-$key) $value
    }

    set fg [option get $itk_component(hull) font Font]
    #set bfg [option get $itk_component(hull) boldFont Font]

    set tab [$itk_component(main) insert end \
        -title "View Settings" \
        -icon [Rappture::icon wrench]]
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    pack $tab.ss -fill both -expand yes 
    blt::tk::scrollbar $tab.ss.xs		
    blt::tk::scrollbar $tab.ss.ys		
    set inner [blt::tk::frame $tab.ss.frame]
    $inner configure -borderwidth 4

    set ::Rappture::VtkViewer::_settings($this-isosurface) 0
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

    checkbutton $inner.wireframe \
        -text "Wireframe" \
        -variable [itcl::scope _settings($this-wireframe)] \
        -command [itcl::code $this FixSettings wireframe] \
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

    blt::table $inner \
        0,0 $inner.axes  -columnspan 2 -anchor w \
        1,0 $inner.edges  -columnspan 2 -anchor w \
        2,0 $inner.wireframe  -columnspan 2 -anchor w \
        3,0 $inner.volume  -columnspan 2 -anchor w \
        4,0 $inner.legend  -columnspan 2 -anchor w 

    if 0 {
    bind $inner <Map> [itcl::code $this GetVolumeInfo $inner]
    }
    blt::table configure $inner r* -resize none
    blt::table configure $inner r5 -resize expand
}

itcl::body Rappture::VtkViewer::BuildVolumeTab {} {
    foreach { key value } {
        light		40
        transp		50
        opacity		1000
    } {
        set _settings($this-$key) $value
    }

    set tab [$itk_component(main) insert end \
        -title "Volume Settings" \
        -icon [Rappture::icon volume-on]]
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    pack $tab.ss -fill both -expand yes 
    blt::tk::scrollbar $tab.ss.xs		
    blt::tk::scrollbar $tab.ss.ys		
    set inner [blt::tk::frame $tab.ss.frame]
    $inner configure -borderwidth 4

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

itcl::body Rappture::VtkViewer::BuildCutplanesTab {} {

    set tab [$itk_component(main) insert end \
        -title "Cutplane Settings" \
        -icon [Rappture::icon cutbutton]]
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    pack $tab.ss -fill both -expand yes 
    blt::tk::scrollbar $tab.ss.xs		
    blt::tk::scrollbar $tab.ss.ys		
    set inner [blt::tk::frame $tab.ss.frame]
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
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x]
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
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y]
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
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z]
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
        0,3 $itk_component(zCutScale) 

    blt::table configure $inner r0 r1 c* -resize none
    blt::table configure $inner r2 c4 -resize expand
    blt::table configure $inner c0 -width 2
    blt::table configure $inner c1 c2 c3 -padx 2
}

itcl::body Rappture::VtkViewer::BuildCameraTab {} {
    set tab [$itk_component(main) insert end \
        -title "Camera Settings" \
        -icon [Rappture::icon camera]]
    blt::scrollset $tab.ss \
        -xscrollbar $tab.ss.xs \
        -yscrollbar $tab.ss.ys \
        -window $tab.ss.frame
    blt::tk::scrollbar $tab.ss.xs		
    blt::tk::scrollbar $tab.ss.ys		
    pack $tab.ss -fill both -expand yes 
    set inner [blt::tk::frame $tab.ss.frame]
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

# ----------------------------------------------------------------------
# USAGE: FixSettings <what> ?<value>?
#
# Used internally to update rendering settings whenever parameters
# change in the popup settings panel.  Sends the new settings off
# to the back end.
# ----------------------------------------------------------------------
itcl::body Rappture::VtkViewer::FixSettings {what {value ""}} {
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
        }
        "xcutplane" - "ycutplane" - "zcutplane" {
            set axis [string range $what 0 0]
            set bool $_settings($this-$what)
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

if 0 {
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
        [itcl::code $this Slice axis x toggle]
    Rappture::Tooltip::for $itk_component(xslice) \
        "Toggle the X cut plane on/off"
    grid $itk_component(xslice) -row 0 -column 0 -sticky ew -padx 1

    itk_component add xslicer {
        ::scale $itk_component(slicers).xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move x]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(xslicer) -row 1 -column 0 -padx 1
    Rappture::Tooltip::for $itk_component(xslicer) \
        "@[itcl::code $this Slicertip x]"

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
        [itcl::code $this Slice axis y toggle]
    Rappture::Tooltip::for $itk_component(yslice) \
        "Toggle the Y cut plane on/off"
    grid $itk_component(yslice) -row 0 -column 1 -sticky ew -padx 1

    itk_component add yslicer {
        ::scale $itk_component(slicers).yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move y]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(yslicer) -row 1 -column 1 -padx 1
    Rappture::Tooltip::for $itk_component(yslicer) \
        "@[itcl::code $this Slicertip y]"

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
    grid $itk_component(zslice) -row 0 -column 2 -sticky ew -padx 1
    bind $itk_component(zslice) <ButtonPress> \
        [itcl::code $this Slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zslice) \
        "Toggle the Z cut plane on/off"

    itk_component add zslicer {
        ::scale $itk_component(slicers).zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this Slice move z]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(zslicer) -row 1 -column 2 -padx 1
    Rappture::Tooltip::for $itk_component(zslicer) \
        "@[itcl::code $this Slicertip z]"

}

itcl::body Rappture::VtkViewer::GetLimits {} {
    return [list $_limits(xMin) $_limits(xMax) \
                $_limits(yMin) $_limits(yMax) \
                $_limits(zMin) $_limits(zMax)]
}

itcl::body Rappture::VtkViewer::ComputeLimits { args } {
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
