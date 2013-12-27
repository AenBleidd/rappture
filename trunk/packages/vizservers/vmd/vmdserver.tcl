# ----------------------------------------------------------------------
#  HUBZERO: server for VMD
#
#  This program runs VMD and acts as a server for client applications.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2013 - HUBzero Foundation, LLC
# ======================================================================

set ::tcl_interactive 0

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
#display resize 300 300

# initialize work queue and epoch counter (see server_send_image)
set Epoch 0
set Work(queue) ""
set Sendqueue ""

set parser [interp create -safe]

foreach cmd {
  vmdinfo
  vmdbench
  animate
  color
  axes
  display
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
  rock
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
# USAGE: reset
#
# Executes the "command arg arg..." string in the server and substitutes
# the result into the template string in place of each "%v" field.
# Sends the result back to the client.
# ----------------------------------------------------------------------
proc cmd_reset {} {
    global client

    # reset the view so we get a good scale matrix below
    display resetview

    # reset scale -- figure out size by querying molinfo for first molecule
    set nmol [lindex [molinfo list] 0]
    if {$nmol ne ""} {
        set matrix [molinfo $nmol get scale_matrix]
        set sf [lindex [lindex [lindex $matrix 0] 0] 0]
        vmd_scale to $sf
        server_send_result $client "nv>scale $sf"
    }

    axes location off
}
$parser alias reset cmd_reset

# ----------------------------------------------------------------------
# USAGE: resize <w> <h>
#
# Resizes the visualization window to the given width <w> and height
# <h>.  The next image sent should be this size.
# ----------------------------------------------------------------------
proc cmd_resize {w h} {
    display resize $w $h
    display update
}
$parser alias resize cmd_resize

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

    # clear any existing views
    cmd_view forget

    # load new files
    set op "new"
    foreach file $args {
        mol $op $file waitfor all
        set op "addfile"
    }
}
$parser alias load cmd_load

# ----------------------------------------------------------------------
# USAGE: view define <name> <script>
# USAGE: view show <name>
# USAGE: view clear
# USAGE: view forget ?<name> <name>...?
#
# Used to define and manipulate views of the trajectory information
# loaded previously by the "load" command.  The "define" operation
# defines the script that loads a view called <name>.  The "show"
# operation executes that script to show the view.  The "clear"
# operation clears the current view (usually in preparation for 
# showing another view).  The "forget" operation erases one or more
# view definitions; if no names are specified, then all views are
# forgotten.
# ----------------------------------------------------------------------
proc cmd_view {option args} {
    global Views parser
    switch -- $option {
        define {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"view define name script\""
            }
            set name [lindex $args 0]
            set script [lindex $args 1]
            set Views($name) $script
        }
        show {
            if {[llength $args] != 1} {
                error "wrong # args: should be \"view show name\""
            }
	    set name [lindex $args 0]
            if {![info exists Views($name)]} {
                error "bad view name \"$name\": should be one of [join [array names Views] {, }]"
            }

            # clear the old view
            cmd_view clear

            # use a safe interp to keep things safe
            if {[catch {$parser eval $Views($name)} result]} {
                error "$result\nwhile loading view \"$name\""
            }
        }
        clear {
            set numOfRep [lindex [mol list top] 12]
            for {set i 1} {$i <= $numOfRep} {incr i} {
                mol delrep top 0
            }
            cmd_reset
        }
        forget {
            if {[llength $args] == 0} {
                set args [array names Views]
            }
            foreach name $args {
                catch {unset Views($name)}
            }
        }
        default {
            error "bad option \"$option\": should be define, show, clear, forget"
        }
    }
}
$parser alias view cmd_view

# ----------------------------------------------------------------------
# USAGE: frames time <epochValue> <start> ?<finish>? ?<inc>?
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
    global client Epoch Work

    # check incoming parameters
    switch -- $what {
      time {
        set epochValue [lindex $args 0]
        set start [lindex $args 1]
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
                    lappend Work(queue) [list epoch $epochValue frame $n num $n]
                    set Work($n) 1
                }
            }
        } else {
            # generate frames in <<play direction
            for {set n $start} {$n >= $finish} {incr n $inc} {
                if {![info exists Work($n)]} {
                    lappend Work(queue) [list epoch $epochValue frame $n num $n]
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
        if {![string is integer -strict $num] || $num <= 0} {
            server_oops $client "bad number of rotation frames \"$num\" should be integer > 0"
            return
        }

        set rot [list $mx $my $mz]
        for {set n 0} {$n < $num} {incr n} {
            lappend Work(queue) [list epoch $epochValue rotate $rot num $n]
            set Work($n) 1
        }
      }
      max {
        set nmol [lindex [molinfo list] 0]
        if {$nmol ne ""} {
            return [molinfo $nmol get numframes]
        }
        return 0
      }
      default {
        error "bad option \"$what\": should be time, rotate, max"
      }
    }

    # service the queue at some point
    server_send_image -eventually
}
$parser alias frames cmd_frames

# ----------------------------------------------------------------------
# SERVER CORE
# ----------------------------------------------------------------------
proc server_accept {cid addr port} {
    fileevent $cid readable [list server_handle $cid $cid]
    fconfigure $cid -buffering none -blocking 0

    # identify server type to this client
    puts $cid "vmd 0.1"
}

proc server_handle {cin cout} {
    global parser buffer client

    if {[gets $cin request] < 0} {
        # when client drops connection, we can exit
        # nanoscale will spawn a new server next time we need it
        server_exit $cin $cout
    } else {
        append buffer($cin) $request "\n"
        if {[info complete $buffer($cin)]} {
            set request $buffer($cin)
            set buffer($cin) ""
            set client $cout
            if {[catch {$parser eval $request} result] == 0} {
                server_send_image -eventually
            } else {
                server_oops $cout $result
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
        if {$data(epoch) == $Epoch} {
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
#

#set the screen to a good size
display resize 300 300


# turn off constant updates -- only need them during server_send_image
display update off

proc server_send_image {{when -now}} {
    global client Epoch Work Sendqueue

    if {$when eq "-eventually"} {
        after cancel server_send_image
        after 1 server_send_image
        return
    } elseif {$when ne "-now"} {
        error "bad option \"$when\" for server_send_image: should be -now or -eventually"
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
            foreach {mx my mz} $item(rotate) break
            rotate x by $mx
            rotate y by $my
            rotate z by $mz
        } else {
            puts "ERROR: bad work frame: [array get item]"
        }
        catch {unset Work($item(num))}
        break
    }

    # force VMD to update and grab the screen
    display update
    tkrender SnapShot

    set data [SnapShot data -format PPM]
    server_send_result $client "nv>image epoch $item(epoch) frame $item(num) length [string length $data]" $data

    # if there's more work in the queue, try again later
    if {[llength $Work(queue)] > 0} {
        after 1 server_send_image
    }
}

proc server_send_result {cout cmd {data ""}} {
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
    fileevent stdin readable [list server_handle stdin stdout]

    # identify server type to this client
    puts stdout "vmd 0.1"
    flush stdout
    fconfigure stdout -buffering none -blocking 0
}

# vmd automatically drops into an event loop at this point...
set ::tcl_interactive 0
