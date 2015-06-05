# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: image - represents a picture image
#
#  This object represents a Tk image.  It is convenient to have it
#  expressed as an Itcl object, so it can be managed just like a
#  curve, table, etc.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture { # forward declaration }

itcl::class Rappture::Image {
    constructor {xmlobj path} { # defined below }
    destructor { # defined below }

    public method tkimage {} { return $_image }
    public method hints {{keyword ""}}

    private variable _xmlobj ""  ;# ref to lib obj with image data
    private variable _path ""    ;# path in _xmlobj where data sits
    private variable _image ""   ;# underlying image data
    private variable _hints
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Image::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj $xmlobj
    set _path $path
    set data [string trim [$xmlobj get $path.current]]
    if {[string length $data] == 0} {
        set _image [image create photo]
    } else {
        set _image [image create photo -data $data]
    }

    set _hints(note) [string trim [$_xmlobj get $_path.note.contents]]
    set _hints(tooldir) [$_xmlobj get tool.version.application.directory(tool)]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Image::destructor {} {
    image delete $_image
}

# ----------------------------------------------------------------------
# USAGE: hints ?<keyword>?
#
# Returns a list of key/value pairs for various hints about showing
# this image.  If a particular <keyword> is specified, then it returns
# the hint for that <keyword>, if it exists.
# ----------------------------------------------------------------------
itcl::body Rappture::Image::hints {{keyword ""}} {
    if {$keyword != ""} {
        if {[info exists _hints($keyword)]} {
            return $_hints($keyword)
        }
        return ""
    }
    return [array get _hints]
}
