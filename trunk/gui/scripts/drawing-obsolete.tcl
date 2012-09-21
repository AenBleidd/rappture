# ----------------------------------------------------------------------
#  COMPONENT: drawingobsolete - 2D drawing of data
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *DrawingObsolete.width 4i widgetDefault
option add *DrawingObsolete.height 3i widgetDefault

itcl::class Rappture::DrawingObsolete {
    inherit itk::Widget

#    constructor {owner path args} {
#        Rappture::ControlOwner::constructor $owner
#    } { # defined below }
    constructor {owner path args} { # defined below }
private variable _xmlobj ""
private variable _owner ""
private variable _path ""

    public method value {args}
    public method hilite {elem state}
    public method activate {elem x y}

    protected method _buildPopup {elem}

    private variable _elem2popup  ;# maps drawing elem => popup of controls
}

itk::usual DrawingObsolete {
    keep -cursor -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingObsolete::constructor {owner path args} {
    set _owner $owner
    set _path $path

    itk_component add canvas {
        canvas $itk_interior.canv
    } {
        usual
        keep -width -height
    }
    pack $itk_component(canvas) -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingObsolete::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 0} {
        return $_xmlobj
    } elseif {[llength $args] > 1} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    if {$onlycheck} {
        # someday we may add validation...
        return
    }

    set c $itk_component(canvas)
    $c delete all
    foreach elem [array names _elem2popup] {
        destroy $_elem2popup($elem)
    }
    catch {unset $elem}
    set _xmlobj [lindex $args 0]

    if {"" != $_xmlobj} {
        foreach elem [$_xmlobj children] {
            switch -glob -- $elem {
                about* {
                    continue
                }
                rectangle* {
                    set xy0 [$_xmlobj get $elem.xya]
                    set xy1 [$_xmlobj get $elem.xyb]
                    set fill [$_xmlobj get $elem.fill]
                    set outl [$_xmlobj get $elem.outl]
                    set wdth [$_xmlobj get $elem.width]
                    if {"" != $wdth && $wdth > 0 && $outl == ""} {
                        set outl black
                    }
                    if {"" == $fill && "" == $outl} {
                        _buildPopup $elem

                        # this part gets highlighted
                        eval $c create rect $xy0 $xy1 [list -outline "" -fill "" -tags $elem]
                        # this part is the sensor that detects events
                        set id [eval $c create rect $xy0 $xy1 [list -outline "" -fill ""]]
                        $c bind $id <Enter> [itcl::code $this hilite $elem on]
                        $c bind $id <Leave> [itcl::code $this hilite $elem off]
                        $c bind $id <ButtonPress> [itcl::code $this activate $elem %X %Y]
                    } else {
                        if {"" == $fill} { set fill "{}" }
                        if {"" == $outl} { set outl "{}" }
                        eval $c create rect $xy0 $xy1 -width $wdth -outline $outl -fill $fill
                    }
                }
                oval* {
                    set xy0 [$_xmlobj get $elem.xya]
                    set xy1 [$_xmlobj get $elem.xyb]
                    set fill [$_xmlobj get $elem.fill]
                    set outl [$_xmlobj get $elem.outl]
                    set wdth [$_xmlobj get $elem.width]
                    if {"" != $wdth && $wdth > 0 && $outl == ""} {
                        set outl black
                    }
                    if {"" == $fill && "" == $outl} {
                    } else {
                        if {"" == $fill} { set fill "{}" }
                        if {"" == $outl} { set outl "{}" }
                    }
                    eval $c create oval $xy0 $xy1 -width $wdth -outline $outl -fill $fill
                }
                polygon* {
                    set xy [$_xmlobj get $elem.xy]
                    regsub -all "\n" $xy " " xy
                    set smth [$_xmlobj get $elem.smooth]
                    set fill [$_xmlobj get $elem.fill]
                    set outl [$_xmlobj get $elem.outl]
                    set wdth [$_xmlobj get $elem.width]
                    if {"" != $wdth && $wdth > 0 && $outl == ""} {
                        set outl black
                    }
                    if {"" == $fill && "" == $outl} {
                    } else {
                        if {"" == $fill} { set fill "{}" }
                        if {"" == $outl} { set outl "{}" }
                    }
                    eval $c create polygon $xy -width $wdth -outline $outl -fill $fill -smooth $smth
                }
                text* {
                    set xy [$_xmlobj get $elem.xy]
                    set txt [$_xmlobj get $elem.text]
                    eval $c create text $xy -text $txt
                }
            }
        }
    }
    return $_xmlobj
}

# ----------------------------------------------------------------------
# USAGE: hilite <elem> <state>
#
# Used internally to highlight parts of the drawing when the mouse
# passes over it.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingObsolete::hilite {elem state} {
    if {$state} {
        $itk_component(canvas) itemconfigure $elem -outline red -width 2
    } else {
        $itk_component(canvas) itemconfigure $elem -width 0
    }
}

# ----------------------------------------------------------------------
# USAGE: activate <elem> <x> <y>
#
# Pops up the controls associated with a drawing element.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingObsolete::activate {elem x y} {
    if {[info exists _elem2popup($elem)]} {
        after 10 [list $_elem2popup($elem) activate @$x,$y above]
    }
}

# ----------------------------------------------------------------------
# USAGE: _buildPopup <elem>
#
# Pops up the controls associated with a drawing element.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingObsolete::_buildPopup {elem} {
    if {![info exists _elem2popup($elem)]} {
        set n 0
        while {1} {
            set popup $itk_component(canvas).popup[incr n]
            if {![winfo exists $popup]} {
                break
            }
        }
        Rappture::Balloon $popup -title [$_xmlobj get $elem.parameters.about.label]
        set inner [$popup component inner]
        Rappture::Controls $inner.cntls $_owner
        pack $inner.cntls -expand yes -fill both
        foreach child [$_xmlobj children $elem.parameters] {
            if {[string match about* $child]} {
                continue
            }
            $inner.cntls insert end $_path.$elem.parameters.$child
        }

        set _elem2popup($elem) $popup
    }
}
