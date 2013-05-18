

/* 
 * ----------------------------------------------------------------------
 *  RpPdbToVtk - 
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "tcl.h"


static INLINE char *
SkipSpaces(char *string) 
{
    while (isspace(*string)) {
	string++;
    }
    return string;
}

static INLINE char *
GetLine(char **stringPtr, const char *endPtr) 
{
    char *line, *p;

    line = SkipSpaces(*stringPtr);
    for (p = line; p < endPtr; p++) {
	if (*p == '\n') {
	    *stringPtr = p + 1;
	    return line;
	}
    }
    *stringPtr = p;
    return line;
}

static const char *elemNames[] = {
    "Hydrogen",				/* 1 */		
    "Helium",				/* 2 */		
    "Lithium",				/* 3 */		
    "Beryllium",			/* 4 */		
    "Boron",				/* 5 */		
    "Carbon",				/* 6 */		
    "Nitrogen",				/* 7 */		
    "Oxygen",				/* 8 */		
    "Fluorine",				/* 9 */		
    "Neon",				/* 10 */	
    "Sodium",				/* 11 */	
    "Magnesium",			/* 12 */	
    "Aluminium",			/* 13 */	
    "Silicon",				/* 14 */	
    "Phosphorus",			/* 15 */	
    "Sulfur",				/* 16 */	
    "Chlorine",				/* 17 */	
    "Argon",				/* 18 */	
    "Potassium",			/* 19 */	
    "Calcium",				/* 20 */	
    "Scandium",				/* 21 */	
    "Titanium",				/* 22 */	
    "Vanadium",				/* 23 */	
    "Chromium",				/* 24 */	
    "Manganese",			/* 25 */	
    "Iron",				/* 26 */	
    "Cobalt",				/* 27 */	
    "Nickel",				/* 28 */	
    "Copper",				/* 29 */	
    "Zinc",				/* 30 */	
    "Gallium",				/* 31 */	
    "Germanium",		/* 32 */	
    "Arsenic",		/* 33 */	
    "Selenium",		/* 34 */	
    "Bromine",		/* 35 */	
    "Krypton",		/* 36 */	
    "Rubidium",		/* 37 */	
    "Strontium",		/* 38 */	
    "Yttrium",		/* 39 */	
    "Zirconium",		/* 40 */	
    "Niobium",		/* 41 */	
    "Molybdenum",		/* 42 */	
    "Technetium",		/* 43 */	
    "Ruthenium",		/* 44 */	
    "Rhodium",		/* 45 */	
    "Palladium",		/* 46 */	
    "Silver",		/* 47 */	
    "Cadmium",		/* 48 */	
    "Indium",		/* 49 */	
    "Tin",			/* 50 */	
    "Antimony",		/* 51 */	
    "Tellurium",		/* 52 */	
    "Iodine",		/* 53 */	
    "Xenon",		/* 54 */	
    "Cesium",		/* 55 */	
    "Barium",		/* 56 */	
    "Lanthanum",		/* 57 */	
    "Cerium",		/* 58 */	
    "Praseodymium",		/* 59 */	
    "Neodymium",		/* 60 */	
    "Promethium",		/* 61 */	
    "Samarium",		/* 62 */	
    "Europium",		/* 63 */	
    "Gadolinium",		/* 64 */	
    "Terbium",		/* 65 */	
    "Dysprosium",		/* 66 */	
    "Holmium",		/* 67 */	
    "Erbium",		/* 68 */	
    "Thulium",		/* 69 */	
    "Ytterbium",		/* 70 */	
    "Lutetium",		/* 71 */	
    "Hafnium",		/* 72 */	
    "Tantalum",		/* 73 */	
    "Tungsten",		/* 74 */	
    "Rhenium",		/* 75 */	
    "Osmium",		/* 76 */	
    "Iridium",		/* 77 */	
    "Platinum",		/* 78 */	
    "Gold",			/* 79 */	
    "Mercury",		/* 80 */	
    "Thallium",		/* 81 */	
    "Lead",			/* 82 */	
    "Bismuth",		/* 83 */	
    "Polonium",		/* 84 */	
    "Astatine",		/* 85 */	
    "Radon",		/* 86 */	
    "Francium",		/* 87 */	
    "Radium",		/* 88 */	
    "Actinium",		/* 89 */	
    "Thorium",		/* 90 */	
    "Protactinium",		/* 91 */	
    "Uranium",		/* 92 */	
    "Neptunium",		/* 93 */	
    "Plutonium",		/* 94 */	
    "Americium",		/* 95 */	
    "Curium",		/* 96 */	
    "Berkelium",		/* 97 */	
    "Californium",		/* 98 */	
    "Einsteinium",		/* 99 */	
    "Fermium",		/* 100 */	
    "Mendelevium",		/* 101 */	
    "Nobelium",		/* 102 */	
    "Lawrencium",		/* 103 */	
    "Rutherfordium",	/* 104 */	
    "Dubnium",		/* 105 */	
    "Seaborgium",		/* 106 */	
    "Bohrium",		/* 107 */	
    "Hassium",		/* 108 */	
    "Meitnerium",		/* 109 */	
    "Darmstadtium",		/* 110 */	
    "Roentgenium",		/* 111 */	
    "Copernicium",		/* 112 */	
    "Ununtrium",		/* 113 */	
    "Flerovium",		/* 114 */	
    "Ununpentium",		/* 115 */	
    "Livermorium",		/* 116 */	
    "Ununseptium",		/* 117 */	
    "Ununoctium",		/* 118 */	
    NULL
};

static const char *symbolNames[] = {
    "H",			/* 1 */		
    "He", 			/* 2 */		
    "Li", 			/* 3 */		
    "Be", 			/* 4 */		
    "B", 			/* 5 */		
    "C", 			/* 6 */		
    "N", 			/* 7 */		
    "O", 			/* 8 */		
    "F", 			/* 9 */		
    "Ne", 			/* 10 */	
    "Na", 			/* 11 */	
    "Mg", 			/* 12 */	
    "Al", 			/* 13 */	
    "Si", 			/* 14 */	
    "P", 			/* 15 */	
    "S", 			/* 16 */	
    "Cl", 			/* 17 */	
    "Ar", 			/* 18 */	
    "K", 			/* 19 */	
    "Ca", 			/* 20 */	
    "Sc", 			/* 21 */	
    "Ti", 			/* 22 */	
    "V", 			/* 23 */	
    "Cr", 			/* 24 */	
    "Mn", 			/* 25 */	
    "Fe", 			/* 26 */	
    "Co", 			/* 27 */	
    "Ni", 			/* 28 */	
    "Cu", 			/* 29 */	
    "Zn", 			/* 30 */	
    "Ga", 			/* 31 */	
    "Ge", 			/* 32 */	
    "As", 			/* 33 */	
    "Se", 			/* 34 */	
    "Br", 			/* 35 */	
    "Kr", 			/* 36 */	
    "Rb", 			/* 37 */	
    "Sr", 			/* 38 */	
    "Y", 			/* 39 */	
    "Zr", 			/* 40 */	
    "Nb", 			/* 41 */	
    "Mo", 			/* 42 */	
    "Tc", 			/* 43 */	
    "Ru", 			/* 44 */	
    "Rh", 			/* 45 */	
    "Pd", 			/* 46 */	
    "Ag", 			/* 47 */	
    "Cd", 			/* 48 */	
    "In", 			/* 49 */	
    "Sn", 			/* 50 */	
    "Sb", 			/* 51 */	
    "Te", 			/* 52 */	
    "I", 			/* 53 */	
    "Xe", 			/* 54 */	
    "Cs", 			/* 55 */	
    "Ba", 			/* 56 */	
    "La", 			/* 57 */	
    "Ce", 			/* 58 */	
    "Pr", 			/* 59 */	
    "Nd", 			/* 60 */	
    "Pm", 			/* 61 */	
    "Sm", 			/* 62 */	
    "Eu", 			/* 63 */	
    "Gd", 			/* 64 */	
    "Tb", 			/* 65 */	
    "Dy", 			/* 66 */	
    "Ho", 			/* 67 */	
    "Er", 			/* 68 */	
    "Tm", 			/* 69 */	
    "Yb", 			/* 70 */	
    "Lu", 			/* 71 */	
    "Hf", 			/* 72 */	
    "Ta", 			/* 73 */	
    "W", 			/* 74 */	
    "Re", 			/* 75 */	
    "Os", 			/* 76 */	
    "Ir", 			/* 77 */	
    "Pt", 			/* 78 */	
    "Au", 			/* 79 */	
    "Hg", 			/* 80 */	
    "Tl", 			/* 81 */	
    "Pb", 			/* 82 */	
    "Bi", 			/* 83 */	
    "Po", 			/* 84 */	
    "At", 			/* 85 */	
    "Rn", 			/* 86 */	
    "Fr", 			/* 87 */	
    "Ra", 			/* 88 */	
    "Ac", 			/* 89 */	
    "Th", 			/* 90 */	
    "Pa", 			/* 91 */	
    "U", 			/* 92 */	
    "Np", 			/* 93 */	
    "Pu", 			/* 94 */	
    "Am", 			/* 95 */	
    "Cm", 			/* 96 */	
    "Bk", 			/* 97 */	
    "Cf", 			/* 98 */	
    "Es", 			/* 99 */	
    "Fm", 			/* 100 */	
    "Md", 			/* 101 */	
    "No", 			/* 102 */	
    "Lr", 			/* 103 */	
    "Rf", 			/* 104 */	
    "Db", 			/* 105 */	
    "Sg", 			/* 106 */	
    "Bh", 			/* 107 */	
    "Hs", 			/* 108 */	
    "Mt", 			/* 109 */	
    "Ds", 			/* 110 */	
    "Rg", 			/* 111 */	
    "Cn", 			/* 112 */	
    "Uut", 			/* 113 */	
    "Fl", 			/* 114 */	
    "Uup", 			/* 115 */	
    "Lv", 			/* 116 */	
    "Uus", 			/* 117 */	
    "Uuo", 			/* 118 */	
    NULL
};

int 
VerifyElement(const char *elemName, const char *symbolName)
{
    const char **p;
    int elemIndex, symbolIndex;

    symbolIndex = elemIndex = -1;

    for (p = symbolNames; *p != NULL; p++) {
	if (strcasecmp(elemName, *p) == 0) {
	    elemIndex = (p - symbolNames) + 1;
	    break;
	}
    }
    for (p = symbolNames; *p != NULL; p++) {
	if (strcasecmp(symbolName, *p) == 0) {
	    symbolIndex = (p - symbolNames) + 1;
	    break;
	}
    }
    if (symbolIndex > 0) {
	if (elemIndex < 0) {
	    return symbolIndex;
	}
	if (symbolIndex != elemIndex) {
	    return -1;
	}
	return elemIndex;
    } else if (elemIndex > 0) {
	return elemIndex;
    }
    return -1;
}

/* 
 *  PdbToVtk string
 */

static int
PdbToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv) 
{
    Tcl_Obj *objPtr, *pointsObjPtr, *atomsObjPtr, *verticesObjPtr;
    char *p, *pend;
    char *string;
    char mesg[2000];
    int length, numAtoms, numConnections;
    Tcl_HashTable serialTable;
    int i;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    int isNew;

    numConnections = numAtoms = 0;
    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
			 Tcl_GetString(objv[0]), " string\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_InitHashTable(&serialTable, TCL_ONE_WORD_KEYS);
    string = Tcl_GetStringFromObj(objv[1], &length);
    pointsObjPtr = Tcl_NewStringObj("", -1);
    atomsObjPtr = Tcl_NewStringObj("", -1);
    verticesObjPtr = Tcl_NewStringObj("", -1);
    Tcl_IncrRefCount(pointsObjPtr);
    Tcl_IncrRefCount(atomsObjPtr);
    Tcl_IncrRefCount(verticesObjPtr);
    objPtr = NULL;
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	char *line;
	char c;

	line = GetLine(&p, pend);
        if (line >= pend) {
	    break;			/* EOF */
	}
        if ((line[0] == '#') || (line[0] == '\n')) {
	    continue;			/* Skip blank or comment lines. */
	}
	c = line[0];
	
	if (((c == 'A') && (strncmp(line, "ATOM  ", 6) == 0)) ||
	    ((c == 'H') && (strncmp(line, "HETATM", 6) == 0))) {
	    char buf[200];
	    char atomName[6];
	    char symbolName[3];
	    int serial;
	    double x, y, z;
	    int atom;

	    strncpy(buf, line + 6, 5);
	    buf[5] = '\0';
	    if (Tcl_GetInt(interp, buf, &serial) != TCL_OK) {
		Tcl_AppendResult(interp, "bad serial number \"", buf,
				 "\"", (char *)NULL);
		goto error;
	    }
	    hPtr = Tcl_CreateHashEntry(&serialTable, (char *)serial, &isNew);
	    if (!isNew) {
		Tcl_AppendResult(interp, "serial number \"", buf,
				 "\" found more than once", (char *)NULL);
		goto error;
	    }
	    Tcl_SetHashValue(hPtr, numAtoms);
	    Tcl_ListObjAppendElement(interp, verticesObjPtr, 
				     Tcl_NewIntObj(1));
	    Tcl_ListObjAppendElement(interp, verticesObjPtr, 
				     Tcl_NewIntObj(serial));

	    strncpy(atomName, line + 12, 4);
	    atomName[4] = '\0';
	    
	    strncpy(buf, line + 30, 8);
	    buf[8] = '\0';
	    if (Tcl_GetDouble(interp, buf, &x) != TCL_OK) {
		Tcl_AppendResult(interp, "bad x-coordinate \"", buf,
				 "\"", (char *)NULL);
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, Tcl_NewDoubleObj(x));
	    strncpy(buf, line + 38, 8);
	    buf[8] = '\0';
	    if (Tcl_GetDouble(interp, buf, &y) != TCL_OK) {
		Tcl_AppendResult(interp, "bad y-coordinate \"", buf,
				 "\"", (char *)NULL);
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, Tcl_NewDoubleObj(y));
	    strncpy(buf, line + 46, 8);
	    buf[8] = '\0';
	    if (Tcl_GetDouble(interp, buf, &z) != TCL_OK) {
		Tcl_AppendResult(interp, "bad z-coordinate \"", buf,
				 "\"", (char *)NULL);
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, Tcl_NewDoubleObj(z));
	    strncpy(symbolName, line + 77, 2);
	    symbolName[2] = '\0';

	    atom = VerifyElement(SkipSpaces(atomName), SkipSpaces(symbolName));
	    if (atom < 0) {
		Tcl_AppendResult(interp, "bad atom/element \"", atomName,
				 "\" \"", symbolName, "\"", (char *)NULL);
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, atomsObjPtr, Tcl_NewIntObj(atom));
	    numAtoms++;
	} else if ((c == 'C') && (strncmp(line, "CONECT", 6) == 0)) {
	    numConnections++;
	}
    }
    objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
    Tcl_AppendToObj(objPtr, "Converted from PDB file\n", -1);
    Tcl_AppendToObj(objPtr, "ASCII\n", -1);
    Tcl_AppendToObj(objPtr, "DATASET POLYDATA\n", -1);
    sprintf(mesg, "POINTS %d float\n", numAtoms);
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, pointsObjPtr);
    sprintf(mesg, "\n");
    Tcl_AppendToObj(objPtr, mesg, -1);

#ifdef notdef
    if (numConnections > 0) {
	sprintf(mesg, "LINES %d %d\n", numConnections, numConnections * 3);
	Tcl_AppendToObj(objPtr, mesg, -1);
	for (hPtr = Tcl_FirstHashEntry(&connectTable, &iter); hPtr != NULL;
	     hPtr = Tcl_NextHashEntry(&iter)) {
	    connectPtr = Tcl_GetHashEntry(hPtr);
	    sprintf(mesg, "2 %d %d\n", connectPtr->from, connectPtr->to);
	}
    }
    Tcl_AppendObjToObj(objPtr, linesObjPtr);
    sprintf(mesg, "\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
#endif
    
    sprintf(mesg, "VERTICES %d %d\n", numAtoms, numAtoms * 2);
    Tcl_AppendToObj(objPtr, mesg, -1);
    for (i = 0; i < numAtoms; i++) {
	sprintf(mesg, " 1 %d\n", i);
	Tcl_AppendToObj(objPtr, mesg, -1);
    }
    sprintf(mesg, "POINT_DATA %d\n", numAtoms);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SCALARS element int\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "LOOKUP_TABLE default\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, atomsObjPtr);
    Tcl_DeleteHashTable(&serialTable);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(atomsObjPtr);
    Tcl_DecrRefCount(verticesObjPtr);
    if (objPtr != NULL) {
	Tcl_SetObjResult(interp, objPtr);
    }
    return TCL_OK;
 error:
    Tcl_DeleteHashTable(&serialTable);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(atomsObjPtr);
    Tcl_DecrRefCount(verticesObjPtr);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  RpPdbToVtk_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "PdbToVtk" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpPdbToVtk_Init(Tcl_Interp *interp)
{
    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::PdbToVtk", PdbToVtkCmd,
        NULL, NULL);
    return TCL_OK;
}
