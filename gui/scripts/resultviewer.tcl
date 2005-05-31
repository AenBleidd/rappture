# ----------------------------------------------------------------------
#  COMPONENT: ResultViewer - plots a collection of related results
#
#  This widget plots a collection of results that all represent
#  the same quantity, but for various ranges of input values.  It
#  is normally used as part of an Analyzer, to plot the various
#  results selected by a ResultSet.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005
#  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk

option add *ResultViewer.width 4i widgetDefault
option add *ResultViewer.height 4i widgetDefault

itcl::class Rappture::ResultViewer {
    inherit itk::Widget

    itk_option define -colors colors Colors ""
    itk_option define -clearcommand clearCommand ClearCommand ""
    itk_option define -simulatecommand simulateCommand SimulateCommand ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {xmlobj path}
    public method replace {index xmlobj path}
    public method clear {}

    public method plot {option args}

    protected method _plotAdd {xmlobj {settings ""}}
    protected method _fixScale {args}
    protected proc _xml2data {xmlobj path}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _mode ""        ;# current plotting mode (xy, etc.)
    private variable _mode2widget    ;# maps plotting mode => widget
    private variable _dataobjs ""    ;# list of all data objects in this widget
    private variable _plotlist ""    ;# list of indices plotted in _dataobjs
}
                                                                                
itk::usual ResultViewer {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    # create a dispatcher for events
    Rappture::dispatcher _dispatcher
    $_dispatcher register !scale
    $_dispatcher dispatch $this !scale \
        [itcl::code $this _fixScale]

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::destructor {} {
    foreach obj $_dataobjs {
        itcl::delete object $obj
    }
}

# ----------------------------------------------------------------------
# USAGE: add <xmlobj> <path>
#
# Adds a new result to this result viewer.  Scans through all existing
# results to look for a difference compared to previous results.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::add {xmlobj path} {
    if {$path != ""} {
        set dobj [_xml2data $xmlobj $path]
    } else {
        set dobj ""
    }
    lappend _dataobjs $dobj

    $_dispatcher event -idle !scale
}

# ----------------------------------------------------------------------
# USAGE: replace <index> <xmlobj> <path>
#
# Stores a new result to this result viewer, overwriting the previous
# result at position <index>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::replace {index xmlobj path} {
    set dobj [lindex $_dataobjs $index]
    if {"" != $dobj} {
        itcl::delete object $dobj
    }

    set dobj [_xml2data $xmlobj $path]
    set _dataobjs [lreplace $_dataobjs $index $index $dobj]

    $_dispatcher event -idle !scale
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears all results in this result viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::clear {} {
    plot clear

    foreach obj $_dataobjs {
        itcl::delete object $obj
    }
    set _dataobjs ""
}

# ----------------------------------------------------------------------
# USAGE: plot add ?<index> <settings> <index> <settings> ...?
# USAGE: plot clear
#
# Used to manipulate the contents of this viewer.  The "plot clear"
# command clears the current viewer.  Data is still stored in the
# widget, but the results are not shown on screen.  The "plot add"
# command adds the data at the specified <index> to the plot.  If
# the optional <settings> are specified, then they are applied
# to the plot; otherwise, default settings are used.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::plot {option args} {
    switch -- $option {
        add {
            foreach {index opts} $args {
                set dobj [lindex $_dataobjs $index]
                if {"" != $dobj} {
                    _plotAdd $dobj $opts
                }
            }
        }
        clear {
            # clear the contents of the current mode
            if {"" != $_mode} {
                $_mode2widget($_mode) delete
            }
            set _plotlist ""
        }
        default {
            error "bad option \"$option\": should be add or clear"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _plotAdd <dataobj> <settings>
#
# Used internally to add a <dataobj> representing some data to
# the plot at the top of this widget.  The data is added to the
# current plot.  Use the "clear" function to clear before adding
# new data.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::_plotAdd {dataobj {settings ""}} {
    switch -- [$dataobj info class] {
        ::Rappture::Curve {
            set mode "xy"
            if {![info exists _mode2widget($mode)]} {
                set w $itk_interior.xy
                Rappture::XyResult $w
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Field {
            switch -- [$dataobj components -dimensions] {
                1D {
                    set mode "xy"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.xy
                        Rappture::XyResult $w
                        set _mode2widget($mode) $w
                    }
                }
                2D {
                    set mode "contour"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.xy
                        Rappture::ContourResult $w
                        set _mode2widget($mode) $w
                    }
                }
                default {
                    error "can't handle [$dataobj components -dimensions] field"
                }
            }
        }
        ::Rappture::LibraryObj {
            switch -- [$dataobj element -as type] {
                log {
                    set mode "log"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.log
                        Rappture::TextResult $w
                        set _mode2widget($mode) $w
                    }
                }
                table {
                    # table for now -- should have a Table object!
                    set mode "energies"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.energies
                        Rappture::EnergyLevels $w
                        set _mode2widget($mode) $w
                    }
                }
            }
        }
        default {
            error "don't know how to plot <$type> data"
        }
    }

    if {$mode != $_mode && $_mode != ""} {
        return  ;# mixing data that doesn't mix -- ignore it!
    }

    # are we plotting in a new mode? then change widgets
    if {$_mode2widget($mode) != [pack slaves $itk_interior]} {
        # remove any current window
        foreach w [pack slaves $itk_interior] {
            pack forget $w
        }
        pack $_mode2widget($mode) -expand yes -fill both

        set _mode $mode
        $_dispatcher event -idle !scale
    }
    $_mode2widget($mode) add $dataobj $settings
}

# ----------------------------------------------------------------------
# USAGE: _fixScale ?<eventArgs>...?
#
# Invoked automatically whenever a new dataset is added to fix the
# overall scales of the viewer.  This makes the visualizer consistent
# across all <dataobj> in this widget, so that it can plot all
# available data.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::_fixScale {args} {
    if {"" != $_mode} {
        eval $_mode2widget($_mode) scale $_dataobjs
    }
}

# ----------------------------------------------------------------------
# USAGE: _xml2data <xmlobj> <path>
#
# Used internally to create a data object for the data at the
# specified <path> in the <xmlobj>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::_xml2data {xmlobj path} {
    set type [$xmlobj element -as type $path]
    switch -- $type {
        curve {
            return [Rappture::Curve ::#auto $xmlobj $path]
        }
        field {
            return [Rappture::Field ::#auto $xmlobj $path]
        }
        table {
            return [$xmlobj element -as object $path]
        }
        log {
            return [$xmlobj element -as object $path]
        }
        time - status {
            return ""
        }
    }
    error "don't know how to plot <$type> data"
}
