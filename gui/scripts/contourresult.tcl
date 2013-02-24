# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: contourresult - contour plot in a ResultSet
#
#  This widget is a contour plot for 2D meshes with a scalar value.
#  It is normally used in the ResultViewer to show results from the
#  run of a Rappture tool.  Use the "add" and "delete" methods to
#  control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT
package require Img

option add *ContourResult.width 4i widgetDefault
option add *ContourResult.height 4i widgetDefault
option add *ContourResult.foreground black widgetDefault
option add *ContourResult.controlBackground gray widgetDefault
option add *ContourResult.controlDarkBackground #999999 widgetDefault
option add *ContourResult.plotBackground black widgetDefault
option add *ContourResult.plotForeground white widgetDefault
option add *ContourResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::ContourResult {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    protected method _rebuild {}
    protected method _clear {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _slice {option args}
    protected method _3dView {theta phi}
    protected method _fixLimits {}
    protected method _slicertip {axis}
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
    private variable _download ""  ;# snapshot for download
}

itk::usual ContourResult {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::constructor {args} {
    package require vtk
    package require vtkinteraction
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

    foreach val {xmin xmax ymin ymax zmin zmax vmin vmax} {
        set _limits($val) ""
    }

    itk_component add controls {
        frame $itk_interior.cntls
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
            -command [itcl::code $this _zoom reset]
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
            -command [itcl::code $this _zoom in]
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
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
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
    grid $itk_component(xslice) -row 0 -column 0 -sticky ew -padx 1

    itk_component add xslicer {
        ::scale $itk_component(slicers).xval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move x]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(xslicer) -row 1 -column 0 -padx 1
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
    grid $itk_component(yslice) -row 0 -column 1 -sticky ew -padx 1

    itk_component add yslicer {
        ::scale $itk_component(slicers).yval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move y]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(yslicer) -row 1 -column 1 -padx 1
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
    grid $itk_component(zslice) -row 0 -column 2 -sticky ew -padx 1
    bind $itk_component(zslice) <ButtonPress> \
        [itcl::code $this _slice axis z toggle]
    Rappture::Tooltip::for $itk_component(zslice) \
        "Toggle the Z cut plane on/off"

    itk_component add zslicer {
        ::scale $itk_component(slicers).zval -from 100 -to 0 \
            -width 10 -orient vertical -showvalue off -state disabled \
            -borderwidth 1 -highlightthickness 0 \
            -command [itcl::code $this _slice move z]
    } {
        usual
        ignore -borderwidth
        ignore -highlightthickness
        rename -highlightbackground -controlbackground controlBackground Background
        rename -troughcolor -controldarkbackground controlDarkBackground Background
    }
    grid $itk_component(zslicer) -row 1 -column 2 -padx 1
    Rappture::Tooltip::for $itk_component(zslicer) \
        "@[itcl::code $this _slicertip z]"

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
    $this-renWin LineSmoothingOn
    $this-renWin PolygonSmoothingOn
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

    #
    # Create a photo for download snapshots
    #
    set _download [image create photo]

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

    image delete $_download
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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::download {option args} {
    switch $option {
        coming {
            if {[catch {blt::winop snap $itk_component(area) $_download}]} {
                $_download configure -width 1 -height 1
                $_download put #000000
            }
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            # Get the image data (as base64) and decode it back to binary.
            # This is better than writing to temporary files.  When we switch
            # to the BLT picture image it won't be necessary to decode the
            # image data.
            set bytes [$_download data -format "jpeg -quality 100"]
            set bytes [Rappture::encoding::decode -as b64 $bytes]
            return [list .jpg $bytes]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
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
    set id 0

    # determine the dimensionality from the topmost (raised) object
    set dlist [get]
    set dataobj [lindex $dlist end]
    if {$dataobj != ""} {
        set _dims [lindex [lsort [$dataobj components -dimensions]] end]
    } else {
        set _dims "0D"
    }

    #
    # LOOKUP TABLE FOR COLOR CONTOURS
    #
    # use vmin/vmax if possible, otherwise get from data
    if {$_limits(vmin) == "" || $_limits(vmax) == ""} {
        set v0 0
        set v1 1
        if {[info exists _obj2vtk($dataobj)]} {
            set pd [lindex $_obj2vtk($dataobj) 0]
            if {"" != $pd} {
                foreach {v0 v1} [$pd GetScalarRange] break
            }
        }
    } else {
        set v0 $_limits(vmin)
        set v1 $_limits(vmax)
    }

    set lu $this-lookup$id
    vtkLookupTable $lu
    $lu SetTableRange $v0 $v1
    $lu SetHueRange 0.66667 0.0
    $lu Build

    lappend _obj2vtk($dataobj) $lu

    if {$_dims == "3D"} {
        #
        # 3D LIGHTS (on both sides of all three axes)
        #
        set x0 $_limits(xmin)
        set x1 $_limits(xmax)
        set xm [expr {0.5*($x0+$x1)}]
        set y0 $_limits(ymin)
        set y1 $_limits(ymax)
        set ym [expr {0.5*($y0+$y1)}]
        set z0 $_limits(zmin)
        set z1 $_limits(zmax)
        set zm [expr {0.5*($z0+$z1)}]
        set xr [expr {$x1-$x0}]
        set yr [expr {$y1-$y0}]
        set zr [expr {$z1-$z0}]

        set lt $this-light$id
        vtkLight $lt
        $lt SetColor 1 1 1
        $lt SetAttenuationValues 0 0 0
        $lt SetFocalPoint $xm $ym $zm
        $lt SetLightTypeToHeadlight
        $this-ren AddLight $lt
        lappend _lights($this-ren) $lt

    } else {
    }

    # scan through all data objects and build the contours
    set firstobj 1
    foreach dataobj [get] {
        foreach comp [$dataobj components] {
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
puts stderr "ContourResult: dataobj=$dataobj mesh=$mesh "
                    switch -- [$mesh GetClassName] {
                      vtkPoints {
                        # handle cloud of 3D points
                        set pd $this-polydata$id
                        vtkPolyData $pd
                        $pd SetPoints $mesh
                        [$pd GetPointData] SetScalars [$dataobj values $comp]

                        set tr $this-triangles$id
                        vtkDelaunay3D $tr
                        $tr SetInput $pd
                        $tr SetTolerance 0.0000000000001
                        set source [$tr GetOutput]

                        set mp $this-mapper$id
                        vtkPolyDataMapper $mp

                        lappend _obj2vtk($dataobj) $pd $tr $mp
                      }
                      vtkUnstructuredGrid {
                        # handle 3D grid with connectivity
                        set gr $this-grdata$id
                        vtkUnstructuredGrid $gr
                        $gr ShallowCopy $mesh
                        [$gr GetPointData] SetScalars [$dataobj values $comp]
                        set source $gr

                        lappend _obj2vtk($dataobj) $gr
                      }
                      vtkRectilinearGrid {
                        # handle 3D grid with connectivity
                        set gr $this-grdata$id
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
                    set iso $this-iso$id
                    vtkContourFilter $iso
                      $iso SetInput $source

                    set mp $this-isomap$id
                    vtkPolyDataMapper $mp
                      $mp SetInput [$iso GetOutput]

                    set ac $this-isoactor$id
                    vtkActor $ac
                      $ac SetMapper $mp
                      [$ac GetProperty] SetOpacity 0.3
                      [$ac GetProperty] SetDiffuse 0.5
                      [$ac GetProperty] SetAmbient 0.7
                      [$ac GetProperty] SetSpecular 10.0
                      [$ac GetProperty] SetSpecularPower 200.0
                    $this-ren AddActor $ac

                    lappend _obj2vtk($dataobj) $iso $mp $ac
                    lappend _actors($this-ren) $ac

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

                            $this-ren AddActor $ac
                            lappend _actors($this-ren) $ac
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

                        $this-ren AddActor $txa
                        lappend _actors($this-ren) $txa

                        lappend _obj2vtk($dataobj) $tx $txa

                        # turn off all slicers by default
                        foreach axis {x y z} {
                            $itk_component(${axis}slicer) configure -state normal
                            $itk_component(${axis}slicer) set 50
                            _slice move $axis 50
                            _slice axis $axis off
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
                    $this-ren AddActor $ac
                    lappend _actors($this-ren) $ac

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
                $this-ren AddActor $ac
                lappend _actors($this-ren) $ac

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
                $this-ren AddActor $ola
                lappend _actors($this-ren) $ola

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
                        $this-ren AddActor $vac

                        lappend _obj2vtk($dataobj) $vtx $vmp $vac
                        lappend _actors($this-ren) $vac

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

                $this-ren2 AddActor2D $lg
                lappend _actors($this-ren2) $lg
                lappend _obj2vtk($dataobj) $lg
            }

            incr id
        }
        set firstobj 0
    }
    _fixLimits
    _zoom reset

    #
    # HACK ALERT!  A single ResetCamera doesn't seem to work for
    #   some contour data.  You have to do it multiple times to
    #   get to the right zoom factor on data.  I hope 20 times is
    #   enough.  I hate Vtk sometimes...
    #
    for {set i 0} {$i < 20} {incr i} {
        $this-ren ResetCamera
        [$this-ren GetActiveCamera] Zoom 1.5
    }

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
    set _slicer(xplane) ""
    set _slicer(yplane) ""
    set _slicer(zplane) ""
    set _slicer(xslice) ""
    set _slicer(yslice) ""
    set _slicer(zslice) ""
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
                set scalex [expr {$_limits(xmax)-$_limits(xmin)}]
                set scaley [expr {$_limits(ymax)-$_limits(ymin)}]
                set dx [expr {double($x-$_click(x))/$w*$scalex}]
                set dy [expr {double($y-$_click(y))/$h*$scaley}]

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
# USAGE: _slice axis x|y|z ?on|off|toggle?
# USAGE: _slice move x|y|z <newval>
#
# Called automatically when the user drags the slider to move the
# cut plane that slices 3D data.  Gets the current value from the
# slider and moves the cut plane to the appropriate point in the
# data set.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_slice {option args} {
    if {$_slicer(xplane) == ""} {
        # no slicer? then bail out!
        return
    }
    switch -- $option {
        axis {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"_slice axis x|y|z ?on|off|toggle?\""
            }
            set axis [lindex $args 0]
            set op [lindex $args 1]
            if {$op == ""} { set op "on" }

            if {[$itk_component(${axis}slice) cget -relief] == "raised"} {
                set current "off"
            } else {
                set current "on"
            }

            if {$op == "toggle"} {
                if {$current == "on"} { set op "off" } else { set op "on" }
            }

            if {$op} {
                $itk_component(${axis}slicer) configure -state normal
                $_slicer(${axis}slice) VisibilityOn
                $itk_component(${axis}slice) configure -relief sunken
            } else {
                $itk_component(${axis}slicer) configure -state disabled
                $_slicer(${axis}slice) VisibilityOff
                $itk_component(${axis}slice) configure -relief raised
            }
            $this-renWin Render
        }
        move {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_slice move x|y|z newval\""
            }
            set axis [lindex $args 0]
            set newval [lindex $args 1]

            set xm [expr {0.5*($_limits(xmax)+$_limits(xmin))}]
            set ym [expr {0.5*($_limits(ymax)+$_limits(ymin))}]
            set zm [expr {0.5*($_limits(zmax)+$_limits(zmin))}]

            set newval [expr {0.01*($newval-50)
                *($_limits(${axis}max)-$_limits(${axis}min))
                  + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]

            # show the current value in the readout
            if {$_slicer(readout) != ""} {
                $_slicer(readout) SetInput "$axis = $newval"
            }

            # keep a little inside the volume, or the slice will disappear!
            if {$newval == $_limits(${axis}min)} {
                set range [expr {$_limits(${axis}max)-$_limits(${axis}min)}]
                set newval [expr {$newval + 1e-6*$range}]
            }

            # xfer new value to the proper dimension and move the cut plane
            set ${axis}m $newval
            $_slicer(${axis}plane) SetOrigin $xm $ym $zm

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
# USAGE: _slicertip <axis>
#
# Used internally to generate a tooltip for the x/y/z slicer controls.
# Returns a message that includes the current slicer value.
# ----------------------------------------------------------------------
itcl::body Rappture::ContourResult::_slicertip {axis} {
    set val [$itk_component(${axis}slicer) get]
    set val [expr {0.01*($val-50)
        *($_limits(${axis}max)-$_limits(${axis}min))
          + 0.5*($_limits(${axis}max)+$_limits(${axis}min))}]
    return "Move the [string toupper $axis] cut plane.\nCurrently:  $axis = $val"
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
# CONFIGURATION OPTION: -plotbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ContourResult::plotbackground {
    foreach {r g b} [_color2rgb $itk_option(-plotbackground)] break
    $this-ren SetBackground $r $g $b
    $this-renWin Render
    $this-ren2 SetBackground $r $g $b
    $this-renWin2 Render
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -plotforeground
# ----------------------------------------------------------------------
itcl::configbody Rappture::ContourResult::plotforeground {
    after cancel [itcl::code $this _rebuild]
    after idle [itcl::code $this _rebuild]
}
