# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  UTILITY: bugreport
#
#  This redefines the usual Tcl bgerror command to install a nicer
#  looking bug handler.  Bug reports can be submitted back to a
#  HUBzero-based site as support tickets.  Additional information
#  can be obtained by defining procedures as bugreport::instrumented
#  proc (captures entrance/exit from proc) and by calling
#  bugreport::remark with extra info along the way.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
option add *BugReport*Label.font {Helvetica -12} startupFile
option add *BugReport*banner*foreground white startupFile
option add *BugReport*banner*background #a9a9a9 startupFile
option add *BugReport*banner*highlightBackground #a9a9a9 startupFile
option add *BugReport*banner.title.font {Helvetica -18 bold} startupFile
option add *BugReport*xmit*wrapLength 3i startupFile
option add *BugReport*expl.width 50 startupFile
option add *BugReport*expl.font {Helvetica -12} startupFile
option add *BugReport*expl.boldFont {Helvetica -12 bold} startupFile
option add *BugReport*comments.l.font {Helvetica -12 italic} startupFile
option add *BugReport*comments.info.text.font {Helvetica -12} startupFile
option add *BugReport*details*font {Courier -12} startupFile

namespace eval Rappture::bugreport {
    # details from the current trouble report, which user may decide to submit
    variable details

    # status from bugreport::instrumented/remark in case a bug occurs
    variable extraStack ""
    variable extraInfo ""

    # assume that if there's a problem launching a job, we should know it
    variable reportJobFailures 1

    # submit these kinds of tickets by default
    variable settings
    set settings(user) $::tcl_platform(user)
    set settings(type) "automatic"
    set settings(group) ""
    set settings(category) "Rappture"
}

# ----------------------------------------------------------------------
# USAGE: install
#
# Called once in the main program to install this bug reporting
# facility.  Any unexpected errors after this call will be handled
# by this mechanism.
# ----------------------------------------------------------------------
proc Rappture::bugreport::install {} {
    ::proc ::bgerror {err} { ::Rappture::bugreport::activate $err }
    # Load tls early to prevent error in tls::initlib and [pwd]
    package require http
    package require tls
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
    variable details
    variable settings

    if {"@SHOWDETAILS" == $err} {
        pack propagate .bugreport yes
        pack forget .bugreport.expl
        pack forget .bugreport.xmit
        pack forget .bugreport.done
        pack forget .bugreport.cntls.show
        pack .bugreport.cntls -after .bugreport.banner -side bottom -fill x
        pack .bugreport.details -after .bugreport.banner \
            -expand yes -fill both -padx 8 -pady 8
        pack .bugreport.comments -after .bugreport.details \
            -expand yes -fill both -padx 8 -pady {0 8}

        update idletasks
        set w [winfo reqwidth .bugreport]
        set h [winfo reqheight .bugreport]

        set x [winfo rootx .main]
        set y [winfo rooty .main]

        set mw [winfo width .main]
        if { $mw == 1 } {
            set mw [winfo reqwidth .main]
        }
        set mh [winfo height .main]
        if { $mh == 1 } {
            set mh [winfo reqwidth .main]
        }
        if { $mw > $w } {
            set x [expr { $x + (($mw-$w)/2) }]
        }
        if { $mh > $h } {
            set y [expr { $y + (($mh-$h)/2) }]
        }
        wm geometry .bugreport +$x+$y
        raise .bugreport
        return
    }

    # gather details so we can submit trouble reports later
    # do this now, before we do anything with "catch" down below
    # that might mask the errorInfo
    register $err

    pack propagate .bugreport yes
    pack forget .bugreport.details
    pack forget .bugreport.xmit
    pack forget .bugreport.done
    pack .bugreport.cntls.show -side right
    pack .bugreport.cntls -after .bugreport.banner -side bottom -fill x
    pack .bugreport.expl -after .bugreport.banner \
        -expand yes -fill both -padx 8 -pady 8
    pack .bugreport.comments -after .bugreport.expl \
        -expand yes -fill both -padx 8 -pady {0 8}

    .bugreport.expl configure -state normal
    .bugreport.expl delete 1.0 end

    set url [Rappture::Tool::resources -huburl]
    if {"" != $url} {
        .bugreport.expl insert end "Something went wrong with this tool.  Help us understand what happened by submitting a trouble report, so we can fix the problem.  If you continue having trouble with this tool, please close it and restart."
        .bugreport.cntls.send configure -state normal
        focus .bugreport.cntls.send
    } else {
        .bugreport.expl insert end "Something went wrong with this tool.  We would ask you to submit a trouble report about the error, but we can't tell what hub it should be submitted to.  If you continue having trouble with this tool, please close it and restart."
        pack forget .bugreport.comments
        .bugreport.cntls.send configure -state disabled
        focus .bugreport.cntls.ok
    }
    fixTextHeight .bugreport.expl
    .bugreport.expl configure -state disabled

    .bugreport.details.info.text configure -state normal
    .bugreport.details.info.text delete 1.0 end
    .bugreport.details.info.text insert end "    USER: $settings(user)\n"
    .bugreport.details.info.text insert end "HOSTNAME: $details(hostname)\n"
    .bugreport.details.info.text insert end "PLATFORM: $details(platform)\n"
    .bugreport.details.info.text insert end "CATEGORY: $details(category)\n"
    .bugreport.details.info.text insert end "    TOOL: $details(referrer)\n"
    .bugreport.details.info.text insert end " SESSION: $details(session)\n"
    .bugreport.details.info.text insert end " SUMMARY: $details(summary)\n"
    .bugreport.details.info.text insert end "---------\n"
    .bugreport.details.info.text insert end $details(stackTrace)
    .bugreport.details.info.text configure -state disabled

    set w [winfo reqwidth .bugreport]
    set h [winfo reqheight .bugreport]

    set x [winfo rootx .main]
    set y [winfo rooty .main]

    set mw [winfo width .main]
    if { $mw == 1 } {
        set mw [winfo reqwidth .main]
    }
    set mh [winfo height .main]
    if { $mh == 1 } {
        set mh [winfo reqwidth .main]
    }

    if { $mw > $w } {
        set x [expr { $x + (($mw-$w)/2) }]
    }
    if { $mh > $h } {
        set y [expr { $y + (($mh-$h)/2) }]
    }

    wm geometry .bugreport +$x+$y
    wm deiconify .bugreport
    raise .bugreport

    catch {grab set .bugreport}
    update
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
# USAGE: instrumented <what> <name> <arglist> <body>
#
# Used instead of the usual Tcl "proc" or itcl::body to define a
# procedure that will automatically register information about its
# execution in the bugreport mechanism.  The <what> parameter should
# be either "proc" or "itcl::body" or something like that.  When the
# procedure starts, it pushes its call information onto the stack,
# then invokes the procedure body, then adds information about the
# return code.
# ----------------------------------------------------------------------
proc Rappture::bugreport::instrumented {what name arglist body} {
    set avals ""
    foreach term $arglist {
        set aname [lindex $term 0]
        append avals "\$$aname "
    }
    uplevel [list $what $name $arglist [format {
        Rappture::bugreport::remark -enter "PROC %s: %s"
        set __status [catch {%s} __result]
        Rappture::bugreport::remark -leave "PROC %s: code($__status) => $__result"
        switch -- $__status {
            0 - 2 {
                return $__result
            }
            3 {
                set __result "invoked \"break\" outside of a loop"
            }
            4 {
                set __result "invoked \"continue\" outside of a loop"
            }
        }
        error $__result $::errorInfo
    } $name $avals $body $name]]
}

# ----------------------------------------------------------------------
# USAGE: remark ?-enter|-leave? <message>
#
# Adds the <message> to the current "extraInfo" being kept about the
# program.  This adds useful debugging info to the report that gets
# sent back when an unexpected error is trapped.  The -enter and -leave
# options are used when a bugreport::instrumented proc starts/exits to
# change the indent level for future messages.
# ----------------------------------------------------------------------
proc Rappture::bugreport::remark {args} {
    variable extraStack
    variable extraInfo

    if {[llength $args] > 1} {
        set option [lindex $args 0]
        set args [lrange $args 1 end]
        switch -- $option {
            -enter {
                if {[llength $args] != 1} {
                    error "wrong # args: should be \"remark -enter message\""
                }
                set mesg [lindex $args 0]
                if {[llength $extraStack] == 0} {
                    set extraInfo ""
                }
                append extraInfo [remark -indent ">> $mesg"]
                set extraStack [linsert $extraStack 0 $mesg]
                return
            }
            -leave {
                if {[llength $args] != 1} {
                    error "wrong # args: should be \"remark -leave message\""
                }
                set mesg [lindex $args 0]
                set extraStack [lrange $extraStack 1 end]
                append extraInfo [remark -indent "<< $mesg"]
                return
            }
            -indent {
                if {[llength $args] != 1} {
                    error "wrong # args: should be \"remark -indent message\""
                }
            }
            default {
                error "bad option \"$option\": should be -enter, -leave, -indent"
            }
        }
    }
    set mesg [lindex $args 0]
    set nlevel [llength $extraStack]
    set indent [string repeat { } [expr {2*$nlevel}]]
    foreach line [split $mesg \n] {
        append extraInfo "$indent$line\n"
        set prefix "   "
    }
}

# ----------------------------------------------------------------------
# USAGE: sanitize <string> ?<replacement>?
#
# Removes any sensitive information in the bug report.  This is useful
# for things such as passwords that should be scrubbed out before any
# ticket is filed.  Replaces the <string> with an optional <replacement>
# string (or ******** by default).  This is usually called in some sort
# of "catch" before forwarding the error on to the usual bgerror routine.
# ----------------------------------------------------------------------
proc Rappture::bugreport::sanitize {str {repl ********}} {
    global errorInfo
    variable extraInfo

    set map [list $str $repl]
    set errorInfo [string map $map $errorInfo]
    set extraInfo [string map $map $extraInfo]
}

# ----------------------------------------------------------------------
# USAGE: attach <name> <string>
#
# Removes any sensitive information in the bug report.  This is useful
# for things such as passwords that should be scrubbed out before any
# ticket is filed.  Replaces the <string> with an optional <replacement>
# string (or ******** by default).  This is usually called in some sort
# of "catch" before forwarding the error on to the usual bgerror routine.
# ----------------------------------------------------------------------
proc Rappture::bugreport::attach {str {repl ********}} {
    global errorInfo
    variable extraInfo

    set map [list $str $repl]
    set errorInfo [string map $map $errorInfo]
    set extraInfo [string map $map $extraInfo]
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

    pack propagate .bugreport no
    pack forget .bugreport.details
    pack forget .bugreport.expl
    pack forget .bugreport.comments
    pack forget .bugreport.cntls
    pack .bugreport.xmit -after .bugreport.banner -padx 8 -pady 8
    .bugreport.xmit.title configure -text "Sending trouble report to [Rappture::Tool::resources -hubname]..."

    # send off the trouble report...
    .bugreport.xmit.icon start
    set status [catch send result]
    .bugreport.xmit.icon stop

    pack propagate .bugreport yes
    pack forget .bugreport.xmit
    pack .bugreport.expl -after .bugreport.banner -padx 8 -pady 8
    .bugreport.expl configure -state normal
    .bugreport.expl delete 1.0 end

    # handle the result
    if {$status != 0} {
        # add error to the details field, so we can see it with magic clicks
        .bugreport.details.info.text configure -state normal
        .bugreport.details.info.text insert 1.0 "Ticket submission failed:\n$result\n-----\n"
        .bugreport.details.info.text configure -state disabled

        .bugreport.expl insert end "Oops! Ticket submission failed:\n$result\n\nIf you want to report the original problem, you can file your own trouble report by going to the web site and clicking on the \"Help\" or \"Support\" link on the main navigation bar.  If you continue having trouble with this tool, please close it and restart."
    } elseif {[regexp {Ticket #([0-9]*) +\((.*?)\) +([0-9]+) +times} $result match ticket extra times]} {
        .bugreport.expl insert end "This problem has been reported as " "" "Ticket #$ticket" bold " in our system." ""
        if {[string is integer $times] && $times > 1} {
            .bugreport.expl insert end "  This particular problem has been reported $times times."
        }
        .bugreport.expl insert end "\n\nIf you continue having trouble with this tool, please close it and restart.  Thanks for reporting the problem and helping us improve things!"
    } else {
        .bugreport.expl insert end "This problem has been reported.  Here is the response from the hub, which may contain information about your ticket:\n" "" $result bold "\n\nIf you continue having trouble with this tool, please close it and restart.  Thanks for reporting the problem and helping us improve things!" ""
    }
    fixTextHeight .bugreport.expl
    .bugreport.expl configure -state disabled
    pack .bugreport.done -side bottom -padx 8 -pady 8
    focus .bugreport.done
}

# ----------------------------------------------------------------------
# USAGE: register <err>
#
# Low-level function used to capture information about a bug report
# prior to calling "send", which actually sends the ticket.  We usually
# let the user preview the information and decide whether or not to
# send the ticket.
# ----------------------------------------------------------------------
proc Rappture::bugreport::register {err} {
    global errorInfo tcl_platform
    variable details
    variable settings
    variable extraInfo

    #
    # Figure out exactly what we'll send if the bug report is
    # submitted, so we can show the user.
    #
    set stackTrace "$err\n---------\n$errorInfo\n---------\n$extraInfo"
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
        set category $settings(category)
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

    set details(summary) $summary
    set details(category) $category
    set details(stackTrace) $stackTrace
    set details(hostname) [info hostname]
    set details(session) [Rappture::Tool::resources -session]
    set details(referrer) [Rappture::Tool::resources -appname]
    set details(platform) [array get tcl_platform]

    Rappture::Logger::log oops! $summary
}

# ----------------------------------------------------------------------
# USAGE: attachment <string>
#
# Low-level function used to capture information about a bug report
# prior to calling "send", which actually sends the ticket.  We usually
# let the user preview the information and decide whether or not to
# send the ticket.
# ----------------------------------------------------------------------
proc Rappture::bugreport::attachment { string } {
    variable details
    set details(attachment) $string
}

# ----------------------------------------------------------------------
# USAGE: send
#
# Low-level function used to send bug reports back to the hub site.
# Error details gathered by a previous call to "register" are sent
# along as a support ticket.  Returns a string of the following form,
# representing details about the new or existing ticket:
#   Ticket #XX (XXXXXX) XX times
# ----------------------------------------------------------------------
proc Rappture::bugreport::send {} {
    variable details
    variable settings

    package require http
    package require tls
    http::register https 443 [list ::tls::socket -tls1 1]

    set report $details(stackTrace)
    set cmts [string trim [.bugreport.comments.info.text get 1.0 end]]
    if {[string length $cmts] > 0} {
        set report "$cmts\n\n[string repeat = 72]\n$report"
    }
    set toolFile ""
    if { [info exists details(attachment)] } {
        set toolFile "/tmp/tool[pid].xml"
        set f [open $toolFile "w"]
        puts $f $details(attachment)
        close $f
        unset details(attachment)
    }
    set query [http::formatQuery \
        option com_support \
        controller tickets \
        upload $toolFile \
        task create \
        no_html 1 \
        report $report \
        sesstoken $details(session) \
        hostname $details(hostname) \
        os $details(platform) \
        category $details(category) \
        summary $details(summary) \
        referrer $details(referrer) \
        login $settings(user) \
        group $settings(group) \
        type $settings(type) \
    ]
    set url [Rappture::Tool::resources -huburl]
    if { $url == "" } {
        set url "http://hubzero.org"
    }
    if {[string index $url end] == "/"} {
        append url "index.php"
    } else {
        append url "/index.php"
    }

    set token [http::geturl $url -query $query -timeout 60000]

    if {[http::ncode $token] != 200} {
        error [http::code $token]
    }
    upvar #0 $token rval
    set info $rval(body)
    http::cleanup $token

    if { $toolFile != "" } {
        file delete $toolFile
    }
    if {[regexp {Ticket #[0-9]* +\(.*?\) +[0-9]+ +times} $info match]} {
        return $match
    }
    error "Report received, but ticket may not have been filed.  Here's the result...\n$info"
}

# ----------------------------------------------------------------------
# USAGE: fixTextHeight <widget>
#
# Used internally to adjust the height of a text widget so it is just
# tall enough to show the info within it.
# ----------------------------------------------------------------------
proc Rappture::bugreport::fixTextHeight {widget} {
    #
    # HACK ALERT!  In Tk8.5, we can count display lines directly.
    #   But for earlier versions, we have to cook up something
    #   similar.
    #
    if {[catch {$widget count -displaylines 1.0 end} h] == 0 && $h > 0} {
        $widget configure -height $h
    } else {
        for {set h 1} {$h < 15} {incr h} {
            $widget configure -height $h
            $widget see 1.0
            update idletasks
            if {"" != [$widget bbox end-1char]} {
                break
            }
        }
    }
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
            # is it being run within a workspace?
            set appname [Rappture::Tool::resources -appname]
            if {[string match {[Ww]orkspace*} $appname]} {
                return 0
            }
            set url [Rappture::Tool::resources -huburl]
            if { $url == "" } {
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
label .bugreport.banner.title -text "Oops! Internal Error"
pack .bugreport.banner.title -side left -padx {0 8} -pady 2

# add these frustration bindings in case the "Dismiss" button is off screen
bind .bugreport.banner.icon <Double-ButtonPress-1> \
    Rappture::bugreport::deactivate
bind .bugreport.banner.title <Double-ButtonPress-1> \
    Rappture::bugreport::deactivate
bind .bugreport <KeyPress-Escape> \
    Rappture::bugreport::deactivate

set bg [.bugreport cget -background]
text .bugreport.expl -borderwidth 0 -highlightthickness 0 -background $bg \
    -height 3 -wrap word
.bugreport.expl tag configure bold \
    -font [option get .bugreport.expl boldFont Font]
#
# HACK ALERT!  We have problems with fixTextHeight working correctly
#   on Windows for Tk8.4 and earlier.  To make it work properly, we
#   add the binding below.  At some point, we'll ditch 8.4 and we can
#   use the new "count -displaylines" option in Tk8.5.
#
bind .bugreport.expl <Map> {Rappture::bugreport::fixTextHeight %W}

frame .bugreport.comments
label .bugreport.comments.l -text "What were you doing just before this error?" -anchor w
pack .bugreport.comments.l -side top -anchor w
Rappture::Scroller .bugreport.comments.info -xscrollmode none -yscrollmode auto
text .bugreport.comments.info.text -width 30 -height 3 -wrap word
.bugreport.comments.info contents .bugreport.comments.info.text
bind .bugreport.comments.info.text <ButtonPress> {focus %W}
pack .bugreport.comments.info -expand yes -fill both

frame .bugreport.cntls
pack .bugreport.cntls -side bottom -fill x
button .bugreport.cntls.ok -text "Ignore" -command {
    Rappture::bugreport::deactivate
}
pack .bugreport.cntls.ok -side left -padx {4 20} -pady 8
button .bugreport.cntls.send -text "Send Trouble Report" -command {
    Rappture::bugreport::submit
}
pack .bugreport.cntls.send -side right -padx 4 -pady 8

button .bugreport.cntls.show -text "Show Details..." \
    -command {Rappture::bugreport::activate @SHOWDETAILS}
pack .bugreport.cntls.show -side right


frame .bugreport.details
Rappture::Scroller .bugreport.details.info -xscrollmode auto -yscrollmode auto
text .bugreport.details.info.text -width 50 -height 15 -wrap none
.bugreport.details.info contents .bugreport.details.info.text
pack .bugreport.details.info -expand yes -fill both

frame .bugreport.xmit
Rappture::Animicon .bugreport.xmit.icon -images {
    circle-ball1 circle-ball2 circle-ball3 circle-ball4
    circle-ball5 circle-ball6 circle-ball7 circle-ball8
}
pack .bugreport.xmit.icon -side left
label .bugreport.xmit.title -anchor w
pack .bugreport.xmit.title -side left -expand yes -fill x

button .bugreport.done -text "Done" \
    -command Rappture::bugreport::deactivate

# this binding keeps the bugreport window on top
bind BugReportOnTop <ButtonPress> {
    wm deiconify %W
    raise %W
}
set btags [bindtags .bugreport]
bindtags .bugreport [linsert $btags 0 BugReportOnTop]

