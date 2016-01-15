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

# create xyz layer
set xyzdp [Rappture::GeoMapDataProviderXYZ #auto \
    {http://otile[1234].mqcdn.com/tiles/1.0.0/map/{z}/{x}/{y}.jpg} \
    -cache false]
set l1 [Rappture::GeoMapLayerImage #auto $xyzdp]


# create a colorramp layer
set colormap {
 0.00 0 0 1 1
 0.25 0 1 1 1
 0.50 0 1 0 1
 0.75 1 1 0 1
 1.00 1 0 0 1
}
set crdp [Rappture::GeoMapDataProviderColorramp #auto \
    {file:///usr/share/geovis/landuse/outlandish.tif} \
    -colormap $colormap]
set l2 [Rappture::GeoMapLayerImage #auto $crdp]
$l2 configure -coverage true


# create an OGR Line layer
set ogrdp [Rappture::GeoMapDataProviderOGR #auto \
    line {file:///data/mygeohub/tools/landuse/afr_countries.shp}]
set l3 [Rappture::GeoMapLayer #auto $ogrdp]


# create a map object
set m [Rappture::Map #auto]

# -------------------------------------------------
# launch the mapviewer
# show individual layers of the map at 10 second intervals

after 10000 {
    # add a layer to the map object
    set l1name [ \
        $m layer add -format blt_tree [ \
            $l1 export -format blt_tree]]

    # add a map to the vis client
    $s scale $m
    $s add $m
}

after 15000 {
    # remove the map from the viewer
    $s remove $m

    # remove the OSM layer from the map
    $m layer delete $l1name

    # add the colorramp layer to the map
    set l2name [ \
        $m layer add -format blt_tree [ \
            $l2 export -format blt_tree]]

    # add the map back to the viewer
    $s scale $m
    $s add $m
}

after 20000 {
    # remove the map from the viewer
    $s remove $m

    # remove the colorramp layer from the map
    $m layer delete $l2name

    # add the line feature layer to the map
    set l3name [ \
        $m layer add -format blt_tree [ \
            $l3 export -format blt_tree]]

    # add the map back to the viewer
    $s scale $m
    $s add $m
}

# build up the map layer by layer at 5 second intervals

after 25000 {
    # remove the map from the viewer
    $s remove $m

    # remove the line feature layer
    $m layer delete $l3name

    # add the OSM layers to the map
    $m layer add -format blt_tree [$l1 export -format blt_tree]

    # add the map back to the viewer
    $s scale $m
    $s add $m
}


after 30000 {
    # add the colorramp layer to the map
    $m layer add -format blt_tree [$l2 export -format blt_tree]

    # add the map back to the viewer
    $s scale $m
    $s add $m
}

after 35000 {
    # add the line feature layer to the map
    $m layer add -format blt_tree [$l3 export -format blt_tree]

    # add the map back to the viewer
    $s scale $m
    $s add $m
}

