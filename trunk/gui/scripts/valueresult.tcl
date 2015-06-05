# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ValueResult - Log output for ResultSet
#
#  This widget is used to show text output in a ResultSet.  The log
#  output from a tool, for example, is rendered as a ValueResult.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *ValueResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

itcl::class Rappture::ValueResult {
    inherit itk::Widget

    constructor {args} { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args}
    public method parameters {title args} { # do nothing }
    public method download {option args}

    protected method _rebuild {}

    private variable _dispatcher "" ;# dispatcher for !events

    private variable _dlist ""    ;# list of data objects being displayed
    private variable _dobj2color  ;# maps data object => color
    private variable _dobj2raise  ;# maps data object => raise flag 0/1
    private variable _dobj2desc   ;# maps data object => description
}

itk::usual ValueResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::constructor {args} {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this _rebuild]; list"

    itk_component add scroller {
        Rappture::Scroller $itk_interior.scroller \
            -xscrollmode auto -yscrollmode auto
    }
    pack $itk_component(scroller) -expand yes -fill both

    itk_component add html {
        Rappture::HTMLviewer $itk_component(scroller).html
    }
    $itk_component(scroller) contents $itk_component(html)

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
itcl::body Rappture::ValueResult::add {dataobj {settings ""}} {
    array set params {
        -color ""
        -brightness 0
        -width ""
        -linestyle ""
        -raise 0
        -description ""
        -param ""
    }
    array set params $settings

    if {$params(-color) == "auto" || $params(-color) == "autoreset"} {
        # can't handle -autocolors yet
        set params(-color) black
    }

    if {"" != $dataobj} {
        # find the value and assign it with the proper coloring
        if {"" != $params(-color) && "" != $params(-brightness)
              && $params(-brightness) != 0} {
            set params(-color) [Rappture::color::brightness \
                $params(-color) $params(-brightness)]
        }

        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos < 0} {
            lappend _dlist $dataobj
            set _dobj2color($dataobj) $params(-color)
            set _dobj2raise($dataobj) $params(-raise)
            set _dobj2desc($dataobj) $params(-description)
            $_dispatcher event -idle !rebuild
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::get {} {
    # put the dataobj list in order according to -raise options
    set dlist $_dlist
    foreach obj $dlist {
        if {[info exists _dobj2raise($obj)] && $_dobj2raise($obj)} {
            set i [lsearch -exact $dlist $obj]
            if {$i >= 0} {
                set dlist [lreplace $dlist $i $i]
                lappend dlist $obj
            }
        }
    }
    return $dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<curve1> <curve2> ...?
#
# Clients use this to delete a curve from the plot.  If no curves
# are specified, then all curves are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified objects
    set changed 0
    foreach obj $args {
        set pos [lsearch -exact $_dlist $obj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _dobj2color($obj)}
            catch {unset _dobj2raise($obj)}
            catch {unset _dobj2desc($obj)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        $_dispatcher event -idle !rebuild
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
itcl::body Rappture::ValueResult::scale {args} {
    # nothing to do for values
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
itcl::body Rappture::ValueResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            if {[llength $_dlist] == 1} {
                set lstr [$_dlist get about.label]
                set mesg "$lstr [$_dlist get current]"
            } else {
                set mesg ""
                foreach obj $_dlist {
                    set lstr [$obj get about.label]
                    append mesg "$lstr [$obj get current]\n"
                    if {[string length $_dobj2desc($obj)] > 0} {
                        foreach line [split $_dobj2desc($obj) \n] {
                            append mesg " * $line\n"
                        }
                        append mesg "\n"
                    }
                }
            }
            return [list .txt $mesg]
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Used internally to rebuild the contents of this widget
# whenever the data within it changes.  Shows the value
# for the topmost data object in its associated color.
# ----------------------------------------------------------------------
itcl::body Rappture::ValueResult::_rebuild {} {
    set html "<html><body>"

    set obj [lindex $_dlist 0]
    if {"" != $obj} {
        set label [$obj get about.label]
        if {"" != $label && [string index $label end] != ":"} {
            append label ":"
        }
        append html "<h3>$label</h3>\n"
    }

    foreach obj $_dlist {
        if {$_dobj2raise($obj)} {
            set bold0 "<b>"
            set bold1 "</b>"
            set bg "background:#ffffcc; border:1px solid #cccccc;"
        } else {
            set bold0 ""
            set bold1 ""
            set bg ""
        }
        if {$_dobj2color($obj) != ""} {
            set color0 "<font style=\"color: $_dobj2color($obj)\">"
            set color1 "</font>"
        } else {
            set color0 ""
            set color1 ""
        }

        append html "<div style=\"margin:8px; padding:4px; $bg\">${bold0}${color0}[$obj get current]${color1}${bold1}"
        if {$_dobj2raise($obj) && [string length $_dobj2desc($obj)] > 0} {
            foreach line [split $_dobj2desc($obj) \n] {
                append html "<li style=\"margin-left:12px;\">$line</li>\n"
            }
        }
        append html "</div>"
    }
    append html "</body></html>"
    $itk_component(html) load $html
}
