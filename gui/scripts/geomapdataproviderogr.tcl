# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataproviderogr -
#               holds data source information for a geomap
#               vector ogr layer
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


itcl::class Rappture::GeoMapDataProviderOGR {
    inherit Rappture::GeoMapDataProvider

    constructor {type url args} {
        Rappture::GeoMapDataProvider::constructor $type "ogr" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    protected method Type { args }

    public method exportToBltTree { tree }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderOGR::constructor {type url args} {

    Type $type
    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderOGR::destructor {} {

}


# ----------------------------------------------------------------------
# Type: get/set type of layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderOGR::Type {args} {

    set valids {icon line point polygon text}


    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?type?"
    }

    set value [Rappture::GeoMapDataProvider::Type]

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        if {[lsearch $valids $value] < 0} {
            error "bad value \"$value\": should be one of \"$valids\""
        }

        Rappture::GeoMapDataProvider::Type $value
    }

    return $value
}


# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderOGR::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        ogr.url [url]
}

