
# ----------------------------------------------------------------------
#  COMPONENT: drawingcontrols - Print X-Y plot.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

#option add *DrawingControls.width 3i widgetDefault
#option add *DrawingControls.height 3i widgetDefault
#option add *DrawingControls*Font "Arial 9" widgetDefault
option add *DrawingControls.padding 4 widgetDefault
option add *DrawingControls.labelFont \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault


itcl::class Rappture::DrawingControls {
    inherit itk::Widget

    constructor {args} {}
    destructor {}

    itk_option define -padding padding Padding 0

    public variable deactivatecommand ""

    public method add { path }
    public method delete { {name "all"} }

    private method ControlChanged { name } 
    private method ControlValue {path {units ""}}
    private method Rebuild {}
    private method FormatLabel {str}
    private method Monitor {name state}
    
    private variable _dispatcher ""
    private variable _controls ""
    private variable _name2info
    private variable _counter 0
    private variable _owner ""
    private variable _frame ""
    private variable _closeOnChange 0
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::constructor { owner args } {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this Rebuild]; list"
    
    set _owner $owner
    set _frame $itk_interior.frame
    frame $_frame 
    itk_component add cancel {
        button $itk_interior.cancel -text "cancel" -command $deactivatecommand
    }
    itk_component add save {
        button $itk_interior.save -text "save"  -command $deactivatecommand
    }
    #
    # Put this frame in whenever the control frame is empty.
    # It forces the size to contract back now when controls are deleted.
    #
    frame $_frame.empty -width 1 -height 1
    blt::table $itk_interior \
	0,0 $_frame -fill both -cspan 2 

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::destructor {} {
    array unset _name2info
}

itcl::body Rappture::DrawingControls::delete { {cname "all"} } {
    set controls $cname
    if { $cname == "all" } {
	set controls $_controls
    }
    foreach name $controls {
	set w $_frame.v$name
	destroy $w
	if { [info exists _name2info($name-label)] } {
	    destroy $_frame.l$name
	}
	set i [lsearch $_controls $name]
	set _controls [lreplace $_controls $i $i]
	array unset _name2info $name-*
    }
}

itcl::body Rappture::DrawingControls::add { path } {
    set pos [llength $_controls]
    incr _counter
    set name "control$_counter"
    set path [$_owner xml element -as path $path]

    set _name2info($name-path) $path
    set _name2info($name-label) ""
    set _name2info($name-type) ""
    set w $_frame.v$name
    set _name2info($name-value) $w
    set _name2info($name-enable) "yes"
    set _name2info($name-disablestyle) "greyout"

    set type [$_owner xml element -as type $path]
    set _name2info($name-type) $type
    switch -- $type {
        choice {
            Rappture::ChoiceEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        filechoice {
            Rappture::FileChoiceEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        filelist {
            Rappture::FileListEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        group {
            Rappture::GroupEntry $w $_owner $path
        }
        loader {
            Rappture::Loader $w $_owner $path -tool [$_owner tool]
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        number {
            Rappture::NumberEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        integer {
            Rappture::IntegerEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        boolean {
            Rappture::BooleanEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        string {
            Rappture::TextEntry $w $_owner $path
            bind $w <<Value>> [itcl::code $this ControlChanged $name]
        }
        image {
            Rappture::ImageEntry $w $_owner $path
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
    # If this element has an <enable> expression, then register its
    # controlling widget here.  
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
                    append enable [format {[ControlValue %s %s]} $stdpath $units]
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

    #set wid [$_owner widgetfor $path]

    if {[lsearch {control group separator note} $type] < 0} {
        # make a label for this control
        set label [$w label]
        if {"" != $label} {
            set _name2info($name-label) $_frame.l$name
            set font [option get $itk_component(hull) labelFont Font]
            label $_name2info($name-label) -text [FormatLabel $label] \
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
    Monitor $name on

    # now that we have a new control, we should fix the layout
    $_dispatcher event -idle !layout
    ControlChanged $name
    return $name
}

# ----------------------------------------------------------------------
# USAGE: ControlChanged <name>
#
# Invoked automatically whenever the value for a control changes.
# Sends a notification along to the tool controlling this panel.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::ControlChanged {name} {
    set wv $_name2info($name-value)
    set value [$wv value]
    set path $_name2info($name-path)
    set wid [$_owner widgetfor $path]
    # Push the new value into the shadow widget.
    $wid value $value
    # Overwrite the default value 
    $_owner xml put $path.default $value
    set path $_name2info($name-path)
    # Let the owner know that this control changed.
    if {"" != $_owner} {
        $_owner changed $path
    }
    eval $deactivatecommand
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Used internally to fix the layout of controls whenever controls
# are added or deleted, or when the control arrangement changes.
# There are a lot of heuristics here trying to achieve a "good"
# arrangement of controls.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::Rebuild {} {
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
    # SCHEME: hlabels
    # simple "Label: Value" layout
    #
    #pack forget $_tabs
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
    set _closeOnChange 0
    set slaves [grid slaves $_frame] 
    if { [llength $slaves] == 1 } {
	set slave [lindex $slaves 0]
	set _closeOnChange 1 
    }
}

# ----------------------------------------------------------------------
# USAGE: ControlValue <path> ?<units>?
#
# Used internally to get the value of a control with the specified
# <path>.  Returns the current value for the control.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::ControlValue {path {units ""}} {
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
# USAGE: FormatLabel <string>
#
# Used internally to format a label <string>.  Trims any excess
# white space and adds a ":" to the end.  That way, all labels
# have a uniform look.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::FormatLabel {str} {
    set str [string trim $str]
    if {"" != $str && [string index $str end] != ":"} {
        append str ":"
    }
    return $str
}

# ----------------------------------------------------------------------
# USAGE: Monitor <name> <state>
#
# Used internally to add/remove bindings that cause the widget
# associated with <name> to notify this controls widget of size
# changes.  Whenever there is a size change, this controls widget
# should fix its layout.
# ----------------------------------------------------------------------
itcl::body Rappture::DrawingControls::Monitor {name state} {
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
# OPTION: -padding
# ----------------------------------------------------------------------
itcl::configbody Rappture::DrawingControls::padding {
    $_dispatcher event -idle !layout
}
