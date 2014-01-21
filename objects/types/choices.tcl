# ----------------------------------------------------------------------
#  EDITOR: choice option values
#
#  Used within the Instant Rappture builder to edit the options
#  associated with a "choice" object.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrChoices {
    constructor {win args} {
        Rappture::getopts args params {
            value -tooltip ""
        }

        frame $win.cntls
        pack $win.cntls -fill x
        button $win.cntls.add -text "Add" -command [itcl::code $this _add]
        pack $win.cntls.add -side left -padx 2 -pady 2
        Rappture::Tooltip::for $win.cntls.add "Adds a new option to the list of choices."
        button $win.cntls.del -text "Delete" -command [itcl::code $this _delete]
        pack $win.cntls.del -side right -padx 2 -pady 2
        Rappture::Tooltip::for $win.cntls.del "Removes an option from the list of choices."

        Rappture::Scroller $win.scrl -height 1.5i \
            -xscrollmode auto -yscrollmode auto
        pack $win.scrl -fill x
        listbox $win.scrl.lbox -selectmode single -exportselection off
        bind $win.scrl.lbox <<ListboxSelect>> [itcl::code $this _edit]
        $win.scrl contents $win.scrl.lbox
        Rappture::Tooltip::for $win.scrl.lbox $params(-tooltip)

        frame $win.editelem -borderwidth 4 -relief flat
        pack $win.editelem -fill x
        grid columnconfigure $win.editelem 1 -weight 1

        label $win.editelem.title -text "Edit selected entry:"
        grid $win.editelem.title -row 0 -column 0 -columnspan 3 -sticky w
        label $win.editelem.l -text "Label:"
        grid $win.editelem.l -row 1 -column 0 -sticky e
        entry $win.editelem.label
        Rappture::Tooltip::for $win.editelem.label "Label for the option that appears in the drop-down list of choices."
        grid $win.editelem.label -row 1 -column 1 -columnspan 2 -sticky ew
        label $win.editelem.v -text "Value:"
        grid $win.editelem.v -row 2 -column 0 -sticky e
        entry $win.editelem.value
        Rappture::Tooltip::for $win.editelem.value "Value for the option reported to your program when this choice is selected."
        grid $win.editelem.value -row 2 -column 1 -sticky ew
        label $win.editelem.vopt -text "(optional)"
        grid $win.editelem.vopt -row 2 -column 2 -sticky e
        label $win.editelem.d -text "Description:"
        grid $win.editelem.d -row 3 -column 0 -sticky e
        Rappture::Scroller $win.editelem.scrl -height 1i -yscrollmode auto
        grid $win.editelem.scrl -row 3 -column 1 -columnspan 2 -sticky nsew
        text $win.editelem.scrl.desc -wrap char
        $win.editelem.scrl contents $win.editelem.scrl.desc
        Rappture::Tooltip::for $win.editelem.scrl.desc "Description of the option that appears in the tooltip when you mouse over the choice when it appears in the combobox."

        bind $win.editelem.label <KeyPress-Return> \
            "[itcl::code $this _finalize label]; focus \[tk_focusNext %W\]"
        bind $win.editelem.label <KeyPress-Tab> \
            "[itcl::code $this _finalize label]; focus \[tk_focusNext %W\]"
        bind $win.editelem.value <KeyPress-Return> "focus \[tk_focusNext %W\]"

        set _win $win
    }
    destructor {
    }

    public method load {opts} {
        set _options ""
        foreach rec $opts {
            set label [lindex $rec 0]
            set value [lindex $rec 1]
            set desc [lindex $rec 2]
            lappend _options [list $label $value $desc]
        }

        # display all options and select the first one for editing
        $_win.scrl.lbox delete 0 end
        foreach rec $_options {
            $_win.scrl.lbox insert end [lindex $rec 0]
        }
        $_win.scrl.lbox selection set 0
        _edit
    }

    public method check {} {
        # should be okay -- labels/values can be anything
    }

    public method save {var} {
        _finalize  ;# apply any pending changes

        upvar $var val
        set val $_options

        return 1
    }

    public method edit {} {
        focus -force $_win.scrl.lbox
    }

    public proc import {xmlobj path} {
        # instead of storing within the given path, we must store a
        # series of <option> tags within the parent on the path
        set path [join [lrange [split $path .] 0 end-1] .]

        # load the info from various <option> tags
        set rval ""
        foreach cpath [$xmlobj children -type option -as path $path] {
            set label [$xmlobj get $cpath.about.label]
            set desc  [$xmlobj get $cpath.about.description]
            set value [$xmlobj get $cpath.value]
            lappend rval [list $label $value $desc]
        }
        return $rval
    }

    public proc export {xmlobj path value} {
        # instead of storing within the given path, we must store a
        # series of <option> tags within the parent on the path
        set path [join [lrange [split $path .] 0 end-1] .]

        # clear out all existing elements
        foreach cpath [$xmlobj children -type option -as path $path] {
            $xmlobj remove $cpath
        }

        # create new elements for all options
        set id 0
        foreach rec $value {
            foreach {label value desc} $rec break
            set elem "option([incr id])"
            $xmlobj put $path.$elem.about.label $label
            if {$desc ne ""} {
                $xmlobj put $path.$elem.about.description $desc
            }
            if {$value ne ""} {
                $xmlobj put $path.$elem.value $value
            }
        }
    }

    public method clear {} {
        set _options ""
        set _editing ""
        $_win.editelem.label delete 0 end
        $_win.editelem.value delete 0 end
        $_win.editelem.scrl.desc delete 1.0 end
    }

    # add a new entry into the list and load it for editing
    private method _add {} {
        _finalize   ;# apply any pending changes
        set name "Option #[incr _count]"
        $_win.scrl.lbox insert end $name
        lappend _options [list $name "" ""]
        $_win.scrl.lbox selection clear 0 end
        $_win.scrl.lbox selection set end
        $_win.scrl.lbox see end
        _edit  ;# edit the new entry
    }

    # delete the selected entry from the list and select the next one
    private method _delete {} {
        set i [$_win.scrl.lbox curselection]
        if {$i ne ""} {
            _finalize clear  ;# clear editing area
            set _options [lreplace $_options $i $i]
            $_win.scrl.lbox delete $i

            # find the next best item to select
            if {$i >= [llength $_options]} {
                set i [expr {[llength $_options]-1}]
            }
            if {$i >= 0} {
                $_win.scrl.lbox selection set $i
                _edit
            } else {
                focus $_win.scrl.lbox
            }
        }
    }

    # edit the selected entry
    private method _edit {} {
        _finalize   ;# apply any pending changes

        set i [lindex [$_win.scrl.lbox curselection] 0]
        if {$i ne ""} {
            set rec [lindex $_options $i]
            $_win.editelem.label insert 0 [lindex $rec 0]
            $_win.editelem.value insert 0 [lindex $rec 1]
            $_win.editelem.scrl.desc insert 1.0 [lindex $rec 2]

            $_win.editelem.label selection from 0
            $_win.editelem.label selection to end
            focus $_win.editelem.label

            set _editing $i
        }
    }

    # finalize any changes for the entry being edited
    private method _finalize {{what clear}} {
        if {$_editing ne "" && $_editing < [llength $_options]} {
            set lval [string trim [$_win.editelem.label get]]
            set vval [string trim [$_win.editelem.value get]]
            set dval [string trim [$_win.editelem.scrl.desc get 1.0 end]]
            set rec [list $lval $vval $dval]
            set _options [lreplace $_options $_editing $_editing $rec]
            $_win.scrl.lbox delete $_editing
            $_win.scrl.lbox insert $_editing $lval
            if {$what ne "clear"} {
                $_win.scrl.lbox selection set $_editing
            }
        }

        # no longer editing anything -- clear out
        if {$what eq "clear"} {
            set _editing ""
            $_win.editelem.label delete 0 end
            $_win.editelem.value delete 0 end
            $_win.editelem.scrl.desc delete 1.0 end
        }
    }

    private variable _win ""       ;# containing frame
    private variable _count 0      ;# counter for unique numbers
    private variable _options ""   ;# list of options: {label value desc}
    private variable _editing ""   ;# index of entry being edited
}
