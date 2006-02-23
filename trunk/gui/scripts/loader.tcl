# ----------------------------------------------------------------------
#  COMPONENT: loader - widget for loading examples and old runs
#
#  This widget is a glorified combobox that is used to load various
#  example files into the application.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *Loader.textForeground black widgetDefault
option add *Loader.textBackground white widgetDefault

itcl::class Rappture::Loader {
    inherit itk::Widget

    itk_option define -tool tool Tool ""

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _uploadValue {string}
    protected method _tooltip {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this loader
    private variable _lastlabel "";# label of last example loaded

    private variable _uppath ""   ;# path to Upload... component
    private variable _updesc ""   ;# description for Upload... data
    private variable _upfilter "" ;# filter used for upload data

    private variable _dnpath ""   ;# path to Download... component
}

itk::usual Loader {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    itk_component add combo {
        Rappture::Combobox $itk_interior.combo -editable no
    } {
        usual
        keep -width
    }
    pack $itk_component(combo) -expand yes -fill both
    bind $itk_component(combo) <<Value>> [itcl::code $this _newValue]

    eval itk_initialize $args

    # example files are stored here
    if {$itk_option(-tool) != ""} {
        set fdir [$itk_option(-tool) installdir]
    } else {
        set fdir "."
    }
    set defval [$_owner xml get $path.default]

    #
    # If this loader has a <new> section, then create that
    # entry first.
    #
    set newfile ""
    foreach comp [$_owner xml children -type new $path] {
        set name [$_owner xml get $path.$comp]
        set fname [file join $fdir examples $name]

        if {[file exists $fname]} {
            set newfile $fname
            if {[catch {set obj [Rappture::library $fname]} result]} {
                puts stderr "WARNING: can't load example file \"$fname\""
                puts stderr "  $result"
            } else {
                $itk_component(combo) choices insert end $obj "New"
                # translate default file name => default label
                if {[string equal $defval [file tail $fname]]} {
                    $_owner xml put $path.default "New"
                }
            }
            break
        } else {
            puts stderr "WARNING: missing example file \"$fname\""
        }
    }

    #
    # If this loader has an <upload> section, then create that
    # entry next.
    #
    foreach comp [$_owner xml children -type upload $path] {
        set topath [$_owner xml get $path.$comp.to]
        if {"" != $topath} {
            set _uppath $topath

            set desc [$_owner xml get $path.$comp.prompt]
            if {"" == $desc} {
                set desc "Use this form to upload data"
                set dest [$owner xml get $_uppath.about.label]
                if {"" != $dest} {
                    append desc " into the $dest area"
                }
                append desc "."
            }
            set _updesc $desc

            $itk_component(combo) choices insert end @upload "Upload..."
            break
        }
    }

    #
    # If this loader has a <download> section, then create that
    # entry next.
    #
    foreach comp [$_owner xml children -type download $path] {
        set frompath [$_owner xml get $path.$comp.from]
        if {"" != $frompath} {
            set _dnpath $frompath
            $itk_component(combo) choices insert end @download "Download..."
            break
        }
    }

    if {[$itk_component(combo) choices size] > 0} {
        $itk_component(combo) choices insert end "---" "---"
    }

    #
    # Scan through and extract example objects, and load them into
    # the combobox.
    #
    set flist ""
    foreach comp [$_owner xml children -type example $path] {
        lappend flist [$_owner xml get $path.$comp]
    }

    # if there are no examples, then look for *.xml
    if {[llength $flist] == 0} {
        set flist *.xml
    }

    catch {unset entries}
    set _counter 0
    foreach ftail $flist {
        set fpath [file join $fdir examples $ftail]

        foreach fname [glob -nocomplain $fpath] {
            if {[string equal $fname $newfile]} {
                continue
            }
            if {[file exists $fname]} {
                if {[catch {set obj [Rappture::library $fname]} result]} {
                    puts stderr "WARNING: can't load example file \"$fname\""
                    puts stderr "  $result"
                } else {
                    set label [$obj get about.label]
                    if {$label == ""} {
                        set label "Example #[incr _counter]"
                    }

                    # if this is new, add it
                    if {![info exists entries($label)]} {
                        set entries($label) $obj
                    }

                    # translate default file name => default label
                    if {[string equal $defval [file tail $fname]]} {
                        $_owner xml put $path.default $label
                    }
                }
            } else {
                puts stderr "WARNING: missing example file \"$fname\""
            }
        }
    }
    foreach label [lsort -dictionary [array names entries]] {
        $itk_component(combo) choices insert end $entries($label) $label
    }

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [$_owner xml get $path.default]
    if {$str != ""} { after 1000 [itcl::code $this value $str] }
}

# ----------------------------------------------------------------------
# USAGE: value ?-check? ?<newval>?
#
# Clients use this to query/set the value for this widget.  With
# no args, it returns the current value for the widget.  If the
# <newval> is specified, it sets the value of the widget and
# sends a <<Value>> event.  If the -check flag is included, the
# new value is not actually applied, but just checked for correctness.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        if {$onlycheck} {
            # someday we may add validation...
            return
        }
        set newval [lindex $args 0]
        $itk_component(combo) value $newval
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    return [$itk_component(combo) value]
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::label {} {
    set label [$_owner xml get $_path.about.label]
    if {"" == $label} {
        set label "Example"
    }
    return $label
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::tooltip {} {
    # query tooltip on-demand based on current choice
    return "@[itcl::code $this _tooltip]"
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the combobox changes.
# Tries to load the selected example into the tool's data structure.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_newValue {} {
    set newval [$itk_component(combo) value]
    set obj [$itk_component(combo) translate $newval]
    if {$obj == "@upload"} {
        if {[Rappture::filexfer::enabled]} {
            set status [catch {Rappture::filexfer::upload \
                $_updesc [itcl::code $this _uploadValue]} result]
            if {$status == 0} {
                Rappture::Tooltip::cue $itk_component(combo) \
                    "Upload starting...\nA web browser page should pop up on your desktop.  Use that form to handle the upload operation."
            } else {
                if {$result == "no clients"} {
                    Rappture::Tooltip::cue $itk_component(combo) \
                        "Can't upload files.  Looks like you might be having trouble with the version of Java installed for your browser."
                } else {
                    bgerror $result
                }
            }
        } else {
            Rappture::Tooltip::cue $itk_component(combo) \
                "Can't upload data.  Upload is not enabled.  Is your SESSION variable set?  Is there an error in your session resources file?"
        }

        # put the combobox back to its last value
        $itk_component(combo) component entry configure -state normal
        $itk_component(combo) component entry delete 0 end
        $itk_component(combo) component entry insert end $_lastlabel
        $itk_component(combo) component entry configure -state disabled

    } elseif {$obj == "@download"} {
        if {[Rappture::filexfer::enabled]} {
            set info [$itk_option(-tool) valuefor $_dnpath]
            set status [catch {Rappture::filexfer::spool $info input.txt} result]
            if {$status != 0} {
                if {$result == "no clients"} {
                    Rappture::Tooltip::cue $itk_component(combo) \
                        "Can't download data.  Looks like you might be having trouble with the version of Java installed for your browser."
                } else {
                    bgerror $result
                }
            }
        } else {
            Rappture::Tooltip::cue $itk_component(combo) \
                "Can't download data.  Download is not enabled.  Is your SESSION variable set?  Is there an error in your session resources file?"
        }

        # put the combobox back to its last value
        $itk_component(combo) component entry configure -state normal
        $itk_component(combo) component entry delete 0 end
        $itk_component(combo) component entry insert end $_lastlabel
        $itk_component(combo) component entry configure -state disabled

    } elseif {$obj == "---"} {
        # put the combobox back to its last value
        $itk_component(combo) component entry configure -state normal
        $itk_component(combo) component entry delete 0 end
        $itk_component(combo) component entry insert end $_lastlabel
        $itk_component(combo) component entry configure -state disabled
    } elseif {$obj != "" && $itk_option(-tool) != ""} {
        $itk_option(-tool) load $obj
        set _lastlabel $newval
    }

    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _tooltip
#
# Returns the tooltip for this widget, given the current choice in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_tooltip {} {
    set str [string trim [$_owner xml get $_path.about.description]]

    # get the description for the current choice, if there is one
    set newval [$itk_component(combo) value]
    set obj [$itk_component(combo) translate $newval]
    if {$obj != ""} {
        if {$obj == "@upload"} {
            append str "\n\nUse this option to upload data from your desktop."
        } else {
            set label [$obj get about.label]
            if {[string length $label] > 0} {
                append str "\n\n$label"
            }

            set desc [$obj get about.description]
            if {[string length $desc] > 0} {
                if {[string length $label] > 0} {
                    append str ":\n"
                } else {
                    append str "\n\n"
                }
                append str $desc
            }
        }
    }
    return [string trim $str]
}

# ----------------------------------------------------------------------
# USAGE: _uploadValue
#
# Invoked automatically whenever the user has uploaded data from
# the "Upload..." option.  Takes the data value (passed as an
# argument) and loads into the destination widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_uploadValue {string} {
    Rappture::Tooltip::cue hide  ;# take down the note about the popup window
    #
    # BE CAREFUL:  This string may have binary characters that
    #   aren't appropriate for a string editor.  Right now, XML
    #   will barf on these characters.  Clip them out and be
    #   done with it.
    #
    regsub -all {[\000-\010\013\014\016-\037\177-\377]} $string {} string
    regsub -all "\r" $string "\n" string
    $itk_option(-tool) valuefor $_uppath $string

    $itk_component(combo) component entry configure -state normal
    $itk_component(combo) component entry delete 0 end
    $itk_component(combo) component entry insert end "Uploaded data"
    $itk_component(combo) component entry configure -state disabled
    set _lastlabel "Uploaded data"
}

# ----------------------------------------------------------------------
# OPTION: -tool
# ----------------------------------------------------------------------
itcl::configbody Rappture::Loader::tool {
    if {[catch {$itk_option(-tool) isa Rappture::Tool} valid] || !$valid} {
        error "object \"$itk_option(-tool)\" is not a Rappture Tool"
    }
}
