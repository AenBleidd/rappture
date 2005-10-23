# ----------------------------------------------------------------------
#  COMPONENT: filexfer - support for file transfer with user's desktop
#
#  This part supports the filexfer Java component that enables Rappture
#  to transfer files to and from the user's desktop.  It acts as a
#  web server.  When a Rappture application starts up, the user gets
#  a Web page that has an embedded VNC applet and the filexfer applet.
#  The Web page connects back to this server, which acts like a Web
#  server and delivers up the filexfer applet.  Then, the applet
#  connects back to the same server and stays in contact with the
#  Rappture application.  Whenever Rappture needs to deliver a file
#  to the user, it sends a message to the applet, which pops up a
#  Web page that connects back to the Rappture application and pulls
#  down the file.  When the user wants to upload a file, Rappture
#  again sends a message to the applet, which presents a Web form
#  and posts the file to the Rappture server.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2005  Purdue Research Foundation
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl

namespace eval Rappture { # forward declaration }
namespace eval Rappture::filexfer {
    variable enabled 0                 ;# set to 1 when this is running
    variable port 9001                 ;# start server on this port
    variable cookie ""                 ;# magic cookie for applet auth
    variable restrictClientAddress ""  ;# allow clients only from this addr
    variable clients                   ;# maps client socket => status
    variable buffer                    ;# request buffer for each client
    variable access                    ;# maps spooled file => access cookie

    # used to generate cookies -- see bakeCookie for details
    variable cookieChars {
        a b c d e f g h i j k l m n o p q r s t u v w x y z
        A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
        0 1 2 3 4 5 6 7 8 9
    }

    #
    # Translates mime type => file extension
    #        and file extension => mime type
    #
    # Used primarily for spooling data files.
    #
    variable mime2ext
    variable ext2mime
    variable mime2type

    foreach {mtype ext type} {
        text/plain                .txt    ascii
        text/html                 .html   ascii
        image/gif                 .gif    binary
        image/jpeg                .jpeg   binary
        application/postscript    .ps     ascii
        application/pdf           .pdf    binary
        application/octet-stream  .jar    binary
        application/octet-stream  .class  binary
    } {
        set mime2ext($mtype) $ext
        set ext2mime($ext) $mtype
        set mime2type($mtype) $type
    }

    #
    # Set up a safe interpreter for loading filexfer options...
    #
    variable optionParser [interp create -safe]
    foreach cmd [$optionParser eval {info commands}] {
        $optionParser hide $cmd
    }
    # this lets us ignore unrecognized commands in the file:
    $optionParser invokehidden proc unknown {args} {}

    $optionParser alias filexfer_port Rappture::filexfer::option_port
    $optionParser alias filexfer_cookie Rappture::filexfer::option_cookie
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::init
#
# Called in the main application to start listening to a particular
# port and start acting like a filexfer server.  Returns 1 if the
# server was enabled, and 0 otherwise.
# ----------------------------------------------------------------------
proc Rappture::filexfer::init {} {
    global env
    variable optionParser
    variable enabled
    variable port
    variable clients

    # keep a list of most recently activated clients
    set clients(order) ""

    #
    # Look for a $SESSION variable and a file called
    # ~/data/sessions/$SESSION/resources.  If found, then
    # load the settings from that file and start a server
    # for filexfer.
    #
    if {[info exists env(SESSION)]} {
        set file ~/data/sessions/$env(SESSION)/resources
        if {![file exists $file]} {
            return 0
        }
        if {[catch {
            set fid [open $file r]
            set info [read $fid]
            close $fid
            $optionParser eval $info
        } result]} {
            after 1 [list tk_messageBox -title Error -icon error -message "Error in resources file:\n$reslt"]
            return 0
        }

        #
        # If the prescribed port is busy, then exit with a special
        # status code so the middleware knows to try again with another
        # port.
        #
        # OH NO! THE DREADED ERROR CODE 9!
        #
        if {[catch {socket -server Rappture::filexfer::accept $port}]} {
            exit 9
        }
        set enabled 1
    }
    return $enabled
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::enabled
#
# Clients use this to see if the filexfer stuff is up and running.
# If so, then the GUI will provide "Download..." and other filexfer
# options.  If not, then Rappture must be running within an
# environment that doesn't support it.
# ----------------------------------------------------------------------
proc Rappture::filexfer::enabled {} {
    variable enabled
    return $enabled
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::spool <string> ?<filename>?
#
# Clients use this to send a file off to the user.  The <string>
# is stored in a file called <filename> in the user's spool directory.
# If there is already a file by that name, then the name is modified
# to make it unique.  Once the string has been stored in the file,
# a message is sent to all clients listening, letting them know
# that the file is available.
# ----------------------------------------------------------------------
proc Rappture::filexfer::spool {string {filename "output.txt"}} {
    global env
    variable enabled
    variable clients
    variable access

    if {$enabled} {
        set dir ~/data/sessions/$env(SESSION)
        if {[file exists [file join $dir $filename]]} {
            #
            # Find a similar file name that doesn't conflict
            # with an existing file:  e.g., output2.txt
            #
            set root [file rootname $filename]
            set ext [file extension $filename]
            set counter 2
            while {1} {
                set filename "$root$counter$ext"
                if {![file exists [file join $dir $filename]]} {
                    break
                }
                incr counter
            }
        }

        set fid [open [file join $dir $filename] w]
        puts $fid $string
        close $fid

        set cid [lindex $clients(order) 0]
        if {$cid == ""} {
            error "no clients"
        }

        set access($filename) [bakeCookie]
        puts $cid "url /spool/$env(SESSION)/$filename?access=$access($filename)"
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::accept <clientId> <address> <port>
#
# Invoked automatically whenever a client tries to connect to this
# server.  Validates the client's incoming <address> and sets up
# callbacks to handle further communication.
# ----------------------------------------------------------------------
proc Rappture::filexfer::accept {cid addr port} {
    variable restrictClientAddress

    #
    # If the client comes from anywhere but the restricted host,
    # then deny the connection.  We should be getting connections
    # only from within the firewall around our own system.
    #
    if {"" != $restrictClientAddress
          && ![string equal $addr $restrictClientAddress]} {
        close $cid
    } else {
        fileevent $cid readable [list Rappture::filexfer::handler $cid]
        #
        # Use auto cr/lf translation for input, but always use
        # binary mode for output.  Otherwise, we'll put out a
        # particular byte count for the body of a response, and
        # it will be wrong after Tcl transforms cr/lf.  Also, some
        # of our data is binary, and it has to be left alone.
        #
        fconfigure $cid -buffering line -translation {auto binary}
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::handler <clientId>
#
# Invoked automatically whenever a message comes in from a client
# to handle the message.
# ----------------------------------------------------------------------
proc Rappture::filexfer::handler {cid} {
    variable buffer

    if {[gets $cid line] < 0} {
        # eof from client -- clean up
        cleanup $cid
    } else {
        #
        # Is the first line of the request?  Then make sure
        # that it's properly formed.
        #
        if {![info exists buffer($cid)]
               && [regexp {^ *[A-Z]+ +[^ ]+ +HTTP/1\.[01]$} $line]} {
            set buffer($cid) $line
            return   ;# wait for more lines to dribble in...
        } elseif {[info exists buffer($cid)]} {
            set line [string trim $line]
            if {"" != $line} {
                append buffer($cid) "\n" $line
                return
            }
            # blank line -- process below...
        } elseif {[regexp { +RAPPTURE$} $line]} {
            set buffer($cid) $line
            # special Rappture request -- process below...
        } else {
            response $cid error -message "Your browser sent a request that this server could not understand.<P>Malformed request: $line"
            cleanup $cid
            return
        }

        #
        # If a buffer already exists, then we're adding on
        # to it.  Look for optional header information.  Don't
        # parse it now--just add it to the buffer.  When we see
        # a blank line, we process the request all at once.
        #
        set errmsg ""
        set lines [split $buffer($cid) \n]
        unset buffer($cid)
        set headers(Connection) close

        # extract the TYPE and URL from the request line
        set line [lindex $lines 0]
        set lines [lrange $lines 1 end]
        if {![regexp {^ *([A-Z]+) +([^ ]+) +(HTTP/1\.[01])$} $line \
              match type url protocol]
            && ![regexp { +(RAPPTURE)$} $line match protocol]} {
            set errmsg "Malformed request: $line"
        }

        if {[string match HTTP/* $protocol]} {
            #
            # HANDLE HTTP/1.x REQUESTS...
            #
            while {"" == $errmsg && [llength $lines] > 0} {
                # extract the "Header: value" lines
                set line [lindex $lines 0]
                set lines [lrange $lines 1 end]

                if {[regexp {^ *([-a-zA-Z0-9_]+): *(.*)} $line \
                      match key val]} {
                    set headers($key) $val
                } else {
                    set errmsg [format "Request header field is missing colon separator.<P>\n<PRE>\n%s</PRE>" $line]
                }
            }

            if {"" != $errmsg} {
                # errors in the header
                response $cid header -status "400 Bad Request" \
                    -connection $headers(Connection)
                response $cid error -message "Your browser sent a request that this server could not understand.<P>$errmsg"
            } else {
                # process the request...
                switch -- $type {
                    GET {
                        request_GET $cid $url headers
                    }
                    default {
                        response $cid header \
                            -status "400 Bad Request" \
                            -connection $headers(Connection)
                        response $cid error -message "Your browser sent a request that this server could not understand.<P>Invalid request type <b>$type</b>"
                    }
                }
            }
            if {$headers(Connection) == "close"} {
                cleanup $cid
            }
        } elseif {$protocol == "RAPPTURE"} {
            #
            # HANDLE SPECIAL RAPPTURE REQUESTS...
            #
            if {[regexp {^ *(REGISTER) +([^ ]+) +([^ ]+) +([^ ]+) +RAPPTURE$} \
                  $line match type user addr cookie]} {
                request_REGISTER $cid $user $addr $cookie
            } elseif {[regexp {^ *UNREGISTER +RAPPTURE$} $line]} {
                request_UNREGISTER $cid
            } elseif {[regexp {^ *ACTIVATE +RAPPTURE$} $line]} {
                request_ACTIVATE $cid
            } elseif {[regexp {^ *DEACTIVATE +RAPPTURE$} $line]} {
                request_DEACTIVATE $cid
            } else {
                response $cid header \
                    -status "400 Bad Request" \
                    -connection $headers(Connection)
                response $cid error -message "Your browser sent a request that this server could not understand.<P>Invalid request type <b>$type</b>"
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::request_GET <clientId> <url> <headerVar>
#
# Used internally to handle GET requests on this server.  Looks for
# the requested <url> and sends it back to <clientId> according to
# the headers in the <headerVar> array in the calling scope.
# ----------------------------------------------------------------------
proc Rappture::filexfer::request_GET {cid url headerVar} {
    global env
    variable access
    upvar $headerVar headers

    #
    # Look for any ?foo=1&bar=2 data embedded in the URL...
    #
    if {[regexp -indices {\?[a-zA-Z0-9_]+\=} $url match]} {
        foreach {s0 s1} $match break
        set args [string range $url [expr {$s0+1}] end]
        set url [string range $url 0 [expr {$s0-1}]]

        foreach part [split $args &] {
            if {[llength [split $part =]] == 2} {
                foreach {key val} [split $part =] break
                set post($key) $val
            }
        }
    }

    #
    # Interpret the URL and fulfill the request...
    #
    if {$url == "/debug" && [info exists env(FILEXFER_DEBUG)]} {
        variable port
        variable cookie
        #
        # DEBUG MODE:  Put out a web page containing the applet
        #   and parameters needed to drive this.  Allow only
        #   if the FILEXFER_DEBUG environment variable is set.
        #
        response $cid header \
            -status "200 OK" \
            -connection $headers(Connection)
        set s [clock seconds]
        set date [clock format $s -format {%a, %d %b %Y %H:%M:%S %Z}]
        puts $cid "Last-Modified: $date"

        set user "???"
        foreach var {USER USERNAME LOGNAME} {
            if {[info exists env($var)]} {
                set user $env($var)
                break
            }
        }

        response $cid body -type text/html -string [format {<html>
<head><title>Rappture::filexfer Debug Page</title></head>
<body BGCOLOR=White>
This page contains the same Java applet that the nanoHUB includes
on each Rappture tool page.  The applet connects back to the
Rappture application and listens for file transfer requests
coming from the user.
<p>
<applet CODE="filexfer.class" ARCHIVE="filexfer.jar" width=300 height=200>
<param name="port" value="%s">
<param name="user" value="%s">
<param name="cookie" value="%s">
</applet>
</body>
</html>
} $port $user $cookie]
    } elseif {[regexp {^/?spool\/(.+)$} $url match tail]} {
        #
        # Send back a spooled file...
        #
        set file [file join ~/data/sessions $tail]
        set fname [file tail $file]

        if {![info exists access($fname)]} {
            response $cid header -status "404 Not Found"
            response $cid error -status "404 Not Found" -message "The requested file <b>$fname</b> is missing.  It may have already been downloaded to your desktop.  You cannot refresh it from the same URL, for security reasons.<p>Instead, download the file again by going back to the original application."
        } elseif {![info exists post(access)]
              || ![string equal $post(access) $access($fname)]} {
            response $cid header -status "401 Unauthorized"
            response $cid error -status "401 Unauthorized" -message "You do not have the proper credentials to access file $fname."
        } else {
            response $cid file -path $file -connection $headers(Connection)
            file delete -force $file
            unset access($fname)
        }
    } elseif {[regexp {^/?[a-zA-Z0-9_]+\.[a-zA-Z]+$} $url match]} {
        #
        # Send back an applet file...
        #
        set url [string trimleft $url /]
        set file [file join $Rappture::installdir filexfer $url]
        response $cid file -path $file -connection $headers(Connection)
    } else {
        #
        # BAD FILE REQUEST:
        #   The user is trying to ask for a file outside of
        #   the normal filexfer installation.  Treat it the
        #   same as file not found.
        response $cid header \
            -status "404 Not Found" \
            -connection $headers(Connection)
        response $cid error -status "404 Not Found" -message "The requested URL $url was not found on this server."
    }
}

# ----------------------------------------------------------------------
# USAGE: request_REGISTER <clientId> <user> <address> <cookie>
#
# Used internally to handle REGISTER requests on this server.  A client
# sends REGISTER requests when it wants to be notified of file transfer
# operations.  The <cookie> must match the one for this server, so
# we know we can trust the client.
# ----------------------------------------------------------------------
proc Rappture::filexfer::request_REGISTER {cid user addr clientCookie} {
    variable clients
    variable cookie

    if {![string equal $cookie $clientCookie]} {
        response $cid header -status "401 Unauthorized"
        response $cid error -status "401 Unauthorized" -message "Credentials are not recognized."
    } else {
        # add this client to the known listeners
        set clients($cid) 0
    }
}

# ----------------------------------------------------------------------
# USAGE: request_UNREGISTER <clientId>
#
# Used internally to handle UNREGISTER requests on this server.
# A client sends this request when it is being destroyed, to let
# the server know that it no longer needs to handle this client.
# ----------------------------------------------------------------------
proc Rappture::filexfer::request_UNREGISTER {cid} {
    variable clients

    set i [lsearch -exact $cid $clients(order)]
    if {$i >= 0} {
        set clients(order) [lreplace $clients(order) $i $i]
    }
    catch {unset clients($cid)}
}

# ----------------------------------------------------------------------
# USAGE: request_ACTIVATE <clientId>
#
# Used internally to handle ACTIVATE requests on this server.  A client
# must first REGISTER with its cookie for authorization.  Then, as
# its thread starts, it sends an ACTIVATE request, letting us know
# that the client is ready to receive notifications.
# ----------------------------------------------------------------------
proc Rappture::filexfer::request_ACTIVATE {cid} {
    variable clients

    #
    # Activate only if the client has already registered
    # properly and is on our known list.
    #
    if {[info exists clients($cid)]} {
        set clients($cid) 1

        # move the most recently activated connection to the front
        set i [lsearch -exact $cid $clients(order)]
        if {$i >= 0} {
            set clients(order) [lreplace $clients(order) $i $i]
        }
        set clients(order) [linsert $clients(order) 0 $cid]
    }
}

# ----------------------------------------------------------------------
# USAGE: request_DEACTIVATE <clientId>
#
# Used internally to handle DEACTIVATE requests on this server.  A client
# must first REGISTER with its cookie for authorization.  Then, as
# its thread starts, it sends an ACTIVATE request.  When its thread
# stops (because the applet is swapped out of the web page), the
# client sends a DEACTIVATE request, and we stop sending messages to
# that client.
# ----------------------------------------------------------------------
proc Rappture::filexfer::request_DEACTIVATE {cid} {
    variable clients

    #
    # Deactivate only if the client has already registered
    # properly and is on our known list.
    #
    if {[info exists clients($cid)]} {
        set clients($cid) 0

        # remove this from the list of activated connections
        set i [lsearch -exact $cid $clients(order)]
        if {$i >= 0} {
            set clients(order) [lreplace $clients(order) $i $i]
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::cleanup <clientId>
#
# Used internally to close and clean up a client connection.
# Clears any data associated with the client.
# ----------------------------------------------------------------------
proc Rappture::filexfer::cleanup {cid} {
    variable clients
    variable buffer

    catch {close $cid}

    if {[info exists clients($cid)]} {
        unset clients($cid)
    }
    set i [lsearch -exact $clients(order) $cid]
    if {$i >= 0} {
        set clients(order) [lreplace $clients(order) $i $i]
    }

    if {[info exists buffer($cid)] && "" != $buffer($cid)} {
        unset buffer($cid)
    }
}

# ----------------------------------------------------------------------
# USAGE: response <channel> header -status <s> -connection <c>
# USAGE: response <channel> body -string <s> -type <t>
# USAGE: response <channel> error -message <m>
# USAGE: response <channel> file -path <f>
#
# Used internally to generate responses to the client.  Returns a
# string representing the requested response.
# ----------------------------------------------------------------------
proc Rappture::filexfer::response {cid what args} {
    variable mime2ext
    variable ext2mime
    variable mime2type

    switch -- $what {
        header {
            Rappture::getopts args params {
                value -status ""
                value -connection close
            }
            set s [clock seconds]
            set date [clock format $s -format {%a, %d %b %Y %H:%M:%S %Z}]
            puts $cid [format "HTTP/1.1 %s
Date: %s
Server: Rappture
Connection: %s" $params(-status) $date $params(-connection)]
        }

        body {
            Rappture::getopts args params {
                value -string ""
                value -type "auto"
            }
            if {$params(-type) == "auto"} {
                if {[isbinary $params(-string)]} {
                    set params(-type) "application/octet-stream"
                } else {
                    set params(-type) "text/plain"
                }
            }
            puts $cid [format "Content-type: %s\nContent-length: %d\n" \
                $params(-type) [string length $params(-string)]]

            if {$mime2type($params(-type)) == "binary"} {
                # binary data -- send data as raw bytes
                set olde [fconfigure $cid -encoding]
                fconfigure $cid -buffering none -encoding binary
                puts -nonewline $cid $params(-string)
                flush $cid
                fconfigure $cid -buffering line -encoding $olde
            } else {
                # ascii data -- send normally
                puts $cid $params(-string)
            }
        }

        error {
            Rappture::getopts args params {
                value -status "400 Bad Request"
                value -message ""
            }
            set heading [lrange $params(-status) 1 end]
            set html [format "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">
<HTML><HEAD>
<TITLE>%s</TITLE>
</HEAD><BODY>
<H1>%s</H1>
%s
</BODY></HTML>" $params(-status) $heading $params(-message)]
            response $cid body -type text/html -string $html
        }

        file {
            Rappture::getopts args params {
                value -path ""
                value -connection close
            }
            if {![file exists $params(-path)]} {
                #
                # FILE NOT FOUND:
                #   The user is requesting some file that is not part of
                #   the standard filexfer installation.
                #
                response $cid header \
                    -status "404 Not Found" \
                    -connection $params(-connection)

                response $cid error -status "404 Not Found" -message "The requested file $params(-path) was not found on this server."
            } elseif {[catch {
                    set fid [open $params(-path) r]
                    set data [read $fid]
                    close $fid
                } result]} {

                response $cid error -status "500 Internal Server Error" -message "The requested file $params(-path) is not installed properly on this server."
                response $cid header \
                    -status "500 Internal Server Error" \
                    -connection $params(-connection)
            } else {
                #
                # READ AND RETURN THE FILE
                #
                set ext [file extension $params(-path)]
                if {[info exists ext2mime($ext)]} {
                    set mtype $ext2mime($ext)
                } else {
                    if {[isbinary $data]} {
                        set mtype application/octet-stream
                    } else {
                        set mtype text/plain
                    }
                }

                if {$mime2type($mtype) == "binary"} {
                    # if this is binary data, read it again and get pure bytes
                    catch {
                        set fid [open $params(-path) r]
                        fconfigure $fid -translation binary -encoding binary
                        set data [read $fid]
                        close $fid
                    } result
                }
                response $cid header \
                    -status "200 OK" \
                    -connection $params(-connection)
                set s [file mtime $params(-path)]
                set date [clock format $s -format {%a, %d %b %Y %H:%M:%S %Z}]
                puts $cid "Last-Modified: $date"

                response $cid body -type $mtype -string $data
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: isbinary <string>
#
# Used internally to see if the given <string> has binary data.
# If so, then it must be treated differently.  Normal translation
# of carriage returns and line feeds must be suppressed.
# ----------------------------------------------------------------------
proc Rappture::filexfer::isbinary {string} {
    # look for binary characters, but avoid things like \t \n etc.
    return [regexp {[\000-\006\016-\037\177-\400]} $string]
}

# ----------------------------------------------------------------------
# USAGE: bakeCookie
#
# Used internally to create a one-time use cookie, passed to clients
# to secure file transfer.  Only clients should know the cookie, so
# only clients will have access to files.
# ----------------------------------------------------------------------
proc Rappture::filexfer::bakeCookie {} {
    variable cookieChars

    set cmax [expr {[llength $cookieChars]-1}]
    set cookie ""
    while {[string length $cookie] < 40} {
        set rindex [expr {round(rand()*$cmax)}]
        append cookie [lindex $cookieChars $rindex]
    }
    return $cookie
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::option_port <port>
#
# Called when the "filexfer_port" directive is encountered while
# parsing the "resources" file.  Assigns the port that the filexfer
# server should be listening to.
# ----------------------------------------------------------------------
proc Rappture::filexfer::option_port {newport} {
    variable port
    set port $newport
}

# ----------------------------------------------------------------------
# USAGE: Rappture::filexfer::option_cookie <cookie>
#
# Called when the "filexfer_cookie" directive is encountered while
# parsing the "resources" file.  Assigns the port that the filexfer
# server should be listening to.
# ----------------------------------------------------------------------
proc Rappture::filexfer::option_cookie {newcookie} {
    variable cookie
    set cookie $newcookie
}
