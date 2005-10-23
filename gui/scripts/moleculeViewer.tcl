# ----------------------------------------------------------------------
#  COMPONENT: MoleculeViewer - view a molecule in 3D
#
#  This widget brings up a 3D representation of a molecule, which you
#  can rotate.  It extracts atoms and bonds from the Rappture XML
#  representation for a <molecule>.
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

option add *MoleculeViewer.width 3i widgetDefault
option add *MoleculeViewer.height 3i widgetDefault
option add *MoleculeViewer.backdrop black widgetDefault

blt::bitmap define MoleculeViewer-reset {
#define reset_width 12
#define reset_height 12
static unsigned char reset_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x04, 0x02, 0x04, 0x02, 0x04, 0x02,
   0x04, 0x02, 0x04, 0x02, 0x04, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap define MoleculeViewer-zoomin {
#define zoomin_width 12
#define zoomin_height 12
static unsigned char zoomin_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x11, 0x01, 0x11, 0x01, 0x7d, 0x01, 0x11, 0x01,
   0x11, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}

blt::bitmap define MoleculeViewer-zoomout {
#define zoomout_width 12
#define zoomout_height 12
static unsigned char zoomout_bits[] = {
   0x7c, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01, 0x01, 0x7d, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x82, 0x03, 0xfc, 0x07, 0x80, 0x0f, 0x00, 0x0f, 0x00, 0x06};
}

blt::bitmap define MoleculeViewer-atoms {
#define atoms_width 12
#define atoms_height 12
static unsigned char atoms_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x02, 0x4c, 0x02, 0xc8, 0x03,
   0x48, 0x02, 0x48, 0x02, 0x5c, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

itcl::class Rappture::MoleculeViewer {
    inherit itk::Widget

    itk_option define -backdrop backdrop Backdrop "black"
    itk_option define -device device Device ""

    constructor {tool args} { # defined below }
    destructor { # defined below }

    public method emblems {option}

    protected method _clear {}
    protected method _redraw {}
    protected method _zoom {option}
    protected method _move {option x y}
    protected method _3dView {theta phi psi}
    protected method _color2rgb {color}

    private variable _dispatcher "" ;# dispatcher for !events

    private variable _tool ""    ;# tool containing this viewer
    private variable _actors ""  ;# list of actors in renderer
    private variable _label2atom ;# maps 2D text actor => underlying atom
    private variable _view       ;# view params for 3D view
    private variable _limits     ;# limits of x/y/z axes
    private variable _click      ;# info used for _move operations
}
                                                                                
itk::usual MoleculeViewer {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::constructor {tool args} {
    set _tool $tool

    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"
    $_dispatcher register !render
    $_dispatcher dispatch $this !render "$this-renWin Render; list"
    $_dispatcher register !fixsize
    $_dispatcher dispatch $this !fixsize \
        "[itcl::code $this emblems fixPosition]; list"

    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    vtkRenderWindow $this-renWin
    vtkRenderer $this-ren
    $this-renWin AddRenderer $this-ren

    vtkRenderWindowInteractor $this-int
    $this-int SetRenderWindow $this-renWin

    vtkSphereSource $this-sphere
    $this-sphere SetRadius 1.0
    $this-sphere SetThetaResolution 18
    $this-sphere SetPhiResolution 18

    vtkPolyDataMapper $this-map
    $this-map SetInput [$this-sphere GetOutput]

    vtkCoordinate $this-xyzconv
    $this-xyzconv SetCoordinateSystemToWorld

    set _view(theta) 0
    set _view(phi) 0
    set _view(psi) 0

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
            -bitmap MoleculeViewer-reset \
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
            -bitmap MoleculeViewer-zoomin \
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
            -bitmap MoleculeViewer-zoomout \
            -command [itcl::code $this _zoom out]
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(zoomout) -padx 4 -pady 4
    Rappture::Tooltip::for $itk_component(zoomout) "Zoom out"

    itk_component add labels {
        label $itk_component(controls).labels \
            -borderwidth 1 -padx 1 -pady 1 \
            -bitmap MoleculeViewer-atoms
    } {
        usual
        ignore -borderwidth
        rename -highlightbackground -controlbackground controlBackground Background
    }
    pack $itk_component(labels) -padx 4 -pady 8 -ipadx 1 -ipady 1
    Rappture::Tooltip::for $itk_component(labels) "Show/hide the labels on atoms"
    bind $itk_component(labels) <ButtonPress> \
        [itcl::code $this emblems toggle]

    #
    # RENDERING AREA
    #
    itk_component add area {
        frame $itk_interior.area
    }
    pack $itk_component(area) -expand yes -fill both
    bind $itk_component(area) <Configure> \
        [list $_dispatcher event -idle !fixsize]

    itk_component add renderer {
        vtkTkRenderWidget $itk_component(area).ren -rw $this-renWin
    } {
    }
    pack $itk_component(renderer) -expand yes -fill both

    eval itk_initialize $args

    # prevent interactions -- use our own
    blt::busy hold $itk_component(area) -cursor left_ptr
    bind $itk_component(area)_Busy <ButtonPress> \
        [itcl::code $this _move click %x %y]
    bind $itk_component(area)_Busy <B1-Motion> \
        [itcl::code $this _move drag %x %y]
    bind $itk_component(area)_Busy <ButtonRelease> \
        [itcl::code $this _move release %x %y]

    emblems on
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::destructor {} {
    rename $this-renWin ""
    rename $this-ren ""
    rename $this-int ""
    rename $this-sphere ""
    rename $this-map ""
    rename $this-xyzconv ""
}

# ----------------------------------------------------------------------
# USAGE: _clear
#
# Used internally to clear the scene whenever it is about to change.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_clear {} {
    foreach a $_actors {
        $this-ren RemoveActor $a
        rename $a ""
    }
    set _actors ""
    catch {unset _label2atom}

    foreach lim {xmin xmax ymin ymax zmin zmax} {
        set _limits($lim) ""
    }

    $this-ren ResetCamera
    $_dispatcher event -now !render
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to rebuild the scene whenever options within this
# widget change.  Destroys all actors and rebuilds them from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_redraw {} {
    blt::busy hold $itk_component(hull); update

    _clear

    if {$itk_option(-device) != ""} {
        set dev $itk_option(-device)
        set lib [Rappture::library standard]

        set counter 0
        foreach atom [$dev children -type atom components.molecule] {
            set symbol [$dev get components.molecule.$atom.symbol]
            set xyz [$dev get components.molecule.$atom.xyz]
            regsub {,} $xyz {} xyz

            # update overall limits for molecules along all axes
            foreach axis {x y z} val $xyz {
                if {"" == $_limits(${axis}min)} {
                    set _limits(${axis}min) $val
                    set _limits(${axis}max) $val
                } else {
                    if {$val < $_limits(${axis}min)} {
                        set _limits(${axis}min) $val
                    }
                    if {$val > $_limits(${axis}max)} {
                        set _limits(${axis}max) $val
                    }
                }
            }

            # create an actor for each atom
            set aname $this-actor[incr counter]
            vtkActor $aname
            $aname SetMapper $this-map
            eval $aname SetPosition $xyz
            $this-ren AddActor $aname

            set sfac 0.7
            set scale [$lib get elements.($symbol).scale]
            if {$scale != ""} {
                $aname SetScale [expr {$sfac*$scale}]
            }
            set color [$lib get elements.($symbol).color]
            if {$color != ""} {
                eval [$aname GetProperty] SetColor [_color2rgb $color]
            }

            lappend _actors $aname

            # create a label for each atom
            set lname $this-label$counter
            vtkTextActor $lname
            $lname SetInput "$counter $symbol"
            $lname ScaledTextOff

            set tprop [$lname GetTextProperty]
            $tprop SetJustificationToCentered
            $tprop SetVerticalJustificationToCentered
            $tprop ShadowOn
            $tprop SetColor 1 1 1

            set _label2atom($lname) $aname
            lappend _actors $lname
        }
        if {[$itk_component(labels) cget -relief] == "sunken"} {
            emblems on
        }
        _zoom reset
    }
    $this-ren ResetCamera
    $_dispatcher event -idle !render

    blt::busy release $itk_component(hull)
}

# ----------------------------------------------------------------------
# USAGE: _zoom in
# USAGE: _zoom out
# USAGE: _zoom reset
#
# Called automatically when the user clicks on one of the zoom
# controls for this widget.  Changes the zoom for the current view.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_zoom {option} {
    switch -- $option {
        in {
            [$this-ren GetActiveCamera] Zoom 1.25
        }
        out {
            [$this-ren GetActiveCamera] Zoom 0.8
        }
        reset {
            $this-ren ResetCamera
            _3dView 45 45 0
        }
    }
    $_dispatcher event -later !fixsize
    $_dispatcher event -idle !render
}

# ----------------------------------------------------------------------
# USAGE: _move click <x> <y>
# USAGE: _move drag <x> <y>
# USAGE: _move release <x> <y>
#
# Called automatically when the user clicks/drags/releases in the
# plot area.  Moves the plot according to the user's actions.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_move {option x y} {
    switch -- $option {
        click {
            blt::busy configure $itk_component(area) -cursor fleur
            set _click(x) $x
            set _click(y) $y
            set _click(theta) $_view(theta)
            set _click(phi) $_view(phi)
            set _click(psi) $_view(psi)
        }
        drag {
            if {[array size _click] == 0} {
                _move click $x $y
            } else {
                set w [winfo width $itk_component(renderer)]
                set h [winfo height $itk_component(renderer)]
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
                #if {$theta < 2} { set theta 2 }
                #if {$theta > 178} { set theta 178 }

                if {$theta > 45 && $theta < 135} {
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

                _3dView $theta $phi $psi
                emblems fixPosition
                $_dispatcher event -idle !render

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
# USAGE: _3dView <theta> <phi> <psi>
#
# Used internally to change the position of the camera for 3D data
# sets.  Sets the camera according to the angles <theta> (angle from
# the z-axis) and <phi> (angle from the x-axis in the x-y plane).
# Both angles are in degrees.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_3dView {theta phi psi} {
    set deg2rad 0.0174532927778
    set xp [expr {sin($theta*$deg2rad)*cos($phi*$deg2rad)}]
    set yp [expr {sin($theta*$deg2rad)*sin($phi*$deg2rad)}]
    set zp [expr {cos($theta*$deg2rad)}]

    set xm [expr {0.5*($_limits(xmax)+$_limits(xmin))}]
    set ym [expr {0.5*($_limits(ymax)+$_limits(ymin))}]
    set zm [expr {0.5*($_limits(zmax)+$_limits(zmin))}]

    set cam [$this-ren GetActiveCamera]
    set zoom [$cam GetViewAngle]
    $cam SetViewAngle 30

    $cam SetFocalPoint $xm $ym $zm
    $cam SetPosition [expr {$xm-$xp}] [expr {$ym-$yp}] [expr {$zm+$zp}]
    $cam ComputeViewPlaneNormal
    $cam SetViewUp 0 0 1  ;# z-dir is up
    $cam OrthogonalizeViewUp
    $cam Azimuth $psi
    $this-ren ResetCamera
    $cam SetViewAngle $zoom

    # fix up the labels so they sit over the new atom positions
    emblems fixPosition

    set _view(theta) $theta
    set _view(phi) $phi
    set _view(psi) $psi
}

# ----------------------------------------------------------------------
# USAGE: emblems on
# USAGE: emblems off
# USAGE: emblems toggle
# USAGE: emblems fixPosition
#
# Used internally to turn labels associated with atoms on/off, and to
# update the positions of the labels so they sit on top of each atom.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::emblems {option} {
    switch -- $option {
        on {
            set state 1
        }
        off {
            set state 0
        }
        toggle {
            if {[$itk_component(labels) cget -relief] == "sunken"} {
                set state 0
            } else {
                set state 1
            }
        }
        fixPosition {
            foreach lname [array names _label2atom] {
                set aname $_label2atom($lname)
                set xyz [$aname GetPosition]
                eval $this-xyzconv SetValue $xyz
                set xy [$this-xyzconv GetComputedViewportValue $this-ren]
                eval $lname SetDisplayPosition $xy
            }
            return
        }
        default {
            error "bad option \"$option\": should be on, off, toggle, fixPosition"
        }
    }

    if {$state} {
        $itk_component(labels) configure -relief sunken
        foreach lname [array names _label2atom] {
            catch {$this-ren AddActor2D $lname}
        }
        emblems fixPosition
    } else {
        $itk_component(labels) configure -relief raised
        foreach lname [array names _label2atom] {
            catch {$this-ren RemoveActor $lname}
        }
    }
    $_dispatcher event -idle !render
}

# ----------------------------------------------------------------------
# USAGE: _color2rgb color
#
# Used internally to convert a Tk color name into the r,g,b values
# used in Vtk (scaled 0-1).
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_color2rgb {color} {
    foreach {r g b} [winfo rgb $itk_component(hull) $color] {}
    set r [expr {$r/65535.0}]
    set g [expr {$g/65535.0}]
    set b [expr {$b/65535.0}]
    return [list $r $g $b]
}

# ----------------------------------------------------------------------
# OPTION: -backdrop
# ----------------------------------------------------------------------
itcl::configbody Rappture::MoleculeViewer::backdrop {
    eval $this-ren SetBackground [_color2rgb $itk_option(-backdrop)]
    $_dispatcher event -idle !render
}

# ----------------------------------------------------------------------
# OPTION: -device
# ----------------------------------------------------------------------
itcl::configbody Rappture::MoleculeViewer::device {
    if {$itk_option(-device) != ""
          && ![Rappture::library isvalid $itk_option(-device)]} {
        error "bad value \"$itk_option(-device)\": should be Rappture::library object"
    }
    _clear

    if {$itk_option(-device) != ""} {
        set state [$itk_option(-device) get components.molecule.about.emblems]
        if {$state == "" || ![string is boolean $state] || !$state} {
            emblems off
        } else {
            emblems on
        }
    }
    $_dispatcher event -idle !redraw
}
