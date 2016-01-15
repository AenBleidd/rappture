package require RapptureGUI

set commondir [file join [file dirname [info script]] .. common]
source [file join $commondir geovis_settings.tcl]

set width 400
set height 300
wm geometry . ${width}x${height}
update

Rappture::MapViewer::SetServerList $GEOVIS_SERVERS
set s [Rappture::MapViewer .g [list localhost:2015]]

pack .g -expand yes -fill both

# create a OSGeo WMS layer
set dp1 [Rappture::GeoMapDataProviderWMS #auto \
    {http://vmap0.tiles.osgeo.org/wms/vmap0} "basic" "jpeg" -transparent false]
set l1 [Rappture::GeoMapLayerImage #auto $dp1 \
        -label  "OSGeo WMS Layer" \
        -description "base layer from OSGeo" \
        -opacity 0.9 ]

# create a Population Density 2000 layer
set dp4 [Rappture::GeoMapDataProviderWMS #auto \
    {http://sedac.ciesin.columbia.edu/geoserver/wms} \
    "gpw-v3:gpw-v3-population-density_2000" "png" -transparent true]
set l4 [Rappture::GeoMapLayer #auto $dp4 \
        -label  "Population Density 2000" \
        -description "Columbia Population Density 2000 Data" \
        -opacity 0.9 ]

# create a crop climate layer
set dp5 [Rappture::GeoMapDataProviderWMS #auto \
    {http://sedac.ciesin.columbia.edu/geoserver/wms} \
    "crop-climate:crop-climate-effects-climate-global-food-production" \
    "png" -transparent true]
set l5 [Rappture::GeoMapLayer #auto $dp5 \
        -label  "Crop Climate" \
        -description "Columbia Crop Climate Data" \
        -opacity 0.5 ]


# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]
$m layer add -format blt_tree [$l4 export -format blt_tree]
$m layer add -format blt_tree [$l5 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
