# ----------------------------------------------------------------------
#  COMPONENT: controls - a container for various Rappture controls
#
#  This widget is a smart frame acting as a container for controls.
#  Controls are added to this panel, and the panel itself decides
#  how to arrange them given available space.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *Controls.padding 4 widgetDefault
option add *Controls.labelFont \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Controls {
    inherit itk::Widget

    itk_option define -padding padding Padding 0

    constructor {owner args} { # defined below }

    public method insert {pos path}
    public method delete {first {last ""}}
    public method index {name}
    public method control {args}

    protected method _layout {}
    protected method _controlChanged {path}
    protected method _formatLabel {str}
    protected method _changeTabs {}

    private variable _owner ""       ;# controls belong to this owner
    private variable _tabs ""        ;# optional tabset for groups
    private variable _frame ""       ;# pack controls into this frame
    private variable _counter 0      ;# counter for control names
    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _controls ""    ;# list of known controls
    private variable _name2info      ;# maps control name => info
}
                                                                                
itk::usual Controls {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::constructor {owner args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this _layout]; list"

    set _owner $owner

    Rappture::Scroller $itk_interior.sc -xscrollmode none -yscrollmode auto
    pack $itk_interior.sc -expand yes -fill both
    set f [$itk_interior.sc contents frame]

    set _tabs [blt::tabset $f.tabs -borderwidth 0 -relief flat \
        -side top -tearoff 0 -highlightthickness 0 \
        -selectbackground $itk_option(-background) \
        -selectcommand [itcl::code $this _changeTabs]]

    set _frame [frame $f.inner]
    pack $_frame -expand yes -fill both

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> <path>
#
# Clients use this to insert a control into this panel.  The control
# is inserted into the list at position <pos>, which can be an integer
# starting from 0 or the keyword "end".  Information about the control
# is taken from the specified <path>.
#
# Returns a name that can be used to identify the control in other
# methods.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::insert {pos path} {
    if {"end" == $pos} {
        set pos [llength $_controls]
    } elseif {![string is integer $pos]} {
        error "bad index \"$pos\": should be integer or \"end\""
    }

    incr _counter
    set name "control$_counter"

    set _name2info($name-path) $path
    set _name2info($name-label) ""
    set _name2info($name-value) [set w $_frame.v$name]

    set type [$_owner xml element -as type $path]
    switch -- $type {
        choice {
            Rappture::ChoiceEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        group {
            Rappture::GroupEntry $w $_owner $path
        }
        loader {
            Rappture::Loader $w $_owner $path -tool [$_owner tool]
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        number {
            Rappture::NumberEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        integer {
            Rappture::IntegerEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        boolean {
            Rappture::BooleanEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        string {
            Rappture::TextEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $path]
        }
        control {
            set label [$_owner xml get $path.label]
            if {"" == $label} { set label "Simulate" }
            set service [$_owner xml get $path.service]
            button $w -text $label -command [list $service run]
        }
        separator {
            # no widget to create
            set _name2info($name-value) "--"
        }
        default {
            error "don't know how to add control type \"$type\""
        }
    }

    if {$type != "control" && $type != "separator"} {
        $_owner widgetfor $path $w

        # make a label for this control
        set label [$w label]
        if {"" != $label} {
            set _name2info($name-label) $_frame.l$name
            set font [option get $itk_component(hull) labelFont Font]
            label $_name2info($name-label) -text [_formatLabel $label] \
                -font $font
        }

        # register the tooltip for this control
        set tip [$w tooltip]
        if {"" != $tip} {
            Rappture::Tooltip::for $w $tip

            # add the tooltip to the label too, if there is one
            if {$_name2info($name-label) != ""} {
                Rappture::Tooltip::for $_name2info($name-label) $tip
            }
        }
    }

    # insert the new control onto the known list
    set _controls [linsert $_controls $pos $name]

    # now that we have a new control, we should fix the layout
    $_dispatcher event -idle !layout

    return $name
}

# ----------------------------------------------------------------------
# USAGE: delete <first> ?<last>?
#
# Clients use this to delete one or more controls from this widget.
# The <first> and <last> represent the integer index of the desired
# control.  You can use the "index" method to convert a control name to
# its integer index.  If only <first> is specified, then that one
# control is deleted.  If <last> is specified, then all controls in the
# range <first> to <last> are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::delete {first {last ""}} {
    if {$last == ""} {
        set last $first
    }
    if {![regexp {^[0-9]+|end$} $first]} {
        error "bad index \"$first\": should be integer or \"end\""
    }
    if {![regexp {^[0-9]+|end$} $last]} {
        error "bad index \"$last\": should be integer or \"end\""
    }

    foreach name [lrange $_controls $first $last] {
        if {"" != $_name2info($name-label)} {
            destroy $_name2info($name-label)
        }
        if {"" != $_name2info($name-value)} {
            destroy $_name2info($name-value)
        }
        $_owner widgetfor $_name2info($name-path) ""

        unset _name2info($name-path)
        unset _name2info($name-label)
        unset _name2info($name-value)
    }
    set _controls [lreplace $_controls $first $last]

    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: index <name>|@n
#
# Clients use this to convert a control <name> into its corresponding
# integer index.  Returns an error if the <name> is not recognized.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::index {name} {
    set i [lsearch $_controls $name]
    if {$i >= 0} {
        return $i
    }
    if {[regexp {^@([0-9]+)$} $name match i]} {
        return $i
    }
    error "bad control name \"$name\": should be @int or one of [join [lsort $_controls] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: control ?-label|-value|-path? ?<name>|@n?
#
# Clients use this to get information about controls.  With no args, it
# returns a list of all control names.  Otherwise, it returns the frame
# associated with a control name.  The -label option requests the label
# widget instead of the value widget.  The -path option requests the
# path within the XML that the control affects.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::control {args} {
    if {[llength $args] == 0} {
        return $_controls
    }
    Rappture::getopts args params {
        flag switch -value default
        flag switch -label
        flag switch -path
    }
    if {[llength $args] == 0} {
        error "missing control name"
    }
    set i [index [lindex $args 0]]
    set name [lindex $_controls $i]

    set opt $params(switch)
    return $_name2info($name$opt)
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Used internally to fix the layout of controls whenever controls
# are added or deleted, or when the control arrangement changes.
# There are a lot of heuristics here trying to achieve a "good"
# arrangement of controls.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_layout {} {
    #
    # Clear any existing layout
    #
    foreach name $_controls {
        foreach elem {label value} {
            set w $_name2info($name-$elem)
            if {$w != "" && [winfo exists $w]} {
                grid forget $w
            }
        }
    }
    if {[$_tabs size] > 0} {
        $_tabs delete 0 end
    }

    #
    # Decide on a layout scheme:
    #   tabs ...... best if all elements within are groups
    #   hlabels ... horizontal labels (label: value)
    #
    if {[llength $_controls] >= 2} {
        # assume tabs for multiple groups
        set scheme tabs
        foreach name $_controls {
            set w $_name2info($name-value)

            if {[winfo class $w] != "GroupEntry"} {
                # something other than a group? then fall back on hlabels
                set scheme hlabels
                break
            }
        }
    } else {
        set scheme hlabels
    }

    switch -- $scheme {
      tabs {
        #
        # SCHEME: tabs
        # put a series of groups into a tabbed notebook
        #

        # use inner frame within tabs to show current group
        pack $_tabs -before $_frame -fill x

        set gn 1
        foreach name $_controls {
            set wv $_name2info($name-value)
            $wv configure -heading no

            set label [$wv component heading cget -text]
            if {"" == $label} {
                set label "Group #$gn"
            }
            set _name2info($name-label) $label

            $_tabs insert end $label \
                -activebackground $itk_option(-background)

            incr gn
        }

        # compute the overall size
        # BE CAREFUL: do this after setting "-heading no" above
        set maxw 0
        set maxh 0
        update idletasks
        foreach name $_controls {
            set wv $_name2info($name-value)
            set w [winfo reqwidth $wv]
            if {$w > $maxw} { set maxw $w }
            set h [winfo reqheight $wv]
            if {$h > $maxh} { set maxh $h }
        }
        $_frame configure -width $maxw -height $maxh

        grid propagate $_frame off
        grid columnconfigure $_frame 0 -weight 1
        grid rowconfigure $_frame 0 -weight 1

        $_tabs select 0; _changeTabs
      }

      hlabels {
        #
        # SCHEME: hlabels
        # simple "Label: Value" layout
        #
        pack forget $_tabs
        grid propagate $_frame on
        grid columnconfigure $_frame 0 -weight 0
        grid rowconfigure $_frame 0 -weight 0

        set row 0
        foreach name $_controls {
            set wl $_name2info($name-label)
            if {$wl != "" && [winfo exists $wl]} {
                grid $wl -row $row -column 0 -sticky e
            }

            set wv $_name2info($name-value)
            if {$wv != "" && [winfo exists $wv]} {
                if {$wl != ""} {
                    grid $wv -row $row -column 1 -sticky ew
                } else {
                    grid $wv -row $row -column 0 -columnspan 2 -sticky ew
                }

                set frame [winfo parent $wv]
                grid rowconfigure $frame $row -weight 0
                grid rowconfigure $frame $row -weight 0

                switch -- [winfo class $wv] {
                    TextEntry {
                        if {[regexp {[0-9]+x[0-9]+} [$wv size]]} {
                            grid $wl -sticky n -pady 4
                            grid $wv -sticky nsew
                            grid rowconfigure $frame $row -weight 1
                            grid columnconfigure $frame 1 -weight 1
                        }
                    }
                    GroupEntry {
                        $wv configure -heading yes
                    }
                }
                grid columnconfigure $frame 1 -weight 1
            } elseif {$wv == "--"} {
                grid rowconfigure $frame $row -minsize 10
            }

            incr row
            grid rowconfigure [winfo parent $w] $row \
                -minsize $itk_option(-padding)
            incr row
        }
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: _controlChanged <path>
#
# Invoked automatically whenever the value for the control with the
# XML <path> changes.  Sends a notification along to the tool
# controlling this panel.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_controlChanged {path} {
    if {"" != $_owner} {
        $_owner changed $path
    }
}

# ----------------------------------------------------------------------
# USAGE: _formatLabel <string>
#
# Used internally to format a label <string>.  Trims any excess
# white space and adds a ":" to the end.  That way, all labels
# have a uniform look.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_formatLabel {str} {
    set str [string trim $str]
    if {"" != $str && [string index $str end] != ":"} {
        append str ":"
    }
    return $str
}

# ----------------------------------------------------------------------
# USAGE: _changeTabs
#
# Used internally to change tabs when the user clicks on a tab
# in the "tabs" layout mode.  This mode is used when the widget
# contains nothing but groups, as a compact way of representing
# the groups.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_changeTabs {} {
    set i [$_tabs index select]
    set name [lindex $_controls $i]
    if {"" != $name} {
        foreach w [grid slaves $_frame] {
            grid forget $w
        }

        set wv $_name2info($name-value)
        grid $wv -row 0 -column 0 -sticky new
    }
}

# ----------------------------------------------------------------------
# OPTION: -padding
# ----------------------------------------------------------------------
itcl::configbody Rappture::Controls::padding {
    $_dispatcher event -idle !layout
}
