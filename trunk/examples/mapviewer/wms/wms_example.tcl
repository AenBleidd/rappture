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

# create a OSGeo WMS layer
array set dp1 {
    url {http://vmap0.tiles.osgeo.org/wms/vmap0}
    layers "basic"
    format "jpeg"
    transparent false
}
array set l1 {
    label "OSGeo WMS Layer"
    description "base layer from OSGeo"
    opacity 1.0
}

# create a Population Density 2000 layer
array set dp4 {
    url {http://sedac.ciesin.columbia.edu/geoserver/wms}
    layers "gpw-v3:gpw-v3-population-density_2000"
    format "png"
    transparent true
}
array set l4 {
    label "Population Density 2000"
    description "Columbia Population Density 2000 Data"
    opacity 0.9
}

# create a crop climate layer
array set dp5 {
    url {http://sedac.ciesin.columbia.edu/geoserver/wms}
    layers "crop-climate:crop-climate-effects-climate-global-food-production"
    format "png"
    transparent true
}
array set l5 {
    label "Crop Climate"
    description "Columbia Crop Climate Data"
    opacity 0.5
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    l1 [array get l1] \
    wms [array get dp1]

$map addLayer image \
    l4 [array get l4] \
    wms [array get dp4]

$map addLayer image \
    l5 [array get l5] \
    wms [array get dp5]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
