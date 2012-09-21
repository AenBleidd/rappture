# ----------------------------------------------------------------------
#  COMPONENT: CurveOutput - output viewer for curves (x/y plots)
#
#  This widget is able to visualize Rappture ObjVal objects of type
#  CurveValue.  It produces X/Y plots showing the curve.  Use the "add"
#  and "delete" methods to control the data objects shown on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::objects::CurveOutput {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}} {
        return [$itk_component(xy) add $dataobj $settings]
    }
    public method get {} {
        return [$itk_component(xy) get]
    }
    public method delete {args} {
        return [eval $itk_component(xy) delete $args]
    }
    public method scale {args} {
        return [eval $itk_component(xy) scale $args]
    }
    public method parameters {title args} {
        return [eval $itk_component(xy) parameters [list $title] $args]
    }
    public method download {option args} {
        return [eval $itk_component(xy) download $option $args]
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::objects::CurveOutput::constructor {args} {
    itk_component add xy {
        ::Rappture::XyResult $itk_interior.xy
    } {
        keep -background -foreground -cursor -font
        keep -autocolors -gridcolor -activecolor -dimcolor
    }
    pack $itk_component(xy) -expand yes -fill both

    eval itk_initialize $args
}
