# ----------------------------------------------------------------------
#  TEST HARNESS for the P2P infrastructure
#
#  This script drives the test setup and visualization for the P2P
#  infrastructure.  It launches the authority server(s) and various
#  workers, and helps to visualize their interactions.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

# recognize other library files in this same directory
set dir [file dirname [info script]]
lappend auto_path $dir

set time0 [clock seconds]
set processes ""
set nodes(all) ""
set nodeRadius 15

option add *highlightBackground [. cget -background]
option add *client*background gray
option add *client*highlightBackground gray
option add *client*troughColor darkGray

# ======================================================================
#  SHAPES
# ======================================================================
itcl::class Shape {
    private variable _canvas ""      ;# shape sits on this canvas
    private variable _ranges ""      ;# list of time ranges for shape
    private common _shapesOnCanvas   ;# maps canvas => list of shapes

    public variable command ""  ;# command template used to create shape

    constructor {canvas args} {
        # add this shape to the list of shapes on this canvas
        lappend _shapesOnCanvas($canvas) $this
        set _canvas $canvas
        eval configure $args
    }
    destructor {
        # remove this shape from the list of shapes on the canvas
        set i [lsearch $_shapesOnCanvas($_canvas) $this]
        if {$i >= 0} {
            set _shapesOnCanvas($_canvas) \
                [lreplace $_shapesOnCanvas($_canvas) $i $i]
        }
    }

    # ------------------------------------------------------------------
    #  METHOD: addRange <time0> <time1>
    #  Declares that this shape exists during the given time range
    #  between <time0> and <time1>.
    # ------------------------------------------------------------------
    public method addRange {t0 t1} {
        # see if there's any overlap with existing ranges
        set ri0 -1
        set ri1 -1
        for {set i 0} {$i < [llength $_ranges]} {incr i} {
            set pair [lindex $_ranges $i]
            foreach {r0 r1} $pair break
            if {$r0 >= $t0 && $r0 <= $t1} {
                set ri0 $i
            }
            if {$r1 >= $t0 && $r1 <= $t1} {
                set ri1 $i
            }
            incr i
        }

        if {$ri0 < 0 && $ri1 < 1} {
            # doesn't overlap with anything -- insert in right place
            for {set i 0} {$i < [llength $_ranges]} {incr i} {
                set pair [lindex $_ranges $i]
                foreach {r0 r1} $pair break
                if {$t0 < $r0} break
            }
            set _ranges [linsert $_ranges $i [list $t0 $t1]]
        } elseif {$ri0 >= 0 && $ri1 >= 0} {
            # overlaps on both limits -- bridge the middle part
            set r0 [lindex [lindex $_ranges $ri0] 0]
            set r1 [lindex [lindex $_ranges $ri1] 1]
            set _ranges [lreplace $_ranges $ri0 $ri1 [list $r0 $r1]] 
        } elseif {$ri0 >= 0} {
            # overlaps on the lower limit
            for {set i [expr {[llength $_ranges]-1}]} {$i >= 0} {incr i -1} {
                set pair [lindex $_ranges $i]
                foreach {r0 r1} $pair break
                if {$r0 > $t0 && $r1 < $t1} {
                    # remove any ranges completely contained in this one
                    set _ranges [lreplace $_ranges $i $i] 
                }
                set _ranges [lreplace $_ranges $ri0 $ri1 [list $r0 $r1]] 
            }
            set pair [lindex $_ranges $ri0]
            foreach {r0 r1} $pair break
            set _ranges [lreplace $_ranges $ri0 $ri0 [list $r0 $t1]] 
        } else {
            # overlaps on the upper limit
            for {set i [expr {[llength $_ranges]-1}]} {$i >= 0} {incr i -1} {
                set pair [lindex $_ranges $i]
                foreach {r0 r1} $pair break
                if {$r0 > $t0 && $r1 < $t1} {
                    # remove any ranges completely contained in this one
                    set _ranges [lreplace $_ranges $i $i] 
                }
                set _ranges [lreplace $_ranges $ri0 $ri1 [list $r0 $r1]] 
            }
            set pair [lindex $_ranges $ri1]
            foreach {r0 r1} $pair break
            set _ranges [lreplace $_ranges $ri1 $ri1 [list $t0 $r1]] 
        }
    }

    # ------------------------------------------------------------------
    #  METHOD: exists <time>
    #  Checks to see if this shape exists at the given <time>.
    #  Returns 1 if so, and 0 otherwise.
    # ------------------------------------------------------------------
    public method exists {t} {
        for {set i 0} {$i < [llength $_ranges]} {incr i} {
            set pair [lindex $_ranges $i]
            foreach {r0 r1} $pair break
            if {$t >= $r0 && $t <= $r1} {
                return 1
            }
        }
        return 0
    }

    # ------------------------------------------------------------------
    #  PROC: draw <canvas> <time>
    #  Finds the list of shapes that exist on the given <canvas> at
    #  the specified time, and executes all of their associated
    #  -command templates, adding them to the canvas.
    # ------------------------------------------------------------------
    public proc draw {canvas time} {
        global times
        if {[info exists _shapesOnCanvas($canvas)]} {
            set y0 5
            if {[info exists times($time)]} {
                $canvas create text 5 $y0 -anchor nw -text $times($time)
                incr y0 15
            }
            $canvas create text 5 $y0 -anchor nw -tags entity

            foreach obj $_shapesOnCanvas($canvas) {
                if {[$obj exists $time]} {
                    set cmd [$obj cget -command]
                    regsub -all %c $cmd $canvas cmd
                    eval $cmd
                }
            }
        }
    }

    # ------------------------------------------------------------------
    #  PROC: clear <canvas>
    #  Removes all shapes associated with the specified <canvas>.
    # ------------------------------------------------------------------
    public proc clear {canvas} {
        if {[info exists _shapesOnCanvas($canvas)]} {
            eval itcl::delete object $_shapesOnCanvas($canvas)
        }
    }
}

# ======================================================================
#  Build the main interface
# ======================================================================
frame .client -borderwidth 8 -relief flat
pack .client -side right -fill y
button .client.getbids -text "Get Bids:" -command test_bids
pack .client.getbids -side top -anchor w
frame .client.cntls
pack .client.cntls -side bottom -fill x
button .client.cntls.run -text "Spend" -command test_spend
pack .client.cntls.run -side left
entry .client.cntls.points -width 8
pack .client.cntls.points -side left
label .client.cntls.pointsl -text "points"
pack .client.cntls.pointsl -side left

frame .client.bids
pack .client.bids -side bottom -expand yes -fill both
scrollbar .client.bids.ysbar -orient vertical -command {.client.bids.info yview}
pack .client.bids.ysbar -side right -fill y
listbox .client.bids.info -yscrollcommand {.client.bids.ysbar set}
pack .client.bids.info -side left -expand yes -fill both

frame .cntls
pack .cntls -fill x

button .cntls.start -text "Start" -command test_start
pack .cntls.start -side left -padx 4 -pady 2

button .cntls.stop -text "Stop" -command test_stop -state disabled
pack .cntls.stop -side left -padx 4 -pady 2

button .cntls.reload -text "Reload" -command test_reload
pack .cntls.reload -side left -padx 4 -pady 2

button .cntls.layout -text "New Layout" -command {
    foreach key [array names nodes *-\[xy\]] {
        unset nodes($key)
    }
    after idle test_reload
}
pack .cntls.layout -side left -padx 4 -pady 2

entry .cntls.workers -width 5
pack .cntls.workers -side right -padx {0 4} -pady 2
.cntls.workers insert end "3"
label .cntls.workersl -text "Workers:"
pack .cntls.workersl -side right -pady 2

frame .player
pack .player -side bottom -fill x

button .player.back -text "<" -command {test_frame_go -1 nudge}
pack .player.back -side left -padx 4 -pady 2

button .player.fwd -text ">" -command {test_frame_go 1 nudge}
pack .player.fwd -side left -padx 4 -pady 2

button .player.err -text "0 errors" -command {wm deiconify .errors; raise .errors}
pack .player.err -side right -padx 4 -pady 2
.player.err configure -state disabled

scale .player.scale -label "Frame" -orient horizontal \
    -from 0 -to 1 -showvalue 0 -command {test_frame_go 1}
pack .player.scale -side left -expand yes -fill x -padx 4 -pady 2

frame .view
pack .view -side bottom -anchor w
label .view.show -text "Show:"
grid .view.show -row 0 -column 0 -sticky e
radiobutton .view.network -text "P2P Network" -variable view -value network -command test_view_change
grid .view.network -row 0 -column 1 -sticky w
radiobutton .view.traffic -text "Network traffic" -variable view -value traffic -command test_view_change
grid .view.traffic -row 1 -column 1 -sticky w

frame .diagram
pack .diagram -expand yes -fill both
canvas .diagram.network -width 500 -height 400
canvas .diagram.traffic -width 500 -height 400

after idle .view.traffic invoke

toplevel .errors
wm title .errors "Error Messages"
wm withdraw .errors
wm protocol .errors WM_DELETE_WINDOW {wm withdraw .errors}
scrollbar .errors.ysbar -orient vertical -command {.errors.info yview}
pack .errors.ysbar -side right -fill y
text .errors.info -yscrollcommand {.errors.ysbar set} -font {Courier 12}
pack .errors.info -expand yes -fill both
.errors.info tag configure timecode -foreground gray
.errors.info tag configure error -foreground red -font {Courier 12 bold}

proc test_stop {} {
    global processes

    # kill any existing processes
    foreach job $processes {
        exec kill $job
    }
    set processes ""

    .cntls.stop configure -state disabled
    .cntls.start configure -state normal

    after idle test_reload
}

proc test_start {} {
    global processes

    # clean up existing log files...
    foreach fname [glob -nocomplain /tmp/log*] {
        file delete $fname
    }

    # launch a new authority server
    lappend processes [exec tclsh authority.tcl &]

    # launch a series of workers
    for {set i 0} {$i < [.cntls.workers get]} {incr i} {
        lappend processes [exec tclsh worker.tcl &]
        after [expr {int(rand()*5000)}]
    }

    .cntls.start configure -state disabled
    .cntls.stop configure -state normal
}

proc test_reload {} {
    global time0 nodes actions times nodeRadius

    array set colors {
        authority blue
        worker gray
        foreman red
    }

    Shape::clear .diagram.network
    Shape::clear .diagram.traffic
    .errors.info configure -state normal
    .errors.info delete 1.0 end
    set tmax 0
    set errs 0

    #
    # Scan through all files and generate positions for all nodes.
    #
    foreach fname [glob -nocomplain /tmp/log*] {
        set fid [open $fname r]
        set info [read $fid]
        close $fid

        set lasttime ""
        set t0val ""; set first ""
        set t1val ""; set last ""
        set info [split $info \n]
        foreach line $info {
            if {[regexp {^([0-9]+/[0-9]+/[0-9]+ [0-9]+:[0-9]+:[0-9]+)} $line match tval]} {
                if {"" == $t0val} {
                    set first $line
                    set t0val [expr {([clock scan $tval]-$time0)*100}]
                } else {
                    set last $line
                    set t1val [expr {([clock scan $tval]-$time0)*100 + 99}]
                }
            }
        }

        if {"" == $t0val || "" == $t1val} {
            # can't find any log statements -- skip this file!
            continue
        }

        # get the address for this host
        if {[regexp {started at port ([0-9]+)} $info match port]} {
            if {[regexp {options [^\n]+ ip ([^ ]+)} $info match ip]} {
                set addr $ip:$port
            } else {
                set addr 127.0.0.1:$port
            }
            set shape oval
            regexp {^([0-9]+/[0-9]+/[0-9]+ [0-9]+:[0-9]+:[0-9]+) +(authority|worker)} $first match t0 type
        } elseif {[regexp -- {foreman<-} $info]} {
            set shape rectangle
            set addr "foreman"
            set type "foreman"
        } else {
            # unknown log file -- skip it
            continue
        }
        set margin 20
        set r $nodeRadius

        set nodes($fname) $addr
        set nodes($addr-log) $fname
        set nodes($addr-type) $type
        if {![info exists nodes($addr-x)]} {
            set w [expr {[winfo width .diagram]-2*$margin}]
            set h [expr {[winfo height .diagram]-2*$margin}]
            set nodes($addr-x) [expr {int(rand()*$w) + $margin}]
            set nodes($addr-y) [expr {int(rand()*$h) + $margin}]
        }
        set x $nodes($addr-x)
        set y $nodes($addr-y)

        foreach canv {.diagram.traffic .diagram.network} {
            set s [Shape ::#auto $canv -command \
                [list %c create $shape [expr {$x-$r}] [expr {$y-$r}] \
                [expr {$x+$r}] [expr {$y+$r}] \
                -outline black -fill $colors($type) \
                -tags [list $fname $fname-node]]]
            $s addRange $t0val $t1val
        }

        append actions($t0val) "$type $addr online\n"
        append actions($t1val) "$type $addr offline\n"

        if {$t1val > $tmax} { set tmax $t1val }
    }
                
    #
    # Scan through files again and generate shapes for all messages
    #
    foreach fname [glob -nocomplain /tmp/log*] {
        set fid [open $fname r]
        set info [read $fid]
        close $fid
puts "\nscanning $fname"

        catch {unset started}
        set peerlist(addrs) ""
        set peerlist(time) 0
        set lasttime ""
        set counter 0
        foreach line [split $info \n] {
            if {[regexp {^([0-9]+/[0-9]+/[0-9]+ [0-9]+:[0-9]+:[0-9]+) (.+)$} $line match time mesg]} {
                set tval [expr {([clock scan $time]-$time0)*100}]

                if {$time == $lasttime} {
                    set tval [expr {$tval + [incr counter]}]
                }
                set lasttime $time

                if {$tval > $tmax} { set tmax $tval }

                set cid ""
                if {[regexp {accepted: +([^ ]+) +\((.+)\)} $mesg match addr cid]} {
                    append actions($tval) $mesg \n
                    set started(connect$cid-time) $tval
                    set started(connect$cid-addr) ?

                } elseif {[regexp {dropped: +([^ ]+) +\((.+)\)} $mesg match addr cid] && [info exists started(connect$cid-time)]} {
                    incr tval 99  ;# end of this second
                    append actions($tval) $mesg \n
                    set from $nodes($fname)
                    set x0 $nodes($from-x)
                    set y0 $nodes($from-y)
                    set x1 $nodes($addr-x)
                    set y1 $nodes($addr-y)
                    set s [Shape ::#auto .diagram.traffic -command \
                        [list %c create line $x0 $y0 $x1 $y1 -width 3 -fill gray -tags [list $fname $fname-cnx]]]
                    $s addRange $started(connect$cid-time) $tval

                    unset started(connect$cid-time)
                    unset started(connect$cid-addr)

                } elseif {[regexp {(incoming) message from ([^ ]+) \((sock[0-9]+)\): +(.+) => (.*)} $mesg match which addr cid cmd result]
                       || [regexp {(outgoing) message to ([^ ]+): +(.+)} $mesg match which addr cmd]} {
                    switch -- $which {
                        outgoing {
                            set from $addr
                            set to $nodes($fname)
                        }
                        incoming {
                            set from $nodes($fname)
                            set to $addr
                            # show incoming messages later in time
                            incr tval 50
                        }
                    }
                    append actions($tval) $mesg \n
                    set x0 $nodes($from-x)
                    set y0 $nodes($from-y)
                    set x1 $nodes($to-x)
                    set y1 $nodes($to-y)
                    set w [expr {[winfo width .diagram]/2}]

                    set s [Shape ::#auto .diagram.traffic -command \
                        [list %c create line [expr $x0-3] [expr $y0-3] [expr $x1-3] [expr $y1-3] -fill black -arrow first -tags transient]]
                    $s addRange $tval $tval

                    set s [Shape ::#auto .diagram.traffic -command \
                        [list %c create text [expr {0.5*($x0+$x1)}] [expr {0.5*($y0+$y1)-1}] -width $w -fill black -anchor s -text $cmd -tags transient]]
                    $s addRange $tval $tval

                    if {$which == "incoming" && "" != [string trim $result]} {
                        if {[regexp {^ok:} $result]} {
                            set color black
                        } else {
                            set color red
                        }
                        set s [Shape ::#auto .diagram.traffic -command \
                            [list %c create line [expr $x0+3] [expr $y0+3] [expr $x1+3] [expr $y1+3] -fill $color -arrow last -tags transient]]
                        $s addRange $tval $tval

                        set s [Shape ::#auto .diagram.traffic -command \
                            [list %c create text [expr {0.5*($x0+$x1)}] [expr {0.5*($y0+$y1)+1}] -width $w -fill $color -anchor n -text $result -tags transient]]
                        $s addRange $tval $tval
                    }

                    # no address for this client yet?  then save this info
                    if {"" != $cid
                           && [info exists started(connect$cid-addr)]
                           && $started(connect$cid-addr) == "?"} {
                        set started(connect$cid-addr) $addr
                    }

                } elseif {[regexp {connected to peers: (.*)} $mesg match plist]} {
                    # draw a highlight ring, indicating that peers have changed
                    set from $nodes($fname)
                    set x $nodes($from-x)
                    set y $nodes($from-y)
                    foreach canv {.diagram.traffic .diagram.network} {
                        set s [Shape ::#auto $canv -command \
                            [list %c create oval [expr {$x-$r-3}] [expr {$y-$r-3}] [expr {$x+$r+3}] [expr {$y+$r+3}] -outline red -fill "" -tags transient]]
                        $s addRange $tval [expr {$tval+20}]
                    }

                    # draw lines from the previous peer list, now that
                    # we know it's time range is done.
                    foreach addr $peerlist(addrs) {
                        set from $nodes($fname)
                        set x0 $nodes($from-x)
                        set y0 $nodes($from-y)
                        set x1 $nodes($addr-x)
                        set y1 $nodes($addr-y)
                        set s [Shape ::#auto .diagram.network -command \
                            [list %c create line $x0 $y0 $x1 $y1 -width 3 -fill gray -tags [list $fname $fname-cnx]]]
                        $s addRange $peerlist(time) $tval
                    }

                    # save the start of this new peer list
                    set peerlist(addrs) $plist
                    set peerlist(time) $tval
                } elseif {[regexp {ERROR} $mesg match addr]} {
                    .errors.info insert end $time timecode $mesg error "\n"
                    incr errs
                }
                set times($tval) $time
            }
        }

        # any connections hanging out that we haven't drawn?
        foreach key [array names started connect*-addr] {
            regexp {connect([^-]+)-addr} $key match cid
            set from $nodes($fname)
            set addr $started($key)

            if {![info exists nodes($addr-x)]} {
                unset started(connect$cid-time)
                unset started(connect$cid-addr)
                continue
            }
            set x0 $nodes($from-x)
            set y0 $nodes($from-y)
            set x1 $nodes($addr-x)
            set y1 $nodes($addr-y)
            set s [Shape ::#auto .diagram.traffic -command \
                [list %c create line $x0 $y0 $x1 $y1 -width 3 -fill gray -tags [list $fname $fname-cnx]]]
            $s addRange $started(connect$cid-time) $tval

            unset started(connect$cid-time)
            unset started(connect$cid-addr)
        }

        .diagram.traffic bind $fname <Enter> "
            %W itemconfigure $fname-node -outline red
            %W itemconfigure $fname-cnx -fill red
            %W raise $fname-cnx
            %W itemconfigure entity -text {$nodes($fname)}
        "
        .diagram.traffic bind $fname <Leave> "
            %W itemconfigure $fname-node -outline black
            %W itemconfigure $fname-cnx -fill gray
            %W raise transient
            %W itemconfigure entity -text {}
        "

        foreach canv {.diagram.network .diagram.traffic} {
            $canv bind $fname-node <ButtonPress-1> \
                [list worker_node_click %W $fname %x %y]
            $canv bind $fname-node <B1-Motion> \
                [list worker_node_drag %W $fname %x %y]
            $canv bind $fname-node <ButtonRelease-1> \
                [list worker_node_release %W $fname %x %y]
        }

        # any peer connections that we haven't drawn?  then do it now
        foreach addr $peerlist(addrs) {
            set from $nodes($fname)
            set x0 $nodes($from-x)
            set y0 $nodes($from-y)
            set x1 $nodes($addr-x)
            set y1 $nodes($addr-y)
            set s [Shape ::#auto .diagram.network -command \
                [list %c create line $x0 $y0 $x1 $y1 -width 3 -fill gray -tags [list $fname $fname-cnx]]]
            $s addRange $peerlist(time) $tmax
        }

        .diagram.network bind $fname <Enter> "
            %W itemconfigure $fname-node -outline red
            %W itemconfigure $fname-cnx -fill red
            %W raise $fname-cnx
        "
        .diagram.network bind $fname <Leave> "
            %W itemconfigure $fname-node -outline black
            %W itemconfigure $fname-cnx -fill gray
            %W raise transient
        "
    }
    .player.scale configure -to $tmax
    .errors.info configure -state disabled
    if {$errs == 0} {
        .player.err configure -state normal -text "0 errors"
        .player.err configure -state disabled
    } else {
        .player.err configure -state normal -text "$errs error[expr {($errs == 1) ? {} : {s}}]"
    }

    after cancel test_visualize
    after idle test_visualize
}

proc worker_node_click {canv fname x y} {
    $canv itemconfigure $fname-node -stipple gray50
}

proc worker_node_drag {canv fname x y} {
    global nodeRadius
    $canv itemconfigure $fname-node -stipple ""
    $canv coords $fname-node [expr {$x-$nodeRadius}] [expr {$y-$nodeRadius}] [expr {$x+$nodeRadius}] [expr {$y+$nodeRadius}]
}

proc worker_node_release {canv fname x y} {
    global move nodes

    worker_node_drag $canv $fname $x $y
    $canv itemconfigure $fname-node -stipple ""

    set addr $nodes($fname)
    set nodes($addr-x) $x
    set nodes($addr-y) $y
    after idle test_reload
}

proc test_frame_go {dir position} {
    global actions times

    set tmax [.player.scale cget -to]
    if {"nudge" == $position} {
        set nframe [.player.scale get]
        incr nframe $dir
    } else {
        set nframe $position
    }

    while {![info exists actions($nframe)] && $nframe > 0 && $nframe < $tmax} {
        incr nframe $dir
    }

    # set the scale to this new position
    .player.scale configure -command ""
    .player.scale set $nframe
    .player.scale configure -command {test_frame_go 1}

    after cancel test_visualize
    after idle test_visualize
}

proc test_visualize {} {
    .diagram.traffic delete all
    Shape::draw .diagram.traffic [.player.scale get]
    .diagram.traffic raise transient

    .diagram.network delete all
    Shape::draw .diagram.network [.player.scale get]
    .diagram.network raise transient
}

proc test_view_change {} {
    global view
    foreach widget [pack slaves .diagram] {
        catch {pack forget $widget}
    }
    pack .diagram.$view -expand yes -fill both
}

proc test_bids {} {
    set info [Rappture::foreman::bids]
    .client.bids.info delete 0 end
    eval .client.bids.info insert end $info
}
