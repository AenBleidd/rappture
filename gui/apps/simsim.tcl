#!/bin/sh
#\
exec wish "$0" $*

package require Rappture
package require RapptureGUI
# package require cmdline

proc defaultHandler {child} {
    set childchildList {}
    set defaultList [$child children -as object -type "default"]
    if {[llength $defaultList] != 0} {
        set defaultNode [lindex $defaultList 0]
        set value [$defaultNode get]
        if {$value == ""} {
            set childchildList [$child children]
        } else {
            $child put "current" $value
        }
    } else {
        if {"components" != [$child element -as type]} {
            set childchildList [$child children]
        } elseif {"parameters" != [$child element -as type]} {
            set childchildList [$child children]
        }
    }
    return $childchildList
}

proc random {m M} {
    return [expr {$m+(rand()*($M-$m))}]
}

proc randomInt {m M} {
    return [expr {$m+(int(rand()*($M-$m)))}]
}


proc numberHandler {child} {
    set value ""
    set units [$child get "units"]
    set min [$child get "min"]
    set max [$child get "max"]

    if {"" != $min} {
        if {"" != $units} {
            set min [Rappture::Units::convert $min -to $units -units off]
        }
    }

    if {"" != $max} {
        if {"" != $units} {
            set max [Rappture::Units::convert $max -to $units -units off]
        }
    }

    if {"yes" == [$child get "preset"]} {
        $child remove "preset"
    } else {
        if { ("" != $min) && ("" != $max) } {
            set value [random $min $max]
            $child put current $value$units
        # } elseif { {"" != $min} && {"" == $max} } {
            # min value provided
        # } elseif { {"" == $min} && {"" != $max} } {
            # max value provided
        } else {
            defaultHandler(child)
        }
    }


}

proc integerHandler {child} {
    set value ""
    set units [$child get "units"]
    set min [$child get "min"]
    set max [$child get "max"]

    if {"" != $min} {
        if {"" != $units} {
            set min [Rappture::Units::convert $min -to $units -units off]
        }
    }

    if {"" != $max} {
        if {"" != $units} {
            set max [Rappture::Units::convert $max -to $units -units off]
        }
    }

    if {"yes" == [$child get "preset"]} {
        $child remove "preset"
    } else {
        if { ("" != $min) && ("" != $max) } {
            set value [randomInt $min $max]
            $child put current $value$units
        # } elseif { {"" != $min} && {"" == $max} } {
            # min value provided
        # } elseif { {"" == $min} && {"" != $max} } {
            # max value provided
        } else {
            defaultHandler(child)
        }
    }

}

proc booleanHandler {child} {
    if {"yes" == [$child get "preset"]} {
        $child remove "preset"
    } else {
        set value [expr {int(rand()*2)}]
        if {value == 1} {
            set value "yes"
        } else {
            set value "no"
        }
        $child put "current" $value
    }
}

proc defaultize {xmlobj} {
    set childList [$xmlobj children -as object input]
    foreach child $childList {
        set type [$child element -as type]
        if {"number" == $type} {
            defaultHandler $child
        } elseif {"integer" == $type} {
            defaultHandler $child
        } elseif {"boolean" == $type} {
            defaultHandler $child
        } elseif {"string" == $type} {
            defaultHandler $child
        } elseif {"loader" == $type} {
            defaultHandler $child
        } elseif {"structure" == $type} {
            defaultHandler $child
        } else {
            defaultHandler $child
        }
    }
}

proc randomize {presetArr xmlobj} {
    upvar $presetArr presets
    set childList [$xmlobj children -as object input]

    foreach child $childList {
        set cpath [$child element -as path]

        if {"." == [string index $cpath 0]} {
            # this is because tcl's library module (element -as path)
            # returns a crazy dot in the 0th position
            set cpath [string range $cpath 1 end]
        }

        set ppath [$child parent -as path]
        set cPresets [array get presets $cpath*]

        foreach {cPresetsPath cPresetsVal} $cPresets {
            set cutIdx [expr {[string length $cpath] + 1}]
            set iPath [string range $cPresetsPath $cutIdx end]
            $child put $iPath $cPresetsVal
            $child put "preset" "yes"
            unset presets($cPresetsPath)
        }

        switch -- [$child element -as type] {
            number      { numberHandler  $child }
            integer     { integerHandler $child }
            boolean     { booleanHandler $child }
            string      { defaultHandler $child }
            loader      { loaderHandler  $child }
            structure   { defaultHandler $child }
            default     { defaultHandler $child }
        }

    }
}

proc writeDriver {lib} {
   set driverFile "driver[clock seconds].xml"
   set fp [open $driverFile "w"]
   puts -nonewline $fp [$lib xml]
   close $fp
   return $driverFile
}

proc parsePathVal {listVar returnVar} {
    upvar $listVar params
    upvar $returnVar presetArr
    catch {unset presetArr}

    # initialize variables
    set pathValStr ""
    set match "junk"

    while {"" != $match} {
        set match ""
        set path ""
        set val ""
        set pathValStr [string trimleft [join $params " "]]

        # search the params for:
        # 1) xml path
        # 2) followed by = sign
        # 3) followed by a starting " sign
        # 4) followed by any text (including spaces)
        # 5) followed by an ending " sign
        # ex: input.number(temperature).current="400K"
        # ex: input.number(temperature).current=400K
        # ex: input.string(blahh).current="hi momma, i love you!"
        #    \"([^\"]+)\"
        #    ((\"([^\"]+)\")|(:([^:]+):)|(\'([^\']+)\')|([^\s]+))
        regexp -expanded {
            (
                [a-zA-Z0-9]+(\([a-zA-Z0-9]+\))?
                (\.[a-zA-Z0-9]+(\([a-zA-Z0-9]+\))?)*
            )
            =
            ((\"([^\"]+)\")|(\'([^\']+)\')|([^\s]+))
        } $pathValStr match path junk junk junk val junk
        if {"" != $match} {
            # remove the matching element from orphaned params list
            foreach p [split $match] {
                set paramsIdx [lsearch -exact $params $p]
                set params [lreplace $params $paramsIdx $paramsIdx]
            }

            # add the name and value to our preset array
            set presetArr($path) $val
        }
    }
}

proc printHelp {} {
    puts "simsim ?--tool <path> | --driver <path>? ?--randomize? ?--compare <path>? ?--help?"
    puts ""
    puts " --tool <path>         - use the tool.xml file specified at <path>"
    puts " --randomize           - use random values instead of default"
    puts "                         values for inputs elements"
    puts " --driver <path>       - run the application with the provided"
    puts "                         driver.xml file."
    puts " --compare <path>      - compare the results with the provided"
    puts "                         run.xml file."
    puts " --help                - print this help menu."
    exit 0
}

proc diffs {xmlobj1 xmlobj2} {
    set rlist ""

    # query the values for all entities in both objects
    set thisv [concat [Rappture::entities $xmlobj1 "input"] [Rappture::entities $xmlobj1 "output"]]
    set otherv [concat [Rappture::entities $xmlobj2 "input"] [Rappture::entities $xmlobj2 "output"]]

    # scan through values for this object and compare against other one
    foreach path $thisv {
        set i [lsearch -exact $otherv $path]
        if {$i < 0} {
            foreach {raw norm} [value $xmlobj1 $path] break
            lappend rlist - $path $raw ""
        } else {
            foreach {traw tnorm} [value $xmlobj1 $path] break
            foreach {oraw onorm} [value $xmlobj2 $path] break
            if {![string equal $tnorm $onorm]} {
                lappend rlist c $path $traw $oraw
            }
            set otherv [lreplace $otherv $i $i]
        }
    }

    #add any values left over in the other object
    foreach path $otherv {
        foreach {oraw onorm} [Rappture::LibraryObj::value $xmlobj2 $path] break
        lappend rlist + $path "" $oraw
    }
    return $rlist
}

proc value {libobj path} {
    switch -- [$libobj element -as type $path] {
        structure {
            set raw $path
            # try to find a label to represent the structure
            set val [$libobj get $path.about.label]
            if {"" == $val} {
                set val [$libobj get $path.current.about.label]
            }
            if {"" == $val} {
                if {[$libobj element $path.current] != ""} {
                    set comps [$libobj children $path.current.components]
                    set val "<structure> with [llength $comps] components"
                } else {
                    set val "<structure>"
                }
            }
            return [list $raw $val]
        }
        number {
            # get the usual value...
            set raw ""
            if {"" != [$libobj element $path.current]} {
                set raw [$libobj get $path.current]
            } elseif {"" != [$libobj element $path.default]} {
                set raw [$libobj get $path.default]
            }
            if {"" != $raw} {
                set val $raw
                # then normalize to default units
                set units [$libobj get $path.units]
                if {"" != $units} {
                    set val [Rappture::Units::convert $val \
                        -context $units -to $units -units off]
                }
            }
            return [list $raw $val]
        }
        curve {
            set raw ""
            if {"" != [$libobj element $path.component.xy]} {
                set raw [$libobj get $path.component.xy]
            }
            return [list $raw $raw]
        }
        log {
            set raw ""
            if {"" != [$libobj element]} {
                set raw [$libobj get]
            }
            return [list $raw $raw]
        }
        cloud {
            set raw ""
            if {"" != [$libobj element $path.points]} {
                set raw [$libobj get $path.points]
            }
            return [list $raw $raw]
        }
        field {
            set raw ""
            if {"" != [$libobj element $path.component.values]} {
                set raw [$libobj get $path.component.values]
            }
            return [list $raw $raw]
        }


    }

    # for all other types, get the value (current, or maybe default)
    set raw ""
    if {"" != [$libobj element $path.current]} {
        set raw [$libobj get $path.current]
    } elseif {"" != [$libobj element $path.default]} {
        set raw [$libobj get $path.default]
    }
    return [list $raw $raw]
}




# keep the wish window from popping up
wm withdraw .

# set default values
set intf "./tool.xml"
set cintf ""
set defaults false
array set presets []
set i 0
set oParams {}

#set options {
#    {tool.arg       "./tool.xml"    "use the specified tool.xml"}
#    {driver.arg     ""              "use the specified driver.xml"}
#    {compare.arg    ""              "compare the result with this run.xml"}
#    {randomize                      "use random values for input elements"}
#}
#
#set usage " \[options] ?<path>=<val> <path>=<val> ...? \noptions:"
#
#array set params [cmdline::getoptions argv $options $usage]
#
#set intf $params(tool)
#
#foreach n [array names params] {
#    puts "params($n) = $params($n)"
#}
#exit 0

# parse command line arguments
set argc [llength $argv]
for {set i 0} {$i < $argc} {incr i} {
    set opt [lindex $argv $i]
    if {("-t" == $opt) || ("--tool" == $opt)} {
        if {[expr {$i + 1}] < $argc} {
            incr i
            set intf [lindex $argv $i]
            # puts "using $intf"
            # need to check to see if file exists, if not raise error
        } else {
            printHelp
        }
    } elseif {("-d" == $opt) || ("--driver" == $opt)} {
        if {[expr {$i + 1}] < $argc} {
            incr i
            set intf [lindex $argv $i]
            # puts "using $intf"
            # need to check to see if file exists, if not raise error
        } else {
            printHelp
        }
    } elseif {("-r" == $opt) || ("--randomize" == $opt)} {
        set defaults false
    } elseif {("-c" == $opt) || ("--compare" == $opt)} {
        if {[expr {$i + 1}] < $argc} {
            incr i
            set cintf [lindex $argv $i]
            # puts "comparing results with $cintf"
            # need to check to see if file exists, if not raise error
        } else {
            printHelp
        }
    } elseif {("-h" == $opt) || ("--help" == $opt)} {
        printHelp
    } else {
        # puts "bad value: $argv"
        # printHelp
        # place all extra params in the params array
        lappend oParams $opt
    }
}

# parse out path=val combinations from the list of orphaned parameters
parsePathVal oParams presets
if {0 != [llength $oParams]} {
    puts "Could not understand the following parameters"
    puts "oParams = $oParams"
    puts "length oParams = [llength $oParams]"
}

set err ""
if {! [file exists $intf]} {
    append err "\ntool file \"" $intf "\" does not exist, use -t option\n"
    puts $err
    printHelp
}

set xmlobj [Rappture::library $intf]
set installdir [file dirname $intf]

if {true == $defaults} {
    defaultize $xmlobj
} else {
    randomize presets $xmlobj
}
# pick defaults $xmlobj $presets
# pick randoms $xmlobj $presets

set tool [Rappture::Tool ::#auto $xmlobj $installdir]

# read the run.xml file.
# from analyzer.tcl:

# execute the job
foreach {status result} [eval $tool run] break

# read back the result from run.xml
if {$status == 0 && $result != "ABORT"} {
    if {[regexp {=RAPPTURE-RUN=>([^\n]+)} $result match file]} {
        set resultxmlobj [Rappture::library $file]

        # do comparison if user chose to compare with other results
        if {"" != $cintf} {
            if {"" != $resultxmlobj} {
                set compobj [Rappture::library $cintf]
                set difflist [diffs $resultxmlobj $compobj]
                if {[llength $difflist] > 0} {
                    puts $difflist
                }
            }
        }
    } else {
        set status 1
        puts "Can't find result file in output.\nDid you call Rappture::result in your simulator?"
    }
}

exit 0
