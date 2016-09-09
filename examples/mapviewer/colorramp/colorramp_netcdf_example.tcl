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

set path "local://ameantemp.nc"

array set colorrampParams [subst {
    url {$path}
    colormap {$colormap}
}]
array set mtempParams {
    label "Annual Mean Temperature"
    description "NetCDF Colormap"
    opacity 0.7
    profile "global-geodetic"
    cache false
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    osm [array get osmParams] \
    xyz [array get xyzParams]
$map addLayer image \
    mtemp [array get mtempParams] \
    colorramp [array get colorrampParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map
