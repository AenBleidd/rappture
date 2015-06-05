# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: service - represents a tool embedded within another tool
#
#  The tool.xml description for a Rappture tool can contain one or
#  more embedded <tool> references.  Each Rappture::Service object
#  represents an embedded <tool> and manages its operation,
#  contributing some service to the overall tool.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require BLT

itcl::class Rappture::Service {
    constructor {owner path args} { # defined below }
    destructor { # defined below }

    public method control {}
    public method input {}
    public method output {args}

    public method run {}
    public method abort {}
    public method clear {}

    protected method _link {from to}

    private variable _owner ""    ;# thing managing this service
    private variable _path ""     ;# path to the <tool> description in _owner
    private variable _tool ""     ;# underlying tool for this service
    private variable _control ""  ;# control: auto/manual
    private variable _show        ;# <show> info for input/output methods
    private variable _obj2path    ;# maps object in _show => service path
    private variable _path2widget ;# maps path for object in _show => widget
    private variable _path2path   ;# maps path for object in _show => link path
    private variable _result ""   ;# result from last run
}

# ----------------------------------------------------------------------
# CONSTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Service::constructor {owner path args} {
    if {[catch {$owner isa Rappture::ControlOwner} valid] != 0 || !$valid} {
        error "bad object \"$owner\": should be Rappture::ControlOwner"
    }
    set _owner $owner
    set _path $path

    #
    # Load up the tool description from the <interface> file.
    #
    set intf [string trim [$_owner xml get $path.interface]]
    if {"" == $intf} {
        puts "can't find <interface> description for tool at $path"
    } else {
        set installdir [[$_owner tool] installdir]
        regsub -all @tool $intf $installdir intf

        set xmlobj [Rappture::library $intf]
        set installdir [file dirname $intf]
        set _tool [Rappture::Tool ::\#auto $xmlobj $installdir]
        set _control [string trim [$_tool xml get tool.control]]

        #
        # Scan through the <tool> and establish all of the
        # relationships:
        #
        #   <show> ... Add to list of "input" for this service.
        #              Caller will add controls to the interface.
        #
        #   <link> ... Link this value to another input/output
        #              that exists in the containing tool.
        #
        #   <set> .... Hard-code the value for this input/output.
        #
        foreach dir {input output} {
            set _show($dir) ""
            foreach cname [$_owner xml children $path.$dir] {
                set ppath $path.$dir.$cname

                set spath [string trim [$_owner xml get $ppath.path]]
                if {"" == $spath} {
                    error "missing <path> at $ppath"
                }

                set type [$_owner xml element -as type $ppath]
                switch -- $type {
                  show {
puts "show: $spath"
                      set tpath [string trim [$_owner xml get $ppath.to]]
                      if {"" == $tpath && $dir == "input"} {
                          error "missing <to> at $ppath"
                      }
                      set obj [$_tool xml element -as object $spath]
                      puts " => $obj"
                      lappend _show($dir) $obj
                      set _obj2path($obj) $spath

                      if {$dir == "input"} {
                          puts "link: $tpath => $spath"
                          $_owner notify add $this $tpath \
                              [itcl::code $this _link $tpath $spath]
                      }
                  }
                  link {
                      set tpath [string trim [$_owner xml get $ppath.to]]
                      if {"" == $tpath} {
                          error "missing <to> at $ppath"
                      }
                      if {"" == [$_owner xml element $tpath]} {
                          error "bad <to> path \"$tpath\" at $ppath"
                      }
                      if {$dir == "input"} {
                          puts "link: $tpath => $spath"
                          $_owner notify add $this $tpath \
                              [itcl::code $this _link $tpath $spath]
                      } else {
                          puts "path2path: $spath => $tpath"
                          set _path2path($spath) $tpath
                      }
                  }
                    set {
                        if {"" == [$_owner xml element $ppath.value]} {
                        error "missing <value> at $ppath"
                    }
puts "set: $spath from $ppath.value"
                    $_tool xml copy $spath from \
                        [$_owner xml object] $ppath.value
                  }
                }
            }
        }

        if {$_control != "auto"} {
            set _show(control) [$_tool xml element -as object tool.control]
        } else {
            set _show(control) ""
        }
    }

    eval configure $args
}

# ----------------------------------------------------------------------
# DESTRUCTOR
# ----------------------------------------------------------------------
itcl::body Rappture::Service::destructor {} {
    foreach dir [array names _show] {
        foreach obj $_show($dir) {
            itcl::delete object $obj
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: control
#
# Used by the container that displays this service to determine what
# the control for this service should look like.  Returns "" if
# there is no control--the service is invoked automatically whenever
# the inputs change.  Otherwise, it returns a list of the form
# {key value key value ...} with attributes that configure the button
# controlling this service.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::control {} {
    return $_show(control)
}

# ----------------------------------------------------------------------
# USAGE: input
#
# Used by the container that displays this service to describe any
# inputs for this service that should be added to the main service.
# Returns a list of XML objects representing various input controls.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::input {} {
    return $_show(input)
}

# ----------------------------------------------------------------------
# USAGE: output
# USAGE: output for <object> <widget>
#
# Used by the container that displays this service to describe any
# outputs for this service that should be added to the main service.
#
# With no args, it returns a list of XML objects representing various
# outputs.  The caller uses this information to create various output
# widgets.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::output {args} {
    if {[llength $args] == 0} {
        return $_show(output)
    }
    set option [lindex $args 0]
    switch -- $option {
        for {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"output for object widget\""
            }
            set obj [lindex $args 1]
            set win [lindex $args 2]
            if {[info exists _obj2path($obj)]} {
                set path $_obj2path($obj)
puts "OUTPUT $path => $win"
                set _path2widget($path) $win
            } else {
                puts "error: don't recognize output object $obj"
            }
        }
        default {
            error "bad option \"$option\": should be for"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: run
#
# This method causes the service to run.  All widgets are synchronized
# to the current XML representation, and a "driver.xml" file is
# created as the input for the run.  That file is fed to the tool
# according to the <tool><command> string, and the job is executed.
#
# All outputs are pushed out to the tool containing this service
# according to the <outputs> directive for the service.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::run {} {
puts "running..."
    clear
    foreach {status result} [$_tool run] break

    if {$status == 0 && $result != "ABORT"} {
        if {[regexp {=RAPPTURE-RUN=>([^\n]+)} $result match file]} {
            set xmlobj [Rappture::library $file]
            #
            # Scan through all outputs and copy them to the final output
            # for the tool.  If they have widgets associated with them,
            # set the value for the widget.
            #
puts "showing output..."
            foreach cname [$xmlobj children output] {
                set path output.$cname
puts " for $path"
                $_owner xml copy $path from $xmlobj $path

                if {[info exists _path2widget($path)]} {
                    set obj [$xmlobj element -as object $path]
puts " sending $obj to $_path2widget($path)"
                    $_path2widget($path) value $obj
                }
                if {[info exists _path2path($path)]} {
puts " sending $path to $_path2path($path)"
                    $_owner xml copy $_path2path($path) from $xmlobj $path
                    set w [$_owner widgetfor $_path2path($path)]
                    if {$w != ""} {
                        set obj [$_owner xml element -as object $_path2path($path)]
                        $w value $obj
                    }
                }
            }
            set _result $xmlobj
        } else {
            set status 1
            set result "Can't find result file in output:\n\n$result"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: abort
#
# Clients use this during a "run" to abort the current job.
# Kills the job and forces the "run" method to return.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::abort {} {
    $_tool abort
}

# ----------------------------------------------------------------------
# USAGE: clear
#
# Clears any result hanging around from the last run.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::clear {} {
    if {"" != $_result} {
        foreach cname [$_result children output] {
            set path output.$cname

            if {[info exists _path2widget($path)]} {
                $_path2widget($path) value ""
            }
        }
        itcl::delete object $_result
        set _result ""
    }
}

# ----------------------------------------------------------------------
# USAGE: _link <fromPath> <toPath>
#
# Used internally to link the value at <fromPath> in the outer tool
# to the value at <toPath> for this service.  If this service is
# automatic and <toPath> is an input, this also invokes the service.
# ----------------------------------------------------------------------
itcl::body Rappture::Service::_link {from to} {
    $_tool xml copy $to from [$_owner xml object] $from
    if {$_control == "auto" && [regexp -nocase {^input\.} $to]} {
        after cancel [itcl::code $this run]
        after idle [itcl::code $this run]
    }
}
