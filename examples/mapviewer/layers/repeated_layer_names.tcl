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

# create xyz layer
array set xyzParams {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
array set osm {
    label "OSM"
    cache false
}

# create an feature icon layer from a csv file using the OGR driver
# the longitude and latitude fields must be named "longitude" and
# "latitude" respectively
#
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server.
# set path "local://coord.csv"
set path "local://coord1.csv"
array set ogrParams1 [subst {
    url {$path}
}]
array set iconParams1 {
    label  "CSV data 1"
    description "using OGR data provider to load data from a CSV file"
    style "-icon pin3"
}

set path "local://coord2.csv"
array set ogrParams2 [subst {
    url {$path}
}]
array set iconParams2 {
    label  "CSV data 2"
    description "using OGR data provider to load data from a CSV file"
    style "-icon pin7"
}

# create a map object
set map [Rappture::Map #auto]

# add a layer to the map object
$map addLayer image \
    osm [array get osm] \
    xyz [array get xyzParams]

# add csv layer 1 to the map object
$map addLayer icon \
    csvdata [array get iconParams1] \
    ogr [array get ogrParams1]

$mapviewer scale $map
$mapviewer add $map


after 5000 {
    # remove csv layer 1 from the map object
    $map deleteLayer csvdata

    # add csv layer 2 to the map object
    # with the same layer name as our previous layer
    $map addLayer icon \
        csvdata [array get iconParams2] \
        ogr [array get ogrParams2]

    # Update map viewer
    $mapviewer clear
    $mapviewer add $map
    puts stderr "Deleted csv layer 1, Added csv layer 2"
}
