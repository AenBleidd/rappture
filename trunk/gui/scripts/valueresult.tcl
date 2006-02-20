# ----------------------------------------------------------------------
#  COMPONENT: ValueResult - Log output for ResultSet
#
#  This widget is used to show text output in a ResultSet.  The log
#  output from a tool, for example, is rendered as a ValueResult.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *ValueResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault
option add *ValueResult.boldFont \
    -*-helvetica-bold-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::ValueResult {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {option}

    set _dataobj ""  ;# data object currently being displayed
}
                                                                                
itk::usual ValueResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::constructor {args} {
    itk_component add label {
        label $itk_interior.l
    }
    pack $itk_component(label) -side left

    itk_component add value {
        label $itk_interior.value -anchor w
    } {
        usual
        rename -font -boldfont boldFont Font
        ignore -foreground
    }
    pack $itk_component(value) -side left -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  If the optional
# <settings> are specified, then the are applied to the data.  Allowed
# settings are -color and -brightness, -width, -linestyle and -raise.
# (Many of these are ignored.)
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::add {dataobj {settings ""}} {
    array set params {
        -color ""
        -brightness ""
        -width ""
        -linestyle ""
        -raise ""
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

    $itk_component(label) configure -text ""
    $itk_component(value) configure -text ""

    if {"" != $dataobj} {
        set label [$dataobj get about.label]
        if {"" != $label && [string index $label end] != ":"} {
            append label ":"
        }
        $itk_component(label) configure -text $label

        # find the value and assign it with the proper coloring
        if {"" != $params(-color) && "" != $params(-brightness)
              && $params(-brightness) != 0} {
            set params(-color) [Rappture::color::brightness \
                $params(-color) $params(-brightness)]
        }
        if {$params(-color) != ""} {
            $itk_component(value) configure -foreground $params(-color)
        } else {
            $itk_component(value) configure -foreground $itk_option(-foreground)
        }
        $itk_component(value) configure -text [$dataobj get current]
    }
    set _dataobj $dataobj
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::get {} {
    return $_dataobj
}

# ----------------------------------------------------------------------
# USAGE: delete ?<curve1> <curve2> ...?
#
# Clients use this to delete a curve from the plot.  If no curves
# are specified, then all curves are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::delete {args} {
    $itk_component(label) configure -text ""
    $itk_component(value) configure -text ""
    set _dataobj ""
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
itcl::body Rappture::ValueResult::scale {args} {
    # nothing to do for values
}

# ----------------------------------------------------------------------
# USAGE: download coming
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::download {option} {
    switch $option {
        coming {
            # nothing to do
        }
        now {
            set lstr [$itk_component(label) cget -text]
            set vstr [$itk_component(value) cget -text]
            return [list .txt "$lstr $vstr"]
        }
        default {
            error "bad option \"$option\": should be coming, now"
        }
    }
}
