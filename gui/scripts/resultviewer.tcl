# ----------------------------------------------------------------------
#  COMPONENT: ResultViewer - plots a collection of related results
#
#  This widget plots a collection of results that all represent
#  the same quantity, but for various ranges of input values.  It
#  is normally used as part of an Analyzer, to plot the various
#  results selected by a ResultSet.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
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

    constructor {args} { # defined below }
    destructor { # defined below }

    public method add {index xmlobj path}
    public method clear {{index ""}}
    public method value {xmlobj}

    public method plot {option args}
    public method download {option args}

    protected method _plotAdd {xmlobj {settings ""}}
    protected method _fixScale {args}
    protected proc _xml2data {xmlobj path}

    private variable _dispatcher ""  ;# dispatchers for !events
    private variable _mode ""        ;# current plotting mode (xy, etc.)
    private variable _mode2widget    ;# maps plotting mode => widget
    private variable _dataslots ""   ;# list of all data objects in this widget
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
# USAGE: clear ?<index>?
#
# Clears one or all results in this result viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::ResultViewer::clear {{index ""}} {
    if {"" != $index} {
        # clear one result
        if {$index >= 0 && $index < [llength $_dataslots]} {
            set slot [lindex $_dataslots $index]
            foreach dobj $slot {
                itcl::delete object $dobj
            }
            set _dataslots [lreplace $_dataslots $index $index ""]
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
            set params ""
            foreach {index opts} $args {
                if {$index == "params"} {
                    set params $opts
                    continue
                }
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
        ::Rappture::Field {
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
                2D {
                    set mode "contour"
                    if {![info exists _mode2widget($mode)]} {
                        if { [$dataobj isunirect2d] } {
                            if { [$dataobj hints type] == "contour" } {
                                set resultMode "vtkcontour" 
                            } elseif { [info exists env(VTKHEIGHTMAP)] } {
                                set resultMode "vtkheightmap"
                            } else {
                                set resultMode "heightmap"
                            }
                        } else {
                            set resultMode "vtk"
                        }
                        set extents [$dataobj extents]
                        if { $extents > 1 } {
                            set resultMode "flowvis"
                        }
                        set w $itk_interior.contour
                        if { ![winfo exists $w] } {
                            Rappture::Field2DResult $w -mode $resultMode
                        }
                        set _mode2widget($mode) $w
                    }
                }
                3D {
                    set mode "field3D"
                    if {![info exists _mode2widget($mode)]} {
                        switch -- [$dataobj type] {
                            "vtk" {
                                set fmt "vtk"
                            }
                            "opendx" - "dx" - "points-on-mesh" {
                                set fmt "nanovis"
                                set extents [$dataobj extents]
                                if { $extents > 1 } {
                                    set fmt "flowvis"
                                }
                            }
                            "vtkvolume" {
                                set fmt "vtkvolume"
                            }
                            "vtkstreamlines" {
                                set fmt "vtkstreamlines"
                            }
                        }
                        set w $itk_interior.field3D
                        Rappture::Field3DResult $w -mode $fmt
                        set _mode2widget($mode) $w
                    }
                }
                default {
                    error "can't handle \"$dims\" dimension field"
                }
            }
        }
        ::Rappture::Mesh {
            switch -- [$dataobj dimensions] {
                2 {
                    set mode "mesh"
                    if {![info exists _mode2widget($mode)]} {
                        set w $itk_interior.mesh
                        Rappture::MeshResult $w
                        set _mode2widget($mode) $w
                    }
                }
                default {
                    error "can't handle [$dataobj dimensions]D field"
                }
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
    set type [$xmlobj element -as type $path]
    switch -- $type {
        curve {
            return [Rappture::Curve ::#auto $xmlobj $path]
        }
        datatable {
            return [Rappture::DataTable ::#auto $xmlobj $path]
        }
        histogram {
            return [Rappture::Histogram ::#auto $xmlobj $path]
        }
        field {
            return [Rappture::Field ::#auto $xmlobj $path]
        }
        mesh {
            return [Rappture::Mesh ::#auto $xmlobj $path]
        }
        table {
            return [Rappture::Table ::#auto $xmlobj $path]
        }
        image {
            return [Rappture::Image ::#auto $xmlobj $path]
        }
        sequence {
            return [Rappture::Sequence ::#auto $xmlobj $path]
        }
        string - log {
            return [$xmlobj element -as object $path]
        }
        structure {
            return [$xmlobj element -as object $path]
        }
        number - integer - boolean - choice {
            return [$xmlobj element -as object $path]
        }
        drawing3d - drawing {
            return [Rappture::Drawing ::#auto $xmlobj $path]
        }
        time - status {
            return ""
        }
    }
    error "don't know how to plot <$type> data path=$path"
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
