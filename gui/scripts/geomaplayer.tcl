# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: geomaplayer - holds layer information for a geomap
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


itcl::class Rappture::GeoMapLayer {
    constructor {provider args} {
        # defined below
    }
    destructor {
        # defined below
    }

    private variable _provider ""

    public variable label ""
    public variable description ""
    public variable attribution ""
    public variable visibility "true"
    public variable opacity "1.0"

    private method Provider { provider }

    public method type { }
    public method driver { }
    public method url { }
    public method cache { }

    public method export { args }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::constructor {provider args} {

    Provider $provider

    eval configure $args
}


# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::destructor {} {
    itcl::delete object $_provider
}


# ----------------------------------------------------------------------
# Provider: set data source for the layer
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::Provider { provider } {

    if {![$provider isa Rappture::GeoMapDataProvider]} {
        error "bad value: \"$provider\": should be a Rappture::GeoMapDataProvider object"
    }

    set _provider $provider
}


# ----------------------------------------------------------------------
# Label: get/set label variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayer::label {

    if {[string compare "" $label] == 0} {
        error "bad value: \"$label\": should be a non-empty string"
    }
}


# ----------------------------------------------------------------------
# Description: get/set description variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayer::description {

    if {[string compare "" $description] == 0} {
        error "bad value: \"$description\": should be a non-empty string"
    }
}


# ----------------------------------------------------------------------
# Attribution: get/set attribution variable
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayer::attribution {

    if {[string compare "" $attribution] == 0} {
        error "bad value: \"$attribution\": should be a non-empty string"
    }
}


# ----------------------------------------------------------------------
# Visibility: get/set the initial visibility of the layer
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayer::visibility {

    if {[string is bool $visibility] == 0} {
        error "bad value: \"$visibility\": should be a boolean"
    }
}


# ----------------------------------------------------------------------
# Opacity: set the initial opacity of the layer
# ----------------------------------------------------------------------
itcl::configbody Rappture::GeoMapLayer::opacity {

    if {[string is double $opacity] == 0} {
        error "bad value: \"$opacity\": should be a double"
    }

    if {[expr {$opacity < 0.0}] == 1 || [expr {$opacity > 1.0}] == 1} {
        error "bad value: \"$opacity\": should be in range \[0.0,1.0\]"
    }
}


# ----------------------------------------------------------------------
# type: clients use this public method to query type of this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::type {} {
    return [$_provider type]
}


# ----------------------------------------------------------------------
# driver: clients use this public method to query the driver for this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::driver {} {
    return [$_provider driver]
}


# ----------------------------------------------------------------------
# url: clients use this public method to query the initial
#      value of the urlf or this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::url {} {
    return [$_provider url]
}


# ----------------------------------------------------------------------
# cache: clients use this public method to query the initial
#           value of the cache flag for this layer
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::cache {} {
    return [$_provider cache]
}


# ----------------------------------------------------------------------
# export: clients use this public method to serialize the object to a
#       blt::tree object
#
# export -format blt_tree
#
# ----------------------------------------------------------------------
itcl::body Rappture::GeoMapLayer::export {args} {
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
            set t [blt::tree create]
            $_provider exportToBltTree $t
            $t set root \
                label $label \
                description $description \
                attribution $attribution \
                visible $visibility \
                opacity $opacity \

        }
        default {
            error "bad format \"$format\": should be one of $valids"
        }
    }

    return $t
}

