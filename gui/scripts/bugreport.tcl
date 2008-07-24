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
    -*-helvetica-bold-r-normal-*-18-* startupFile
option add *BugReport*Label.font \
    -*-helvetica-medium-r-normal-*-12-* startupFile
option add *BugReport*xmit*wrapLength 3i startupFile
option add *BugReport*expl.width 50 startupFile
option add *BugReport*expl.font \
    -*-helvetica-medium-r-normal-*-12-* startupFile
option add *BugReport*expl.boldFont \
    -*-helvetica-bold-r-normal-*-12-* startupFile

namespace eval Rappture::bugreport {
    # assume that if there's a problem launching a job, we should know it
    variable reportJobFailures 1
}

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
    global env errorInfo

    if {"@SHOWDETAILS" == $err} {
        pack forget .bugreport.xmit
        pack forget .bugreport.ok
        pack .bugreport.details -after .bugreport.banner \
            -expand yes -fill both -padx 8 -pady 8
        focus .bugreport.details.cntls.ok
        return
    }

    # always fill in details so we can submit trouble reports later
    .bugreport.details.info.text configure -state normal
    .bugreport.details.info.text delete 1.0 end
    .bugreport.details.info.text insert end "$err\n-----\n$errorInfo"
    .bugreport.details.info.text configure -state disabled

    if {[shouldReport for oops]} {
        pack forget .bugreport.details
        pack forget .bugreport.expl
        pack .bugreport.ok -side bottom -after .bugreport.banner -pady {0 8}
        pack .bugreport.xmit -after .bugreport.ok -padx 8 -pady 8
        focus .bugreport.ok
        set dosubmit 1
    } else {
        pack forget .bugreport.expl
        pack forget .bugreport.xmit
        pack forget .bugreport.ok
        pack .bugreport.details -after .bugreport.banner \
            -expand yes -fill both -padx 8 -pady 8
        focus .bugreport.details.cntls.ok
        set dosubmit 0
    }

    set w [winfo reqwidth .bugreport]
    set h [winfo reqheight .bugreport]
    set x [expr {([winfo screenwidth .bugreport]-$w)/2}]
    set y [expr {([winfo screenheight .bugreport]-$w)/2}]

    wm geometry .bugreport +$x+$y
    wm deiconify .bugreport
    raise .bugreport

    catch {grab set .bugreport}
    update

    if {$dosubmit} {
        submit
    }
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
# USAGE: submit
#
# Takes details currently stored in the panel and registers them
# as a support ticket on the hosting hub site.  Pops up a panel
# during the process and informs the user of the result.
# ----------------------------------------------------------------------
proc Rappture::bugreport::submit {} {
    set info [.bugreport.details.info.text get 1.0 end]

    pack forget .bugreport.details
    pack .bugreport.ok -side bottom -after .bugreport.banner -pady {0 8}
    pack .bugreport.xmit -after .bugreport.ok -padx 8 -pady 8
    .bugreport.xmit.title configure -text "Sending trouble report to [Rappture::Tool::resources -hubname]..."
    focus .bugreport.ok

    # send off the trouble report...
    .bugreport.xmit.icon start
    set status [catch {register $info} result]
    .bugreport.xmit.icon stop

    pack forget .bugreport.xmit
    pack .bugreport.expl -after .bugreport.ok -padx 8 -pady 8
    .bugreport.expl configure -state normal
    .bugreport.expl delete 1.0 end

    # handle the result
    if {$status != 0} {
        # add error to the details field, so we can see it with magic clicks
        .bugreport.details.info.text configure -state normal
        .bugreport.details.info.text insert 1.0 "Ticket submission failed:\n$result\n-----\n"
        .bugreport.details.info.text configure -state disabled

        .bugreport.expl insert end "This tool encountered an unexpected error.  We tried to submit a trouble report automatically, but that failed.  If you want to report this incident, you can file your own trouble report.  Look for the \"Help\" or \"Support\" links on the main navigation bar of the web site.\n\nIf you continue having trouble with this tool, please close it and launch another session."
    } elseif {[regexp {Ticket #([0-9]*) +\((.*?)\) +([0-9]+) +times} $result match ticket extra times]} {
        .bugreport.expl insert end "This tool encountered an unexpected error.  The problem has been reported as " "" "Ticket #$ticket" bold " in our system." ""
        if {[string is integer $times] && $times > 1} {
            .bugreport.expl insert end "  This particular problem has been reported $times times."
        }
        .bugreport.expl insert end "\n\nIf you continue having trouble with this tool, please close it and launch another session."
    } else {
        .bugreport.expl insert end "This tool encountered an unexpected error, and the problem was reported.  Here is the response from the hub, which may contain information about your ticket:\n" "" $result bold "\n\nIf you continue having trouble with this tool, please close it and launch another session." ""
    }
    for {set h 1} {$h < 50} {incr h} {
        .bugreport.expl configure -height $h
        .bugreport.expl see 1.0
        update idletasks
        if {"" != [.bugreport.expl bbox end-1char]} {
            break
        }
    }
    .bugreport.expl configure -state disabled
}

# ----------------------------------------------------------------------
# USAGE: download
#
# Used to download the current ticket information to the user's
# desktop.
# ----------------------------------------------------------------------
proc Rappture::bugreport::download {} {
    set info [.bugreport.details.info.text get 1.0 end]
    Rappture::filexfer::download $info bugreport.txt
}

# ----------------------------------------------------------------------
# USAGE: register <stackTrace>
#
# Low-level function used to send bug reports back to the hub site.
# Error details in the <stackTrace> are posted to a URL that creates
# a support ticket.  Returns a string of the following form,
# representing details about the new or existing ticket:
#   Ticket #XX (XXXXXX) XX times
# ----------------------------------------------------------------------
proc Rappture::bugreport::register {stackTrace} {
    global env tcl_platform

    package require http
    package require tls
    http::register https 443 ::tls::socket

    if {![regexp {^([^\n]+)\n} $stackTrace match summary]} {
        if {[string length $stackTrace] == 0} {
            set summary "Unexpected error from Rappture"
        } else {
            set summary $stackTrace
        }
    }
    if {[string length $summary] > 200} {
        set summary "[string range $summary 0 200]..."
    }
    if {[string match {Problem launching job*} $summary]} {
        append summary " (in tool \"[Rappture::Tool::resources -appname]\")"
        set category "Tools"
    } else {
        set category "Rappture"
    }

    # make sure that the stack trace isn't too long
    set toolong 20000
    if {[string length $stackTrace] > $toolong} {
        #
        # If this came from "Problem launching job", then it will have
        # a "== RAPPTURE INPUT ==" part somewhere in the middle.  Try
        # to show the first part, this middle part, and the very last
        # part, cutting out whatever we have to in the middle.
        #
        if {[regexp -indices {\n== RAPPTURE INPUT ==\n} $stackTrace match]} {
            foreach {smid0 smid1} $match break
            set quarter [expr {$toolong/4}]
            set s0 $quarter
            set smid0 [expr {$smid0-$quarter}]
            set smid1 [expr {$smid1+$quarter}]
            set s1 [expr {[string length $stackTrace]-$quarter}]

            if {$smid0 < $s0} {
                # first part is short -- truncate last part
                set stackTrace "[string range $stackTrace 0 $smid1]\n...\n[string range $stackTrace [expr {[string length $stackTrace]-($toolong-$smid1)}] end]"
            } elseif {$smid1 > $s1} {
                # last part is short -- truncate first part
                set tailsize [expr {[string length $stackTrace]-$smid0}]
                set stackTrace "[string range $stackTrace 0 [expr {$toolong-$tailsize}]]\n...\n[string range $stackTrace $smid0 end]"
            } else {
                # rappture input line is right about in the middle
                set stackTrace "[string range $stackTrace 0 $s0]\n...\n[string range $stackTrace $smid0 $smid1]\n...\n[string range $stackTrace $s1 end]"
            }
        } else {
            # no Rappture input -- just show first part and last part
            set half [expr {$toolong/2}]
            set stackTrace "[string range $stackTrace 0 $half]\n...\n[string range $stackTrace [expr {[string length $stackTrace]-$half}] end]"
        }
    }

    set query [http::formatQuery \
        option com_support \
        task create \
        no_html 1 \
        report $stackTrace \
        login $tcl_platform(user) \
        sesstoken [Rappture::Tool::resources -session] \
        hostname [info hostname] \
        category $category \
        summary $summary \
        referrer "tool \"[Rappture::Tool::resources -appname]\"" \
    ]
    
    set url [Rappture::Tool::resources -huburl]
    if {[string index $url end] == "/"} {
        append url "index2.php"
    } else {
        append url "/index2.php"
    }

    set token [http::geturl $url -query $query -timeout 60000]

    if {[http::ncode $token] != 200} {
        error [http::code $token]
    }
    upvar #0 $token rval
    if {[regexp {Ticket #[0-9]* +\(.*?\) +[0-9]+ +times} $rval(body) match]} {
        return $match
    }
    error "Report received, but ticket may not have been filed.  Here's the result...\n$rval(body)"
}

# ----------------------------------------------------------------------
# USAGE: shouldReport jobfailures <boolean>
# USAGE: shouldReport for ?oops|jobs?
#
# Used internally to determine whether or not this system should
# automatically report errors back to the hosting hub.  Returns 1
# if the tool should, and 0 otherwise.  The decision is made based
# on whether this is a current tool in production, whether it is
# being tested in a workspace, and whether the tool commonly generates
# problems (by pilot error in its input deck).
# ----------------------------------------------------------------------
proc Rappture::bugreport::shouldReport {option value} {
    global env

    switch -- $option {
        jobfailures {
            variable reportJobFailures
            if {![string is boolean $value]} {
                error "bad value \"$value\": should be boolean"
            }
            set reportJobFailures $value
        }
        for {
            # is this a tool in production?
            if {![info exists env(RAPPTURE_VERSION)]
                  || $env(RAPPTURE_VERSION) != "current"} {
                return 0
            }

            # is it being run within a workspace?
            set appname [Rappture::Tool::resources -appname]
            if {[string match {[Ww]orkspace*} $appname]} {
                return 0
            }

            # if this is a problem launching a job and the tool
            # expects this, then don't bother with automatic reports.
            variable reportJobFailures
            if {"jobs" == $value && !$reportJobFailures} {
                return 0
            }

            # this is a real problem -- report it!
            return 1
        }
        default {
            error "bad option \"$option\": should be jobfailures or for"
        }
    }
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

# add these frustration bindings in case the "Dismiss" button is off screen
bind .bugreport.banner.icon <Double-ButtonPress-1> \
    Rappture::bugreport::deactivate
bind .bugreport.banner.title <Double-ButtonPress-1> \
    Rappture::bugreport::deactivate

button .bugreport.ok -text "Dismiss" -command Rappture::bugreport::deactivate
pack .bugreport.ok -side bottom -pady {0 8}

frame .bugreport.xmit
Rappture::Animicon .bugreport.xmit.icon -images {
    circle-ball1 circle-ball2 circle-ball3 circle-ball4
    circle-ball5 circle-ball6 circle-ball7 circle-ball8
}
pack .bugreport.xmit.icon -side left
label .bugreport.xmit.title -anchor w
pack .bugreport.xmit.title -side left -expand yes -fill x

text .bugreport.expl -borderwidth 0 -highlightthickness 0 -wrap word
.bugreport.expl tag configure bold \
    -font [option get .bugreport.expl boldFont Font]

bind .bugreport.expl <Control-1><Control-1><Control-3><Control-3> {
    Rappture::bugreport::activate @SHOWDETAILS
}

bind .bugreport.expl <Control-1><Control-1><Control-Shift-1><Control-Shift-1> {
    Rappture::bugreport::activate @SHOWDETAILS
}

frame .bugreport.details
frame .bugreport.details.cntls
pack .bugreport.details.cntls -side bottom -fill x
button .bugreport.details.cntls.ok -text "Dismiss" -command {
    Rappture::bugreport::deactivate
}
pack .bugreport.details.cntls.ok -side right -padx 2 -pady 4
button .bugreport.details.cntls.send -text "Send Trouble Report" -command {
    Rappture::bugreport::submit
}
pack .bugreport.details.cntls.send -side left -padx 2 -pady 4
button .bugreport.details.cntls.dload -text "Download" -command {
    Rappture::bugreport::download
}
pack .bugreport.details.cntls.dload -side left -padx 2 -pady 4

Rappture::Scroller .bugreport.details.info -xscrollmode auto -yscrollmode auto
text .bugreport.details.info.text -width 50 -height 15 -wrap none
.bugreport.details.info contents .bugreport.details.info.text
pack .bugreport.details.info -expand yes -fill both

# this binding keeps the bugreport window on top
bind BugReportOnTop <ButtonPress> {
    wm deiconify %W
    raise %W
}
set btags [bindtags .bugreport]
bindtags .bugreport [linsert $btags 0 BugReportOnTop]
