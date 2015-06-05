# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  WIDGET: diffview - view differences between two strings
#
#  These are the class bindings for the Diffview widget.  The widget
#  itself is implemented in C code, but this file customizes the
#  behavior of the widget by adding default bindings.  It is loaded
#  when the widget initializes itself.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
namespace eval Rappture { # forward declaration }

# ----------------------------------------------------------------------
# Code for "usual" itk options
# ----------------------------------------------------------------------
if {[catch {package require Itk}] == 0} {
    itk::usual Diffview {
        keep -background -foreground -cursor
        keep -addedbackground -addedforeground
        keep -deletedbackground -deletedforeground -overstrike
        keep -changedbackground -changedforeground
        keep -highlightcolor -highlightthickness
        rename -highlightbackground -background background Background
    }
}


# ----------------------------------------------------------------------
# Key navigation
# ----------------------------------------------------------------------
bind Diffview <Key-Up> {
    %W yview scroll -1 units
}
bind Diffview <Control-Key-Up> {
    %W yview scroll -1 pages
}
bind Diffview <Key-Prior> {
    %W yview scroll -1 pages
}

bind Diffview <Key-Down> {
    %W yview scroll 1 units
}
bind Diffview <Control-Key-Down> {
    %W yview scroll 1 pages
}
bind Diffview <Key-Next> {
    %W yview scroll 1 pages
}

bind Diffview <Key-Left> {
    %W xview scroll -1 units
}
bind Diffview <Control-Key-Left> {
    %W xview scroll -1 pages
}

bind Diffview <Key-Right> {
    %W xview scroll 1 units
}
bind Diffview <Control-Key-Right> {
    %W xview scroll 1 pages
}

# ----------------------------------------------------------------------
# Click/drag for fast scanning
# ----------------------------------------------------------------------
bind Diffview <ButtonPress-2> {
    %W scan mark %x %y
}
bind Diffview <B2-Motion> {
    %W scan dragto %x %y
}

# ----------------------------------------------------------------------
# MouseWheel bindings
# ----------------------------------------------------------------------
switch -- [tk windowingsystem] {
    classic - aqua {
        bind Diffview <MouseWheel> {
            %W yview scroll [expr {-(%D)}] units
        }
        bind Diffview <Option-MouseWheel> {
            %W yview scroll [expr {-10*(%D)}] units
        }
        bind Diffview <Shift-MouseWheel> {
            %W xview scroll [expr {-(%D)}] units
        }
        bind Diffview <Shift-Option-MouseWheel> {
            %W xview scroll [expr {-10*(%D)}] units
        }
    }
    x11 {
        # support for mousewheels on Linux comes from buttons 4/5
        bind Diffview <4> {
            if {!$tk_strictMotif} {
                %W yview scroll -5 units
            }
        }
        bind Diffview <5> {
            if {!$tk_strictMotif} {
                %W yview scroll 5 units
            }
        }

        # in case anyone generates a <MouseWheel> event
        bind Diffview <MouseWheel> {
            %W yview scroll [expr {-(%D/120)*4}] units
        }
    }
    default {
        bind Diffview <MouseWheel> {
            %W yview scroll [expr {-(%D/120)*4}] units
        }
    }
}
