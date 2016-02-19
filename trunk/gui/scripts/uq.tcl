package require Itk
namespace eval Rappture {}

itcl::class Rappture::UQ {

    constructor {varlist num type uqargs} {
        #puts "Rappture::UQ constructor $varlist:$type:$uqargs"
        set _varlist $varlist
        set _type $type
        set _args $uqargs
        set _num $num
    }

    destructor {
        # defined below
    }

    public method run_dialog {win}
    public method num_runs {}
    public method type {} {return $_type}
    public method args {} {return $_args}

    protected method _setWaitVariable {state}
    protected method _adjust_level {win}
    protected method _set_text {win}
    protected method _init_num_pts_array {}
    public variable _type "smolyak"
    private variable _args "2"
    private variable _varlist {}
    private variable _wait_uq 0
    private variable _num_pts {}
    private variable _go ""
    private variable _num 0
}

# ----------------------------------------------------------------------
# Display a dialog for setting UQ options.
# Returns 0 to cancel, 1 to continue.
# ----------------------------------------------------------------------
itcl::body Rappture::UQ::run_dialog {win} {

    set popup .pop_simulate
    if {[winfo exists $popup]} {destroy $popup}

    if { ![winfo exists $popup] } {

        Rappture::Balloon $popup -title "UQ Options"
        set inner [$popup component inner]
        labelframe  $inner.type -text "UQ Type"

        set _val [Rappture::Combobox $inner.type.val -width 20 -editable no]
        pack $_val -side top -expand yes -fill x -pady 5
        $_val choices insert end smolyak "Smolyak"
        $_val value [$_val label $_type]

        label $inner.type.labeltext -text "Level" -font "Arial 9"
        Rappture::Spinint $inner.type.level -min 1 -max 20
        $inner.type.level value $_args
        bind $inner.type.level <<Value>> [itcl::code $this _adjust_level $inner]
        pack $inner.type.labeltext $inner.type.level -side left -expand yes -fill x -pady 5
        pack $inner.type -side top -fill x

        set fr [frame $inner.frame]

        set nr [num_runs]
        label $fr.text

        #     bind $_val <<Value>> [itcl::code $this _change_param_type $inner]

        button $fr.cancel -text Cancel -command  [itcl::code $this _setWaitVariable 0]
        set _go $fr.go
        button $_go -text "Launch Jobs" -command  [itcl::code $this _setWaitVariable 1]
        _set_text $inner

        pack $fr.text -side top -pady 10
        pack $_go $fr.cancel -pady 4 -side right
        pack $fr
    }
    update idletasks

    _setWaitVariable -1
    $popup activate $win below
    tkwait variable [itcl::scope _wait_uq]
    $popup deactivate

    return $_wait_uq
}

itcl::body Rappture::UQ::_setWaitVariable {state} {
    set _wait_uq $state
}

itcl::body Rappture::UQ::_set_text {win} {
    set nr [num_runs]

    switch $_type {
        smolyak {
            if {$nr == 0} {
                set text "ERROR: The number of required jobs exceeds 100,000!\n\  Please\n\
                adjust the polynomial level or reduce the number of UQ variables."
                $_go configure -state disabled
            } else {
                set text "This method builds a polynomial response\
                function of degree $_args.\n It will require $nr runs."
                $_go configure -state normal
            }
        }
        default {
            set text "This will require $nr runs of the tool."
            $_go configure -state normal
        }
    }
    $win.frame.text configure -text $text
}

itcl::body Rappture::UQ::_adjust_level {win} {
    set _args [$win.type.level value]
    _set_text $win
}

itcl::body Rappture::UQ::num_runs {} {
    #puts "varlist=$_varlist"
    set numvars  $_num
    #puts "UQ num_runs $numvars $_type $_args"
    if {![array exists _num_pts]} { _init_num_pts_array }
    switch $_type {
        smolyak {
            return $_num_pts($numvars,$_args)
        }
        default {
            return $_args
        }
    }
}

itcl::body Rappture::UQ::_init_num_pts_array {} {
    # array is indexed (dim,level)
    unset _num_pts
    array set _num_pts [list \
        0,0 0 \
        0,1 0 \
        0,2 0 \
        0,3 0 \
        0,4 0 \
        0,5 0 \
        0,6 0 \
        0,7 0 \
        0,8 0 \
        0,9 0 \
        0,10 0 \
        0,11 0 \
        0,12 0 \
        0,13 0 \
        0,14 0 \
        0,15 0 \
        0,16 0 \
        0,17 0 \
        0,18 0 \
        0,19 0 \
        1,0 0 \
        1,1 3 \
        1,2 5 \
        1,3 9 \
        1,4 17 \
        1,5 33 \
        1,6 65 \
        1,7 129 \
        1,8 257 \
        1,9 513 \
        1,10 1025 \
        1,11 2049 \
        1,12 4097 \
        1,13 8193 \
        1,14 16385 \
        1,15 32769 \
        1,16 65537 \
        1,17 131073 \
        1,18 0 \
        1,19 0 \
        2,0 0 \
        2,1 5 \
        2,2 13 \
        2,3 29 \
        2,4 65 \
        2,5 145 \
        2,6 321 \
        2,7 705 \
        2,8 1537 \
        2,9 3329 \
        2,10 7169 \
        2,11 15361 \
        2,12 32769 \
        2,13 69633 \
        2,14 147457 \
        2,15 0 \
        2,16 0 \
        2,17 0 \
        2,18 0 \
        2,19 0 \
        3,0 0 \
        3,1 7 \
        3,2 25 \
        3,3 69 \
        3,4 177 \
        3,5 441 \
        3,6 1073 \
        3,7 2561 \
        3,8 6017 \
        3,9 13953 \
        3,10 32001 \
        3,11 72705 \
        3,12 163841 \
        3,13 0 \
        3,14 0 \
        3,15 0 \
        3,16 0 \
        3,17 0 \
        3,18 0 \
        3,19 0 \
        4,0 0 \
        4,1 9 \
        4,2 41 \
        4,3 137 \
        4,4 401 \
        4,5 1105 \
        4,6 2929 \
        4,7 7537 \
        4,8 18945 \
        4,9 46721 \
        4,10 113409 \
        4,11 0 \
        4,12 0 \
        4,13 0 \
        4,14 0 \
        4,15 0 \
        4,16 0 \
        4,17 0 \
        4,18 0 \
        4,19 0 \
        5,0 0 \
        5,1 11 \
        5,2 61 \
        5,3 241 \
        5,4 801 \
        5,5 2433 \
        5,6 6993 \
        5,7 19313 \
        5,8 51713 \
        5,9 135073 \
        5,10 0 \
        5,11 0 \
        5,12 0 \
        5,13 0 \
        5,14 0 \
        5,15 0 \
        5,16 0 \
        5,17 0 \
        5,18 0 \
        5,19 0 \
        6,0 0 \
        6,1 13 \
        6,2 85 \
        6,3 389 \
        6,4 1457 \
        6,5 4865 \
        6,6 15121 \
        6,7 44689 \
        6,8 127105 \
        6,9 0 \
        6,10 0 \
        6,11 0 \
        6,12 0 \
        6,13 0 \
        6,14 0 \
        6,15 0 \
        6,16 0 \
        6,17 0 \
        6,18 0 \
        6,19 0 \
        7,0 0 \
        7,1 15 \
        7,2 113 \
        7,3 589 \
        7,4 2465 \
        7,5 9017 \
        7,6 30241 \
        7,7 95441 \
        7,8 287745 \
        7,9 0 \
        7,10 0 \
        7,11 0 \
        7,12 0 \
        7,13 0 \
        7,14 0 \
        7,15 0 \
        7,16 0 \
        7,17 0 \
        7,18 0 \
        7,19 0 \
        8,0 0 \
        8,1 17 \
        8,2 145 \
        8,3 849 \
        8,4 3937 \
        8,5 15713 \
        8,6 56737 \
        8,7 190881 \
        8,8 0 \
        8,9 0 \
        8,10 0 \
        8,11 0 \
        8,12 0 \
        8,13 0 \
        8,14 0 \
        8,15 0 \
        8,16 0 \
        8,17 0 \
        8,18 0 \
        8,19 0 \
        9,0 0 \
        9,1 19 \
        9,2 181 \
        9,3 1177 \
        9,4 6001 \
        9,5 26017 \
        9,6 100897 \
        9,7 0 \
        9,8 0 \
        9,9 0 \
        9,10 0 \
        9,11 0 \
        9,12 0 \
        9,13 0 \
        9,14 0 \
        9,15 0 \
        9,16 0 \
        9,17 0 \
        9,18 0 \
        9,19 0 \
        10,0 0 \
        10,1 21 \
        10,2 221 \
        10,3 1581 \
        10,4 8801 \
        10,5 41265 \
        10,6 171425 \
        10,7 0 \
        10,8 0 \
        10,9 0 \
        10,10 0 \
        10,11 0 \
        10,12 0 \
        10,13 0 \
        10,14 0 \
        10,15 0 \
        10,16 0 \
        10,17 0 \
        10,18 0 \
        10,19 0 \
        11,0 0 \
        11,1 23 \
        11,2 265 \
        11,3 2069 \
        11,4 12497 \
        11,5 63097 \
        11,6 280017 \
        11,7 0 \
        11,8 0 \
        11,9 0 \
        11,10 0 \
        11,11 0 \
        11,12 0 \
        11,13 0 \
        11,14 0 \
        11,15 0 \
        11,16 0 \
        11,17 0 \
        11,18 0 \
        11,19 0 \
        12,0 0 \
        12,1 25 \
        12,2 313 \
        12,3 2649 \
        12,4 17265 \
        12,5 93489 \
        12,6 442001 \
        12,7 0 \
        12,8 0 \
        12,9 0 \
        12,10 0 \
        12,11 0 \
        12,12 0 \
        12,13 0 \
        12,14 0 \
        12,15 0 \
        12,16 0 \
        12,17 0 \
        12,18 0 \
        12,19 0 \
        13,0 0 \
        13,1 27 \
        13,2 365 \
        13,3 3329 \
        13,4 23297 \
        13,5 134785 \
        13,6 0 \
        13,7 0 \
        13,8 0 \
        13,9 0 \
        13,10 0 \
        13,11 0 \
        13,12 0 \
        13,13 0 \
        13,14 0 \
        13,15 0 \
        13,16 0 \
        13,17 0 \
        13,18 0 \
        13,19 0 \
        14,0 0 \
        14,1 29 \
        14,2 421 \
        14,3 4117 \
        14,4 30801 \
        14,5 189729 \
        14,6 0 \
        14,7 0 \
        14,8 0 \
        14,9 0 \
        14,10 0 \
        14,11 0 \
        14,12 0 \
        14,13 0 \
        14,14 0 \
        14,15 0 \
        14,16 0 \
        14,17 0 \
        14,18 0 \
        14,19 0 \
        15,0 0 \
        15,1 31 \
        15,2 481 \
        15,3 5021 \
        15,4 40001 \
        15,5 261497 \
        15,6 0 \
        15,7 0 \
        15,8 0 \
        15,9 0 \
        15,10 0 \
        15,11 0 \
        15,12 0 \
        15,13 0 \
        15,14 0 \
        15,15 0 \
        15,16 0 \
        15,17 0 \
        15,18 0 \
        15,19 0 \
        16,0 0 \
        16,1 33 \
        16,2 545 \
        16,3 6049 \
        16,4 51137 \
        16,5 353729 \
        16,6 0 \
        16,7 0 \
        16,8 0 \
        16,9 0 \
        16,10 0 \
        16,11 0 \
        16,12 0 \
        16,13 0 \
        16,14 0 \
        16,15 0 \
        16,16 0 \
        16,17 0 \
        16,18 0 \
        16,19 0 \
        17,0 0 \
        17,1 35 \
        17,2 613 \
        17,3 7209 \
        17,4 64465 \
        17,5 470561 \
        17,6 0 \
        17,7 0 \
        17,8 0 \
        17,9 0 \
        17,10 0 \
        17,11 0 \
        17,12 0 \
        17,13 0 \
        17,14 0 \
        17,15 0 \
        17,16 0 \
        17,17 0 \
        17,18 0 \
        17,19 0 \
        18,0 0 \
        18,1 37 \
        18,2 685 \
        18,3 8509 \
        18,4 80257 \
        18,5 616657 \
        18,6 0 \
        18,7 0 \
        18,8 0 \
        18,9 0 \
        18,10 0 \
        18,11 0 \
        18,12 0 \
        18,13 0 \
        18,14 0 \
        18,15 0 \
        18,16 0 \
        18,17 0 \
        18,18 0 \
        18,19 0 \
        19,0 0 \
        19,1 39 \
        19,2 761 \
        19,3 9957 \
        19,4 98801 \
        19,5 797241 \
        19,6 0 \
        19,7 0 \
        19,8 0 \
        19,9 0 \
        19,10 0 \
        19,11 0 \
        19,12 0 \
        19,13 0 \
        19,14 0 \
        19,15 0 \
        19,16 0 \
        19,17 0 \
        19,18 0 \
        19,19 0 \
    ]
}
