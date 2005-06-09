# ----------------------------------------------------------------------
#  COMPONENT: contourresult - contour plot in a ResultSet
#
#  This widget is a contour plot for 2D meshes with a scalar value.
#  It is normally used in the ResultViewer to show results from the
#  run of a Rappture tool.  Use the "add" and "delete" methods to
#  control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require vtk
package require vtkinteraction
package require BLT

blt::bitmap define ContourResult-reset {
#define reset_width 12
#define reset_height 12
static unsigned char reset_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap define ContourResult-zoomin {
#define zoomin_width 12
#define zoomin_height 12
static unsigned char zoomin_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x11, 0x01, 0x11, 0x01, 0x7d, 0x01, 0x11, 0x01,
   0x11, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}

blt::bitmap define ContourResult-zoomout {
#define zoomout_width 12
#define zoomout_height 12
static unsigned char zoomout_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}

option add *ContourResult.width 4i widgetDefault
option add *ContourResult.height 4i widgetDefault
option add *ContourResult.foreground black widgetDefault
option add *ContourResult.controlBackground gray widgetDefault
option add *ContourResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::ContourResult {
    inherit itk::Widget

    itk_option define -foreground foreground Foreground ""
    itk_option define -background background Background ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}

    protected method _rebuild {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _fixLimits {}
    protected method _color2rgb {color}

    private variable _dlist ""     ;# list of data objects
    private variable _obj2color    ;# maps dataobj => plotting color
    private variable _obj2width    ;# maps dataobj => line width
    private variable _obj2raise    ;# maps dataobj => raise flag 0/1
    private variable _obj2vtk      ;# maps dataobj => vtk objects
    private variable _actors       ;# list of actors for each renderer
    private variable _click        ;# info used for _move operations
    private variable _xmin ""      ;# autoscale min for x-axis
    private variable _xmax ""      ;# autoscale max for x-axis
    private variable _ymin ""      ;# autoscale min for y-axis
    private variable _ymax ""      ;# autoscale max for y-axis
    private variable _zmin ""      ;# autoscale min for z-axis
    private variable _zmax ""      ;# autoscale max for z-axis

    private common _counter 0      ;# used for auto-generated names
}

itk::usual ContourResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add controls {
        frame $itk_interior.cntls
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side right -fill y

    itk_component add reset {
        button $itk_component(controls).reset \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ContourResult-reset \
            -command [itcl::code $this _zoom reset]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(reset) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(reset) "Reset the view to the default zoom level"

    itk_component add zoomin {
        button $itk_component(controls).zin \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ContourResult-zoomin \
            -command [itcl::code $this _zoom in]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomin) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomin) "Zoom in"

    itk_component add zoomout {
        button $itk_component(controls).zout \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap ContourResult-zoomout \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add area {
        frame $itk_interior.area
    }
    pack $itk_component(area) -expand yes -fill both

    vtkRenderer $this-ren
    vtkRenderWindow $this-renWin
    $this-renWin AddRenderer $this-ren
    vtkRenderWindowInteractor $this-iren
    $this-iren SetRenderWindow $this-renWin

    itk_component add plot {
        vtkTkRenderWidget $itk_component(area).plot -rw $this-renWin \
            -width 1 -height 1
    } {
    }
    pack $itk_component(plot) -expand yes -fill both


    vtkRenderer $this-ren2
    vtkRenderWindow $this-renWin2
    $this-renWin2 AddRenderer $this-ren2
    vtkRenderWindowInteractor $this-iren2
    $this-iren2 SetRenderWindow $this-renWin2

    itk_component add legend {
        vtkTkRenderWidget $itk_component(area).legend -rw $this-renWin2 \
            -width 1 -height 40
    } {
    }
    pack $itk_component(legend) -side bottom -fill x

    eval itk_initialize $args

    blt::busy hold $itk_component(area) -cursor left_ptr
    bind $itk_component(area)_Busy <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(area)_Busy <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(area)_Busy <ButtonRelease> \
        [itcl::code $this _move release %x %y]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::destructor {} {
    rename $this-renWin ""
    rename $this-ren ""
    rename $this-iren ""

    rename $this-renWin2 ""
    rename $this-ren2 ""
    rename $this-iren2 ""

    after cancel [itcl::code $this _rebuild]
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::add {dataobj {settings ""}} {
    array set params {
        -color black
        -width 1
        -linestyle solid
        -brightness 0
        -raise 0
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    set pos [lsearch -exact $dataobj $_dlist]
    if {$pos < 0} {
        lappend _dlist $dataobj
        set _obj2color($dataobj) $params(-color)
        set _obj2width($dataobj) $params(-width)
        set _obj2raise($dataobj) $params(-raise)

        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::get {} {
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
itcl::body Rappture::ContourResult::delete {args} {
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
        after cancel [itcl::code $this _rebuild]
        after idle [itcl::code $this _rebuild]
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
itcl::body Rappture::ContourResult::scale {args} {
    set _xmin ""
    set _xmax ""
    set _ymin ""
    set _ymax ""
    set _zmin ""
    set _zmax ""
    foreach obj $args {
        foreach axis {x y z} {
            foreach {min max} [$obj limits $axis] break
            if {"" != $min && "" != $max} {
                if {"" == [set _${axis}min]} {
                    set _${axis}min $min
                    set _${axis}max $max
                } else {
                    if {$min < [set _${axis}min]} {
                        set _${axis}min $min
                    }
                    if {$max > [set _${axis}max]} {
                        set _${axis}max $max
                    }
                }
            }
        }
    }
    _fixLimits
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_rebuild {} {
    # clear out any old constructs
    foreach ren [array names _actors] {
        foreach actor $_actors($ren) {
            $ren RemoveActor $actor
        }
        set _actors($ren) ""
    }
    foreach dataobj [array names _obj2vtk] {
        foreach cmd $_obj2vtk($dataobj) {
            rename $cmd ""
        }
        set _obj2vtk($dataobj) ""
    }

    # scan through all data objects and build the contours
    set _counter 0
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            set pd $this-polydata$_counter
            vtkPolyData $pd
            $pd SetPoints [$dataobj mesh $comp]
            [$pd GetPointData] SetScalars [$dataobj values $comp]

            # use _zmin/_zmax if possible, otherwise get from data
            if {$_zmin == "" || $_zmax == ""} {
                foreach {z0 z1} [$pd GetScalarRange] break
            } else {
                set z0 $_zmin
                set z1 $_zmax
            }

            set tr $this-triangles$_counter
            vtkDelaunay2D $tr
            $tr SetInput $pd
            $tr SetTolerance 0.0000000000001

            set lu $this-lookup$_counter
            vtkLookupTable $lu
            $lu SetTableRange $z0 $z1
            $lu SetHueRange 0.66667 0.0
            $lu Build

            lappend _obj2vtk($dataobj) $pd $tr $lu

            #
            # Add color contours.
            #
            if {$_counter == 0} {
                set mp $this-mapper$_counter
                vtkPolyDataMapper $mp
                $mp SetInput [$tr GetOutput]
                $mp SetScalarRange $z0 $z1
                $mp SetLookupTable $lu

                set ac $this-actor$_counter
                vtkActor $ac
                $ac SetMapper $mp
                $ac SetPosition 0 0 0
                [$ac GetProperty] SetColor 0 0 0
                $this-ren AddActor $ac
                lappend _actors($this-ren) $ac

                lappend _obj2vtk($dataobj) $mp $ac
            }

            #
            # Add color lines
            #
            if {$_counter > 0} {
                set cf $this-clfilter$_counter
                vtkContourFilter $cf
                $cf SetInput [$tr GetOutput]
                $cf GenerateValues 20 $z0 $z1

                set mp $this-clmapper$_counter
                vtkPolyDataMapper $mp
                $mp SetInput [$cf GetOutput]
                $mp SetScalarRange $z0 $z1
                $mp SetLookupTable $lu

                set ac $this-clactor$_counter
                vtkActor $ac
                $ac SetMapper $mp
                [$ac GetProperty] SetColor 1 1 1
                $ac SetPosition 0 0 0
                $this-ren AddActor $ac
                lappend _actors($this-ren) $ac

                lappend _obj2vtk($dataobj) $cf $mp $ac
            }

            #
            # Add an outline around the data
            #
            set olf $this-olfilter$_counter
            vtkOutlineFilter $olf
            $olf SetInput [$tr GetOutput]

            set olm $this-olmapper$_counter
            vtkPolyDataMapper $olm
            $olm SetInput [$olf GetOutput]

            set ola $this-olactor$_counter
            vtkActor $ola
            $ola SetMapper $olm
            eval [$ola GetProperty] SetColor 0 0 0
            $this-ren AddActor $ola
            lappend _actors($this-ren) $ola

            lappend _obj2vtk($dataobj) $olf $olm $ola

            #
            # Add a legend with the scale.
            #
            set lg $this-legend$_counter
            vtkScalarBarActor $lg
            $lg SetLookupTable $lu
            [$lg GetPositionCoordinate] SetCoordinateSystemToNormalizedViewport
            [$lg GetPositionCoordinate] SetValue 0.1 0.1
            $lg SetOrientationToHorizontal
            $lg SetWidth 0.8
            $lg SetHeight 1.0

            set tp [$lg GetLabelTextProperty]
            eval $tp SetColor [_color2rgb $itk_option(-foreground)]
            $tp BoldOff
            $tp ItalicOff
            $tp ShadowOff
            #eval $tp SetShadowColor [_color2rgb gray]

            $this-ren2 AddActor2D $lg
            lappend _actors($this-ren2) $lg
            lappend _obj2vtk($dataobj) $lg

            incr _counter
        }
    }
    _fixLimits
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_zoom {option} {
    switch -- $option {
        in {
            set camera [$this-ren GetActiveCamera]
            set zoom [$camera Zoom 1.25]
            $this-renWin Render
        }
        out {
            set camera [$this-ren GetActiveCamera]
            set zoom [$camera Zoom 0.8]
            $this-renWin Render
        }
        reset {
            $this-ren ResetCamera
            [$this-ren GetActiveCamera] Zoom 1.5
            $this-renWin Render
            $this-renWin2 Render
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
# USAGE: _move drag <x> <y>
# USAGE: _move release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_move {option x y} {
    switch -- $option {
        click {
            blt::busy configure $itk_component(area) -cursor fleur
            set _click(x) $x
            set _click(y) $y
        }
        drag {
            if {[array size _click] == 0} {
                _move click $x $y
            } else {
                set w [winfo width $itk_component(plot)]
                set h [winfo height $itk_component(plot)]
                set dx [expr {double($x-$_click(x))/$w}]
                set dy [expr {double($y-$_click(y))/$h}]
                foreach actor $_actors($this-ren) {
                    foreach {ax ay az} [$actor GetPosition] break
                    $actor SetPosition [expr {$ax+$dx}] [expr {$ay-$dy}] 0
                }
                $this-renWin Render

                set _click(x) $x
                set _click(y) $y
            }
        }
        release {
            _move drag $x $y
            blt::busy configure $itk_component(area) -cursor left_ptr
            catch {unset _click}
        }
        default {
            error "bad option \"$option\": should be click, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLimits
#
# Used internally to apply automatic limits to the axes for the
# current plot.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_fixLimits {} {
    $this-ren ResetCamera
    [$this-ren GetActiveCamera] Zoom 1.5
    $this-renWin Render
    $this-renWin2 Render
}

# ----------------------------------------------------------------------
# USAGE: _color2rgb <color>
#
# Used internally to convert a color name to a set of {r g b} values
# needed for vtk.  Each r/g/b component is scaled in the range 0-1.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_color2rgb {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] break
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -background
# ----------------------------------------------------------------------
itcl::configbody Rappture::ContourResult::background {
    foreach {r g b} [_color2rgb $itk_option(-background)] break
    $this-ren SetBackground $r $g $b
    $this-renWin Render
    $this-ren2 SetBackground $r $g $b
    $this-renWin2 Render
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -foreground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ContourResult::foreground {
    after cancel [itcl::code $this _rebuild]
    after idle [itcl::code $this _rebuild]
}
