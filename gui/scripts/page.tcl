# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: page - single page of widgets
#
#  This widget is a smart frame.  It takes the XML description for
#  a Rappture <input> or an <input><phase> and decides how to lay
#  out the widgets for the controls within it.  It uses various
#  heuristics to achieve a decent layout under a variety of
#  circumstances.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::Page {
    inherit itk::Widget

    constructor {owner path args} { # defined below }

    protected method _buildGroup {frame xmlobj path}
    protected method _link {xmlobj path widget path2}

    private variable _owner ""       ;# thing managing this page
}

itk::usual Page {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Page::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] || !$valid} {
        error "object \"$owner\" is not a Rappture::ControlOwner"
    }
    set _owner $owner
    set xmlobj [$owner xml object]

    set type [$xmlobj element -as type $path]
    if {$type != "input" && $type != "phase"} {
        error "bad path \"$path\" in $xmlobj: should be <input> or <input><phase>"
    }

    eval itk_initialize $args

    # build all of the controls for this page
    _buildGroup $itk_interior $xmlobj $path
}

# ----------------------------------------------------------------------
# USAGE: _buildGroup <frame> <xmlobj> <path>
#
# Used internally when this page is being constructed to build the
# controls within the group at the specified <path> in the <xmlobj>.
# The controls are added to the given <frame>.
# ----------------------------------------------------------------------
itcl::body Rappture::Page::_buildGroup {frame xmlobj path} {
    frame $frame.results
    pack $frame.results -side right -fill y

    set deveditor ""

    #
    # Scan through all remaining input elements.  If there is an
    # ambient group, then add its children to the device editor,
    # if there is one.
    #
    set num 0
    set clist [$xmlobj children $path]
    while {[llength $clist] > 0} {
        set cname [lindex $clist 0]
        set clist [lrange $clist 1 end]

        set type [$xmlobj element -as type $path.$cname]
        if {$type == "about"} {
            continue
        }
        if {$type == "loader"} {
            #
            # Add <loader>'s at the top of the page.
            #
            if {![winfo exists $frame.loaders]} {
                frame $frame.loaders
                pack $frame.loaders -side top -fill x

                frame $frame.loaders.sep -height 2 \
                    -borderwidth 1 -relief sunken
                pack $frame.loaders.sep -side bottom -fill x -pady 4
            }
            set w "$frame.loaders.l[incr num]"
            Rappture::Controls $w $_owner
            pack $w -fill x
            $w insert end $path.$cname
        } elseif {$type == "structure"} {
            #
            # Add <structure>'s as the central element of the page.
            #
            set w "$frame.device[incr num]"
            Rappture::DeviceEditor ::$w $_owner@$path.$cname.current
            pack $w -expand yes -fill both
            $_owner widgetfor $path.$cname $w
            bind $w <<Value>> [list $_owner changed $path.$cname]

            if {"" == $deveditor} {
                set deveditor $w
            }

            # if there's a default value, load it now
            if {"" != [$xmlobj element -as type $path.$cname.current]} {
                set elem $path.$cname.current
            } else {
                set elem $path.$cname.default
            }
            if {"" != [$xmlobj element -as type $elem]} {
                set val [$xmlobj get $elem]
                if {[string length $val] > 0} {
                    $w value $val
                    $xmlobj put $path.$cname.current $val
                } else {
                    set obj [$xmlobj element -as object $elem]
                    $w value $obj
                    $xmlobj put $path.$cname.current $obj
                }
            }

            # if there's a link, then set up a callback to load from it
            set link [$xmlobj get $path.$cname.link]
            if {"" != $link} {
                $_owner notify add $this $link \
                    [itcl::code $this _link $xmlobj $link $w $path.$cname]
            }
        } elseif {$type == "tool"} {
            set service [Rappture::Service ::#auto $_owner $path.$cname]
            #
            # Scan through all extra inputs associated with this subtool
            # and create corresponding inputs in the top-level tool.
            # Then, add the input names to the list being processed here,
            # so that we'll create the controls during subsequent passes
            # through the loop.
            #
            set extra ""
            foreach obj [$service input] {
                set cname [$obj element]
                $xmlobj copy $path.$cname from $obj ""
                lappend extra $cname
            }

            #
            # If there's a control for this service, then add it
            # to the end of the extra controls added above.
            #
            foreach obj [$service control] {
                set cname [$obj element]
                $xmlobj copy $path.$cname from $obj ""
                $xmlobj put $path.$cname.service $service
                lappend extra $cname
            }
            if {[llength $extra] > 0} {
                set clist [eval linsert [list $clist] 0 $extra]
            }

            #
            # Scan through all outputs associated with this subtool
            # and create any corresponding feedback widgets.
            #
            foreach obj [$service output] {
                set cname [$obj element]
                $xmlobj copy $cname from $obj ""

                # pick a good size based on output type
                set w $frame.results.result[incr num]
                set type [$obj element -as type]
                switch -- $type {
                    number - integer - boolean - choice {
                        Rappture::ResultViewer $w -width 0 -height 0
                        pack $w -fill x -padx 4 -pady 4
                    }
                    default {
                        Rappture::ResultViewer $w -width 4i -height 4i
                        pack $w -expand yes -fill both -padx 4 -pady 4
                    }
                }
                $service output for $obj $w
            }
        } elseif {$type == "current"} {
            # Don't do anything.
        } else {
            # create a control panel, if necessary
            if {![winfo exists $frame.cntls]} {
                Rappture::Controls $frame.cntls $_owner \
                    -layout [$xmlobj get $path.about.layout]
                pack $frame.cntls -expand yes -fill both -pady 4
            }

            # if this is a group, then build that group
            if {[$xmlobj element -as type $path.$cname] eq "group"} {
                if {[$xmlobj element -as id $path.$cname] eq "ambient"
                       && $deveditor != ""} {
                    set w [$deveditor component top]
                } else {
                    if {[$_owner widgetfor $path.$cname] ne ""} {
                        # widget already created -- skip this
                    } elseif {[catch {$frame.cntls insert end $path.$cname} c]} {
                        global errorInfo
                        error $c "$c\n$errorInfo\n    (while building control for $path.$cname)"
                    } else {
                        set gentry [$frame.cntls control $c]
                        set w [$gentry component inner]
                    }
                }
                _buildGroup $w $xmlobj $path.$cname
            } else {
                if {[$_owner widgetfor $path.$cname] ne ""} {
                    # widget already created -- skip this
                } elseif {[catch {$frame.cntls insert end $path.$cname} c]} {
                    global errorInfo
                    error $c "$c\n$errorInfo\n    (while building control for $path.$cname)"
                }
            }
        }
    }
}

itcl::body Rappture::Page::_link {xmlobj path w path2} {
    if {"" != [$xmlobj element -as type $path.current]} {
        set val [$xmlobj get $path.current]
        if {[string length $val] > 0} {
            $w value $val
            $xmlobj put $path.current $val
        } else {
            set obj [$xmlobj element -as object $path.current]
            $w value $obj
            $xmlobj put $path.current $obj
        }
    }
    $_owner changed $path2
}
