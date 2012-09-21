# ----------------------------------------------------------------------
#  EDITOR: programming language
#
#  Used within the Instant Rappture builder to edit the "language"
#  which is an attribute of the tool.  This control offers any of
#  the built-in template languages, or the "custom" option (which
#  allows the user to specify the tool invocation command).
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrLanguage {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        Rappture::Combobox $win.val -width 12 -editable no
        pack $win.val -side left
        foreach name [lsort [RapptureBuilder::templates::languages]] {
            $win.val choices insert end $name $name
        }
        $win.val choices insert end "--" "Other"

        bind $win.val <<Value>> [itcl::code $this _react]

        Rappture::Tooltip::for $win.val $params(-tooltip)

        frame $win.custom
        label $win.custom.l -text "Command:"
        pack $win.custom.l -side left
        entry $win.custom.val
        pack $win.custom.val -side left -expand yes -fill x
        Rappture::Tooltip::for $win.custom.val "Enter the command line that Rappture should invoke to run your simulation program.  Include @driver somewhere on this line; it will be replaced with the name of the Rappture driver file (usually passed in as the first argument on the command line).  If you include @tool, it will be replaced with the name of the directory where the tool.xml file is installed.  Instead of hard-coding your program path, you can express it as a relative path from @tool."

        set _win $win
    }

    public method check {} {
        if {[$_win.val translate [$_win.val value]] eq "--"} {
            if {[string trim [$_win.custom.val get]] eq ""} {
                return [list error "Must specify a custom invocation command for this tool"]
            }
        }
        return ""
    }

    public method load {val} {
        # If the value is !language, then it may have been saved earlier
        if {[string index $val 0] eq "!"} {
            $_win.val value [string range $val 1 end]
            $_win.custom.val delete 0 end
            return
        }

        # It's hard to reload a real command from a tool.xml file.
        # Scan through all languages and treat @@FILENAME@@ as a wildcard.
        # See if any of the languages match.
        foreach lang [RapptureBuilder::templates::languages] {
            set cmd [RapptureBuilder::templates::generate command \
                -language $lang -macros {@@FILENAME@@ * @@FILEROOT@@ *}]

            if {[string match $cmd $val]} {
                $_win.val value $lang
                $_win.custom.val delete 0 end
                return
            }
        }
        # must be a custom command
        $_win.val value "Other"
        $_win.custom.val delete 0 end
        $_win.custom.val insert end $val
    }

    public method save {var} {
        upvar $var value
        set str [$_win.val translate [$_win.val value]]
        if {$str eq "--"} {
            # save custom command
            set value [string trim [$_win.custom.val get]]
        } else {
            # save !language and substitute proper command later
            set value "!$str"
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.val
    }

    public proc import {xmlobj path} {
        return [$xmlobj get $path]
    }

    public proc export {xmlobj path value} {
        set value [string trim $value]
        if {$value ne ""} {
            $xmlobj put $path $value
        }
    }

    # invoked whenever the user changes the language choice
    private method _react {} {
        if {[$_win.val translate [$_win.val value]] eq "--"} {
            # custom option -- put up extra controls
            pack $_win.custom -side left -expand yes -fill x
        } else {
            pack forget $_win.custom
        }
    }

    private variable _win ""       ;# containing frame
}
