# ----------------------------------------------------------------------
#  COMPONENT: MoleculeViewer - view a molecule in 3D
#
#  This widget brings up a 3D representation of a molecule, which you
#  can rotate.  It extracts atoms and bonds from the Rappture XML
#  representation for a <molecule>.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require vtk
package require vtkinteraction

option add *MoleculeViewer.width 4i widgetDefault
option add *MoleculeViewer.height 4i widgetDefault
option add *MoleculeViewer.backdrop black widgetDefault

itcl::class Rappture::MoleculeViewer {
    inherit itk::Widget

    itk_option define -backdrop backdrop Backdrop "black"
    itk_option define -device device Device ""

    constructor {tool args} { # defined below }
    destructor { # defined below }

    protected method _render {}
    protected method _color2rgb {color}

    private variable _tool ""    ;# tool containing this viewer
    private variable _actors ""  ;# list of actors in renderer
}
                                                                                
itk::usual MoleculeViewer {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::constructor {tool args} {
    set _tool $tool

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

    itk_component add renderer {
        vtkTkRenderWidget $itk_interior.ren -rw $this-renWin
    } {
    }
    pack $itk_component(renderer) -expand yes -fill both

    eval itk_initialize $args
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
}

# ----------------------------------------------------------------------
# USAGE: _render
#
# Used internally to rebuild the scene whenever options within this
# widget change.  Destroys all actors and rebuilds them from scratch.
# ----------------------------------------------------------------------
itcl::body Rappture::MoleculeViewer::_render {} {
    foreach a $_actors {
        $this-ren RemoveActor $a
        rename $a ""
    }
    set _actors ""

    if {$itk_option(-device) != ""} {
        set dev $itk_option(-device)
        set lib [Rappture::library standard]

        set counter 0
        foreach atom [$dev children -type atom components.molecule] {
            set symbol [$dev get components.molecule.$atom.symbol]
            set xyz [$dev get components.molecule.$atom.xyz]
            regsub {,} $xyz {} xyz

            set aname "::actor[incr counter]"
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
        }
    }
    $this-ren ResetCamera
    $this-renWin Render
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
    $this-renWin Render
}

# ----------------------------------------------------------------------
# OPTION: -device
# ----------------------------------------------------------------------
itcl::configbody Rappture::MoleculeViewer::device {
    if {$itk_option(-device) != ""
          && ![Rappture::library isvalid $itk_option(-device)]} {
        error "bad value \"$itk_option(-device)\": should be Rappture::library object"
    }
    after idle [itcl::code $this _render]
}

#package require Rappture
#Rappture::MoleculeViewer .e
#pack .e -expand yes -fill both
#
#set dev [Rappture::library {<?xml version="1.0"?>
#<structure>
#<components>
#<molecule id="Aspirin">
#  <formula>???</formula>
#  <info>Aspirin molecule</info>
#  <atom id="1">
#    <symbol>C</symbol>
#    <xyz>-1.892  -0.992  -1.578</xyz>
#  </atom>
#  <atom id="2">
#    <symbol>C</symbol>
#    <xyz>-1.370  -2.149  -0.990</xyz>
#  </atom>
#  <atom id="3">
#    <symbol>C</symbol>
#    <xyz>-0.079  -2.146  -0.464</xyz>
#  </atom>
#  <atom id="4">
#    <symbol>C</symbol>
#    <xyz>0.708  -0.986  -0.521</xyz>
#  </atom>
#  <atom id="5">
#    <symbol>C</symbol>
#    <xyz>0.203   0.156  -1.196</xyz>
#  </atom>
#  <atom id="6">
#    <symbol>C</symbol>
#    <xyz>-1.108   0.161  -1.654</xyz>
#  </atom>
#  <atom id="7">
#    <symbol>C</symbol>
#    <xyz>2.085  -1.030   0.104</xyz>
#  </atom>
#  <atom id="8">
#    <symbol>O</symbol>
#    <xyz>2.533  -2.034   0.636</xyz>
#  </atom>
#  <atom id="9">
#    <symbol>O</symbol>
#    <xyz>2.879   0.025   0.112</xyz>
#  </atom>
#  <atom id="10">
#    <symbol>O</symbol>
#    <xyz>0.753   1.334  -1.084</xyz>
#  </atom>
#  <atom id="11">
#    <symbol>C</symbol>
#    <xyz>0.668   2.025   0.034</xyz>
#  </atom>
#  <atom id="12">
#    <symbol>O</symbol>
#    <xyz>1.300   3.063   0.152</xyz>
#  </atom>
#  <atom id="13">
#    <symbol>C</symbol>
#    <xyz>-0.243   1.577   1.144</xyz>
#  </atom>
#  <atom id="14">
#    <symbol>H</symbol>
#    <xyz>-2.879  -0.962  -1.985</xyz>
#  </atom>
#  <atom id="15">
#    <symbol>H</symbol>
#    <xyz>-1.988  -3.037  -0.955</xyz>
#  </atom>
#  <atom id="16">
#    <symbol>H</symbol>
#    <xyz>0.300  -3.063  -0.005</xyz>
#  </atom>
#  <atom id="17">
#    <symbol>H</symbol>
#    <xyz>-1.489   1.084  -2.059</xyz>
#  </atom>
#  <atom id="18">
#    <symbol>H</symbol>
#    <xyz>2.566   0.782  -0.326</xyz>
#  </atom>
#  <atom id="19">
#    <symbol>H</symbol>
#    <xyz>-0.761   0.636   0.933</xyz>
#  </atom>
#  <atom id="20">
#    <symbol>H</symbol>
#    <xyz>-1.009   2.349   1.290</xyz>
#  </atom>
#  <atom id="21">
#    <symbol>H</symbol>
#    <xyz>0.346   1.435   2.059</xyz>
#  </atom>
#</molecule>
#</components>
#</structure>}]
# add connectivity at some point...
#CONECT    1    2    6   14                   
#CONECT    2    1    3   15                   
#CONECT    3    2    4   16                   
#CONECT    4    3    5    7                   
#CONECT    5    4    6   10                   
#CONECT    6    1    5   17                   
#CONECT    7    4    8    9                   
#CONECT    8    7                             
#CONECT    9    7   18                        
#CONECT   10    5   11                        
#CONECT   11   10   12   13                   
#CONECT   12   11                             
#CONECT   13   11   19   20   21              
#CONECT   14    1                             
#CONECT   15    2                             
#CONECT   16    3                             
#CONECT   17    6                             
#CONECT   18    9                             
#CONECT   19   13                             
#CONECT   20   13                             
#CONECT   21   13                

#.e configure -device $dev
