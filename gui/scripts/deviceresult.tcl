# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: DeviceResult - output for <structure>
#
#  This widget is used to show <structure> output.  It is similar
#  to a DeviceEditor, but does not allow editing.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *DeviceResult.width 4i widgetDefault
option add *DeviceResult.height 4i widgetDefault
option add *DeviceResult.font \
    -*-courier-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::DeviceResult {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    private variable _dataobj ""  ;# data object currently being displayed
}

itk::usual DeviceResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceResult::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add viewer {
        # turn off auto-cleanup -- resultset swaps results in and out
        Rappture::DeviceEditor $itk_interior.dev "" -autocleanup no
    }
    pack $itk_component(viewer) -expand yes -fill both

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
itcl::body Rappture::DeviceResult::add {dataobj {settings ""}} {
    array set params {
        -color ""
        -brightness ""
        -width ""
        -linestyle ""
        -raise ""
        -description ""
        -param ""
    }
    array set params $settings

    eval $itk_component(viewer) add $dataobj [list $settings]

    set _dataobj $dataobj
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceResult::get {} {
    return $_dataobj
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceResult::delete {args} {
    eval $itk_component(viewer) delete $args
    set _dataobj ""
}

# ----------------------------------------------------------------------
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all dataobjs--even those not showing on the screen.
# Because of this, the limits are appropriate for all dataobjs as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceResult::scale {args} {
    # nothing to do for structures
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
itcl::body Rappture::DeviceResult::download {option args} {
    return [eval $itk_component(viewer) download $option $args]
}
