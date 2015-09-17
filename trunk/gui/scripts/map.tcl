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
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method earthfile {}
    public method hints { args }
    public method isGeocentric {}
    public method isvalid {} {
        return $_isValid;
    }
    public method layer { layerName }
    public method layers {}
    public method type { layerName }
    public method viewpoint { viewpointName }
    public method viewpoints {}

    protected method Parse { xmlobj path }

    private variable _tree "";         # Tree of information about the map.
    private variable _isValid 0;
    private common _nextLayer 0;       # Counter used to generate unique
                                       # layer names.
    private common _nextViewpoint 0;   # Counter used to generate unique
                                       # viewpoint names.
    private common _layerTypes
    private common _mapTypes
    array set _layerTypes {
        "image"         0
        "elevation"     1
        "polygon"       2
        "point"         3
        "icon"          4
        "line"          5
        "label"         6
    }
    array set _mapTypes {
        "geocentric"    0
        "projected"     1
    }
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
#   Parses the map description in the XML object.
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
        set layerType [$layers get $layer.type]
        if { ![info exists _layerTypes($layerType)] } {
            error "invalid layer type \"$layerType\": should be one of [array names _layerTypes]"
        }
        $_tree set $child "name" $layer
        $_tree set $child "type" $layerType
        foreach key { label description attribution profile srs verticalDatum } {
            $_tree set $child $key [$layers get $layer.$key]
        }
        # Common settings (for all layer types) with defaults
        foreach { key defval } { visible 1 cache 1 } {
            $_tree set $child $key $defval
            set val [$layers get $layer.$key]
            if {$val != ""} {
                $_tree set $child $key $val
            }
        }
        # These are settings for which there should be no default
        # We want to know if they have been set by the user or not
        # Not all layer types use these
        foreach key { opacity content priority style } {
            set val [$layers get $layer.$key]
            if {$val != ""} {
                $_tree set $child $key $val
            }
        }
        $_tree set $child "driver" "debug"
        set colorramp [$layers element -as type $layer.colorramp]
        if { $colorramp != "" } {
            $_tree set $child "colorramp.elevdriver" "gdal"
            $_tree set $child "colorramp.colormap" "0 0 0 0 1 1 1 1 1 1"
            set cmap [$layers get $layer.colorramp.colormap]
            if {$cmap != ""} {
                # Normalize whitespace
                regsub -all "\[ \t\r\n\]+" [string trim $cmap] " " cmap
                $_tree set $child "colorramp.colormap" $cmap
            }
            foreach key { url elevdriver } {
                set value [$layers get $layer.colorramp.$key]
                if {$value != ""} {
                    $_tree set $child "colorramp.$key" $value
                }
            }
            set file [$layers get $layer.colorramp.file]
            if { $file != "" } {
                # FIXME: Add test for valid file path
                $_tree set $child "colorramp.url" $file
            }
            $_tree set $child "driver" "colorramp"
        }
        set gdal [$layers element -as type $layer.gdal]
        if { $gdal != "" } {
            foreach key { url } {
                set value [$layers get $layer.gdal.$key]
                $_tree set $child "gdal.$key" $value
            }
            set file [$layers get $layer.gdal.file]
            if { $file != "" } {
                # FIXME: Add test for valid file path
                $_tree set $child "gdal.url" $file
            }
            $_tree set $child "driver" "gdal"
        }
        set ogr [$layers element -as type $layer.ogr]
        if { $ogr != "" } {
            foreach key { url } {
                set value [$layers get $layer.ogr.$key]
                $_tree set $child "ogr.$key" $value
            }
            set file [$layers get $layer.ogr.file]
            if { $file != "" } {
                # FIXME: Add test for valid file path
                $_tree set $child "ogr.url" $file
            }
            $_tree set $child "driver" "ogr"
        }
        set tfs [$layers element -as type $layer.tfs]
        if { $tfs != "" } {
            foreach key { url format } {
                set value [$layers get $layer.tfs.$key]
                $_tree set $child "tfs.$key" $value
            }
            $_tree set $child "driver" "tfs"
        }
        set tms [$layers element -as type $layer.tms]
        if { $tms != "" } {
            foreach key { url tmsType format } {
                set value [$layers get $layer.tms.$key]
                $_tree set $child "tms.$key" $value
            }
            $_tree set $child "driver" "tms"
        }
        set wfs [$layers element -as type $layer.wfs]
        if { $wfs != "" } {
            foreach key { url typename format maxfeatures request_buffer } {
                set value [$layers get $layer.wfs.$key]
                $_tree set $child "wfs.$key" $value
            }
            $_tree set $child "driver" "wfs"
        }
        set wms [$layers element -as type $layer.wms]
        if { $wms != "" } {
            foreach key { url layers format transparent } {
                set value [$layers get $layer.wms.$key]
                $_tree set $child "wms.$key" $value
            }
            $_tree set $child "driver" "wms"
        }
        set xyz [$layers element -as type $layer.xyz]
        if { $xyz != "" } {
            foreach key { url } {
                set value [$layers get $layer.xyz.$key]
                $_tree set $child "xyz.$key" $value
            }
            $_tree set $child "driver" "xyz"
        }
    }
    $_tree set root "label"       [$map get "about.label"]
    $_tree set root "attribution" [$map get "about.attribution"]
    $_tree set root "style"       [$map get "style"]
    $_tree set root "camera"      [$map get "camera"]
    set parent [$_tree insert root -label "viewpoints"]
    set viewpoints [$map element -as object "viewpoints"]
    if { $viewpoints != "" } {
        foreach viewpoint [$viewpoints children -type viewpoint] {
            set name "viewpoint[incr _nextViewpoint]"
            set child [$_tree insert $parent -label $name]
            $_tree set $child "name" $viewpoint
            set haveX 0
            set haveZ 0
            set haveSRS 0
            set haveVertDatum 0
            foreach key { label description x y z distance heading pitch srs verticalDatum } {
                set val [$viewpoints get $viewpoint.$key]
                if {$val != ""} {
                    if {$key == "x"} {
                        set haveX 1
                    } elseif {$key == "z"} {
                        set haveZ 1
                    } elseif {$key == "srs"} {
                        set haveSRS 1
                    } elseif {$key == "verticalDatum"} {
                        set haveVertDatum 1
                    }
                    $_tree set $child $key $val
                }
            }
            if {!$haveX} {
                set lat [$viewpoints get $viewpoint.latitude]
                set long [$viewpoints get $viewpoint.longitude]
                $_tree set $child x $long
                $_tree set $child y $lat
                if {!$haveSRS} {
                    $_tree set $child srs wgs84
                }
                if {!$haveVertDatum} {
                    $_tree set $child verticalDatum ""
                }
            }
            if {!$haveZ} {
                set z [$viewpoints get $viewpoint.altitude]
                if {$z != ""} {
                    $_tree set $child z $z
                }
            }
        }
    }

    set projection [$map get "projection"]
    set extents    [$map get "extents"]
    if { $projection  == "" } {
        if { $extents != "" } {
            error "cannot specify extents without a projection"
        }
        set projection "global-mercator"; # Default projection.
    } elseif { $projection == "geodetic" && $extents == "" } {
        set projection "global-geodetic"
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
        toolid          tool.id
        toolname        tool.name
        toolcommand     tool.execute
        tooltitle       tool.title
        toolrevision    tool.version.application.revision
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
# USAGE: viewpoints
#
# Returns a list of IDs for the viewpoints in the map
# ----------------------------------------------------------------------
itcl::body Rappture::Map::viewpoints {} {
    set list {}
    foreach node [$_tree children root->"viewpoints"] {
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
# USAGE: viewopint <viewopintName>
#
# Returns an array of settings for the named viewpoint
# ----------------------------------------------------------------------
itcl::body Rappture::Map::viewpoint { viewopintName } {
    set id [$_tree findchild root->"viewpoints" $viewopintName]
    if { $id < 0 } {
        error "unknown viewpoint \"$viewpointName\""
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

itcl::body Rappture::Map::earthfile {} {
    array set info [$_tree get root]
    append out "<map"
    append out " name=\"$info(label)\""
    append out " type=\"$info(type)\""
    append out " version=\"2\""
    append out ">\n"
    # Profile is optional
    if { [info exists info(projection)] } {
        append out " <options>\n"
        append out "  <profile"
        append out " srs=\"$info(projection)\""
        if { [info exists info(extents)] && $info(extents) != "" } {
            foreach {x1 y1 x2 y2} $info(extents) break
            append out " xmin=\"$x1\""
            append out " ymin=\"$y1\""
            append out " xmax=\"$x2\""
            append out " ymax=\"$y2\""
        }
        append out "/>\n"
        append out " </options>\n"
    }
    foreach node [$_tree children root->"layers"] {
        array unset info
        array set info [$_tree get $node]
        set label [$_tree label $node]
        switch -- $info(type) {
            "image" {
                append out " <image"
                append out " name=\"$label\""
                append out " driver=\"gdal\""
                if { [info exists info(opacity)] } {
                    append out " opacity=\"$info(opacity)\""
                }
                if { $info(visible) } {
                    append out " visible=\"true\""
                } else {
                    append out " visible=\"false\""
                }
                append out ">\n"
                append out "  <url>$info(url)</url>\n"
                append out " </image>\n"
            }
            "elevation" {
                append out " <elevation"
                append out " name=\"$label\""
                append out " driver=\"gdal\""
                if { $info(visible) } {
                    append out " visible=\"true\""
                } else {
                    append out " visible=\"false\""
                }
                append out ">\n"
                append out "  <url>$info(url)</url>\n"
                append out " </elevation>\n"
            }
            default {
                puts stderr "Type $info(type) not implemented in earthfile"
            }
        }
    }
    append out "</map>\n"
}
