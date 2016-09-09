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

# create an XYZ world layer
array set dp1 {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
array set l1 {
    label "OSM Map"
    description "MapQuest OpenStreetMap Street base layer"
    opacity 1.0
}

# create a XYZ Precip layer
array set dp2 {
    url {http://[abc].tile.openweathermap.org/map/precipitation/{z}/{x}/{y}.png}
}
array set l2 {
    label "OpenWeatherMap Precip"
    description "Precip layer from OWM"
    opacity 0.7
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    l1 [array get l1] \
    xyz [array get dp1]
$map addLayer image \
    l2 [array get l2] \
    xyz [array get dp2]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
