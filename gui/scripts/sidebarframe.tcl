# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: SidebarFrame - pop-out controls for visualization widgets
#
#  The sidebar provides a way to put a thin strip of controls along the
#  side of a visualization widget, with tabs that cause control panels
#  to pop out.  The SidebarFrame has an empty frame (component "frame")
#  on the left and a sidebar that pops out on the right.
# ======================================================================
#  AUTHOR:  George Howlett, Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

option add *SidebarFrame.width 3i widgetDefault
option add *SidebarFrame.height 3i widgetDefault
option add *SidebarFrame.titlebarBackground #6666cc widgetDefault
option add *SidebarFrame.titlebarForeground white widgetDefault
option add *SidebarFrame.controlBackground gray widgetDefault
option add *SidebarFrame*cntls*highlightBackground gray widgetDefault
option add *SidebarFrame.sashRelief flat widgetDefault
option add *SidebarFrame.sashActiveRelief solid widgetDefault
option add *SidebarFrame.sashColor gray widgetDefault
option add *SidebarFrame.sashWidth 2 widgetDefault
option add *SidebarFrame.sashPadding 2 widgetDefault
option add *SidebarFrame.sashCursor sb_h_double_arrow

itcl::class Rappture::SidebarFrame {
    inherit itk::Widget

    itk_option define -sashrelief sashRelief Relief ""
    itk_option define -sashactiverelief sashActiveRelief SashActiveRelief ""
    itk_option define -sashcolor sashColor SashColor ""
    itk_option define -sashwidth sashWidth SashWidth 0
    itk_option define -sashpadding sashPadding SashPadding 0
    itk_option define -sashcursor sashCursor Cursor ""

    public variable resizeframe 1
    constructor {args} { # defined below }

    public method insert {pos args}
    public method exists {which}
    public method panel {which}
    public method select {which}
    public method pop {what}
    public method width { size } {
        set _width $size
    }
    public method enable { which }
    public method disable { which }

    protected method _toggleTab {which}
    protected method _sash {op x}
    protected method _fixLayout {args}
    protected method TabIndex { which }

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _state "closed" ;# sidebar open/closed
    private variable _panels         ;# maps panel => title, etc.
    private variable _selected ""    ;# selected panel
    private variable _width "auto"   ;# width adjusted by sash or "auto"
    private variable _counter 0      ;# counter for auto-generated names
    private variable _title2panel
}

itk::usual SidebarFrame {
    keep -background -cursor
    keep -titlebarbackground -titlebarforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::constructor {args} {
    itk_option add hull.width hull.height

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !layout
    $_dispatcher dispatch $this !layout [itcl::code $this _fixLayout]

    # fix the layout whenever the window size changes
    bind SidebarFrame <Configure> [itcl::code %W _fixLayout]

    #
    # Empty frame for main widget
    #
    itk_component add frame {
        frame $itk_interior.f
    }

    #
    # Sash along the left side of sidebar
    #
    itk_component add sashbg {
        frame $itk_interior.sashbg
    } {
        usual
        rename -cursor -sashcursor sashCursor Cursor
    }

    itk_component add sash {
        frame $itk_component(sashbg).sash -borderwidth 1
    } {
        usual
        ignore -background -borderwidth
        rename -relief -sashrelief sashRelief Relief
        rename -width -sashwidth sashWidth SashWidth
        rename -cursor -sashcursor sashCursor Cursor
    }
    pack $itk_component(sash) -side left -fill y

    foreach c {sash sashbg} {
        bind $itk_component($c) <Enter> \
            [itcl::code $this _sash enter %X]
        bind $itk_component($c) <Leave> \
            [itcl::code $this _sash leave %X]
        bind $itk_component($c) <ButtonPress-1> \
            [itcl::code $this _sash grab %X]
        bind $itk_component($c) <B1-Motion> \
            [itcl::code $this _sash drag %X]
        bind $itk_component($c) <ButtonRelease-1> \
            [itcl::code $this _sash release %X]
    }

    itk_component add sidebar {
        frame $itk_interior.sbar
    }

    #
    # Title bar along top of sidebar
    #
    itk_component add titlebar {
        frame $itk_component(sidebar).tbar -highlightthickness 0
    } {
        usual
        ignore -highlightthickness
        rename -background -titlebarbackground titlebarBackground Background
    }
    pack $itk_component(titlebar) -side top -fill x

    itk_component add popbutton {
        button $itk_component(titlebar).popb \
            -borderwidth 1 -relief flat -overrelief raised \
            -highlightthickness 0 \
            -image [Rappture::icon sbar-open] \
            -command [itcl::code $this pop toggle]
    } {
        usual
        ignore -borderwidth -relief -overrelief -highlightthickness
        rename -background -titlebarbackground titlebarBackground Background
        rename -activebackground -titlebarbackground titlebarBackground Background
    }
    pack $itk_component(popbutton) -side left -padx 6 -pady 2
    Rappture::Tooltip::for $itk_component(popbutton) \
        "Open/close the sidebar"

    itk_component add title {
        label $itk_component(titlebar).title -anchor w -font "Arial 10"
    } {
        usual
        ignore -font
        rename -foreground -titlebarforeground titlebarForeground Foreground
        rename -background -titlebarbackground titlebarBackground Background
    }
    pack $itk_component(title) -side left -expand yes -fill both -padx 1 -pady 1

    #
    # Area for active panel
    #
    itk_component add area {
        Rappture::Scroller $itk_component(sidebar).area \
            -xscrollmode auto -yscrollmode auto \
            -highlightthickness 0
    }
    $itk_component(area) contents frame

    itk_component add controlbar {
        frame $itk_component(sidebar).cbar
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controlbar) -side left -fill y

    #
    # Control area above the tabs
    #
    itk_component add controls {
        frame $itk_component(controlbar).cntls -height 20
    } {
        usual
        rename -background -controlbackground controlBackground Background
    }
    pack $itk_component(controls) -side top -pady {8 20}

    #
    # Tabs used to select sidebar panels.
    #
    # Note:  Bugs in BLT 2.4 tabset/VNC server crashes the server
    #        when -outerpad is set to 0.
    #
    itk_component add tabs {
        blt::tabset $itk_component(controlbar).tabs \
            -highlightthickness 0 -tearoff 0 -side left \
            -bd 0 -gap 0 -tabborderwidth 1 \
            -outerpad 1
    } {
        keep -background -cursor
        ignore -highlightthickness -borderwidth
        rename -highlightbackground -controlbackground controlBackground \
            Background
        rename -background -controlbackground controlBackground \
            Background
    }
    pack $itk_component(tabs) -side top -expand yes -anchor e -padx {4 0} -fill y

    eval itk_initialize $args

    # make sure we fix up the layout at some point
    $_dispatcher event -idle !layout
}

# ----------------------------------------------------------------------
# USAGE: insert <pos> ?-title t? ?-icon i?
#
# Adds a new panel into this widget at the given position <pos>.  The
# panel has a tab with the specified -icon, and is labeled by the
# -title string in the titlebar area when it is selected.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::insert {pos args} {
    Rappture::getopts args panel "
        value -title Options
        value -icon [Rappture::icon cboff]
    "
    if {[llength $args] > 0} {
        error "wrong # args: should be \"insert pos ?-title t? ?-icon i?\""
    }

    set f [$itk_component(area) contents]
    set pname "panel[incr _counter]"
    itk_component add $pname {
        frame $f.$pname
    }

    $itk_component(tabs) insert $pos $pname \
        -image $panel(-icon) -text "" -padx 0 -pady 0 \
        -command [itcl::code $this _toggleTab $pname]

    set _title2panel($panel(-title)) $pname
    Rappture::Tooltip::text $itk_component(tabs)-$pname \
        "Open/close sidebar for $panel(-title)"
    $itk_component(tabs) bind $pname <Enter> \
        [list ::Rappture::Tooltip::tooltip pending %W-$pname @%X,%Y]
    $itk_component(tabs) bind $pname <Leave> \
        [list ::Rappture::Tooltip::tooltip cancel]
    $itk_component(tabs) bind $pname <ButtonPress> \
        [list ::Rappture::Tooltip::tooltip cancel]
    $itk_component(tabs) bind $pname <KeyPress> \
        [list ::Rappture::Tooltip::tooltip cancel]

    set _panels($pname-title) $panel(-title)
    if { ![info exists _panels(all)] || $pos == "end" } {
        lappend _panels(all) $pname
    } else {
        set _panels(all) [linsert $_panels(all) $pos $pname]
    }
    if {$_selected == ""} {
        set _selected $pname
        if {$_state == "open"} {
            $itk_component(title) configure -text $panel(-title)
        }
    }

    return $itk_component($pname)
}

# ----------------------------------------------------------------------
# USAGE: panel <which>
#
# Returns the frame representing the requested panel.  The <which>
# argument can be a panel index, name, or title, or the keyword
# "current" for the selected panel.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::panel {which} {
    switch -glob -- $which {
        current {
            return $itk_component($_selected)
        }
        [0-9]* {
            set pname [lindex $_panels(all) $which]
            return $itk_component($pname)
        }
        panel[0-9]* {
            if {[info exists itk_component($which)]} {
                return $itk_component($which)
            }
            error "bad panel name \"$which\""
        }
        default {
            foreach pname $_panels(all) {
                if {[string equal $_panels($pname-title) $which]} {
                    return $itk_component($pname)
                }
            }
            error "bad panel title \"$which\""
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: exists <which>
#
# Returns the frame representing the requested panel.  The <which>
# argument can be a panel index, name, or title, or the keyword
# "current" for the selected panel.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::exists { title } {
    if { [info exists _title2panel($title)] } {
        return 1
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: select <which>
#
# Pops open the sidebar and selects the specified panel.  The <which>
# argument can be a panel index, name, or title.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::select {which} {
    set pname ""
    switch -glob -- $which {
        [0-9]* {
            set pname [lindex $_panels(all) $which]
        }
        panel[0-9]* {
            if {[info exists itk_component($which)]} {
                set pname $which
            }
        }
        default {
            foreach p $_panels(all) {
                if {[string equal $_panels($p-title) $which]} {
                    set pname $p
                    break
                }
            }
        }
    }
    if {$pname == ""} {
        error "bad panel name \"$which\": should be panel id, title, or index"
    }

    if {$_state == "closed"} {
        pop open
    }

    set minw [winfo reqwidth $itk_component(controlbar)]
    if {$_width != "auto" && $_width < $minw+50} {
        set _width [expr {$minw+50}]
        $_dispatcher event -idle !layout
    }
    set n [$itk_component(tabs) index -name $pname]
    $itk_component(tabs) select $n

    $itk_component(title) configure -text $_panels($pname-title)

    set f [$itk_component(area) contents]
    foreach w [pack slaves $f] {
        pack forget $w
    }
    pack $itk_component($pname) -expand yes -fill both

    #
    # HACK ALERT!  Force the scroller to check the size of the
    # panel that we just slid in under the covers.  Make it
    # think the panel and the scroller itself have changed size.
    #
    event generate [winfo parent $f] <Configure>
    event generate $f <Configure>

    set _selected $pname
    return $pname
}

# ----------------------------------------------------------------------
# USAGE: pop open|close|toggle
#
# Used to open/close the sidebar area.  When open, the selected panel
# appears and the titlebar shows its name.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::pop {how} {
    if {$how == "toggle"} {
        if {$_state == "closed"} {
            set how "open"
        } else {
            set how "close"
        }
    }

    switch -- $how {
        open {
            $itk_component(popbutton) configure \
                -image [Rappture::icon sbar-closed]
            pack $itk_component(area) -side right -expand yes -fill both

            set _state "open"
            select $_selected
            $_dispatcher event -idle !layout
        }
        close {
            $itk_component(popbutton) configure \
                -image [Rappture::icon sbar-open]
            $itk_component(title) configure -text ""
            pack forget $itk_component(area)

            set _state "closed"
            $_dispatcher event -idle !layout
        }
        default {
            error "bad option \"$how\": should be open, close, toggle"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: disable <which>
#
# Pops open the sidebar and selects the specified panel.  The <which>
# argument can be a panel index, name, or title.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::disable {which} {
    set index [TabIndex $which]
    set tab [$itk_component(tabs) get $index]
    $itk_component(tabs) tab configure $tab -state disabled
}


# ----------------------------------------------------------------------
# USAGE: enable <which>
#
# Pops open the sidebar and selects the specified panel.  The <which>
# argument can be a panel index, name, or title.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::enable {which} {
    set index [TabIndex $which]
    set tab [$itk_component(tabs) get $index]
    $itk_component(tabs) tab configure $tab -state normal
}

# ----------------------------------------------------------------------
# USAGE: TabIndex <which>
#
# Pops open the sidebar and selects the specified panel.  The <which>
# argument can be a panel index, name, or title.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::TabIndex {which} {
    set pname ""
    switch -glob -- $which {
        [0-9]* {
            set pname [lindex $_panels(all) $which]
        }
        panel[0-9]* {
            if {[info exists itk_component($which)]} {
                set pname $which
            }
        }
        default {
            foreach p $_panels(all) {
                if {[string equal $_panels($p-title) $which]} {
                    set pname $p
                    break
                }
            }
        }
    }
    if {$pname == ""} {
        error "bad panel name \"$which\": should be panel id, title, or index"
    }
    set n [$itk_component(tabs) index -name $pname]
    return $n
}

# ----------------------------------------------------------------------
# USAGE: _toggleTab <which>
#
# Invoked automatically when the user clicks on a tab for the sidebar.
# If the sidebar is closed, it is automatically opened and the tab is
# selected.  If the sidebar is opened, then it's closed.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::_toggleTab {which} {
    if {$_state == "closed"} {
        pop open
        select $which
    } elseif {[$itk_component(tabs) index -name $_selected]
          == [$itk_component(tabs) index -name $which]} {
        pop close
    } else {
        select $which
    }
}

# ----------------------------------------------------------------------
# USAGE: _sash <op> <X>
#
# Invoked automatically when the user clicks/drags on a sash, to resize
# the sidebar.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::_sash {op X} {
    switch -- $op {
        enter {
            # mouse over sash -- make it active
            if {$itk_option(-sashactiverelief) != ""} {
                $itk_component(sash) configure -relief $itk_option(-sashactiverelief)
            }
        }
        leave {
            # mouse left sash -- back to normal
            $itk_component(sash) configure -relief $itk_option(-sashrelief)
        }
        grab {
            if {$_state == "closed"} { pop open }
            _sash drag $X
        }
        drag {
            set w [winfo width $itk_component(hull)]
            set minw [winfo reqwidth $itk_component(controlbar)]
            set dx [expr {$X - [winfo rootx $itk_component(hull)]}]
            set sashw [winfo reqwidth $itk_component(sashbg)]
            set _width [expr {$w - $dx - $sashw/2}]

            if {$_width < $minw} { set _width $minw }
            if {$_width > $w-50} { set _width [expr {$w-50}] }
            _fixLayout
        }
        release {
            set minw [winfo reqwidth $itk_component(controlbar)]
            if {$_width-$minw < 40} {
                set _width "auto"
                pop close
            }
        }
        default {
            error "bad option \"$op\": should be enter, leave, grab, drag, release"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixLayout ?<eventArgs>...?
#
# Used internally to update the layout of panes whenever a new pane
# is added or a sash is moved.
# ----------------------------------------------------------------------
itcl::body Rappture::SidebarFrame::_fixLayout {args} {
    set w [winfo width $itk_component(hull)]
    set h [winfo height $itk_component(hull)]

    set sashw [winfo reqwidth $itk_component(sashbg)]

    set tabw [winfo reqwidth $itk_component(tabs)]
    set btnw [winfo reqwidth $itk_component(controls)]
    set ctrlw [expr {($tabw > $btnw) ? $tabw : $btnw}]

    if {$_state == "closed"} {
        set sbarw $ctrlw
    } else {
        if {$_width == "auto"} {
            # pop open to the size of the widest pane
            set sbarw 0
            foreach pname $_panels(all) {
                set pw [winfo reqwidth $itk_component($pname)]
                if {$pw > $sbarw} {
                    set sbarw $pw
                }
            }
            set sbarw [expr {$sbarw + $ctrlw + $sashw}]
        } else {
            set sbarw $_width
        }
    }

    # don't let the sidebar take up too much of the window area
    if {$sbarw > 0.75*$w} {
        set sbarw [expr {int(0.75*$w)}]
    }

    set x1 [expr {$w - $sbarw - $sashw}]
    set x2 [expr {$w - $sbarw}]
    if { $resizeframe } {
        set framew $x1
    } else {
        set framew [expr $w - $ctrlw - $sashw]
    }
    place $itk_component(frame) -x 0 -y 0 -anchor nw -width $framew -height $h
    place $itk_component(sashbg) -x $x1 -y 0 -anchor nw -width $sashw -height $h
    place $itk_component(sidebar) -x $x2 -y 0 -anchor nw \
        -width $sbarw -height $h
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -sashpadding
# ----------------------------------------------------------------------
itcl::configbody Rappture::SidebarFrame::sashpadding {
    pack $itk_component(sash) -padx $itk_option(-sashpadding)
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -sashcolor
# ----------------------------------------------------------------------
itcl::configbody Rappture::SidebarFrame::sashcolor {
    if {$itk_option(-sashcolor) != ""} {
        $itk_component(sash) configure -background $itk_option(-sashcolor)
    } else {
        $itk_component(sash) configure -background $itk_option(-background)
    }
}

