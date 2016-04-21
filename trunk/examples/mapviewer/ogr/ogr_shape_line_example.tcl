package require Tk
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

# create an feature line layer from a shape file using the OGR driver
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server. Because this is a shape file that could require
# many other external files, all files that start with "afr_countries"
# will also be transferred to the geovis server.
set path "local://afr_countries.shp"
array set ogrParams [subst {
    url {$path}
}]
array set layerParams {
    label  "African Countries Shape File (Line)"
    description "Shape file with line data for African country borders"
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer line \
    countries [array get layerParams] \
    ogr [array get ogrParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
