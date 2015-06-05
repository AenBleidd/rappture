# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: datatableresult - X/Y plot in a ResultSet
#
#  This widget is an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  Use the "add" and
#  "delete" methods to control the dataobjs showing on the plot.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *DataTableResult.width 3i widgetDefault
option add *DataTableResult.height 3i widgetDefault
option add *DataTableResult.gridColor #d9d9d9 widgetDefault
option add *DataTableResult.activeColor blue widgetDefault
option add *DataTableResult.dimColor gray widgetDefault
option add *DataTableResult.controlBackground gray widgetDefault
option add *DataTableResult.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault

option add *DataTableResult*Balloon*Entry.background white widgetDefault

itk::usual TreeView {
    keep -foreground -cursor
}

itcl::class Rappture::DataTableResult {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -activecolor activeColor ActiveColor ""
    itk_option define -dimcolor dimColor DimColor ""

    private variable _tree ""

    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }
    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method scale {args} {
        # Do nothing
    }
    public method snap { w h }
    public method tooltip { desc x y }
    public method parameters {title args} {
        # do nothing
    }
    public method download {option args}

    protected method Rebuild {}

    private variable _dispatcher "" ;# dispatcher for !events
    private variable _dlist ""     ;# list of dataobj objects
    private variable _dataobj2color  ;# maps dataobj => plotting color
    private variable _dataobj2width  ;# maps dataobj => line width
    private variable _dataobj2dashes ;# maps dataobj => BLT -dashes list
    private variable _dataobj2raise  ;# maps dataobj => raise flag 0/1
    private variable _raised    "";
    common _downloadPopup          ;# download options from popup
}

itk::usual DataTableResult {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::constructor {args} {
    if { [catch {
    Rappture::dispatcher _dispatcher
    $_dispatcher register !rebuild
    $_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    array set _downloadPopup {
        format csv
    }
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    set _tree [blt::tree create]
    Rappture::Scroller $itk_interior.scroller \
        -xscrollmode auto -yscrollmode auto

    itk_component add treeview {
        blt::treeview $itk_interior.scroller.tv -borderwidth 1 \
            -highlightthickness 0 -tree $_tree
    } {
        usual
        ignore -borderwidth -highlightthickness
    }
    $itk_component(treeview) style textbox lowered -background grey95
    $itk_component(treeview) style textbox raised -background white
    $itk_interior.scroller contents $itk_component(treeview)
    pack $itk_interior.scroller -fill both -expand yes
    eval itk_initialize $args
    } err] != 0} {
        puts stderr errs=$err
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::destructor {} {
    if { $_tree != "" } {
        blt::tree destroy $_tree
    }
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a dataobj to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -type "line"
        -raise 0
        -linestyle solid
        -description ""
        -param ""
    }
    array set params $settings
    if { $params(-raise) } {
        set _raised $dataobj
    }
    lappend _dlist $dataobj
    $_dispatcher event -idle !rebuild
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::get {} {
    return $_dlist
}

# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }
    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        if { $dataobj == $_raised } {
            set _raised ""
        }
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            set changed 1
        }
    }
    set _raised [lindex $_dlist 0]
    # If anything changed, then rebuild the table
    if {$changed} {
        $_dispatcher event -idle !rebuild
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
itcl::body Rappture::DataTableResult::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            set popup .datatableresultdownload
            if {![winfo exists .datatableresultdownload]} {
                # if we haven't created the popup yet, do it now
                Rappture::Balloon $popup \
                    -title "[Rappture::filexfer::label downloadWord] as..."
                set inner [$popup component inner]
                label $inner.summary -text "" -anchor w
                pack $inner.summary -side top
                radiobutton $inner.datatable \
                    -text "Data as Comma-Separated Values" \
                    -variable Rappture::DataTableResult::_downloadPopup(format) \
                    -value csv
                pack $inner.datatable -anchor w
                button $inner.go -text [Rappture::filexfer::label download] \
                    -command [lindex $args 0]
                pack $inner.go -pady 4
            } else {
                set inner [$popup component inner]
            }
            set num [llength [get]]
            set num [expr {($num == 1) ? "1 result" : "$num results"}]
            $inner.summary configure -text "[Rappture::filexfer::label downloadWord] $num in the following format:"
            update idletasks ;# fix initial sizes
            return $popup
        }
        now {
            set popup .datatableresultdownload
            if {[winfo exists .datatableresultdownload]} {
                $popup deactivate
            }
            switch -- $_downloadPopup(format) {
                csv {
                    # reverse the objects so the selected data appears on top
                    set dlist ""
                    foreach dataobj [get] {
                        set dlist [linsert $dlist 0 $dataobj]
                    }

                    # generate the comma-separated value data for these objects
                    set csvdata ""
                    foreach dataobj $dlist {
                        append csvdata "[string repeat - 60]\n"
                        append csvdata " [$dataobj hints label]\n"
                        if {[info exists _dataobj2desc($dataobj)]
                            && [llength [split $_dataobj2desc($dataobj) \n]] > 1} {
                            set indent "for:"
                            foreach line [split $_dataobj2desc($dataobj) \n] {
                                append csvdata " $indent $line\n"
                                set indent "    "
                            }
                        }
                        append csvdata "[string repeat - 60]\n"

                        append csvdata "[$dataobj hints xlabel], [$dataobj hints ylabel]\n"
                        set first 1
                        foreach comp [$dataobj components] {
                            if {!$first} {
                                # blank line between components
                                append csvdata "\n"
                            }
                            set xv [$dataobj mesh $comp]
                            set yv [$dataobj values $comp]
                            foreach x [$xv values] y [$yv values] {
                                append csvdata [format "%20.15g, %20.15g\n" $x $y]
                            }
                            set first 0
                        }
                        append csvdata "\n"
                    }
                    return [list .txt $csvdata]
                }
            }
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::DataTableResult::Rebuild {} {
    eval $_tree delete [$_tree children 0]

    foreach dataobj $_dlist {
        scan $dataobj "::dataTable%d" suffix
        incr suffix

        set newtree [$dataobj values]
        # Copy the data object's tree onto our tree.
        set dest [$_tree firstchild 0]
        foreach src [$newtree children 0] {
            if { $dest == -1 } {
                set dest [$_tree insert 0]
            }
            foreach {name value} [$newtree get $src] {
                set label "$name \#$suffix"
                $_tree set $dest $label $value
                set labels($label) 1
            }
            set dest [$_tree nextsibling $dest]
        }
    }
    foreach col [$itk_component(treeview) column names] {
        if { [string match "BLT TreeView*" $col] } {
            continue
        }
        $itk_component(treeview) column delete $col
    }
    $itk_component(treeview) column configure treeView -hide yes
    set dataobj [lindex $_dlist 0]
    if { $dataobj != "" } {
        foreach { label description style } [$dataobj columns] {
            foreach c [lsort -dictionary [array names labels $label*]] {
                eval $itk_component(treeview) column insert end [list $c] $style
                $itk_component(treeview) column bind $c <Enter> \
                    [itcl::code $this tooltip $description %X %Y]
                $itk_component(treeview) column bind $c <Leave> \
                    { Rappture::Tooltip::tooltip cancel }
            }
        }
    }
    if { [llength $_dlist] == 1 } {
        foreach { label description style } [$dataobj columns] {
            foreach c [lsort -dictionary [array names labels $label*]] {
                $itk_component(treeview) column configure $c -text $label
            }
        }
    }
    if { $_raised != "" } {
        foreach c [$itk_component(treeview) column names] {
            $itk_component(treeview) column configure $c -style lowered
        }
        scan $_raised "::dataTable%d" suffix
        incr suffix
        foreach { label description style } [$_raised columns] {
            set c "$label \#$suffix"
            $itk_component(treeview) column configure $c -style raised
        }
    }
}

itcl::body Rappture::DataTableResult::snap { w h } {
    set g $itk_component(plot)
    if { $w <= 0 || $h <= 0 } {
        set w [winfo width $g]
        set h [winfo height $g]
    }
    set img [image create picture -width $w -height $h]
    $g snap $img -width $w -height $h
    return $img
}

itcl::body Rappture::DataTableResult::tooltip { description x y } {
    Rappture::Tooltip::text $itk_component(treeview) $description
    Rappture::Tooltip::tooltip pending $itk_component(treeview) @$x,$y
}
