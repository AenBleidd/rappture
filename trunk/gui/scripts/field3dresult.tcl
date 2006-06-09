# ----------------------------------------------------------------------
#  COMPONENT: field3dresult - plot a field in a ResultSet
#
#  This widget visualizes scalar/vector fields on 3D meshes.
#  It is normally used in the ResultViewer to show results from the
#  run of a Rappture tool.  Use the "add" and "delete" methods to
#  control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Field3DResult.width 4i widgetDefault
option add *Field3DResult.height 4i widgetDefault
option add *Field3DResult.foreground black widgetDefault
option add *Field3DResult.controlBackground gray widgetDefault
option add *Field3DResult.controlDarkBackground #999999 widgetDefault
option add *Field3DResult.plotBackground black widgetDefault
option add *Field3DResult.plotForeground white widgetDefault
option add *Field3DResult.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Field3DResult {
    inherit itk::Widget

    itk_option define -mode mode Mode "auto"

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method download {option args}

    # resources file tells us the nanovis server
    public common _nanovisHosts ""
    public proc setNanovisServer {namelist} {
        if {[regexp {^[a-zA-Z0-9\.]+:[0-9]+(,[a-zA-Z0-9\.]+:[0-9]+)*$} $namelist match]} {
            set _nanovisHosts $namelist
        } else {
            error "bad nanovis server address \"$namelist\": should be host:port,host:port,..."
        }
    }
}

# must use this name -- plugs into Rappture::resources::load
proc field3d_init_resources {} {
    Rappture::resources::register \
        nanovis_server Rappture::Field3DResult::setNanovisServer
}

itk::usual Field3DResult {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field3DResult::constructor {args} {
    array set flags {
        -mode auto
    }
    array set flags $args

    if {"" != $_nanovisHosts && $flags(-mode) != "vtk"} {
        itk_component add renderer {
            Rappture::NanovisViewer $itk_interior.ren $_nanovisHosts
        }
        pack $itk_component(renderer) -expand yes -fill both

        # can't connect to rendering farm?  then fall back to older viewer
        if {![$itk_component(renderer) isconnected]} {
            destroy $itk_component(renderer)
        }
    }

    if {![info exists itk_component(renderer)]} {
        itk_component add renderer {
            Rappture::ContourResult $itk_interior.ren
        }
        pack $itk_component(renderer) -expand yes -fill both
    }

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Field3DResult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::Field3DResult::add {dataobj {settings ""}} {
    eval $itk_component(renderer) add $dataobj [list $settings]
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::Field3DResult::get {} {
    return [$itk_component(renderer) get]
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Field3DResult::delete {args} {
    eval $itk_component(renderer) delete $args
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
itcl::body Rappture::Field3DResult::scale {args} {
    eval $itk_component(renderer) scale $args
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
itcl::body Rappture::Field3DResult::download {option args} {
    $itk_component(renderer) download $option
}
