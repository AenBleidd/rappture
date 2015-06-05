# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: FileChoiceEntry - widget for entering a choice of strings
#
#  This widget represents a <choice> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::FileChoiceEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    private variable _rebuildPending 0

    constructor {owner path args} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method value {args}

    public method label {}
    public method tooltip {}

    protected method Rebuild {}
    protected method NewValue {}
    protected method Tooltip {}
    protected method WhenIdle {}
    private method DoGlob { cwd patterns }
    private method Glob { pattern }

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
    private variable _str2val     ;# maps option label => option value
}

itk::usual FileChoiceEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add choice {
        Rappture::Combobox $itk_interior.choice -editable no
    }
    pack $itk_component(choice) -expand yes -fill both
    bind $itk_component(choice) <<Value>> [itcl::code $this NewValue]

    # First time, parse the <pattern> elements to generate notify callbacks
    # for each template found.
    foreach cname [$_owner xml children -type pattern $_path] {
        set glob [string trim [$_owner xml get $_path.$cname]]
        # Successively replace each template with its value.
        while { [regexp -indices {@@[^@]*@@} $glob range] } {
            foreach {first last} $range break
            set i1 [expr $first + 2]
            set i2 [expr $last  - 2]
            set cpath [string range $glob $i1 $i2]
            set value [$_owner xml get $cpath.$cname]
            set glob [string replace $glob $first $last $value]
            $_owner notify add $this $cpath [itcl::code $this WhenIdle]
        }
    }
    $_owner notify sync
    eval itk_initialize $args
    Rebuild
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::destructor {} {
    $_owner notify remove $this
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
itcl::body Rappture::FileChoiceEntry::value {args} {
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
        if {[info exists _str2val($newval)]} {
            # this is a label -- use it directly
            $itk_component(choice) value $newval
            set newval $_str2val($newval)  ;# report the actual value
        } else {
            # this is a value -- search for corresponding label
            foreach str [array names _str2val] {
                if {$_str2val($str) == $newval} {
                    $itk_component(choice) value $str
                    break
                }
            }
        }
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set str [$itk_component(choice) value]
    if {[info exists _str2val($str)]} {
        return $_str2val($str)
    }
    return $str
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::label {} {
    set label [$_owner xml get $_path.about.label]
    if {"" == $label} {
        set label "Choice"
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
itcl::body Rappture::FileChoiceEntry::tooltip {} {
    # query tooltip on-demand based on current choice
    return "@[itcl::code $this Tooltip]"
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Used internally to rebuild the contents of this choice widget
# whenever something that it depends on changes.  Scans through the
# information in the XML spec and builds a list of choices for the
# widget.
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::Rebuild {} {
    set _rebuildPending 0
    # get rid of any existing choices
    $itk_component(choice) choices delete 0 end
    array unset _str2val
    #
    # Plug in the various options for the choice.
    #
    set max 10
    $_owner notify sync
    set allfiles {}
    foreach cname [$_owner xml children -type pattern $_path] {
        set glob [string trim [$_owner xml get $_path.$cname]]
        # Successively replace each template with its value.
        while { [regexp -indices {@@[^@]*@@} $glob range] } {
            foreach {first last} $range break
            set i1 [expr $first + 2]
            set i2 [expr $last  - 2]
            set cpath [string range $glob $i1 $i2]
            set value [$_owner xml get $cpath.current]
            if { $value == "" } {
                set value [$_owner xml get $cpath.default]
            }
            set glob [string replace $glob $first $last $value]
        }
        # Replace the template with the substituted value.
        set files [Glob $glob]
        set allfiles [concat $allfiles $files]
    }
    set first ""
    set tail ""
    foreach file $allfiles {
        set tail [file tail $file]
        if { $first == "" } {
            set first $tail
        }
        set root [file root $tail]
        $itk_component(choice) choices insert end $file $tail
        set _str2val($tail) $file
        set len [string length $tail]
        if {$len > $max} { set max $len }
    }
    $itk_component(choice) configure -width $max
    $itk_component(choice) value $tail
}

# ----------------------------------------------------------------------
# USAGE: NewValue
#
# Invoked automatically whenever the value in the choice changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::NewValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: Tooltip
#
# Returns the tooltip for this widget, given the current choice in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::FileChoiceEntry::Tooltip {} {
    set tip [string trim [$_owner xml get $_path.about.description]]
    # get the description for the current choice, if there is one
    set str [$itk_component(choice) value]
    set path [$itk_component(choice) translate $str]
    set desc $path
    if {$path == ""} {
        set desc [$_owner xml get $_path.about.description]
    }

    if {[string length $str] > 0 && [string length $desc] > 0} {
        append tip "\n\n$str:\n$desc"
    }
    return $tip
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::FileChoiceEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(choice) configure -state $itk_option(-state)
}

itcl::body Rappture::FileChoiceEntry::WhenIdle {} {
    if { !$_rebuildPending } {
        after 10 [itcl::code $this Rebuild]
        set _rebuildPending 1
    }
}

itcl::body Rappture::FileChoiceEntry::DoGlob { cwd patterns } {
    set rest [lrange $patterns 1 end]
    set pattern [file join $cwd [lindex $patterns 0]]
    set files ""
    if { [llength $rest] > 0 } {
        if { [catch {
            glob -nocomplain -type d $pattern
        } dirs] != 0 } {
            puts stderr "can't glob \"$pattern\": $dirs"
            return
        }
        foreach d $dirs {
            set files [concat $files [DoGlob $d $rest]]
        }
    } else {
        if { [catch {
            glob -nocomplain $pattern
        } files] != 0 } {
            puts stderr "can't glob \"$pattern\": $files"
            return
        }
    }
    return [lsort -dictionary $files]
}

#
# Glob --
#
#       Matches a single pattern for files. This differs from the
#       Tcl glob by
#
#       1. Only matches files, not directories.
#       2. Doesn't stop on errors (e.g. unreadable directories).
#
itcl::body Rappture::FileChoiceEntry::Glob { pattern } {
    return [DoGlob "" [file split $pattern]]
}

