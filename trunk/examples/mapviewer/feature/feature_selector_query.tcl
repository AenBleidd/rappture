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

# Parameters for base layer
array set xyzParams {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
array set osm {
    label "OSM"
    cache false
}

# Parameters for feature layer
array set ogrParams {
    url {local://afr_elas.shp}
}
array set countries {
    label   "Countries"
    opacity 0.7
}
set stylesheet {
  s1 {
    fill: #0000ff;
    stroke: #000000;
    stroke-width: 3;
    altitude-clamping: terrain-drape;
  }
  s2 {
    fill: #ff0000;
    stroke: #000000;
    stroke-width: 3;
    altitude-clamping: terrain-drape;
  }
}
array set selector1 {
    id    1
    style s1
    query "POP2005 < 5000000"
}
array set selector2 {
    id    2
    style s2
    query "POP2005 >= 5000000"
}
set numSelectors 2
for {set i 1} {$i < $numSelectors+1} {incr i} {
   lappend selectors [array get selector$i]
}

# Create a map object
set map [Rappture::Map #auto]

# Configure layers
$map addLayer image \
    osm [array get osm] \
    xyz [array get xyzParams]

$map addLayer feature \
    countries [array get countries] \
    ogr [array get ogrParams] \
    $stylesheet "" $selectors

# Add map to viewer
$mapviewer add $map
$mapviewer scale $map

