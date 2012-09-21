# ----------------------------------------------------------------------
#  EDITOR: file attribute values
#
#  Used within the Instant Rappture builder to set file paths.
#  Rappture tries to avoid files, but it's okay for things like the
#  <note> element, which points to documentation.
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrFile {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }
        label $win.file -text "select a file" -width 10 -anchor e -fg gray60
        pack $win.file -side left
        Rappture::Tooltip::for $win.file $params(-tooltip)

        button $win.getfile -text "Choose..." -command [itcl::code $this _getfile]
        pack $win.getfile -side left

        set _win $win
    }

    public method load {val} {
        if {[string length $val] > 0} {
            $_win.file configure -text $val -foreground black

            # if the file name is long, then set up to clip and show the tail
            set w [expr {[winfo width $_win]-[winfo width $_win.getfile]}]
            set fn [$_win.file cget -font]
            if {[font measure $fn $val] > $w} {
                $_win.file configure -width 10
                pack $_win.file -expand yes -fill x
            } else {
                $_win.file configure -width 0
                pack $_win.file -expand no
            }
        }
    }

    public method check {} {
        set fname [$_win.file cget -text]
        if {"" != $fname && ![string equal $fname "select a file"]
              && ![regexp {^file://(.+)} $fname]} {
            return [list error "Bad value \"$fname\": should be file://filename"]
        }
    }

    public method save {var} {
        upvar $var value

        set err [lindex [check] 1]
        if {[string length $err] > 0} {
            Rappture::Tooltip::cue $_win $err
            return 0
        }

        set value [$_win.file cget -text]
        if {[string equal $value "select a file"]} {
            set value ""
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.getfile
    }

    public proc import {xmlobj path} {
        # trivial import -- just return info as-is from XML
        return [$xmlobj get $path]
    }

    public proc export {xmlobj path value} {
        global LastToolXmlFile

        # save a relative path to this file
        if {[regexp {^file://(.+)} $value match fname]} {
            if {[file pathtype $fname] == "absolute"} {
                set dir [file dirname $LastToolXmlFile]
                if {[string match $dir/* $fname]} {
                    set len [string length "$dir/"]
                    set value "file://[string range $fname $len end]"
                } elseif {"" != $LastToolXmlFile} {
                    error "Can't find file \"$fname\" relative to tool.xml file.  All items for a tool should be bundled together in the same directory, so they can be easily relocated later."
                }
            }
        }

        if {[string length $value] > 0} {
            $xmlobj put $path $value
        }
    }

    protected method _getfile {} {
        set fname [tk_getOpenFile -title "Rappture: Select HTML File" -defaultextension .html -filetypes { {{HTML files} {.html .htm}} {{All files} *} }]
        if {"" != $fname} {
            $_win.file configure -text "file://$fname" -foreground black

            # if the file name is long, then set up to clip and show the tail
            set w [expr {[winfo width $_win]-[winfo width $_win.getfile]}]
            set fn [$_win.file cget -font]
            if {[font measure $fn $fname] > $w} {
                $_win.file configure -width 10
                pack $_win.file -expand yes -fill x
            } else {
                $_win.file configure -width 0
                pack $_win.file -expand no
            }
        }
    }

    private variable _win ""       ;# containing frame
}
