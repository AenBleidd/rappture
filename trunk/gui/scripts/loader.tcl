# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: loader - widget for loading examples and old runs
#
#  This widget is a glorified combobox that is used to load various
#  example files into the application.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _uploadValue {path args}
    protected method _downloadValues {}
    protected method _tooltip {}
    protected method _log {}

    private method SetDefaultValue { value }
    private method EventuallySetDefaultValue { value }

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this loader
    private variable _lastlabel "";# label of last example loaded

    private variable _uppath ""   ;# list: path label desc ...
    private variable _dnpaths ""  ;# list of download element paths
    private common _dnpath2state  ;# maps download path => yes/no state
    private variable _copyfrom "" ;# copy xml objects from here in example lib
    private variable _copyto ""   ;# copy xml objects here in example lib
    private variable _label2file  ;# maps combobox labels to filenames
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
        Rappture::Combobox $itk_interior.combo -editable no \
            -interactcommand [itcl::code $this _log]
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
    set defval [string trim [$_owner xml get $path.default]]

    #
    # If this loader has a <new> section, then create that
    # entry first.
    #
    set newfile ""
    foreach comp [$_owner xml children -type new $path] {
        set name  [string trim [$_owner xml get $path.$comp]]
        set fname [file join $fdir examples $name]

        if {[file exists $fname]} {
            set newfile $fname

            if {[catch {set obj [Rappture::library $fname]} result]} {
                puts stderr "WARNING: can't load example file \"$fname\""
                puts stderr "  $result"
            } else {
                set label "New"
                $itk_component(combo) choices insert end $obj $label

                # if this is new, add it to the label->file hash
                if {![info exists entries($label)]} {
                    set entries($label) $obj
                    set _label2file($label) [file tail $fname]
                }

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
        foreach tcomp [$_owner xml children -type to $path.$comp] {
            set topath [string trim [$_owner xml get $path.$comp.$tcomp]]
            if { [$_owner xml element -as id $topath] == "" } {
                puts stderr \
                    "unknown <upload> path \"$topath\": please fix tool.xml"
                continue
            }
            set label [string trim [$_owner xml get $topath.about.label]]
            set desc  [string trim [$_owner xml get $topath.about.description]]
            lappend _uppath $topath $label $desc
        }
        break
    }
    if {[llength $_uppath] > 0} {
        $itk_component(combo) choices insert end \
            @upload [Rappture::filexfer::label upload]
    }

    #
    # If this loader has a <download> section, then create that
    # entry next.  Build a popup for choices if there is more than
    # one download element.
    #
    Rappture::Balloon $itk_component(hull).download \
        -title "Choose what to [string tolower [Rappture::filexfer::label downloadWord]]:"
    set inner [$itk_component(hull).download component inner]

    set i 0
    foreach comp [$_owner xml children -type download $path] {
        foreach dcomp [$_owner xml children -type from $path.$comp] {
            set frompath [string trim [$_owner xml get $path.$comp.$dcomp]]
            if {"" != $frompath} {
                lappend _dnpaths $frompath
                set _dnpath2state($this-$frompath) [expr {$i == 0}]

                set label [string trim [$_owner xml get $frompath.about.label]]
                checkbutton $inner.cb$i -text $label \
                    -variable ::Rappture::Loader::_dnpath2state($this-$frompath)
                pack $inner.cb$i -anchor w
                incr i
            }
        }
    }
    button $inner.go -text [Rappture::filexfer::label download] \
        -command [itcl::code $this _downloadValues]
    pack $inner.go -side bottom -padx 50 -pady {4 2}

    if {[llength $_dnpaths] > 0} {
        $itk_component(combo) choices insert end \
            @download [Rappture::filexfer::label download]
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
        lappend flist [string trim [$_owner xml get $path.$comp]]
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

                    # if this is new, add it to the label->file hash
                    if {![info exists entries($label)]} {
                        set entries($label) $obj
                        set _label2file($label) [file tail $fname]
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

    set _copyfrom [string trim [$_owner xml get $path.copy.from]]
    set _copyto   [string trim [$_owner xml get $path.copy.to]]

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [string trim [$_owner xml get $path.default]]
    if { $str != "" } {
        EventuallySetDefaultValue $str
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::destructor {} {
    # be sure to clean up entries for this widget's download paths
    foreach path $_dnpaths {
        catch {unset _dnpath2state($this-$path)}
    }
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
    set label [string trim [$_owner xml get $_path.about.label]]
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

itcl::body Rappture::Loader::SetDefaultValue { value } {
    $itk_component(combo) value $value
    _newValue
}
#
#
# EventuallySetDefaultValue --
#
#   Sets the designated default value for the loader.  This must be done
#   after the entire application is assembled, otherwise the default values
#   set up by the loader will be overwritten by the various widgets
#   themselves when they try to set their default values.
#
itcl::body Rappture::Loader::EventuallySetDefaultValue { value } {
    after 100 [itcl::code $this SetDefaultValue $value]
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
        set tool [Rappture::Tool::resources -appname]
        Rappture::filexfer::upload \
            $tool $_uppath [itcl::code $this _uploadValue]

        # put the combobox back to its last value
        $itk_component(combo) component entry configure -state normal
        $itk_component(combo) component entry delete 0 end
        $itk_component(combo) component entry insert end $_lastlabel
        $itk_component(combo) component entry configure -state disabled

    } elseif {$obj == "@download"} {
        if {[llength $_dnpaths] == 1} {
            _downloadValues
        } else {
            $itk_component(hull).download activate $itk_component(combo) below
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
        if {("" != $_copyfrom) && ("" != $_copyto)} {
            $obj copy $_copyto from $_copyfrom
        }
        $_owner xml put $_path.file $_label2file($newval)
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
# USAGE: _log
#
# Used internally to send info to the logging mechanism.  Calls the
# Rappture::Logger mechanism to log the change to this input.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_log {} {
    Rappture::Logger::log input $_path [$itk_component(combo) value]
}

# ----------------------------------------------------------------------
# USAGE: _uploadValue ?<key> <value> <key> <value> ...?
#
# Invoked automatically whenever the user has uploaded data from
# the "Upload..." option.  Takes the data value (passed as an
# argument) and loads into the destination widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_uploadValue {args} {
    array set data $args

    if {[info exists data(error)]} {
        Rappture::Tooltip::cue $itk_component(combo) $data(error)
    }

    if {[info exists data(path)] && [info exists data(data)]} {
        Rappture::Tooltip::cue hide  ;# take down note about the popup window
        $itk_option(-tool) valuefor $data(path) $data(data)

        $itk_component(combo) component entry configure -state normal
        $itk_component(combo) component entry delete 0 end
        $itk_component(combo) component entry insert end "Uploaded data"
        $itk_component(combo) component entry configure -state disabled
        set _lastlabel "Uploaded data"
    }
}

# ----------------------------------------------------------------------
# USAGE: _downloadValues
#
# Used internally to download all values checked by the popup that
# controls downloading.  Sends the values for the various controls
# out to the user by popping up separate browser windows.
# ----------------------------------------------------------------------
itcl::body Rappture::Loader::_downloadValues {} {
    # take down the popup (in case it was posted)
    $itk_component(hull).download deactivate

    set mesg ""
    foreach path $_dnpaths {
        if {$_dnpath2state($this-$path)} {
            set info [$itk_option(-tool) valuefor $path]
            set mesg [Rappture::filexfer::download $info input.txt]
            if {"" != $mesg} { break }
        }
    }

    if {"" != $mesg} {
        Rappture::Tooltip::cue $itk_component(combo) $mesg
    }
}

# ----------------------------------------------------------------------
# OPTION: -tool
# ----------------------------------------------------------------------
itcl::configbody Rappture::Loader::tool {
    if {[catch {$itk_option(-tool) isa Rappture::Tool} valid] || !$valid} {
        error "object \"$itk_option(-tool)\" is not a Rappture Tool"
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::Loader::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(combo) configure -state $itk_option(-state)
}
