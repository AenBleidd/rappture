# ----------------------------------------------------------------------
#  COMPONENT: filexfer - support for file transfer with user's desktop
#
#  Supports interactions with the filexfer middleware, enabling
#  Rappture to transfer files to and from the user's desktop.
#  Files are downloaded by invoking "exportfile", and uploaded by
#  invoking "importfile".  The middleware handles the rest.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2007  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

namespace eval Rappture { # forward declaration }
namespace eval Rappture::filexfer {
    variable enabled 0   ;# set to 1 when middleware is in place
    variable commands    ;# complete path to exportfile/importfile
    variable job         ;# used to manage bgexec jobs
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::init
#
# Called in the main application to see if the filexfer middleware
# is installed.  Returns 1 if the middleware is installed, and 0
# otherwise.
# ----------------------------------------------------------------------
proc Rappture::filexfer::init {} {
    variable enabled
    variable commands

    #
    # Look for the exportfile/importfile commands and see if
    # they appear to be working.  If we have both, then this
    # is "enabled".
    #
    foreach op {export import} {
        set prog "${op}file"
        set path [auto_execok $prog]
        if {"" == $path} {
            foreach dir {/apps/filexfer /apps/bin} {
                set p [file join $dir $prog]
                if {[file executable $path]} {
                    set path $p
                    break
                }
            }
        }
        if {[file executable $path]} {
            set commands($op) $path
        } else {
            return 0
        }
    }

    return [set enabled 1]
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::enabled
#
# Clients use this to see if the filexfer stuff is up and running.
# If so, then the GUI will provide "Download..." and other filexfer
# options.  If not, then Rappture must be running within an
# environment that doesn't support it.
# ----------------------------------------------------------------------
proc Rappture::filexfer::enabled {} {
    variable enabled
    return $enabled
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::upload <toolName> <controlList> <callback>
#
# Clients use this to prompt the user to upload a file.  The string
# <toolName> is used to identify the application within the web form.
# The <controlList> is a list of controls that could be uploaded:
#
#   { <id1> <label1> <desc1>  <id2> <label2> <desc2> ... }
#
# The user is prompted for each of the controls in <controlList>.
# If successful, the <callback> is invoked to handle the uploaded
# information.  If anything goes wrong, the same callback is used
# to post errors near the widget that invoked this operation.
# ----------------------------------------------------------------------
proc Rappture::filexfer::upload {tool controlList callback} {
    global env
    variable enabled
    variable commands
    variable job

    if {$enabled} {
        set cmd $commands(import)
        if {"" != $tool} {
            lappend cmd --for "for $tool"
        }

        set dir ~/data/sessions/$env(SESSION)/spool
        set i 0
        foreach {path label desc} $controlList {
            set file [file join $dir upload[pid]-[incr i]]
            set file2path($file) $path
            set file2label($file) $label
            lappend cmd --label $label $file
        }

        uplevel #0 [list $callback error "Upload starting...\nA web browser page should pop up on your desktop.  Use that form to handle the upload operation.\n\nIf the upload form doesn't pop up, make sure that you're allowing pop ups from this site.  If it still doesn't pop up, you may be having trouble with the version of Java installed for your browser.  See our Support area for details.\n\nClick anywhere to dismiss this message."]

        set job(output) ""
        set job(error) ""

        set status [catch {eval \
            blt::bgexec ::Rappture::filexfer::job(control) \
              -output ::Rappture::filexfer::job(output) \
              -error ::Rappture::filexfer::job(error) \
              $cmd
        } result]

        if {$status == 0} {
            set changed ""
            set errs ""
            foreach file $job(output) {
                # load the uploaded for this control
                set status [catch {
                    set fid [open $file r]
                    fconfigure $fid -translation binary -encoding binary
                    set info [read $fid]
                    close $fid
                    file delete -force $file
                } result]

                if {$status != 0} {
                    append errs "Error loading data for \"$file2label($file)\":\n$result\n"
                } else {
                    lappend changed $file2path($file)
                    uplevel #0 [list $callback path $file2path($file) data $info]
                }
            }
            if {[llength $changed] == 0} {
                set errs "The form was empty, so none of your controls were changed.  In order to upload a file, you must select the file name--or enter text into the copy/paste area--before pushing the Upload button."
            }
            if {[string length $errs] > 0} {
                uplevel #0 [list $callback error $errs]
            }
        } else {
            uplevel #0 [list $callback error $job(error)]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::download <string> ?<filename>?
#
# Clients use this to send a file off to the user.  The <string>
# is stored in a file called <filename> in the user's spool directory.
# If there is already a file by that name, then the name is modified
# to make it unique.  Once the string has been stored in the file,
# a message is sent to all clients listening, letting them know
# that the file is available.
#
# If anything goes wrong, this function returns a string that should
# be displayed to the user to explain the problem.
# ----------------------------------------------------------------------
proc Rappture::filexfer::download {string {filename "output.txt"}} {
    global env
    variable enabled
    variable commands
    variable job

    if {$enabled} {
        # make a spool directory, if we don't have one already
        set dir ~/data/sessions/$env(SESSION)/spool
        if {![file exists $dir]} {
            catch {file mkdir $dir}
        }

        if {[file exists [file join $dir $filename]]} {
            #
            # Find a similar file name that doesn't conflict
            # with an existing file:  e.g., output2.txt
            #
            set root [file rootname $filename]
            set ext [file extension $filename]
            set counter 2
            while {1} {
                set filename "$root$counter$ext"
                if {![file exists [file join $dir $filename]]} {
                    break
                }
                incr counter
            }
        }

        #
        # Save the file in the spool directory, then have it
        # exported.
        #
        if {[catch {
            set file [file join $dir $filename]
            set fid [open $file w]
            fconfigure $fid -encoding binary -translation binary
            puts -nonewline $fid $string
            close $fid
        } result]} {
            return $result
        }

        set job(output) ""
        set job(error) ""

        set status [catch {blt::bgexec ::Rappture::filexfer::job(control) \
            -output ::Rappture::filexfer::job(output) \
            -error ::Rappture::filexfer::job(error) \
            $commands(export) --timeout 300 --delete $file} result]

        if {$status != 0} {
            return $Rappture::filexfer::job(error)
        }
    }
    return ""
}
