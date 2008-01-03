
# ----------------------------------------------------------------------
#  NanovisServer - 
#
#  This class is used to share the nanovis hosts variable between the
#  Field2D and Field3D components.  Formerly the _nanovisHosts variable
#  was included in the Field3D class.  
#
#  My plan is to make this class the parent of the NanovisViewer and
#  HeightmapViewer classes. I'm going to move out all the server-related 
#  methods like _send*, _receive*, connect, disconnect, etc. from those
#  classes into this one.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

itcl::class Rappture::NanovisServer {
    public common _nanovisHosts ""
    public proc setServer {namelist} {
        if {[regexp {^[a-zA-Z0-9\.]+:[0-9]+(,[a-zA-Z0-9\.]+:[0-9]+)*$} $namelist match]} {
            set _nanovisHosts $namelist
        } else {
            error "bad nanovis server address \"$namelist\": should be host:port,host:port,..."
        }
    }
    public proc getServer {} {
	return $_nanovisHosts
    }
}

