package require Tk
package require Rappture
package require RapptureGUI

set colors23 {
    FF0000FF
    FF8000FF
    FFAA00FF
    FFD500FF
    FFFF00FF
    D5FF00FF
    AAFF00FF
    80FF00FF
    55FF00FF
    2BFF00FF
    00FF00FF
    00FF2AFF
    00FF55FF
    00FF80FF
    00FFAAFF
    00FFD4FF
    00FFFFFF
    00D4FFFF
    00AAFFFF
    0080FFFF
    0055FFFF
    002BFFFF
    0000FFFF
}

set cutoffs24 {
    0.019100
    0.067600
    0.083600
    0.099600
    0.116000
    0.132000
    0.148000
    0.164000
    0.180000
    0.196000
    0.212000
    0.228000
    0.244000
    0.260000
    0.276000
    0.292000
    0.308000
    0.324000
    0.340000
    0.356000
    0.372000
    0.388000
    0.404000
    0.420000
}


proc handleSelect {option {args ""}} {
    global selectedFeatures
    switch $option {
        "annotation" {
            # User clicked a map annotation (not a feature) like a user-added pin, etc.
         }
        "clear" {
            # Clear any previous selection, user clicked on background where there is no feature
            # No args
        }
        "feature" {
            # args: op featureIDList layerName
            foreach {op featureIdList layerName} $args break
            switch $op {
                "add" {
                }
                "set" {
                    foreach {op featureIdList layerName} $args break
                    lappend selectedFeatures $featureIdList
                    adjustLayerHighlight $op $selectedFeatures $layerName
                }
                "remove" {
                }
            }
        }
    }
}


proc adjustLayerHighlight {option featureIdList layerName} {
    global map
    global mapviewer
    global stylesheet
    global cutoffs24
    set selectors [generateSelectors $cutoffs24 $featureIdList]
    $map deleteLayer "countries"
    set layerName "countries"
    addCountryLayer $map $layerName $stylesheet $selectors
    $mapviewer refresh
}

proc generateStyleSheet {colors} {

    set style_t {
      sID {
        fill: #COLOR;
        stroke: #000000ff;
        stroke-width: 3;
        altitude-clamping: terrain-drape;
      }
    }
    set stylesheet ""
    for {set i 0} {$i < [llength $colors]} {incr i} {
        # append stylesheet "\n" [string map {ID $i COLOR [lindex $colors $i]} $style_t]
        set s $style_t
        regsub ID $s $i s
        regsub COLOR $s [lindex $colors $i] s
        append stylesheet $s
    }

    # add a color for fields marked as NA or -1 where there was no data
    append stylesheet [string map {ID NA COLOR f2f2f2ff} $style_t]

    # add a color for selected fields
    append stylesheet [string map {ID SE COLOR 000000FF} $style_t]

    return $stylesheet
}

proc generateSelectors {cutoffs {featureIdList {}}} {
    set selectors ""
    set numSelectors [llength $cutoffs]
    for {set i 0} {$i < $numSelectors-1} {incr i} {
       set lower [lindex $cutoffs $i]
       set upper [lindex $cutoffs [expr $i+1]]
       lappend selectors [list id $i style s$i query "elas >= $lower and elas < $upper"]
    }

    # add a selector for fields marked as NA or -1 where there was no data
    incr i
    lappend selectors [list id $i style sNA query "elas = -1"]

    # add a selector for selected fields
    incr i
    lappend selectors [list id $i style sSE query "FID IN ([join $featureIdList ","])"]

    return $selectors
}

proc addCountryLayer {map layerName stylesheet selectors} {
    # Parameters for feature layer
    array set ogrParams {
        url {local://afr_elas.shp}
    }
    array set countries {
        label   "Countries"
        opacity 1.0
    }

    # Configure layers
    $map addLayer feature \
        $layerName [array get countries] \
        ogr [array get ogrParams] \
        $stylesheet "" $selectors
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

# setup a global variable to accumulate selected features
set selectedFeatures {0 1 2 3 4}

set stylesheet [generateStyleSheet $colors23]
set selectors [generateSelectors $cutoffs24 $selectedFeatures]

# Create a map object
set map [Rappture::Map #auto]

set layerName "countries"
addCountryLayer $map $layerName $stylesheet $selectors

# Configure the placard
set placardAttributes [list \
    name "Country Name" \
    ISO3 "Country Code" \
    elas "Elasticity" \
]
set placardStyle {
    fill: #B6B6B688;
    text-fill: #000000;
    text-size: 12.0;
}
set placardPadding 5
$map setPlacardConfig countries \
    $placardAttributes \
    $placardStyle \
    $placardPadding


# Add map to viewer
$mapviewer add $map
$mapviewer scale $map

# setup feature callback
$mapviewer setSelectCallback "handleSelect"

after 2000 {
    $mapviewer camera zoom layer $map $layerName
}

