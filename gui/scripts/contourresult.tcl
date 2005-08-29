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

blt::bitmap define ContourResult-xslice {
#define x_width 12
#define x_height 12
static unsigned char x_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x9c, 0x03, 0x98, 0x01, 0xf0, 0x00, 0x60, 0x00,
   0x60, 0x00, 0xf0, 0x00, 0x98, 0x01, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap define ContourResult-yslice {
#define y_width 12
#define y_height 12
static unsigned char y_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x0e, 0x07, 0x0c, 0x03, 0x98, 0x01, 0xf0, 0x00,
   0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00};
}
blt::bitmap define ContourResult-zslice {
#define z_width 12
#define z_height 12
static unsigned char z_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x84, 0x03, 0xc0, 0x01, 0xe0, 0x00,
   0x70, 0x00, 0x38, 0x00, 0x1c, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

option add *ContourResult.width 4i widgetDefault
option add *ContourResult.height 4i widgetDefault
option add *ContourResult.foreground black widgetDefault
option add *ContourResult.controlBackground gray widgetDefault
option add *ContourResult.controlDarkBackground #999999 widgetDefault
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
    public method download {}

    protected method _rebuild {}
    protected method _clear {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _slice {option args}
    protected method _3dView {theta phi}
    protected method _fixLimits {}
    protected method _color2rgb {color}

    private variable _dlist ""     ;# list of data objects
    private variable _dims ""      ;# dimensionality of data objects
    private variable _obj2color    ;# maps dataobj => plotting color
    private variable _obj2width    ;# maps dataobj => line width
    private variable _obj2raise    ;# maps dataobj => raise flag 0/1
    private variable _obj2vtk      ;# maps dataobj => vtk objects
    private variable _actors       ;# list of actors for each renderer
    private variable _lights       ;# list of lights for each renderer
    private variable _click        ;# info used for _move operations
    private variable _slicer       ;# vtk transform used for 3D slice plane
    private variable _limits       ;# autoscale min/max for all axes
    private variable _view         ;# view params for 3D view

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

    set _slicer(axis) ""
    set _slicer(plane) ""
    set _slicer(readout) ""
    set _view(theta) 0
    set _view(phi) 0

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

    itk_component add xslice {
        label $itk_component(controls).xslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap ContourResult-xslice
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(xslice) <ButtonPress> \
        [itcl::code $this _slice axis x]
    Rappture::Tooltip::for $itk_component(xslice) "Slice along x-axis"

    itk_component add yslice {
        label $itk_component(controls).yslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap ContourResult-yslice
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(yslice) <ButtonPress> \
        [itcl::code $this _slice axis y]
    Rappture::Tooltip::for $itk_component(yslice) "Slice along y-axis"

    itk_component add zslice {
        label $itk_component(controls).zslice \
            -borderwidth 1 -relief raised -padx 1 -pady 1 \
            -bitmap ContourResult-zslice
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    bind $itk_component(zslice) <ButtonPress> \
        [itcl::code $this _slice axis z]
    Rappture::Tooltip::for $itk_component(zslice) "Slice along z-axis"

    itk_component add slicer {
        ::scale $itk_component(controls).slicer -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    pack $itk_component(slicer) -side bottom -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(slicer) "Move the cut plane"

    #
    # RENDERING AREA
    #
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
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::destructor {} {
    _clear
    after cancel [itcl::code $this _rebuild]

    rename $this-renWin ""
    rename $this-ren ""
    rename $this-iren ""

    rename $this-renWin2 ""
    rename $this-ren2 ""
    rename $this-iren2 ""
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
    _fixLimits
}

# ----------------------------------------------------------------------
# USAGE: download
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::download {} {
    return ""
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_rebuild {} {
    _clear

    # determine the dimensionality from the topmost (raised) object
    set dlist [get]
    set dataobj [lindex $dlist end]
    if {$dataobj != ""} {
        set _dims [$dataobj components -dimensions]
    } else {
        set _dims "0D"
    }

    # scan through all data objects and build the contours
    set _counter 0
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
            #
            # LOOKUP TABLE FOR COLOR CONTOURS
            #
            # use vmin/vmax if possible, otherwise get from data
            if {$_limits(vmin) == "" || $_limits(vmax) == ""} {
                foreach {v0 v1} [$pd GetScalarRange] break
            } else {
                set v0 $_limits(vmin)
                set v1 $_limits(vmax)
            }

            set lu $this-lookup$_counter
            vtkLookupTable $lu
            $lu SetTableRange $v0 $v1
            $lu SetHueRange 0.66667 0.0
            $lu Build

            lappend _obj2vtk($dataobj) $lu

            #
            # Add color contours.
            #
            if {$_counter == 0} {
                if {$_dims == "3D"} {
                    pack $itk_component(slicer) -side bottom -padx 4 -pady 4
                    pack $itk_component(zslice) -side bottom -padx 4 -pady 4
                    pack $itk_component(yslice) -side bottom -padx 4 -pady 4
                    pack $itk_component(xslice) -side bottom -padx 4 -pady 4

                    #
                    # 3D LIGHTS (on both sides of all three axes)
                    #
                    set xm [expr {0.5*($_limits(xmax)+$_limits(xmin))}]
                    set ym [expr {0.5*($_limits(ymax)+$_limits(ymin))}]
                    set zm [expr {0.5*($_limits(zmax)+$_limits(zmin))}]
                    set xr [expr {$_limits(xmax)-$_limits(xmin)}]
                    set yr [expr {$_limits(ymax)-$_limits(ymin)}]
                    set zr [expr {$_limits(zmax)-$_limits(zmin)}]

                    set lt $this-lightxm$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition [expr {$xm-$xr}] 0 0
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    set lt $this-lightxp$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition [expr {$xm+$xr}] 0 0
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    set lt $this-lightym$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition 0 [expr {$ym-$yr}] 0
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    set lt $this-lightyp$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition 0 [expr {$ym+$yr}] 0
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    set lt $this-lightzm$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition 0 0 [expr {$zm-$zr}]
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    set lt $this-lightzp$_counter
                    vtkLight $lt
                    $lt SetColor 1 1 1
                    $lt SetAttenuationValues 0 0 0
                    $lt SetFocalPoint $xm $ym $zm
                    $lt SetPosition 0 0 [expr {$zm+$zr}]
                    $this-ren AddLight $lt
                    lappend _lights($this-ren) $lt

                    #
                    # 3D DATA SET
                    #
                    set mesh [$dataobj mesh $comp]
                    switch -- [$mesh GetClassName] {
                      vtkPoints {
                        # handle cloud of 3D points
                        set pd $this-polydata$_counter
                        vtkPolyData $pd
                        $pd SetPoints $mesh
                        [$pd GetPointData] SetScalars [$dataobj values $comp]

                        set tr $this-triangles$_counter
                        vtkDelaunay3D $tr
                        $tr SetInput $pd
                        $tr SetTolerance 0.0000000000001
                        set source [$tr GetOutput]

                        set mp $this-mapper$_counter
                        vtkPolyDataMapper $mp

                        lappend _obj2vtk($dataobj) $pd $tr $mp
                      }
                      vtkUnstructuredGrid {
                        # handle 3D grid with connectivity
                        set gr $this-grdata$_counter
                        vtkUnstructuredGrid $gr
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
                    # 3D CUT PLANE
                    #
                    set pl $this-cutplane$_counter
                    vtkPlane $pl
                    set _slicer(plane) $pl
                    _slice axis "z"

                    set ct $this-cutter$_counter
                    vtkCutter $ct
                    $ct SetInput $source
                    $ct SetCutFunction $pl

                    set mp $this-cutmapper$_counter
                    vtkPolyDataMapper $mp
                    $mp SetInput [$ct GetOutput]
                    $mp SetScalarRange $v0 $v1
                    $mp SetLookupTable $lu

                    lappend _obj2vtk($dataobj) $pl $ct $mp

                    set ac $this-actor$_counter
                    vtkActor $ac
                    $ac SetMapper $mp
                    $ac SetPosition 0 0 0
                    [$ac GetProperty] SetColor 0 0 0
                    $this-ren AddActor $ac
                    lappend _actors($this-ren) $ac
                    lappend _obj2vtk($dataobj) $ac

                    set olf $this-3dolfilter$_counter
                    vtkOutlineFilter $olf
                    $olf SetInput $source

                    set olm $this-3dolmapper$_counter
                    vtkPolyDataMapper $olm
                    $olm SetInput [$olf GetOutput]

                    set ola $this-3dolactor$_counter
                    vtkActor $ola
                    $ola SetMapper $olm
                    eval [$ola GetProperty] SetColor 0 0 0
                    $this-ren AddActor $ola
                    lappend _actors($this-ren) $ola

                    lappend _obj2vtk($dataobj) $olf $olm $ola

                    #
                    # CUT PLANE READOUT
                    #
                    set tx $this-text$_counter
                    vtkTextMapper $tx
                    set tp [$tx GetTextProperty]
                    eval $tp SetColor [_color2rgb $itk_option(-foreground)]
                    $tp SetVerticalJustificationToTop
                    set _slicer(readout) $tx

                    set txa $this-texta$_counter
                    vtkActor2D $txa
                    $txa SetMapper $tx
                    [$txa GetPositionCoordinate] \
                        SetCoordinateSystemToNormalizedDisplay
                    [$txa GetPositionCoordinate] SetValue 0.02 0.98

                    $this-ren AddActor $txa
                    lappend _actors($this-ren) $txa

                    lappend _obj2vtk($dataobj) $tx $txa
                } else {
                    pack forget $itk_component(xslice)
                    pack forget $itk_component(yslice)
                    pack forget $itk_component(zslice)
                    pack forget $itk_component(slicer)

                    set pd $this-polydata$_counter
                    vtkPolyData $pd
                    $pd SetPoints [$dataobj mesh $comp]
                    [$pd GetPointData] SetScalars [$dataobj values $comp]

                    set tr $this-triangles$_counter
                    vtkDelaunay2D $tr
                    $tr SetInput $pd
                    $tr SetTolerance 0.0000000000001
                    set source [$tr GetOutput]

                    set mp $this-mapper$_counter
                    vtkPolyDataMapper $mp
                    $mp SetInput $source
                    $mp SetScalarRange $v0 $v1
                    $mp SetLookupTable $lu

                    set ac $this-actor$_counter
                    vtkActor $ac
                    $ac SetMapper $mp
                    $ac SetPosition 0 0 0
                    [$ac GetProperty] SetColor 0 0 0
                    $this-ren AddActor $ac
                    lappend _actors($this-ren) $ac

                    lappend _obj2vtk($dataobj) $pd $tr $mp $ac
                }
            }

            #
            # Add color lines
            #
            if {$_counter > 0} {
                set cf $this-clfilter$_counter
                vtkContourFilter $cf
                $cf SetInput $source
                $cf GenerateValues 20 $v0 $v1

                set mp $this-clmapper$_counter
                vtkPolyDataMapper $mp
                $mp SetInput [$cf GetOutput]
                $mp SetScalarRange $v0 $v1
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
            $olf SetInput $source

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
    _zoom reset

    # prevent interactions -- use our own
    blt::busy hold $itk_component(area) -cursor left_ptr
    bind $itk_component(area)_Busy <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(area)_Busy <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(area)_Busy <ButtonRelease> \
        [itcl::code $this _move release %x %y]
}

# ----------------------------------------------------------------------
# USAGE: _clear
#
# Used internally to clear the drawing area and tear down all vtk
# objects in the current scene.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_clear {} {
    # clear out any old constructs
    foreach ren [array names _actors] {
        foreach actor $_actors($ren) {
            $ren RemoveActor $actor
        }
        set _actors($ren) ""
    }
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
    set _slicer(axis) ""
    set _slicer(plane) ""
    set _slicer(readout) ""
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
            [$this-ren GetActiveCamera] Zoom 1.25
            $this-renWin Render
        }
        out {
            [$this-ren GetActiveCamera] Zoom 0.8
            $this-renWin Render
        }
        reset {
            if {$_dims == "3D"} {
                [$this-ren GetActiveCamera] SetViewAngle 30
                $this-ren ResetCamera
                _3dView 45 45
            } else {
                $this-ren ResetCamera
                [$this-ren GetActiveCamera] Zoom 1.5
            }
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
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
        }
        drag {
            if {[array size _click] == 0} {
                _move click $x $y
            } else {
                set w [winfo width $itk_component(plot)]
                set h [winfo height $itk_component(plot)]
                set dx [expr {double($x-$_click(x))/$w}]
                set dy [expr {double($y-$_click(y))/$h}]

                if {$_dims == "2D"} {
                    #
                    # Shift the contour plot in 2D
                    #
                    foreach actor $_actors($this-ren) {
                        foreach {ax ay az} [$actor GetPosition] break
                        $actor SetPosition [expr {$ax+$dx}] [expr {$ay-$dy}] 0
                    }
                    $this-renWin Render
                } elseif {$_dims == "3D"} {
                    #
                    # Rotate the camera in 3D
                    #
                    set theta [expr {$_view(theta) - $dy*180}]
                    if {$theta < 2} { set theta 2 }
                    if {$theta > 178} { set theta 178 }
                    set phi [expr {$_view(phi) - $dx*360}]

                    _3dView $theta $phi
                    $this-renWin Render
                }
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
# USAGE: _slice axis x|y|z
# USAGE: _slice move <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_slice {option args} {
    if {$_slicer(plane) == ""} {
        # no slicer? then bail out!
        return
    }
    switch -- $option {
        axis {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_slice axis xyz\""
            }
            set axis [lindex $args 0]

            switch -- $axis {
                x { $_slicer(plane) SetNormal 1 0 0 }
                y { $_slicer(plane) SetNormal 0 1 0 }
                z { $_slicer(plane) SetNormal 0 0 1 }
                default {
                    error "bad axis \"$axis\": should be x, y, z"
                }
            }

            set _slicer(axis) $axis
            $itk_component(slicer) set 50
            _slice move 50
            $this-renWin Render

            foreach a {x y z} {
                $itk_component(${a}slice) configure -relief raised
            }
            $itk_component(${axis}slice) configure -relief sunken
        }
        move {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"_slice move newval\""
            }
            set newval [lindex $args 0]

            set xm [expr {0.5*($_limits(xmax)+$_limits(xmin))}]
            set ym [expr {0.5*($_limits(ymax)+$_limits(ymin))}]
            set zm [expr {0.5*($_limits(zmax)+$_limits(zmin))}]

            set a $_slicer(axis)
            set newval [expr {0.01*($newval-50)
                *($_limits(${a}max)-$_limits(${a}min))
                  + 0.5*($_limits(${a}max)+$_limits(${a}min))}]

            # show the current value in the readout
            if {$_slicer(readout) != ""} {
                $_slicer(readout) SetInput "$a = $newval"
            }

            # keep a little inside the volume, or the slice will disappear!
            if {$newval == $_limits(${a}min)} {
                set range [expr {$_limits(${a}max)-$_limits(${a}min)}]
                set newval [expr {$newval + 1e-6*$range}]
            }

            # xfer new value to the proper dimension and move the cut plane
            set ${a}m $newval
            $_slicer(plane) SetOrigin $xm $ym $zm

            $this-renWin Render
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
itcl::body Rappture::ContourResult::_3dView {theta phi} {
    set deg2rad 0.0174532927778
    set xn [expr {sin($theta*$deg2rad)*cos($phi*$deg2rad)}]
    set yn [expr {sin($theta*$deg2rad)*sin($phi*$deg2rad)}]
    set zn [expr {cos($theta*$deg2rad)}]

    set xm [expr {0.5*($_limits(xmax)+$_limits(xmin))}]
    set ym [expr {0.5*($_limits(ymax)+$_limits(ymin))}]
    set zm [expr {0.5*($_limits(zmax)+$_limits(zmin))}]

    set cam [$this-ren GetActiveCamera]
    set zoom [$cam GetViewAngle]
    $cam SetViewAngle 30

    $cam SetFocalPoint $xm $ym $zm
    $cam SetPosition [expr {$xm-$xn}] [expr {$ym-$yn}] [expr {$zm+$zn}]
    $cam ComputeViewPlaneNormal
    $cam SetViewUp 0 0 1  ;# z-dir is up
    $cam OrthogonalizeViewUp
    $this-ren ResetCamera
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
