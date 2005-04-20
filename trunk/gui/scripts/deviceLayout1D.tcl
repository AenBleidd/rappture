# ----------------------------------------------------------------------
#  COMPONENT: deviceLayout1D - visualizer for 1D device geometries
#
#  This widget is a simple visualizer for the layout of 1D devices.
#  It takes the Rappture XML representation for a 1D device and draws
#  the series of slabs representing the various material layers in the
#  device.  It can be configured to allow simple selection and editing
#  of material layers.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *DeviceLayout1D.width 4i widgetDefault
option add *DeviceLayout1D.deviceSize 0.25i widgetDefault
option add *DeviceLayout1D.deviceOutline black widgetDefault
option add *DeviceLayout1D.annotate all widgetDefault
option add *DeviceLayout1D.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::DeviceLayout1D {
    inherit itk::Widget

    itk_option define -font font Font ""
    itk_option define -library library Library ""
    itk_option define -device device Device ""
    itk_option define -devicesize deviceSize DeviceSize 0
    itk_option define -deviceoutline deviceOutline DeviceOutline ""
    itk_option define -leftmargin leftMargin LeftMargin 0
    itk_option define -rightmargin rightMargin RightMargin 0

    constructor {args} { # defined below }
    public method limits {}
    public method extents {what}
    public method controls {option args}

    protected method _layout {}
    protected method _redraw {}
    protected method _drawLayer {index x0 x1}
    protected method _drawMolecule {index x0 x1}
    protected method _drawAnnotation {index x0 x1}
    protected method _mater2color {mater}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _sizes         ;# maps size name => pixels

    private variable _library ""    ;# LibraryObj for library information
    private variable _device ""     ;# LibraryObj for device representation
    private variable _slabs ""      ;# list of node names for slabs in device
    private variable _z0 ""         ;# list parallel to _slabs with z0
                                    ;#   coord for lhs of each slab
    private variable _zthick ""     ;# list parallel to _slabs with thickness
                                    ;#   for each slab
    private variable _maters ""     ;# list parallel to _slabs with material
                                    ;#   for each slab

    private variable _controls      ;# maps control path => status on/off

    private common _icons
    set _icons(molecule) [image create photo -file \
        [file join $Rappture::installdir scripts images molecule.gif]]
}
                                                                                
itk::usual DeviceLayout1D {
    keep -background -cursor
    keep -library -device
    keep -deviceoutline -devicesize
    keep -selectbackground -selectforeground -selectborderwidth
    keep -width
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this _layout]; list"
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw "[itcl::code $this _redraw]; list"

    itk_option add hull.width
    pack propagate $itk_component(hull) no

    itk_component add area {
        canvas $itk_interior.area -borderwidth 0 -highlightthickness 0
    } {
        usual
        ignore -borderwidth -relief
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(area) -fill both
    bind $itk_component(area) <Configure> \
        [list $_dispatcher event -idle !redraw]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: limits
#
# Clients use this to query the limits of the x-axis for the
# current device.  Returns the min/max limits indicating the
# physical size of the device.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::limits {} {
    if {[$_dispatcher ispending !layout]} {
        $_dispatcher cancel !layout
        _layout
    }
    set zmin [lindex $_z0 0]
    set zmax [lindex $_z0 end]
    return [list $zmin $zmax]
}

# ----------------------------------------------------------------------
# USAGE: extents <what>
#
# Clients use this to query the size of various things within this
# widget--similar to the "extents" method of the BLT graph.  Returns
# the size of some item in pixels.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::extents {what} {
    switch -- $what {
        bar3D { return $_sizes(bar45) }
        default {
            error "bad option \"$what\": should be bar3D"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: controls add <path>
#
# Clients use this to add hints about the controls that should be
# added to the layout area.  Common paths are components.slab#.material
# and components.slab#.thickness.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::controls {option args} {
    switch -- $option {
        add {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"controls add path\""
            }
            set path [lindex $args 0]
            set _controls($path) 1
            $_dispatcher event -idle !layout
        }
        remove {
            error "not yet implemented"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Called automatically whenever something changes that affects the
# layout of the widget.  Recalculates the layout and computes a good
# overall value for the minimum height of the widget.  This may cause
# the widget to change size, which in turn would trigger another
# _redraw.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_layout {} {
    # first, recompute the overall height of this widget
    set h [expr {$_sizes(bar)+$_sizes(bar45)+20}]

    set fnt $itk_option(-font)
    if {[regexp {\.material} [array names _controls]]} {
        # one of the slabs has its material displayed
        set extra [expr {1.2*[font metrics $fnt -linespace]}] 
        set h [expr {$h+$extra}]
    }
    if {[regexp {\.thickness} [array names _controls]]} {
        # one of the slabs has its thickness displayed
        set extra [expr {1.2*[font metrics $fnt -linespace]}] 
        set h [expr {$h+$extra}]
    }

    # see if any of the slabs has a label
    if {$_device != ""} {
        foreach nn [$_device children components] {
            if {"" != [$_device get components.$nn.about.label]} {
                set extra [expr {1.2*[font metrics $fnt -linespace]}] 
                set h [expr {$h+$extra}]
                break
            }
        }
    }

    # a little extra height for the molecule image
    if {"" != [$_device element components.molecule]} {
        set h [expr {$h+15}]
    }

    set oldh [component hull cget -height]
    if {$h != $oldh} {
        component hull configure -height $h
        $_dispatcher event -idle !redraw
    }

    # next, scan through the device and compute layer positions
    set slabs ""
    set z0 ""
    set zthick ""
    set maters ""

    set z 0
    if {$_device != ""} {
        foreach nn [$_device children components] {
            switch -glob -- $nn {
                slab* - molecule* {
                    set tval [$_device get components.$nn.thickness]
                    set tval [Rappture::Units::convert $tval \
                        -context um -to um -units off]
                    lappend slabs components.$nn
                    lappend z0 $z
                    lappend zthick $tval
                    lappend maters [$_device get components.$nn.material]

                    set z [expr {$z+$tval}]
                }
                default {
                    # element not recognized -- skip it
                }
            }
        }
    }
    lappend z0 $z

    # something change? then store new layout and redraw
    if {![string equal $z0 $_z0]
          || ![string equal $zthick $_zthick]
          || ![string equal $maters $_maters]} {
        set _slabs $slabs
        set _z0 $z0
        set _zthick $zthick
        set _maters $maters

        $_dispatcher event -idle !redraw
    }
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Called automatically whenever the device geometry changes, or when
# the canvas changes size, to redraw all elements within it.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_redraw {} {
    set c $itk_component(area)

    # clean up images and delete all other items
    foreach item [$c find withtag image] {
        image delete [$c itemcget $item -image]
    }
    $c delete all

    set w [expr {[winfo width $c]-1 - $_sizes(lm) - $_sizes(rm)}]

    set x0 $_sizes(lm)
    set x1 [expr {$x0 + $w}]

    set zmax [lindex $_z0 end]
    set xx0 $x0
    set xx1 $x1

    set drewslab 0
    for {set i 0} {$i < [llength $_slabs]} {incr i} {
        set name [lindex $_slabs $i]
        if {[regexp {slab[0-9]*$} $name]} {
            set z0 [lindex $_z0 $i]
            set zthick [lindex $_zthick $i]
            set xx0 [expr {double($z0)/$zmax * ($x1-$x0) + $x0}]
            set xx1 [expr {double($z0+$zthick)/$zmax * ($x1-$x0) + $x0}]
            _drawLayer $i $xx0 $xx1
            _drawAnnotation $i $xx0 $xx1
            set drewslab 1
        } else {
            if {$drewslab} {
                _drawLayer cap $xx0 $xx1  ;# draw the end cap
                set drewslab 0
            }
            if {[regexp {molecule[0-9]*$} $name]} {
                set z0 [lindex $_z0 $i]
                set zthick [lindex $_zthick $i]
                set xx0 [expr {double($z0)/$zmax * ($x1-$x0) + $x0}]
                set xx1 [expr {double($z0+$zthick)/$zmax * ($x1-$x0) + $x0}]
                _drawMolecule $i $xx0 $xx1
                _drawAnnotation $i $xx0 $xx1
            }
        }
    }
    if {[llength $_slabs] > 0} {
        _drawLayer cap $xx0 $xx1  ;# draw the end cap
    }
}

# ----------------------------------------------------------------------
# USAGE: _drawLayer <index> <x0> <x1>
#
# Used internally within _redraw to draw one material layer at the
# <index> within the slab list into the active area.  The layer is
# drawn between coordinates <x0> and <x1> on the canvas.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_drawLayer {index x0 x1} {
    set c $itk_component(area)
    set h [expr {[winfo height $c]-1}]
    # a little extra height for the molecule image
    if {"" != [$_device element components.molecule]} {
        set h [expr {$h-15}]
    }

    set y0 $h
    set y0p [expr {$y0-$_sizes(bar45)}]
    set y1p [expr {$y0-$_sizes(bar)}]
    set y1 [expr {$y1p-$_sizes(bar45)}]

    set x0p [expr {$x0+$_sizes(bar45)}]
    set x1p [expr {$x1+$_sizes(bar45)}]

    set lcolor $itk_option(-deviceoutline)

    if {$index == "cap"} {
        #
        # Draw the outline around the end cap
        #
        $c create line $x1 $y0  $x1 $y1p  $x1p $y1 -fill $lcolor

    } elseif {$index < [llength $_slabs]} {
        set fcolor [_mater2color [lindex $_maters $index]]

        #
        # Draw one segment of the bar in the canvas:
        #
        #      ___________________  ____ y1
        #     /                  /| ____ y0p
        #    /__________________/ / ____ y1p
        #    |__________________|/: ____ y0
        #    : :                : :
        #   x0 x0p             x1 x1p
        #
        $c create polygon \
            $x0 $y0  $x1 $y0  $x1p $y0p  $x1p $y1  $x0p $y1  $x0 $y1p  $x0 $y0 \
            -outline $lcolor -fill $fcolor
        $c create line $x0 $y1p  $x1 $y1p -fill $lcolor
    }
}

# ----------------------------------------------------------------------
# USAGE: _drawMolecule <index> <x0> <x1>
#
# Used internally within _redraw to draw one molecule layer at the
# <index> within the slab list into the active area.  The layer is
# drawn between coordinates <x0> and <x1> on the canvas.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_drawMolecule {index x0 x1} {
    set c $itk_component(area)
    set h [expr {[winfo height $c]-1}]
    # a little extra height for the molecule image
    if {"" != [$_device element components.molecule]} {
        set h [expr {$h-15}]
    }

    set y0 $h
    set y0p [expr {$y0-$_sizes(bar45)}]
    set y1p [expr {$y0-$_sizes(bar)}]
    set y1 [expr {$y1p-$_sizes(bar45)}]
    set x0p [expr {$x0+$_sizes(bar45)}]

    set x [expr {0.5*($x0+$x0p)}]
    set y [expr {0.5*($y0+$y0p) + 0.5*($y1-$y0p)}]

    set w [image width $_icons(molecule)]
    set h [image height $_icons(molecule)]
    set dx [expr {round($x1-$x0)}]
    set dy [expr {round(double($x1-$x0)/$w*$h)}]
    set imh [image create photo -width $dx -height $dy]
    blt::winop resample $_icons(molecule) $imh

    $c create image $x $y -anchor w -image $imh -tags image
}

# ----------------------------------------------------------------------
# USAGE: _drawAnnotation <index> <x0> <x1>
#
# Used internally within _redraw to draw one material layer at the
# <index> within the slab list into the active area.  The layer is
# drawn between coordinates <x0> and <x1> on the canvas.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_drawAnnotation {index x0 x1} {
    set c $itk_component(area)
    set h [expr {[winfo height $c]-1}]
    # a little extra height for the molecule image
    if {"" != [$_device element components.molecule]} {
        set h [expr {$h-15}]
    }

    set y0 $h
    set y1 [expr {$y0-$_sizes(bar)-$_sizes(bar45)}]

    set x0p [expr {$x0+$_sizes(bar45)}]
    set x1p [expr {$x1+$_sizes(bar45)}]
    set xmid [expr {0.5*($x0p+$x1p)}]

    set fnt $itk_option(-font)
    set lh [font metrics $fnt -linespace]
    set ymid [expr {$y1-2-0.5*$lh}]
    set y [expr {$y1-2}]

    #
    # If there's a .thickness control for this slab, draw it here.
    #
    set elem [lindex $_slabs $index]
    set path "structure.$elem.thickness"
    if {[info exists _controls($path)] && $_controls($path)} {
        set zthick [lindex $_zthick $index]
        set zthick [Rappture::Units::convert $zthick -context um -to um]

        $c create line $x0p $y $x0p [expr {$y-$lh}]
        $c create line $x1p $y $x1p [expr {$y-$lh}]

        $c create line $x0p $ymid $x1p $ymid -arrow both
        $c create text $xmid [expr {$ymid-2}] -anchor s -text $zthick
        set y [expr {$y-2.0*$lh}]
    }

    #
    # If there's a .material control for this slab, draw it here.
    #
    set elem [lindex $_slabs $index]
    set path "structure.$elem.material"
    if {[info exists _controls($path)] && $_controls($path)} {
        set mater [lindex $_maters $index]
        set w [expr {12+[font measure $fnt $mater]}]
        set x [expr {$x1p - 0.5*($x1p-$x0p-$w)}]
        $c create rectangle [expr {$x-10}] [expr {$y-10}] \
            $x $y -outline black -fill [_mater2color $mater]
        $c create text [expr {$x-12}] [expr {$y-5}] -anchor e \
            -text $mater
        set y [expr {$y-1.2*$lh}]
    }

    #
    # If there's a <label> for this layer, then draw it.
    #
    if {"" != $_device} {
        set label [$_device get $elem.about.label]
        if {"" != $label} {
            set y [expr {$y-0.5*$lh}]
            $c create text [expr {0.5*($x0p+$x1p)}] $y -anchor s \
                -text $label
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _mater2color <material>
#
# Used internally to convert a <material> name to the color that
# should be used to represent it in the viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_mater2color {mater} {
    if {$_library != ""} {
        set color [$_library get materials.($mater).color]
        if {$color != ""} {
            return $color
        }
    }
    return gray
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -font
#
# Controls the font used to all text on the layout, including
# annotations.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::font {
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -library
#
# Set to the Rappture::Library object representing the library with
# material properties and other info.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::library {
    if {$itk_option(-library) != ""} {
        if {![Rappture::library isvalid $itk_option(-library)]} {
            error "bad value \"$itk_option(-library)\": should be Rappture::Library"
        }
    }
    set _library $itk_option(-library)
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -device
#
# Set to the Rappture::Library object representing the device being
# displayed in the viewer.  If set to "", the viewer is cleared to
# display nothing.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::device {
    if {$itk_option(-device) != ""} {
        if {![Rappture::library isvalid $itk_option(-device)]} {
            error "bad value \"$itk_option(-device)\": should be Rappture::Library"
        }
    }
    set _device $itk_option(-device)
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -devicesize
#
# Sets the height of the bar representing the device area.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::devicesize {
    if {[catch {
          winfo pixels $itk_component(hull) $itk_option(-devicesize)
      } pixels]} {
        error "bad screen distance \"$itk_option(-devicesize)\""
    }
    set _sizes(bar) $pixels
    set _sizes(bar45) [expr {0.5*sqrt(2)*$pixels}]

    set pixels [winfo pixels $itk_component(hull) $itk_option(-rightmargin)]
    set _sizes(rm) [expr {($pixels > 0) ? $pixels : $_sizes(bar45)}]

    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -deviceoutline
#
# Sets the outline color around the various slabs in the device.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::deviceoutline {
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -leftmargin
#
# Sets the width of the left margin, a blank area along the left
# side.  Setting the margin allows the device layout to line up
# with a graph of internal fields.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::leftmargin {
    if {[catch {
          winfo pixels $itk_component(hull) $itk_option(-leftmargin)
      } pixels]} {
        error "bad screen distance \"$itk_option(-leftmargin)\""
    }
    set _sizes(lm) $pixels
    $_dispatcher event -idle !layout
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -rightmargin
#
# Sets the width of the right margin, a blank area along the right
# side.  Setting the margin allows the device layout to line up
# with a graph of internal fields.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::rightmargin {
    if {[catch {
          winfo pixels $itk_component(hull) $itk_option(-rightmargin)
      } pixels]} {
        error "bad screen distance \"$itk_option(-rightmargin)\""
    }
    set _sizes(rm) [expr {($pixels > 0) ? $pixels : $_sizes(bar45)}]
    $_dispatcher event -idle !layout
    $_dispatcher event -idle !redraw
}
