package require Rappture
package require RapptureGUI

Rappture::resources::load

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

# this method is called when a user clicks on a icon in the map
# the method is used at the bottom of this example
# the callback is called with two arguments:
# option - should be one of "annotation", "clear", "feature"
proc handler {option {args ""}} {
    switch $option {
        "annotation" {
            # pass
        }
        "clear" {
            # previously selected features have been unselected.
            # no arguments associated with this option.
            puts "select clear"
        }
        "feature" {
            # a feature was selected, the server returns 4 values about the feature
            # that are held in args:
            # globalObjId - the global object identifier
            # featureIdList - a single value or list of feature identifiers.
            #                 if multiple features are returned, they arei
            #                 enclosed in curly brackets.
            # numFeaturesInLayer - the number of features in the layer
            # layerName - the name of the layer the features were found in.
            #             only features from a single layer can be selected
            #             at a time.
            foreach {globalObjId featureIdList numFeaturesInLayer layerName} $args break
            puts "handler caught\
                globalObjId=\"$globalObjId\"\
                featureIdList=\"$featureIdList\"\
                numFeaturesInLayer=\"$numFeaturesInLayer\"\
                layerName=\"$layerName\""
        }
        "region" {
            puts "select region $args"
        }
        default {
            error "bad option \"$option\": should be one of annotation,\
                clean, or feature"
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
    url {http://otile[1234].mqcdn.com/tiles/1.0.0/map/{z}/{x}/{y}.jpg}
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

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
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

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map


# set the proc/method to be called when a feature is selected on the map.
# in this case we give the full path to the previously defined
# procedure named "handler". for an itcl class method, you may need to
# use itcl::code to get the full path of the  callback method:
# for example: [itcl::code $this handler]
$mapviewer setSelectCallback ::handler
