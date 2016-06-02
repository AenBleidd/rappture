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


# Parameters for feature layer
array set ogrParams {
    url {local://afr_elas.shp}
}
array set countries {
    label   "Countries"
    opacity 1.0
}
set stylesheet {
  s1 {
    fill: #98AFC7;
    stroke: #000000;
    stroke-width: 3;
    altitude-clamping: terrain-drape;
  }
  s2 {
    fill: #00FFFF;
    stroke: #000000;
    stroke-width: 3;
    altitude-clamping: terrain-drape;
  }
}
array set selector1 {
    id    1
    style s1
    query "POP2005 >= 0"
}
array set selector2 {
    id    2
    style s2
    query "FID IN (0,1)"
}
set numSelectors 2
for {set i 1} {$i <= $numSelectors} {incr i} {
   lappend selectors [array get selector$i]
}

# Create a map object
set map [Rappture::Map #auto]

# Configure layers
set layerId 0
$map addLayer feature \
    countries$layerId [array get countries] \
    ogr [array get ogrParams] \
    $stylesheet "" $selectors

# Add map to viewer
$mapviewer add $map
$mapviewer scale $map

after 2000 {
    $mapviewer camera zoom layer $map countries$layerId
}

after 5000 {
    puts "updating layer: reusing layer name (delete/add)"

    $map deleteLayer countries$layerId

    set selector2(query) "FID IN (0,1,2,3,4,5,6,7,8,12,15,44)"
    set selectors ""
    for {set i 1} {$i <= $numSelectors} {incr i} {
       lappend selectors [array get selector$i]
    }

    $map addLayer feature \
        countries$layerId [array get countries] \
        ogr [array get ogrParams] \
        $stylesheet "" $selectors

    puts "refreshing mapviewer"
    $mapviewer refresh
}


after 10000 {
    puts "updating layer: new layer name"

    set selector2(query) "FID IN (7,8,12,15,44)"
    set selectors ""
    for {set i 1} {$i <= $numSelectors} {incr i} {
       lappend selectors [array get selector$i]
    }

    incr layerId
    $map addLayer feature \
        countries$layerId [array get countries] \
        ogr [array get ogrParams] \
        $stylesheet "" $selectors

    puts "refreshing mapviewer"
    $mapviewer refresh
}


after 15000 {
    puts "updating layer selectors using addSelector/deleteSelector"

    $map deleteSelector countries$layerId $selector2(id)
    set selector2(query) "FID IN (33,34,35,36,37,38,39,4,48,21,43)"
    $map addSelector countries$layerId $selector2(id) [array get selector2]

    puts "refreshing mapviewer"
    $mapviewer refresh
}
