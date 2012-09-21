# ----------------------------------------------------------------------
#  LIBRARY: Handler
#  Base class for Client and Server.  Handles protocol declarations
#  for messages received by the handler, and knows how to process
#  messages.
# ----------------------------------------------------------------------
#  Michael McLennan (mmclennan@purdue.edu)
# ======================================================================
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

itcl::class Handler {
    private variable _namesp    ;# maps protocol name => namespace of cmds
    private variable _parser    ;# maps protocol name => parser for cmds
    private variable _buffer    ;# maps connection => cmd buffer
    private variable _protocol  ;# maps connection => protocol name
    private variable _cname     ;# maps connection => nice name for debug
    private common _counter 0

    constructor {args} {
        #
        # Define the DEFAULT protocol, which clients/servers use when
        # they first connect to define the protocol they intend to speak.
        #
        protocol DEFAULT
        define DEFAULT protocol {version} {
            variable handler
            variable cid
            $handler connectionSpeaks $cid $version
            return ""
        }
        define DEFAULT exception {message} {
            log error "ERROR: $message"
        }

        eval configure $args
    }

    destructor {
        foreach protocol [array names _parser] {
            interp delete $_parser($protocol)
        }
        foreach protocol [array names _namesp] {
            namespace delete $_namesp($protocol)
        }
    }

    public method protocol {name}
    public method define {protocol name arglist body}
    public method connections {{protocol *}}
    public method connectionName {cid {name ""}}
    public method connectionSpeaks {cid protocol}

    protected method handle {cid}
    protected method finalize {protocol}
    protected method dropped {cid}
}

# ----------------------------------------------------------------------
#  USAGE: protocol <name>
#
#  Used to define a protocol that this client/server recognizes.
#  A protocol has an associated safe interpreter full of commands
#  that the client/server recognizes.  When each connection is
#  established, the other party must declare the  protocol that it
#  intends to speak up front, so the client/server can select the
#  appropriate interpreter for all incoming requests.
# ----------------------------------------------------------------------
itcl::body Handler::protocol {name} {
    if {[info exists _namesp($name)]} {
        error "protocol \"$name\" already defined"
    }
    set _namesp($name) "[namespace current]::bodies[incr _counter]"
    namespace eval $_namesp($name) {}
    set _parser($name) [interp create -safe]
    foreach cmd [$_parser($name) eval {info commands}] {
        $_parser($name) hide $cmd
    }
    $_parser($name) invokehidden proc unknown {args} {}
    $_parser($name) expose error
}

# ----------------------------------------------------------------------
#  USAGE: define <protocol> <name> <arglist> <body>
#
#  Used to define a command that this handler recognizes.  The command
#  is called <name> in the safe interpreter associated with the given
#  <protocol>, which should have been defined previously via the
#  "protocol" method.  The new command exists with the same name in a
#  special namespace in the main interpreter.  It is implemented with
#  an argument list <arglist> and a <body> of Tcl code.
# ----------------------------------------------------------------------
itcl::body Handler::define {protocol name arglist body} {
    if {![info exists _namesp($protocol)]} {
        error "can't define command \"$name\": protocol \"$protocol\" doesn't exist"
    }
    proc [set _namesp($protocol)]::$name $arglist $body
    $_parser($protocol) alias $name [set _namesp($protocol)]::$name
    finalize $protocol
}

# ----------------------------------------------------------------------
#  USAGE: connections ?<protocol>?
#
#  Returns a list of file handles for current connections that match
#  the glob-style <protocol> pattern.  If no pattern is given, then
#  it returns all connections.  Each handle can be passed to
#  connectionName and connectionSpeaks to get more information.
# ----------------------------------------------------------------------
itcl::body Handler::connections {{pattern *}} {
    set rlist ""
    foreach cid [array names _protocol] {
        if {[string match $pattern $_protocol($cid)]} {
            lappend rlist $cid
        }
    }
    return $rlist
}

# ----------------------------------------------------------------------
#  USAGE: connectionName <sockId> ?<name>?
#
#  Used to set/get the nice name associated with a <sockId> connection.
#  The nice name is used for log messages, and makes debugging easier
#  than chasing around a bunch of "sock3" handle names.  If no name
#  is specified, then it defaults to the file descriptor name.
# ----------------------------------------------------------------------
itcl::body Handler::connectionName {cid {name ""}} {
    if {[string length $name] > 0} {
        set _cname($cid) $name
    }
    if {[info exists _cname($cid)]} {
        return "$_cname($cid) ($cid)"
    }
    return $cid
}

# ----------------------------------------------------------------------
#  USAGE: connectionSpeaks <sockId> <protocol>
#
#  Used internally to define what protocol the entity on the other
#  side of the connection speaks.  This is usually invoked when that
#  entity sends the "protocol" message, and the built-in "protocol"
#  command in the DEFAULT parser uses this method to register the
#  protocol for the entity.
# ----------------------------------------------------------------------
itcl::body Handler::connectionSpeaks {cid protocol} {
    if {"DEFAULT" != $protocol && ![info exists _parser($protocol)]} {
        error "protocol \"$protocol\" not recognized"
    }
    set _protocol($cid) $protocol
}

# ----------------------------------------------------------------------
#  USAGE: handle <cid>
#
#  Invoked automatically whenever a message comes in on the file
#  handle <cid> from the entity on the other side of the connection.
#  This handler reads the message and executes it in a safe
#  interpreter, thereby responding to it.
# ----------------------------------------------------------------------
itcl::body Handler::handle {cid} {
    if {[gets $cid request] < 0} {
        dropped $cid
    } elseif {[info exists _protocol($cid)]} {
        # complete command? then process it below...
        append _buffer($cid) $request "\n"
        if {[info complete $_buffer($cid)]} {
            set request $_buffer($cid)
            set _buffer($cid) ""

            # what protocol does this entity speak?
            set protocol $_protocol($cid)

            # Some commands need to know the identity of the entity
            # on the other side of the connection.  Save it as a
            # global variable in the namespace where the protocol
            # command exists.
            set [set _namesp($protocol)]::handler $this
            set [set _namesp($protocol)]::cid $cid

            # execute the incoming command...
            set mesg " => "
            if {[catch {$_parser($protocol) eval $request} result] == 0} {
                if {[string length $result] > 0} {
                    catch {puts $cid $result}
                    append mesg "ok: $result"
                }
            } else {
                catch {puts $cid [list exception $result]}
                append mesg "exception: $result"
            }
            log debug "incoming message from [connectionName $cid]: [string trimright $request \n] $mesg"
        }
    }
}

# ----------------------------------------------------------------------
#  USAGE: finalize <protocol>
#
#  Called whenever a new command is added to the handler.  Updates
#  the "unknown" command to report a proper usage message (including
#  all valid keywords) when a bad command is encountered.
# ----------------------------------------------------------------------
itcl::body Handler::finalize {protocol} {
    set p $_parser($protocol)
    $p hide error
    $p hide unknown
    set cmds [lsort [$p invokehidden info commands]]
    $p expose error
    $p expose unknown

    $p invokehidden proc unknown {cmd args} [format {
        error "bad command \"$cmd\": should be %s"
    } [join $cmds {, }]]
}

# ----------------------------------------------------------------------
#  USAGE: dropped <cid>
#
#  Invoked automatically whenever a client connection drops, to
#  log the event and remove all trace of the client.  Derived classes
#  can override this method to perform other functions too.
# ----------------------------------------------------------------------
itcl::body Handler::dropped {cid} {
    log system "dropped: [connectionName $cid]"

    # connection has connection -- forget this entity
    catch {close $cid}
    catch {unset _buffer($cid)}
    catch {unset _protocol($cid)}
    catch {unset _cname($cid)}
}
