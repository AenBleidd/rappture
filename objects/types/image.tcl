# ----------------------------------------------------------------------
#  EDITOR: image attribute values
#
#  Used within the Instant Rappture builder to edit image values.
# ======================================================================
#  AUTHOR:  Michael McLennan, Purdue University
#  Copyright (c) 2004-2012  HUBzero Foundation, LLC
#
#  See the file "license.terms" for information on usage and
#  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# ======================================================================
itcl::class AttrImage {
    constructor {win args} {
        Rappture::getopts args params {
            value -maxsize ""
            value -rescale no
            value -tooltip ""
        }
        set _currview [image create photo]
        set _maxsize $params(-maxsize)
        set _rescale $params(-rescale)

        label $win.image -image $_currview -borderwidth 1 -relief solid
        pack $win.image -side left
        Rappture::Tooltip::for $win.image $params(-tooltip)

        frame $win.btns
        button $win.btns.load -text "Load..." \
            -command [itcl::code $this loadFromFile]
        pack $win.btns.load -side left -padx {8 2}
        button $win.btns.save -text "Save..." \
            -command [itcl::code $this saveToFile]
        pack $win.btns.save -side left -padx 2
        button $win.btns.clear -text "Clear" \
            -command [itcl::code $this clear]
        pack $win.btns.clear -side left -padx 2

        set _win $win
        _assign image ""  ;# redraw with empty image
    }
    destructor {
        image delete $_currview
        if {$_actual ne ""} { image delete $_actual }
    }

    public method load {bytes} {
        _assign data $bytes
    }

    public method check {} {
        # images should be okay -- they get checked when they're loaded
    }

    public method save {var} {
        upvar $var bytes

        if {$_actual eq ""} {
            set bytes ""
        } else {
            if {$_rescale} {
                set bytes [$_currview data -format png]
            } else {
                set bytes [$_actual data -format png]
            }
        }
        return 1
    }

    public method edit {} {
        focus -force $_win.btns.load
    }

    public proc import {xmlobj path} {
        # trivial import -- just return info as-is from XML
        return [$xmlobj get $path]
    }

    public proc export {xmlobj path value} {
        # trivial export -- just save info as-is into XML
        if {[string length $value] > 0} {
            $xmlobj put $path $value
        }
    }

    public method loadFromFile {} {
        set fname [tk_getOpenFile -title "Rappture: Load Image From File" \
            -filetypes {
                {{Image Files}   {.bmp .gif .ico .jpg .jpeg .pcx .png .pgm .ppm .sgi .sun .tga .tiff .xbm .xpm}}
                {{BMP Files}     .bmp}
                {{GIF Files}     .gif}
                {{JPEG Files}    {.jpg .jpeg}}
                {{PNG Files}     .png}
                {{TIFF Files}    .tiff}
                {{All Files}     *}
            }]
        if {$fname != ""} {
            if {[catch {image create photo -file $fname} result]} {
                tk_messageBox -title "Rappture: Error" -message "Image load failed:\n$result"
            } else {
                _assign image $result
            }
        }
    }

    public method saveToFile {} {
        set fname [tk_getSaveFile -title "Rappture: Save Image To File" \
            -defaultextension ".png" -filetypes {
                {{Image Files}   {.bmp .gif .ico .jpg .jpeg .pcx .png .pgm .ppm .sgi .sun .tga .tiff .xbm .xpm}}
                {{BMP Files}     .bmp}
                {{GIF Files}     .gif}
                {{JPEG Files}    {.jpg .jpeg}}
                {{PNG Files}     .png}
                {{TIFF Files}    .tiff}
                {{All Files}     *}
            }]
        if {$fname != ""} {
            set cmds {
                set fid [open $fname w]
                set fmt [string range [file extension $fname] 1 end]
                if {$fmt eq "jpg"} {set fmt "jpeg"}
                if {$fmt eq "pgm"} {set fmt "ppm -grayscale"}
                set data [eval $_actual data -format $fmt]
                if {![regexp {^[a-zA-Z0-9+/=]+(\n[a-zA-Z0-9+/=]+)*$} $data]
                      || [catch {Rappture::encoding::decode -as b64 $data} bytes]} {
                    # oops! not base64 -- use ascii data directly
                    set bytes $data
                } else {
                    # must have decoded binary data from b64
                    fconfigure $fid -encoding binary -translation binary
                }
                puts -nonewline $fid $bytes
                close $fid
            }
            if {[catch $cmds err]} {
                tk_messageBox -title "Rappture: Error" -message "Image save failed:\n$err"
            }
        }
    }

    public method clear {} {
        _assign image ""
    }

    private method _assign {type data} {
        # assign the new image data into the _actual image
        switch -- $type {
            image {
                if {$_actual ne ""} {
                    image delete $_actual
                    set _actual ""
                }
                if {$data ne "" && [catch {image type $data}] == 0} {
                    set _actual $data
                }
            }
            data {
                if {$data eq ""} {
                    # nothing to display -- clear out the image
                    if {$_actual ne ""} {
                        image delete $_actual
                        set _actual ""
                    }
                } else {
                    if {$_actual eq ""} {
                        set _actual [image create photo -data $data]
                    } else {
                        $_actual configure -data $data
                    }
                }
            }
        }

        # redraw the _actual image to _currview based on -maxsize
        if {$_actual eq ""} {
            # generate a diagonal cross-hatch image for "no image"
            if {$_maxsize eq ""} {
                set size 50
            } else {
                set size [expr {2*$_maxsize/3}]
                if {$size < 10} { set size 10 }
            }
            set diag [Rappture::icon diag]
            set dw [image width $diag]
            set dh [image height $diag]
            $_currview configure -width $size -height $size
            for {set i 0} {$i < $size/$dw+1} {incr i} {
                for {set j 0} {$j < $size/$dh+1} {incr j} {
                    set x [expr {$i*$dw}]
                    set y [expr {$j*$dh}]
                    $_currview copy $diag -to $x $y
                }
            }
        } elseif {$_maxsize ne ""} {
            # scale the image to the specified maxsize
            set w $_maxsize
            set h $_maxsize
            set iw [image width $_actual]
            set ih [image height $_actual]
            if {$iw > $ih} {
                set h [expr {round($w/double($iw)*$ih)}]
            } else {
                set w [expr {round($h/double($ih)*$iw)}]
            }
            $_currview configure -width $w -height $h
            $_currview blank
            blt::winop resample $_actual $_currview box
        } else {
            # no maxsize -- display the image as-is
            set iw [image width $_actual]
            set ih [image height $_actual]
            $_currview configure -width $iw -height $ih
            $_currview blank
            $_currview copy $_actual
        }

        # fix the position of the buttons based on the image size
        if {[image width $_currview] > 200} {
            pack $_win.image -side top -anchor w
            pack $_win.btns -side top -anchor w
        } else {
            pack $_win.image -side left
            pack $_win.btns -side left
        }
    }

    private variable _win ""       ;# containing frame
    private variable _maxsize ""   ;# integer => max width/height for image
    private variable _rescale 0    ;# boolean => limit image to maxsize
    private variable _currview ""  ;# tk image for current image view
    private variable _actual ""    ;# tk image for actual image
}
