package require Rappture
package require RapptureGUI

Rappture::resources::load

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

set width 400
set height 300
wm geometry . ${width}x${height}
update

set mapviewer [Rappture::MapViewer .g]

pack .g -expand yes -fill both

# create an feature point layer from a csv file using the OGR driver
# the longitude and latitude fields must be named "longitude" and
# "latitude" respectively
#
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server.
# set path "local://coord.csv"
set path "local://station_clean.csv"
array set ogrParams [subst {
    url {$path}
}]
array set layerParams {
    label "CSV point data"
    description "using OGR data provider to load CSV point data"
    style "-color red -size 5"
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer point \
    points [array get layerParams] \
    ogr [array get ogrParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
