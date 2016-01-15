# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataprovidertfs -
#               holds data source information for a geomap
#               vector tfs layer
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


itcl::class Rappture::GeoMapDataProviderTFS {
    inherit Rappture::GeoMapDataProvider

    constructor {type url format args} {
        Rappture::GeoMapDataProvider::constructor $type "tfs" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    private variable _format "json"

    protected method Type { args }

    public method exportToBltTree { tree }
    public method format { args }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTFS::constructor {type url format args} {

    Type $type
    format $format
    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTFS::destructor {} {

}


# ----------------------------------------------------------------------
# Type: get/set type of layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTFS::Type {args} {

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
# format: get/set format that the TFS server should return
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTFS::format {args} {

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
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderTFS::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        tfs.url [url] \
        tfs.format $_format
}

