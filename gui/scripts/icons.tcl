# -*- mode: tcl; indent-tabs-mode: nil -*-
# ----------------------------------------------------------------------
#  COMPONENT: icons - utility for loading icons from a library
#
#  This utility makes it easy to load GIF and XBM files installed
#  in a library in the final installation.  It is used throughout
#  the Rappture GUI, whenever an icon is needed.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================

package require BLT

namespace eval Rappture::icon {
    variable iconpath [list [file join $RapptureGUI::library scripts images]]
    variable icons
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::searchpath ?<dirname> <dirname>...?
#
# Adds one or more directories onto the icon path searched when
# locating icons in Rappture::icon.  You can do the same thing by
# lappend'ing onto the "iconpath" variable, but this call avoids
# duplicates and makes it easier
# ----------------------------------------------------------------------
proc Rappture::icon::searchpath {args} {
    variable iconpath
    foreach dir $args {
        if {[file isdirectory $dir]} {
            if {[lsearch $iconpath $dir] < 0} {
                lappend iconpath $dir
            }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon <name>
#
# Searches for an icon called <name> on all of the directories in
# the search path set up by RapptureGUI::iconpath.
# ----------------------------------------------------------------------
proc Rappture::icon {name} {
    variable ::Rappture::icon::iconpath
    variable ::Rappture::icon::icons

    #
    # Already loaded? then return it directly
    #
    if {[info exists icons($name)]} {
        return $icons($name)
    }

    #
    # Search for the icon along the iconpath search path
    #
    set file ""
    foreach dir $iconpath {
        set path [file join $dir $name.*]
        set file [lindex [glob -nocomplain $path] 0]
        if {"" != $file} {
            break
        }
    }

    set imh ""
    if {"" != $file} {
        switch -- [file extension $file] {
            .gif - .jpg - .png {
                set imh [image create photo -file $file]
            }
            .xbm {
                set fid [open $file r]
                set data [read $fid]
                close $fid
                set imh bitmap-$name
                blt::bitmap define $imh $data
            }
        }
    }
    if {"" != $imh} {
        set icons($name) $imh
    }
    return $imh
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::data <image> ?<format>?
#
# Returns the bytes the represent an <image> in the requested
# <format>, which can get "gif" or "jpeg".
# ----------------------------------------------------------------------
proc Rappture::icon::data {image {format "gif"}} {
    if {[catch {$image data -format $format} result]} {
        if {"too many colors" == $result && "" != [auto_execok djpeg]} {
            #
            # HACK ALERT!  We should use "blt::winop quantize" to
            #   reduce the number of colors for GIF format.  But
            #   it has a bug right now, so we work around it like this.
            #
            set tmpfile /tmp/image[pid].dat
            $image write $tmpfile -format "jpeg -quality 100"
            set fid [open "| djpeg -gif -quantize 250 $tmpfile" r]
            fconfigure $fid -encoding binary -translation binary
            set result [read $fid]
            close $fid
            file delete -force $tmpfile
        } else {
            set result ""
        }
    }
    return $result
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_animate <delay> ?<image> <image>...?
#
# Takes a series if <images> and composited them into a single,
# animated GIF image, with the <delay> in milliseconds between
# frames.  Returns binary data in GIF89a format.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_animate {delay args} {
    if {[llength $args] < 1} {
        error "must have at least one image for animation"
    }
    set delay [expr {round($delay*0.01)}] ;# convert to 1/100s of second

    set imh [lindex $args 0]
    set bytes [data $imh gif]
    if {[string length $bytes] == 0} {
        return ""  ;# can't query image data -- bail out!
    }
    gif_parse $bytes first

    set final "GIF89a"
    gif_put_short final $first(screen-w)
    gif_put_short final $first(screen-h)
    gif_put_byte final $first(screen-packed)
    gif_put_byte final $first(screen-bg)
    gif_put_byte final $first(screen-aspect)
    gif_put_block final $first(colors)

    gif_put_byte final 0x21  ;# looping block
    gif_put_byte final 0xFF
    gif_put_byte final 11
    gif_put_block final "NETSCAPE2.0"
    gif_put_byte final 3     ;# 3 bytes in this block
    gif_put_byte final 1     ;# turn looping on
    gif_put_short final 0    ;# number of loops (forever)
    gif_put_byte final 0     ;# block terminator

    gif_put_byte final 0x21  ;# graphic control block
    gif_put_byte final 0xF9
    gif_put_byte final 4     ;# 4 bytes in this block
    gif_put_byte final 0     ;# packed bits
    gif_put_short final [expr {5*$delay}]  ;# delay time *0.01s
    gif_put_byte final 0     ;# transparency index
    gif_put_byte final 0     ;# block terminator

    gif_put_byte final 0x2C  ;# image block
    gif_put_short final $first(im-0-left)
    gif_put_short final $first(im-0-top)
    gif_put_short final $first(im-0-wd)
    gif_put_short final $first(im-0-ht)
    gif_put_byte final $first(im-0-packed)
    if {($first(im-0-packed) & 0x80) != 0} {
        gif_put_block final $first(im-0-colors)
    }
    gif_put_block final $first(im-0-data)

    foreach imh [lrange $args 1 end] {
        catch {unset gif}
        gif_parse [data $imh gif] gif

        gif_put_byte final 0x21     ;# graphic control block
        gif_put_byte final 0xF9
        gif_put_byte final 4        ;# 4 bytes in this block
        gif_put_byte final 0        ;# packed bits
        gif_put_short final $delay  ;# delay time *0.01s
        gif_put_byte final 0        ;# transparency index
        gif_put_byte final 0        ;# block terminator

        gif_put_byte final 0x2C     ;# image block
        gif_put_short final $gif(im-0-left)
        gif_put_short final $gif(im-0-top)
        gif_put_short final $gif(im-0-wd)
        gif_put_short final $gif(im-0-ht)

        if {[string length $gif(im-0-colors)] > 0} {
            gif_put_byte final $gif(im-0-packed)
            gif_put_block final $gif(im-0-colors)
        } else {
            set packed [expr {($gif(im-0-packed) & 0xF8)
                | ($gif(screen-packed) & 0x87)}]
            gif_put_byte final $packed
            gif_put_block final $gif(colors)
        }
        gif_put_block final $gif(im-0-data)
    }

    gif_put_byte final 0x3B  ;# terminate GIF

    return $final
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_parse <gifImageData> <array>
#
# Takes the data from a GIF image, parses it into various components,
# and returns the information in the <array> in the calling scope.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_parse {gifImageData arrayVar} {
    upvar $arrayVar data
    if {[string range $gifImageData 0 2] != "GIF"} {
        error "not GIF data"
    }
    set data(version) [string range $gifImageData 0 5]
    set pos 6

    set data(screen-w) [gif_get_short $gifImageData pos]
    set data(screen-h) [gif_get_short $gifImageData pos]
    set data(screen-packed) [gif_get_byte $gifImageData pos]
    set data(screen-bg) [gif_get_byte $gifImageData pos]
    set data(screen-aspect) [gif_get_byte $gifImageData pos]

    set ctsize [expr {3*(1 << ($data(screen-packed) & 0x07)+1)}]
    set data(colors) [gif_get_block $gifImageData pos $ctsize]

    set n 0
    while {1} {
        set ctrl [gif_get_byte $gifImageData pos]
        switch -- [format "0x%02X" $ctrl] {
          0x21 {
            set ext [gif_get_byte $gifImageData pos]
            switch -- [format "0x%02X" $ext] {
              0xF9 {
                # graphic control
                set bsize [gif_get_byte $gifImageData pos]
                set data(gc-$n-packed) [gif_get_byte $gifImageData pos]
                set data(gc-$n-delay) [gif_get_short $gifImageData pos]
                set data(gc-$n-transp) [gif_get_byte $gifImageData pos]
                set bterm [gif_get_byte $gifImageData pos]
                if {$bterm != 0} { error "bad magic $bterm" }
              }
              0xFE {
                # comment extension -- skip and ignore
                while {1} {
                    set count [gif_get_byte $gifImageData pos]
                    if {$count == 0} {
                        break
                    }
                    incr pos $count
                }
              }
              0xFF {
                set bsize [gif_get_byte $gifImageData pos]
                set data(app-name) [gif_get_block $gifImageData pos $bsize]
                set data(app-bytes) ""
                while {1} {
                    set count [gif_get_byte $gifImageData pos]
                    gif_put_byte data(app-bytes) $count
                    if {$count == 0} {
                        break
                    }
                    gif_put_block data(app-bytes) \
                        [gif_get_block $gifImageData pos $count]
                }
              }
              default {
                error [format "unknown extension code 0x%02X" $ext]
              }
            }
          }
          0x2C {
            # image data
            set data(im-$n-left) [gif_get_short $gifImageData pos]
            set data(im-$n-top) [gif_get_short $gifImageData pos]
            set data(im-$n-wd) [gif_get_short $gifImageData pos]
            set data(im-$n-ht) [gif_get_short $gifImageData pos]
            set data(im-$n-packed) [gif_get_byte $gifImageData pos]
            set data(im-$n-colors) ""
            if {($data(im-$n-packed) & 0x80) != 0} {
                set ctsize [expr {3*(1 << ($data(im-$n-packed) & 0x07)+1)}]
                set data(im-$n-colors) [gif_get_block $gifImageData pos $ctsize]
            }

            set data(im-$n-data) ""
            gif_put_byte data(im-$n-data) \
                [gif_get_byte $gifImageData pos] ;# lwz min code size
            while {1} {
                set count [gif_get_byte $gifImageData pos]
                gif_put_byte data(im-$n-data) $count
                if {$count == 0} {
                    break
                }
                gif_put_block data(im-$n-data) \
                    [gif_get_block $gifImageData pos $count]
            }
            incr n
          }
          0x3B {
            # end of image data
            break
          }
          default {
            error [format "unexpected control byte 0x%02X" $ctrl]
          }
        }
    }
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_get_byte <buffer> <posVar>
#
# Extracts one byte of information from the <buffer> at the index
# specified by <posVar> in the calling scope.  Increments <posVar>
# to move past the byte and returns the byte of information.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_get_byte {buffer posVar} {
    upvar $posVar pos
    set byte [string range $buffer $pos $pos]
    incr pos 1

    binary scan $byte c rval
    if {$rval < 0} {incr rval 256}
    return $rval
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_get_short <buffer> <posVar>
#
# Extracts one short int of information from the <buffer> at the index
# specified by <posVar> in the calling scope.  Increments <posVar>
# to move past the int and returns the information.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_get_short {buffer posVar} {
    upvar $posVar pos
    set bytes [string range $buffer $pos [expr {$pos+1}]]
    incr pos 2

    binary scan $bytes s rval
    if {$rval < 0} {incr rval 65536}
    return $rval
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_get_block <buffer> <posVar> <size>
#
# Extracts <size> bytes of information from the <buffer> at the index
# specified by <posVar> in the calling scope.  Increments <posVar>
# to move past the byte and returns the byte of information.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_get_block {buffer posVar size} {
    upvar $posVar pos
    set bytes [string range $buffer $pos [expr {$pos+$size-1}]]
    incr pos $size
    return $bytes
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_put_byte <buffer> <charVal>
#
# Appends one byte of information onto the <buffer> in the calling
# scope.  The <charVal> is an integer in the range 0-255.  It is
# formated as a single byte and appended onto the buffer.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_put_byte {bufferVar char} {
    upvar $bufferVar buffer
    append buffer [binary format c $char]
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_put_short <buffer> <shortVal>
#
# Appends one byte of information onto the <buffer> in the calling
# scope.  The <shortVal> is an integer in the range 0-65535.  It is
# formated as a 2-byte short integer and appended onto the buffer.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_put_short {bufferVar short} {
    upvar $bufferVar buffer
    append buffer [binary format s $short]
}

# ----------------------------------------------------------------------
# USAGE: Rappture::icon::gif_put_block <buffer> <val>
#
# Appends a string <val> onto the <buffer> in the calling scope.
# ----------------------------------------------------------------------
proc Rappture::icon::gif_put_block {bufferVar val} {
    upvar $bufferVar buffer
    append buffer $val
}
