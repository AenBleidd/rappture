# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: field3dresult - plot a field in a ResultSet
#
#  This widget visualizes scalar/vector fields on 3D meshes.
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

option add *Field3DResult.width 4i widgetDefault
option add *Field3DResult.height 4i widgetDefault
option add *Field3DResult.foreground black widgetDefault
option add *Field3DResult.controlBackground gray widgetDefault
option add *Field3DResult.controlDarkBackground #999999 widgetDefault
option add *Field3DResult.plotBackground black widgetDefault
option add *Field3DResult.plotForeground white widgetDefault
option add *Field3DResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Field3DResult {
    inherit itk::Widget

    itk_option define -mode mode Mode "auto"

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
    set servers ""
    switch -- $flags(-mode) {
        "auto" - "nanovis" - "flowvis" {
            set servers [Rappture::VisViewer::GetServerList "nanovis"]
        }
        "isosurface" - "heightmap" - "streamlines" - "vtkviewer" - "vtkvolume" - "glyphs" {
            set servers [Rappture::VisViewer::GetServerList "vtkvis"]
        }
        "vtk" {
            # Old vtk contour widget
        }
        default {
            puts stderr "unknown render mode \"$flags(-mode)\""
        }
    }
    if { "" != $servers && $flags(-mode) != "vtk"} {
        switch -- $flags(-mode) {
            "auto" - "nanovis" {
                itk_component add renderer {
                    Rappture::NanovisViewer $itk_interior.ren $servers
                }
            }
            "flowvis" {
                itk_component add renderer {
                    Rappture::FlowvisViewer $itk_interior.ren $servers
                }
            }
            "glyphs" {
                itk_component add renderer {
                    Rappture::VtkGlyphViewer $itk_interior.glyphs $servers
                }
            }
            "contour" - "heightmap" {
                itk_component add renderer {
                    Rappture::VtkHeightmapViewer $itk_interior.ren $servers
                }
            }
            "isosurface" {
                itk_component add renderer {
                    Rappture::VtkIsosurfaceViewer $itk_interior.ren $servers
                }
            }
            "streamlines" {
                itk_component add renderer {
                    Rappture::VtkStreamlinesViewer $itk_interior.ren $servers
                }
            }
            "vtkviewer" {
                itk_component add renderer {
                    Rappture::VtkViewer $itk_interior.ren $servers
                }
            }
            "vtkvolume" {
                itk_component add renderer {
                    Rappture::VtkVolumeViewer $itk_interior.ren $servers
                }
            }
            default {
                puts stderr "unknown render mode \"$flags(-mode)\""
            }
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
    eval $itk_component(renderer) download $option $args
}

itcl::body Rappture::Field3DResult::snap { w h } {
    return [$itk_component(renderer) snap $w $h]
}
