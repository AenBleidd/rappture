# ----------------------------------------------------------------------
#  COMPONENT: controls - a container for various Rappture controls
#
#  This widget is a smart frame acting as a container for controls.
#  Controls are added to this panel, and the panel itself decides
#  how to arrange them given available space.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Controls.padding 4 widgetDefault
option add *Controls.labelFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::Controls {
    inherit itk::Widget

    itk_option define -padding padding Padding 0

    constructor {owner args} { # defined below }
    destructor { # defined below }

    public method insert {pos path}
    public method delete {first {last ""}}
    public method index {name}
    public method control {args}
    public method refresh {}

    protected method _layout {}
    protected method _monitor {name state}
    protected method _controlChanged {name}
    protected method _controlValue {path {units ""}}
    protected method _formatLabel {str}
    protected method _changeTabs {}
    protected method _resize {}

    private variable _owner ""       ;# controls belong to this owner
    private variable _tabs ""        ;# optional tabset for groups
    private variable _frame ""       ;# pack controls into this frame
    private variable _counter 0      ;# counter for control names
    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _controls ""    ;# list of known controls
    private variable _showing ""     ;# list of enabled (showing) controls
    private variable _name2info      ;# maps control name => info
    private variable _scheme ""      ;# layout scheme (tabs/hlabels)
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
    $_dispatcher register !resize
    $_dispatcher dispatch $this !resize "[itcl::code $this _resize]; list"

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

    #
    # Put this frame in whenever the control frame is empty.
    # It forces the size to contract back now when controls are deleted.
    #
    frame $_frame.empty -width 1 -height 1

    #
    # Set up a binding that all inserted widgets will use so that
    # we can monitor their size changes.
    #
    bind Controls-$this <Configure> \
        [list $_dispatcher event -idle !resize]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::destructor {} {
    delete 0 end
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
    set path [$_owner xml element -as path $path]

    set _name2info($name-path) $path
    set _name2info($name-label) ""
    set _name2info($name-type) ""
    set _name2info($name-value) [set w $_frame.v$name]
    set _name2info($name-enable) "yes"
    set _name2info($name-disablestyle) "greyout"

    set type [$_owner xml element -as type $path]
    set _name2info($name-type) $type
    switch -- $type {
        choice {
            Rappture::ChoiceEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        drawing {
            Rappture::DrawingEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        group {
            Rappture::GroupEntry $w $_owner $path
        }
        loader {
            Rappture::Loader $w $_owner $path -tool [$_owner tool]
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        number {
            Rappture::NumberEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        integer {
            Rappture::IntegerEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        boolean {
            Rappture::BooleanEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        string {
            Rappture::TextEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        image {
            Rappture::ImageEntry $w $_owner $path
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
        note {
            Rappture::Note $w $_owner $path
        }
        periodicelement {
            Rappture::PeriodicElementEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this _controlChanged $name]
        }
        default {
            error "don't know how to add control type \"$type\""
        }
    }

    #
    # If this element has an <enable> expression, then register
    # its controlling widget here.
    #
    set notify [string trim [$_owner xml get $path.about.notify]]

    set disablestyle [string trim [$_owner xml get $path.about.disablestyle]]
    if { $disablestyle != "" } {
	set _name2info($name-disablestyle) $disablestyle
    }
    #
    # If this element has an <enable> expression, then register
    # its controlling widget here.
    #
    set enable [string trim [$_owner xml get $path.about.enable]]
    if {"" == $enable} {
        set enable yes
    }
    if {![string is boolean $enable]} {
        set re {([a-zA-Z_]+[0-9]*|\([^\(\)]+\)|[a-zA-Z_]+[0-9]*\([^\(\)]+\))(\.([a-zA-Z_]+[0-9]*|\([^\(\)]+\)|[a-zA-Z_]+[0-9]*\([^\(\)]+\)))*(:[-a-zA-Z0-9/]+)?}
        set rest $enable
        set enable ""
        set deps ""
        while {1} {
            if {[regexp -indices $re $rest match]} {
                foreach {s0 s1} $match break

                if {[string index $rest [expr {$s0-1}]] == "\""
                      && [string index $rest [expr {$s1+1}]] == "\""} {
                    # string in ""'s? then leave it alone
                    append enable [string range $rest 0 $s1]
                    set rest [string range $rest [expr {$s1+1}] end]
                } else {
                    #
                    # This is a symbol which should be substituted
                    # it can be either:
                    #   input.foo.bar
                    #   input.foo.bar:units
                    #
                    set cpath [string range $rest $s0 $s1]
                    set parts [split $cpath :]
                    set ccpath [lindex $parts 0]
                    set units [lindex $parts 1]

                    # make sure we have the standard path notation
                    set stdpath [$_owner regularize $ccpath]
                    if {"" == $stdpath} {
                        puts stderr "WARNING: don't recognize parameter $cpath in <enable> expression for $path.  This may be buried in a structure that is not yet loaded."
                        set stdpath $ccpath
                    }
                    # substitute [_controlValue ...] call in place of path
                    append enable [string range $rest 0 [expr {$s0-1}]]
                    append enable [format {[_controlValue %s %s]} $stdpath $units]
                    lappend deps $stdpath
                    set rest [string range $rest [expr {$s1+1}] end]
                }
            } else {
                append enable $rest
                break
            }
        }

        foreach cpath $deps {
            $_owner dependenciesfor $cpath $path
        }
    }
    set _name2info($name-enable) $enable

    $_owner widgetfor $path $w

    if {[lsearch {control group separator note} $type] < 0} {
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
    _monitor $name on

    # now that we have a new control, we should fix the layout
    $_dispatcher event -idle !layout
    _controlChanged $name

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
        _monitor $name off

        if {"" != $_name2info($name-label)} {
            destroy $_name2info($name-label)
        }
        if {"" != $_name2info($name-value)} {
            destroy $_name2info($name-value)
        }
        $_owner widgetfor $_name2info($name-path) ""
	array unset _name2info $name-*
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
    if {$name == "end"} {
        return [expr {[llength $_controls]-1}]
    }
    error "bad control name \"$name\": should be @int or one of [join [lsort $_controls] {, }]"
}

# ----------------------------------------------------------------------
# USAGE: control ?-label|-value|-path|-enable? ?<name>|@n?
#
# Clients use this to get information about controls.  With no args, it
# returns a list of all control names.  Otherwise, it returns the frame
# associated with a control name.  The -label option requests the label
# widget instead of the value widget.  The -path option requests the
# path within the XML that the control affects.  The -enable option
# requests the enabling condition for this control.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::control {args} {
    if {[llength $args] == 0} {
        return $_controls
    }
    Rappture::getopts args params {
        flag switch -value default
        flag switch -label
        flag switch -path
        flag switch -enable
        flag switch -disablestyle
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
# USAGE: refresh
#
# Clients use this to refresh the layout of the control panel
# whenever a widget within the panel changes visibility state.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::refresh {} {
    $_dispatcher event -idle !layout
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
    grid forget $_frame.empty

    #
    # Decide which widgets should be shown and which should be hidden.
    #
    set hidden ""
    set showing ""
    foreach name $_controls {
        set show 1
        set cond $_name2info($name-enable)
        if {[string is boolean $cond] && !$cond} {
            # hard-coded "off" -- ignore completely
        } elseif {[catch {expr $cond} show] == 0} {
            set type $_name2info($name-type)
	    set disablestyle $_name2info($name-disablestyle)
            set lwidget $_name2info($name-label)
            set vwidget $_name2info($name-value)
            if {[lsearch -exact {group image structure} $type] >= 0 || 
		$disablestyle == "hide" } {
                if {$show ne "" && $show} {
                    lappend showing $name
                } else {
                    lappend hidden $name
                }
            } else {
                # show other objects, but enable/disable them
                lappend showing $name
                if {$show ne "" && $show} {
                    if {[winfo exists $vwidget]} {
                        $vwidget configure -state normal
                    }
                    if {[winfo exists $lwidget]} {
                        $lwidget configure -foreground \
                            [lindex [$lwidget configure -foreground] 3]
                    }
                } else {
                    if {[winfo exists $vwidget]} {
                        $vwidget configure -state disabled
                    }
                    if {[winfo exists $lwidget]} {
                        $lwidget configure -foreground gray
                    }
                }
            }
        } else {
            bgerror "Error in <enable> expression for \"$_name2info($name-path)\":\n  $show"
        }
    }

    # store the showing tabs in the object so it can be used in _changeTabs
    set _showing $showing

    #
    # Decide on a layout scheme:
    #   tabs ...... best if all elements within are groups
    #   hlabels ... horizontal labels (label: value)
    #
    if {[llength $showing] >= 2} {
        # assume tabs for multiple groups
        set _scheme tabs
        foreach name $showing {
            set w $_name2info($name-value)

            if {$w == "--" || [winfo class $w] != "GroupEntry"} {
                # something other than a group? then fall back on hlabels
                set _scheme hlabels
                break
            }
        }
    } else {
        set _scheme hlabels
    }

    switch -- $_scheme {
      tabs {
        #
        # SCHEME: tabs
        # put a series of groups into a tabbed notebook
        #

        # use inner frame within tabs to show current group
        pack $_tabs -before $_frame -fill x

        set gn 1
        foreach name $showing {
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
        $_dispatcher event -now !resize

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

        set expand 0  ;# most controls float to top
        set row 0
        foreach name $showing {
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

                grid rowconfigure $_frame $row -weight 0

                switch -- [winfo class $wv] {
                    TextEntry {
                        if {[regexp {[0-9]+x[0-9]+} [$wv size]]} {
                            grid $wl -sticky n -pady 4
                            grid $wv -sticky nsew
                            grid rowconfigure $_frame $row -weight 1
                            grid columnconfigure $_frame 1 -weight 1
                            set expand 1
                        }
                    }
                    GroupEntry {
                        $wv configure -heading yes

                        #
                        # Scan through all children in this group
                        # and see if any demand more space.  If the
                        # group contains a structure or a note, then
                        # make sure that the group itself is set to
                        # expand/fill.
                        #
                        set queue [winfo children $wv]
                        set expandgroup 0
                        while {[llength $queue] > 0} {
                            set w [lindex $queue 0]
                            set queue [lrange $queue 1 end]
                            set c [winfo class $w]
                            if {[lsearch {DeviceEditor Note} $c] >= 0} {
                                set expandgroup 1
                                break
                            }
                            eval lappend queue [winfo children $w]
                        }
                        if {$expandgroup} {
                            set expand 1
                            grid $wv -sticky nsew
                            grid rowconfigure $_frame $row -weight 1
                        }
                    }
                    Note {
                        grid $wv -sticky nsew
                        grid rowconfigure $_frame $row -weight 1
                        set expand 1
                    }
                }
                grid columnconfigure $_frame 1 -weight 1
            } elseif {$wv == "--"} {
                grid rowconfigure $_frame $row -minsize 10
            }

            incr row
            grid rowconfigure $_frame $row -minsize $itk_option(-padding)
            incr row
        }
        grid $_frame.empty -row $row

        #
        # If there are any hidden items, then make the bottom of
        # this form fill up any extra space, so the form floats
        # to the top.  Otherwise, it will jitter around as the
        # hidden items come and go.
        #
        if {[llength $hidden] > 0 && !$expand} {
            grid rowconfigure $_frame 99 -weight 1
        } else {
            grid rowconfigure $_frame 99 -weight 0
        }
      }
    }
}

# ----------------------------------------------------------------------
# USAGE: _monitor <name> <state>
#
# Used internally to add/remove bindings that cause the widget
# associated with <name> to notify this controls widget of size
# changes.  Whenever there is a size change, this controls widget
# should fix its layout.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_monitor {name state} {
    set tag "Controls-$this"
    set wv $_name2info($name-value)
    if {$wv == "--" || [catch {bindtags $wv} btags]} {
        return
    }
    set i [lsearch $btags $tag]

    if {$state} {
        if {$i < 0} {
            bindtags $wv [linsert $btags 0 $tag]
        }
    } else {
        if {$i >= 0} {
            bindtags $wv [lreplace $btags $i $i]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _controlChanged <name>
#
# Invoked automatically whenever the value for a control changes.
# Sends a notification along to the tool controlling this panel.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_controlChanged {name} {
    set path $_name2info($name-path)

    #
    # Let the owner know that this control changed.
    #
    if {"" != $_owner} {
        $_owner changed $path
    }
}

# ----------------------------------------------------------------------
# USAGE: _controlValue <path> ?<units>?
#
# Used internally to get the value of a control with the specified
# <path>.  Returns the current value for the control.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_controlValue {path {units ""}} {
    if {"" != $_owner} {
        set val [$_owner valuefor $path]
        if {"" != $units} {
            set val [Rappture::Units::convert $val -to $units -units off]
        }
        return $val
    }
    return ""
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
    # we use _showing here instead of _controls because sometimes tabs
    # are disabled, and the index of the choosen tab always matches
    # _showing, even if tabs are disabled.
    set name [lindex $_showing $i]
    if {"" != $name} {
        foreach w [grid slaves $_frame] {
            grid forget $w
        }

        set wv $_name2info($name-value)
        grid $wv -row 0 -column 0 -sticky new
    }
}

# ----------------------------------------------------------------------
# USAGE: _resize
#
# Used internally to resize the widget when its contents change.
# ----------------------------------------------------------------------
itcl::body Rappture::Controls::_resize {} {
    switch -- $_scheme {
        tabs {
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
        }
        hlabels {
            # do nothing
        }
    }
}

# ----------------------------------------------------------------------
# OPTION: -padding
# ----------------------------------------------------------------------
itcl::configbody Rappture::Controls::padding {
    $_dispatcher event -idle !layout
}
