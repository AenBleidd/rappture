# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomaplayerimage - holds generic image layer information
#                                for a geomap
#
# ======================================================================
#  AUTHOR:  Derrick Kearney, Purdue University
#  Copyright (c) 2004-2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture {
    # forward declaration
}


itcl::class Rappture::GeoMapLayerImage {
    inherit Rappture::GeoMapLayer

    constructor {provider args} {
        eval Rappture::GeoMapLayer::constructor $provider $args
    } {
        # defined below
    }
    destructor {
        # defined below
    }

    public variable coverage "false"

    public method export { args }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayerImage::constructor {provider args} {

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayerImage::destructor {} {

}


# ----------------------------------------------------------------------
# Coverage: get/set the coverage flag for this layer
#
# Coverage determines how the data is interpolated by the gpu
# true means data should not be interpolated
# false means data should be interpolated
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayerImage::coverage {

    if {[string is bool $coverage] == 0} {
        error "bad value: \"$coverage\": should be a boolean"
    }
}


# ----------------------------------------------------------------------
# export: clients use this public method to serialize the object to a
#       blt::tree object
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayerImage::export {args} {

    set t [eval Rappture::GeoMapLayer::export $args]

    set valids "-format"
    set format "blt_tree"

    while {[llength $args] > 0} {
        set flag [lindex $args 0]
        switch -- $flag {
            "-format" {
                if {[llength $args] > 1} {
                    set format [lindex $args 1]
                    set args [lrange $args 2 end]
                } else {
                    error "wrong number args: should be ?-format <format>?"
                }
            }
            default {
                error "invalid option \"$flag\": should be one of $valids"
            }
        }
    }

    set valids "blt_tree"

    switch -- $format {
        "blt_tree" {
            $t set root coverage $coverage
        }
        default {
            error "bad format \"$format\": should be one of $valids"
        }
    }

    return $t
}
