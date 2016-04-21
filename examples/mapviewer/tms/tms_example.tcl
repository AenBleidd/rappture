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

# create an TMS layer
array set tmsParams {
    url {http://readymap.org/readymap/tiles/1.0.0/7/}
}
array set layerParams {
    label  "ReadyMap Aerial"
    description "ReadyMap Satellite/Aerial Imagery"
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    readymap [array get layerParams] \
    tms [array get tmsParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
