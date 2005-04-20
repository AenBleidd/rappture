# ----------------------------------------------------------------------
#  COMPONENT: deviceViewer1D - visualizer for 1D device geometries
#
#  This widget is a simple visualizer for 1D devices.  It takes the
#  Rappture XML representation for a 1D device and draws various
#  facets of the data.  Each facet shows the physical layout along
#  with some other quantity.  The "Electrical" facet shows electrical
#  contacts.  The "Doping" facet shows the doping profile, and so
#  forth.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004  Purdue Research Foundation, West Lafayette, IN
# ======================================================================
package require Itk
package require BLT

option add *DeviceViewer1D.width 4i widgetDefault
option add *DeviceViewer1D.height 4i widgetDefault
option add *DeviceViewer1D.padding 4 widgetDefault
option add *DeviceViewer1D.deviceSize 0.25i widgetDefault
option add *DeviceViewer1D.deviceOutline black widgetDefault

itcl::class Rappture::DeviceViewer1D {
    inherit itk::Widget

    itk_option define -device device Device ""
    itk_option define -tool tool Tool ""

    constructor {args} { # defined below }
    destructor { # defined below }

    public method controls {option args}
                                                                                
    protected method _fixTabs {}
    protected method _changeTabs {}
    protected method _fixAxes {}
    protected method _align {}

    protected method _marker {option {name ""} {path ""}}

    protected method _controlCreate {container libObj path}
    protected method _controlSet {widget libObj path}

    private variable _device ""     ;# LibraryObj for device rep
    private variable _tool ""       ;# LibraryObj for tool parameters
    private variable _tab2fields    ;# maps tab name => list of fields
    private variable _units ""      ;# units for field being edited
    private variable _restrict ""   ;# restriction expr for field being edited
    private variable _marker        ;# marker currently being edited
}
                                                                                
itk::usual DeviceViewer1D {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::constructor {args} {
    itk_option add hull.width hull.height
    pack propagate $itk_component(hull) no

    itk_component add tabs {
        blt::tabset $itk_interior.tabs -borderwidth 0 -relief flat \
            -side bottom -tearoff 0 \
            -selectcommand [itcl::code $this _changeTabs]
    } {
        keep -activebackground -activeforeground
        keep -background -cursor -font
        rename -highlightbackground -background background Background
        keep -highlightcolor -highlightthickness
        keep -tabbackground -tabforeground
        rename -selectbackground -background background Background
        rename -selectforeground -foreground foreground Foreground
    }
    pack $itk_component(tabs) -expand yes -fill both

    itk_component add -protected inner {
        frame $itk_component(tabs).inner
    }

    itk_component add ambient {
        frame $itk_component(inner).ambient
    }
    pack $itk_component(ambient) -side top -fill x

    itk_component add layout {
        Rappture::DeviceLayout1D $itk_component(inner).layout
    }
    pack $itk_component(layout) -side top -fill x -pady 4

    itk_component add graph {
        blt::graph $itk_component(inner).graph \
            -highlightthickness 0 -plotpadx 0 -plotpady 0 \
            -width 3i -height 3i
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(graph) -expand yes -fill both
    $itk_component(graph) legend configure -hide yes

    bind $itk_component(graph) <Configure> "
        after cancel [itcl::code $this _fixAxes]
        after idle [itcl::code $this _fixAxes]
    "

    itk_component add geditor {
        Rappture::Editor $itk_component(graph).editor \
            -activatecommand [itcl::code $this _marker activate] \
            -validatecommand [itcl::code $this _marker validate] \
            -applycommand [itcl::code $this _marker apply]
    }

    itk_component add devcntls {
        Rappture::Notebook $itk_component(inner).devcntls
    }
    pack $itk_component(devcntls) -side bottom -fill x

    eval itk_initialize $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::destructor {} {
    set _device ""
    foreach name [array names _tab2fields] {
        eval itcl::delete object $_tab2fields($name)
    }
    after cancel [list catch [itcl::code $this _fixAxes]]
    after cancel [list catch [itcl::code $this _align]]
}

# ----------------------------------------------------------------------
# USAGE: controls add <parameter>
# USAGE: controls remove <parameter>|all
#
# Clients use this to add a control to the internal panels of this
# widget.  If the <parameter> is ambient*, then the control is added
# to the top, so it goes along with the layout of the device.  If
# it is structure.fields.field*, then it goes in one of the field
# panels.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::controls {option args} {
    switch -- $option {
        add {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"controls add parameter\""
            }
            set path [lindex $args 0]
            if {[string match structure.fields.field* $path]} {
            } elseif {[string match structure.components* $path]} {
                $itk_component(layout) controls add $path
            } else {
                _controlCreate $itk_component(ambient) $_tool $path
            }
        }
        remove {
            error "not yet implemented"
        }
        default {
            error "bad option \"$option\": should be add or remove"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _fixTabs
#
# Used internally to search for fields and create corresponding
# tabs whenever a device is installed into this viewer.
#
# If there are no tabs, then the widget is packed so that it appears
# directly.  Otherwise, the interior reconfigured and assigned to
# the current tab.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_fixTabs {} {
    #
    # Release any info left over from the last device.
    #
    foreach name [array names _tab2fields] {
        eval itcl::delete object $_tab2fields($name)
    }
    catch {unset _tab2fields}

    #
    # Scan through the current device and extract the list of
    # fields.  Create a tab for each field.
    #
    if {$_device != ""} {
        foreach nn [$_device children fields] {
            if {[string match field* $nn]} {
                set name [$_device get $nn.label]
                if {$name == ""} {
                    set name $nn
                }

                set fobj [Rappture::Field ::#auto $_device $_device $nn]
                lappend _tab2fields($name) $fobj
            }
        }
    }
    set tabs [lsort [array names _tab2fields]]

    if {[$itk_component(tabs) size] > 0} {
        $itk_component(tabs) delete 0 end
    }

    if {[llength $tabs] <= 0} {
        #
        # No fields or one field?  Then we don't need to bother
        # with tabs.  Just pack the inner frame directly.  If
        # there are no fields, get rid of the graph.
        #
        pack $itk_component(inner) -expand yes -fill both
        if {[llength $tabs] > 0} {
            pack $itk_component(graph) -expand yes -fill both
        } else {
            pack forget $itk_component(graph)
            $itk_component(layout) configure -leftmargin 0 -rightmargin 0
        }
    } else {
        #
        # Two or more fields?  Then create a tab for each field
        # and select the first one by default.  Make sure the
        # graph is packed.
        #
        pack forget $itk_component(inner)
        pack $itk_component(graph) -expand yes -fill both

        foreach name $tabs {
            $itk_component(tabs) insert end $name \
                -activebackground $itk_option(-background)
        }
        $itk_component(tabs) select 0
    }
    _changeTabs

    #
    # Fix the right margin of the graph so that it has enough room
    # to display the right-hand edge of the device.
    #
    $itk_component(graph) configure \
        -rightmargin [$itk_component(layout) extents bar3D]
}

# ----------------------------------------------------------------------
# USAGE: _changeTabs
#
# Used internally to change the field being displayed whenever a new
# tab is selected.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_changeTabs {} {
    set graph $itk_component(graph)

    #
    # Figure out which tab is selected and make the inner frame
    # visible in that tab.
    #
    set i [$itk_component(tabs) index select]
    if {$i != ""} {
        set name [$itk_component(tabs) get $i]
        $itk_component(tabs) tab configure $name \
            -window $itk_component(inner) -fill both
    } else {
        set name [lindex [array names _tab2fields] 0]
    }

    #
    # Update the graph to show the current field.
    #
    eval $graph element delete [$graph element names]
    eval $graph marker delete [$graph marker names]

    foreach {zmin zmax} [$itk_component(layout) limits] { break }
    if {$zmax > $zmin} {
        $graph axis configure x -min $zmin -max $zmax -title "Position (um)"
    }

    # turn on auto limits
    $graph axis configure y -min "" -max ""

    set flist ""
    if {[info exists _tab2fields($name)]} {
        set flist $_tab2fields($name)
    }

    set n 0
    foreach fobj $flist {
        catch {unset hints}
        array set hints [$fobj hints]

        if {[info exists hints(units)]} {
            set _units $hints(units)
            $graph axis configure y -title "$name ($hints(units))"
        } else {
            set _units ""
            $graph axis configure y -title $name
        }

        if {[info exists hints(restrict)]} {
            set _restrict $hints(restrict)
        } else {
            set _restrict ""
        }

        if {[info exists hints(scale)]
              && [string match log* $hints(scale)]} {
            $graph axis configure y -logscale yes
        } else {
            $graph axis configure y -logscale no
        }

        foreach comp [$fobj components] {
            set elem "elem[incr n]"
            foreach {xv yv} [$fobj vectors $comp] { break }
            $graph element create $elem -x $xv -y $yv -symbol "" -linewidth 2

            if {[info exists hints(color)]} {
                $graph element configure $elem -color $hints(color)
            }

            foreach {path x y val} [$fobj controls get $comp] {
                $graph marker create text -coords [list $x $y] \
                    -text $val -anchor s -name $comp.$x -background ""
                $graph marker bind $comp.$x <Enter> \
                    [itcl::code $this _marker enter $comp.$x]
                $graph marker bind $comp.$x <Leave> \
                    [itcl::code $this _marker leave $comp.$x]
                $graph marker bind $comp.$x <ButtonPress> \
                    [itcl::code $this _marker edit $comp.$x $fobj/$path]
            }
        }
    }

    # let the widget settle, then fix the axes to "nice" values
    after cancel [itcl::code $this _fixAxes]
    after 20 [itcl::code $this _fixAxes]
}

# ----------------------------------------------------------------------
# USAGE: _fixAxes
#
# Used internally to adjust the y-axis limits of the graph to "nice"
# values, so that any control marker associated with the value,
# for example, remains on screen.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_fixAxes {} {
    set graph $itk_component(graph)

    #
    # HACK ALERT!
    # Use this code to fix up the y-axis limits for the BLT graph.
    # The auto-limits don't always work well.  We want them to be
    # set to a "nice" number slightly above or below the min/max
    # limits.
    #
    set log [$graph axis cget y -logscale]
    foreach {min max} [$graph axis limits y] { break }

    if {$log} {
        set min [expr {0.9*$min}]
        set max [expr {1.1*$max}]
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

    # bump up the max so that it's big enough to show control markers
    set fnt $itk_option(-font)
    set h [expr {[font metrics $fnt -linespace] + 5}]
    foreach mname [$graph marker names] {
        set xy [$graph marker cget $mname -coord]
        foreach {x y} [eval $graph transform $xy] { break }
        set y [expr {$y-$h}]  ;# find top of text in pixels
        foreach {x y} [eval $graph invtransform [list 0 $y]] { break }
        if {$y > $max} { set max $y }
    }

    if {$log} {
        set min [expr {pow(10.0,floor(log10($min)))}]
        set max [expr {pow(10.0,ceil(log10($max)))}]
    } else {
        set min [expr {0.1*floor(10*$min)}]
        set max [expr {0.1*ceil(10*$max)}]
    }

    $graph axis configure y -min $min -max $max

    after cancel [list catch [itcl::code $this _align]]
    after 100 [list catch [itcl::code $this _align]]
}

# ----------------------------------------------------------------------
# USAGE: _align
#
# Used internally to align the margins of the device layout and the
# graph, so that two areas line up.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_align {} {
    set graph $itk_component(graph)

    #
    # Set the left/right margins of the layout so that it aligns
    # with the graph.  Set the right margin of the graph so it
    # it is big enough to show the 3D edge of the device that
    # hangs over on the right-hand side.
    #
    update
    foreach {xmin xmax} [$graph axis limits x] { break }
    set lm [$graph xaxis transform $xmin]
    $itk_component(layout) configure -leftmargin $lm

    set w [winfo width $graph]
    set rm [expr {$w-[$graph xaxis transform $xmax]}]
    $itk_component(layout) configure -rightmargin $rm
}

# ----------------------------------------------------------------------
# USAGE: _marker enter <name>
# USAGE: _marker leave <name>
# USAGE: _marker edit <name> <path>
# USAGE: _marker activate
# USAGE: _marker validate <value>
# USAGE: _marker apply <value>
#
# Used internally to manipulate the control markers draw on the
# graph for a field.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_marker {option {name ""} {path ""}} {
    switch -- $option {
        enter {
            $itk_component(graph) marker configure $name -background #e5e5e5
        }
        leave {
            $itk_component(graph) marker configure $name -background ""
        }
        edit {
            set _marker(name) $name
            set _marker(fobj) [lindex [split $path /] 0]
            set _marker(path) [lindex [split $path /] 1]
            $itk_component(geditor) activate
        }
        activate {
            set g $itk_component(graph)
            set val [$g marker cget $_marker(name) -text]
            foreach {x y} [$g marker cget $_marker(name) -coords] { break }
            foreach {x y} [$g transform $x $y] { break }
            set x [expr {$x + [winfo rootx $g] - 4}]
            set y [expr {$y + [winfo rooty $g] - 5}]

            set fnt $itk_option(-font)
            set h [expr {[font metrics $fnt -linespace] + 2}]
            set w [expr {[font measure $fnt $val] + 5}]

            return [list text $val \
                x [expr {$x-$w/2}] \
                y [expr {$y-$h}] \
                w $w \
                h $h]
        }
        validate {
            if {$_units != ""} {
                if {[catch {Rappture::Units::convert $name \
                        -context $_units -to $_units} result] != 0} {
                    if {[regexp {^bad.*: +(.)(.+)} $result match first tail]
                          || [regexp {(.)(.+)} $result match first tail]} {
                        set result "[string toupper $first]$tail"
                    }
                    bell
                    Rappture::Tooltip::cue $itk_component(geditor) $result
                    return 0
                }
                if {"" != $_restrict
                      && [catch {Rappture::Units::convert $result \
                        -context $_units -to $_units -units off} value] == 0} {

                    set rexpr $_restrict
                    regsub -all value $rexpr {$value} rexpr
                    if {[catch {expr $rexpr} result] == 0 && !$result} {
                        bell
                        Rappture::Tooltip::cue $itk_component(geditor) "Should satisfy the condition: $_restrict"
                        return 0
                    }
                }
            }
            return 1
        }
        apply {
            if {$_units != ""} {
                catch {Rappture::Units::convert $name \
                    -context $_units -to $_units} value
            } else {
                set value $name
            }

            $_marker(fobj) controls put $_marker(path) $value
            event generate $itk_component(hull) <<Edit>>

            _changeTabs
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _controlCreate <container> <libObj> <path>
#
# Used internally to create a gauge widget and pack it into the
# given <container>.  When the gauge is set, it updates the value
# for the <path> in the <libObj>.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_controlCreate {container libObj path} {
    set presets ""
    foreach pre [$libObj children -type preset $path] {
        lappend presets \
            [$libObj get $path.$pre.value] \
            [$libObj get $path.$pre.label]
    }

    set type Rappture::Gauge
    set units [$libObj get $path.units]
    if {$units != ""} {
        set desc [Rappture::Units::description $units]
        if {[string match temperature* $desc]} {
            set type Rappture::TemperatureGauge
        }
    }

    set counter 0
    set w "$container.gauge[incr counter]"
    while {[winfo exists $w]} {
        set w "$container.gauge[incr counter]"
    }

    # create the widget
    $type $w -units $units -presets $presets
    pack $w -side top -anchor w
    bind $w <<Value>> [itcl::code $this _controlSet $w $libObj $path]

    set min [$libObj get $path.min]
    if {"" != $min} { $w configure -minvalue $min }

    set max [$libObj get $path.max]
    if {"" != $max} { $w configure -maxvalue $max }

    set str [$libObj get $path.default]
    if {$str != ""} { $w value $str }

    if {$type == "Rappture::Gauge" && "" != $min && "" != $max} {
        set color [$libObj get $path.color]
        if {$color == ""} {
            set color blue
        }
        if {$units != ""} {
            set min [Rappture::Units::convert $min -to $units -units off]
            set max [Rappture::Units::convert $max -to $units -units off]
        }
        $w configure -spectrum [Rappture::Spectrum ::#auto [list \
            $min white $max $color] -units $units]
    }

    set str [$libObj get $path.label]
    if {$str != ""} {
        set help [$libObj get $path.help]
        if {"" != $help} {
            append str "\n$help"
        }
        if {$units != ""} {
            set desc [Rappture::Units::description $units]
            append str "\n(units of $desc)"
        }
        Rappture::Tooltip::for $w $str
    }

    set str [$libObj get $path.icon]
    if {$str != ""} {
        $w configure -image [image create photo -data $str]
    }
}

# ----------------------------------------------------------------------
# USAGE: _controlSet <widget> <libObj> <path>
#
# Invoked automatically whenever an internal control changes value.
# Queries the new value for the control and assigns the value to the
# given <path> on the XML object <libObj>.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_controlSet {widget libObj path} {
    set newval [$widget value]
    $libObj put $path.current $newval
    event generate $itk_component(hull) <<Edit>>
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -device
#
# Set to the Rappture::Library object representing the device being
# displayed in the viewer.  If set to "", the viewer is cleared to
# display nothing.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceViewer1D::device {
    if {$itk_option(-device) != ""} {
        if {![Rappture::library isvalid $itk_option(-device)]} {
            error "bad value \"$itk_option(-device)\": should be Rappture::Library"
        }
    }
    set _device $itk_option(-device)
    _fixTabs
}

# ----------------------------------------------------------------------
# CONFIGURATION OPTION: -tool
#
# Set to the Rappture::Library object containing tool parameters.
# Needed only if controls are added to the widget, so the controls
# can update the tool parameters.
# ----------------------------------------------------------------------
itcl::configbody Rappture::DeviceViewer1D::tool {
    if {$itk_option(-tool) != ""} {
        if {![Rappture::library isvalid $itk_option(-tool)]} {
            error "bad value \"$itk_option(-tool)\": should be Rappture::Library"
        }
    }
    set _tool $itk_option(-tool)
}
