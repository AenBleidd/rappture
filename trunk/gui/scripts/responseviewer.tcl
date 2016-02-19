# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: ResponseViewer - Response Surface (surogate Model) Viewer
#
# Gets response data from PUQ and displayes it using vtk.
# ======================================================================
#  Copyright (c) 2004-2014  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT

option add *ResponseViewer.width 4i widgetDefault
option add *ResponseViewer*cursor crosshair widgetDefault
option add *ResponseViewer.height 4i widgetDefault
option add *ResponseViewer.foreground black widgetDefault
option add *ResponseViewer.controlBackground gray widgetDefault
option add *ResponseViewer.controlDarkBackground #999999 widgetDefault
option add *ResponseViewer.plotBackground black widgetDefault
option add *ResponseViewer.plotForeground white widgetDefault
option add *ResponseViewer.font \
    -*-helvetica-medium-r-normal-*-12-* widgetDefault


itcl::class Rappture::ResponseViewer {
    inherit itk::Widget

    itk_option define -plotforeground plotForeground Foreground ""
    itk_option define -plotbackground plotBackground Background ""

    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method add {dataobj {settings ""}}
    public method delete {args}
    public method download {option args}

    private variable _response
    private variable _settings
    private variable _label
    private variable _dobj ""
    private method _combobox_cb {}
    private method _plot_response {var1 var2}
}

itk::usual ResponseViewer {
    keep -background -foreground -cursor -font
    keep -plotbackground -plotforeground
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResponseViewer::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) yes

    itk_component add frame {
        frame $itk_interior.frame
    }

    itk_component add frame2 {
        frame $itk_interior.frame2
    }
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::ResponseViewer::destructor {} {

}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a dataobj to the plot.
# ----------------------------------------------------------------------
itcl::body Rappture::ResponseViewer::add {uqobj {settings ""}} {
    #puts "ResponseViewer ADD $uqobj ($settings)"
    set _response [$uqobj response]
    set vars [$uqobj variables]
    set labs [$uqobj vlabels]
    set _settings $settings
    set _label [$uqobj label]

    if {[llength $vars] > 1} {
        set cb1 [Rappture::Combobox $itk_interior.frame2.cb1 -width 10 -editable no]
        set cb2 [Rappture::Combobox $itk_interior.frame2.cb2 -width 10 -editable no]

        foreach v $vars l $labs {
            $cb1 choices insert end $v $l
            $cb2 choices insert end $v $l
        }
        $cb1 value [$cb1 label [lindex $vars 0]]
        $cb2 value [$cb2 label [lindex $vars 1]]
        bind $cb1 <<Value>> [itcl::code $this _combobox_cb]
        bind $cb2 <<Value>> [itcl::code $this _combobox_cb]

        set vs [label $itk_interior.frame2.desc -text "vs"]
        grid $cb1 $vs $cb2
    } else {
        set label [lindex $labs 0]
        set lab [label $itk_interior.frame2.desc -text "Response for $label"]
        grid $lab
    }

    set rmsep [$uqobj rmse]
    if {$rmsep > 10} {
        set color red
    } elseif {$rmsep > 5} {
        set color orange
    } elseif {$rmsep > 2} {
        set color yellow
    } else {
        set color black
    }
    set rmse [label $itk_interior.frame2.rmse -text "RMSE: $rmsep%" -foreground $color]

    grid $rmse -row 0 -column 4 -sticky e
    grid $itk_component(frame) -row 0 -column 0 -sticky nsew
    grid $itk_component(frame2) -row 1 -column 0 -sticky nsew
    grid columnconfigure $itk_component(frame) 0 -weight 1
    grid columnconfigure $itk_component(frame2) 3 -weight 1
    grid rowconfigure $itk_component(frame) 0 -weight 1
    grid columnconfigure $itk_component(hull) 0 -weight 1
    grid rowconfigure $itk_component(hull) 0 -weight 1

    if {[llength $vars] > 1} {
        _combobox_cb
    } else {
        if {$_dobj != ""} {
            destroy $itk_component(frame).field
            destroy _dobj
            set _dobj ""
        }
        set vname [lindex $vars 0]
        after idle [itcl::code $this _plot_response $vname $vname]
    }
}

itcl::body Rappture::ResponseViewer::_plot_response {var1 var2} {
    #puts "plotting $var1 vs $var2"

    set fname response_result.xml

    set rs $_response
    if {[catch {exec get_response.py $fname $rs $var1 $var2 $_label} res]} {
        set fp [open "response.err" r]
        set rdata [read $fp]
        close $fp
        puts "Surrogate Model failed: $res\n$rdata"
        error "Sampling of Surrogate Model failed: $res\n$rdata"
        # FIXME: Maybe put in text box instead of error()?
        return
    }

    set xmlobj [Rappture::library $fname]
    #puts "xmlobj=$xmlobj"
    set w $itk_component(frame).field

    if {$var1 == $var2} {
        set path "output.curve(scatter)"
        set _dobj [Rappture::Curve ::#auto $xmlobj $path]
        Rappture::XyResult $w
        $w add $_dobj {-color red}

        set path "output.curve(response)"
        set _dobj [Rappture::Curve ::#auto $xmlobj $path]

    } else {
        set path "output.field(f2d)"
        set _dobj [Rappture::Field ::#auto $xmlobj $path]
        Rappture::FieldResult $w -mode heightmap
    }
    $w add $_dobj $_settings
    $w scale $_dobj
    #puts "added _dobj"
    #puts "w=$w"
    pack $w -expand yes -fill both
}

itcl::body Rappture::ResponseViewer::_combobox_cb {} {
    if {$_dobj != ""} {
        destroy $itk_component(frame).field
        destroy _dobj
        set _dobj ""
    }
    set var1 [$itk_component(frame2).cb1 current]
    set var2 [$itk_component(frame2).cb2 current]
    _plot_response $var1 $var2
}


itcl::body Rappture::ResponseViewer::download {option args} {
    $itk_component(frame).field download $option $args
}