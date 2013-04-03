/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#define NUM_ELEMENTS 109

// jmol colors (taken from VisIt)
const unsigned char g_elementColors[NUM_ELEMENTS+1][3] = {
    { 0x20, 0x20, 0x20 }, // ?
    { 0xFF, 0xFF, 0xFF }, // H
    { 0xD9, 0xFF, 0xFF }, // He
    { 0xCC, 0x80, 0xFF }, // Li
    { 0xC2, 0xFF, 0x00 }, // Be
    { 0xFF, 0xB5, 0xB5 }, // B
    { 0x90, 0x90, 0x90 }, // C
    { 0x30, 0x50, 0xF8 }, // N
    { 0xFF, 0x0D, 0x0D }, // O
    { 0x90, 0xE0, 0x50 }, // F
    { 0xB3, 0xE3, 0xF5 }, // Ne
    { 0xAB, 0x5C, 0xF2 }, // Na
    { 0x8A, 0xFF, 0x00 }, // Mg
    { 0xBF, 0xA6, 0xA6 }, // Al
    { 0xF0, 0xC8, 0xA0 }, // Si
    { 0xFF, 0x80, 0x00 }, // P
    { 0xFF, 0xFF, 0x30 }, // S
    { 0x1F, 0xF0, 0x1F }, // Cl
    { 0x80, 0xD1, 0xE3 }, // Ar
    { 0x8F, 0x40, 0xD4 }, // K
    { 0x3D, 0xFF, 0x00 }, // Ca
    { 0xE6, 0xE6, 0xE6 }, // Sc
    { 0xBF, 0xC2, 0xC7 }, // Ti
    { 0xA6, 0xA6, 0xAB }, // V
    { 0x8A, 0x99, 0xC7 }, // Cr
    { 0x9C, 0x7A, 0xC7 }, // Mn
    { 0xE0, 0x66, 0x33 }, // Fe
    { 0xF0, 0x90, 0xA0 }, // Co
    { 0x50, 0xD0, 0x50 }, // Ni
    { 0xC8, 0x80, 0x33 }, // Cu
    { 0x7D, 0x80, 0xB0 }, // Zn
    { 0xC2, 0x8F, 0x8F }, // Ga
    { 0x66, 0x8F, 0x8F }, // Ge
    { 0xBD, 0x80, 0xE3 }, // As
    { 0xFF, 0xA1, 0x00 }, // Se
    { 0xA6, 0x29, 0x29 }, // Br
    { 0x5C, 0xB8, 0xD1 }, // Kr
    { 0x70, 0x2E, 0xB0 }, // Rb
    { 0x00, 0xFF, 0x00 }, // Sr
    { 0x94, 0xFF, 0xFF }, // Y
    { 0x94, 0xE0, 0xE0 }, // Zr
    { 0x73, 0xC2, 0xC9 }, // Nb
    { 0x54, 0xB5, 0xB5 }, // Mo
    { 0x3B, 0x9E, 0x9E }, // Tc
    { 0x24, 0x8F, 0x8F }, // Ru
    { 0x0A, 0x7D, 0x8C }, // Rh
    { 0x00, 0x69, 0x85 }, // Pd
    { 0xC0, 0xC0, 0xC0 }, // Ag
    { 0xFF, 0xD9, 0x8F }, // Cd
    { 0xA6, 0x75, 0x73 }, // In
    { 0x66, 0x80, 0x80 }, // Sn
    { 0x9E, 0x63, 0xB5 }, // Sb
    { 0xD4, 0x7A, 0x00 }, // Te
    { 0x94, 0x00, 0x94 }, // I
    { 0x42, 0x9E, 0xB0 }, // Xe
    { 0x57, 0x17, 0x8F }, // Cs
    { 0x00, 0xC9, 0x00 }, // Ba
    { 0x70, 0xD4, 0xFF }, // La
    { 0xFF, 0xFF, 0xC7 }, // Ce
    { 0xD9, 0xFF, 0xC7 }, // Pr
    { 0xC7, 0xFF, 0xC7 }, // Nd
    { 0xA3, 0xFF, 0xC7 }, // Pm
    { 0x8F, 0xFF, 0xC7 }, // Sm
    { 0x61, 0xFF, 0xC7 }, // Eu
    { 0x45, 0xFF, 0xC7 }, // Gd
    { 0x30, 0xFF, 0xC7 }, // Tb
    { 0x1F, 0xFF, 0xC7 }, // Dy
    { 0x00, 0xFF, 0x9C }, // Ho
    { 0x00, 0xE6, 0x75 }, // Er
    { 0x00, 0xD4, 0x52 }, // Tm
    { 0x00, 0xBF, 0x38 }, // Yb
    { 0x00, 0xAB, 0x24 }, // Lu
    { 0x4D, 0xC2, 0xFF }, // Hf
    { 0x4D, 0xA6, 0xFF }, // Ta
    { 0x21, 0x94, 0xD6 }, // W
    { 0x26, 0x7D, 0xAB }, // Re
    { 0x26, 0x66, 0x96 }, // Os
    { 0x17, 0x54, 0x87 }, // Ir
    { 0xD0, 0xD0, 0xE0 }, // Pt
    { 0xFF, 0xD1, 0x23 }, // Au
    { 0xB8, 0xB8, 0xD0 }, // Hg
    { 0xA6, 0x54, 0x4D }, // Tl
    { 0x57, 0x59, 0x61 }, // Pb
    { 0x9E, 0x4F, 0xB5 }, // Bi
    { 0xAB, 0x5C, 0x00 }, // Po
    { 0x75, 0x4F, 0x45 }, // At
    { 0x42, 0x82, 0x96 }, // Rn
    { 0x42, 0x00, 0x66 }, // Fr
    { 0x00, 0x7D, 0x00 }, // Ra
    { 0x70, 0xAB, 0xFA }, // Ac
    { 0x00, 0xBA, 0xFF }, // Th
    { 0x00, 0xA1, 0xFF }, // Pa
    { 0x00, 0x8F, 0xFF }, // U
    { 0x00, 0x80, 0xFF }, // Np
    { 0x00, 0x6B, 0xFF }, // Pu
    { 0x54, 0x5C, 0xF2 }, // Am
    { 0x78, 0x5C, 0xE3 }, // Cm
    { 0x8A, 0x4F, 0xE3 }, // Bk
    { 0xA1, 0x36, 0xD4 }, // Cf
    { 0xB3, 0x1F, 0xD4 }, // Es
    { 0xB3, 0x1F, 0xBA }, // Fm
    { 0xB3, 0x0D, 0xA6 }, // Md
    { 0xBD, 0x0D, 0x87 }, // No
    { 0xC7, 0x00, 0x66 }, // Lr
    { 0xCC, 0x00, 0x59 }, // Rf
    { 0xD1, 0x00, 0x4F }, // Db
    { 0xD9, 0x00, 0x45 }, // Sg
    { 0xE0, 0x00, 0x38 }, // Bh
    { 0xE6, 0x00, 0x2E }, // Hs
    { 0xEB, 0x00, 0x26 }  // Mt
};

// Covalent radii in Angstroms from VisIt
const float g_covalentRadii[NUM_ELEMENTS+1] = {
     .1f,  // ?  0
    0.32f, // H  1
    0.93f, // He 2
    1.23f, // Li 3
    0.90f, // Be 4
    0.82f, // B  5
    0.77f, // C  6
    0.75f, // N  7
    0.73f, // O  8
    0.72f, // F  9
    0.71f, // Ne 10
    1.54f, // Na 11
    1.36f, // Mg 12
    1.18f, // Al 13
    1.11f, // Si 14
    1.06f, // P  15
    1.02f, // S  16
    0.99f, // Cl 17
    0.98f, // Ar 18
    2.03f, // K  19
    1.74f, // Ca 20
    1.44f, // Sc 21
    1.32f, // Ti 22
    1.22f, // V  23
    1.18f, // Cr 24
    1.17f, // Mn 25
    1.17f, // Fe 26
    1.16f, // Co 27
    1.15f, // Ni 28
    1.17f, // Cu 29
    1.25f, // Zn 30
    1.26f, // Ga 31
    1.22f, // Ge 32
    1.20f, // As 33
    1.16f, // Se 34
    1.14f, // Br 35
    1.12f, // Kr 36
    2.16f, // Rb 37
    1.91f, // Sr 38
    1.62f, // Y  39
    1.45f, // Zr 40
    1.34f, // Nb 41
    1.30f, // Mo 42
    1.27f, // Tc 43
    1.25f, // Ru 44
    1.25f, // Rh 45
    1.28f, // Pd 46
    1.34f, // Ag 47
    1.48f, // Cd 48
    1.44f, // In 49
    1.41f, // Sn 50
    1.40f, // Sb 51
    1.36f, // Te 52
    1.33f, // I  53
    1.31f, // Xe 54
    2.35f, // Cs 55
    1.98f, // Ba 56
    1.69f, // La 57
    1.65f, // Ce 58
    1.65f, // Pr 59
    1.64f, // Nd 60
    1.63f, // Pm 61
    1.62f, // Sm 62
    1.85f, // Eu 63
    1.61f, // Gd 64
    1.59f, // Tb 65
    1.59f, // Dy 66
    1.58f, // Ho 67
    1.57f, // Er 68
    1.56f, // Tm 69
    1.74f, // Yb 70
    1.56f, // Lu 71
    1.44f, // Hf 72
    1.34f, // Ta 73
    1.30f, // W  74
    1.28f, // Re 75
    1.26f, // Os 76
    1.27f, // Ir 77
    1.30f, // Pt 78
    1.34f, // Au 79
    1.49f, // Hg 80
    1.48f, // Tl 81
    1.47f, // Pb 82
    1.46f, // Bi 83
    1.46f, // Po 84
    1.45f, // At 85
    1.50f, // Rn 86 // 86-96 from en.wikipedia.org/wiki/Covalent_radius 10/6/09
    2.60f, // Fr 87
    2.21f, // Ra 88
    2.15f, // Ac 89
    2.06f, // Th 90
    2.00f, // Pa 91
    1.96f, // U  92
    1.90f, // Np 93
    1.87f, // Pu 94
    1.80f, // Am 95
    1.69f, // Cm 96
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f
};

// Atomic radii in Angstroms from VisIt
const float g_atomicRadii[NUM_ELEMENTS+1] = {
     .1f,  // ?  0
    0.79f, // H  1
    0.49f, // He 2
    2.05f, // Li 3
    1.40f, // Be 4
    1.17f, // B  5
    0.91f, // C  6
    0.75f, // N  7
    0.65f, // O  8
    0.57f, // F  9
    0.51f, // Ne 10
    2.23f, // Na 11
    1.72f, // Mg 12
    1.82f, // Al 13
    1.46f, // Si 14
    1.23f, // P  15
    1.09f, // S  16
    0.97f, // Cl 17
    0.88f, // Ar 18
    2.77f, // K  19
    2.23f, // Ca 20
    2.09f, // Sc 21
    2.00f, // Ti 22
    1.92f, // V  23
    1.85f, // Cr 24
    1.79f, // Mn 25
    1.72f, // Fe 26
    1.67f, // Co 27
    1.62f, // Ni 28
    1.57f, // Cu 29
    1.53f, // Zn 30
    1.81f, // Ga 31
    1.52f, // Ge 32
    1.33f, // As 33
    1.22f, // Se 34
    1.12f, // Br 35
    1.03f, // Kr 36
    2.98f, // Rb 37
    2.45f, // Sr 38
    2.27f, // Y  39
    2.16f, // Zr 40
    2.08f, // Nb 41
    2.01f, // Mo 42
    1.95f, // Tc 43
    1.89f, // Ru 44
    1.83f, // Rh 45
    1.79f, // Pd 46
    1.75f, // Ag 47
    1.71f, // Cd 48
    2.00f, // In 49
    1.72f, // Sn 50
    1.53f, // Sb 51
    1.42f, // Te 52
    1.32f, // I  53
    1.24f, // Xe 54
    3.34f, // Cs 55
    2.78f, // Ba 56
    2.74f, // La 57
    2.70f, // Ce 58
    2.67f, // Pr 59
    2.64f, // Nd 60
    2.62f, // Pm 61
    2.59f, // Sm 62
    2.56f, // Eu 63
    2.54f, // Gd 64
    2.51f, // Tb 65
    2.49f, // Dy 66
    2.47f, // Ho 67
    2.45f, // Er 68
    2.42f, // Tm 69
    2.40f, // Yb 70
    2.25f, // Lu 71
    2.16f, // Hf 72
    2.09f, // Ta 73
    2.02f, // W  74
    1.97f, // Re 75
    1.92f, // Os 76
    1.87f, // Ir 77
    1.83f, // Pt 78
    1.79f, // Au 79
    1.76f, // Hg 80
    2.08f, // Tl 81
    1.81f, // Pb 82
    1.63f, // Bi 83
    1.53f, // Po 84
    1.43f, // At 85
    1.34f, // Rn 86
    2.60f, // Fr 87 (using covalent radius just to get a number here)
    2.15f, // Ra 88 (88-99 from "http://en.wikipedia.org/wiki/Atomic_radii_of_the_elements_(data_page)", which references J.C. Slater, J. Chem. Phys. 1964, 41, 3199.)
    1.95f, // Ac 89
    1.80f, // Th 90
    1.80f, // Pa 91
    1.75f, // U  92
    1.75f, // Np 93
    1.75f, // Pu 94
    1.75f, // Am 95
    1.74f, // Cm 96 (96-99 use metallic radius to just get a number)
    1.70f, // Bk 97
    1.86f, // Cf 98
    1.86f, // Es 99
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f,
    .1f
};

// From JMol / OpenBabel
/// van der Waals radii in Angstroms
const float g_vdwRadii[NUM_ELEMENTS+1] = {
    1.0,    //   0  ?
    1.200f, //   1  H
    1.400f, //   2  He
    1.820f, //   3  Li
    1.700f, //   4  Be
    2.080f, //   5  B
    1.950f, //   6  C
    1.850f, //   7  N
    1.700f, //   8  O
    1.730f, //   9  F
    1.540f, //  10  Ne
    2.270f, //  11  Na
    1.730f, //  12  Mg
    2.050f, //  13  Al
    2.100f, //  14  Si
    2.080f, //  15  P
    2.000f, //  16  S
    1.970f, //  17  Cl
    1.880f, //  18  Ar
    2.750f, //  19  K
    1.973f, //  20  Ca
    1.700f, //  21  Sc
    1.700f, //  22  Ti
    1.700f, //  23  V
    1.700f, //  24  Cr
    1.700f, //  25  Mn
    1.700f, //  26  Fe
    1.700f, //  27  Co
    1.630f, //  28  Ni
    1.400f, //  29  Cu
    1.390f, //  30  Zn
    1.870f, //  31  Ga
    1.700f, //  32  Ge
    1.850f, //  33  As
    1.900f, //  34  Se
    2.100f, //  35  Br
    2.020f, //  36  Kr
    1.700f, //  37  Rb
    1.700f, //  38  Sr
    1.700f, //  39  Y
    1.700f, //  40  Zr
    1.700f, //  41  Nb
    1.700f, //  42  Mo
    1.700f, //  43  Tc
    1.700f, //  44  Ru
    1.700f, //  45  Rh
    1.630f, //  46  Pd
    1.720f, //  47  Ag
    1.580f, //  48  Cd
    1.930f, //  49  In
    2.170f, //  50  Sn
    2.200f, //  51  Sb
    2.060f, //  52  Te
    2.150f, //  53  I
    2.160f, //  54  Xe
    1.700f, //  55  Cs
    1.700f, //  56  Ba
    1.700f, //  57  La
    1.700f, //  58  Ce
    1.700f, //  59  Pr
    1.700f, //  60  Nd
    1.700f, //  61  Pm
    1.700f, //  62  Sm
    1.700f, //  63  Eu
    1.700f, //  64  Gd
    1.700f, //  65  Tb
    1.700f, //  66  Dy
    1.700f, //  67  Ho
    1.700f, //  68  Er
    1.700f, //  69  Tm
    1.700f, //  70  Yb
    1.700f, //  71  Lu
    1.700f, //  72  Hf
    1.700f, //  73  Ta
    1.700f, //  74  W
    1.700f, //  75  Re
    1.700f, //  76  Os
    1.700f, //  77  Ir
    1.720f, //  78  Pt
    1.660f, //  79  Au
    1.550f, //  80  Hg
    1.960f, //  81  Tl
    2.020f, //  82  Pb
    1.700f, //  83  Bi
    1.700f, //  84  Po
    1.700f, //  85  At
    1.700f, //  86  Rn
    1.700f, //  87  Fr
    1.700f, //  88  Ra
    1.700f, //  89  Ac
    1.700f, //  90  Th
    1.700f, //  91  Pa
    1.860f, //  92  U
    1.700f, //  93  Np
    1.700f, //  94  Pu
    1.700f, //  95  Am
    1.700f, //  96  Cm
    1.700f, //  97  Bk
    1.700f, //  98  Cf
    1.700f, //  99  Es
    1.700f, // 100  Fm
    1.700f, // 101  Md
    1.700f, // 102  No
    1.700f, // 103  Lr
    1.700f, // 104  Rf
    1.700f, // 105  Db
    1.700f, // 106  Sg
    1.700f, // 107  Bh
    1.700f, // 108  Hs
    1.700f, // 109  Mt
};

const char * g_elementNames[NUM_ELEMENTS+1] = {
    "?",    //   0  ?
    "H",    //   1  H
    "He",   //   2  He
    "Li",   //   3  Li
    "Be",   //   4  Be
    "B",    //   5  B
    "C",    //   6  C
    "N",    //   7  N
    "O",    //   8  O
    "F",    //   9  F
    "Ne",   //  10  Ne
    "Na",   //  11  Na
    "Mg",   //  12  Mg
    "Al",   //  13  Al
    "Si",   //  14  Si
    "P",    //  15  P
    "S",    //  16  S
    "Cl",   //  17  Cl
    "Ar",   //  18  Ar
    "K",    //  19  K
    "Ca",   //  20  Ca
    "Sc",   //  21  Sc
    "Ti",   //  22  Ti
    "V",    //  23  V
    "Cr",   //  24  Cr
    "Mn",   //  25  Mn
    "Fe",   //  26  Fe
    "Co",   //  27  Co
    "Ni",   //  28  Ni
    "Cu",   //  29  Cu
    "Zn",   //  30  Zn
    "Ga",   //  31  Ga
    "Ge",   //  32  Ge
    "As",   //  33  As
    "Se",   //  34  Se
    "Br",   //  35  Br
    "Kr",   //  36  Kr
    "Rb",   //  37  Rb
    "Sr",   //  38  Sr
    "Y",    //  39  Y
    "Zr",   //  40  Zr
    "Nb",   //  41  Nb
    "Mo",   //  42  Mo
    "Tc",   //  43  Tc
    "Ru",   //  44  Ru
    "Rh",   //  45  Rh
    "Pd",   //  46  Pd
    "Ag",   //  47  Ag
    "Cd",   //  48  Cd
    "In",   //  49  In
    "Sn",   //  50  Sn
    "Sb",   //  51  Sb
    "Te",   //  52  Te
    "I",    //  53  I
    "Xe",   //  54  Xe
    "Cs",   //  55  Cs
    "Ba",   //  56  Ba
    "La",   //  57  La
    "Ce",   //  58  Ce
    "Pr",   //  59  Pr
    "Nd",   //  60  Nd
    "Pm",   //  61  Pm
    "Sm",   //  62  Sm
    "Eu",   //  63  Eu
    "Gd",   //  64  Gd
    "Tb",   //  65  Tb
    "Dy",   //  66  Dy
    "Ho",   //  67  Ho
    "Er",   //  68  Er
    "Tm",   //  69  Tm
    "Yb",   //  70  Yb
    "Lu",   //  71  Lu
    "Hf",   //  72  Hf
    "Ta",   //  73  Ta
    "W",    //  74  W
    "Re",   //  75  Re
    "Os",   //  76  Os
    "Ir",   //  77  Ir
    "Pt",   //  78  Pt
    "Au",   //  79  Au
    "Hg",   //  80  Hg
    "Tl",   //  81  Tl
    "Pb",   //  82  Pb
    "Bi",   //  83  Bi
    "Po",   //  84  Po
    "At",   //  85  At
    "Rn",   //  86  Rn
    "Fr",   //  87  Fr
    "Ra",   //  88  Ra
    "Ac",   //  89  Ac
    "Th",   //  90  Th
    "Pa",   //  91  Pa
    "U",    //  92  U
    "Np",   //  93  Np
    "Pu",   //  94  Pu
    "Am",   //  95  Am
    "Cm",   //  96  Cm
    "Bk",   //  97  Bk
    "Cf",   //  98  Cf
    "Es",   //  99  Es
    "Fm",   // 100  Fm
    "Md",   // 101  Md
    "No",   // 102  No
    "Lr",   // 103  Lr
    "Rf",   // 104  Rf
    "Db",   // 105  Db
    "Sg",   // 106  Sg
    "Bh",   // 107  Bh
    "Hs",   // 108  Hs
    "Mt",   // 109  Mt
};
