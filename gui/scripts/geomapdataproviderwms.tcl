# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataproviderwms -
#               holds data source information for a geomap
#               raster wms layer
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture {
    # forward declaration
}


itcl::class Rappture::GeoMapDataProviderWMS {
    inherit Rappture::GeoMapDataProvider

    constructor {url layers format args} {
        Rappture::GeoMapDataProvider::constructor "image" "wms" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    private variable _layers ""
    private variable _format ""

    public variable transparent "false"

    public method layers { args }
    public method format { args }

    public method exportToBltTree { tree }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWMS::constructor {url layers format args} {

    layers $layers
    format $format

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWMS::destructor {} {

}


# ----------------------------------------------------------------------
# layers: set/get the layer names to be requested from this data source
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWMS::layers { args } {

    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?layers?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        # layers should be non-empty list
        if {[llength $value] == 0} {
            error "bad value \"$value\": should be a non-empty list of strings"
        }

        set _layers $value
    }

    return $_layers
}


# ----------------------------------------------------------------------
# format: set/get the image format that will be returned from the WMS server
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWMS::format { args } {

    set valids {png jpeg}

    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?format?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        if {[lsearch $valids $value] < 0} {
            error "bad value \"$value\": should be one of \"$valids\""
        }

        set _format $value
    }

    return $_format
}


# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWMS::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        wms.url [url] \
        wms.layers $_layers \
        wms.format $_format \
        wms.transparent $transparent \
}


# ----------------------------------------------------------------------
# transparent: get/set the transparent flag for the layer
#
# Tells the geovis server whether or not to request a transparent image
# from the WMS server, which allows for lower layers to show through
# this layer where no data exists. Only works for PNG image format.
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProviderWMS::transparent {

    if {[string compare "" $transparent] == 0} {
        error "bad value \"$transparent\": should be a non-empty boolean"
    }

    if {[string is bool $transparent] == 0} {
        error "bad value \"$transparent\": should be a boolean"
    }
}


