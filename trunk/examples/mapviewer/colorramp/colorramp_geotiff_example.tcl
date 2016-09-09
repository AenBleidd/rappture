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

# create a colorramp layer
set colormap {
 0.00 0 0 1 1
 0.25 0 1 1 1
 0.50 0 1 0 1
 0.75 1 1 0 1
 1.00 1 0 0 1
}

array set colorrampParams [subst {
    url {file:///usr/share/geovis/landuse/scplot_with_coordinate_system.tif}
    colormap {$colormap}
}]
array set landUseParams {
    label "Land Use"
    description "Land Use data from Landparam tool"
    opacity 1.0
    coverage true
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    osm [array get osmParams] \
    xyz [array get xyzParams]
$map addLayer image \
    landUse [array get landUseParams] \
    colorramp [array get colorrampParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
