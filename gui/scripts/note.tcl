# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: Note - widget for displaying HTML notes
#
#  This widget represents an entry on a control panel that displays
#  information to the user.  It is not an input; it helps to describe
#  the interface.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require http

itcl::class Rappture::Note {
    inherit itk::Widget

    # need this only to be like other controls
    itk_option define -state state State ""

    constructor {owner path args} { # defined below }

    public method value {args}

    public method label {}
    public method tooltip {}

    protected method _setContents {info}
    protected method _escapeChars {text}

    private variable _owner ""    ;# thing managing this control
    private variable _path ""     ;# path in XML to this note
}

itk::usual Note {
    keep -cursor
    keep -foreground -background
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Note::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    #
    # Create the widget and configure it properly based on other
    # hints in the XML.
    #
    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add html {
        Rappture::HTMLviewer $itk_component(scroller).html
    }
    $itk_component(scroller) contents $itk_component(html)

    eval itk_initialize $args

    _setContents [string trim [$_owner xml get $_path.contents]]
    set w [string trim [$_owner xml get $_path.width]]
    if { $w != "" } {
        $itk_component(html) configure -width $w
    }
    set h [string trim [$_owner xml get $_path.height]]
    if { $h != "" } {
        $itk_component(html) configure -height $h
    }
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
itcl::body Rappture::Note::value {args} {
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
        set newval [lindex $args 0]
        _setContents $newval
        return $newval

    } elseif {[llength $args] != 0} {
        error "wrong # args: should be \"value ?-check? ?newval?\""
    }

    #
    # Query the value and return.
    #
    error "don't know how to check value of <note>"
    return
}

# ----------------------------------------------------------------------
# USAGE: label
#
# Clients use this to query the label associated with this widget.
# Reaches into the XML and pulls out the appropriate label string.
# ----------------------------------------------------------------------
itcl::body Rappture::Note::label {} {
    error "can't get label"
}

# ----------------------------------------------------------------------
# USAGE: tooltip
#
# Clients use this to query the tooltip associated with this widget.
# Reaches into the XML and pulls out the appropriate description
# string.  Returns the string that should be used with the
# Rappture::Tooltip facility.
# ----------------------------------------------------------------------
itcl::body Rappture::Note::tooltip {} {
    error "can't get tooltip"
}

# ----------------------------------------------------------------------
# USAGE: _setContents <info>
#
# Used internally to load HTML info into the display area for this
# widget.
# ----------------------------------------------------------------------
itcl::body Rappture::Note::_setContents {info} {
    switch -regexp -- $info {
        ^https?:// {
            if {[catch {http::geturl $info} token] == 0} {
                set html [http::data $token]
                http::cleanup $token
            } else {
                set html "<html><body><h1>Oops! Internal Error</h1><p>[_escapeChars $token]</p></body></html>"
            }
            $itk_component(html) load $html
        }
        ^file:// {
            set file [string range $info 7 end]
            if {[file pathtype $file] != "absolute"} {
                # find the file on a search path
                set dir [[$_owner tool] installdir]
                set searchlist [list $dir [file join $dir docs]]
                foreach dir $searchlist {
                    if {[file readable [file join $dir $file]]} {
                        set file [file join $dir $file]
                        break
                    }
                }
            }

            # load the contents of the file
            set cmds {
                set fid [open $file r]
                set html [read $fid]
                close $fid
            }
            if {[catch $cmds result]} {
                set html "<html><body><h1>Oops! File Not Found</h1><p>[_escapeChars $result]</p></body></html>"
            }

            # not HTML? then escape nasty characters and display it.
            if {![regexp {<html>.*</html>} $html]} {
                set html "<html><body><p>[_escapeChars $html]</p></body></html>"
            }
            $itk_component(html) load $html -in $file
        }
        default {
            set html "<html><body><p>[_escapeChars $info]</p></body></html>"
            $itk_component(html) load $html
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _escapeChars <info>
#
# Used internally to escape HTML characters in ordinary text.  Used
# when trying to display ordinary text, which may have things like
# "<b>" or "x < 2" embedded within it.
# ----------------------------------------------------------------------
itcl::body Rappture::Note::_escapeChars {info} {
    regsub -all & $info \001 info
    regsub -all \" $info {\&quot;} info
    regsub -all < $info {\&lt;} info
    regsub -all > $info {\&gt;} info
    regsub -all \001 $info {\&amp;} info
    return $info
}
