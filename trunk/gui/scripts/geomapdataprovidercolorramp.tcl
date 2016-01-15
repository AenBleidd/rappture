# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomapdataprovidercolorramp -
#               holds data source information for a geomap
#               raster colorramp layer
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


itcl::class Rappture::GeoMapDataProviderColorramp {
    inherit Rappture::GeoMapDataProvider

    constructor {url args} {
        Rappture::GeoMapDataProvider::constructor "image" "colorramp" $url
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    public variable colormap "0 0 0 0 1 1 1 1 1 1"
    public variable elevdriver "gdal"
    public variable profile "geodetic"

    public method exportToBltTree { tree }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderColorramp::constructor {url args} {

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderColorramp::destructor {} {

}


# ----------------------------------------------------------------------
# colormap: set colormap for this data source
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProviderColorramp::colormap {

    # colormap should be non-empty list
    if {[llength $colormap] == 0} {
        error "bad value \"$colormap\": should be a non-empty list of doubles"
    }

    # colormap entries should have 5 values each
    set lcm [llength $colormap]
    if { [expr {$lcm % 5}] != 0 } {
        error "bad value \"$colormap\": colormap entries should have 5 values each"
    }

    foreach {p r g b a} $colormap {
        # point should be a double precision value
        if {[string is double $p] == 0} {
            error "bad value \"$p\": should be a double"
        }
        # r g b and a should be double precision values in range [0.0,1.0]
        foreach v [list $r $g $b $a] {
            if {[string is double $v] == 0} {
                error "bad value \"$v\": should be a double"
            }
            if {[expr {$v < 0.0}] == 1 || [expr {$v > 1.0}] == 1} {
                error "bad value \"$v\": should be in range \[0.0,1.0\]"
            }
        }
    }

    # normalize spacing and save the colormap
    set colormap [regsub -all "\[ \t\r\n\]+" [string trim $colormap] " "]
}

# ----------------------------------------------------------------------
# elevdriver: get/set elevdriver to be used by this datasource
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProviderColorramp::elevdriver {

    set valids {gdal tms}

    if {[string compare "" $elevdriver] == 0} {
        error "bad value \"$elevdriver\": should be a non-empty string"
    }

    if {[lsearch $valids $elevdriver] < 0} {
        error "bad value \"$elevdriver\": should be one of \"$valids\""
    }
}


# ----------------------------------------------------------------------
# profile: get/set profile for to be used for this datasource
#
# example profiles include:
#   global-geodetic
#   global-mercator
#   geodetic
#   mercator
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapDataProviderColorramp::profile {

#    set valids {global-geodetic global-mercator geodetic mercator}

    if {[string compare "" $profile] == 0} {
        error "bad value \"$profile\": should be a non-empty string"
    }

#    if {[lsearch $valids $profile] < 0} {
#        error "bad value \"$profile\": should be one of \"$valids\""
#    }
}


# ----------------------------------------------------------------------
# ExportToBltTree: export object to a blt::tree
#
# ExportToBltTree $tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapDataProviderColorramp::exportToBltTree {tree} {

    Rappture::GeoMapDataProvider::exportToBltTree $tree

    $tree set root \
        colorramp.url [url] \
        colorramp.colormap $colormap \
        colorramp.elevdriver $elevdriver \
        profile $profile \
}

