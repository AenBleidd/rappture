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


# create an XYZ world layer
set dp1 [Rappture::GeoMapDataProviderXYZ #auto \
    {http://otile[1234].mqcdn.com/tiles/1.0.0/map/{z}/{x}/{y}.jpg}]
set l1 [Rappture::GeoMapLayerImage #auto $dp1 \
        -label  "OSM Map" \
        -description "MapQuest OpenStreetMap Street base layer" \
        -opacity 1.0 ]


# create a colorramp layer
set colormap {
 -33.00 0.5 0.0 1.0 1.0
 -24.75 0.0 0.0 1.0 1.0
 -16.50 0.0 0.5 1.0 1.0
   0.00 1.0 1.0 1.0 1.0
  16.50 1.0 1.0 0.0 1.0
  33.00 1.0 0.0 0.0 1.0
}

# here we use the local:// prefix to specify that the
# file netcdf file needs to be transferred from the
# client to the server.
# the netcdf file for this data source is also available
# on the server at:
# file:///usr/share/geovis/ameantemp.nc

set path "local://[file join [file dirname [info script]] ameantemp.nc]"

set dp2 [Rappture::GeoMapDataProviderColorramp #auto $path \
    -colormap $colormap \
    -profile "global-geodetic" \
    -cache false]
set l2 [Rappture::GeoMapLayerImage #auto $dp2 \
        -label  "Annual Mean Temperature" \
        -description "NetCDF Colormap" \
        -opacity 0.7 \
        -coverage false]

# create a map object
set m [Rappture::Map #auto]

# add all layers to the map object
$m layer add -format blt_tree [$l1 export -format blt_tree]
$m layer add -format blt_tree [$l2 export -format blt_tree]

# add a map to the vis client
$s scale $m
$s add $m
