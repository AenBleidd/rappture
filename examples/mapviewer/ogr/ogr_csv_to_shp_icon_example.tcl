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


# create an xyz layer for the OpenStreetMaps base layer
array set xyzParams {
    url {http://[abc].tile.openstreetmap.org/{z}/{x}/{y}.png}
}
array set osm {
    label "OSM Map"
    description "MapQuest OpenStreetMap Street base layer"
    opacity 1.0
}


# create a shape file from a csv file
# the vrt file documents how to read the csv file
# and the name of the lon/lat columns.
#
# use the following text as a template for the vrt file.
set vrt_template {
<OGRVRTDataSource>
  <OGRVRTLayer name="$shpfbase">
    <SrcDataSource relativeToVRT="1">.</SrcDataSource>
    <SrcLayer>$csvfbase</SrcLayer>
    <GeometryType>wkbPoint</GeometryType>
    <LayerSRS>$srs</LayerSRS>
    <GeometryField encoding="PointFromColumns" x="$lonColumn" y="$latColumn"/>
  </OGRVRTLayer>
</OGRVRTDataSource>
}

set shpfbase "output"
set shpfname "$shpfbase.shp"
set csvfbase "coord2"
set csvfname "$csvfbase.csv"
set vrtfname "$csvfbase.vrt"
set lonColumn "x"
set latColumn "y"
set srs "WGS84"


# substitute the longitude and latitude column names
# along with the name of the csv file, the shapefile
# and the SRS to use when createing the shapefile.
set vrt_data [string trim [subst $vrt_template]]

set fp [open $vrtfname "w"]
puts -nonewline $fp $vrt_data
close $fp

# use the ogr2ogr command to create a dbf file from the csv file
eval exec [list ogr2ogr -overwrite -f "ESRI Shapefile" . $csvfname]

# use the ogr2ogr command to create the shape file from the vrt file
eval exec [list ogr2ogr -overwrite -f "ESRI Shapefile" . $vrtfname]


# create a feature icon layer from a shape file using the OGR driver
# we add the local:// prefix to tell the map viewer widget that
# the file exists locally and needs to be transferred to the
# geovis server. Because this is a shape file that could require
# many other external files, all files that start with "output"
# will also be transferred to the geovis server.
set path "local://$shpfname"
array set ogrParams [subst {
    url {$path}
}]
array set iconParams {
    label  "CSV data as SHP"
    description "using OGR data provider to load data from a SHP file converted from a CSV file"
    style "-icon pin3 -declutter 0"
}

# create a map object
set map [Rappture::Map #auto]

# add all layers to the map object
$map addLayer image \
    osm [array get osm] \
    xyz [array get xyzParams]
$map addLayer icon \
    lonlats [array get iconParams] \
    ogr [array get ogrParams]

# add a map to the vis client
$mapviewer scale $map
$mapviewer add $map

# zoom to the csv file layer
after 2000 {
    $mapviewer camera zoom layer $map lonlats
}

# zoom to an extent
# must set srs to non-empty string
after 5000 {
    set xmin -25
    set ymin -40
    set xmax 60
    set ymax 45
    set duration 2.0
    # srs set from above
    $mapviewer camera zoom extent $xmin $ymin $xmax $ymax $duration $srs
}
