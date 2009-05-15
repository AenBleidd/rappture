set VERBOSE 0
if  {[info procs test] != "test" } {
    source defs
}

test flow.1 {flow no args} {
    list [catch {flow} msg] $msg
} {1 {wrong # args: should be one of...
  flow add name ?option value...?
  flow delete name...
  flow exists name
  flow names ?pattern?
  flow next 
  flow reset 
  flow video width height numFrames frameRate bitRate }}

test flow.2 {flow badOp } {
    list [catch {flow badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  flow add name ?option value...?
  flow delete name...
  flow exists name
  flow names ?pattern?
  flow next 
  flow reset 
  flow video width height numFrames frameRate bitRate }}

test flow.3 {flow add} {
    list [catch {flow add} msg] $msg
} {1 {wrong # args: should be "flow add name ?option value...?"}}

test flow.4 {flow names} {
    list [catch {flow names} msg] $msg
} {0 {}}

test flow.5 {flow names pattern1} {
    list [catch {flow names} msg] $msg
} {0 {}}

test flow.6 {flow names pattern1 pattern2} {
    list [catch {flow names} msg] $msg
} {0 {}}


test flow.7 {flow add ::test::flow} {
    list [catch {flow add ::test::flow } msg] $msg
} {0 ::test::flow}


test flow.8 {flow add myFlow } {
    list [catch {flow add myFlow} msg] $msg
} {0 myFlow}

test flow.8 {flow exists myFlow } {
    list [catch {flow exists myFlow} msg] $msg
} {0 1}

test flow.8 {flow exists badFlow } {
    list [catch {flow exists badFlow} msg] $msg
} {0 0}

test flow.9 {flow names} {
    list [catch {flow names} msg] $msg
} {0 {::test::flow myFlow}}

test flow.10 {flow delete} {
    list [catch {flow delete} msg] $msg
} {0 {}}


test flow.11 {flow delete badName } {
    list [catch {flow delete badName } msg] $msg
} {1 {can't find a flow "badName"}}

test flow.12 {flow delete ::test::flow myFlow } {
    list [catch {flow delete ::test::flow myFlow } msg] $msg
} {0 {}}

test flow.8 {flow exists myFlow } {
    list [catch {flow exists myFlow} msg] $msg
} {0 0}

test flow.13 {flow names} {
    list [catch {flow names} msg] $msg
} {0 {}}

test flow.14 {flow add myFlow } {
    list [catch {flow add myFlow} msg] $msg
} {0 myFlow}

test flow.15 {flow add myFlow } {
    list [catch {flow add myFlow} msg] $msg
} {1 {flow "myFlow" already exists.}}

test flow.16 {flow add myFlow2 -slice } {
    list [catch {flow add myFlow2 -slice } msg] $msg
} {1 {value for "-slice" missing}}

test flow.17 {flow add myFlow2 -slice yes -position 10% -axis y -badSwitch x } {
    list [catch {flow add myFlow2 -slice  yes -position 10% -axis y -badSwitch x} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -slice boolean
   -axis axis
   -hide boolean
   -position number
   -transferfunction name
   -volume boolean
   -outline boolean}}

test flow.18 {myFlow (no args)} {
    list [catch {myFlow} msg] $msg
} {1 {wrong # args: should be one of...
  myFlow box oper ?args?
  myFlow configure ?switches?
  myFlow data oper ?args?
  myFlow particles oper ?args?}}

test flow.19 {myFlow configure} {
    list [catch {myFlow configure} msg] $msg
} {0 {}}

test flow.20 {myFlow configure -badSwitch} {
    list [catch {myFlow configure -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -slice boolean
   -axis axis
   -hide boolean
   -position number
   -transferfunction name
   -volume boolean
   -outline boolean}}


test flow.21 {myFlow configure -slice} {
    list [catch {myFlow configure -slice} msg] $msg
} {1 {value for "-slice" missing}}

test flow.22 {myFlow configure -slice badValue} {
    list [catch {myFlow configure -slice badValue} msg] $msg
} {1 {expected boolean value but got "badValue"}}

test flow.23 {myFlow configure -slice yes} {
    list [catch {myFlow configure -slice yes} msg] $msg
} {0 {}}

test flow.24 {myFlow configure -slice no} {
    list [catch {myFlow configure -slice no} msg] $msg
} {0 {}}

test flow.25 {myFlow configure -axis} {
    list [catch {myFlow configure -axis} msg] $msg
} {1 {value for "-axis" missing}}

test flow.26 {myFlow configure -axis badValue} {
    list [catch {myFlow configure -axis badValue} msg] $msg
} {1 {bad axis "badValue": should be x, y, or z}}

test flow.27 {myFlow configure -axis 1.0} {
    list [catch {myFlow configure -axis 1.0} msg] $msg
} {1 {bad axis "1.0": should be x, y, or z}}

test flow.28 {myFlow configure -axis x} {
    list [catch {myFlow configure -axis x} msg] $msg
} {0 {}}

test flow.29 {myFlow configure -axis X} {
    list [catch {myFlow configure -axis X} msg] $msg
} {0 {}}

test flow.30 {myFlow configure -axis y} {
    list [catch {myFlow configure -axis y} msg] $msg
} {0 {}}

test flow.31 {myFlow configure -axis Y} {
    list [catch {myFlow configure -axis Y} msg] $msg
} {0 {}}

test flow.32 {myFlow configure -axis z} {
    list [catch {myFlow configure -axis z} msg] $msg
} {0 {}}

test flow.33 {myFlow configure -axis Z} {
    list [catch {myFlow configure -axis Z} msg] $msg
} {0 {}}

test flow.34 {myFlow configure -hide} {
    list [catch {myFlow configure -hide} msg] $msg
} {1 {value for "-hide" missing}}

test flow.35 {myFlow configure -hide badValue} {
    list [catch {myFlow configure -hide badValue} msg] $msg
} {1 {expected boolean value but got "badValue"}}

test flow.36 {myFlow configure -hide yes} {
    list [catch {myFlow configure -hide yes} msg] $msg
} {0 {}}

test flow.37 {myFlow configure -hide no} {
    list [catch {myFlow configure -hide no} msg] $msg
} {0 {}}

test flow.38 {myFlow configure -position} {
    list [catch {myFlow configure -position} msg] $msg
} {1 {value for "-position" missing}}

test flow.39 {myFlow configure -position badValue} {
    list [catch {myFlow configure -position badValue} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test flow.40 {myFlow configure -position 0%} {
    list [catch {myFlow configure -position 0%} msg] $msg
} {0 {}}

test flow.41 {myFlow configure -position 100%} {
    list [catch {myFlow configure -position 100%} msg] $msg
} {0 {}}

test flow.42 {myFlow configure -position 100% -axis z -hide no -slice yes} {
    list [catch {myFlow configure -position 100.0% -axis z -hide no -slice yes} msg] $msg
} {0 {}}

test flow.43 {myFlow data (no args)} {
    list [catch {myFlow data} msg] $msg
} {1 {wrong # args: should be one of...
  myFlow data file fileName extents
  myFlow data follows size extents}}

test flow.44 {myFlow data badOp} {
    list [catch {myFlow data badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myFlow data file fileName extents
  myFlow data follows size extents}}

test flow.45 {myFlow data file} {
    list [catch {myFlow data file} msg] $msg
} {1 {wrong # args: should be "myFlow data file fileName extents"}}

test flow.46 {myFlow data file fileName} {
    list [catch {myFlow data file fileName} msg] $msg
} {1 {wrong # args: should be "myFlow data file fileName extents"}}

test flow.47 {myFlow data file badFileName 3} {
    list [catch {myFlow data file badFileName 3} msg] $msg
} {1 {can't load data from "badFileName": Rappture::Buffer::load()
:
can't open "badFileName": No such file or directory}}

test flow.48 {myFlow data follows} {
    list [catch {myFlow data follows} msg] $msg
} {1 {wrong # args: should be "myFlow data follows size extents"}}

test flow.49 {myFlow data follows 0} {
    list [catch {myFlow data follows 0} msg] $msg
} {1 {wrong # args: should be "myFlow data follows size extents"}}

test flow.50 {myFlow data follows 0 3} {
    list [catch {myFlow data follows 0 3} msg] $msg
} {1 {bad # bytes request "0" for "data follows"}}

test flow.51 {myFlow data file data/flowvis_dx_files/jwire/J-wire-vec.dx 3} {
    list [catch {myFlow data file data/flowvis_dx_files/jwire/J-wire-vec.dx 3} msg] $msg
} {0 {}}

test flow.52 {myFlow (no args)} {
    list [catch {myFlow} msg] $msg
} {1 {wrong # args: should be one of...
  myFlow box oper ?args?
  myFlow configure ?switches?
  myFlow data oper ?args?
  myFlow particles oper ?args?}}

test flow.53 {myFlow particles} {
    list [catch {myFlow particles} msg] $msg
} {1 {wrong # args: should be one of...
  myFlow particles add name ?switches?
  myFlow particles configure name ?switches?
  myFlow particles delete ?name...?
  myFlow particles names ?pattern?}}


test flow.54 {myFlow particles badOp} {
    list [catch {myFlow particles badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myFlow particles add name ?switches?
  myFlow particles configure name ?switches?
  myFlow particles delete ?name...?
  myFlow particles names ?pattern?}}

test flow.55 {myFlow particles names} {
    list [catch {myFlow particles names} msg] $msg
} {0 {}}

test flow.56 {myFlow particles names pattern1} {
    list [catch {myFlow particles names pattern1} msg] $msg
} {0 {}}

test flow.57 {myFlow particles names pattern1} {
    list [catch {myFlow particles names pattern1} msg] $msg
} {0 {}}

test flow.58 {myFlow particles add} {
    list [catch {myFlow particles add} msg] $msg
} {1 {wrong # args: should be "myFlow particles add name ?switches?"}}

test flow.59 {myFlow particles add myPlane} {
    list [catch {myFlow particles add myPlane} msg] $msg
} {0 myPlane}

test flow.60 {myFlow particles add myPlane} {
    list [catch {myFlow particles add myPlane} msg] $msg
} {1 {particle injection plane "myPlane" already exists.}}

test flow.61 {myFlow particles names} {
    list [catch {myFlow particles names} msg] $msg
} {0 myPlane}

test flow.62 {myFlow particles delete myPlane } {
    list [catch {myFlow particles delete myPlane} msg] $msg
} {0 {}}

test flow.63 {myFlow particles names} {
    list [catch {myFlow particles names} msg] $msg
} {0 {}}

test flow.64 {myFlow particles add myPlane -hide} {
    list [catch {myFlow particles add myPlane -hide} msg] $msg
} {1 {value for "-hide" missing}}

test flow.65 {myFlow particles add myPlane -hide badValue} {
    list [catch {myFlow particles add myPlane -hide badValue} msg] $msg
} {1 {expected boolean value but got "badValue"}}

test flow.66 {myFlow particles add myPlane -hide yes} {
    list [catch {myFlow particles add myPlane -hide yes} msg] $msg
} {0 myPlane}

test flow.67 {myFlow particles configure myPlane -badSwitch} {
    list [catch {myFlow particles configure myPlane -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -axis string
   -color {r g b a}
   -hide boolean
   -position number}}

test flow.68 {myFlow particles configure} {
    list [catch {myFlow particles configure} msg] $msg
} {1 {wrong # args: should be "myFlow particles configure name ?switches?"}}

test flow.69 {myFlow particles configure badName} {
    list [catch {myFlow particles configure badName} msg] $msg
} {1 {can't find a particle injection plane "badName"}}

test flow.70 {myFlow particles configure -hide} {
    list [catch {myFlow particles configure -hide} msg] $msg
} {1 {can't find a particle injection plane "-hide"}}

test flow.71 {myFlow particles configure myPlane} {
    list [catch {myFlow particles configure myPlane} msg] $msg
} {0 {}}

test flow.72 {myFlow particles configure myPlane -hide} {
    list [catch {myFlow particles configure myPlane -hide} msg] $msg
} {1 {value for "-hide" missing}}

test flow.73 {myFlow particles configure myPlane -hide yes} {
    list [catch {myFlow particles configure myPlane -hide yes} msg] $msg
} {0 {}}

test flow.74 {myFlow particles configure myPlane -hide no} {
    list [catch {myFlow particles configure myPlane -hide no} msg] $msg
} {0 {}}

test flow.75 {myFlow particles configure myPlane -position} {
    list [catch {myFlow particles configure myPlane -position} msg] $msg
} {1 {value for "-position" missing}}

test flow.76 {myFlow particles add myPlane -position badValue} {
    list [catch {myFlow particles configure myPlane -position badValue} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test flow.77 {myFlow particles configure myPlane -position 100%} {
    list [catch {myFlow particles configure myPlane -position 100%} msg] $msg
} {0 {}}

test flow.78 {myFlow particles configure myPlane -position 0%} {
    list [catch {myFlow particles configure myPlane -position 0%} msg] $msg
} {0 {}}

test flow.79 {myFlow particles configure myPlane -position 1e+2% -hide yes} {
    list [catch {myFlow particles configure myPlane -position 1e+2% -hide yes} msg] $msg
} {0 {}}

test flow.80 {myFlow particles configure myPlane -position} {
    list [catch {myFlow particles configure myPlane -position} msg] $msg
} {1 {value for "-position" missing}}

test flow.81 {myFlow particles add myPlane -position badValue} {
    list [catch {myFlow particles configure myPlane -position badValue} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test flow.82 {myFlow particles configure myPlane -position 100%} {
    list [catch {myFlow particles configure myPlane -position 100%} msg] $msg
} {0 {}}

test flow.83 {myFlow particles configure myPlane -position 0%} {
    list [catch {myFlow particles configure myPlane -position 0%} msg] $msg
} {0 {}}

test flow.84 {myFlow particles configure myPlane -position 100% -hide yes} {
    list [catch {myFlow particles configure myPlane -position 100% -hide yes} msg] $msg
} {0 {}}

test flow.85 {myFlow particles configure myPlane -color} {
    list [catch {myFlow particles configure myPlane -color} msg] $msg
} {1 {value for "-color" missing}}

test flow.86 {myFlow particles add myPlane -color badValue} {
    list [catch {myFlow particles configure myPlane -color badValue} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.87 {myFlow particles configure myPlane -color 1.0} {
    list [catch {myFlow particles configure myPlane -color 1.0} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.88 {myFlow particles configure myPlane -color { 1. 1. 1.}} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1.}} msg] $msg
} {0 {}}

test flow.89 {myFlow particles configure myPlane -color { 1. 1. 1. }} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1.}} msg] $msg
} {0 {}}

test flow.90 {myFlow particles configure myPlane -color { 1. 1. 1. 1. }} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1. 1.}} msg] $msg
} {0 {}}

test flow.91 {myFlow particles configure myPlane -color { 1. 1. 1. 1. 1. }} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1. 1. 1.}} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.92 {myFlow particles configure myPlane -color { 10  1. 1. 1. }} {
    list [catch {myFlow particles configure myPlane -color {10 1. 1. 1.}} msg] $msg
} {1 {bad component value in "10 1. 1. 1.": color values must be [0..1]}}

test flow.93 {myFlow particles configure myPlane -color { badValue 1. 1. 1}} {
    list [catch {myFlow particles configure myPlane -color {badValue 1 1 1}} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test flow.94 {myFlow particles configure myPlane -color { 1. 1. 1. 2. }} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1. 2.}} msg] $msg
} {1 {bad component value in "1. 1. 1. 2.": color values must be [0..1]}}

test flow.95 {myFlow particles configure myPlane -color { 1. 1. 1. 1. 1. }} {
    list [catch {myFlow particles configure myPlane -color {1. 1. 1. 1. 1.}} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.96 {myFlow particles configure myPlane -color { 0 0 0 0 }} {
    list [catch {myFlow particles configure myPlane -color { 0 0 0 0 }} msg] $msg
} {0 {}}

test flow.97 {myFlow particles configure myPlane -axis} {
    list [catch {myFlow particles configure myPlane -axis} msg] $msg
} {1 {value for "-axis" missing}}

test flow.98 {myFlow particles configure myPlane -axis badValue} {
    list [catch {myFlow particles configure myPlane -axis badValue} msg] $msg
} {1 {bad axis "badValue": should be x, y, or z}}

test flow.99 {myFlow particles configure myPlane -axis 1.0} {
    list [catch {myFlow particles configure myPlane -axis 1.0} msg] $msg
} {1 {bad axis "1.0": should be x, y, or z}}

test flow.100 {myFlow particles configure myPlane -axis x} {
    list [catch {myFlow particles configure myPlane -axis x} msg] $msg
} {0 {}}

test flow.101 {myFlow particles configure myPlane -axis X} {
    list [catch {myFlow particles configure myPlane -axis X} msg] $msg
} {0 {}}

test flow.102 {myFlow particles configure myPlane -axis y} {
    list [catch {myFlow particles configure myPlane -axis y} msg] $msg
} {0 {}}

test flow.103 {myFlow particles configure myPlane -axis Y} {
    list [catch {myFlow particles configure myPlane -axis Y} msg] $msg
} {0 {}}

test flow.104 {myFlow particles configure myPlane -axis z} {
    list [catch {myFlow particles configure myPlane -axis z} msg] $msg
} {0 {}}

test flow.105 {myFlow particles configure myPlane -axis Z} {
    list [catch {myFlow particles configure myPlane -axis Z} msg] $msg
} {0 {}}


test flow.106 {myFlow particles configure myPlane -position 100% -hide yes} {
    list [catch {myFlow particles configure myPlane -position 100% -hide yes} msg] $msg
} {0 {}}

test flow.107 {myFlow particles add myPlane2 -hide no} {
    list [catch {myFlow particles add myPlane2 -hide no} msg] $msg
} {0 myPlane2}

test flow.108 {myFlow particles names} {
    list [catch {myFlow particles names} msg] $msg
} {0 {myPlane2 myPlane}}

test flow.109 {myFlow particles delete myPlane myPlane2} {
    list [catch {myFlow particles delete myPlane myPlane2} msg] $msg
} {0 {}}

test flow.110 {myFlow particles names} {
    list [catch {myFlow particles names} msg] $msg
} {0 {}}

test flow.111 {myFlow box} {
    list [catch {myFlow box} msg] $msg
} {1 {wrong # args: should be one of...
  myFlow box add name ?switches?
  myFlow box configure name ?switches?
  myFlow box delete ?name...?
  myFlow box names ?pattern?}}

test flow.112 {myFlow box badOp} {
    list [catch {myFlow box badOp} msg] $msg
} {1 {bad operation "badOp": should be one of...
  myFlow box add name ?switches?
  myFlow box configure name ?switches?
  myFlow box delete ?name...?
  myFlow box names ?pattern?}}

test flow.113 {myFlow box names} {
    list [catch {myFlow box names} msg] $msg
} {0 {}}

test flow.114 {myFlow box names pattern1} {
    list [catch {myFlow box names pattern1} msg] $msg
} {0 {}}

test flow.115 {myFlow box names pattern1} {
    list [catch {myFlow box names pattern1} msg] $msg
} {0 {}}

test flow.116 {myFlow box add} {
    list [catch {myFlow box add} msg] $msg
} {1 {wrong # args: should be "myFlow box add name ?switches?"}}

test flow.117 {myFlow box add myBox} {
    list [catch {myFlow box add myBox} msg] $msg
} {0 myBox}

test flow.118 {myFlow box add myBox} {
    list [catch {myFlow box add myBox} msg] $msg
} {1 {box "myBox" already exists in flow "myFlow"}}

test flow.119 {myFlow box names} {
    list [catch {myFlow box names} msg] $msg
} {0 myBox}

test flow.120 {myFlow box delete myBox } {
    list [catch {myFlow box delete myBox} msg] $msg
} {0 {}}

test flow.121 {myFlow box names} {
    list [catch {myFlow box names} msg] $msg
} {0 {}}

test flow.122 {myFlow box add myBox -hide} {
    list [catch {myFlow box add myBox -hide} msg] $msg
} {1 {value for "-hide" missing}}

test flow.123 {myFlow box add myBox -hide badValue} {
    list [catch {myFlow box add myBox -hide badValue} msg] $msg
} {1 {expected boolean value but got "badValue"}}

test flow.124 {myFlow box add myBox -hide yes} {
    list [catch {myFlow box add myBox -hide yes} msg] $msg
} {0 myBox}

test flow.125 {myFlow box configure myBox -badSwitch} {
    list [catch {myFlow box configure myBox -badSwitch} msg] $msg
} {1 {unknown switch "-badSwitch"
following switches are available:
   -color {r g b a}
   -corner1 {x y z}
   -corner2 {x y z}
   -hide boolean
   -linewidth number}}

test flow.126 {myFlow box configure} {
    list [catch {myFlow box configure} msg] $msg
} {1 {wrong # args: should be "myFlow box configure name ?switches?"}}

test flow.127 {myFlow box configure badName} {
    list [catch {myFlow box configure badName} msg] $msg
} {1 {can't find a box "badName" in flow "myFlow"}}

test flow.128 {myFlow box configure -hide} {
    list [catch {myFlow box configure -hide} msg] $msg
} {1 {can't find a box "-hide" in flow "myFlow"}}

test flow.129 {myFlow box configure myBox} {
    list [catch {myFlow box configure myBox} msg] $msg
} {0 {}}

test flow.130 {myFlow box configure myBox -hide} {
    list [catch {myFlow box configure myBox -hide} msg] $msg
} {1 {value for "-hide" missing}}

test flow.131 {myFlow box configure myBox -hide yes} {
    list [catch {myFlow box configure myBox -hide yes} msg] $msg
} {0 {}}

test flow.132 {myFlow box configure myBox -hide no} {
    list [catch {myFlow box configure myBox -hide no} msg] $msg
} {0 {}}

test flow.133 {myFlow box configure myBox -corner1} {
    list [catch {myFlow box configure myBox -corner1} msg] $msg
} {1 {value for "-corner1" missing}}

test flow.134 {myFlow box add myBox -corner1 badValue} {
    list [catch {myFlow box configure myBox -corner1 badValue} msg] $msg
} {1 {wrong # of elements for box coordinates:  should be "x y z"}}

test flow.135 {myFlow box configure myBox -corner1 {0 0 0}} {
    list [catch {myFlow box configure myBox -corner1 {0 0 0}} msg] $msg
} {0 {}}

test flow.136 {myFlow box configure myBox -corner1 {0 0 0} -corner2 { 1 1 1 }} {
    list [catch {
	myFlow box configure myBox -corner1 {0 0 0} -corner2 {1 1 1}
    } msg] $msg
} {0 {}}

test flow.137 {myFlow box configure myBox -corner1 { 1 1 1 } -hide yes} {
    list [catch {myFlow box configure myBox -corner1 {1 1 1} -hide yes} msg] $msg
} {0 {}}

test flow.138 {myFlow box configure myBox -color} {
    list [catch {myFlow box configure myBox -color} msg] $msg
} {1 {value for "-color" missing}}

test flow.139 {myFlow box add myBox -color badValue} {
    list [catch {myFlow box configure myBox -color badValue} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.140 {myFlow box configure myBox -color 1.0} {
    list [catch {myFlow box configure myBox -color 1.0} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.141 {myFlow box configure myBox -color { 1. 1. 1.}} {
    list [catch {myFlow box configure myBox -color {1. 1. 1.}} msg] $msg
} {0 {}}

test flow.142 {myFlow box configure myBox -color { 1. 1. 1. }} {
    list [catch {myFlow box configure myBox -color {1. 1. 1.}} msg] $msg
} {0 {}}

test flow.143 {myFlow box configure myBox -color { 1. 1. 1. 1. }} {
    list [catch {myFlow box configure myBox -color {1. 1. 1. 1.}} msg] $msg
} {0 {}}

test flow.144 {myFlow box configure myBox -color { 1. 1. 1. 1. 1. }} {
    list [catch {myFlow box configure myBox -color {1. 1. 1. 1. 1.}} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.145 {myFlow box configure myBox -color { 10  1. 1. 1. }} {
    list [catch {myFlow box configure myBox -color {10 1. 1. 1.}} msg] $msg
} {1 {bad component value in "10 1. 1. 1.": color values must be [0..1]}}

test flow.146 {myFlow box configure myBox -color { badValue 1. 1. 1}} {
    list [catch {myFlow box configure myBox -color {badValue 1 1 1}} msg] $msg
} {1 {expected floating-point number but got "badValue"}}

test flow.147 {myFlow box configure myBox -color { 1. 1. 1. 2. }} {
    list [catch {myFlow box configure myBox -color {1. 1. 1. 2.}} msg] $msg
} {1 {bad component value in "1. 1. 1. 2.": color values must be [0..1]}}

test flow.148 {myFlow box configure myBox -color { 1. 1. 1. 1. 1. }} {
    list [catch {myFlow box configure myBox -color {1. 1. 1. 1. 1.}} msg] $msg
} {1 {wrong # of elements in color definition}}

test flow.149 {myFlow box configure myBox -color { 0 0 0 0 }} {
    list [catch {myFlow box configure myBox -color { 0 0 0 0 }} msg] $msg
} {0 {}}

test flow.150 {myFlow box configure myBox -corner1 {1 1 1} -corner2 {0 0 0} -hide yes} {
    list [catch {myFlow box configure myBox -corner1 {1 1 1} -corner2 {0 0 0} -hide yes} msg] $msg
} {0 {}}

test flow.151 {myFlow box configure myBox -corner1 {1 1 1} -corner2 {0 0 0} -hide yes} {
    list [catch {myFlow box configure myBox -corner1 {1 1 1} -corner2 {0 0 0} -hide yes} msg] $msg
} {0 {}}


test flow.152 {myFlow box add myBox2 -hide no} {
    list [catch {myFlow box add myBox2 -hide no} msg] $msg
} {0 myBox2}

test flow.153 {myFlow box names} {
    list [catch {myFlow box names} msg] $msg
} {0 {myBox2 myBox}}

test flow.154 {myFlow box delete myBox myBox2} {
    list [catch {myFlow box delete myBox myBox2} msg] $msg
} {0 {}}

test flow.155 {myFlow box names} {
    list [catch {myFlow box names} msg] $msg
} {0 {}}

test flow.156 {flow no args} {
    list [catch {flow} msg] $msg
} {1 {wrong # args: should be one of...
  flow add name ?option value...?
  flow delete name...
  flow exists name
  flow names ?pattern?
  flow next 
  flow reset 
  flow video width height numFrames frameRate bitRate }}

test flow.157 {flow names} {
    list [catch {flow names} msg] $msg
} {0 myFlow}

test flow.158 {myFlow data file data/flowvis_dx_files/jwire/J-wire-vec.dx 3} {
    list [catch {myFlow data file data/flowvis_dx_files/jwire/J-wire-vec.dx 3} msg] $msg
} {0 {}}

test flow.159 {camera angle 0 0 0} {
    list [catch {
	screen 800 600
	camera angle 45 0 0

	camera zoom 1.5
	axis visible off
    } msg] $msg
} {0 {}}

test flow.160 {flow next} {
    list [catch {flow next} msg] $msg
} {0 {}}

test flow.161 {myFlow particles add plane1 -position 90% -axis x -color {1 0 0} } {
    list [catch {
	myFlow particles add plane1 -position 90% -axis x \
	    -hide no -color {1 0 0 0.2}} msg] $msg
} {0 plane1}

test flow.162 {myFlow particles add plane2 -position 10% -axis x -color {0 0 1} } {
    list [catch {
	myFlow particles add plane2 -position 10% -axis x \
	    -hide no -color {0 0 1 0.2}} msg] $msg
} {0 plane2}

test flow.163 {myFlow configure -axis z -position 100% -slice yes} {
    list [catch {
	myFlow configure -axis z -position 100% -slice yes \
	    -volume yes -outline yes
    } msg] $msg
} {0 {}}

test flow.164 {myFlow box add box1} {
    list [catch {
	myFlow box add box1 -hide no -corner1 "0 0 0" \
	    -corner2 "3000 3000 3000" -color "0 1 1"
    } msg] $msg
} {0 box1}

test flow.165 {myFlow box add box2} {
    list [catch {
	myFlow box add box2 -hide no \
	    -corner1 "0 -100 -100" -corner2 "3000 400 400" \
	    -color "1 1 0" -linewidth 5.0
    } msg] $msg
} {0 box2}

test flow.166 {myFlow box add box3} {
    list [catch {
	myFlow box add box3 -hide no \
	    -corner1 "1000 -150 -100" -corner2 "2000 450 450" \
	    -color "1 0 1" -linewidth 10.2
    } msg] $msg
} {0 box3}

test flow.167 {axis visible no} {
    list [catch {axis visible no} msg] $msg
} {0 {}}

test flow.168 {flow next extraArg} {
    list [catch {flow next extraArg} msg] $msg
} {1 {wrong # args: should be "flow next "}}

test flow.169 {flow next} {
    list [catch {flow next} msg] $msg
} {0 {}}

test flow.170 {flow reset} {
    list [catch {flow reset} msg] $msg
} {0 {}}

test flow.171 {flow next (5 frames)} {
    list [catch {
	flow reset
	for { set i 0 } { $i < 5 } { incr i } {
	    flow next
	}
    } msg] $msg
} {0 {}}

test flow.172 {flow reset} {
    list [catch {flow reset} msg] $msg
} {0 {}}

test flow.173 {flow next (5 frames)} {
    list [catch {
	flow reset
	for { set i 0 } { $i < 5 } { incr i } {
	    flow next
	}
    } msg] $msg
} {0 {}}

test flow.174 {flow delete myFlow} {
    list [catch {flow delete myFlow} msg] $msg
} {0 {}}


exit 0


