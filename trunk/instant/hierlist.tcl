# ----------------------------------------------------------------------
#  COMPONENT: hierlist - hierarchical list of elements
#
#  This widget is similar to the BLT hierbox or treeview, but it
#  allows for embedded windows at each node.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2010  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *Hierlist.width 2i widgetDefault
option add *Hierlist.height 3i widgetDefault
option add *Hierlist.indent 20 widgetDefault
option add *Hierlist.icon {treepl treemn} widgetDefault
option add *Hierlist.selectBackground cyan widgetDefault
option add *Hierlist.dropLineColor blue widgetDefault
option add *Hierlist.font {helvetica -12} widgetDefault

itcl::class Rappture::Hierlist {
    inherit itk::Widget Rappture::Dragdrop

    itk_option define -indent indent Indent 10
    itk_option define -padding padding Padding 4
    itk_option define -icon icon Icon ""
    itk_option define -font font Font ""
    itk_option define -title title Title "%type: ~%id"
    itk_option define -selectbackground selectBackground Foreground ""
    itk_option define -droplinecolor dropLineColor Foreground ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method tree {option args}
    public method select {what {how "-notify"}}
    public method curselection {args}
    public method toggle {what}

    public method xview {args} { eval $itk_component(area) xview $args }
    public method yview {args} { eval $itk_component(area) yview $args }

    protected method _redraw {args}
    protected method _redrawChildren {node indent ypos}
    protected method _edit {option node field args}

    # define these for drag-n-drop support of items
    protected method dd_get_source {widget x y}
    protected method dd_scan_target {x y data}
    protected method dd_finalize {option args}

    private variable _dispatcher ""  ;# dispatcher for !events
    private variable _tree ""        ;# BLT tree object for node data
    private variable _current ""     ;# current selected node
    private variable _droppos ""     ;# node index for dragdrop target pos
    private variable _imh            ;# open/close images
}

itk::usual Hierlist {
    keep -cursor -font -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) off

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !redraw
    $_dispatcher dispatch $this !redraw [itcl::code $this _redraw]

    # create a root node
    set _tree [blt::tree create]
    $_tree set 0 terminal yes  ;# so you can't drop at the top level

    itk_component add area {
        canvas $itk_interior.area -highlightthickness 0
    } {
        usual
        ignore -borderwidth -relief
        ignore -highlightthickness -highlightbackground -highlightcolor
        keep -xscrollcommand -yscrollcommand
    }
    pack $itk_component(area) -expand yes -fill both

    # this widget exports nodes via drag-n-drop
    dragdrop source $itk_component(area)

    itk_component add editor {
        Rappture::Editor $itk_interior.editor
    }

    set _imh(open) [image create photo]
    set _imh(close) [image create photo]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::destructor {} {
    foreach name [array names _imh] {
        image delete $_imh($name)
    }
    blt::tree destroy $_tree
}

# ----------------------------------------------------------------------
# USAGE: tree insert <node> <pos> ?<field1> <val1> <field2> <val2> ...?
# USAGE: tree delete ?<node> <node> ...?
# USAGE: tree get <node> <field>
# USAGE: tree set <node> <field> <value>
# USAGE: tree path <node> ?<pattern>?
# USAGE: tree children <node>
#
# Clients use this to manipulate items in the tree being displayed
# within the widget.  Each <node> is an integer node number in the
# underlying BLT tree object.  The root node is node 0.  Each node
# has a series of <field>'s with values.  The fields are used for the
# -title of each node, and the "terminal" field controls whether or
# not a node can have children added.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::tree {option args} {
    switch -- $option {
        insert {
            set node [lindex $args 0]
            set pos [lindex $args 1]
            if {$pos == "end"} {set pos [llength [$_tree children $node]]}
            set args [lrange $args 2 end]
            set newnode [$_tree insert $node -at $pos -data [eval list open yes terminal yes dragdrop yes $args]]

            # make sure we fix up the layout at some point
            $_dispatcher event -idle !redraw

            return $newnode
        }
        delete {
            if {$args == "all"} {
                $_tree delete 0
            } else {
                foreach node $args {
                    $_tree delete $node
                }
            }

            # make sure we fix up the layout at some point
            $_dispatcher event -idle !redraw
        }
        get {
            return [eval $_tree get $args]
        }
        set {
            return [eval $_tree set $args]
        }
        path {
            if {[llength $args] < 1 || [llength $args] > 2} {
                error "wrong # args: should be \"tree path node ?pattern?\""
            }
            set node [lindex $args 0]
            set pattern [lindex $args 1]
            if {$pattern == ""} { set pattern %id }

            set path ""
            while {$node > 0} {
                catch {unset info}
                foreach {name val} [$_tree get $node] {
                    set info(%$name) $val
                    set info(%lc:$name) [string tolower $val]
                    set info(%uc:$name) [string toupper $val]
                }
                set name [string map [array get info] $pattern]
                set path "$name.$path"
                set node [$_tree parent $node]
            }
            return [string trimright $path .]
        }
        children {
            set node [lindex $args 0]
            return [$_tree children $node]
        }
        default {
            error "bad option \"$option\": should be insert, delete, get, set, path, children"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: select <what> ?-notify|-silent?
#
# Called when you click on an item to select it.  Highlights the item
# and returns information about it for the curselection.  This normally
# triggers a <<Selection>> event on the widget, but that is suppressed
# if the -silent option is given.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::select {what {how "-notify"}} {
    set c $itk_component(area)
    if {$what == "none"} {
        set what ""
    }
    set same [string equal $what $_current]

    if {"" != $_current} {
        # clear any current selection
        $c itemconfigure item:$_current-bg -fill ""
        set _current ""
    }

    if {"" != $what} {
        $c itemconfigure item:$what-bg -fill $itk_option(-selectbackground)
        set _current $what
    }

    if {!$same && ![string equal $how "-silent"]} {
        # selection changed? then tell clients
        event generate $itk_component(hull) <<Selection>>
    }
}

# ----------------------------------------------------------------------
# USAGE: curselection
# USAGE: curselection -path <string>
# USAGE: curselection -field <name>
#
# Returns information about the current selection.  Returns "" if
# there is no selection.  If "-field <name>" is specified, then it
# returns the value of that field for the current selection.  If
# the "-path <string>" is specified, then it returns the path to
# the current node, using <string> as a pattern for each node name
# and filling in any %field items for each node.  With no extra args,
# it returns the node number from the underlying BLT tree.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::curselection {args} {
    if {"" == $_current} {
        return ""
    }

    if {[llength $args] == 0} {
        return $_current

    } elseif {[llength $args] == 2} {
        switch -- [lindex $args 0] {
            -path {
                set pattern [lindex $args 1]
                return [tree path $_current $pattern]
            }
            -field {
                return [$_tree get $_current [lindex $args 1]]
            }
            default {
                error "bad option \"[lindex $args 0]\": should be -field, -path"
            }
        }
    } else {
        error "wrong # args: should be \"curselection ?-path str? ?-field name?\""
    }
}

# ----------------------------------------------------------------------
# USAGE: toggle <what>
#
# Called when you click on an item to select it.  Highlights the item
# and returns information about it for the curselection.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::toggle {what} {
    set flipped [expr {![$_tree get $what open]}]
    $_tree set $what open $flipped
    $_dispatcher event -idle !redraw
}

# ----------------------------------------------------------------------
# USAGE: _redraw ?<eventArgs>...?
#
# Used internally to redraw all items in the hierarchy after the
# tree has changed somehow.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::_redraw {args} {
    set c $itk_component(area)
    $c delete all

    set lineht [font metrics $itk_option(-font) -linespace]
    set ypos [expr {0.5*$lineht + $itk_option(-padding)}]
    _redrawChildren 0 0 ypos

    foreach {x0 y0 x1 y1} [$c bbox text] break
    $c configure -scrollregion [list 0 0 [expr {$x1+10}] [expr {$y1+10}]]

    # did a selected node get folded up when the parent closed?
    if {"" == [$c find withtag selected]} {
        select none
    }
}

# ----------------------------------------------------------------------
# USAGE: _redrawChildren <node> <indent> <yposVar>
#
# Used internally to redraw all items in the hierarchy after the
# tree has changed somehow.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::_redrawChildren {node indent yposVar} {
    set c $itk_component(area)
    upvar $yposVar ypos

    set xmid [expr {$indent + $itk_option(-indent)/2}]
    set xtxt [expr {$indent + $itk_option(-indent)}]
    set lineht [font metrics $itk_option(-font) -linespace]

    foreach n [$_tree children $node] {
        # background rectangle for selection highlight
        set tlist [list item:$n item:$n-bg]
        if {[string equal $n $_current]} {
            set bg $itk_option(-selectbackground)
            lappend tlist selected
        } else {
            set bg ""
        }
        $c create rect 0 [expr {$ypos-0.5*$lineht-1}] 1000 [expr {$ypos+0.5*$lineht+1}] -outline "" -fill $bg -tags $tlist

        $c bind item:$n <ButtonRelease> [itcl::code $this select $n]

        # +/- button for expanding the hierarchy
        if {![$_tree get $n terminal]} {
            if {[$_tree get $n open]} {
                set imh $_imh(open)
            } else {
                set imh $_imh(close)
            }
            $c create image $xmid $ypos -anchor c -image $imh -tags plus:$n
            $c bind plus:$n <ButtonPress> [itcl::code $this toggle $n]
        }

        # label for this node 
        catch {unset data}
        array set data [$_tree get $n]
        set str $itk_option(-title)
        set xpos $xtxt
        while {[regexp -indices {~?%(lc:|uc:)?([a-z]+)} $str match how field]} {
            foreach {s0 s1} $match break
            foreach {h0 h1} $how break
            foreach {f0 f1} $field break

            set pre [string range $str 0 [expr {$s0-1}]]
            if {[string length $pre] > 0} {
                $c create text $xpos $ypos -anchor w -text $pre -font $itk_option(-font) -tags [list text item:$n item:$n-text]
                set xwd [font measure $itk_option(-font) $pre]
                set xpos [expr {$xpos+$xwd}]
            }

            set editable 0
            set fname [string range $str $f0 $f1]
            if {[string index $str $s0] == "~"} {
                set editable 1
            }
            if {[info exists data($fname)]} {
                set how [string range $str $h0 $h1]
                switch -- $how {
                    lc: { set label [string tolower $data($fname)] }
                    uc: { set label [string toupper $data($fname)] }
                    default { set label $data($fname) }
                }

                # draw the editable part of the title string
                set id [$c create text $xpos $ypos -anchor w -text $label -font $itk_option(-font) -tags [list text item:$n item:$n-text item:$n-$fname]]
                if {$editable} {
                    $c bind $id <ButtonPress-1> [itcl::code $this _edit start $n $fname]
                }
                set xwd [font measure $itk_option(-font) $label]
                set xpos [expr {$xpos+$xwd}]
            }
            set str [string range $str [expr {$s1+1}] end]
        }

        # draw the last little bit on title string, if there is any
        if {[string length $str] > 0} {
            $c create text $xpos $ypos -anchor w -text $str -font $itk_option(-font) -tags [list text item:$n item:$n-text]
            set xwd [font measure $itk_option(-font) $str]
            set xpos [expr {$xpos+$xwd}]
        }

        set ypos [expr {$ypos+$lineht+$itk_option(-padding)}]

        # indent and draw all children
        if {![$_tree get $n terminal] && [$_tree get $n open]} {
            # if this node has children, draw them here
            _redrawChildren $n $xtxt ypos
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _edit start <node> <field>
# USAGE: _edit activate <node> <field>
# USAGE: _edit validate <node> <field> <value>
# USAGE: _edit apply <node> <field> <value>
#
# Used internally to handle the pop-up editor that edits part of
# the title string for a node.  The "start" operation is called when
# the user clicks on the title to edit it.  If the node is already
# selected, this brings up an editor for the field value, allowing the
# user to change the field.  The "activate" operation is called by
# the editor to figure out where it should pop up.  The "validate"
# operation is called to check the new value and make sure that it
# is okay.  The "apply" operation applies the value to the node.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::_edit {option n f args} {
    switch -- $option {
        start {
            if {[string equal $n $_current]} {
                $itk_component(editor) configure \
                    -activatecommand [itcl::code $this _edit activate $n $f] \
                    -validatecommand [itcl::code $this _edit validate $n $f] \
                    -applycommand [itcl::code $this _edit apply $n $f]
                $itk_component(editor) activate
            }
        }
        activate {
            set str [$_tree get $n $f]
            foreach {x0 y0 x1 y1} [$itk_component(area) bbox item:$n-$f] break
            set w [expr {$x1-$x0}]
            set h [expr {$y1-$y0}]
            set x0 [expr {[winfo rootx $itk_component(area)]+$x0}]
            set y0 [expr {[winfo rooty $itk_component(area)]+$y0}]

            return [list text $str x [expr {$x0-2}] y [expr {$y0-2}] w $w h $h]
        }
        validate {
            set val [lindex $args 0]
            if {![regexp {^[a-zA-Z0-9_]+$} $val]} {
                bell
                return 0
            }
            return 1
        }
        apply {
            set val [lindex $args 0]
            $_tree set $n $f $val
            $itk_component(area) itemconfigure item:$n-$f -text $val
        }
        default {
            error "bad option \"$option\": should be start, activate, validate, apply"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: dd_get_source <widget> <x> <y>
#
# Looks at the given <widget> and <x>,<y> coordinate to figure out
# what data value the source is exporting.  Returns a string that
# identifies the type of the data.  This string is passed along to
# targets via the dd_scan_target method.  If the target may check
# the source type and reject the data.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::dd_get_source {widget x y} {
    set data ""
    foreach id [$itk_component(area) find overlapping $x $y $x $y] {
        foreach tag [$itk_component(area) gettags $id] {
            # search for a tag like item:NNN and make sure it's selected
            if {[regexp {^item:([0-9]+)$} $tag match node]
                  && $node == $_current} {

                # some nodes have drag-n-drop turned off
                if {[$_tree get $node dragdrop]} {
                    set data [list node:$node [$_tree get $node]]
                }
                break
            }
        }
    }
    # return drag-n-drop data, if we found it
    return $data
}

# ----------------------------------------------------------------------
# USAGE: dd_scan_target <x> <y> <data>
#
# Looks at the given <x>,<y> coordinate and checks to see if the
# dragdrop <data> can be accepted at that point.  Returns 1 if so,
# and 0 if the data is rejected.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::dd_scan_target {x y data} {
    set c $itk_component(area)
    set _droppos ""  ;# assume no place to drop

    switch -glob -- $data {
        node:* {
            # convert from screen coords to canvas coords (for scrollbars)
            set x [$c canvasx $x]
            set y [$c canvasy $y]

            # search a little above the hot-spot
            set y [expr {$y-5}]

            foreach id [$c find overlapping $x $y $x $y] {
                foreach tag [$c gettags $id] {
                    if {[regexp {^item:([0-9]+)$} $tag match node]} {
                        # draw the dropline beneath the item found
                        if {"" == [$c find withtag dropline]} {
                            $c create line 0 0 0 0 \
                                -fill $itk_option(-droplinecolor) -tags dropline
                        }

                        foreach {x0 y0 x1 y1} [$c bbox item:$node-bg] break
                        if {![$_tree get $node terminal]} {
                            # drop on a node with children
                            if {$y > $y0 + 0.6*($y1-$y0)} {
                                # drop near the bottom -- insert as first child
                                set _droppos "$node -at 0"
                                set d [$_tree depth $node]
                                set x0 [expr {($d+1)*$itk_option(-indent)}]
                                $c coords dropline $x0 $y1 $x1 $y1
                            } else {
                                # drop on top -- insert as last child
                                set x0 [expr {$x0+1}]
                                set x1 [expr {[winfo width $c]-1}]
                                $c coords dropline $x0 $y0 $x1 $y0 $x1 $y1 $x0 $y1 $x0 $y0
                                set last [llength [$_tree children $node]]
                                set _droppos "$node -at $last"
                            }
                        } else {
                            set pnode [$_tree parent $node]
                            if {![$_tree get $pnode terminal]} {
                                set pos [expr {[$_tree position $node]+1}]
                                set _droppos "$pnode -at $pos"
                                set d [$_tree depth $node]
                                set x0 [expr {$d*$itk_option(-indent)}]
                                $c coords dropline $x0 $y1 $x1 $y1
                            }
                        }

                        break
                    }
                }
            }
            if {"" != $_droppos} {
                return 1
            }
            $c coords dropline -1 -1 -1 -1
            return 0
        }
    }
    return 0
}

# ----------------------------------------------------------------------
# USAGE: dd_finalize drop -op start|end -from <w> -to <w> \
#                           -x <x> -y <y> -data <data>
# USAGE: dd_finalize cancel
#
# Handles the end of a drag and drop operation.  The operation can be
# completed with a successful drop of data, or cancelled.
# ----------------------------------------------------------------------
itcl::body Rappture::Hierlist::dd_finalize {option args} {
    $itk_component(area) delete dropline

    if {$option == "drop" && "" != $_droppos} {
        array set params $args
        switch -glob -- $params(-data) {
            node:* {
                if {$params(-op) == "end"
                      && [string equal $params(-from) $params(-to)]} {

                    regexp {node:([0-9]+)} $params(-data) match node
                    eval $_tree move $node $_droppos
                } else {
                    set dlist [list open yes terminal yes dragdrop yes]
                    eval lappend dlist [lrange $params(-data) 1 end]
                    eval $_tree insert $_droppos -data [list $dlist]
                }
                # make sure the parent is open so we can see this node
                set parent [lindex $_droppos 0]
                if {![$_tree get $parent open]} {
                    $_tree set $parent open 1
                }

                $_dispatcher event -idle !redraw
                return 1
            }
        }
        return 0
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -icon
# ----------------------------------------------------------------------
itcl::configbody Rappture::Hierlist::icon {
    set imh0 [lindex $itk_option(-icon) 0]
    set imh1 [lindex $itk_option(-icon) 1]
    if {$imh1 == ""} { set imh1 $imh0 }

    set imh0 [Rappture::icon $imh0]
    if {$imh0 == ""} {
        $_imh(open) configure -width 1 -height 1
        $_imh(open) blank
    } else {
        $_imh(open) configure -width [image width $imh0] -height [image height $imh0]
        $_imh(open) copy $imh0
    }

    set imh1 [Rappture::icon $imh1]
    if {$imh1 == ""} {
        $_imh(close) configure -width 1 -height 1
        $_imh(close) blank
    } else {
        $_imh(close) configure -width [image width $imh1] -height [image height $imh1]
        $_imh(close) copy $imh1
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -selectbackground
# ----------------------------------------------------------------------
itcl::configbody Rappture::Hierlist::selectbackground {
    $itk_component(area) itemconfigure select \
        -background $itk_option(-selectbackground)
}
