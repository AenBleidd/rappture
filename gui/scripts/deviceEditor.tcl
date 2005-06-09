# ----------------------------------------------------------------------
#  COMPONENT: deviceEditor - general-purpose device editor
#
#  This widget takes a <structure> description and chooses a specific
#  editor appropriate for the contents of the structure.  If the
#  <structure> contains a molecule, then it chooses a MoleculeViewer.
#  If it contains a 1D device, then it chooses a deviceViewer1D.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *DeviceEditor.width 5i widgetDefault
option add *DeviceEditor.height 5i widgetDefault

itcl::class Rappture::DeviceEditor {
    inherit itk::Widget

    constructor {owner args} { # defined below }

    public method value {args}

    # used for syncing embedded widgets
    public method widgetfor {path {widget ""}}
    public method changed {path}
    public method sync {}
    public method tool {}

    protected method _redraw {}
    protected method _type {xmlobj}

    private variable _owner ""       ;# owner containing this editor
    private variable _xmlobj ""      ;# XML <structure> object
    private variable _path2widget    ;# maps path => widget in this editor
}
                                                                                
itk::usual DeviceEditor {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::constructor {owner args} {
    set _owner $owner

    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add top {
        frame $itk_interior.top
    }
    pack $itk_component(top) -fill x

    itk_component add editors {
        Rappture::Notebook $itk_interior.editors
    }
    pack $itk_component(editors) -expand yes -fill both

    eval itk_initialize $args
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
itcl::body Rappture::DeviceEditor::value {args} {
    set onlycheck 0
    set i [lsearch -exact $args -check]
    if {$i >= 0} {
        set onlycheck 1
        set args [lreplace $args $i $i]
    }

    if {[llength $args] == 1} {
        # delete any existing object
        if {$_xmlobj != ""} {
            itcl::delete object $_xmlobj
            set _xmlobj ""
        }
        set newval [lindex $args 0]
        if {$newval != ""} {
            if {$onlycheck} {
                return
            }
            if {![Rappture::library isvalid $newval]} {
                error "bad value \"$newval\": should be Rappture::Library"
            }
            set _xmlobj $newval
        }
        _redraw
        event generate $itk_component(hull) <<Value>>

    } elseif {[llength $args] == 0} {
        sync  ;# querying -- must sync controls with the value
    } else {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }
    return $_xmlobj
}

# ----------------------------------------------------------------------
# USAGE: widgetfor <path> ?<widget>?
#
# Used by embedded widgets such as a Controls panel to register the
# various controls associated with this page.  That way, this editor
# knows what widgets to look at when syncing itself to the underlying
# XML data.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::widgetfor {path {widget ""}} {
    # if this is a query operation, then look for the path
    if {"" == $widget} {
        if {[info exists _path2widget($path)]} {
            return $_path2widget($path)
        }
        return ""
    }

    # otherwise, associate the path with the given widget
    if {[info exists _path2widget($path)]} {
        error "$path already associated with widget $_path2widget($path)"
    }
    set _path2widget($path) $widget
}

# ----------------------------------------------------------------------
# USAGE: changed <path>
#
# Invoked automatically by the various widgets associated with this
# editor whenever their value changes.  If this tool has a -analyzer,
# then it is notified that input has changed, so it can reset itself
# for a new analysis.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::changed {path} {
    if {"" != $_owner} {
        $_owner changed $path
    }
}

# ----------------------------------------------------------------------
# USAGE: sync
#
# Used by descendents such as a Controls panel to register the
# various controls associated with this page.  That way, this Tool
# knows what widgets to look at when syncing itself to the underlying
# XML data.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::sync {} {
    foreach path [array names _path2widget] {
        $_xmlobj put $path.current [$_path2widget($path) value]
    }
}

# ----------------------------------------------------------------------
# USAGE: tool
#
# Clients use this to figure out which tool is associated with
# this object.  Returns the tool containing this editor.
# ----------------------------------------------------------------------
itcl::body Rappture::Tool::tool {} {
    return [$_owner tool]
}

# ----------------------------------------------------------------------
# USAGE: _redraw
#
# Used internally to load new device data into the appropriate
# editor.  If the editor needs to be created, it is created and
# activated within this widget.  Then, the data is loaded into
# the editor.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::_redraw {} {
    switch -- [_type $_xmlobj] {
        molecule {
            if {[catch {$itk_component(editors) page molecule} p]} {
                set p [$itk_component(editors) insert end molecule]
                Rappture::MoleculeViewer $p.mol $this
                pack $p.mol -expand yes -fill both
            }
            $p.mol configure -device $_xmlobj
            $itk_component(editors) current molecule
        }
        device1D {
            if {[catch {$itk_component(editors) page device1D} p]} {
                set p [$itk_component(editors) insert end device1D]
                Rappture::DeviceViewer1D $p.dev $this
                pack $p.dev -expand yes -fill both
            }
            $p.dev configure -device $_xmlobj
            $itk_component(editors) current device1D
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _type <xmlobj>
#
# Used internally to determine the type of the device data stored
# within the <xmlobj>.  Returns a name such as "molecule" or
# "device1D" indicating the type of editor that should be used to
# display the data.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::_type {xmlobj} {
    if {$xmlobj == ""} {
        return ""
    }
    if {[llength [$xmlobj children -type molecule components]] > 0} {
        return "molecule"
    }
    return "device1D"
}
