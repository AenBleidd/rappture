# ----------------------------------------------------------------------
#  COMPONENT: xyplot - X/Y plot for analyzing results
#
#  This widget is an X/Y plot, meant to view line graphs produced
#  as output from the run of a Rappture tool.  It takes a -layout
#  object describing what should be plotted, and an -output object
#  containing the data.  This widget puts it all together, and lets
#  the user explore the results.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *Xyplot.width 4i widgetDefault
option add *Xyplot.height 4i widgetDefault
option add *Xyplot.font \
    -*-helvetica-medium-r-normal-*-*-120-* widgetDefault

itcl::class Rappture::Xyplot {
    inherit itk::Widget

    itk_option define -layout layout Layout ""
    itk_option define -output output Output ""

    constructor {args} { # defined below }
    destructor { # defined below }

    protected method _rebuild {}

    private variable _device ""  ;# device contained in -output
    private variable _path2obj   ;# maps field/curve name => object
}
                                                                                
itk::usual Xyplot {
    keep -background -foreground -cursor -font
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Xyplot::constructor {args} {
    option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add plot {
        blt::graph $itk_interior.plot \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -rightmargin 10
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(plot) -expand yes -fill both

    Blt_ZoomStack $itk_component(plot)
    $itk_component(plot) legend configure -hide yes

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Xyplot::destructor {} {
    foreach name [array names _path2obj] {
        itcl::delete object $_path2obj($name)
    }
    if {$_device != ""} {
        itcl::delete object $_device
    }
}

# ----------------------------------------------------------------------
# USAGE: _rebuild
#
# Called automatically whenever something changes that affects the
# data in the widget.  Clears any existing data and rebuilds the
# widget to display new data.
# ----------------------------------------------------------------------
itcl::body Rappture::Xyplot::_rebuild {} {
    set g $itk_component(plot)
    set layout $itk_option(-layout)
    set run $itk_option(-output)

    # first clear out the widget
    eval $g element delete [$g element names]
    $g axis configure y -min "" -max ""

    foreach name [array names _path2obj] {
        itcl::delete object $_path2obj($name)
    }
    catch {unset _path2obj}

    # now extract any new data and plot it
    if {$layout != "" && $run != ""} {
        set count 0
        foreach item [$layout children] {
          switch -glob -- $item {
            title {
                $g configure -title [$layout get title]
            }
            legend {
                set where [$layout get legend]
                if {$where == "off"} {
                    $itk_component(plot) legend configure -hide yes
                } else {
                    $itk_component(plot) legend configure -hide no \
                        -position plotarea -anchor $where -borderwidth 0
                }
            }
            xaxis {
                $g xaxis configure -title [$layout get xaxis]
            }
            yaxis {
                $g yaxis configure -title [$layout get yaxis]
            }
            field* {
              set name [$layout get $item]
              if {"" != [$run element output.($name)]} {
                  set fobj [Rappture::Field ::#auto $_device $run output.($name)]
                  set _path2obj($name) $fobj
                  foreach {xv yv} [$fobj vectors component0] { break }

                  set elem "elem[incr count]"
                  set label [$fobj hints label]
                  $g element create $elem -x $xv -y $yv \
                      -symbol "" -linewidth 2 -label $label

                  set color [$fobj hints color]
                  if {$color != ""} {
                      $g element configure $elem -color $color
                  }

                  set style [$layout get $item.style]
                  if {$style == ""} {
                      set style [$fobj hints style]
                  }
                  if {$style != ""} {
                      eval $g element configure $elem $style
                  }
               }
            }
            curve* {
              set name [$layout get $item]
              if {"" != [$run element output.($name)]} {
                  set cobj [Rappture::Curve ::#auto $run output.($name)]
                  set _path2obj($name) $cobj
                  foreach {xv yv} [$cobj vectors component] { break }

                  set elem "elem[incr count]"
                  set label [$cobj hints label]
                  $g element create $elem -x $xv -y $yv \
                      -symbol "" -linewidth 2 -label $label

                  set color [$cobj hints color]
                  if {$color != ""} {
                      $g element configure $elem -color $color
                  }

                  set style [$layout get $item.style]
                  if {$style == ""} {
                      set style [$fobj hints style]
                  }
                  if {$style != ""} {
                      eval $g element configure $elem $style
                  }
               }
            }
          }
        }

        #
        # HACK ALERT!
        # Use this code to fix up the y-axis limits for the BLT graph.
        # The auto-limits don't always work well.  We want them to be
        # set to a "nice" number slightly above or below the min/max
        # limits.
        #
        set log [$g axis cget y -logscale]
        foreach {min max} [$g axis limits y] { break }
        if {$log} {
            if {$min == $max} {
                set min [expr {0.9*$min}]
                set max [expr {1.1*$max}]
            }
            set min [expr {pow(10.0,floor(log10($min)))}]
            set max [expr {pow(10.0,ceil(log10($max)))}]
        } else {
            if {$min > 0} {
                set min [expr {0.95*$min}]
            } else {
                set min [expr {1.05*$min}]
            }
            if {$max > 0} {
                set max [expr {1.05*$max}]
            } else {
                set max [expr {0.95*$max}]
            }
        }
        $g axis configure y -min $min -max $max
    }
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -layout
#
# Set to the Rappture::Library object representing the layout being
# displayed in the plot.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Xyplot::layout {
    if {$itk_option(-layout) != ""} {
        if {![Rappture::library isvalid $itk_option(-layout)]} {
            error "bad value \"$itk_option(-layout)\": should be Rappture::Library"
        }
    }
    after cancel [itcl::code $this _rebuild]
    after idle [itcl::code $this _rebuild]
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -output
#
# Set to the Rappture::Library object representing the data being
# displayed in the plot.
# ----------------------------------------------------------------------
itcl::configbody Rappture::Xyplot::output {
    if {$_device != ""} {
        itcl::delete object $_device
        set _device ""
    }
    if {$itk_option(-output) != ""} {
        if {![Rappture::library isvalid $itk_option(-output)]} {
            error "bad value \"$itk_option(-output)\": should be Rappture::Library"
        }
        set _device [$itk_option(-output) element -flavor object structure]
    }
    after cancel [itcl::code $this _rebuild]
    after idle [itcl::code $this _rebuild]
}
