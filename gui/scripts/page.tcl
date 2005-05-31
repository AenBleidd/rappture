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
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

itcl::class Rappture::Page {
    inherit itk::Widget

    constructor {tool path args} { # defined below }

    protected method _buildGroup {frame xmlobj path}

    private variable _tool ""        ;# tool controlling this page
}
                                                                                
itk::usual Page {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Page::constructor {tool path args} {
    if {[catch {$tool isa Rappture::Tool} valid] || !$valid} {
        error "object \"$tool\" is not a Rappture Tool"
    }
    set _tool $tool
    set xmlobj [$tool xml object]

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
    set deveditor ""

    #
    # Scan through all input elements in this group and look
    # for a <loader>.  Add those first, at the top of the group.
    #
    set num 0
    foreach cname [$xmlobj children $path] {
        if {[$xmlobj element -as type $path.$cname] == "loader"} {
            if {![winfo exists $frame.loaders]} {
                frame $frame.loaders
                pack $frame.loaders -side top -fill x

                frame $frame.loaders.sep -height 2 \
                    -borderwidth 1 -relief sunken
                pack $frame.loaders.sep -side bottom -fill x -pady 4
            }
            set w "$frame.loaders.l[incr num]"
            Rappture::Controls $w $_tool
            pack $w -fill x
            $w insert end $xmlobj $path.$cname
        }
    }

    #
    # Scan through all input elements and look for any top-level
    # <structure> elements.  Create these next.
    #
    set num 0
    foreach cname [$xmlobj children $path] {
        if {[$xmlobj element -as type $path.$cname] == "structure"} {
            set w "$frame.device[incr num]"
            Rappture::DeviceEditor $w $_tool
            pack $w -expand yes -fill both
            $_tool widgetfor $path.$cname $w

            if {"" == $deveditor} {
                set deveditor $w
            }

            # if there's a default value, load it now
            if {"" != [$xmlobj element -as type $path.$cname.default]} {
                set val [$xmlobj get $path.$cname.default]
                if {[string length $val] > 0} {
                    $w value $val
                } else {
                    set obj [$xmlobj element -as object $path.$cname.default]
                    $w value $obj
                }
            }
        }
    }

    #
    # Scan through all remaining input elements.  If there is an
    # ambient group, then add its children to the device editor,
    # if there is one.
    #
    foreach cname [$xmlobj children $path] {
        if {[string match "about*" $cname]} {
            continue
        }

        if {[$_tool widgetfor $path.$cname] == ""} {
            # create a control panel, if necessary
            if {![winfo exists $frame.cntls]} {
                Rappture::Controls $frame.cntls $_tool
                pack $frame.cntls -fill x
            }

            # if this is a group, then build that group
            if {[$xmlobj element -as type $path.$cname] == "group"} {
                if {[$xmlobj element -as id $path.$cname] == "ambient"
                       && $deveditor != ""} {
                    set w [$deveditor component top]
                } else {
                    set c [$frame.cntls insert end $xmlobj $path.$cname]
                    set w [$frame.cntls control $c]
                }
                _buildGroup $w $xmlobj $path.$cname
            } else {
                $frame.cntls insert end $xmlobj $path.$cname
            }
        }
    }
}
