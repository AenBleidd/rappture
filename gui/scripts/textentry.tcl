# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: textentry - general-purpose text entry widget
#
#  This widget is a cross between the Tk entry and text widgets.  For
#  one-line messages, it acts like an entry widget.  For larger
#  messages, it morphs into a text widget.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *TextEntry.size auto widgetDefault
option add *TextEntry.width 0 widgetDefault
option add *TextEntry.height 0 widgetDefault
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

    itk_option define -state state State "normal"
    itk_option define -disabledforeground disabledForeground DisabledForeground ""
    itk_option define -disabledbackground disabledBackground DisabledBackground ""
    itk_option define -width width Width 0
    itk_option define -height height Height 0

    constructor {owner path args} {
        # defined below
    }

    public method value {args}

    public method label {}
    public method tooltip {}
    public method size {} { return $_size }

    protected method _layout {}
    protected method _setValue {value}
    protected method _newValue {}
    protected method _edit {option args}
    protected method _uploadValue {args}
    protected method _downloadValue {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _owner ""      ;# thing managing this control
    private variable _path ""       ;# path in XML to this number

    private variable _layout ""     ;# entry or full text size
    private variable _value ""      ;# value inside the widget
    private variable _size ""       ;# size hint from XML
    private variable _icon ""       ;# size hint from XML
}

itk::usual TextEntry {
    keep -foreground -background -textbackground -font -cursor
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

    set _size [string trim [$_owner xml get $path.size]]

    set hints [string trim [$_owner xml get $path.about.hints]]
    set icon  [string trim [$_owner xml get $path.about.icon]]
    if {[string length $icon] > 0} {
        set _icon [image create photo -data $icon]
    }
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

    # Trimming for now.  May need a <verbatim> flag to indicate to leave
    # the string alone.
    set str [string trim [$_owner xml get $path.default]]
    if {"" != $str} {
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
        set newval [string trim [lindex $args 0]]
        _setValue $newval
        _newValue

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    switch -- $_layout {
        entry {
            return [$itk_component(entry) get]
        }
        text {
            return [$itk_component(text) get 1.0 end-1char]
        }
        binary {
            return $_value
        }
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
    set label [string trim [$_owner xml get $_path.about.label]]
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
    set str [string trim [$_owner xml get $_path.about.description]]
    return $str
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
        if {[string length $_value] > 1920} {
            # if size is really big, don't bother counting lines
            set size "80x24"
        } else {
            set chars 0
            set lines 0
            foreach line [split $_value \n] {
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
    }

    if {$size == "binary" || [Rappture::encoding::is binary $_value]} {
        set newlayout "binary"
    } elseif {[regexp {^[0-9]+$} $size]} {
        set newlayout "entry"
    } elseif {[regexp {^([0-9]+)x([0-9]+)$} $size match w h]} {
        set newlayout "text"
    }

    if {$newlayout != $_layout} {
        set oldval ""
        if {$_layout == "entry"} {
            set oldval [$itk_component(entry) get]
        } elseif {$_layout == "text"} {
            set oldval [$itk_component(text) get 1.0 end-1char]
        }

        # Set the _layout here because unpacking the widgets triggers
        # the "_edit finalize" method (focus change).

        set _layout $newlayout

        # take down any existing widget
        foreach win [pack slaves $itk_interior] {
            if { [winfo name $win] != "hints" } {
                pack forget $win
            }
        }

        switch -- $newlayout {
            entry {
                # don't have the entry widget yet? then create it
                if {![winfo exists $itk_interior.entry]} {
                    itk_component add entry {
                        entry $itk_interior.entry
                    } {
                        usual
                        rename -background -textbackground textBackground Background
                        rename -foreground -textforeground textForeground Foreground
                    }
                    $itk_component(entry) configure \
                        -background $itk_option(-textbackground) \
                        -foreground $itk_option(-textforeground)

                    # Make sure these event bindings occur after the class bindings.
                    # Otherwise you'll always get the entry value before the edit.
                    bind textentry-$this <KeyPress> \
                        [itcl::code $this _newValue]
                    bind textentry-$this <KeyPress-Return> \
                        [itcl::code $this _edit finalize]
                    bind textentry-$this <Control-KeyPress-a> \
                        "[list $itk_component(entry) selection range 0 end]; break"
                    bind textentry-$this <FocusOut> \
                        [itcl::code $this _edit finalize]
                    bind textentry-$this <Unmap> \
                        [itcl::code $this _edit finalize]
                    set bindtags [bindtags $itk_component(entry)]
                    lappend bindtags textentry-$this
                    bindtags $itk_component(entry) $bindtags

                    itk_component add emenu {
                        menu $itk_component(entry).menu -tearoff 0
                    }
                    $itk_component(emenu) add command \
                        -label "Cut" -accelerator "^X" \
                        -command [itcl::code $this _edit action entry cut]
                    $itk_component(emenu) add command \
                        -label "Copy" -accelerator "^C" \
                        -command [itcl::code $this _edit action entry copy]
                    $itk_component(emenu) add command \
                        -label "Paste" -accelerator "^V" \
                        -command [itcl::code $this _edit action entry paste]
                    $itk_component(emenu) add command \
                        -label "Select All" -accelerator "^A" \
                        -command [itcl::code $this _edit action entry selectall]
                    bind $itk_component(entry) <<PopupMenu>> \
                        [itcl::code $this _edit menu emenu %X %Y]
                }

                # show the entry widget
                pack $itk_component(entry) -expand yes -fill both

                # load any previous value
                regsub -all "\n" $oldval "" oldval
                $itk_component(entry) delete 0 end
                $itk_component(entry) insert end $oldval
            }

            text {
                if {![winfo exists $itk_interior.scrl]} {
                    itk_component add scrollbars {
                        Rappture::Scroller $itk_interior.scrl \
                            -xscrollmode auto -yscrollmode auto
                    }

                    itk_component add text {
                        text $itk_component(scrollbars).text \
                            -width 1 -height 1 -wrap char
                    } {
                        usual
                        ignore -highlightthickness -highlightcolor
                        rename -background -textbackground textBackground Background
                        rename -foreground -textforeground textForeground Foreground
                        rename -font -codefont codeFont CodeFont
                    }
                    $itk_component(text) configure \
                        -background $itk_option(-textbackground) \
                        -foreground $itk_option(-textforeground) \
                        -font $itk_option(-codefont) \
                        -highlightthickness 1
                    $itk_component(scrollbars) contents $itk_component(text)

                    # Make sure these event bindings occur after the class bindings.
                    # Otherwise you'll always get the text value before the edit.
                    bind textentry-$this <KeyPress> \
                        [itcl::code $this _newValue]
                    # leave [Return] key alone for multi-line text so the user
                    # can enter newlines and keep editing
                    bind textentry-$this <Control-KeyPress-a> \
                        "[list $itk_component(text) tag add sel 1.0 end]; break"
                    bind textentry-$this <FocusOut> \
                        [itcl::code $this _edit finalize]
                    bind textentry-$this <Unmap> \
                        [itcl::code $this _edit finalize]
                    set bindtags [bindtags $itk_component(text)]
                    lappend bindtags textentry-$this
                    bindtags $itk_component(text) $bindtags

                    itk_component add tmenu {
                        menu $itk_component(text).menu -tearoff 0
                    }
                    $itk_component(tmenu) add command \
                        -label "Cut" -accelerator "^X" \
                        -command [itcl::code $this _edit action text cut]
                    $itk_component(tmenu) add command \
                        -label "Copy" -accelerator "^C" \
                        -command [itcl::code $this _edit action text copy]
                    $itk_component(tmenu) add command \
                        -label "Paste" -accelerator "^V" \
                        -command [itcl::code $this _edit action text paste]
                    $itk_component(tmenu) add command \
                        -label "Select All" -accelerator "^A" \
                        -command [itcl::code $this _edit action text selectall]
                    $itk_component(tmenu) add separator

                    $itk_component(tmenu) add command \
                        -label [Rappture::filexfer::label upload] \
                        -command [itcl::code $this _uploadValue -start]
                    $itk_component(tmenu) add command \
                        -label [Rappture::filexfer::label download] \
                        -command [itcl::code $this _downloadValue]

                    bind $itk_component(text) <<PopupMenu>> \
                        [itcl::code $this _edit menu tmenu %X %Y]
                }

                # show the text editor widget
                pack $itk_component(scrollbars) -expand yes -fill both
                $itk_component(text) configure -width $w -height $h

                # load any previous value
                $itk_component(text) delete 1.0 end
                $itk_component(text) insert end $oldval
            }

            binary {
                if {![winfo exists $itk_interior.bin]} {
                    itk_component add binary {
                        frame $itk_interior.bin
                    }
                    set icon $_icon
                    if { $icon == "" } {
                        set icon [Rappture::icon binary]
                    }
                    itk_component add binicon {
                        ::label $itk_component(binary).binicon \
                            -image $icon -borderwidth 0
                    }
                    pack $itk_component(binicon) -side left

                    itk_component add bininfo {
                        ::label $itk_component(binary).bininfo \
                            -text "Empty\n0 bytes" \
                            -width 5 -justify left -anchor w -borderwidth 0
                    }
                    pack $itk_component(bininfo) -side left -expand yes -fill x -padx 4

                    itk_component add bmenu {
                        menu $itk_component(binary).menu -tearoff 0
                    }
                    $itk_component(bmenu) add command \
                        -label [Rappture::filexfer::label upload] \
                        -command [itcl::code $this _uploadValue -start]
                    $itk_component(bmenu) add command \
                        -label [Rappture::filexfer::label download] \
                        -command [itcl::code $this _downloadValue]

                    bind $itk_component(binicon) <<PopupMenu>> \
                        [itcl::code $this _edit menu bmenu %X %Y]
                    bind $itk_component(bininfo) <<PopupMenu>> \
                        [itcl::code $this _edit menu bmenu %X %Y]
                }

                # show the binary mode rep
                pack $itk_component(binary) -side top -fill x

            }
            default {
                error "don't know how to handle mode \"$newlayout\" for string editor"
            }
        }
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
        set _value $newval
    } else {
        # ascii file -- map carriage returns to line feeds
        regsub -all "\r\n" $newval "\n" newval
        regsub -all "\r" $newval "\n" newval
        set _value $newval
    }

    # fix up the layout so the display widgets exist, then load the new value
    _layout

    switch -- $_layout {
        entry {
            $itk_component(entry) configure -state normal
            $itk_component(entry) delete 0 end
            $itk_component(entry) insert end $_value
            $itk_component(entry) configure -state $itk_option(-state)
        }
        text {
            $itk_component(text) configure -state normal
            $itk_component(text) delete 1.0 end
            $itk_component(text) insert end $_value
            $itk_component(text) configure -state $itk_option(-state)
        }
        binary {
            set desc [Rappture::utils::datatype $_value]
            append desc "\n[Rappture::utils::binsize [string length $_value]]"
            $itk_component(bininfo) configure -text $desc
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
# USAGE: _edit action <which> <action>
# USAGE: _edit menu <which> <X> <Y>
# USAGE: _edit finalize
# USAGE: _edit log
#
# Used internally to manage edit operations.
# ----------------------------------------------------------------------
itcl::body Rappture::TextEntry::_edit {option args} {
    if {$itk_option(-state) == "disabled"} {
        return  ;# disabled? then bail out here!
    }
    switch -- $option {
        action {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"_edit $option which action\""
            }
            set widget [lindex $args 0]
            set action [lindex $args 1]
            switch -- $action {
                cut - copy - paste {
                    set event "<<[string totitle $action]>>"
                    event generate $itk_component($widget) $event
                }
                selectall {
                    switch -- $widget {
                        entry { $itk_component(entry) selection range 0 end }
                        text  { $itk_component(text) tag add sel 1.0 end }
                        default { error "don't know how to select for $widget" }
                    }
                }
                default {
                    error "don't know how to handle action $action"
                }
            }
            Rappture::Logger::log input $_path -action $action
        }
        menu {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_edit $option which x y\""
            }
            set mname [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]
            tk_popup $itk_component($mname) $x $y
        }
        finalize {
            event generate $itk_component(hull) <<Final>>

            # log each final value
            set newval [value]
            if {$newval ne $_value} {
                Rappture::Logger::log input $_path $newval
                set _value $newval
            }
        }
        default {
            error "bad option \"$option\": should be action, menu, log"
        }
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

    set opt [string trim [lindex $args 0]]
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
                Rappture::Logger::log warning $_path $data(error)
            }
            if {[info exists data(data)]} {
                Rappture::Tooltip::cue hide  ;# take down note about the popup
                _setValue $data(data)
                _newValue
                Rappture::Logger::log upload $_path $data(data)
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

    if {$mesg ne ""} {
        switch -- $_layout {
            entry   { set widget $itk_component(entry) }
            text    { set widget $itk_component(text) }
            default { set widget $itk_component(hull) }
        }
        Rappture::Tooltip::cue $widget $mesg
        Rappture::Logger::log warning $_path $mesg
    } else {
        Rappture::Logger::log download $_path
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::TextEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    if {[info exists itk_component(text)]} {
        $itk_component(text) configure -state $itk_option(-state)
        $itk_component(tmenu) entryconfigure "Cut" -state $itk_option(-state)
        $itk_component(tmenu) entryconfigure "Copy" -state $itk_option(-state)
        $itk_component(tmenu) entryconfigure "Paste" -state $itk_option(-state)
        if {$itk_option(-state) == "disabled"} {
            $itk_component(text) configure \
                -foreground $itk_option(-disabledforeground) \
                -background $itk_option(-disabledbackground)
        } else {
            $itk_component(text) configure \
                -foreground $itk_option(-foreground) \
                -background $itk_option(-textbackground)
        }
    }
    if {[info exists itk_component(entry)]} {
        $itk_component(entry) configure -state $itk_option(-state)
        $itk_component(emenu) entryconfigure "Cut" -state $itk_option(-state)
        $itk_component(emenu) entryconfigure "Copy" -state $itk_option(-state)
        $itk_component(emenu) entryconfigure "Paste" -state $itk_option(-state)
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


