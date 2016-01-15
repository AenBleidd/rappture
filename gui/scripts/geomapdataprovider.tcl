# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataprovider -
#               holds data source information for a geomap layer
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


itcl::class Rappture::GeoMapDataProvider {
    constructor {type driver url args} {
        # defined below
    }
    destructor {
        # defined below
    }

    private variable _type ""
    private variable _driver ""
    private variable _url ""

    public variable attribution ""
    public variable cache "true"

    protected method Type { args }
    protected method Driver { args }
    protected method Url { args }

    public method type { }
    public method driver { }
    public method url { }

    public method exportToBltTree { tree }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::constructor {type driver url args} {

    Type $type
    Driver $driver
    Url $url

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::destructor {} {

}


# ----------------------------------------------------------------------
# Type: get/set type of layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::Type {args} {

    set valids {image elevation feature icon line point polygon text}


    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?type?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        if {[lsearch $valids $value] < 0} {
            error "bad value \"$value\": should be one of \"$valids\""
        }

        set _type $value
    }

    return $_type
}


# ----------------------------------------------------------------------
# Driver: get/set driver for layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::Driver {args} {

    set valids {arcgis colorramp debug gdal ogr tfs tms wcs wfs wms xyz}

    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?driver?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        if {[lsearch $valids $value] < 0} {
            error "bad value \"$value\": should be one of \"$valids\""
        }

        set _driver $value
    }

    return $_driver
}


# ----------------------------------------------------------------------
# Url: get/set url of the layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::Url {args} {

    if {[llength $args] > 1} {
        error "wrong # of arguments: should be ?url?"
    }

    if {[llength $args] == 1} {

        set value [lindex $args 0]

        if {[string compare "" $value] == 0} {
            error "bad value \"$value\": should be a non-empty string"
        }

        # this re is fragile when some urls are missing ending forward slashes:
        # http://myurl.com vs. http://myurl.com/
        set re {[^:]+://([^:/]+(:[0-9]+)?)?/.*}
        if {[regexp $re $value] == 0} {
            error "bad value \"$value\": should be a valid url or url pattern"
        }

        set _url $value
    }

    return $_url
}


# ----------------------------------------------------------------------
# attribution: get/set the attribution text for the layer
#
# Tells the server what copyright text to show for layer.
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProvider::attribution {
    # empty
}


# ----------------------------------------------------------------------
# cache: get/set the cache flag for the layer
#
# Tells the server whether or not to cache images from
# external sources.
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProvider::cache {

    if {[string is bool $cache] == 0} {
        error "bad value \"$cache\": should be a boolean"
    }
}


# ----------------------------------------------------------------------
# type: clients use this public method to query type of this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::type {} {
    return [Type]
}


# ----------------------------------------------------------------------
# driver: clients use this public method to query the driver for this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::driver {} {
    return [Driver]
}


# ----------------------------------------------------------------------
# url: clients use this public method to query the initial
#      value of the url or this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::url {} {
    return [Url]
}


# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProvider::exportToBltTree {tree} {

    $tree set root \
        type $_type \
        driver $_driver \
        cache $cache \
        attribution $attribution
}

