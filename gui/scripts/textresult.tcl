# ----------------------------------------------------------------------
#  COMPONENT: TextResult - Log output for ResultSet
#
#  This widget is used to show text output in a ResultSet.  The log
#  output from a tool, for example, is rendered as a TextResult.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *TextResult.width 4i widgetDefault
option add *TextResult.height 4i widgetDefault
option add *TextResult.font \
    -*-courier-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::TextResult {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}

    set _dataobj ""  ;# data object currently being displayed
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

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add text {
        text $itk_component(scroller).text -width 1 -height 1 -wrap none
    }
    $itk_component(scroller) contents $itk_component(text)
    $itk_component(text) configure -state disabled

    $itk_component(text) tag configure ERROR -foreground red

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
        -raise ""
    }
    foreach {opt val} $settings {
        if {![info exists params($opt)]} {
            error "bad setting \"$opt\": should be [join [lsort [array names params]] {, }]"
        }
        set params($opt) $val
    }

    $itk_component(text) configure -state normal
    $itk_component(text) delete 1.0 end

    if {"" != $dataobj} {
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
        } else {
            # any other string output -- add it directly
            $itk_component(text) insert end [$dataobj get]
        }
    }

    $itk_component(text) configure -state disabled
    set _dataobj $dataobj
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
    $itk_component(text) configure -state normal
    $itk_component(text) delete 1.0 end
    $itk_component(text) configure -state disabled
    set _dataobj ""
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
