package require Tk
package require Rappture
package require RapptureGUI

Rappture::resources::load

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

# This method is called when a user modifies the selection in the map view.
# This callback is installed in the viewer using setSelectCallback in the code below.
# The callback has two arguments:
#   option - should be one of "annotation", "clear", "feature", "region"
#   args - depends on option, see below
proc selectHandler {option {args ""}} {
    switch $option {
        "annotation" {
            # An annotation (not in a feature layer) was selected, the single argument
            # is a list of the selected annotation names.
        }
        "clear" {
            # Previously selected features or annotations have been deselected.
            # No arguments.
            puts "select clear"
        }
        "feature" {
            # The feature selection set changed, the arguments are:
            #   op - "add", "delete" or "set"
            #   featureIdList - a list of feature identifiers.
            #   layerName - the name of the layer the features were found in.
            foreach {op featureIdList layerName} $args break
            switch $op {
                "add" {
                    puts "select feature add:\nfeatureIDList=\"$featureIdList\"\nlayerName=\"$layerName\""
                }
                "delete" {
                    puts "select feature delete:\nfeatureIDList=\"$featureIdList\"\nlayerName=\"$layerName\""
                }
                "set" {
                    puts "select feature set:\nfeatureIDList=\"$featureIdList\"\nlayerName=\"$layerName\""
                }
                default {
                    error "bad op \"$op\": should be one of: add, delete or set"
                }
            }
        }
        "region" {
            # An area defined by two wgs84 corner points was selected.
            # Here x is wgs84 decimal degrees longitude and y is wgs84 decimal degrees latitude.
            foreach {xmin ymin xmax ymax} $args break
            puts "select region: ($xmin, $ymin) - ($xmax, $ymax)"
        }
        default {
            error "bad option \"$option\": should be one of: annotation, clear, feature or region"
        }
    }
}

set width 400
set height 300
wm geometry . ${width}x${height}
update

set mapviewer [Rappture::MapViewer .g]

pack .g -expand yes -fill both

# Driver parameter array for XYZ provider
array set xyzParams {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
# Image layer parameter array
array set osmParams {
    label  "OSM Map"
    description "MapQuest OpenStreetMap Street base layer"
    opacity 1.0
}

# Driver parameter array for OGR provider
array set ogrParams {
    url {local://station_clean.csv}
}
# Icon layer parameter array
array set stationParams {
    label  "CSV data"
    description "using OGR data provider to load data from a CSV file"
    style "-icon pin4"
}
# Stylesheet for use with generic feature layer
set stylesheet "
station {
    icon: /opt/hubzero/rappture/render/lib/resources/pin04.png;
    icon-declutter: true;
    text-content: \[Building_Name\];
    text-fill: #000000;
    text-halo: #FFFFFF;
    text-declutter: true;
}
"

# Create a map object
set map [Rappture::Map #auto]

# Add all layers to the map object
$map addLayer image \
    osm [array get osmParams] \
    xyz [array get xyzParams]
$map addLayer icon \
    stations [array get stationParams] \
    ogr [array get ogrParams]
#$map addLayer feature \
    stations [array get stationParams] \
    ogr [array get ogrParams] \
    $stylesheet

# Add a map to the vis client
$mapviewer scale $map
$mapviewer add $map


# Set the proc/method to be called when a feature is selected on the map.
# In this case we give the full path to the previously defined
# procedure named "selectHandler". For an itcl class method, you may need to
# use itcl::code to get the full path of the callback method, e.g.
# [itcl::code $this selectHandler]
$mapviewer setSelectCallback ::selectHandler
