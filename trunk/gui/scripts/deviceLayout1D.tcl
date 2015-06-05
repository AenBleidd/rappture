# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *DeviceLayout1D.width 3.5i widgetDefault
option add *DeviceLayout1D.deviceSize 0.25i widgetDefault
option add *DeviceLayout1D.deviceOutline black widgetDefault
option add *DeviceLayout1D.annotate all widgetDefault
option add *DeviceLayout1D.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::DeviceLayout1D {
    inherit itk::Widget

    itk_option define -font font Font ""
    itk_option define -device device Device ""
    itk_option define -width width Width 0
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
    protected method _drawIcon {index x0 x1 imh}
    protected method _drawAnnotation {index x0 x1}
    protected method _mater2color {mater}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _sizes         ;# maps size name => pixels

    private variable _device ""     ;# LibraryObj for device representation
    private variable _slabs ""      ;# list of node names for slabs in device
    private variable _z0 ""         ;# list parallel to _slabs with z0
                                    ;#   coord for lhs of each slab
    private variable _z1 ""         ;# list parallel to _slabs with z1
                                    ;#   coord for rhs of each slab
    private variable _maters ""     ;# list parallel to _slabs with material
                                    ;#   for each slab
    private variable _colors ""     ;# list parallel to _slabs with color
                                    ;#   for each slab

    private variable _controls      ;# maps control path => status on/off

    private variable _icons         ;# maps icon data => image handle
}

itk::usual DeviceLayout1D {
    keep -background -cursor
    keep -device -deviceoutline -devicesize
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

    pack propagate $itk_component(hull) no

    itk_component add area {
        canvas $itk_interior.area -borderwidth 0 -highlightthickness 0
    } {
        usual
        ignore -borderwidth -relief
        ignore -highlightthickness -highlightbackground -highlightcolor
    }
    pack $itk_component(area) -expand yes -fill both
    bind $itk_component(area) <Configure> \
        [list $_dispatcher event -idle !redraw]

    eval itk_initialize $args

    set _sizes(header) 1
    set _sizes(bararea) 1
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
    set zmax [lindex $_z1 end]
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
    #
    # First, recompute the overall width/height of this widget...
    #
    # requested minimum size
    set wmax [winfo pixels $itk_component(hull) $itk_option(-width)]
    if {$wmax <= 0} { set wmax 100 }

    # size of an ordinary material bar:
    set hmax [expr {$_sizes(bar)+$_sizes(bar45)+2}]

    # add the maximum size of any embedded icons:
    if {$_device != ""} {
        foreach nn [$_device children components] {
            set icon [$_device get components.$nn.about.icon]
            if {"" != $icon} {
                if {[info exists _icons($icon)]} {
                    set imh $_icons($icon)
                } else {
                    set imh [image create photo -data $icon]
                    set _icons($icon) $imh
                }

                set w [image width $_icons($icon)]
                if {$w > $wmax} {
                    set wmax $w
                }

                set h [image height $_icons($icon)]
                if {$h > $hmax} {
                    set hmax $h
                }
            }
        }
    }
    set _sizes(bararea) $hmax

    set fnt $itk_option(-font)
    # see if any of the slabs has a material
    foreach m $_maters {
        if {"" != $m} {
            set extra [expr {1.5*[font metrics $fnt -linespace]}]
            set hmax [expr {$hmax+$extra}]
            break
        }
    }

    # see if any of the slabs has a label
    if {$_device != ""} {
        foreach nn [$_device children components] {
            if {"" != [$_device get components.$nn.about.label]} {
                set extra [expr {1.2*[font metrics $fnt -linespace]}]
                set hmax [expr {$hmax+$extra}]
                break
            }
        }
    }

    set oldw [component hull cget -width]
    set oldh [component hull cget -height]
    if {$wmax != $oldw || $hmax != $oldh} {
        component hull configure -width $wmax -height $hmax
        $_dispatcher event -idle !redraw
    }
    set _sizes(header) [expr {$hmax - $_sizes(bararea)}]

    # next, scan through the device and compute layer positions
    set slabs ""
    set z0 ""
    set z1 ""
    set maters ""
    set colors ""

    if {$_device != ""} {
        # get the default system of units
        set units [set defunits [$_device get units]]
        if {$units == "arbitrary" || $units == ""} {
            set defunits "m"
            set units "um"
        }

        foreach nn [$_device children components] {
            switch -glob -- $nn {
                box* {
                    # get x-coord for each corner
                    set c0 [$_device get components.$nn.corner0]
                    regsub -all , $c0 { } c0
                    set c0 [lindex $c0 0]
                    set c0 [Rappture::Units::convert $c0 \
                        -context $defunits -to $units -units off]

                    set c1 [$_device get components.$nn.corner1]
                    regsub -all , $c1 { } c1
                    set c1 [lindex $c1 0]
                    set c1 [Rappture::Units::convert $c1 \
                        -context $defunits -to $units -units off]

                    lappend slabs components.$nn
                    lappend z0 $c0
                    lappend z1 $c1

                    set m [$_device get components.$nn.material]
                    lappend maters $m

                    if {"" != $m} {
                        set c [_mater2color $m]
                    } else {
                        set c [$_device get components.$nn.about.color]
                    }
                    if {"" == $c} { set c gray }
                    lappend colors $c
                }
                default {
                    # element not recognized -- skip it
                }
            }
        }
    }

    # something change? then store new layout and redraw
    if {![string equal $z0 $_z0]
          || ![string equal $z1 $_z1]
          || ![string equal $maters $_maters]
          || ![string equal $colors $_colors]} {
        set _slabs $slabs
        set _z0 $z0
        set _z1 $z1
        set _maters $maters
        set _colors $colors

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
    $c delete all

    set w [expr {[winfo width $c]-1 - $_sizes(lm) - $_sizes(rm)}]

    set x0 $_sizes(lm)
    set x1 [expr {$x0 + $w}]

    set zmax [lindex $_z1 end]
    set xx0 $x0
    set xx1 $x1

    for {set i 0} {$i < [llength $_slabs]} {incr i} {
        set name [lindex $_slabs $i]
        set z0 [lindex $_z0 $i]
        set z1 [lindex $_z1 $i]
        set xx0 [expr {double($z0)/$zmax * ($x1-$x0) + $x0}]
        set xx1 [expr {double($z1)/$zmax * ($x1-$x0) + $x0}]

        set icon [$_device get $name.about.icon]
        if {"" != $icon} {
            if {[info exists _icons($icon)]} {
                set imh $_icons($icon)
            } else {
                set imh [image create photo -data $icon]
                set _icons($icon) $imh
            }
            _drawIcon $i $xx0 $xx1 $imh
        } else {
            _drawLayer $i $xx0 $xx1
        }
        _drawAnnotation $i $xx0 $xx1
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
    set h [expr {$_sizes(header) + $_sizes(bararea) - 1}]

    set bsize [expr {$_sizes(bar)+$_sizes(bar45)+2}]
    set y0 [expr {$h - 0.5*$_sizes(bararea) + 0.5*$bsize}]
    set y0p [expr {$y0-$_sizes(bar45)}]
    set y1p [expr {$y0-$_sizes(bar)}]
    set y1 [expr {$y1p-$_sizes(bar45)}]

    set x0p [expr {$x0+$_sizes(bar45)}]
    set x1p [expr {$x1+$_sizes(bar45)}]

    set lcolor $itk_option(-deviceoutline)

    if {$index < [llength $_slabs]} {
        set fcolor [lindex $_colors $index]

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

        #
        # Draw the outline around the end cap
        #
        $c create line $x1 $y0  $x1 $y1p  $x1p $y1 -fill $lcolor
    }
}

# ----------------------------------------------------------------------
# USAGE: _drawIcon <index> <x0> <x1> <imh>
#
# Used internally within _redraw to draw a material layer that is
# represented by an icon.  The layer sits at <index> within the slab
# list into the active area.  The layer is drawn between coordinates
# <x0> and <x1> on the canvas.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceLayout1D::_drawIcon {index x0 x1 imh} {
    set c $itk_component(area)
    set h [expr {$_sizes(header) + $_sizes(bararea) - 1}]

    set bsize [expr {$_sizes(bar)+$_sizes(bar45)+2}]
    set y0 [expr {$h - 0.5*$_sizes(bararea) + 0.5*$bsize}]
    set y0p [expr {$y0-$_sizes(bar45)}]
    set y1p [expr {$y0-$_sizes(bar)}]
    set y1 [expr {$y1p-$_sizes(bar45)}]
    set x0p [expr {$x0+$_sizes(bar45)}]
    set x1p [expr {$x1+$_sizes(bar45)}]

    set xx0 [expr {0.5*($x0+$x0p)}]
    set xx1 [expr {0.5*($x1+$x1p)}]
    set y [expr {0.5*($y0+$y0p) + 0.5*($y1-$y0p)}]

    $c create image [expr {0.5*($xx0+$xx1)}] $y -anchor c -image $imh
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

    set ytop [expr {$_sizes(header)+1}]
    set x0p [expr {$x0+$_sizes(bar45)}]
    set x1p [expr {$x1+$_sizes(bar45)}]
    set xmid [expr {0.5*($x0p+$x1p)}]

    set fnt $itk_option(-font)
    set lh [font metrics $fnt -linespace]
    set ymid [expr {$ytop-2-0.5*$lh}]
    set y [expr {$ytop-4}]

    #
    # If there's a .material control for this slab, draw it here.
    #
    set elem [lindex $_slabs $index]
    set mater [lindex $_maters $index]
    if {"" != $mater} {
        set x $x1p
        $c create rectangle [expr {$x-10}] [expr {$y-14}] \
            [expr {$x-0}] [expr {$y-4}] \
            -outline black -fill [_mater2color $mater]
        set x [expr {$x-12}]
        $c create text $x [expr {$y-7}] -anchor e \
            -text $mater
        set y [expr {$y-1.5*$lh}]
    }

    #
    # If there's a <label> for this layer, then draw it.
    #
    if {"" != $_device} {
        set label [$_device get $elem.about.label]
        if {"" != $label} {
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
    set lib [Rappture::library standard]
    set color [$lib get materials.($mater).color]
    if {$color != ""} {
        return $color
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
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -width
#
# Sets the minimum overall width of the widget.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceLayout1D::width {
    if {[catch {
          winfo pixels $itk_component(hull) $itk_option(-width)
      } pixels]} {
        error "bad screen distance \"$itk_option(-width)\""
    }
    $_dispatcher event -idle !layout
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
