# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ChoiceEntry - widget for entering a choice of strings
#
#  This widget represents a <choice> entry on a control panel.
#  It is used to choose one of several mutually-exclusive strings.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::ChoiceEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _rebuild {}
    protected method _newValue {}
    protected method _tooltip {}
    protected method _log {}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
    private variable _str2val     ;# maps option label => option value
}

itk::usual ChoiceEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::constructor {owner path args} {
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
        Rappture::Combobox $itk_interior.choice -editable no \
            -interactcommand [itcl::code $this _log]
    }
    pack $itk_component(choice) -expand yes -fill both
    bind $itk_component(choice) <<Value>> [itcl::code $this _newValue]

    eval itk_initialize $args

    _rebuild
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::destructor {} {
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
itcl::body Rappture::ChoiceEntry::value {args} {
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
                if {$_str2val($str) eq $newval} {
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
itcl::body Rappture::ChoiceEntry::label {} {
    set label [string trim [$_owner xml get $_path.about.label]]
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
itcl::body Rappture::ChoiceEntry::tooltip {} {
    # query tooltip on-demand based on current choice
    return "@[itcl::code $this _tooltip]"
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Used internally to rebuild the contents of this choice widget
# whenever something that it depends on changes.  Scans through the
# information in the XML spec and builds a list of choices for the
# widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_rebuild {} {
    # get rid of any existing choices
    $itk_component(choice) choices delete 0 end
    catch {unset _str2val}

    #
    # Plug in the various options for the choice.
    #
    set max 10
    foreach cname [$_owner xml children -type option $_path] {
        set path [string trim [$_owner xml get $_path.$cname.path]]
        if {"" != $path} {
            # look for the input element controlling this path
            set found 0
            foreach cntl [Rappture::entities [$_owner xml object] "input"] {
                set len [string length $cntl]
                if {[string equal -length $len $cntl $path]} {
                    set found 1
                    break
                }
            }
            if {$found} {
                #
                # Choice comes from a list of matching entities at
                # a particular XML path.  Use the <label> as a template
                # for each item on the path.
                #
                $_owner notify add $this $cntl [itcl::code $this _rebuild]

                set label \
                    [string trim [$_owner xml get $_path.$cname.about.label]]
                if {"" == $label} {
                    set label "%type #%n"
                }

                set ppath [Rappture::LibraryObj::path2list $path]
                set leading [join [lrange $ppath 0 end-1] .]
                set tail [lindex $ppath end]
                set n 1
                foreach ccname [$_owner xml children $leading] {
                    if {[string match $tail $ccname]} {
                        set subst(%n) $n
                        set subst(%type) [$_owner xml element -as type $leading.$ccname]
                        set subst(%id) [$_owner xml element -as id $leading.$ccname]
                        foreach detail [$_owner xml children $leading.$ccname] {
                            set subst(%$detail) \
                                [$_owner xml get $leading.$ccname.$detail]
                        }
                        set str [string map [array get subst] $label]
                        $itk_component(choice) choices insert end \
                            $leading.$ccname $str
                        incr n
                    }
                }
                $itk_component(choice) value ""
            } else {
                puts "can't find controlling entity for path \"$path\""
            }
        } else {
            #
            # Choice is an ordinary LABEL.
            # Add the label as-is into the list of choices.
            #
            set val [string trim [$_owner xml get $_path.$cname.value]]
            set str [string trim [$_owner xml get $_path.$cname.about.label]]
            if {"" == $val} {
                set val $str
            }
            if {"" != $str} {
                set _str2val($str) $val
                $itk_component(choice) choices insert end $_path.$cname $str
                set len [string length $str]
                if {$len > $max} { set max $len }
            }
        }
    }
    $itk_component(choice) configure -width $max

    #
    # Assign the default value to this widget, if there is one.
    #
    set defval [string trim [$_owner xml get $_path.default]]
    if {"" != $defval} {
        if {[info exists _str2val($defval)]} {
            $itk_component(choice) value $defval
        } else {
            foreach str [array names _str2val] {
                if {$_str2val($str) == $defval} {
                    $itk_component(choice) value $str
                    break
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the choice changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _tooltip
#
# Returns the tooltip for this widget, given the current choice in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_tooltip {} {
    set tip [string trim [$_owner xml get $_path.about.description]]

    # get the description for the current choice, if there is one
    set str [$itk_component(choice) value]
    set path [$itk_component(choice) translate $str]
    set desc ""
    if {$path != ""} {
        set desc [string trim [$_owner xml get $path.about.description]]
    }

    if {[string length $str] > 0 && [string length $desc] > 0} {
        append tip "\n\n$str:\n$desc"
    }
    return $tip
}

# ----------------------------------------------------------------------
# USAGE: _log
#
# Used internally to send info to the logging mechanism.  Calls the
# Rappture::Logger mechanism to log the change to this input.
# ----------------------------------------------------------------------
itcl::body Rappture::ChoiceEntry::_log {} {
    Rappture::Logger::log input $_path [$itk_component(choice) value]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::ChoiceEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    $itk_component(choice) configure -state $itk_option(-state)
}
