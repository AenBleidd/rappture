# -*- mode: tcl; indent-tabs-mode: nil -*-

# ----------------------------------------------------------------------
#  COMPONENT: response - extracts data from an XML description of a
#  response surface (surrogate model).  Makes it available to the
#  responseviewer.
#
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require BLT

namespace eval Rappture {
    # forward declaration
}

itcl::class Rappture::Response {
    constructor {xmlobj path} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method get {}
    public method variables {}
    public method label {}
    public method vlabels {}
    public method response {}
    public method rmse {}

    private variable _xmlobj "";      # ref to lib obj
    private variable _response "";
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Response::constructor {xmlobj path} {
    if {![Rappture::library isvalid $xmlobj]} {
        error "bad value \"$xmlobj\": should be LibraryObj"
    }
    set _xmlobj $xmlobj
    set _response [$xmlobj element -as object $path]
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Response::destructor {} {
    # itcl::delete object $_response
    set _response ""
}

itcl::body Rappture::Response::variables {} {
    return [$_response get variables]
}

itcl::body Rappture::Response::rmse {} {
    return [$_response get rmse]
}

itcl::body Rappture::Response::response {} {
    return [$_response get value]
}

# Get the response surface label
itcl::body Rappture::Response::label {} {
    set lab [$_response get about.label]
    if {[regexp {([^\(]*)\(([^\)]+)\)} $lab match lab uq_part]} {
        set lab [string trim $lab]
    }
    return $lab
}

# Get the input variable labels
itcl::body Rappture::Response::vlabels {} {
    return [$_response get labels]
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of xml objects being plotted.
# Currently limited to one.
# ----------------------------------------------------------------------
itcl::body Rappture::Response::get {} {
    return $_response
}
