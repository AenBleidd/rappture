# -*- mode: tcl; indent-tabs-mode: nil -*- 
# ----------------------------------------------------------------------
#  COMPONENT: XAuth - authentication for Twitter/OAuth services
#
#  This library is used for XAuth authenication with HUBzero services.
#  Takes a username/password and obtains a token for other web services
#  calls.
#
#    XAuth::credentials load ~/.xauth
#    set clientToken [XAuth::credentials get nanoHUB.org -token]
#    set clientSecret [XAuth::credentials get nanoHUB.org -secret]
#
#    XAuth::init $site $clientToken $clientSecret -user $username $password
#    XAuth::call $site $method $params
#
#  Check out this awesome description of the whole XAuth process:
#    http://weblog.bluedonkey.org/?p=959
#    https://dev.twitter.com/docs/oauth/xauth
#
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2015  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
package require Itcl
package require http
package require base64
package require sha1
package require tls
http::register https 443 [list ::tls::socket -tls1 0 -ssl2 0 -ssl3 0]

namespace eval XAuth {
    # stores token/secret info from a file containing site data
    variable sites

    # parser for managing sites files
    variable parser [interp create -safe]

    foreach cmd [$parser eval {info commands}] {
        $parser hide $cmd
    }
    $parser alias site ::XAuth::credentials add
    $parser invokehidden proc unknown {args} {
        error "bad command \"$args\": should be sites"
    }
    $parser expose error

    # maps a web services url prefix to client token/secret
    variable clients

    # maps a web services url prefix to an authenticated session token
    variable tokens

    # list of http redirects (so we can detect infinite loops)
    variable redirects ""
}

# used to store values from JSON objects
itcl::class JsonObject {
    constructor {{parent ""}} {
        if {$parent ne ""} {
            if {[catch {$parent is JsonObject} valid] || !$valid} {
                error "bad value \"$parent\": should be JsonObject"
            }
            $parent attach $this
        }
    }

    destructor {
        foreach obj $_children {
            itcl::delete object $obj
        }
    }

    method attach {args} {
        foreach obj $args {
            if {[catch {$obj is JsonObject} valid] || !$valid} {
                error "bad value \"$obj\": should be JsonObject"
            }
            lappend _children $obj
        }
    }

    method assign {args} {
        switch -- [llength $args] {
            1 {
                catch {unset _dict}
                set _value [lindex $args 0]
                set _type "scalar"
                if {[catch {$_value is JsonObject} valid] == 0 && $valid} {
                    attach $_value
                }
            }
            2 {
                set key [lindex $args 0]
                set value [lindex $args 1]
                if {[catch {$value is JsonObject} valid] == 0 && $valid} {
                    attach $value
                }

                if {$key eq "-element"} {
                    catch {unset _dict}
                    lappend _value $value
                    set _type "vector"
                } else {
                    catch {unset _value}
                    set _dict($key) $value
                    set _type "struct"
                }
            }
            default {
                error "wrong # args: should be \"assign value\" or \"assign -element value\" or \"assign key value\""
            }
        }
    }

    method get {{what ""}} {
        switch -- $_type {
            scalar {
                if {$what ne "" && $what ne "-scalar"} {
                    error "type mismatch -- requested scalar but got $_type"
                }
                return $_value
            }
            vector {
                if {$what ne "" && $what ne "-vector"} {
                    error "type mismatch -- requested vector but got $_type"
                }
                return $_value
            }
            struct {
                if {$what eq ""} {
                    return [array names _dict]
                } elseif {[info exists _dict($what)]} {
                    return $_dict($what)
                } else {
                    return ""
                }
            }
            default {
                error "internal error: bad type \"$_type\""
            }
        }
    }

    method type {} {
        return $_type
    }

    protected variable _children ""
    protected variable _type "scalar"
    protected variable _value ""
    protected variable _dict

    # decode JSON -- returns a JsonObject
    proc decode {str {leftoverVar ""}} {
        # look for opening curly brace (7B)
        if {[regexp -indices {^[[:space:]]*\x7B} $str match]} {
            set obj [JsonObject ::#auto]
            set str [substr $str $match -( >]
            while {1} {
                # should set "string":value
                if {[regexp -indices {^[[:space:]]*"(([^\\\"]|\\.)*)"[[:space:]]*:} $str match key]} {
                    set key [substr $str $key | |]
                    set str [substr $str $match -( >]
                    set val [decode $str str]
                    $obj assign $key $val
                    if {[regexp -indices {^[[:space:]]*,} $str match]} {
                        # found comma -- keep going
                        set str [substr $str $match -( >]
                    } elseif {[regexp -indices {^[[:space:]]*\x7D} $str match]} {
                        # found closing curly brace (7D)
                        if {$leftoverVar ne ""} {
                            upvar $leftoverVar rest
                            set rest [substr $str $match -( >]
                        }
                        return $obj
                    } else {
                        error "syntax error -- expected , or \x7D but got \"[string range $str 0 20]...\""
                    }
                } else {
                    error "syntax error -- expected \"string\":value but got \"[string range $str 0 20]...\""
                }
            }
        } elseif {[regexp -indices {^[[:space:]]*\x5B} $str match]} {
            # found opening square bracket (5B) -- start of array...
            set obj [JsonObject ::#auto]
            set str [substr $str $match -( >]

            if {[regexp -indices {^[[:space:]]*\x5D} $str match]} {
                # empty list
                if {$leftoverVar ne ""} {
                    upvar $leftoverVar rest
                    set rest [substr $str $match -( >]
                }
                return $obj
            }
            while {1} {
                # decode the element and add to the array
                set val [decode $str str]
                $obj assign -element $val

                if {[regexp -indices {^[[:space:]]*,} $str match]} {
                    # found comma -- keeping going
                    set str [substr $str $match -( >]
                } elseif {[regexp -indices {^[[:space:]]*\x5D} $str match]} {
                    # found closing square bracket (5D)
                    if {$leftoverVar ne ""} {
                        upvar $leftoverVar rest
                        set rest [substr $str $match -( >]
                    }
                    return $obj
                } else {
                    error "syntax error -- expected , or \x7D but got \"[string range $str 0 20]...\""
                }
            }
        } elseif {[regexp -indices {^[[:space:]]*"(([^\\\"]|\\.)*)"} $str match inner]} {
            # found quoted string value
            set val [substr $str $inner | |]

            # convert backslashes and newlines within string
            regsub -all {\\r\\n} $val "\n" val
            regsub -all {\\n} $val "\n" val
            regsub -all {\\(.)} $val {\1} val

            if {$leftoverVar ne ""} {
                upvar $leftoverVar rest
                set rest [substr $str $match -( >]
            }
            return $val
        } elseif {[regexp -indices {^[[:space:]]*([-+]?[0-9]+(\.[0-9]*)?([eEdE][-+]?[0-9]+)?)([^0-9eEdD.]|$)} $str match inner]} {
            # found number value
            set val [substr $str $inner | |]
            if {$leftoverVar ne ""} {
                upvar $leftoverVar rest
                set rest [substr $str $inner -( >]
            }
            return $val
        } elseif {[regexp -indices {^[[:space:]]*(true|false)} $str match inner]} {
            # found true/false value
            set val [substr $str $inner | |]
            if {$leftoverVar ne ""} {
                upvar $leftoverVar rest
                set rest [substr $str $match -( >]
            }
            return $val
        } elseif {[regexp -indices {^[[:space:]]*null} $str match]} {
            if {$leftoverVar ne ""} {
                upvar $leftoverVar rest
                set rest [substr $str $match -( >]
            }
            return ""
        } else {
            error "syntax error at: [string range $str 0 20]..."
        }
    }

    # substr -- given a string an indices from regexp, return a substring
    #  | | ...... return exactly from one index to another
    #  (- -) .... return stuff just inside the two indices
    #  -( > ..... return everything after the last index
    #  0 )- ..... return everything until before the first index
    proc substr {str match lim0 lim1} {
        foreach {m0 m1} $match break
        switch -- $lim0 {
            0  { set s0 0 }
            (- { set s0 [expr {$m0+1}] }
            -( { set s0 [expr {$m1+1}] }
            -) { set s0 [expr {$m1-1}] }
            |  { set s0 $m0 }
            default { error "don't understand limit \"$lim0\"" }
        }
        switch -- $lim1 {
            >  { set s1 end }
            -) { set s1 [expr {$m1-1}] }
            )- { set s0 [expr {$m0-1}] }
            (- { set s1 [expr {$m0+1}] }
            -( { set s1 [expr {$m1+1}] }
            |  { set s1 $m1 }
            default { error "don't understand limit \"$lim1\"" }
        }
        return [string range $str $s0 $s1]
    }
}

# ----------------------------------------------------------------------
# USAGE: XAuth::init <site> <clientToken> <clientSecret> -user <u> <p>
# USAGE: XAuth::init <site> <clientToken> <clientSecret> -session <n> <t>
#
# Should be called to initialize this library.  Can be initialized
# one of two ways:
#
#   -user <u> <p> ...... sends username <u> and password <p>
#   -session <n> <t> ... sends tool session number <n> and token <t>
#
# Sends the credentials to the <site> for authentication.  The client
# token and secret are registered to identify the application.
# If successful, this call stores an authenticated session token in
# the tokens array for the <site> URL.  Subsequent calls to XAuth::call
# use this token to identify the user.
# ----------------------------------------------------------------------
proc XAuth::init {site clientToken clientSecret args} {
    variable clients
    variable tokens

    set option [lindex $args 0]
    switch -- $option {
        -user {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"-user name password\""
            }
            set uname [lindex $args 1]
            set passw [lindex $args 2]
        }
        -session {
            if {[llength $args] != 3} {
                error "wrong # args: should be \"-session number token\""
            }
            set snum [lindex $args 1]
            set stok [lindex $args 2]

            # store session info for later -- no need for oauth stuff
            set tokens($site) [list session $snum $stok]
            set clients($site) [list $clientToken $clientSecret]
            return
        }
        default {
            if {[llength $args] != 2} {
                error "wrong # args: should be \"XAuth::init site token secret ?-option? arg arg\""
            }
            set uname [lindex $args 0]
            set passw [lindex $args 1]
        }
    }

    if {![regexp {^https://} $site]} {
        error "bad site URL \"$site\": should be https://..."
    }
    set site [string trimright $site /]

    if {![regexp {^[0-9a-zA-Z]+$} $clientToken]} {
        error "bad client token \"$clientToken\": should be alphanumeric"
    }

    set url $site/oauth/access_token
    set nonce [XAuth::nonce]
    set tstamp [clock seconds]


    # Twitter has this awesome test page:
    # https://dev.twitter.com/docs/oauth/xauth
    #
    # Use these values...
    #   set url https://api.twitter.com/oauth/access_token
    #   set clientToken JvyS7DO2qd6NNTsXJ4E7zA
    #   set clientSecret 9z6157pUbOBqtbm0A0q4r29Y2EYzIHlUwbF4Cl9c
    #   set nonce 6AN2dKRzxyGhmIXUKSmp1JcB4pckM8rD3frKMTmVAo
    #   set tstamp 1284565601
    #   set passw twitter-xauth
    #   set uname oauth_test_exec
    #
    # and the signature should be: 1L1oXQmawZAkQ47FHLwcOV%2Bkjwc%3D

    # BE CAREFUL -- put these parameters in exactly this order
    set query [http::formatQuery \
        oauth_consumer_key $clientToken \
        oauth_nonce $nonce \
        oauth_signature_method "HMAC-SHA1" \
        oauth_timestamp $tstamp \
        oauth_version "1.0" \
        x_auth_mode "client_auth" \
        x_auth_password $passw \
        x_auth_username $uname \
    ]

    set base "POST&[urlencode $url]&[urlencode $query]"
    set key "$clientSecret&"
    set sig [urlencode [base64::encode [sha1::hmac -bin -key $key $base]]]

    # build the header and send the request
    set auth [format "OAuth oauth_consumer_key=\"%s\", oauth_nonce=\"%s\", oauth_signature_method=\"HMAC-SHA1\", oauth_signature=\"%s\", oauth_timestamp=\"%s\", oauth_version=\"1.0\"" $clientToken $nonce $sig $tstamp]

    set result [XAuth::fetch $url -headers [list Authorization $auth] -query $query]

    # pick apart the result and extra: oauth_token, oauth_token_secret
    foreach param [split $result &] {
        if {[regexp {^(oauth[^=]+)=(.+)} $param match name val]} {
            set got($name) $val
        }
    }
    if {![info exists got(oauth_token)] || ![info exists got(oauth_token_secret)]} {
        error "authentication failed: $result"
    }

    # success! store the session token for later
    set tokens($site) [list oauth $got(oauth_token) $got(oauth_token_secret)]
    set clients($site) [list $clientToken $clientSecret]
}

# ----------------------------------------------------------------------
# USAGE: XAuth::call <site> <method> ?<params>?
#
# Called after XAuth::init for each web service request.  Calls the
# given <site>/<method> with the specified <params>.  Returns the
# xml result string.
# ----------------------------------------------------------------------
proc XAuth::call {site method {params ""}} {
    variable clients
    variable tokens

    if {![regexp {^https://} $site]} {
        error "bad site URL \"$site\": should be https://..."
    }
    set site [string trimright $site /]
    set method [string trimleft $method /]

    if {![info exists tokens($site)]} {
        error "must call XAuth::init for $site first to authenticate"
    }
    foreach {clientToken clientSecret} $clients($site) break
    foreach {scheme userToken userSecret} $tokens($site) break

    set url $site/$method

    switch -- $scheme {
        oauth {
            set nonce [XAuth::nonce]
            set tstamp [clock seconds]

            # BE CAREFUL -- put all query parameters in alphabetical order
            array set qparams [list \
                oauth_consumer_key $clientToken \
                oauth_nonce $nonce \
                oauth_signature_method "HMAC-SHA1" \
                oauth_timestamp $tstamp \
                oauth_token $userToken \
                oauth_version "1.0" \
                x_auth_mode "client_auth" \
            ]
            array set qparams $params

            set query ""
            foreach key [lsort [array names qparams]] {
                lappend query $key $qparams($key)
            }
            set query [eval http::formatQuery $query]

            set base "POST&[urlencode $url]&[urlencode $query]"
            set key "$clientSecret&$userSecret"
            set sig [urlencode [base64::encode [sha1::hmac -bin -key $key $base]]]

            # build the header and send the request
            set auth [format "OAuth oauth_consumer_key=\"%s\", oauth_token=\"%s\", oauth_nonce=\"%s\", oauth_signature_method=\"HMAC-SHA1\", oauth_signature=\"%s\", oauth_timestamp=\"%s\", oauth_version=\"1.0\"" $clientToken $userToken $nonce $sig $tstamp]
            set hdr [list Authorization $auth]
        }
        session {
            set hdr [list sessionnum $userToken sessiontoken $userSecret]
            set query ""
            foreach {key val} $params {
                lappend query $key $val
            }
            set query [eval http::formatQuery $query]
        }
        default {
            error "internal error -- don't understand call scheme \"$scheme\""
        }
    }
    return [XAuth::fetch $url -headers $hdr -query $query]
}

# ----------------------------------------------------------------------
# USAGE: XAuth::fetch <url> ?-headers <keyvalList>? ?-query <str>?
#
# Sends a GET/POST request off to the specified <url>.  If the -query
# string is specified, then this is assumed to be an encoded list of
# parameters and the operation is POST.  Tries a few times in case the
# web site is busy.
# ----------------------------------------------------------------------
proc XAuth::fetch {url args} {
    variable redirects

    set hdr ""
    set query ""
    foreach {key val} $args {
        switch -- $key {
            -headers { set hdr $val }
            -query   { set query $val }
            default {
                error "bad option \"$key\": should be -headers or -query"
            }
        }
    }

    # send off the request a few times
    set ntries 5
    while {1} {
        if {[catch {http::geturl $url -headers $hdr -query $query -timeout 30000} token] == 0} {
            break
        }
        if {[incr ntries -1] <= 0} {
            error "web request \"$url\" failed to load: $token"
        }
        after 5000
    }

    # handle the response
    upvar #0 $token state

    # look for errors
    switch -- [http::ncode $token] {
        200 {
            set rval [http::data $token]

	    array set meta $state(meta)
            if {[info exists meta(Transfer-Encoding)]
                  && $meta(Transfer-Encoding) eq "chunked"} {
                set rval [XAuth::unchunk $rval]
            }

            if {[info exists meta(Content-Type)]
                  && $meta(Content-Type) eq "application/json"} {
                set rval [JsonObject::decode $rval]
            }

            http::cleanup $token
            set redirects ""
            return $rval
        }
        301 - 302 - 303 {
            lappend redirects $url
            if {[llength $redirects] > 5} {
                error "web page redirect loop for $url"
            }
	    array set meta $state(meta)
            if {[info exists meta(Location)]} {
                set newurl $meta(Location)
                if {![regexp {^https?://} $newurl] && [regexp -nocase -indices {^https?://[^/]+} $url match]} {
                    if {[string index $newurl 0] != "/"} {
                        set newurl "/$newurl"
                    }
                    foreach {s0 s1} $match break
                    set newurl "[string range $url $s0 $s1]$newurl"
                }
                return [fetch $newurl -headers $hdr -query $query]
            }
            return ""
        }
        default {
            set status [http::code $token]
            http::cleanup $token
            set redirects ""
            error "web request \"$url\" failed to load: $status"
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: XAuth::urlencode <str>
#
# Encodes a string according to standard HTTP encoding conventions.
# Punctuation characters are converted to their %XX equivalent.
# Returns a properly encoded string.
# ----------------------------------------------------------------------
proc XAuth::urlencode {str} {
    set str [http::formatQuery $str]
    regsub -all {%[a-fA-F0-9][a-fA-F0-9]} $str {[string toupper \0]} str
    return [subst $str]
}

# ----------------------------------------------------------------------
# USAGE: XAuth::nonce
#
# Random nonce (number used once) for the OAuth protocol.  Each nonce
# should be unique when interpreted in conjunction with the timestamp.
# Any large, random number should work here.
# ----------------------------------------------------------------------
proc XAuth::nonce {} {
    set nonce [expr {round(rand()*1e8)}][clock clicks]
    return [sha1::sha1 $nonce]
}

# ----------------------------------------------------------------------
# USAGE: XAuth::unchunk <string>
#
# Used internally to decode a <string> from a web server that has been
# transferred with "chunk" encoding.  In this case, the string contains
# a hexadecimal size, a newline, a chunk of text, another hexadecimal
# size, a newline, another chunk of text, etc.  Returns a clean string
# with all of the hex values removed.
# ----------------------------------------------------------------------
proc XAuth::unchunk {str} {
    set rval ""
    while {[string length $str] > 0} {
        # get the hex string for the length
        set nlpos [string first "\n" $str]
        if {$nlpos < 0} {
            append rval $str
            break
        }
        set hex [string range $str 0 [expr {$nlpos-1}]]

        if {[scan $hex "%x" len] == 1} {
            # get the next chunk with that length
            set from [expr {$nlpos+1}]
            set to [expr {$from+$len}]
            append rval [string range $str $from [expr {$to-1}]]
            set nl [string index $str $to]

            if {$nl eq "\r"} {
                incr to
                set nl [string index $str $to]
            }
            if {$nl ne "\n" && $nl ne ""} {
                error "garbled text in chunk-encoded string -- missing newline"
            }
            set str [string range $str [expr {$to+1}] end]
        } else {
            error "garbled text in chunk-encoded string -- missing hex value"
        }
    }
    return $rval
}

# ----------------------------------------------------------------------
# USAGE: XAuth::credentials load ?<fileName>?
# USAGE: XAuth::credentials get <site> ?<what>?
#
# Clients use this to load information about the client token/secret
# from a file and feed it along to XAuth::init.  The "load" operation
# loads information from a file in the user's home directory.  If not
# specified, the name "~/.xauth" is assumed.  This file contains a
# series of lines as follows:
#
#   site nanoHUB.org -token abLJdjfks -secret kd18293ksjshdkdjejd
#   site HUBzero.org -token adckdsjeL -secret dkejdklsje1wlsjd2je
#   ...
#
# The "get" call returns information for the specified <site> name.
# The optional <what> parameter can be used to request -token or
# -secret.  Otherwise, it returns a list "-token xxx -secret yyy"
# ----------------------------------------------------------------------
proc XAuth::credentials {option args} {
    variable sites
    variable parser

    switch -- $option {
        load {
            if {[llength $args] == 1} {
                set fname [lindex $args 0]
            } elseif {[llength $args] == 0} {
                if {[file exists ~/.xauth]} {
                    set fname "~/.xauth"
                } else {
                    set fname ""
                }
            } else {
                error "wrong # args: should be \"credentials load ?file?\""
            }

            if {$fname ne ""} {
                if {![file readable $fname]} {
                    error "file \"$fname\" not found"
                }
                set fid [open $fname r]
                set info [read $fid]
                close $fid

                if {[catch {$parser eval $info} result]} {
                    error "error in sites file \"$fname\": $result"
                }
            }
        }
        add {
            set name [string tolower [lindex $args 0]]
            foreach {key val} [lrange $args 1 end] {
                if {$key ne "-token" && $key ne "-secret"} {
                    error "bad option \"$key\": should be -token or -secret"
                }
            }
            set sites($name) [lrange $args 1 end]
        }
        get {
            set name [string tolower [lindex $args 0]]
            set what [lindex $args 1]
            if {$what ne "" && $what ne "-token" && $what ne "-secret"} {
                error "bad value \"$what\": should be -token or -secret"
            }

            if {[info exists sites($name)]} {
                if {$what eq ""} {
                    return $sites($name)
                }
                array set data $sites($name)
                return $data($what)
            }
            return ""
        }
        default {
            error "bad option \"$option\": should be load or get"
        }
    }
}
