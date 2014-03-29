# -*- mode: tcl; indent-tabs-mode: nil -*- 

# ----------------------------------------------------------------------
#  COMPONENT: map - extracts data from an XML description of a field
#
#  This object represents a map of data in an XML description of
#  simulator output.  A map is similar to a field, but a field is
#  a quantity versus position in device.  A map is any quantity
#  versus any other quantity.  This class simplifies the process of
#  extracting data vectors that represent the map.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { 
    # forward declaration 
}

itcl::class Rappture::Map {
    private variable _tree "";         # Tree of information about the map.
    private variable _isValid 0;
    private variable _nextLayer 0;     # Counter used to generate unique
                                       # layer names.
    private common _layerTypes
    private common _mapTypes
    array set _layerTypes {
        "raster"        0
        "elevation"     1
        "polygon"       2
        "points"        3
        "circle"        4
        "line"          5
    }
    array set _mapTypes {
        "geocentric"    0
        "projected"     1
    }
    protected method Parse { xmlobj path }

    constructor {xmlobj path} { 
        # defined below 
    }
    destructor { 
        # defined below 
    }

    public method isGeocentric {} 
    public method layers {}
    public method layer { layerName }
    public method hints { args }
    public method isvalid {} {
        return $_isValid;
    }
    public method type { layerName }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Map::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    Parse $xmlobj $path
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Map::destructor {} {
    if { $_tree != "" } {
        blt::tree destroy $_tree
    }
}

#
# hints --
#
itcl::body Rappture::Map::hints { args } {
    switch -- [llength $args] {
        0 {
            return [$_tree get root]
        }
        1 {
            set field [lindex $args 0]
            return [$_tree get root $field ""]
        }
        default {
            error "wrong # args: should \"hints ?name?\""
        }
    }
}

#
# Parse --
#
#       Parses the map description in the XML object.
#
itcl::body Rappture::Map::Parse { xmlobj path } {

    set map [$xmlobj element -as object $path]

    if { $_tree != "" } {
        blt::tree destroy $_tree
    }
    set _tree [blt::tree create]
    set parent [$_tree insert root -label "layers"]
    set layers [$map element -as object "layers"]
    foreach layer [$layers children -type layer] {
        # Unique identifier for layer.
        set name "layer[incr _nextLayer]"
        set child [$_tree insert $parent -label $name]
        $_tree set $child "title" [$layers get $layer.label]
        set layerType [$layers get $layer.type] 
        if { ![info exists _layerTypes($layerType)] } {
            error "invalid layer type \"$layerType\": should be one of [array names _layerTypes]"
        }
        $_tree set $child "type" $layerType
        foreach key { label description url } {
            $_tree set $child $key [$layers get $layer.$key] 
        }
        set file [$layers get $layer.file] 
        if { $file != "" } {
            # FIXME: Add test for valid file path
            $_tree set $child "url" $file
        }
    }
    $_tree set root "label"       [$map get "about.label"]
    $_tree set root "style"       [$map get "style"]
 
    set projection [$map get "projection"]
    set extents    [$map get "extents"]
    if { $projection  == "" } {
        if {$extents != ""} {
            error "cannot specify extents without a projection"
        }
        set projection "global-mercator"; # Default projection.
    }
    # FIXME: Verify projection is valid.
    $_tree set root "projection" $projection
    $_tree set root "extents"    $extents

    set mapType [$map get "type"]
    if { $mapType == "" } {
        set mapType "projected";           # Default type is "projected".
    } 
    if { ![info exists _mapTypes($mapType)] } {
        error "unknown map type \"$mapType\": should be one of [array names _mapTypes]"
    }
    $_tree set root "type" $mapType

    foreach {key path} {
        toolId          tool.id
        toolName        tool.name
        toolCommand     tool.execute
        toolTitle       tool.title
        toolRevision    tool.version.application.revision
    } {
        set str [$xmlobj get $path]
        if { "" != $str } {
            $_tree set root $key $str
        }
    }
    set _isValid 1
}

# ----------------------------------------------------------------------
# USAGE: layers
#
# Returns a list of IDs for the layers in the map
# ----------------------------------------------------------------------
itcl::body Rappture::Map::layers {} {
    set list {}
    foreach node [$_tree children root->"layers"] {
        lappend list [$_tree label $node] 
    }
    return $list
}

# ----------------------------------------------------------------------
# USAGE: layer <layerName>
#
# Returns an array of settings for the named layer
# ----------------------------------------------------------------------
itcl::body Rappture::Map::layer { layerName } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    return [$_tree get $id]
}

# ----------------------------------------------------------------------
# USAGE: type <layerName>
#
# Returns the type of the named layer
# ----------------------------------------------------------------------
itcl::body Rappture::Map::type { layerName } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    return [$_tree get $id "type" ""]
}

# ----------------------------------------------------------------------
# USAGE: isGeocentric
#
# Returns if the map is geocentric (1) or projected (0) 
# ---------------------------------------------------------------------- 
itcl::body Rappture::Map::isGeocentric {} { 
    return [expr {[hints "type"] eq "geocentric"}]
} 
