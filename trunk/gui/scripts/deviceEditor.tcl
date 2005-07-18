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

option add *DeviceEditor.autoCleanUp yes widgetDefault

itcl::class Rappture::DeviceEditor {
    inherit itk::Widget Rappture::ControlOwner

    itk_option define -autocleanup autoCleanUp AutoCleanUp 1

    constructor {owner args} {
        Rappture::ControlOwner::constructor $owner
    } { # defined below }

    public method value {args}

    protected method _redraw {}
    protected method _type {xmlobj}

    private variable _current ""  ;# active device editor
}
                                                                                
itk::usual DeviceEditor {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::constructor {owner args} {
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
        if {$_xmlobj != ""} {
            if {$itk_option(-autocleanup)} {
                # delete any existing object
                itcl::delete object $_xmlobj
            }
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
# USAGE: _redraw
#
# Used internally to load new device data into the appropriate
# editor.  If the editor needs to be created, it is created and
# activated within this widget.  Then, the data is loaded into
# the editor.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::_redraw {} {
    if {$_current != ""} {
        $_current configure -device ""
        set _current ""
    }
    switch -- [_type $_xmlobj] {
        molecule {
            if {[catch {$itk_component(editors) page molecule} p]} {
                set p [$itk_component(editors) insert end molecule]
                Rappture::MoleculeViewer $p.mol $this
                pack $p.mol -expand yes -fill both
            }
            $p.mol configure -device $_xmlobj
            $itk_component(editors) current molecule

            set _current $p.mol
        }
        device1D {
            if {[catch {$itk_component(editors) page device1D} p]} {
                set p [$itk_component(editors) insert end device1D]
                Rappture::DeviceViewer1D $p.dev $this
                pack $p.dev -expand yes -fill both
            }
            $p.dev configure -device $_xmlobj
            $itk_component(editors) current device1D

            set _current $p.dev
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
