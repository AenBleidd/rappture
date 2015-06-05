# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: FileListEntry - widget for entering a choice of strings
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

itk::usual BltTreeView {
    keep -foreground -cursor
}
itk::usual BltScrollset {
    #empty
}

itcl::class Rappture::FileListEntry {
    inherit itk::Widget

    itk_option define -state state State "normal"

    private variable _rebuildPending 0
    private variable _tree ""
    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this number
    private variable _icon ""

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
    protected method NewValue { args }
    protected method Tooltip {}
    protected method WhenIdle {}
    private method DoGlob { cwd patterns }
    private method Glob { pattern }
}

itk::usual FileListEntry {
    keep -cursor -font
    keep -foreground -background
    keep -textforeground -textbackground
    keep -selectbackground -selectforeground -selectborderwidth
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FileListEntry::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path
    set _tree [blt::tree create]

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    set label [$_owner xml get $_path.about.label]
    set desc [$_owner xml get $_path.about.description]
    itk_component add scrollset {
        blt::scrollset $itk_interior.ss \
            -xscrollbar $itk_interior.ss.xs \
            -yscrollbar $itk_interior.ss.ys \
            -window $itk_interior.ss.tree \
            -height 100
    }
    blt::tk::scrollbar $itk_interior.ss.xs
    blt::tk::scrollbar $itk_interior.ss.ys
    itk_component add tree {
        blt::treeview $itk_component(scrollset).tree -linewidth 0 \
            -alternatebackground "" \
            -bg white -selectmode multiple \
            -highlightthickness 0 \
            -tree $_tree \
            -flat yes -separator /  \
            -selectcommand [itcl::code $this NewValue]
    }
    $itk_component(tree) column configure "treeView" -justify left \
        -weight 1.0 -text "" -pad 0 -borderwidth 0 -edit no
    pack $itk_component(scrollset) -fill both -expand yes

    blt::table $itk_interior \
        0,0 $itk_component(scrollset) -fill both
    bind $itk_component(tree) <<Value>> [itcl::code $this NewValue]

    # Standard ButtonPress-1
    $itk_component(tree) bind Entry <ButtonPress-1> {
        Rappture::FileListEntry::SetSelectionAnchor %W current yes set
        set blt::TreeView::_private(scroll) 1
    }
    # Standard B1-Motion
    $itk_component(tree) bind Entry <B1-Motion> {
        set blt::TreeView::_private(x) %x
        set blt::TreeView::_private(y) %y
        set index [%W nearest %x %y]
        Rappture::FileListEntry::SetSelectionAnchor %W $index yes set
    }
    # Standard ButtonRelease-1
    $itk_component(tree) button bind all <ButtonRelease-1> {
        set index [%W nearest %x %y blt::TreeView::_private(who)]
        if { [%W index current] == $index &&
             $blt::TreeView::_private(who) == "button" } {
            %W see -anchor nw current
            %W toggle current
        }
    }
    # Shift-ButtonPress-1
    $itk_component(tree) bind Entry <Shift-ButtonPress-1> {
        Rappture::FileListEntry::SetSelectionAnchor %W current yes set
        set blt::TreeView::_private(scroll) 1
    }
    # Shift-B1-Motion
    $itk_component(tree) bind Entry <Shift-B1-Motion> {
        set blt::TreeView::_private(x) %x
        set blt::TreeView::_private(y) %y
        set index [%W nearest %x %y]
        if { [%W cget -selectmode] == "multiple" } {
            %W selection mark $index
        } else {
            Rappture::FileListEntry::SetSelectionAnchor %W $index yes set
        }
    }
    # Shift-ButtonRelease-1
    $itk_component(tree) bind Entry <Shift-ButtonRelease-1> {
        if { [%W cget -selectmode] == "multiple" } {
            %W selection anchor current
        }
        after cancel $blt::TreeView::_private(afterId)
        set blt::TreeView::_private(afterId) -1
        set blt::TreeView::_private(scroll) 0
    }
    $itk_component(tree) bind Entry <Control-ButtonPress-1> {
        Rappture::FileListEntry::SetSelectionAnchor %W current no toggle
        set blt::TreeView::_private(scroll) 1
    }
    $itk_component(tree) bind Entry <Control-B1-Motion> {
        set blt::TreeView::_private(x) %x
        set blt::TreeView::_private(y) %y
        set index [%W nearest %x %y]
        if { [%W cget -selectmode] == "multiple" } {
            %W selection mark $index
        } else {
            Rappture::FileListEntry::SetSelectionAnchor %W $index no toggle
        }
    }
    $itk_component(tree) bind Entry <Control-ButtonRelease-1> {
        if { [%W cget -selectmode] == "multiple" } {
            %W selection anchor current
        }
        after cancel $blt::TreeView::_private(afterId)
        set blt::TreeView::_private(afterId) -1
        set blt::TreeView::_private(scroll) 0
    }
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

    # if the control has an icon, plug it in
    set str [$_owner xml get $path.about.icon]
    if {$str != ""} {
        set _icon [image create picture -data $str]
    }
    Rebuild
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::FileListEntry::destructor {} {
    blt::tree destroy $_tree
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
itcl::body Rappture::FileListEntry::value {args} {
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
        foreach id [$itk_component(tree) curselection] {
            set path [$_tree get $id "path" ""]
            set path2id($path) $id
        }
        set paths [split $newval ,]
    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    set list {}
    foreach id [$itk_component(tree) curselection] {
        set path [$_tree get $id "path" ""]
        if { $path != "" } {
            lappend list $path
        }
    }
    return [join $list ,]
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::FileListEntry::label {} {
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
itcl::body Rappture::FileListEntry::tooltip {} {
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
itcl::body Rappture::FileListEntry::Rebuild {} {
    set _rebuildPending 0

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
    eval $_tree tag add unused [$_tree children root]
    foreach file $allfiles {
        set tail [file tail $file]
        if { $first == "" } {
            set first $tail
        }
        set tail [file root $tail]
        set id [$_tree index root->"$tail"]
        if { $id < 0 } {
            set data [list path $file show 0]
            set id [$_tree insert root -label $tail -data $data -tag ""]
        } else {
            $_tree tag delete unused $id
        }
        set len [string length $tail]
        if {$len > $max} { set max $len }
    }
    $itk_component(tree) configure -icons ""
    $itk_component(tree) entry configure all -icons ""
    eval $_tree delete [$_tree tag nodes unused]
    $itk_component(tree) configure -width $max
    catch {
        if { ![$itk_component(tree) selection present] } {
            $itk_component(tree) selection set [$_tree firstchild root]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: NewValue
#
# Invoked automatically whenever the value in the choice changes.
# Sends a <<Value>> event to notify clients of the change.
# ----------------------------------------------------------------------
itcl::body Rappture::FileListEntry::NewValue { args } {
    event generate $itk_component(hull) <<Value>>
}

# ----------------------------------------------------------------------
# USAGE: Tooltip
#
# Returns the tooltip for this widget, given the current choice in
# the selector.  This is normally called by the Rappture::Tooltip
# facility whenever it is about to pop up a tooltip for this widget.
# ----------------------------------------------------------------------
itcl::body Rappture::FileListEntry::Tooltip {} {
    set tip [string trim [$_owner xml get $_path.about.description]]
    # get the description for the current choice, if there is one
    set path ""
    set desc ""
    if {$path == ""} {
        set desc [$_owner xml get $_path.about.description]
    }
    set str ""
    if {[string length $str] > 0 && [string length $desc] > 0} {
        append tip "\n\n$str:\n$desc"
    }
    return $tip
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -state
# ----------------------------------------------------------------------
itcl::configbody Rappture::FileListEntry::state {
    set valid {normal disabled}
    if {[lsearch -exact $valid $itk_option(-state)] < 0} {
        error "bad value \"$itk_option(-state)\": should be [join $valid {, }]"
    }
    #$itk_component(tree) configure -state $itk_option(-state)
}

itcl::body Rappture::FileListEntry::WhenIdle {} {
    if { !$_rebuildPending } {
        after 10 [itcl::code $this Rebuild]
        set _rebuildPending 1
    }
}

proc Rappture::FileListEntry::SetSelectionAnchor { w tagOrId clear how } {
    set index [$w index $tagOrId]
    if { $clear } {
        $w selection clearall
    }
    $w see $index
    $w focus $index
    $w selection $how $index
    $w selection anchor $index
}

itcl::body Rappture::FileListEntry::DoGlob { cwd patterns } {
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
    return $files
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
itcl::body Rappture::FileListEntry::Glob { pattern } {
    return [DoGlob "" [file split $pattern]]
}

