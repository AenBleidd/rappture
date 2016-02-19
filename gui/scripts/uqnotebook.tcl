# -*- mode: tcl; indent-tabs-mode: nil -*-

# ----------------------------------------------------------------------
#  COMPONENT: UqNotebook - Tabbed Notebook for UQ results.
#
# ======================================================================
#  AUTHOR:  Martin Hunt, Purdue University
#  Copyright (c) 2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require BLT


itcl::class Rappture::UqNotebook {
    inherit itk::Widget

    itk_option define -gridcolor gridColor GridColor ""
    itk_option define -activecolor activeColor ActiveColor ""
    itk_option define -dimcolor dimColor DimColor ""
    itk_option define -autocolors autoColors AutoColors ""

    constructor {args} {
        # defined below
    }
    destructor {
        # defined below
    }

    public method add {dataobj {settings ""}}
    public method scale {args}
    public method download {option args}
    public method parameters {title args} {}
    public method delete {args}
    public method get {}
    private method _plotit {xmlobj path index settings}
    private variable _index2widget; #_index2widget($sim,$tab) -> $w
    private variable _nblist; # _nblist($sim) returns notebook to pack
    private variable _nbnum 0; # currently displayed notebook number (sim)
}

itk::usual UqNotebook {
    keep -background -cursor -font
}
itk::usual Tabset {
    keep -background -cursor -font
}
itk::usual Tabnotebook {
    keep -background -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::UqNotebook::constructor {args} {
    #Rappture::dispatcher _dispatcher
    #$_dispatcher register !rebuild
    #$_dispatcher dispatch $this !rebuild "[itcl::code $this Rebuild]; list"

    option add hull.width hull.height
    pack propagate $itk_component(hull) yes

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::UqNotebook::destructor {} {
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a dataobj to the plot.  The optional <settings>
# are used to configure the plot.  Allowed settings are -color,
# -brightness, -width, -linestyle and -raise.
# ----------------------------------------------------------------------
itcl::body Rappture::UqNotebook::add {uqobj {settings ""}} {
    #puts "UqNotebook ADD $uqobj ($settings)"
    array set attrs $settings
    array set d [$uqobj get]
    #puts "SIMULATION $attrs(-simulation)"

    # the notebook number is the same as the simulation number
    set nbnum $attrs(-simulation)

    # See if we are already displaying the requested notebook
    if {$nbnum == $_nbnum} {
        return
    }

    if {[info exists _nblist($_nbnum)]} {
        set active [$itk_component(uq_nb_$_nbnum) index select]
        pack forget $itk_component(uq_nb_$_nbnum)
    } else  {
        set active -1
    }

    set _nbnum $nbnum

    if {![info exists _nblist($nbnum)]} {
        #puts "Creating notebook #$nbnum"
        itk_component add uq_nb_$nbnum {
            blt::tabnotebook $itk_interior.uq_nb_$nbnum -tearoff no
        }

        set _nblist($nbnum) $itk_component(uq_nb_$nbnum)

        foreach name [list PDF Probability Sensitivity Response "All Runs" Sequence] {
            if {[info exists d($name)]} {
                foreach {index items} $d($name) break
                $itk_component(uq_nb_$nbnum) insert $index -text $name
                foreach obj $items {
                    foreach {xmlobj path} $obj break
                    _plotit $xmlobj $path $index $settings
                }
            }
        }
    }
    pack $itk_component(uq_nb_$nbnum) -expand yes -fill both
    if {$active >= 0} {
        after 100 $itk_component(uq_nb_$nbnum) select $active
        after idle $itk_component(uq_nb_$nbnum) select $active
    }
}

itcl::body Rappture::UqNotebook::_plotit {xmlobj path index settings} {
    set type [$xmlobj element -as type $path]
    #puts "uqNotebook::_plotit: xmlobj=$xmlobj path=$path type=$type"

    if {[regexp {output.curve\(curve_pdf-[^-]*-([0-9]+)\)} $path match prob]} {
        puts "Probability: $prob"
    } else {
        set prob ""
    }

    array set attrs $settings
    set sim $attrs(-simulation)
    # puts "sim=$sim index=$index"
    set dobj ""

    set w $itk_component(uq_nb_$sim).$index
    switch -- $type {
        response {
            set dobj [Rappture::Response ::#auto $xmlobj $path]
            if {![info exists _index2widget($sim,$index)]} {
                Rappture::ResponseViewer $w
            }
        }
        histogram {
            set dobj [Rappture::Histogram ::#auto $xmlobj $path]
            if {![info exists _index2widget($sim,$index)]} {
                Rappture::HistogramResult $w
            }
        }
        curve {
            set dobj [Rappture::Curve ::#auto $xmlobj $path]
            set type [$dobj hints type]
            if {![info exists _index2widget($sim,$index)]} {
                if {$type == "bars"} {
                    Rappture::BarchartResult $w
                } elseif {$prob != ""} {
                    Rappture::UqCurve $w
                } else {
                    Rappture::XyResult $w
                }
            }
        }
        sequence {
            set dobj [Rappture::Sequence ::#auto $xmlobj $path]
            if {![info exists _index2widget($sim,$index)]} {
                Rappture::SequenceResult $w
            }
        }
        field {
            set dobj [Rappture::Field ::#auto $xmlobj $path]
        }
        mesh {
            set dobj [Rappture::Mesh ::#auto $xmlobj $path]
        }
        default {
            error "Don't know how to plot $type"
        }
    }
    set _index2widget($sim,$index) $w
    if {$dobj != ""} {
        if {$prob != ""} {
            lappend settings -probability $prob
        }
        $w add $dobj $settings
    }
    $itk_component(uq_nb_$sim) tab configure $index -window $w -fill both
}


# ----------------------------------------------------------------------
# USAGE: delete ?<dataobj1> <dataobj2> ...?
#
# Clients use this to delete a dataobj from the plot.  If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::UqNotebook::delete {args} {
    # puts "UqNotebook::delete $args"
    # catch {$itk_component(uq_nb) delete 0 end}

    # foreach index [array name _index2widget] {
    #     puts "destroying index $index"
    #     destroy $_index2widget($index)
    # }
    # foreach dobj $_objlist {
    #     puts "destroying $dobj"
    #     destroy $dobj
    # }
    # unset _index2widget
    # set _objlist {}
}

# ----------------------------------------------------------------------
# USAGE: scale ?<dataobj1> <dataobj2> ...?
#
# Sets the default limits for the overall plot according to the
# limits of the data for all of the given <dataobj> objects.  This
# accounts for all dataobjs--even those not showing on the screen.
# Because of this, the limits are appropriate for all dataobjs as
# the user scans through data in the ResultSet viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::UqNotebook::scale {args} {
    #puts "UqNotebook::scale $args"

    # get all the tabs and call scale
    #set num_tabs [$itk_component(uq_nb_$_nbnum) tab names]
    #for {set i 0} {$i < $num_tabs} {incr i} {
    #    puts "calling scale for tab $i"
    #    eval $_index2widget($_nbnum,$i) scale $args
    #}
}

itcl::body Rappture::UqNotebook::get {} {
    #puts "*************** UqNotebook gets"
}


itcl::body Rappture::UqNotebook::download {option args} {
    #set tab [$itk_component(uq_nb_$_nbnum) index select]
    #puts "UqNotebook::download $option  : $args -> $_index2widget($_nbnum,$tab)"
    #eval $_index2widget($_nbnum,$tab) download $option $args
}
