# ----------------------------------------------------------------------
#  COMPONENT: textentry - general-purpose text entry widget
#
#  This widget is a cross between the Tk entry and text widgets.  For
#  one-line messages, it acts like an entry widget.  For larger
#  messages, it morphs into a text widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *TextEntry.size auto widgetDefault
option add *TextEntry.width 0 widgetDefault
option add *TextEntry.height 0 widgetDefault
option add *TextEntry.editable yes widgetDefault
option add *TextEntry.textBackground white widgetDefault
option add *TextEntry*disabledForeground #a3a3a3 widgetDefault
option add *TextEntry*disabledBackground white widgetDefault

option add *TextEntry.hintForeground gray50 widgetDefault
option add *TextEntry.hintFont \
    -*-helvetica-medium-r-normal-*-10-* widgetDefault
option add *TextEntry.codeFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault


itcl::class Rappture::TextEntry {
    inherit itk::Widget

    itk_option define -editable editable Editable ""
    itk_option define -state state State "normal"
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}
    public method size {} { return $_size }

    protected method _layout {}
    protected method _setValue {value}
    protected method _newValue {}
    protected method _edit {option args}
    protected method _fixState {}
    protected method _uploadValue {args}
    protected method _downloadValue {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _owner ""      ;# thing managing this control
    private variable _path ""       ;# path in XML to this number

    private variable _layout ""     ;# entry or full text size
    private variable _mode "ascii"  ;# ascii text or binary data
    private variable _value ""      ;# value inside the widget
    private variable _size ""       ;# size hint from XML
}
                                                                                
itk::usual TextEntry {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this _layout]; list"

    set _size [$_owner xml get $path.size]

    set hints [$_owner xml get $path.about.hints]
    if {[string length $hints] > 0} {
        itk_component add hints {
            ::label $itk_interior.hints -anchor w -text $hints
        } {
            usual
            rename -foreground -hintforeground hintForeground Foreground
            rename -font -hintfont hintFont Font
        }
        pack $itk_component(hints) -side bottom -fill x
    }

    eval itk_initialize $args

    set str [$_owner xml get $path.default]
    if {"" != $str} {
        _layout  ;# must fix layout or value won't take
        value $str
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
itcl::body Rappture::TextEntry::value {args} {
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
        _setValue $newval

        $_dispatcher event -idle !layout
        event generate $itk_component(hull) <<Value>>
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    if {$_mode == "ascii"} {
        if {$_layout == "entry"} {
            return [$itk_component(entry) get]
        } elseif {$_layout == "text"} {
            return [$itk_component(text) get 1.0 end-1char]
        }
    } else {
        return $_value
    }
    return ""
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::label {} {
    set label [$_owner xml get $_path.about.label]
    if {"" == $label} {
        set label "String"
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
itcl::body Rappture::TextEntry::tooltip {} {
    set str [$_owner xml get $_path.about.description]
    return [string trim $str]
}

# ----------------------------------------------------------------------
# USAGE: _layout
#
# Used internally to change the layout of this widget depending
# on the .size hint and its contents.  Switches between an entry
# and a text widget.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_layout {} {
    set size $_size
    if {$size == "" || $size == "auto"} {
        #
        # If the size is "auto", then look at the current value
        # and count its lines/characters.
        #
        set val ""
        if {$_layout == "entry"} {
            set val [$itk_component(entry) get]
        } elseif {$_layout == "text"} {
            set val [$itk_component(text) get 1.0 end-1char]
        }

        set chars 0
        set lines 0
        foreach line [split $val \n] {
            incr lines
            if {[string length $line] > $chars} {
                set chars [string length $line]
            }
        }
        incr chars

        if {$lines > 1} {
            set size "${chars}x${lines}"
        } else {
            set size $chars
        }
    }

    if {[regexp {^[0-9]+$} $size]} {
        #
        # If the size is WW, then flip to entry mode, with
        # a requested size of WW characters.
        #
        if {$_layout != "entry"} {
            set val ""
            if {$_layout == "text"} {
                set val [$itk_component(text) get 1.0 end-1char]
                destroy $itk_component(text)
                destroy $itk_component(scrollbars)
            }

            itk_component add entry {
                entry $itk_interior.entry
            } {
                usual
                rename -background -textbackground textBackground Background
                rename -foreground -textforeground textForeground Foreground
            }
            pack $itk_component(entry) -expand yes -fill both
            $itk_component(entry) configure \
                -background $itk_option(-textbackground) \
                -foreground $itk_option(-textforeground)

            bind $itk_component(entry) <KeyPress> [itcl::code $this _newValue]
            bind $itk_component(entry) <Control-KeyPress-a> \
                "[list $itk_component(entry) selection range 0 end]; break"

            itk_component add emenu {
                menu $itk_component(entry).menu -tearoff 0
            }
            $itk_component(emenu) add command -label "Cut" -accelerator "^X" \
                -command [list event generate $itk_component(entry) <<Cut>>]
            $itk_component(emenu) add command -label "Copy" -accelerator "^C" \
                -command [list event generate $itk_component(entry) <<Copy>>]
            $itk_component(emenu) add command -label "Paste" -accelerator "^V" \
                -command [list event generate $itk_component(entry) <<Paste>>]
            $itk_component(emenu) add command -label "Select All" -accelerator "^A" -command [list $itk_component(entry) selection range 0 end]
            bind $itk_component(entry) <<PopupMenu>> \
                [itcl::code $this _edit menu emenu %X %Y]

            set _layout "entry"
            _setValue $val
        }
        $itk_component(entry) configure -width $size

    } elseif {[regexp {^([0-9]+)x([0-9]+)$} $size match w h]} {
        #
        # If the size is WWxHH, then flip to text mode, with
        # a requested size of HH lines by WW characters.
        #
        if {$_layout != "text"} {
            set val ""
            if {$_layout == "entry"} {
                set val [$itk_component(entry) get]
                destroy $itk_component(entry)
            }

            itk_component add scrollbars {
                Rappture::Scroller $itk_interior.scrl \
                     -xscrollmode auto -yscrollmode auto
            }
            pack $itk_component(scrollbars) -expand yes -fill both

            itk_component add text {
                text $itk_component(scrollbars).text \
                    -width 1 -height 1 -wrap char
            } {
                usual
                rename -background -textbackground textBackground Background
                rename -foreground -textforeground textForeground Foreground
                rename -font -codefont codeFont CodeFont
            }
            $itk_component(text) configure \
                -background $itk_option(-textbackground) \
                -foreground $itk_option(-textforeground) \
                -font $itk_option(-codefont)
            $itk_component(scrollbars) contents $itk_component(text)

            bind $itk_component(text) <KeyPress> [itcl::code $this _newValue]
            bind $itk_component(text) <Control-KeyPress-a> \
                "[list $itk_component(text) tag add sel 1.0 end]; break"

            itk_component add tmenu {
                menu $itk_component(text).menu -tearoff 0
            }
            $itk_component(tmenu) add command -label "Cut" -accelerator "^X" \
                -command [list event generate $itk_component(text) <<Cut>>]
            $itk_component(tmenu) add command -label "Copy" -accelerator "^C" \
                -command [list event generate $itk_component(text) <<Copy>>]
            $itk_component(tmenu) add command -label "Paste" -accelerator "^V" \
                -command [list event generate $itk_component(text) <<Paste>>]
            $itk_component(tmenu) add command -label "Select All" -accelerator "^A" -command [list $itk_component(text) tag add sel 1.0 end]
            $itk_component(tmenu) add separator

            $itk_component(tmenu) add command \
                -label [Rappture::filexfer::label upload] \
                -command [itcl::code $this _uploadValue -start]
            $itk_component(tmenu) add command \
                -label [Rappture::filexfer::label download] \
                -command [itcl::code $this _downloadValue]

            bind $itk_component(text) <<PopupMenu>> \
                [itcl::code $this _edit menu tmenu %X %Y]

            set _layout "text"
            _setValue $val
        }
        $itk_component(text) configure -width $w -height $h
    }

    #
    # Fix the overall widget size according to -width / -height
    #
    if {$itk_option(-width) == 0 && $itk_option(-height) == 0} {
        pack propagate $itk_component(hull) yes
    } else {
        pack propagate $itk_component(hull) no
        component hull configure \
            -width $itk_option(-width) -height $itk_option(-width)
    }
}

# ----------------------------------------------------------------------
# USAGE: _setValue <newValue>
#
# Used internally to set the value for this widget.  If the <newValue>
# string is ASCII, then it is stored directly and the widget is enabled
# for editing.  Otherwise, the value is cached and a representation of
# the data is displayed.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_setValue {newval} {
    if {[Rappture::encoding::is binary $newval]} {
        # looks like a binary file
        set _mode "binary"
        set _value $newval

        if {$_layout == "entry" || [string match {*x[01]} $_size]} {
            set newval [Rappture::utils::hexdump -lines 0 $_value]
        } else {
            set newval [Rappture::utils::hexdump -lines 1000 $_value]
        }
    } else {
        # ascii file -- map carriage returns to line feeds
        set _mode "ascii"
        set _value ""
        regsub -all "\r\n" $newval "\n" newval
        regsub -all "\r" $newval "\n" newval
    }

    if {$_layout == "entry"} {
        $itk_component(entry) configure -state normal
        $itk_component(emenu) entryconfigure "Cut" -state normal
        $itk_component(emenu) entryconfigure "Paste" -state normal
        $itk_component(entry) delete 0 end
        $itk_component(entry) insert 0 $newval
        if {!$itk_option(-editable) || $_mode == "binary"} {
            $itk_component(entry) configure -state disabled
            $itk_component(emenu) entryconfigure "Cut" -state disabled
            $itk_component(emenu) entryconfigure "Paste" -state disabled
        }
    } elseif {$_layout == "text"} {
        $itk_component(text) configure -state normal
        $itk_component(tmenu) entryconfigure "Cut" -state normal
        $itk_component(tmenu) entryconfigure "Paste" -state normal
        $itk_component(text) delete 1.0 end
        $itk_component(text) insert end $newval
        if {!$itk_option(-editable) || $_mode == "binary"} {
            set hull $itk_component(hull)
            set dfg [option get $hull disabledForeground Foreground]
            set dbg [option get $hull disabledBackground Background]
            $itk_component(text) configure -state disabled \
                -background $dbg -foreground $dfg
            $itk_component(tmenu) entryconfigure "Cut" -state disabled
            $itk_component(tmenu) entryconfigure "Paste" -state disabled
        } else {
            $itk_component(text) configure \
                -background $itk_option(-textbackground) \
                -foreground $itk_option(-textforeground)
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _newValue
#
# Invoked automatically whenever the value in the entry changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_newValue {} {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: _edit menu <which> <X> <Y>
#
# Used internally to manage edit operations.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_edit {option args} {
    if {$itk_option(-state) == "disabled"} {
        return  ;# disabled? then bail out here!
    }
    switch -- $option {
        menu {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_edit $option which x y\""
            }
            set mname [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]
            tk_popup $itk_component($mname) $x $y
        }
        default {
            error "bad option \"$option\": should be menu"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixState
#
# Used internally to update the internal widgets whenever the
# -state/-editable options change.  Enables or disables various
# widgets.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_fixState {} {
    if {$itk_option(-editable) && $itk_option(-state) == "normal"} {
        set state normal
    } else {
        set state disabled
    }
    if {$_layout == "entry"} {
        $itk_component(entry) configure -state $state
        $itk_component(emenu) entryconfigure "Cut" -state $state
        $itk_component(emenu) entryconfigure "Copy" -state $state
        $itk_component(emenu) entryconfigure "Paste" -state $state
    } elseif {$_layout == "text"} {
        $itk_component(text) configure -state $state
        $itk_component(tmenu) entryconfigure "Cut" -state $state
        $itk_component(tmenu) entryconfigure "Copy" -state $state
        $itk_component(tmenu) entryconfigure "Paste" -state $state
    }
}

# ----------------------------------------------------------------------
# USAGE: _uploadValue -start
# USAGE: _uploadValue -assign <key> <value> <key> <value> ...
#
# Used internally to initiate an upload operation.  Prompts the
# user to upload into the text area of this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_uploadValue {args} {
    switch -- $_layout {
        entry   { set widget $itk_component(entry) }
        text    { set widget $itk_component(text) }
        default { set widget $itk_component(hull) }
    }

    set opt [lindex $args 0]
    switch -- $opt {
        -start {
            set tool [Rappture::Tool::resources -appname]
            set cntls [list $_path [label] [tooltip]]
            Rappture::filexfer::upload \
                $tool $cntls [itcl::code $this _uploadValue -assign]
        }
        -assign {
            array set data [lrange $args 1 end] ;# skip option
            if {[info exists data(error)]} {
                Rappture::Tooltip::cue $widget $data(error)
            }
            if {[info exists data(data)]} {
                Rappture::Tooltip::cue hide  ;# take down note about the popup
                _setValue $data(data)
                _newValue
            }
        }
        default {
            error "bad option \"$opt\": should be -start or -assign"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _downloadValue
#
# Used internally to initiate a download operation.  Takes the current
# value and downloads it to the user in a new browser window.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_downloadValue {} {
    set mesg [Rappture::filexfer::download [value] input.txt]

    if {"" != $mesg} {
        switch -- $_layout {
            entry   { set widget $itk_component(entry) }
            text    { set widget $itk_component(text) }
            default { set widget $itk_component(hull) }
        }
        Rappture::Tooltip::cue $widget $mesg
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -editable
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::editable {
    if {![string is boolean -strict $itk_option(-editable)]} {
        error "bad value \"$itk_option(-editable)\": should be boolean"
    }
    _fixState
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    if {$_layout == "text"} {
        if {$itk_option(-state) == "disabled"} {
            set fg [option get $itk_component(text) disabledForeground Foreground]
        } else {
            set fg $itk_option(-foreground)
        }
        $itk_component(text) configure -foreground $fg
    }
    _fixState
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::width {
    # check size to see if it has the proper form
    winfo pixels $itk_component(hull) $itk_option(-width)
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::height {
    # check size to see if it has the proper form
    winfo pixels $itk_component(hull) $itk_option(-height)
    $_dispatcher event -idle !layout
}
