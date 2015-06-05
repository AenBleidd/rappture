# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: TextResult - Log output for ResultSet
#
#  This widget is used to show text output in a ResultSet.  The log
#  output from a tool, for example, is rendered as a TextResult.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *TextResult.width 4i widgetDefault
option add *TextResult.height 4i widgetDefault
option add *TextResult.textBackground white widgetDefault
option add *TextResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault
option add *TextResult.textFont \
    -*-courier-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::TextResult {
    inherit itk::Widget

    constructor {args} {
        # defined below
    }
    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} {
        # do nothing
    }
    public method download {option args}

    public method select {option args}
    public method find {option}
    public method popup {option args}

    private variable _dataobj ""  ;# data object currently being displayed
    private variable _raised      ;# maps all data objects => -raise param
}

itk::usual TextResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    #
    # CONTROL BAR with find/select functions
    #
    itk_component add controls {
        frame $itk_interior.cntls
    }
    pack $itk_component(controls) -side bottom -fill x -pady {4 0}

    itk_component add selectall {
        button $itk_component(controls).selall -text "Select All" \
            -command [itcl::code $this select all]
    }
    pack $itk_component(selectall) -side right -fill y

    itk_component add findl {
        label $itk_component(controls).findl -text "Find:"
    }
    pack $itk_component(findl) -side left

    itk_component add find {
        entry $itk_component(controls).find -width 20
    }
    pack $itk_component(find) -side left

    itk_component add finddown {
        button $itk_component(controls).finddown \
            -image [Rappture::icon finddn] \
            -relief flat -overrelief raised \
            -command [itcl::code $this find down]
    } {
        usual
        ignore -relief
    }
    pack $itk_component(finddown) -side left

    itk_component add findup {
        button $itk_component(controls).findup \
            -image [Rappture::icon findup] \
            -relief flat -overrelief raised \
            -command [itcl::code $this find up]
    } {
        usual
        ignore -relief
    }
    pack $itk_component(findup) -side left

    itk_component add findstatus {
        label $itk_component(controls).finds -width 10 -anchor w
    }
    pack $itk_component(findstatus) -side left -expand yes -fill x

    # shortcut for Return in search field
    bind $itk_component(find) <KeyPress-Return> "
        $itk_component(finddown) configure -relief sunken
        update idletasks
        after 200
        $itk_component(finddown) configure -relief flat
        $itk_component(finddown) invoke
    "
    bind $itk_component(find) <KeyPress> \
        [itcl::code $this find reset]

    #
    # TEXT AREA
    #
    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add text {
        text $itk_component(scroller).text -width 1 -height 1 -wrap none
    } {
        usual
        rename -background -textbackground textBackground Background
        rename -font -textfont textFont Font
    }
    $itk_component(scroller) contents $itk_component(text)
    $itk_component(text) configure -state disabled

    $itk_component(text) tag configure ERROR -foreground red

    itk_component add emenu {
        menu $itk_component(text).menu -tearoff 0
    } {
        ignore -tearoff
    }
    $itk_component(emenu) add command \
        -label "Select All" -accelerator "Ctrl+A" \
        -command [itcl::code $this select all]
    $itk_component(emenu) add command \
        -label "Select None" -accelerator "Esc" \
        -command [itcl::code $this select none]
    bind $itk_component(text) <<PopupMenu>> \
        [itcl::code $this popup menu emenu %X %Y]
    $itk_component(emenu) add command \
        -label "Copy" -accelerator "Ctrl+C" \
        -command [list event generate $itk_component(text) <<Copy>>]

    bind $itk_component(text) <Control-KeyPress-a> \
        [list $itk_component(emenu) invoke "Select All"]
    bind $itk_component(text) <Control-KeyPress-c> \
        [list $itk_component(emenu) invoke "Copy"]
    bind $itk_component(text) <Escape> \
        [list $itk_component(emenu) invoke "Select None" ]
    bind $itk_component(text) <Enter> [list ::focus $itk_component(text)]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  If the optional
# <settings> are specified, then the are applied to the data.  Allowed
# settings are -color and -brightness, -width, -linestyle and -raise.
# (Many of these are ignored.)
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::add {dataobj {settings ""}} {
    array set params {
        -color ""
        -brightness ""
        -width ""
        -linestyle ""
        -raise 0
        -description ""
        -param ""
    }
    array set params $settings

    set replace 0
    if {"" != $dataobj} {
        set _raised($dataobj) $params(-raise)
        if {"" == $_dataobj} {
            set replace 1
        } elseif {$_raised($_dataobj) == 0 && $params(-raise)} {
            set replace 1
        }
    }

    if {$replace} {
        $itk_component(text) configure -state normal
        $itk_component(text) delete 1.0 end

        if {[$dataobj element -as type] == "log"} {
            # log output -- remove special =RAPPTURE-???=> messages
            set message [$dataobj get]
            while {[regexp -indices \
                       {=RAPPTURE-([a-zA-Z]+)=>([^\n]*)(\n|$)} $message \
                        match type mesg]} {

                foreach {i0 i1} $match break
                set first [string range $message 0 [expr {$i0-1}]]
                if {[string length $first] > 0} {
                    $itk_component(text) insert end $first
                }

                foreach {t0 t1} $type break
                set type [string range $message $t0 $t1]
                foreach {m0 m1} $mesg break
                set mesg [string range $message $m0 $m1]
                if {[string length $mesg] > 0
                       && $type != "RUN" && $type != "PROGRESS"} {
                    $itk_component(text) insert end $mesg $type
                    $itk_component(text) insert end \n $type
                }
                set message [string range $message [expr {$i1+1}] end]
            }

            if {[string length $message] > 0} {
                $itk_component(text) insert end $message
                if {[$itk_component(text) get end-2char] != "\n"} {
                    $itk_component(text) insert end "\n"
                }
            }
        } elseif {[$dataobj element -as type] == "string"} {
            # add string values
            set data [$dataobj get current]
            if {[Rappture::encoding::is binary $data]} {
                set data [Rappture::utils::hexdump -lines 1000 $data]
            }
            $itk_component(text) insert end $data
        } else {
            # any other string output -- add it directly
            $itk_component(text) insert end [$dataobj get]
        }
        $itk_component(text) configure -state disabled

        set _dataobj $dataobj
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::get {} {
    return $_dataobj
}

# ----------------------------------------------------------------------
# USAGE: delete ?<curve1> <curve2> ...?
#
# Clients use this to delete a curve from the plot.  If no curves
# are specified, then all curves are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::delete {args} {
    if {[llength $args] == 0} {
        # delete everything
        catch {unset _raised}
        set _dataobj ""
        $itk_component(text) configure -state normal
        $itk_component(text) delete 1.0 end
        $itk_component(text) configure -state disabled
    } else {
        # delete these specific objects
        foreach obj $args {
            catch {unset _raised($obj)}
            if {$obj == $_dataobj} {
                set _dataobj ""
                $itk_component(text) configure -state normal
                $itk_component(text) delete 1.0 end
                $itk_component(text) configure -state disabled
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: scale ?<curve1> <curve2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <curve> objects.  This
# accounts for all curves--even those not showing on the screen.
# Because of this, the limits are appropriate for all curves as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::scale {args} {
    # nothing to do for text
}

# ----------------------------------------------------------------------
# USAGE: download coming
# USAGE: download controls <downloadCommand>
# USAGE: download now
#
# Clients use this method to create a downloadable representation
# of the plot.  Returns a list of the form {ext string}, where
# "ext" is the file extension (indicating the type of data) and
# "string" is the data itself.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            if {"" == $_dataobj} {
                return ""
            }
            if {[$_dataobj element -as type] == "log"} {
                set val [$itk_component(text) get 1.0 end]
            } elseif {[$_dataobj element -as type] == "string"} {
                set val [$_dataobj get current]
            } else {
                set val [$_dataobj get]
            }

            set ext [$_dataobj get filetype]
            if {"" == $ext} {
                if {[Rappture::encoding::is binary $val]} {
                    set ext ".dat"
                } else {
                    set ext ".txt"
                }
            }
            return [list $ext $val]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: select all
#
# Handles various selection operations within the text.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::select {option args} {
    switch -- $option {
        all {
            $itk_component(text) tag add sel 1.0 end
        }
        none {
            if { [$itk_component(text) tag ranges "sel"] != "" } {
                selection clear
            }
        }
        default {
            error "bad option \"$option\": should be all or none"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: find up
# USAGE: find down
# USAGE: find reset
#
# Handles various find operations within the text.  These find a
# bit of text and highlight it within the widget.  The find operation
# starts from the end of the currently highlighted text, or from the
# beginning if there is no highlight.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::find {option} {
    # handle the reset case...
    $itk_component(find) configure \
        -background $itk_option(-background) \
        -foreground $itk_option(-foreground)
    $itk_component(findstatus) configure -text ""

    if {$option == "reset"} {
        return
    }

    # handle the up/down cases...
    set t $itk_component(text)
    set pattern [string trim [$itk_component(find) get]]

    if {"" == $pattern} {
        $itk_component(find) configure -background red -foreground white
        $itk_component(findstatus) configure -text "<< Enter a search string"
        return
    }

    # find the starting point for the search
    set seln [$t tag nextrange sel 1.0]
    if {$seln == ""} {
        set t0 1.0
        set t1 end
    } else {
        foreach {t0 t1} $seln break
    }
    $t tag remove sel 1.0 end

    # search up or down
    switch -- $option {
        up {
            set start [$t index $t0-1char]
            set next [$t search -backwards -nocase -- $pattern $start]
            Rappture::Logger::log text find -up $pattern
        }
        down {
            set start [$t index $t1+1char]
            set next [$t search -forwards -nocase -- $pattern $start]
            Rappture::Logger::log text find -down $pattern
        }
    }

    if {"" != $next} {
        set len [string length $pattern]
        $t tag add sel $next $next+${len}chars
        $t see $next
        set lnum [lindex [split $next .] 0]
        set lines [lindex [split [$t index end] .] 0]
        set percent [expr {round(100.0*$lnum/$lines)}]
        set status "line $lnum   --$percent%--"
    } else {
        set status "Not found"
        $itk_component(find) configure -background red -foreground white
    }
    $itk_component(findstatus) configure -text $status
}

# ----------------------------------------------------------------------
# USAGE: _popup menu <which> <X> <Y>
#
# Used internally to manage edit operations.
# ----------------------------------------------------------------------
itcl::body Rappture::TextResult::popup {option args} {
    switch -- $option {
        menu {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"_popup $option which x y\""
            }
            set mname [lindex $args 0]
            set x [lindex $args 1]
            set y [lindex $args 2]
            tk_popup $itk_component($mname) $x $y
        }
        default {
            error "bad option \"$option\": should be menu"
        }
    }
}
