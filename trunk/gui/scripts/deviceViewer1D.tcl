# -*- mode: tcl; indent-tabs-mode: nil -*-
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
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itk
package require Img
package require BLT

option add *DeviceViewer1D.padding 4 widgetDefault
option add *DeviceViewer1D.deviceSize 0.25i widgetDefault
option add *DeviceViewer1D.deviceOutline black widgetDefault
option add *DeviceViewer1D*graph.width 3i widgetDefault
option add *DeviceViewer1D*graph.height 2i widgetDefault

itcl::class Rappture::DeviceViewer1D {
    inherit itk::Widget

    itk_option define -device device Device ""

    constructor {owner args} { # defined below }
    destructor { # defined below }

    public method add {dataobj {settings ""}}
    public method get {}
    public method delete {args}
    public method parameters {title args} { # do nothing }

    public method controls {option args}
    public method download {option args}

    protected method _loadDevice {}
    protected method _loadParameters {frame path}
    protected method _changeTabs {{why -program}}
    protected method _fixSize {}
    protected method _fixAxes {}
    protected method _align {}

    protected method _marker {option {name ""} {path ""}}

    protected method _controlCreate {container libObj path}
    protected method _controlSet {widget libObj path}

    private variable _owner ""      ;# thing managing this control
    private variable _dlist ""      ;# list of dataobj objects
    private variable _dobj2raise    ;# maps dataobj => raise flag
    private variable _device ""     ;# XML library with <structure>
    private variable _tab2fields    ;# maps tab name => list of fields
    private variable _field2parm    ;# maps field path => parameter name
    private variable _units ""      ;# units for field being edited
    private variable _marker        ;# marker currently being edited
}

itk::usual DeviceViewer1D {
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::constructor {owner args} {
    set _owner $owner

    pack propagate $itk_component(hull) no

    itk_component add tabs {
        blt::tabset $itk_interior.tabs -borderwidth 0 -relief flat \
            -side bottom -tearoff 0 \
            -selectcommand [itcl::code $this _changeTabs -user]
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

    itk_component add top {
        frame $itk_component(inner).top
    }
    pack $itk_component(top) -fill x

    itk_component add layout {
        Rappture::DeviceLayout1D $itk_component(inner).layout
    }
    pack $itk_component(layout) -side top -fill x -pady 4

    itk_component add graph {
        blt::graph $itk_component(inner).graph \
            -highlightthickness 0 -plotpadx 0 -plotpady 0
    } {
        keep -background -foreground -cursor -font
    }
    pack $itk_component(graph) -expand yes -fill both
    $itk_component(graph) legend configure -hide yes

    bind $itk_component(graph) <Configure> "
        [list after cancel [list catch [itcl::code $this _align]]]
        [list after 100 [list catch [itcl::code $this _align]]]
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

    after cancel [list catch [itcl::code $this _fixSize]]
    after idle [list catch [itcl::code $this _fixSize]]
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
    after cancel [list catch [itcl::code $this _loadDevice]]
}

# ----------------------------------------------------------------------
# USAGE: add <dataobj> ?<settings>?
#
# Clients use this to add a data object to the plot.  The optional
# <settings> are used to configure the plot.  Allowed settings are
# -color, -brightness, -width, -linestyle, and -raise. Only
# -brightness and -raise do anything.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::add {dataobj {settings ""}} {
    array set params {
        -color auto
        -brightness 0
        -width 1
        -raise 0
        -linestyle solid
        -description ""
        -param ""
    }
    array set params $settings

    set pos [lsearch -exact $_dlist $dataobj]

    if {$pos < 0} {
        if {![Rappture::library isvalid $dataobj]} {
            error "bad value \"$dataobj\": should be Rappture::library object"
        }

        lappend _dlist $dataobj
        set _dobj2raise($dataobj) $params(-raise)

        after cancel [list catch [itcl::code $this _loadDevice]]
        after idle [list catch [itcl::code $this _loadDevice]]
    }
}

# ----------------------------------------------------------------------
# USAGE: get
#
# Clients use this to query the list of objects being plotted, in
# order from bottom to top of this result.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::get {} {
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
# USAGE: delete ?<dataobj> <dataobj> ...?
#
# Clients use this to delete a dataobj from the plot. If no dataobjs
# are specified, then all dataobjs are deleted.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::delete {args} {
    if {[llength $args] == 0} {
        set args $_dlist
    }

    # delete all specified dataobjs
    set changed 0
    foreach dataobj $args {
        set pos [lsearch -exact $_dlist $dataobj]
        if {$pos >= 0} {
            set _dlist [lreplace $_dlist $pos $pos]
            catch {unset _dobj2raise($dataobj)}
            set changed 1
        }
    }

    # if anything changed, then rebuild the plot
    if {$changed} {
        after cancel [list catch [itcl::code $this _loadDevice]]
        after idle [list catch [itcl::code $this _loadDevice]]
    }
}

# ----------------------------------------------------------------------
# USAGE: controls insert <pos> <xmlobj> <path>
#
# Clients use this to add a control to the internal panels of this
# widget.  Such controls are usually placed at the top of the widget,
# but if possible, they are integrated directly onto the device
# layout or the field area.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::controls {option args} {
    switch -- $option {
        insert {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"controls insert pos xmlobj path\""
            }
            set pos [lindex $args 0]
            set xmlobj [lindex $args 1]
            set path [lindex $args 2]
            if {[string match *structure.parameters* $path]} {
            } elseif {[string match structure.components* $path]} {
                $itk_component(layout) controls insert $pos $xmlobj $path
            }
        }
        default {
            error "bad option \"$option\": should be insert"
        }
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
itcl::body Rappture::DeviceViewer1D::download {option args} {
    switch $option {
        coming {
            # nothing to do
        }
        controls {
            # no controls for this download yet
            return ""
        }
        now {
            return ""  ;# not implemented yet!
        }
        default {
            error "bad option \"$option\": should be coming, controls, now"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _loadDevice
#
# Used internally to search for fields and create corresponding
# tabs whenever a device is installed into this viewer.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_loadDevice {} {
    set _device [lindex [get] end]

    #
    # Release any info left over from the last device.
    #
    foreach name [array names _tab2fields] {
        eval itcl::delete object $_tab2fields($name)
    }
    catch {unset _tab2fields}
    catch {unset _field2parm}

    if {[winfo exists $itk_component(top).cntls]} {
        $itk_component(top).cntls delete 0 end
    }

    #
    # Scan through the current device and extract the list of
    # fields.  Create a tab for each field.
    #
    if {$_device != ""} {
        foreach nn [$_device children fields] {
            set name [$_device get fields.$nn.about.label]
            if {$name == ""} {
                set name $nn
            }

            set fobj [Rappture::Field ::#auto $_device fields.$nn]
            lappend _tab2fields($name) $fobj
        }
    }
    set tabs [lsort [array names _tab2fields]]

    if {[$itk_component(tabs) size] > 0} {
        $itk_component(tabs) delete 0 end
    }

    if {[llength $tabs] <= 0} {
        #
        # No fields?  Then we don't need to bother with tabs.
        # Just pack the inner frame directly.  If there are no
        # fields, get rid of the graph.
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

    #
    # Scan through and look for any parameters in the <structure>.
    # Register any parameters associated with fields, so we can
    # add them as active controls whenever we install new fields.
    # Create controls for any remaining parameters, so the user
    # can see that there's something to adjust.
    #
    if {$_device != ""} {
        _loadParameters $itk_component(top) parameters
    }

    #
    # Install the first tab
    #
    _changeTabs

    #
    # Fix the right margin of the graph so that it has enough room
    # to display the right-hand edge of the device.
    #
    $itk_component(graph) configure \
        -rightmargin [$itk_component(layout) extents bar3D]

    after cancel [list catch [itcl::code $this _fixSize]]
    after idle [list catch [itcl::code $this _fixSize]]
}

# ----------------------------------------------------------------------
# USAGE: _loadParameters <frame> <path>
#
# Used internally in _loadDevice to load child parameters at the
# specified <path> into the <frame>.  If any of the children are
# groups, then this is called recursively to fill in the group
# children.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_loadParameters {frame path} {
    foreach cname [$_device children $path] {
        set handled 0
        set type [$_device element -as type $path.$cname]
        if {$type == "about"} {
            continue
        }
        if {$type == "number"} {
            set name [$_device element -as id $path.$cname]

            # look for a field that uses this parameter
            set found ""
            foreach fname [$_device children fields] {
                foreach comp [$_device children fields.$fname] {
                    set v [$_device get fields.$fname.$comp.constant]
                    if {[string equal $v $name]} {
                        set found "fields.$fname.$comp"
                        break
                    }
                }
                if {"" != $found} break
            }

            if {"" != $found} {
                set _field2parm($found) $name
                set handled 1
            }
        }

        #
        # Any parameter that was not handled above should be handled
        # here, by adding it to a control panel above the device
        # layout area.
        #
        if {!$handled} {
            if {![winfo exists $frame.cntls]} {
                Rappture::Controls $frame.cntls $_owner
                pack $frame.cntls -expand yes -fill both
            }
            $frame.cntls insert end $path.$cname

            #
            # If this is a group, then we must add its children
            # recursively.
            #
            if {$type == "group"} {
                set gr [$frame.cntls control -value end]
                _loadParameters [$gr component inner] $path.$cname
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: _changeTabs ?-user|-program?
#
# Used internally to change the field being displayed whenever a new
# tab is selected.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_changeTabs {{why -program}} {
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

    if {$why eq "-user"} {
        Rappture::Logger::log device1D -field $name
    }

    #
    # Update the graph to show the current field.
    #
    eval $graph element delete [$graph element names]
    eval $graph marker delete [$graph marker names]

    foreach {zmin zmax} [$itk_component(layout) limits] { break }
    if {$zmin != "" && $zmin < $zmax} {
        $graph axis configure x -min $zmin -max $zmax
    }

    if {$_device != ""} {
        set units [$_device get units]
        if {$units != "arbitrary"} {
            $graph axis configure x -hide no -title "Position ($units)"
        } else {
            $graph axis configure x -hide yes
        }
    } else {
        $graph axis configure x -hide no -title "Position"
    }
    $graph axis configure x -checklimits no

    # turn on auto limits
    $graph axis configure y -min "" -max "" -checklimits no

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

        if {[info exists hints(scale)]
              && [string match log* $hints(scale)]} {
            $graph axis configure y -logscale yes
        } else {
            $graph axis configure y -logscale no
        }

        foreach comp [$fobj components] {
            # can only handle 1D meshes here
            if {[$fobj components -dimensions $comp] != "1D"} {
                continue
            }

            set elem "elem[incr n]"
            set xv [$fobj mesh $comp]
            set yv [$fobj values $comp]

            $graph element create $elem -x $xv -y $yv \
                -color black -symbol "" -linewidth 2

            if {[info exists hints(color)]} {
                $graph element configure $elem -color $hints(color)
            }

            foreach {path x y val} [$fobj controls get $comp] {
                if {$path != ""} {
                    set id "control[incr n]"
                    $graph marker create text -coords [list $x $y] \
                        -text $val -anchor s -name $id -background ""
                    $graph marker bind $id <Enter> \
                        [itcl::code $this _marker enter $id]
                    $graph marker bind $id <Leave> \
                        [itcl::code $this _marker leave $id]
                    $graph marker bind $id <ButtonPress> \
                        [itcl::code $this _marker edit $id $fobj/$path]
                }
            }
        }
    }

    # let the widget settle, then fix the axes to "nice" values
    after cancel [list catch [itcl::code $this _fixAxes]]
    after 100 [list catch [itcl::code $this _fixAxes]]
}

# ----------------------------------------------------------------------
# USAGE: _fixSize
#
# Used internally to fix the overall size of this widget based on
# the various parts inside.  Sets the requested width/height of the
# widget so that it is big enough to display the device and its
# fields.
# ----------------------------------------------------------------------
itcl::body Rappture::DeviceViewer1D::_fixSize {} {
    update idletasks
    set w [winfo reqwidth $itk_component(tabs)]
    set h [winfo reqheight $itk_component(tabs)]
    component hull configure -width $w -height $h
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
    if {![winfo ismapped $graph]} {
        after cancel [list catch [itcl::code $this _fixAxes]]
        after 100 [list catch [itcl::code $this _fixAxes]]
        return
    }

    #
    # HACK ALERT!
    # Use this code to fix up the y-axis limits for the BLT graph.
    # The auto-limits don't always work well.  We want them to be
    # set to a "nice" number slightly above or below the min/max
    # limits.
    #
    set log [$graph axis cget y -logscale]
    $graph axis configure y -min "" -max ""
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
            #
            # BE CAREFUL:  Need an update here to force the graph to
            #   refresh itself or else a subsequent click on the
            #   marker will ignore the text that was recently changed,
            #   and fail to generate a <ButtonPress> event!
            #
            update idletasks
        }
        leave {
            $itk_component(graph) marker configure $name -background ""
            #
            # BE CAREFUL:  Need an update here to force the graph to
            #   refresh itself or else a subsequent click on the
            #   marker will ignore the text that was recently changed,
            #   and fail to generate a <ButtonPress> event!
            #
            update idletasks
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
                    Rappture::Logger::log warning $_marker(path) $result
                    return 0
                }
            }
            if {[catch {$_marker(fobj) controls validate $_marker(path) $name} result]} {
                bell
                Rappture::Tooltip::cue $itk_component(geditor) $result
                Rappture::Logger::log warning $_marker(path) $result
                return 0
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
            $_owner changed $_marker(path)
            event generate $itk_component(hull) <<Edit>>
            Rappture::Logger::log input $_marker(path) $value

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
    $type $w -units $units -presets $presets -log $path
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

    delete
    if {"" != $itk_option(-device)} {
        add $itk_option(-device)
    }
    _loadDevice
}
