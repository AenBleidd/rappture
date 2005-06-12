# ----------------------------------------------------------------------
#  COMPONENT: textentry - general-purpose text entry widget
#
#  This widget is a cross between the Tk entry and text widgets.  For
#  one-line messages, it acts like an entry widget.  For larger
#  messages, it morphs into a text widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *TextEntry.size auto widgetDefault
option add *TextEntry.width 0 widgetDefault
option add *TextEntry.height 0 widgetDefault
option add *TextEntry.editable yes widgetDefault
option add *TextEntry.textBackground white widgetDefault

option add *TextEntry.hintForeground gray50 widgetDefault
option add *TextEntry.hintFont \
    -*-helvetica-medium-r-normal-*-*-100-* widgetDefault

itcl::class Rappture::TextEntry {
    inherit itk::Widget

    itk_option define -editable editable Editable ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {xmlobj path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}
    public method size {} { return $_size }

    protected method _layout {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _xmlobj ""   ;# XML containing description
    private variable _path ""     ;# path in XML to this number

    private variable _mode ""       ;# entry or text mode
    private variable _size ""       ;# size hint from XML
}
                                                                                
itk::usual TextEntry {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::constructor {xmlobj path args} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be Rappture::library"
    }
    set _xmlobj $xmlobj
    set _path $path

    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout "[itcl::code $this _layout]; list"

    set _size [$xmlobj get $path.size]

    set hints [$xmlobj get $path.about.hints]
    if {[string length $hints] > 0} {
        itk_component add hints {
            label $itk_interior.hints -anchor w -text $hints
        } {
            usual
            rename -foreground -hintforeground hintForeground Foreground
            rename -font -hintfont hintFont Font
        }
        pack $itk_component(hints) -side bottom -fill x
    }

    eval itk_initialize $args

    set str [$xmlobj get $path.default]
    if {"" != $str} { value $str }
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
        if {$_mode == "entry"} {
            $itk_component(entry) configure -state normal
            $itk_component(emenu) entryconfigure "Cut" -state normal
            $itk_component(emenu) entryconfigure "Copy" -state normal
            $itk_component(emenu) entryconfigure "Paste" -state normal
            $itk_component(entry) delete 0 end
            $itk_component(entry) insert 0 $newval
            if {!$itk_option(-editable)} {
                $itk_component(entry) configure -state disabled
                $itk_component(emenu) entryconfigure "Cut" -state disabled
                $itk_component(emenu) entryconfigure "Copy" -state disabled
                $itk_component(emenu) entryconfigure "Paste" -state disabled
            }
        } elseif {$_mode == "text"} {
            $itk_component(text) configure -state normal
            $itk_component(tmenu) entryconfigure "Cut" -state normal
            $itk_component(tmenu) entryconfigure "Copy" -state normal
            $itk_component(tmenu) entryconfigure "Paste" -state normal
            $itk_component(text) delete 1.0 end
            $itk_component(text) insert end $newval
            if {!$itk_option(-editable)} {
                $itk_component(text) configure -state disabled
                $itk_component(tmenu) entryconfigure "Cut" -state disabled
                $itk_component(tmenu) entryconfigure "Copy" -state disabled
                $itk_component(tmenu) entryconfigure "Paste" -state disabled
            }
        }
        $_dispatcher event -idle !layout
        event generate $itk_component(hull) <<Value>>
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    if {$_mode == "entry"} {
        return [$itk_component(entry) get]
    } elseif {$_mode == "text"} {
        return [$itk_component(text) get 1.0 end-1char]
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
    set label [$_xmlobj get $_path.about.label]
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
    set str [$_xmlobj get $_path.about.description]
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
        if {$_mode == "entry"} {
            set val [$itk_component(entry) get]
        } elseif {$_mode == "text"} {
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
        if {$_mode != "entry"} {
            set val ""
            if {$_mode == "text"} {
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

            itk_component add emenu {
                menu $itk_component(entry).menu -tearoff 0
            }
            $itk_component(emenu) add command -label "Cut" -accelerator "^X" \
                -command [list event generate $itk_component(entry) <<Cut>>]
            $itk_component(emenu) add command -label "Copy" -accelerator "^C" \
                -command [list event generate $itk_component(entry) <<Copy>>]
            $itk_component(emenu) add command -label "Paste" -accelerator "^V" \
                -command [list event generate $itk_component(entry) <<Paste>>]
            bind $itk_component(entry) <<PopupMenu>> {
                tk_popup %W.menu %X %Y
            }

            $itk_component(entry) insert end $val
            if {!$itk_option(-editable)} {
                $itk_component(entry) configure -state disabled
            }
            set _mode "entry"
        }
        $itk_component(entry) configure -width $size

    } elseif {[regexp {^([0-9]+)x([0-9]+)$} $size match w h]} {
        #
        # If the size is WWxHH, then flip to text mode, with
        # a requested size of HH lines by WW characters.
        #
        if {$_mode != "text"} {
            set val ""
            if {$_mode == "entry"} {
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
                    -width 1 -height 1 -wrap word
            } {
                usual
                rename -background -textbackground textBackground Background
                rename -foreground -textforeground textForeground Foreground
            }
            $itk_component(text) configure \
                -background $itk_option(-textbackground) \
                -foreground $itk_option(-textforeground)
            $itk_component(scrollbars) contents $itk_component(text)

            itk_component add tmenu {
                menu $itk_component(text).menu -tearoff 0
            }
            $itk_component(tmenu) add command -label "Cut" -accelerator "^X" \
                -command [list event generate $itk_component(text) <<Cut>>]
            $itk_component(tmenu) add command -label "Copy" -accelerator "^C" \
                -command [list event generate $itk_component(text) <<Copy>>]
            $itk_component(tmenu) add command -label "Paste" -accelerator "^V" \
                -command [list event generate $itk_component(text) <<Paste>>]
            bind $itk_component(text) <<PopupMenu>> {
                tk_popup %W.menu %X %Y
            }

            $itk_component(text) insert end $val
            if {!$itk_option(-editable)} {
                $itk_component(text) configure -state disabled
                $itk_component(menu) entryconfigure "Cut" -state disabled
                $itk_component(menu) entryconfigure "Copy" -state disabled
                $itk_component(menu) entryconfigure "Paste" -state disabled
            }
            set _mode "text"
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
# CONFIGURATION OPTION: -editable
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::editable {
    if {![string is boolean -strict $itk_option(-editable)]} {
        error "bad value \"$itk_option(-editable)\": should be boolean"
    }

    if {$itk_option(-editable)} {
        set state normal
    } else {
        set state disabled
    }
    if {$_mode == "entry"} {
        $itk_component(editor) configure -state $state
        $itk_component(emenu) entryconfigure "Cut" -state $state
        $itk_component(emenu) entryconfigure "Copy" -state $state
        $itk_component(emenu) entryconfigure "Paste" -state $state
    } elseif {$_mode == "text"} {
        $itk_component(text) configure -state $state
        $itk_component(tmenu) entryconfigure "Cut" -state $state
        $itk_component(tmenu) entryconfigure "Copy" -state $state
        $itk_component(tmenu) entryconfigure "Paste" -state $state
    }
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
