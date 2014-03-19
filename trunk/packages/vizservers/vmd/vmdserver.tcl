# ----------------------------------------------------------------------
#  HUBZERO: server for VMD
#
#  This program runs VMD and acts as a server for client applications.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2013 - HUBzero Foundation, LLC
# ======================================================================

# The VMD TCL interpreter is by default interactive.  Turn this off
# so that unknown commands like "scene" don't get exec-ed. 
set ::tcl_interactive 0

proc bgerror {mesg} {
    puts stderr "SERVER ERROR: $mesg"
}

# parse command line args
set Paradigm "socket"
while {[llength $argv] > 0} {
    set opt [lindex $argv 0]
    set argv [lrange $argv 1 end]

    switch -- $opt {
        -socket { set Paradigm "socket" }
        -stdio  { set Paradigm "stdio" }
        default {
            puts stderr "bad option \"$opt\": should be -socket or -stdio"
        }
    }
}

# use this to take snapshots to send to clients
image create photo SnapShot

# set the screen to a good size
set DisplaySize(w) 300
set DisplaySize(h) 300
display resize $DisplaySize(w) $DisplaySize(h)
set DisplaySize(changed) 0

# capture initial display settings for later reset
display antialias on

set DisplayProps(options) ""
foreach key {
    ambientocclusion antialias aoambient aodirect
    backgroundgradient
    culling cuestart cueend cuedensity cuemode
    depthcue distance
    eyesep
    farclip focallength
    height
    nearclip
    projection
    shadows stereo
} {
    if {$key eq "nearclip" || $key eq "farclip"} {
        append DisplayProps(options) [list display $key set [display get $key]] "\n"
    } else {
        append DisplayProps(options) [list display $key [display get $key]] "\n"
    }
}

# initialize work queue and epoch counter (see server_send_image)
set Epoch 0
set Work(queue) ""
set Sendqueue ""
set Scenes(@CURRENT) ""

set parser [interp create -safe]

foreach cmd {
  vmdinfo
  vmdbench
  color
  axes
  imd
  vmdcollab
  vmd_label
  light
  material
  vmd_menu
  stage
  light
  user
  mol
  molinfo
  molecule
  mouse
  mobile
  spaceball
  plugin
  render
  tkrender
  rotate
  rotmat
  vmd_scale
  translate
  sleep
  tool
  measure
  rawtimestep
  gettimestep
  vmdcon
  volmap
  parallel
} {
    $parser alias $cmd $cmd
}

# ----------------------------------------------------------------------
# USAGE: display option ?arg arg...?
#
# Executes the "command arg arg..." string in the server and substitutes
# the result into the template string in place of each "%v" field.
# Sends the result back to the client.
# ----------------------------------------------------------------------
proc cmd_display {args} {
    set option [lindex $args 0]
    if {[lsearch {resize reposition rendermode update fps} $option] >= 0} {
        # ignore these commands -- they cause trouble
        return ""
    }
    eval display $args
}
$parser alias display cmd_display

# ----------------------------------------------------------------------
# USAGE: tellme "command template with %v" command arg arg...
#
# Executes the "command arg arg..." string in the server and substitutes
# the result into the template string in place of each "%v" field.
# Sends the result back to the client.
# ----------------------------------------------------------------------
proc cmd_tellme {fmt args} {
    global parser client

    # evaluate args as a command and subst the result in the fmt string
    if {[catch {$parser eval $args} result] == 0} {
        server_send_result $client "nv>[string map [list %v $result] $fmt]"
    } else {
        server_oops $client $result
    }
}
$parser alias tellme cmd_tellme

# ----------------------------------------------------------------------
# USAGE: resize <w> <h>
#
# Resizes the visualization window to the given width <w> and height
# <h>.  The next image sent should be this size.
# ----------------------------------------------------------------------
proc cmd_resize {w h} {
    global DisplayProps

    # store the desired size in case we downscale
    set DisplayProps(framew) $w
    set DisplayProps(frameh) $h

    server_safe_resize $w $h
}
$parser alias resize cmd_resize

# ----------------------------------------------------------------------
# USAGE: setview ?-rotate <mtx>? ?-scale <mtx>? ?-center <mtx>? ?-global <mtx>?
#
# Sets the view matrix for one or more components of the view.  This
# is a convenient way of getting a view for a particular frame just
# right in one shot.
# ----------------------------------------------------------------------
proc cmd_setview {args} {
    if {[llength $args] == 8} {
        # setting all matrices? then start clean
        display resetview
    }
    foreach {key val} $args {
        switch -- $key {
            -rotate {
                molinfo top set rotate_matrix [list $val]
            }
            -scale {
                molinfo top set scale_matrix [list $val]
            }
            -center {
                molinfo top set center_matrix [list $val]
            }
            -global {
                molinfo top set global_matrix [list $val]
            }
            default {
                error "bad option \"$key\": should be -rotate, -scale, -center, or -global"
            }
        }
    }
}
$parser alias setview cmd_setview

# ----------------------------------------------------------------------
# USAGE: drag start|end
#
# Resizes the visualization window to the given width <w> and height
# <h>.  The next image sent should be this size.
# ----------------------------------------------------------------------
proc cmd_drag {action} {
    global DisplayProps

    switch -- $action {
        start {
            # simplify rendering so it goes faster during drag operations
            set neww [expr {round($DisplayProps(framew)/2.0)}]
            set newh [expr {round($DisplayProps(frameh)/2.0)}]
            server_safe_resize $neww $newh
            display rendermode Normal
            display shadows off

            foreach nmol [molinfo list] {
                set max [molinfo $nmol get numreps]
                for {set nrep 0} {$nrep < $max} {incr nrep} {
                    mol modstyle $nrep $nmol "Lines"
                }
            }
        }
        end {
            # put original rendering options back
            server_safe_resize $DisplayProps(framew) $DisplayProps(frameh)
            display rendermode $DisplayProps(rendermode)
            display shadows $DisplayProps(shadows)

            # restore rendering methods for all representations
            foreach nmol [molinfo list] {
                set max [molinfo $nmol get numreps]
                for {set nrep 0} {$nrep < $max} {incr nrep} {
                    mol modstyle $nrep $nmol $DisplayProps(rep-$nmol-$nrep)
                }
            }
        }
        default {
            error "bad option \"$action\": should be start or end"
        }
    }
}
$parser alias drag cmd_drag

# ----------------------------------------------------------------------
# USAGE: smoothreps <value>
#
# Changes the smoothing factor for all representations of the current
# molecule.
# ----------------------------------------------------------------------
proc cmd_smoothreps {val} {
    if {$val < 0} {
        error "bad smoothing value \"$val\": should be >= 0"
    }
    foreach nmol [molinfo list] {
        set max [molinfo $nmol get numreps]
        for {set nrep 0} {$nrep < $max} {incr nrep} {
            mol smoothrep $nmol $nrep $val
        }
    }
}
$parser alias smoothreps cmd_smoothreps

# ----------------------------------------------------------------------
# USAGE: animate <option> <args>...
# USAGE: rock off
# USAGE: rock x|y|z by <step> ?<n>?
#
# The usual VMD "animate" and "rock" commands are problematic for this
# server.  If we're going to rock or play the animation, the client
# will do it.  Intercept any "animate" and "rock" commands in the scene
# scripts and do nothing.
# ----------------------------------------------------------------------
proc cmd_animate {args} {
    # do nothing
}
$parser alias animate cmd_animate

proc cmd_rock {args} {
    # do nothing
}
$parser alias rock cmd_rock

# ----------------------------------------------------------------------
# USAGE: load <file> <file>...
#
# Loads the molecule data from one or more files, which may be PDB,
# DCD, PSF, etc.
# ----------------------------------------------------------------------
proc cmd_load {args} {
    # clear all existing molecules
    foreach nmol [molinfo list] {
        mol delete $nmol
    }

    # load new files
    set op "new"
    foreach file $args {
        mol $op $file waitfor all
        set op "addfile"
    }

    # BE CAREFUL -- force a "display update" here
    # that triggers something in VMD that changes view matrices now,
    # so if we change them later, the new values stick
    display update
}
$parser alias load cmd_load

# ----------------------------------------------------------------------
# USAGE: scene define <name> <script>
# USAGE: scene show <name> ?-before <viewCmd>? ?-after <viewCmd>?
# USAGE: scene clear
# USAGE: scene forget ?<name> <name>...?
#
# Used to define and manipulate scenes of the trajectory information
# loaded previously by the "load" command.  The "define" operation
# defines the script that loads a scene called <name>.  The "show"
# operation executes that script to show the scene.  The "clear"
# operation clears the current scene (usually in preparation for 
# showing another scene).  The "forget" operation erases one or more
# scene definitions; if no names are specified, then all scenes are
# forgotten.
# ----------------------------------------------------------------------
proc cmd_scene {option args} {
    global Scenes Views DisplayProps parser

    switch -- $option {
        define {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"scene define name script\""
            }
            set name [lindex $args 0]
            set script [lindex $args 1]
            set Scenes($name) $script
        }
        show {
            if {[llength $args] < 1 || [llength $args] > 5} {
                error "wrong # args: should be \"scene show name ?-before cmd? ?-after cmd?\""
            }
            set name [lindex $args 0]
            if {![info exists Scenes($name)]} {
                error "bad scene name \"$name\": should be one of [join [array names Scenes] {, }]"
            }

            set triggers(before) ""
            set triggers(after) ""
            foreach {key val} [lrange $args 1 end] {
                switch -- $key {
                    -before { set triggers(before) $val }
                    -after { set triggers(after) $val }
                    default { error "bad option \"$key\": should be -before, -after" }
                }
            }

            # if -before arg was given, send back the view right now
            if {$triggers(before) ne "" && $Scenes(@CURRENT) ne ""} {
                cmd_tellme $triggers(before) getview
            }

            # clear the old scene
            cmd_scene clear
            display resetview

            # use a safe interp to keep things safe
            foreach val [$parser eval {info vars}] {
                # clear all variables created by previous scripts
                $parser eval [list catch [list unset $val]]
            }
            if {[catch {$parser eval $Scenes($name)} result]} {
                error "$result\nwhile loading scene \"$name\""
            }

            # capture display characteristics in case we ever need to reset
            set DisplayProps(rendermode) [display get rendermode]
            set DisplayProps(shadows) [display get shadows]

            foreach nmol [molinfo list] {
                set max [molinfo $nmol get numreps]
                for {set nrep 0} {$nrep < $max} {incr nrep} {
                    set style [lindex [molinfo $nmol get "{rep $nrep}"] 0]
                    set DisplayProps(rep-$nmol-$nrep) $style
                }
            }

            # store the scene name for later
            set Scenes(@CURRENT) $name

            # if -after arg was given, send back the view after the script
            if {$triggers(after) ne ""} {
                cmd_tellme $triggers(after) getview
            }
        }
        clear {
            foreach mol [molinfo list] {
                set numOfRep [lindex [mol list $mol] 12]
                for {set i 1} {$i <= $numOfRep} {incr i} {
                    mol delrep 0 $mol
                }
            }
            set Scenes(@CURRENT) ""
            catch {unset Views}

            # reset the server properties
            axes location off
            color Display Background black
            eval $DisplayProps(options)
        }
        forget {
            if {[llength $args] == 0} {
                set args [array names Scenes]
            }
            foreach name $args {
                if {$name eq "@CURRENT"} continue
                catch {unset Scenes($name)}
                if {$name eq $Scenes(@CURRENT)} {
                    set Scenes(@CURRENT) ""
                }
            }
        }
        default {
            error "bad option \"$option\": should be define, show, clear, forget"
        }
    }
}
$parser alias scene cmd_scene

# ----------------------------------------------------------------------
# USAGE: frames defview <frame> {matrixNames...} {matrixValues...}
# USAGE: frames time <epochValue> <start> ?<finish>? ?<inc>? ?-defview?
# USAGE: frames rotate <epochValue> <xa> <ya> <za> <number>
# USAGE: frames max
#
# Used to request one or more frames for an animation.  A "time"
# animation is a series of frames between two time points.  A "rotate"
# animation is a series of frames that rotate the view 360 degrees.
#
# The <epochValue> is passed by the client to indicate the relevance of
# the request.  Whenever the client enters a new epoch, it is no longer
# concerned with any earlier epochs, so the server can ignore pending
# images that are out of date.  The server sends back the epoch with
# all frames so the client can understand if the frames are relevant.
#
# The "defview" operation sets the default view associated with each
# frame.  Animation scripts can change the default view to a series of
# fly-through views.  This operation provides a way of storing those
# views.
#
# For a "time" animation, the <start> is a number of a requested frame.
# The <finish> is the last frame in the series.  The <inc> is the step
# by which the frames should be generated, which may be larger than 1.
#
# For a "rotate" animation, the <xa>,<ya>,<za> angles indicate the
# direction of the rotation.  The <number> is the number of frames
# requested for a full 360 degree rotation.
#
# The "frames max" query returns the maximum number of frames in the
# trajectory.  The server uses this to figure out the limits of
# animation.
# ----------------------------------------------------------------------
proc cmd_frames {what args} {
    global client Epoch Work Views

    # check incoming parameters
    switch -- $what {
      time {
        set epochValue [lindex $args 0]
        set start [lindex $args 1]

        set i [lsearch $args -defview]
        if {$i >= 0} {
            set defview 1
            set args [lreplace $args $i $i]
        } else {
            set defview 0
        }

        set finish [lindex $args 2]
        if {$finish eq ""} { set finish $start }
        set inc [lindex $args 3]
        if {$inc eq ""} { set inc 1 }

        if {![string is integer $finish]} {
            server_oops $client "bad animation end \"$finish\" should be integer"
            return
        }
        if {![string is integer $inc] || $inc == 0} {
            server_oops $client "bad animation inc \"$inc\" should be non-zero integer"
            return
        }
        if {($finish < $start && $inc > 0) || ($finish > $start && $inc < 0)} {
            server_oops $client "bad animation limits: from $start to $finish by $inc"
        }

        # new epoch? then clean out work queue
        if {$epochValue > $Epoch} {
            catch {unset Work}
            set Work(queue) ""
            set Epoch $epochValue
        }

        # add these frames to the queue
        if {$inc > 0} {
            # generate frames in play>> direction
            for {set n $start} {$n <= $finish} {incr n $inc} {
                if {![info exists Work($n)]} {
                    lappend Work(queue) [list epoch $epochValue frame $n num $n defview $defview]
                    set Work($n) 1
                }
            }
        } else {
            # generate frames in <<play direction
            for {set n $start} {$n >= $finish} {incr n $inc} {
                if {![info exists Work($n)]} {
                    lappend Work(queue) [list epoch $epochValue frame $n num $n defview $defview]
                    set Work($n) 1
                }
            }
        }
      }
      rotate {
        set epochValue [lindex $args 0]
        set mx [lindex $args 1]
        if {![string is double -strict $mx]} {
            server_oops $client "bad mx rotation value \"$mx\" should be double"
            return
        }
        set my [lindex $args 2]
        if {![string is double -strict $my]} {
            server_oops $client "bad my rotation value \"$my\" should be double"
            return
        }
        set mz [lindex $args 3]
        if {![string is double -strict $mz]} {
            server_oops $client "bad mz rotation value \"$mz\" should be double"
            return
        }
        set num [lindex $args 4]
        if {![string is integer -strict $num] || $num < 2} {
            server_oops $client "bad number of rotation frames \"$num\" should be integer > 1"
            return
        }

        #
        # Compute the rotation matrix for each rotated view.
        # Start with the current rotation matrix.  Rotate that around
        # a vector perpendicular to the plane of rotation for the given
        # angles (mx,my,mz).  Find vector that by rotating some vector
        # such as (1,1,1) by the angles (mx,my,mz).  Do a couple of
        # times and compute the differences between those vectors.
        # Then, compute the cross product of the differences.  The
        # result is the axis of rotation.
        #
        set lastrotx [trans axis x $mx deg]
        set lastroty [trans axis y $my deg]
        set lastrotz [trans axis z $mz deg]
        set lastrot [transmult $lastrotx $lastroty $lastrotz]

        set lastvec [list 1 1 1]
        foreach v {1 2} {
            foreach row $lastrot comp {x y z w} {
                # multiply each row by last vector
                set vec($comp) 0
                for {set i 0} {$i < 3} {incr i} {
                    set vec($comp) [expr {$vec($comp) + [lindex $row $i]}]
                }
            }
            set vec${v}(x) [expr {$vec(x)-[lindex $lastvec 0]}]
            set vec${v}(y) [expr {$vec(y)-[lindex $lastvec 1]}]
            set vec${v}(z) [expr {$vec(z)-[lindex $lastvec 2]}]

            set lastvec [list $vec(x) $vec(y) $vec(z)]
            set lastrot [transmult $lastrot $lastrotx $lastroty $lastrotz]
        }

        set crx [expr {$vec1(y)*$vec2(z)-$vec1(z)*$vec2(y)}]
        set cry [expr {$vec1(z)*$vec2(x)-$vec1(x)*$vec2(z)}]
        set crz [expr {$vec1(x)*$vec2(y)-$vec1(y)*$vec2(x)}]

        set angle [expr {360.0/$num}]
        set rotby [transabout [list $crx $cry $crz] $angle deg]
        set rotm [lindex [molinfo top get rotate_matrix] 0]

        # compute cross product of (1,1,1,0) and rotated vector from above

        for {set n 0} {$n < $num} {incr n} {
            lappend Work(queue) [list epoch $epochValue rotate $rotm num $n defview 0]
            set rotm [transmult $rotby $rotm]
            set Work($n) 1
        }
      }
      defview {
          if {[llength $args] != 3} { error "wrong # args: should be \"defview matrixNameList matrixValueList\"" }
          set n [lindex $args 0]
          if {![string is int $n]} { error "bad frame value \"$n\"" }
          set Views($n) [lrange $args 1 end]
      }
      max {
        set nmol [lindex [molinfo list] 0]
        if {$nmol ne ""} {
            return [molinfo $nmol get numframes]
        }
        return 0
      }
      default {
        error "bad option \"$what\": should be defview, time, rotate, max"
      }
    }

    # service the queue at some point
    server_send_image -eventually
}
$parser alias frames cmd_frames

# ----------------------------------------------------------------------
# USAGE: getview
#
# Used to query the scaling and centering of the initial view set
# by VMD after a molecule is loaded.  Returns the following:
#   <viewName> -rotate <mtx> -global <mtx> -scale <mtx> -center <mtx>
# ----------------------------------------------------------------------
proc cmd_getview {} {
    global Scenes

    if {[llength [molinfo list]] == 0} { return "" }
    if {$Scenes(@CURRENT) eq ""} { return "" }

    set rval [list $Scenes(@CURRENT)]  ;# start with the scene name

    lappend rval -rotate [lindex [molinfo top get rotate_matrix] 0] \
                 -scale [lindex [molinfo top get scale_matrix] 0] \
                 -center [lindex [molinfo top get center_matrix] 0] \
                 -global [lindex [molinfo top get global_matrix] 0]

    return $rval
}
$parser alias getview cmd_getview

#
# USAGE: server_safe_resize <width> <height>
#
# Use this version instead of "display resize" whenever possible.
# The VMD "display resize" goes into the event loop, so calling that
# causes things to execute out of order.  Use this method instead to
# store the change and actually resize later.
#
proc server_safe_resize {w h} {
    global DisplaySize

    if {$w != $DisplaySize(w) || $h != $DisplaySize(h)} {
        set DisplaySize(w) $w
        set DisplaySize(h) $h
        set DisplaySize(changed) yes
    }
}

# ----------------------------------------------------------------------
# SERVER CORE
# ----------------------------------------------------------------------
proc server_accept {cid addr port} {
    global env

    fileevent $cid readable [list server_handle $cid $cid]
    fconfigure $cid -buffering none -blocking 0

    if {[info exists env(LOCAL)]} {
        # identify server type to this client
        # VMD on the hub has this built in, but stock versions can
        # set the environment variable as a work-around
        puts $cid "vmd 0.1"
    }
}

proc server_handle {cin cout} {
    global parser buffer client 

    if {[gets $cin line] < 0} {
        # when client drops connection, we can exit
        # nanoscale will spawn a new server next time we need it
        if {[eof $cin]} {
            server_exit $cin $cout
        }
    } else {
        append buffer($cin) $line "\n"
        if {[info complete $buffer($cin)]} {
            set request $buffer($cin)
            set buffer($cin) ""
            set client $cout
            if {[catch {$parser eval $request} result] == 0} {
                server_send_image -eventually
            } else {
                server_oops $cout $result
		if { [string match "invalid command*" $result] } {
                    bgerror "I got a invalid command: $result"
		    exit 1
		}
            }
        }
    }
}

proc server_send {cout} {
    global Epoch Sendqueue

    # grab the next chunk of output and send it along
    # discard any chunks from an older epoch
    while {[llength $Sendqueue] > 0} {
        set chunk [lindex $Sendqueue 0]
        set Sendqueue [lrange $Sendqueue 1 end]

        catch {unset data}; array set data $chunk
        if {$data(epoch) < 0 || $data(epoch) == $Epoch} {
            catch {puts $cout $data(cmd)}

            # if this command has a binary data block, send it specially
            if {[string length $data(bytes)] > 0} {
                fconfigure $cout -translation binary
                catch {puts $cout $data(bytes)}
                fconfigure $cout -translation auto
            }
            break
        }
    }

    # nothing left? Then stop callbacks until we get more
    if {[llength $Sendqueue] == 0} {
        fileevent $cout writable ""
        server_send_image -eventually
    }
}

proc server_exit {cin cout} {
    catch {close $cin}
    catch {exit 0}
}

# ----------------------------------------------------------------------
# SERVER RESPONSES
# ----------------------------------------------------------------------

# turn off constant updates -- only need them during server_send_image
display update off

proc server_send_image {{when -now}} {
    global client Epoch Work Views Sendqueue DisplaySize

    if {$when eq "-eventually"} {
        after cancel server_send_image
        after 1 server_send_image
        return
    } elseif {$when ne "-now"} {
        error "bad option \"$when\" for server_send_image: should be -now or -eventually"
    }

    # is there a display resize pending? then resize and try again later
    if {$DisplaySize(changed)} {
        set DisplaySize(changed) 0
        after idle [list display resize $DisplaySize(w) $DisplaySize(h)]
        after 20 server_send_image
        return
    }

    # loop through requests in the work queue and skip any from an older epoch
    while {1} {
        if {[llength $Work(queue)] == 0} {
            return
        }

        set rec [lindex $Work(queue) 0]
        set Work(queue) [lrange $Work(queue) 1 end]

        catch {unset item}; array set item $rec
        if {$item(epoch) < $Epoch} {
            catch {unset Work($item(num))}
            continue
        }

        # set the frame characteristics and render this frame
        if {[info exists item(frame)]} {
            animate goto $item(frame)
        } elseif {[info exists item(rotate)]} {
            molinfo top set rotate_matrix [list $item(rotate)]
            # send rotation matrix back to the client so we can pause later
            server_send_latest $client [list nv>rotatemtx $item(num) $item(rotate)]
        } else {
            puts "ERROR: bad work frame: [array get item]"
        }

        # flag to use the stored default view? then set that
        if {[info exists item(defview)] && $item(defview)} {
            if {[info exists Views($item(frame))]} {
                eval molinfo top set $Views($item(frame))
            }
        }
        catch {unset Work($item(num))}
        break
    }

    # force VMD to update and grab the screen
    display update
    tkrender SnapShot

    set data [SnapShot data -format PPM]
    server_send_latest $client "nv>image epoch $item(epoch) frame $item(num) length [string length $data]" $data

    # if there's more work in the queue, try again later
    if {[llength $Work(queue)] > 0} {
        after 1 server_send_image
    }
}

proc server_send_result {cout cmd {data ""}} {
    global Sendqueue

    # add this result to the output queue
    # use the epoch -1 to force the send even if the epoch has changed
    lappend Sendqueue [list epoch -1 cmd $cmd bytes $data]
    fileevent $cout writable [list server_send $cout]
}

proc server_send_latest {cout cmd {data ""}} {
    global Epoch Sendqueue

    # add this result to the output queue
    # wait until the client is ready, then send the output
    lappend Sendqueue [list epoch $Epoch cmd $cmd bytes $data]
    fileevent $cout writable [list server_send $cout]
}

proc server_oops {cout mesg} {
    # remove newlines -- all lines must start with nv>
    set mesg [string map {\n " "} $mesg]
    server_send_result $cout "nv>oops [list $mesg]"
}

if {$Paradigm eq "socket"} {
    socket -server server_accept 2018
} else {
    set cin $vmd_client(read)
    set cout $vmd_client(write)

    fileevent $cin readable [list server_handle $cin $cout]
    fconfigure $cout -buffering none -blocking 0
}

# vmd automatically drops into an event loop at this point...
#
# The VMD TCL interpreter is by default interactive.  Their version
# of tkconsole always turns this on.  Turn this off
# so that unknown commands like "scene" don't get exec-ed. 
set ::tcl_interactive 0
