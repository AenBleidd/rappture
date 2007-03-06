# ----------------------------------------------------------------------
#  COMPONENT: deviceEditor - general-purpose device editor
#
#  This widget takes a <structure> description and chooses a specific
#  editor appropriate for the contents of the structure.  If the
#  <structure> contains a molecule, then it chooses a MoleculeViewer.
#  If it contains a 1D device, then it chooses a deviceViewer1D.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
    public method download {option args}

    protected method _redraw {}
    protected method _type {xmlobj}

    private variable _current ""  ;# active device editor

    public common _hubvisHosts ""

    public proc setHubvisServer {namelist} {
        if {[regexp {^[a-zA-Z0-9\.]+:[0-9]+(,[a-zA-Z0-9\.]+:[0-9]+)*$} $namelist match]} {
            set _hubvisHosts $namelist
        } else {
            error "bad visualization server address \"$namelist\": should be host:port,host:port,..."
        }
    }
}
                                                                                
itk::usual DeviceEditor {
}

# must use this name -- plugs into Rappture::resources::load
proc deviceEditor_init_resources {} {
    Rappture::resources::register \
        hubvis_server Rappture::DeviceEditor::setHubvisServer
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::constructor {owner args} {
    itk_component add top {
        frame $itk_interior.top
    }
    pack $itk_component(top) -fill x

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
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceEditor::download {option args} {
    if {"" != $_current} {
        return [eval $_current download $option $args]
    }
    return ""
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
            if {![winfo exists $itk_component(hull).mol]} {
                catch {destroy $itk_component(hull).dev}
                if {"" != $_hubvisHosts} {
                    Rappture::MolvisViewer $itk_component(hull).mol $_hubvisHosts
                } else {
                    Rappture::MoleculeViewer $itk_component(hull).mol $this
                } 
                pack $itk_component(hull).mol -expand yes -fill both
            }
            $itk_component(hull).mol configure -device $_xmlobj

            set _current $itk_component(hull).mol
        }
        device1D {
            if {![winfo exists $itk_component(hull).dev]} {
                catch {destroy $itk_component(hull).mol}
                Rappture::DeviceViewer1D $itk_component(hull).dev $this
                pack $itk_component(hull).dev -expand yes -fill both
            }
            $itk_component(hull).dev configure -device $_xmlobj

            set _current $itk_component(hull).dev
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
