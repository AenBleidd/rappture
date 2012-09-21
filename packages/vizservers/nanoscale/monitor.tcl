# ----------------------------------------------------------------------
#  MONITOR
#
#  This script checks to see if the nanoscale server is alive, and
#  if not, it attempts to execute another one.  It is run every so
#  often to make sure that the system is up and running.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

# This is where the production nanoscale server should be running
set port 4000

proc email {to subject body} {
    set message "To: $to"
    append message "\nFrom: support@nanohub.org"
    append message "\nSubject: $subject"
    append message "\n\n"  ;# sendmail terminates header with blank line
    append message "$body"
    
    catch {
        set fid [open "| /usr/lib/sendmail -oi -t" "w"]
        puts -nonewline $fid $message
        close $fid
    }
}

if {[catch {socket localhost $port}]} {
    if {[catch {exec /bin/sh /opt/nanovis/bin/start_server &} err]} {
        email mmc@tmail.com "nanoVIS problem" "Can't launch server on [info hostname]:\n$err"
    } else {
        email mmc@tmail.com "nanoVIS restarted" "Relaunched server on [info hostname] at [clock format [clock seconds]]"
    }
}
