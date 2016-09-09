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

# create xyz layer
array set xyzParams {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
array set osm {
    label "OSM"
    cache false
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
array set landuse {
    label "Color Ramp"
    coverage true
}

# create an OGR Line layer
array set ogrParams {
    url {local://afr_countries.shp}
}
array set countries {
    label "Country boundaries"
    style "-color black -width 2.0"
}

# create a map object
set map [Rappture::Map #auto]

# -------------------------------------------------
# launch the mapviewer
# show individual layers of the map at 10 second intervals

# add the OSM layers to the map
$map addLayer image \
    osm [array get osm] \
    xyz [array get xyzParams]

# add the colorramp layer to the map
$map addLayer image \
    landuse [array get landuse] \
    colorramp [array get colorrampParams]

# add the line feature layer to the map
$map addLayer line \
    countries [array get countries] \
    ogr [array get ogrParams]

# add the map back to the viewer
$mapviewer scale $map
$mapviewer add $map

# after 10 seconds, delete the xyz OpenStreetMaps layer
# and update the map viewer widget with the new map.
after 10000 {
    # remove the colorramp layer from the map
    $map deleteLayer landuse

    # Update the map
    $mapviewer refresh
}

