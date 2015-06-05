# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: postern - a back door for debugging
#
#  This utility gives you a console that you can use to debug any
#  live application.  You have to click and type the proper magic
#  in the right spot, and a console area will appear to handle
#  your requests.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT
package require Itk

option add *Postern.size 2 widgetDefault
option add *Postern.activeColor gray widgetDefault
option add *Postern.popup above widgetDefault
option add *Postern*Text.font \
    -*-courier-medium-r-normal-*-12-* widgetDefault
option add *Postern*Text.errorFont \
    -*-courier-medium-o-normal-*-12-* widgetDefault

itcl::class Rappture::Postern {
    inherit itk::Widget

    itk_option define -size size Size 1
    itk_option define -activecolor activeColor ActiveColor ""
    itk_option define -popup popup Popup ""

    constructor {args} { # defined below }
    destructor { # defined below }
    public method open {}
    public method close {}
    public method activate {args}
    public method command {option}

    public method _fake_puts {args}

    private variable _active 0    ;# true when active and able to open
    private variable _focus ""    ;# focus before this widget took over
    private variable _afterid ""  ;# id for after event that clears activate

    private variable _history ""  ;# list of recent commands
    private variable _hpos 0      ;# current index in _history
}

itk::usual Postern {
    keep -cursor -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::constructor {args} {
    global env tcl_platform

    # this sequence gets things started...
    if {$tcl_platform(os) == "Darwin"} {
        # this works better on the Mac
        bind $itk_component(hull) <Control-Button-1><Control-Button-1> \
            [itcl::code $this activate on]
    } else {
        bind $itk_component(hull) <Button-1><Button-1><Button-3><Button-3> \
            [itcl::code $this activate on]
    }

    #
    # Get the magic word from the environment.
    #
    if {[info exists env(RAPPTURE_POSTERN)]} {
        set event ""
        foreach letter [split $env(RAPPTURE_POSTERN) ""] {
            append event "<Key-$letter>"
        }
        bind $itk_component(hull) $event [itcl::code $this open]
    }

    #
    # Build the debug dialog.
    #
    Rappture::Balloon $itk_component(hull).popup \
        -title "Secret Command Console" \
        -deactivatecommand [itcl::code $this activate off]
    set inner [$itk_component(hull).popup component inner]

    Rappture::Scroller $inner.area
    pack $inner.area -expand yes -fill both
    text $inner.area.text
    $inner.area contents $inner.area.text

    $inner.area.text tag configure error -foreground red \
        -font [option get $inner.area.text errorFont Font]
    $inner.area.text tag configure stderr -foreground red \
        -font [option get $inner.area.text errorFont Font]
    $inner.area.text tag configure stdout -foreground blue \
        -font [option get $inner.area.text errorFont Font]

    bind $inner.area.text <KeyPress> \
        [itcl::code $this command key]
    bind $inner.area.text <KeyPress-BackSpace> \
        [itcl::code $this command backspace]
    bind $inner.area.text <Control-KeyPress-h> \
        [itcl::code $this command backspace]

    bind $inner.area.text <KeyPress-Return> \
        [itcl::code $this command execute]
    bind $inner.area.text <KP_Enter> \
        [itcl::code $this command execute]

    bind $inner.area.text <KeyPress-Up> \
        "[itcl::code $this command previous]; break"
    bind $inner.area.text <Control-KeyPress-p> \
        "[itcl::code $this command previous]; break"

    bind $inner.area.text <KeyPress-Down> \
        "[itcl::code $this command next]; break"
    bind $inner.area.text <Control-KeyPress-n> \
        "[itcl::code $this command next]; break"

    command prompt

    eval itk_initialize $args

    # this makes it easier to find the magic spot
    bind $itk_component(hull) <Alt-Enter> [list $itk_component(hull) configure -background $itk_option(-activecolor)]
    bind $itk_component(hull) <Leave> [list $itk_component(hull) configure -background $itk_option(-background)]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::destructor {} {
    if {"" != $_afterid} {
        after cancel $_afterid
        set _afterid ""
    }
}

# ----------------------------------------------------------------------
# USAGE: active ?on|off?
#
# Used to query or set the activation state of this widget.  When
# the widget is "active", it changes color and accepts the magic
# pass phrase that will open up the debug panel.  A short delay after
# being activated, it deactivates itself.
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::activate {args} {
    if {[llength $args] == 0} {
        return $_active
    }

    if {"" != $_afterid} {
        after cancel $_afterid
        set _afterid ""
    }

    if {$args} {
        component hull configure -background $itk_option(-activecolor)
        set _focus [focus]
        focus $itk_component(hull)
        set _active 1
        set _afterid [after 3000 [itcl::code $this activate off]]
    } else {
        focus $_focus
        set _focus ""
        component hull configure -background $itk_option(-background)

        if {[info commands _tcl_puts] != ""} {
            # set puts back to normal
            rename ::puts ""
            rename ::_tcl_puts ::puts
        }
        set _active 0
    }
}

# ----------------------------------------------------------------------
# USAGE: open
#
# Used to open the debug area.  If the widget is active, then the
# debug area pops up near it.  Otherwise, this method does nothing.
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::open {} {
    if {$_active} {
        if {"" != $_afterid} {
            # don't deactivate until we close
            after cancel $_afterid
            set _afterid ""
        }

        $itk_component(hull).popup activate \
            $itk_component(hull) $itk_option(-popup)

        set text [$itk_component(hull).popup component inner].area.text
        focus $text

        # make puts send output to this display
        rename ::puts ::_tcl_puts
        proc ::puts {args} [format {%s _fake_puts $args} $this]
    }
}

itcl::body Rappture::Postern::close {} {
    $itk_component(hull).popup deactivate
}

# ----------------------------------------------------------------------
# USAGE: command prompt
# USAGE: command execute
# USAGE: command next
# USAGE: command previous
# USAGE: command key
#
# Used to handle various editing operations in the text area.
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::command {option} {
    set text [$itk_component(hull).popup component inner].area.text

    switch -- $option {
        prompt {
            if {[lindex [split [$text index end-1char] .] 1] != 0} {
                $text insert end "\n"
            }
            $text insert end "% "
            $text mark set command end-1char
            $text mark gravity command left
            $text mark set insert end
            $text see insert
        }
        key {
            if {[$text compare insert < command]} {
                $text mark set insert end
                $text see insert
            }
        }
        backspace {
            if {[catch {$text index sel.first}] == 0} {
                if {[$text compare sel.first < command]
                      || [$text compare sel.last < command]} {
                    $text tag remove sel 1.0 end
                }
            }
            if {[$text compare insert < command]} {
                $text mark set insert end
                $text see insert
            }
            if {[$text compare insert == command]} {
                return -code break  ;# don't erase past start of command
            }
        }
        execute {
            set cmd [string trim [$text get command end]]
            if {"" == $cmd} {
                command prompt
            } else {
                lappend _history $cmd
                if {[llength $_history] > 100} {
                    set _history [lrange $_history end-100 end]
                }
                set _hpos [llength $_history]

                $text insert end "\n"
                if {[catch {uplevel #0 $cmd} result]} {
                    $text insert end $result error
                } else {
                    $text insert end $result
                }
                command prompt
            }
            return -code break
        }
        next {
            if {$_hpos < [llength $_history]} {
                incr _hpos
                set cmd [lindex $_history $_hpos]
                $text delete command end
                $text insert command $cmd
                $text mark set insert end
                $text see insert
            }
        }
        previous {
            if {$_hpos > 0} {
                incr _hpos -1
                set cmd [lindex $_history $_hpos]
                $text delete command end
                $text insert command $cmd
                $text mark set insert end
                $text see insert
            }
        }
        default {
            error "bad option \"$option\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fake_puts {?-nonewline? ?<channel>? <string>}
#
# This method acts as a replacement for puts.  It sends output
# to the postern screen, itstead of stdin/stdout.
# ----------------------------------------------------------------------
itcl::body Rappture::Postern::_fake_puts {arglist} {
    Rappture::getopts arglist params {
        flag group -nonewline
    }
    switch -- [llength $arglist] {
        1 {
            set channel stdout
            set string [lindex $arglist 0]
        }
        2 {
            set channel [lindex $arglist 0]
            set string [lindex $arglist 1]
        }
        default {
            error "wrong # args: should be \"puts ?-nonewline? ?channel? string\""
        }
    }

    set text [$itk_component(hull).popup component inner].area.text
    if {$channel == "stdout" || $channel == "stderr"} {
        $text insert end $string $channel
        if {!$params(-nonewline)} {
            $text insert end "\n"
        }
    } else {
        eval _tcl_puts $arglist
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -size
# ----------------------------------------------------------------------
itcl::configbody Rappture::Postern::size {
    component hull configure \
        -width $itk_option(-size) \
        -height $itk_option(-size)
}
