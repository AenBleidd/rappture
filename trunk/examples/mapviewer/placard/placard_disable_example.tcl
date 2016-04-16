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

# create an XYZ world layer
array set xyzParams {
    url {http://otile[1234].mqcdn.com/tiles/1.0.0/map/{z}/{x}/{y}.jpg}
}
array set osm {
    label "OSM Map"
    description "MapQuest OpenStreetMap Street base layer"
}

# create an feature point layer from a csv file using the OGR driver
# the longitude and latitude fields must be named "longitude" and
# "latitude" respectively
#
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server.
# set path "local://[file join [file dirname [info script]] coord.csv]"
set path "local://station_clean.csv"
array set ogrParams [subst {
    url {$path}
}]
array set stations {
    label "CSV data"
    description "using OGR data provider to load data from a CSV file"
}
array set placardAttributes {
    latitude Lat
    longitude Lon
}
set placardStyle {
    fill: #B6B6B688;
    text-fill: #000000;
    text-size: 12.0;
}
set placardPadding 5

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    osm [array get osm] \
    xyz [array get xyzParams]
$map addLayer icon \
    stations [array get stations] \
    ogr [array get ogrParams]
$map setPlacardConfig stations \
    [array get placardAttributes] \
    $placardStyle \
    $placardPadding

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map

# Make sure the MapViewer rebuilds the map
update idletasks

# disable the placard for the "stations" layer of $map
$mapviewer placard enable off "$map-stations"
