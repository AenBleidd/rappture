# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataproviderwfs -
#               holds data source information for a geomap
#               vector wfs layer
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


itcl::class Rappture::GeoMapDataProviderWFS {
    inherit Rappture::GeoMapDataProvider

    constructor {type url typename format args} {
        Rappture::GeoMapDataProvider::constructor $type "wfs" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    private variable _format "json"
    private variable _typename ""

    protected method Type { args }

    public method exportToBltTree { tree }
    public method format { args }
    public method typename { args }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::constructor {type url typename format args} {

    Type $type
    typename $typename
    format $format
    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::destructor {} {

}


# ----------------------------------------------------------------------
# Type: get/set type of layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::Type {args} {

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
# format: get/set format that the WFS server should return
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::format {args} {

    set valids {json gml}


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
# typename: set/get the feature name to be requested from this data source
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::typename { args } {

    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?typename?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        set _typename $value
    }

    return $_typename
}


# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderWFS::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        wfs.url [url] \
        wfs.typename $_typename \
        wfs.format $_format
}

