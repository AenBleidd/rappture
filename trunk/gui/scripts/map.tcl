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
    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method addLayer { type name paramArray driver driverParamArray {stylesheet {}} {script {}} {selectors {}} }
    public method addSelector { layerName selectorName paramArray }
    public method addViewpoint { name props }
    public method clearExtents {}
    public method deleteLayer { layerName }
    public method deleteSelector { layerName selectorName }
    public method deleteViewpoint { viewpointName }
    public method dirty { key args } {
        if {[llength $args] == 0} {
            if { [info exists _dirty($key)] } {
                return $_dirty($key)
            } else {
                return 0
            }
        } else {
            set _dirty($key) [lindex $args 0]
        }
    }
    public method getPlacardConfig { layerName }
    public method hasLayer { layerName }
    public method hasSelector { layerName selectorName }
    public method hasViewpoint { viewpointName }
    public method hints { args }
    public method isGeocentric {}
    public method isvalid {} {
        return $_isValid;
    }
    public method layer { layerName args }
    public method layers {}
    public method selectors { layerName }
    public method selector { layerName selectorName }
    public method setAttribution { attribution }
    public method setCamera { camera }
    public method setColormap { layerName colormap }
    public method setDescription { description }
    public method setExtents { xmin ymin xmax ymax {srs "wgs84"} }
    public method setLabel { label }
    public method setPlacardConfig { layerName attrlist style padding }
    public method setProjection { projection }
    public method setScript { layerName script }
    public method setStyle { style }
    public method setStylesheet { layerName stylesheet }
    public method setToolInfo { id name command title revision }
    public method setType { type }
    public method viewpoint { viewpointName }
    public method viewpoints {}

    public proc getFilesFromStylesheet { stylesheet }

    protected method parseXML { xmlobj path }

    protected proc isFileProp { prop }
    protected proc parseStylesheet { stylesheet }

    private variable _tree "";         # Tree of information about the map.
    private variable _isValid 0;
    private variable _dirty;
    array set _dirty {
        viewpoints 0
    }
    private common _nextSelector 0;
    private common _layerTypes
    private common _mapTypes
    array set _layerTypes {
        "image"         0
        "elevation"     1
        "feature"       2
        "polygon"       3
        "point"         4
        "icon"          5
        "line"          6
        "label"         7
    }
    array set _mapTypes {
        "geocentric"    0
        "projected"     1
    }
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Map::constructor {args} {
    set _tree [blt::tree create]
    $_tree insert root -label "layers"
    $_tree insert root -label "viewpoints"
    setLabel "Map"
    setType "projected"
    setProjection "global-mercator"
    clearExtents
    setStyle ""
    setCamera ""
    if {$args == ""} {
        set _isValid 1
    } else {
        set xmlobj [lindex $args 0]
        set path [lindex $args 1]
        if {![Rappture::library isvalid $xmlobj]} {
            error "bad value \"$xmlobj\": should be LibraryObj"
        }
        parseXML $xmlobj $path
    }
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
            error "wrong # args: should be \"hints <?name?>\""
        }
    }
}

#
# parseXML --
#
#   Parses the map description in the XML object.
#
itcl::body Rappture::Map::parseXML { xmlobj path } {
    set map [$xmlobj element -as object $path]

    # Set global map properties
    setLabel [$map get "about.label"]
    setDescription [$map get "about.description"]
    setAttribution [$map get "about.attribution"]

    set mapType [$map get "type"]
    if { $mapType != "" } {
        if {[catch {setType $mapType} msg] != 0} {
            puts stderr "ERROR: $msg"
            return
        }
    }

    set projection [$map get "projection"]
    set extents    [$map get "extents"]
    if { $projection  == "" } {
        if { $extents != "" } {
            puts stderr "ERROR: cannot specify extents without a projection"
            set extents ""
        }
        set projection "global-mercator"; # Default projection.
    } elseif { $projection == "geodetic" || $projection == "global-geodetic" ||
               $projection == "wgs84" || $projection == "epsg:4326" ||
               $projection == "plate-carre" || $projection == "plate-carree" } {
        # Can't use angular units in projection  
        puts stderr "ERROR: Geodetic profile not supported as map projection.  Try using an equirectangular (epsg:32663) projection instead."
        set projection "epsg:32663"
    } elseif { $projection == "equirectangular" ||
               $projection == "eqc-wgs84" } {
        set projection "epsg:32663"
    }
    # FIXME: Verify projection is valid.
    setProjection $projection
    if {$extents != ""} {
        foreach {xmin ymin xmax ymax srs} $extents {}
        if {$srs == ""} {
            setExtents $xmin $ymin $xmax $ymax
        } else {
            setExtents $xmin $ymin $xmax $ymax $srs
        }
    } else {
         clearExtents
    }

    if {[catch {setStyle [$map get "style"]} msg] != 0} {
        puts stderr "ERROR: $msg"
    }
    if {[catch {setCamera [$map get "camera"]} msg] != 0} {
        puts stderr "ERROR: $msg"
    }

    # Parse layers
    set parent [$_tree findchild root "layers"]
    set layers [$map element -as object "layers"]
    foreach layer [$layers children -type layer] {
        # Unique identifier for layer.
        set name [$layers element -as id "$layer"]
        if {[hasLayer $name]} {
            puts stderr "ERROR: Duplicate layer ID '$name', skipping"
            continue
        }
        set layerType [$layers get $layer.type]
        if { ![info exists _layerTypes($layerType)] } {
            puts stderr "ERROR: invalid layer type \"$layerType\": should be one of: [join [array names _layerTypes] {, }]"
            continue
        }
        set child [$_tree insert $parent -label $name]
        $_tree set $child "name" $name
        $_tree set $child "type" $layerType
        foreach key { label description attribution profile srs verticalDatum } {
            $_tree set $child $key [$layers get $layer.$key]
        }
        # Common settings (for all layer types) with defaults
        foreach { key defval } { visible 1 cache 1 shared 0 } {
            $_tree set $child $key $defval
            set val [$layers get $layer.$key]
            if {$val != ""} {
                $_tree set $child $key $val
            }
        }
        # These are settings for which there should be no default
        # We want to know if they have been set by the user or not
        # Not all layer types use these
        foreach key { coverage opacity content priority style } {
            set val [$layers get $layer.$key]
            if {$val != ""} {
                if {$key eq "coverage" && $layerType ne "image"} {
                    puts stderr "ERROR: <coverage> is only valid for layers of type \"image\""
                }
                if {$key eq "content" || $key eq "priority"} {
                    if {$layerType ne "label"} {
                        puts stderr "ERROR: <content> and <priority> are only valid in layers of type \"label\""
                    }
                }
                if {$key eq "opacity" && $layerType eq "elevation"} {
                    puts stderr  "ERROR: <opacity> is not valid for layers of type \"elevation\""
                }
                $_tree set $child $key $val
            }
        }
        set styles [$layers element -as object $layer.styles]
        if {$styles != ""} {
            set val [$styles get stylesheet]
            # Normalize whitespace
            regsub -all "\[ \t\r\n\]+" [string trim $val] " " val
            $_tree set $child stylesheet $val
            set script [$styles get script]
            if {$script != ""} {
                regsub -all "\[\r\n\]+" [string trim $script] " " script
                $_tree set $child script $script
            }
            set sparent [$_tree insert $child -label "selectors"]
            foreach selector [$styles children -type selector] {
                set id [$styles element -as id "$selector"]
                set snode [$_tree insert $sparent -label $id]
                foreach key { name style styleExpression query queryBounds queryOrderBy } {
                    set val [$styles get $selector.$key]
                    if {$val != ""} {
                        $_tree set $snode $key $val
                    }
                }
            }
            rename $styles ""
        }
        set placard [$layers element -as object $layer.placard]
        if {$placard != ""} {
            if {$layerType == "image" || $layerType == "elevation"} {
                puts stderr "ERROR: Placard not supported on image or elevation layers"
            }
            foreach key { attributes style padding } {
                set $key [$placard get $key]
            }
            setPlacardConfig $name $attributes $style $padding
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
        set agglite [$layers element -as type $layer.agglite]
        if { $agglite != "" } {
            foreach key { url } {
                set value [$layers get $layer.agglite.$key]
                $_tree set $child "agglite.$key" $value
            }
            $_tree set $child "driver" "agglite"
        }
        set arcgis [$layers element -as type $layer.arcgis]
        if { $arcgis != "" } {
            foreach key { url token format layers } {
                set value [$layers get $layer.arcgis.$key]
                $_tree set $child "arcgis.$key" $value
            }
            $_tree set $child "driver" "arcgis"
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
            foreach key { connection geometry geometry_url layer ogr_driver build_spatial_index } {
                set value [$layers get $layer.ogr.$key]
                if { $value != "" } {
                    $_tree set $child "ogr.$key" $value
                }
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
        set wcs [$layers element -as type $layer.wcs]
        if { $wcs != "" } {
            foreach key { url identifier format elevationUnit rangeSubset } {
                set value [$layers get $layer.wcs.$key]
                $_tree set $child "wcs.$key" $value
            }
            $_tree set $child "driver" "wcs"
        }
        set wfs [$layers element -as type $layer.wfs]
        if { $wfs != "" } {
            foreach key { url typename format maxfeatures requestBuffer } {
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
    if {$layers != ""} {
        rename $layers ""
    }

    # Parse viewpoints
    set parent [$_tree findchild root "viewpoints"]
    set viewpoints [$map element -as object "viewpoints"]
    if { $viewpoints != "" } {
        foreach viewpoint [$viewpoints children -type viewpoint] {
            set name [$viewpoints element -as id "$viewpoint"]
            if {[hasViewpoint $name]} {
                puts stderr "ERROR: Duplicate viewpoint ID '$name', skipping"
                continue
            }
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
        rename $viewpoints ""
    }

    # Fill in tool info
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

    rename $map ""
    set _isValid 1
}

itcl::body Rappture::Map::setToolInfo { id name command title revision } {
    foreach key [list id name command title revision] {
        set str [set $key]
        if { "" != $str } {
            $_tree set root "tool$key" $str
        }
    }
}

itcl::body Rappture::Map::setType { type } {
    if { ![info exists _mapTypes($type)] } {
        error "unknown map type \"$type\": should be one of: [join [array names _mapTypes] {, }]"
    }
    $_tree set root "type" $type
}

itcl::body Rappture::Map::setProjection { projection } {
    $_tree set root "projection" $projection
}

itcl::body Rappture::Map::clearExtents {} {
    $_tree set root "extents" ""
}

itcl::body Rappture::Map::setExtents { xmin ymin xmax ymax {srs "wgs84"} } {
    $_tree set root "extents" [list $xmin $ymin $xmax $ymax $srs]
}

itcl::body Rappture::Map::setLabel { label } {
    $_tree set root "label" $label
}

itcl::body Rappture::Map::setDescription { description } {
    $_tree set root "description" $description
}

itcl::body Rappture::Map::setAttribution { attribution } {
    $_tree set root "attribution" $attribution
}

itcl::body Rappture::Map::setStyle { style } {
    if {$style != "" && [llength $style] % 2 != 0} {
        error "Bad map style, must be key/value pairs"
    }
    array set styleinfo $style
    foreach key [array names styleinfo] {
        set valid 0
        foreach validkey {-ambient -color -edgecolor -edges -lighting -linewidth -vertscale -wireframe} {
            if {$key == $validkey} {
                set valid 1
                break
            }
        }
        if {!$valid} {
            error "Unknown style setting: $key"
        }
    }
    $_tree set root "style" $style
}

itcl::body Rappture::Map::setCamera { camera } {
    if {$camera != "" && [llength $camera] % 2 != 0} {
        error "Bad camera settings, must be key/value pairs"
    }
    array set caminfo $camera
    foreach key [array names caminfo] {
        set valid 0
        foreach validkey {x y z heading pitch distance xmin ymin xmax ymax srs verticalDatum} {
            if {$key == $validkey} {
                set valid 1
                break
            }
        }
        if {!$valid} {
            error "Unknown camera setting: $key"
        }
    }
    if {([info exists caminfo(x)] || [info exists caminfo(y)] ||
         [info exists caminfo(z)] || [info exists caminfo(distance)]) &&
        ([info exists caminfo(xmin)] || [info exists caminfo(xmax)] ||
         [info exists caminfo(ymin)] || [info exists caminfo(ymax)])} {
        error "Bad camera settings: Cannot set both focal point and extents"
    }
    $_tree set root "camera" $camera
}

itcl::body Rappture::Map::addLayer { type name paramArray driver driverParamArray {stylesheet {}} {script {}} {selectors {}} } {
    set id "$name"
    if {[hasLayer $id]} {
        error "Layer '$id' already exists"
    }
    if { ![info exists _layerTypes($type)] } {
        error "Invalid layer type \"$type\": should be one of: [join [array names _layerTypes] {, }]"
    }
    set parent [$_tree findchild root "layers"]
    set child [$_tree insert $parent -label $id]
    $_tree set $child "name" $name
    $_tree set $child "type" $type
    array set params $paramArray
    foreach key { label description attribution profile srs verticalDatum } {
        if {[info exists params($key)]} {
            $_tree set $child $key $params($key)
        } else {
            $_tree set $child $key ""
        }
    }
    # Common settings (for all layer types) with defaults
    foreach { key defval } { visible 1 cache 1 shared 0 } {
        $_tree set $child $key $defval
        if {[info exists params($key)]} {
            set val $params($key)
            if {$val != ""} {
                $_tree set $child $key $val
            }
        }
    }
    # These are settings for which there should be no default
    # We want to know if they have been set by the user or not
    # Not all layer types use these
    foreach key { coverage opacity content priority style } {
        if {[info exists params($key)]} {
            set val $params($key)
            if {$val != ""} {
                if {$key eq "coverage" && $type ne "image"} {
                    error "Coverage is only valid for layers of type \"image\""
                }
                if {$key eq "content" || $key eq "priority"} {
                    if {$type ne "label"} {
                        error "content and priority are only valid in layers of type \"label\""
                    }
                }
                if {$key eq "opacity" && $type eq "elevation"} {
                    error  "opacity is not valid for layers of type \"elevation\""
                }
                $_tree set $child $key $val
            }
        }
    }
    if {$stylesheet != ""} {
        set val $stylesheet
        # Normalize whitespace
        regsub -all "\[ \t\r\n\]+" [string trim $val] " " val
        $_tree set $child stylesheet $val
    }
    if {$script != ""} {
        regsub -all "\[\r\n\]+" [string trim $script] " " script
        $_tree set $child script $script
    }
    if {$selectors != ""} {
        set sparent [$_tree insert $child -label "selectors"]
        foreach selectorItem $selectors {
            array set selector $selectorItem
            if { [info exists selector(id)] } {
                set id $selector(id)
            } else {
                set id "selector[incr _nextSelector]"
            }
            set snode [$_tree insert $sparent -label $id]
            foreach key { name style styleExpression query queryBounds queryOrderBy } {
                if {[info exists selector($key)]} {
                    set val $selector($key)
                    if {$val != ""} {
                        $_tree set $snode $key $val
                    }
                }
            }
        }
    }
    $_tree set $child "driver" $driver
    switch -- $driver {
        "agglite" {
            array set params $driverParamArray
            foreach key { url featuredriver format typeName } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "agglite.$key" $value
                }
            }
        }
        "arcgis" {
            array set params $driverParamArray
            foreach key { url token format layers } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "arcgis.$key" $value
                }
            }
        }
        "colorramp" {
            array set params $driverParamArray
            $_tree set $child "colorramp.elevdriver" "gdal"
            $_tree set $child "colorramp.colormap" "0 0 0 0 1 1 1 1 1 1"
            if {[info exists params(colormap)]} {
                set cmap $params(colormap)
                if {$cmap != ""} {
                    # Normalize whitespace
                    regsub -all "\[ \t\r\n\]+" [string trim $cmap] " " cmap
                    $_tree set $child "colorramp.colormap" $cmap
                }
            }
            foreach key { url elevdriver } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    if {$value != ""} {
                        $_tree set $child "colorramp.$key" $value
                    }
                }
            }
        }
        "gdal" {
            array set params $driverParamArray
            foreach key { url } {
                set value $params($key)
                $_tree set $child "gdal.$key" $value
            }
        }
        "ogr" {
            array set params $driverParamArray
            foreach key { url } {
                set value $params($key)
                $_tree set $child "ogr.$key" $value
            }
            foreach key { connection geometry geometry_url layer ogr_driver build_spatial_index } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    if { $value != "" } {
                        $_tree set $child "ogr.$key" $value
                    }
                }
            }
        }
        "tfs" {
            array set params $driverParamArray
            foreach key { url format } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "tfs.$key" $value
                }
            }
        }
        "tms" {
            array set params $driverParamArray
            foreach key { url tmsType format } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "tms.$key" $value
                }
            }
        }
        "wcs" {
            array set params $driverParamArray
            foreach key { url identifier format elevationUnit rangeSubset } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "wcs.$key" $value
                }
            }
        }
        "wfs" {
            array set params $driverParamArray
            foreach key { url typename format maxfeatures requestBuffer } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "wfs.$key" $value
                }
            }
        }
        "wms" {
            array set params $driverParamArray
            foreach key { url layers format transparent } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "wms.$key" $value
                }
            }
        }
        "xyz" {
            array set params $driverParamArray
            foreach key { url } {
                if {[info exists params($key)]} {
                    set value $params($key)
                    $_tree set $child "xyz.$key" $value
                }
            }
        }
        default {
            error "Unknown driver \"$driver\""
        }
    }
    set _dirty($id) 1
    return $id
}

itcl::body Rappture::Map::setPlacardConfig { layerName attrlist style padding } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    set type [layer $layerName type]
    if {$type == "image" || $type == "elevation"} {
        error "Placard not supported on image or elevation layers"
    }
    array set placardConf {}
    foreach key { padding } {
        set placardConf($key) [set $key]
    }
    foreach key { attrlist style } {
        # Normalize whitespace
        set val [set $key]
        regsub -all "\[ \t\r\n\]+" [string trim $val] " " val
        set placardConf($key) $val
    }
    $_tree set $id "placard" [array get placardConf]
}

itcl::body Rappture::Map::getPlacardConfig { layerName } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    return [$_tree get $id "placard" ""]
}

itcl::body Rappture::Map::deleteLayer { layerName } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    $_tree delete $id
    array unset _dirty $layerName
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
    catch {
        foreach node [$_tree children root->"viewpoints"] {
            lappend list [$_tree label $node]
        }
    }
    return $list
}

# ----------------------------------------------------------------------
# USAGE: layer <layerName> <?prop?>
#
# Returns an array of settings for the named layer, or a single property
# if specified.
# ----------------------------------------------------------------------
itcl::body Rappture::Map::layer { layerName args } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    switch -- [llength $args] {
        0 {
            return [$_tree get $id]
        }
        1 {
            set prop [lindex $args 0]
            return [$_tree get $id $prop]
        }
        default {
            error "wrong # args: should be \"layer <layerName> <?prop?>\""
        }
    }
}

itcl::body Rappture::Map::hasLayer { layerName } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        return 0
    } else {
        return 1
    }
}

itcl::body Rappture::Map::setScript { layerName script } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    $_tree set $id "script" $script
    set _dirty($layerName) 1
}

itcl::body Rappture::Map::setStylesheet { layerName stylesheet } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    $_tree set $id "stylesheet" $stylesheet
    set _dirty($layerName) 1
}

itcl::body Rappture::Map::setColormap { layerName colormap } {
    set id [$_tree findchild root->"layers" $layerName]
    if { $id < 0 } {
        error "unknown layer \"$layerName\""
    }
    $_tree set $id "colorramp.colormap" $colormap
    set _dirty($layerName) 1
}

# ----------------------------------------------------------------------
# USAGE: selectors
#
# Returns a list of IDs for the selectors in a layer
# ----------------------------------------------------------------------
itcl::body Rappture::Map::selectors { layerName } {
    set list {}
    catch {
        foreach node [$_tree children root->"layers"->"$layerName"->"selectors"] {
            lappend list [$_tree label $node]
        }
    }
    return $list
}

# ----------------------------------------------------------------------
# USAGE: selector
#
# Returns an array of settings for the named selector in the named 
# layer
# ----------------------------------------------------------------------
itcl::body Rappture::Map::selector { layerName selectorName } {
    set id [$_tree findchild root->"layers"->"$layerName"->"selectors" $selectorName]
    if { $id < 0 } {
        error "unknown selector \"$selectorName\""
    }
    return [$_tree get $id]
}

itcl::body Rappture::Map::hasSelector { layerName selectorName } {
    set id [$_tree findchild root->"layers"->"$layerName"->"selectors" $selectorName]
    if { $id < 0 } {
        return 0
    } else {
        return 1
    }
}

itcl::body Rappture::Map::addSelector { layerName name params } {
    set nodeid $name
    set layerid [$_tree findchild root->"layers" $layerName]
    if { $layerid < 0 } {
        error "unknown layer \"$layerName\""
    }
    if {[hasSelector $layerName $nodeid]} {
        error "Selector '$nodeid' already exists"
    }
    set parent [$_tree findchild root->"layers"->"$layerName" "selectors"]
    if { $parent == "" } {
        set parent [$_tree insert $layerid -label "selectors"]
    }
    set child [$_tree insert $parent -label $nodeid]
    array set info $params
    foreach key { name style styleExpression query queryBounds queryOrderBy } {
        if { [info exists info($key)] &&
             $info($key) != ""} {
            $_tree set $child $key $info($key)
        }
    }
    set _dirty($layerName) 1
}

itcl::body Rappture::Map::deleteSelector { layerName selectorName } {
    set id [$_tree findchild root->"layers"->"$layerName"->"selectors" $selectorName]
    if { $id < 0 } {
        error "unknown selector \"$selectorName\""
    }
    $_tree delete $id
    set _dirty($layerName) 1
}

# ----------------------------------------------------------------------
# USAGE: viewpoint <viewpointName>
#
# Returns an array of settings for the named viewpoint
# ----------------------------------------------------------------------
itcl::body Rappture::Map::viewpoint { viewpointName } {
    set id [$_tree findchild root->"viewpoints" $viewpointName]
    if { $id < 0 } {
        error "unknown viewpoint \"$viewpointName\""
    }
    return [$_tree get $id]
}

itcl::body Rappture::Map::hasViewpoint { viewpointName } {
    set id [$_tree findchild root->"viewpoints" $viewpointName]
    if { $id < 0 } {
        return 0
    } else {
        return 1
    }
}

itcl::body Rappture::Map::addViewpoint { name props } {
    set nodeid $name
    if {[hasViewpoint $nodeid]} {
        error "Viewpoint '$nodeid' already exists"
    }
    set parent [$_tree findchild root "viewpoints"]
    set child [$_tree insert $parent -label $nodeid]
    $_tree set $child "name" $name
    set haveX 0
    set haveZ 0
    set haveSRS 0
    set haveVertDatum 0
    array set info $props
    foreach key { label description x y z distance heading pitch srs verticalDatum } {
        if {[info exists info($key)]} {
            set val $info($key)
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
        set lat $info(latitude)
        set long $info(longitude)
        $_tree set $child x $long
        $_tree set $child y $lat
        if {!$haveSRS} {
            $_tree set $child srs wgs84
        }
        if {!$haveVertDatum} {
            $_tree set $child verticalDatum ""
        }
    }
    if {!$haveZ && [info exists info(altitude)]} {
        $_tree set $child z $info(altitude)
    }
    set _dirty(viewpoints) 1
}

itcl::body Rappture::Map::deleteViewpoint { viewpointName } {
    set id [$_tree findchild root->"viewpoints" $viewpointName]
    if { $id < 0 } {
        error "unknown viewpoint \"$viewpointName\""
    }
    $_tree delete $id
    set _dirty(viewpoints) 1
}

# ----------------------------------------------------------------------
# USAGE: isGeocentric
#
# Returns if the map is geocentric (1) or projected (0)
# ----------------------------------------------------------------------
itcl::body Rappture::Map::isGeocentric {} {
    return [expr {[hints "type"] eq "geocentric"}]
}

itcl::body Rappture::Map::isFileProp { prop } {
    foreach fileprop {
        icon
        model
    } {
        if { $prop eq $fileprop } {
            return 1
        }
    }
    return 0
}

itcl::body Rappture::Map::parseStylesheet { stylesheet } {
    set styles [list]
    # First split into style blocks
    set blocks [split $stylesheet "\{\}"]
    if {[llength $blocks] == 1} {
        set blocks [list style $blocks]
    }
    foreach {styleName block} $blocks {
        # Get name/value pairs
        set styleName [string trim $styleName]
        if {$styleName == ""} { set styleName "style" }
        set block [string trim $block " \t\n\r\{\}"]
        if {$block == ""} { continue }
        #puts stderr "styleName: \"$styleName\""
        #puts stderr "block: \"$block\""
        set lines [split $block ";"]
        foreach line $lines {
            set line [string trim $line]
            if {$line == "" || [string index $line 0] == "#"} { continue }
            #puts stderr "line: \"$line\""
            set delim [string first ":" $line]
            set prop [string trim [string range $line 0 [expr {$delim-1}]]]
            set val [string trim [string range $line [expr {$delim+1}] end]]
            set ${styleName}($prop) $val
        }
        lappend styles $styleName [array get $styleName]
    }
    return $styles
}

itcl::body Rappture::Map::getFilesFromStylesheet { stylesheet } {
    set files [list]
    set styles [parseStylesheet $stylesheet]
    foreach {name style} $styles {
        #puts stderr "Style: \"$name\""
        array unset info
        array set info $style
        foreach key [array names info] {
            #puts stderr "Prop: \"$key\" Val: \"$info($key)\""
            if {[isFileProp $key]} {
                lappend files [string trim $info($key) "\""]
            }
        }
    }
    return $files
}
