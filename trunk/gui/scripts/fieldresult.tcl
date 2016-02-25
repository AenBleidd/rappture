# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: fieldresult - plot a field in a ResultSet
#
#  This widget visualizes fields on meshes.
#  It is normally used in the ResultViewer to show results from the
#  run of a Rappture tool.  Use the "add" and "delete" methods to
#  control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *FieldResult.width 4i widgetDefault
option add *FieldResult.height 4i widgetDefault
option add *FieldResult.foreground black widgetDefault
option add *FieldResult.controlBackground gray widgetDefault
option add *FieldResult.controlDarkBackground #999999 widgetDefault
option add *FieldResult.plotBackground black widgetDefault
option add *FieldResult.plotForeground white widgetDefault
option add *FieldResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::FieldResult {
    inherit itk::Widget

    itk_option define -mode mode Mode ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method snap {w h}
    public method parameters {title args} { # do nothing }
    public method download {option args}
}

itk::usual FieldResult {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FieldResult::constructor {args} {
    array set flags {
        -mode ""
    }
    array set flags $args
    switch -- $flags(-mode) {
        "nanovis" {
            itk_component add renderer {
                Rappture::NanovisViewer $itk_interior.ren
            }
        }
        "flowvis" {
            itk_component add renderer {
                Rappture::FlowvisViewer $itk_interior.ren
            }
        }
        "glyphs" {
            itk_component add renderer {
                Rappture::VtkGlyphViewer $itk_interior.ren
            }
        }
        "contour" - "heightmap" {
            itk_component add renderer {
                Rappture::VtkHeightmapViewer $itk_interior.ren \
                    -mode $flags(-mode)
            }
        }
        "isosurface" {
            itk_component add renderer {
                Rappture::VtkIsosurfaceViewer $itk_interior.ren
            }
        }
        "streamlines" {
            itk_component add renderer {
                Rappture::VtkStreamlinesViewer $itk_interior.ren
            }
        }
        "surface" {
            itk_component add renderer {
                Rappture::VtkSurfaceViewer $itk_interior.ren
            }
        }
        "vtkimage" {
            itk_component add renderer {
                Rappture::VtkImageViewer $itk_interior.ren
            }
        }
        "vtkviewer" {
            itk_component add renderer {
                Rappture::VtkViewer $itk_interior.ren
            }
        }
        "vtkvolume" {
            itk_component add renderer {
                Rappture::VtkVolumeViewer $itk_interior.ren
            }
        }
        default {
            puts stderr "unknown render mode \"$flags(-mode)\""
        }
    }
    pack $itk_component(renderer) -expand yes -fill both

    # can't connect to rendering farm?
    if {![$itk_component(renderer) isconnected]} {
        # Should show a message here
        #destroy $itk_component(renderer)
    }

    if {![info exists itk_component(renderer)]} {
        # No valid renderer (or couldn't connect?)
        # Should show a message here
    }
    eval itk_initialize $args
    #update
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FieldResult::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::FieldResult::add {dataobj {settings ""}} {
    if { ![info exists itk_component(renderer)] } {
        puts stderr "add: no renderer created."
        return
    }
    eval $itk_component(renderer) add $dataobj [list $settings]
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::FieldResult::get {} {
    if { ![info exists itk_component(renderer)] } {
        puts stderr "get: no renderer created."
        return
    }
    return [$itk_component(renderer) get]
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::FieldResult::delete {args} {
    if { [info exists itk_component(renderer)] } {
        eval $itk_component(renderer) delete $args
    }
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
itcl::body Rappture::FieldResult::scale {args} {
    if { ![info exists itk_component(renderer)] } {
        puts stderr "scale: no renderer created."
        return
    }
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
itcl::body Rappture::FieldResult::download {option args} {
    if { ![info exists itk_component(renderer)] } {
        puts stderr "download: no renderer created."
        return
    }
    eval $itk_component(renderer) download $option $args
}

itcl::body Rappture::FieldResult::snap { w h } {
    if { ![info exists itk_component(renderer)] } {
        puts stderr "snap: no renderer created."
        return
    }
    return [$itk_component(renderer) snap $w $h]
}
