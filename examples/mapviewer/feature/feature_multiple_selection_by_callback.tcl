package require Tk
package require Rappture
package require RapptureGUI

proc handleSelect {option {args ""}} {
    global selectedFeatures

    switch $option {
        "annotation" {
            # User clicked a map annotation (not a feature) like a user-added pin, etc.
         }
        "clear" {
            # Clear any previous selection, user clicked on background where there is no feature
            # No args
            set selectedFeatures ""
        }
        "feature" {
            # args: op featureIDList layerName
            foreach {op featureIdList layerName} $args break

            puts "select feature $op:\tfeatureIDList=\"$featureIdList\"\tlayerName=\"$layerName\""

            # strip off the mapId from the layerName
            regexp {([^-]*)-(.*)} $layerName matched mapId layerName

            switch $op {
                "add" {
                    set selectedFeatures [concat $selectedFeatures $featureIdList]
                }
                "set" {
                    set selectedFeatures $featureIdList
                }
                "remove" {
                    foreach i $featureIdList {
                        set id [lsearch -exact $selectedFeatures $i]
                        if {$id > 0} {
                            set selectedFeatures [lreplace $selectedFeatures $id $id]
                        }
                    }
                }
            }
            updateHighlightedFeatures $layerName $selectedFeatures
        }
    }
}

proc updateHighlightedFeatures {layerName selectedFeatures} {
    global map
    global mapviewer
    global selector2

    $map deleteSelector $layerName $selector2(id)
    set selector2(query) [concat "FID IN (" [join $selectedFeatures ","] ")"]
    $map addSelector $layerName $selector2(id) [array get selector2]

    puts "refreshing mapviewer"
    $mapviewer refresh
}


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

# Track of which feature ids have been selected
set selectedFeatures "0 1"

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

$mapviewer setSelectCallback "handleSelect"

after 2000 {
    $mapviewer camera zoom layer $map countries$layerId
}

after 5000 {
    # simulating selecting features from outside of the map
    $mapviewer select feature add "44 48 43 21" $map-countries$layerId
    puts "select feature add ..."
}
