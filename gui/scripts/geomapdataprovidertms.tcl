# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataprovidertms -
#               holds data source information for a geomap
#               raster tms layer
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


itcl::class Rappture::GeoMapDataProviderTMS {
    inherit Rappture::GeoMapDataProvider

    constructor {url args} {
        Rappture::GeoMapDataProvider::constructor "image" "tms" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    public method exportToBltTree { tree }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTMS::constructor {url args} {

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTMS::destructor {} {

}

# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTMS::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        tms.url [url]
}

