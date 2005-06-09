# ----------------------------------------------------------------------
#  COMPONENT: loader - widget for loading examples and old runs
#
#  This widget is a glorified combobox that is used to load various
#  example files into the application.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *Loader.textForeground black widgetDefault
option add *Loader.textBackground white widgetDefault

itcl::class Rappture::Loader {
    inherit itk::Widget

    itk_option define -tool tool Tool ""

    constructor {xmlobj path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _newValue {}
    protected method _tooltip {}

    private variable _xmlobj ""   ;# XML containing description
    private variable _path ""     ;# path in XML to this loader
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
itcl::body Rappture::Loader::constructor {xmlobj path args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
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

    #
    # Scan through and extract example objects, and load them into
    # the combobox.
    #
    set defval [$xmlobj get $path.default]

    set flist ""
    foreach comp [$xmlobj children -type example $path] {
        lappend flist [$xmlobj get $path.$comp]
    }

    # if there are no examples, then look for *.xml
    if {[llength $flist] == 0} {
        set flist *.xml
    }

    if {$itk_option(-tool) != ""} {
        set fdir [$itk_option(-tool) installdir]
    } else {
        set fdir "."
    }

    set _counter 1
    foreach ftail $flist {
        set fpath [file join $fdir examples $ftail]
        foreach fname [lsort [glob -nocomplain $fpath]] {
            if {[file exists $fname]} {
                if {[catch {set obj [Rappture::library $fname]} result]} {
                    puts stderr "WARNING: can't load example file \"$fname\""
                    puts stderr "  $result"
                } else {
                    set label [$obj get about.label]
                    if {$label == ""} {
                        set label "Example #$_counter"
                    }
                    $itk_component(combo) choices insert end $obj $label

                    if {[string equal $defval [file tail $fname]]} {
                        $xmlobj put $path.default $label
                    }
                }
            } else {
                puts stderr "WARNING: missing example file \"$fname\""
            }
        }
    }

    #
    # Assign the default value to this widget, if there is one.
    #
    set str [$xmlobj get $path.default]
    if {$str != ""} { after 500 [itcl::code $this value $str] }
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
    set label [$_xmlobj get $_path.about.label]
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
    if {$obj != "" && $itk_option(-tool) != ""} {
        $itk_option(-tool) load $obj
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
    set str [string trim [$_xmlobj get $_path.about.description]]

    # get the description for the current choice, if there is one
    set newval [$itk_component(combo) value]
    set obj [$itk_component(combo) translate $newval]
    if {$obj != ""} {
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
    return [string trim $str]
}

# ----------------------------------------------------------------------
# OPTION: -tool
# ----------------------------------------------------------------------
itcl::configbody Rappture::Loader::tool {
    if {[catch {$itk_option(-tool) isa Rappture::Tool} valid] || !$valid} {
        error "object \"$itk_option(-tool)\" is not a Rappture Tool"
    }
}
