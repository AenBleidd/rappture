# ----------------------------------------------------------------------
#  UTILITY: bugreport
#
#  This redefines the usual Tcl bgerror command to install a nicer
#  looking bug handler.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2006  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

option add *BugReport*banner*foreground white startupFile
option add *BugReport*banner*background #a9a9a9 startupFile
option add *BugReport*banner*highlightBackground #a9a9a9 startupFile
option add *BugReport*banner*font \
    -*-helvetica-bold-r-normal-*-*-180-* startupFile
option add *BugReport*expl.font \
    -*-helvetica-medium-r-normal-*-*-120-* startupFile
option add *BugReport*expl.wrapLength 3i startupFile

namespace eval Rappture::bugreport { # forward declaration }

# ----------------------------------------------------------------------
# USAGE: install
#
# Called once in the main program to install this bug reporting
# facility.  Any unexpected errors after this call will be handled
# by this mechanism.
# ----------------------------------------------------------------------
proc Rappture::bugreport::install {} {
    proc ::bgerror {err} { ::Rappture::bugreport::activate $err }
}

# ----------------------------------------------------------------------
# USAGE: activate <error>
#
# Used internally to pop up the bug handler whenver a bug is
# encountered.  Tells the user that there is a bug and logs the
# problem, so it can be fixed.
# ----------------------------------------------------------------------
proc Rappture::bugreport::activate {err} {
    global errorInfo

    set w [winfo reqwidth .bugreport]
    set h [winfo reqheight .bugreport]
    set x [expr {([winfo screenwidth .bugreport]-$w)/2}]
    set y [expr {([winfo screenheight .bugreport]-$w)/2}]

    wm geometry .bugreport +$x+$y
    wm deiconify .bugreport
    raise .bugreport

    # should log the error someday too...

    catch {grab set .bugreport}
}

# ----------------------------------------------------------------------
# USAGE: deactivate
#
# Used internally to take down the bug handler dialog.
# ----------------------------------------------------------------------
proc Rappture::bugreport::deactivate {} {
    grab release .bugreport
    wm withdraw .bugreport

    # reset the grab in case it's hosed
    Rappture::grab::reset
}

# ----------------------------------------------------------------------
# Build the bug reporting dialog
# ----------------------------------------------------------------------
toplevel .bugreport -class BugReport -borderwidth 1 -relief solid
wm overrideredirect .bugreport 1
wm withdraw .bugreport

frame .bugreport.banner -background #a9a9a9
pack .bugreport.banner -side top -fill x
label .bugreport.banner.icon -image [Rappture::icon alert]
pack .bugreport.banner.icon -side left -padx 2 -pady 2
label .bugreport.banner.title -text "Oops! Unexpected Error"
pack .bugreport.banner.title -side left -padx {0 8} -pady 2

button .bugreport.ok -text "OK" -command Rappture::bugreport::deactivate
pack .bugreport.ok -side bottom -pady {0 8}

label .bugreport.expl -text "You've found a bug in this application.\n\nIf it's not a serious bug, you may be able to continue using the tool.  But if the tool doesn't seem to behave properly after this, please close this session and start the tool again.\n\nYou can help us improve nanoHUB by reporting this error.  Click on the \"Report Problems\" link in the web page containing this tool session, and tell us what you were doing when the error occurred." -justify left
pack .bugreport.expl -padx 8 -pady 8

# this binding keeps the bugreport window on top
bind BugReportOnTop <ButtonPress> {
    wm deiconify %W
    raise %W
}
set btags [bindtags .bugreport]
bindtags .bugreport [linsert $btags 0 BugReportOnTop]
