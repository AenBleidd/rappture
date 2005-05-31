# ----------------------------------------------------------------------
#  COMPONENT: TextResult - Log output for ResultSet
#
#  This widget is used to show text output in a ResultSet.  The log
#  output from a tool, for example, is rendered as a TextResult.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *TextResult.width 4i widgetDefault
option add *TextResult.height 4i widgetDefault
option add *TextResult.font \
    -*-courier-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::TextResult {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}}
    public method delete {args}
    public method scale {args}
}
                                                                                
itk::usual TextResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add text {
        text $itk_component(scroller).text -width 1 -height 1 -wrap none
    }
    $itk_component(scroller) contents $itk_component(text)
    $itk_component(text) configure -state disabled

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  If the optional
# <settings> are specified, then the are applied to the data.  Allowed
# settings are -color and -width/-raise (ignored).
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::add {dataobj {settings ""}} {
    array set params {
        -color ""
        -width ""
        -raise ""
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    $itk_component(text) configure -state normal
    $itk_component(text) delete 1.0 end

    if {"" != $dataobj} {
        set txt [$dataobj get]
        if {"" != $params(-color)} {
#
# ignore color for now -- may use it some day
#
#            $itk_component(text) insert end $txt special
#            $itk_component(text) tag configure special \
#                -foreground $params(-color)
            $itk_component(text) insert end $txt
        } else {
            $itk_component(text) insert end $txt
        }
    }

    $itk_component(text) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: delete ?<curve1> <curve2> ...?
#
# Clients use this to delete a curve from the plot.  If no curves
# are specified, then all curves are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::delete {args} {
    $itk_component(text) configure -state normal
    $itk_component(text) delete 1.0 end
    $itk_component(text) configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: scale ?<curve1> <curve2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <curve> objects.  This
# accounts for all curves--even those not showing on the screen.
# Because of this, the limits are appropriate for all curves as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::scale {args} {
    # nothing to do for text
}
