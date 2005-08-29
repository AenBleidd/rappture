#
# Use this to test the HTTP protocol.  Start this server,
# then treat localhost:9001 as a web site, and watch the
# traffic between the client and the web server at localhost:80
#
set port 80
if {[llength $argv] > 0} {
    set port [lindex $argv 0]
}

set client ""
set server ""

proc accept {cid addr port} {
    global client
    fileevent $cid readable [list client_handler $cid]
    fconfigure $cid -buffering line
    set client $cid
}

proc client_handler {cid} {
    global client server port
    if {[gets $cid line] < 0} {
        close $cid
        set client ""
    } else {
        if {$server == ""} {
            set server [socket localhost $port]
            fileevent $server readable [list server_handler $server]
            fconfigure $server -buffering line
        }
        puts "CLIENT: $line"
        puts $server $line
        flush $server
    }
}

proc server_handler {sid} {
    global client server
    if {[gets $sid line] < 0} {
        close $sid
        set server ""
    } else {
        if {[regexp {[\000-\006\016-\037\177-\400]} $line]} {
            puts "SERVER: --[string length $line] bytes--"
        } else {
            puts "SERVER: $line"
        }
        catch {puts $client $line}
        catch {flush $client}
    }
}

socket -server accept 9001
vwait forever
