# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ResultViewer - plots a collection of related results
#
#  This widget plots a collection of results that all represent
#  the same quantity, but for various ranges of input values.  It
#  is normally used as part of an Analyzer, to plot the various
#  results selected by a ResultSet.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk

itcl::class Rappture::ResultViewer {
    inherit itk::Widget

    itk_option define -width width Width 4i
    itk_option define -height height Height 4i
    itk_option define -colors colors Colors ""
    itk_option define -clearcommand clearCommand ClearCommand ""
    itk_option define -simulatecommand simulateCommand SimulateCommand ""

    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method add {index xmlobj path}
    public method clear {{index ""}}
    public method value {xmlobj}

    public method plot {option args}
    public method download {option args}

    protected method _plotAdd {xmlobj {settings ""}}
    protected method _fixScale {args}
    protected method _xml2data {xmlobj path}
    protected method _cleanIndex {index}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _mode ""        ;# current plotting mode (xy, etc.)
    private variable _mode2widget    ;# maps plotting mode => widget
    private variable _dataslots ""   ;# list of all data objects in this widget
    private variable _xml2data       ;# maps xmlobj => data obj in _dataslots
}

itk::usual ResultViewer {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::constructor {args} {
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
    foreach slot $_dataslots {
        foreach obj $slot {
            itcl::delete object $obj
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: add <index> <xmlobj> <path>
#
# Adds a new result to this result viewer at the specified <index>.
# Data is taken from the <xmlobj> object at the <path>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::add {index xmlobj path} {
    set index [_cleanIndex $index]
    set dobj [_xml2data $xmlobj $path]

    #
    # If the index doesn't exist, then fill in empty slots and
    # make it exist.
    #
    for {set i [llength $_dataslots]} {$i <= $index} {incr i} {
        lappend _dataslots ""
    }
    set slot [lindex $_dataslots $index]
    lappend slot $dobj
    set _dataslots [lreplace $_dataslots $index $index $slot]

    $_dispatcher event -idle !scale
}

# ----------------------------------------------------------------------
# USAGE: clear ?<index>|<xmlobj>?
#
# Clears one or all results in this result viewer.  If a particular
# <index> is specified, then all data objects at that index are
# deleted.  If a particular <xmlobj> is specified, then all data
# objects related to that <xmlobj> are removed--regardless of whether
# they reside at one or more indices.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::clear {{index ""}} {
    if {$index ne ""} {
        # clear one result
        if {[catch {_cleanIndex $index} i] == 0} {
            if {$i >= 0 && $i < [llength $_dataslots]} {
                set slot [lindex $_dataslots $i]
                foreach dobj $slot {
                    if {"" != $_mode} {
                        $_mode2widget($_mode) delete $dobj
                    }
                    itcl::delete object $dobj
                }
                set _dataslots [lreplace $_dataslots $i $i ""]
                $_dispatcher event -idle !scale
            }
        } else {
            foreach key [array names _xml2data $index-*] {
                set dobj $_xml2data($key)

                # search for and remove all references to this data object
                for {set n 0} {$n < [llength $_dataslots]} {incr n} {
                    set slot [lindex $_dataslots $n]
                    set pos [lsearch -exact $slot $dobj]
                    if {$pos >= 0} {
                        set slot [lreplace $slot $pos $pos]
                        set _dataslots [lreplace $_dataslots $n $n $slot]
                        $_dispatcher event -idle !scale
                    }
                }

                if {"" != $_mode} {
                    $_mode2widget($_mode) delete $dobj
                }
                # destroy the object and forget it
                itcl::delete object $dobj
                unset _xml2data($key)
            }
        }
    } else {
        # clear all results
        plot clear
        foreach slot $_dataslots {
            foreach dobj $slot {
                itcl::delete object $dobj
            }
        }
        set _dataslots ""
        catch {unset _xml2data}
    }
}

# ----------------------------------------------------------------------
# USAGE: value <xmlobj>
#
# Convenience method for showing a single value.  Loads the value
# into the widget via add/clear, then immediately plots the value.
# This makes the widget consistent with other widgets, such as
# the DeviceEditor, etc.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::value {xmlobj} {
    clear
    if {"" != $xmlobj} {
        add 0 $xmlobj ""
        plot add 0 ""
    }
}

# ----------------------------------------------------------------------
# USAGE: plot add ?<simnum> <settings> <simnum> <settings> ...?
# USAGE: plot clear
#
# Used to manipulate the contents of this viewer.  The "plot clear"
# command clears the current viewer.  Data is still stored in the
# widget, but the results are not shown on screen.  The "plot add"
# command adds the data at the specified <simnum> to the plot.  Each
# <simnum> is the simulation number, like "#1", "#2", "#3", etc.  If
# the optional <settings> are specified, then they are applied
# to the plot; otherwise, default settings are used.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::plot {option args} {
    switch -- $option {
        add {
            set params ""
            foreach {index opts} $args {
                if {$index == "params"} {
                    set params $opts
                    continue
                }

                set index [_cleanIndex $index]
                lappend opts "-simulation" [expr $index + 1]
                set reset "-color autoreset"
                set slot [lindex $_dataslots $index]
                foreach dobj $slot {
                    set settings ""
                    # start with color reset, only for first object in series
                    if {"" != $reset} {
                        set settings $reset
                        set reset ""
                    }
                    # add default settings from data object
                    if {[catch {$dobj hints style} style] == 0} {
                        eval lappend settings $style
                    }
                    if {[catch {$dobj hints type} type] == 0} {
                        if {"" != $type} {
                            eval lappend settings "-type $type"
                        }
                    }
                    # add override settings passed in here
                    eval lappend settings $opts
                    _plotAdd $dobj $settings
                }
            }
            if {"" != $params && "" != $_mode} {
                eval $_mode2widget($_mode) parameters $params
            }
        }
        clear {
            # clear the contents of the current mode
            if {"" != $_mode} {
                $_mode2widget($_mode) delete
            }
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
        ::Rappture::DataTable {
            set mode "datatable"
            if {![info exists _mode2widget($mode)]} {
                set w $itk_interior.datatable
                Rappture::DataTableResult $w
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Drawing {
            set mode "vtkviewer"
            if {![info exists _mode2widget($mode)]} {
                set servers [Rappture::VisViewer::GetServerList "vtkvis"]
                set w $itk_interior.vtkviewer
                Rappture::VtkViewer $w $servers
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Histogram {
            set mode "histogram"
            if {![info exists _mode2widget($mode)]} {
                set w $itk_interior.histogram
                Rappture::HistogramResult $w
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Curve {
            set type [$dataobj hints type]
            set mode "xy"
            if { $type == "bars" } {
                if {![info exists _mode2widget($mode)]} {
                    set w $itk_interior.xy
                    Rappture::BarchartResult $w
                    set _mode2widget($mode) $w
                }
            } else {
                if {![info exists _mode2widget($mode)]} {
                    set w $itk_interior.xy
                    Rappture::XyResult $w
                    set _mode2widget($mode) $w
                }
            }
        }
        ::Rappture::Map {
            if { ![$dataobj isvalid] } {
                return;                 # Ignore invalid map objects.
            }
            set mode "map"
            if {![info exists _mode2widget($mode)]} {
                set servers [Rappture::VisViewer::GetServerList "geovis"]
                set w $itk_interior.$mode
                Rappture::MapViewer $w $servers
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Field {
            if { ![$dataobj isvalid] } {
                return;                 # Ignore invalid field objects.
            }
            set dims [lindex [lsort [$dataobj components -dimensions]] end]
            switch -- $dims {
                1D {
                    set mode "xy"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.xy
                        Rappture::XyResult $w
                        set _mode2widget($mode) $w
                    }
                }
                default {
                    set mode [$dataobj viewer]
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.$mode
                        if { ![winfo exists $w] } {
                            Rappture::FieldResult $w -mode $mode
                        }
                        set _mode2widget($mode) $w
                    }
                }
            }
        }
        ::Rappture::Mesh {
            if { ![$dataobj isvalid] } {
                return;                 # Ignore invalid mesh objects.
            }
            set mode "vtkmeshviewer"
            if {![info exists _mode2widget($mode)]} {
                set servers [Rappture::VisViewer::GetServerList "vtkvis"]
                set w $itk_interior.$mode
                Rappture::VtkMeshViewer $w $servers
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Table {
            set cols [Rappture::EnergyLevels::columns $dataobj]
            if {"" != $cols} {
                set mode "energies"
                if {![info exists _mode2widget($mode)]} {
                    set w $itk_interior.energies
                    Rappture::EnergyLevels $w
                    set _mode2widget($mode) $w
                }
            }
        }
        ::Rappture::LibraryObj {
            switch -- [$dataobj element -as type] {
                string - log {
                    set mode "log"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.log
                        Rappture::TextResult $w
                        set _mode2widget($mode) $w
                    }
                }
                structure {
                    set mode "structure"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.struct
                        Rappture::DeviceResult $w
                        set _mode2widget($mode) $w
                    }
                }
                number - integer {
                    set mode "number"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.number
                        Rappture::NumberResult $w
                        set _mode2widget($mode) $w
                    }
                }
                boolean - choice {
                    set mode "value"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.value
                        Rappture::ValueResult $w
                        set _mode2widget($mode) $w
                    }
                }
            }
        }
        ::Rappture::Image {
            set mode "image"
            if {![info exists _mode2widget($mode)]} {
                set w $itk_interior.image
                Rappture::ImageResult $w
                set _mode2widget($mode) $w
            }
        }
        ::Rappture::Sequence {
            set mode "sequence"
            if {![info exists _mode2widget($mode)]} {
                set w $itk_interior.image
                Rappture::SequenceResult $w
                set _mode2widget($mode) $w
            }
        }
        default {
            error "don't know how to plot <$type> data [$dataobj info class]"
        }
    }

    if {$mode != $_mode && $_mode != ""} {
        set nactive [llength [$_mode2widget($_mode) get]]
        if {$nactive > 0} {
            return  ;# mixing data that doesn't mix -- ignore it!
        }
    }
    # Are we plotting in a new mode? then change widgets
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
        set dlist ""
        foreach slot $_dataslots {
            foreach dobj $slot {
                lappend dlist $dobj
            }
        }
        eval $_mode2widget($_mode) scale $dlist
    }
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
itcl::body Rappture::ResultViewer::download {option args} {
    if {"" == $_mode} {
        return ""
    }
    return [eval $_mode2widget($_mode) download $option $args]
}

# ----------------------------------------------------------------------
# USAGE: _xml2data <xmlobj> <path>
#
# Used internally to create a data object for the data at the
# specified <path> in the <xmlobj>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::_xml2data {xmlobj path} {
    if {[info exists _xml2data($xmlobj-$path)]} {
        return $_xml2data($xmlobj-$path)
    }

    set type [$xmlobj element -as type $path]
    switch -- $type {
        curve {
            set dobj [Rappture::Curve ::#auto $xmlobj $path]
        }
        datatable {
            set dobj [Rappture::DataTable ::#auto $xmlobj $path]
        }
        histogram {
            set dobj [Rappture::Histogram ::#auto $xmlobj $path]
        }
        field {
            set dobj [Rappture::Field ::#auto $xmlobj $path]
        }
        map {
            set dobj [Rappture::Map ::#auto $xmlobj $path]
        }
        mesh {
            set dobj [Rappture::Mesh ::#auto $xmlobj $path]
        }
        table {
            set dobj [Rappture::Table ::#auto $xmlobj $path]
        }
        image {
            set dobj [Rappture::Image ::#auto $xmlobj $path]
        }
        sequence {
            set dobj [Rappture::Sequence ::#auto $xmlobj $path]
        }
        string - log {
            set dobj [$xmlobj element -as object $path]
        }
        structure {
            set dobj [$xmlobj element -as object $path]
        }
        number - integer - boolean - choice {
            set dobj [$xmlobj element -as object $path]
        }
        drawing3d - drawing {
            set dobj [Rappture::Drawing ::#auto $xmlobj $path]
        }
        time - status {
            set dobj ""
        }
        default {
            error "don't know how to plot <$type> data path=$path"
        }
    }

    # store the mapping xmlobj=>dobj so we can find this result later
    if {$dobj ne ""} {
        set _xml2data($xmlobj-$path) $dobj
    }
    return $dobj
}

# ----------------------------------------------------------------------
# USAGE: _cleanIndex <index>
#
# Used internally to create a data object for the data at the
# specified <path> in the <xmlobj>.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::_cleanIndex {index} {
    set index [lindex $index 0]
    if {[regexp {^#([0-9]+)} $index match num]} {
        return [expr {$num-1}]  ;# start from 0 instead of 1
    } elseif {[string is integer -strict $index]} {
        return $index
    }
    error "bad plot index \"$index\": should be 0,1,2,... or #1,#2,#3,..."
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -width
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultViewer::width {
    set w [winfo pixels $itk_component(hull) $itk_option(-width)]
    set h [winfo pixels $itk_component(hull) $itk_option(-height)]
    if {$w == 0 || $h == 0} {
        pack propagate $itk_component(hull) yes
    } else {
        component hull configure -width $w -height $h
        pack propagate $itk_component(hull) no
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -height
# ----------------------------------------------------------------------
itcl::configbody Rappture::ResultViewer::height {
    set h [winfo pixels $itk_component(hull) $itk_option(-height)]
    set w [winfo pixels $itk_component(hull) $itk_option(-width)]
    if {$w == 0 || $h == 0} {
        pack propagate $itk_component(hull) yes
    } else {
        component hull configure -width $w -height $h
        pack propagate $itk_component(hull) no
    }
}
