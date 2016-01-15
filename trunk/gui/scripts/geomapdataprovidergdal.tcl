# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataprovidergdal -
#               holds data source information for a geomap
#               raster gdal layer
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


itcl::class Rappture::GeoMapDataProviderGdal {
    inherit Rappture::GeoMapDataProvider

    constructor {url args} {
        Rappture::GeoMapDataProvider::constructor "image" "gdal" $url
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
itcl::body Rappture::GeoMapDataProviderGdal::constructor {url args} {

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderGdal::destructor {} {

}

# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderGdal::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        gdal.url [url]
}

