

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

typedef struct {
    int from, to;
} ConnectKey;

typedef struct {
    int lineNum;
    char atomName[6];			/* Atom name.  */
    char symbolName[3];			/* Symbol name.  */
    char residueName[4];		/* Residue name. */
    int serial;				/* Serial # in PDB file. */
    double x, y, z;			/* Coordinates of atom. */
    int number;				/* Atomic number */
} PdbAtom;

static int lineNum = 0;
static const char *
Itoa(int number) 
{
    static char buf[200];
    sprintf(buf, "%d", number);
    return buf;
}

static INLINE const char *
SkipSpaces(const char *string) 
{
    while (isspace(*string)) {
	string++;
    }
    return string;
}

static int
IsSpaces(const char *string) 
{
    const char *p;
    for (p = string; *p != '\0'; p++) {
	if (!isspace(*p)) {
	    return 0;
	}
    }
    return 1;
}

static INLINE const char *
GetLine(const char **stringPtr, const char *endPtr, int *lengthPtr) 
{
    const char *line;
    const char *p;

    line = SkipSpaces(*stringPtr);
    for (p = line; p < endPtr; p++) {
	if (*p == '\n') {
	    *stringPtr = p + 1;
	    *lengthPtr = p - line;
	    return line;
	}
    }
    *stringPtr = p;
    return line;
}

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
GetAtomicNumber(Tcl_Interp *interp, PdbAtom *atomPtr)
{
    const char **p;
    char name[3], *namePtr;
    int elemIndex, symbolIndex;

    symbolIndex = elemIndex = -1;

    /* 
     * The atomic symbol may be found the atom name in various locations
     *	"C   "
     *  " C  "
     *  "  C "
     "  "   C"
     "  "C 12"
     "  " C12"
     */
    if ((atomPtr->atomName[0] == ' ') && (atomPtr->atomName[1] == ' ')) {
	name[0] = atomPtr->atomName[2];
	name[1] = atomPtr->atomName[3];
    } else {
	name[0] = atomPtr->atomName[0];
	name[1] = atomPtr->atomName[1];
    }
    name[2] = '\0';
    if ((name[0] != ' ') || (name[1] != ' ')) {
	namePtr = name;
	if (name[0] == ' ') {
	    namePtr++;
	}
	if (name[1] == ' ') {
	    name[1] = '\0';
	}
	for (p = symbolNames; *p != NULL; p++) {
	    if (strcasecmp(namePtr, *p) == 0) {
		elemIndex = (p - symbolNames) + 1;
		break;
	    }
	}
    }
    name[0] = atomPtr->symbolName[0];
    name[1] = atomPtr->symbolName[1];
    name[2] = '\0';
    if (isdigit(name[1])) {
	sscanf(name, "%d", &atomPtr->number);
	return TCL_OK;
    }
    if ((name[0] != ' ') || (name[1] != ' ')) {
	namePtr = name;
	if (name[0] == ' ') {
	    namePtr++;
	}
	if (name[1] == ' ') {
	    name[1] = '\0';
	}
	for (p = symbolNames; *p != NULL; p++) {
	    if (strcasecmp(namePtr, *p) == 0) {
		symbolIndex = (p - symbolNames) + 1;
		break;
	    }
	}
    }
    if (symbolIndex > 0) {
	if (elemIndex < 0) {
	    atomPtr->number = symbolIndex;
	    return TCL_OK;
	}
	if (symbolIndex != elemIndex) {
	    fprintf(stderr, "atomName %s and symbolName %s don't match\n",
		    atomPtr->atomName, atomPtr->symbolName);
	    atomPtr->number = symbolIndex;
	    return TCL_OK;
	}
	atomPtr->number = elemIndex;
	return TCL_OK;
    } else if (elemIndex > 0) {
	atomPtr->number = elemIndex;
	return TCL_OK;
    }
    Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
		     ": bad atom \"", atomPtr->atomName, "\" or element \"", 
		     atomPtr->symbolName, "\"", (char *)NULL);
    return TCL_ERROR;
}

static int 
SerialToIndex(Tcl_Interp *interp, Tcl_HashTable *tablePtr, const char *string,
	      int *indexPtr)
{
    int serial;
    long lserial, lindex;
    Tcl_HashEntry *hPtr;

    string = SkipSpaces(string);
    if (Tcl_GetInt(interp, string, &serial) != TCL_OK) {
	Tcl_AppendResult(interp, ": line ", Itoa(lineNum), 
		": invalid serial number \"", string, 
		"\" in CONECT record", (char *)NULL);
	return TCL_ERROR;
    }
    lserial = (long)serial;
    hPtr = Tcl_FindHashEntry(tablePtr, (char *)lserial);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "serial number \"", string, 
			 "\" not found in table", (char *)NULL);
	return TCL_ERROR;
    }
    lindex = (long)Tcl_GetHashValue(hPtr);
    *indexPtr = (int)lindex;
    return TCL_OK;
}

static int
GetCoordinates(Tcl_Interp *interp, const char *line, PdbAtom *atomPtr)
{
    char buf[200];

    strncpy(buf, line + 30, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &atomPtr->x) != TCL_OK) {
	Tcl_AppendResult(interp, "bad x-coordinate \"", buf,
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    strncpy(buf, line + 38, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &atomPtr->y) != TCL_OK) {
	Tcl_AppendResult(interp, "bad y-coordinate \"", buf,
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    strncpy(buf, line + 46, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &atomPtr->z) != TCL_OK) {
	Tcl_AppendResult(interp, "bad z-coordinate \"", buf,
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* 
 *  PdbToVtk string
 */
static int
PdbToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv) 
{
    Tcl_Obj *objPtr, *pointsObjPtr, *atomsObjPtr, *verticesObjPtr;
    const char *p, *pend;
    char *string;
    char mesg[2000];
    int length, nextIndex;
    Tcl_HashTable serialTable, conectTable;
    int i;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    int isNew;

    lineNum = nextIndex = 0;
    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
			 Tcl_GetString(objv[0]), " string\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_InitHashTable(&serialTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&conectTable, sizeof(ConnectKey) / sizeof(int));
    string = Tcl_GetStringFromObj(objv[1], &length);
    pointsObjPtr = Tcl_NewStringObj("", -1);
    atomsObjPtr = Tcl_NewStringObj("", -1);
    verticesObjPtr = Tcl_NewStringObj("", -1);
    Tcl_IncrRefCount(pointsObjPtr);
    Tcl_IncrRefCount(atomsObjPtr);
    Tcl_IncrRefCount(verticesObjPtr);
    objPtr = NULL;
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	const char *line;
	char c;
	int lineLength;

	lineLength = 0;			/* Suppress compiler warning. */
	lineNum++;
	line = GetLine(&p, pend, &lineLength);
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
	    long lserial;
	    PdbAtom atom;

	    if (lineLength < 47) {
		Tcl_AppendResult(interp, "short ATOM line \"", line, "\"", 
			(char *)NULL);
		goto error;
	    }		
	    strncpy(buf, line + 6, 5);
	    buf[5] = '\0';
	    if (Tcl_GetInt(interp, buf, &atom.serial) != TCL_OK) {
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			": bad serial number \"", buf, "\"", (char *)NULL);
		goto error;
	    }
	    lserial = (long)atom.serial;
	    hPtr = Tcl_CreateHashEntry(&serialTable, (char *)lserial, &isNew);
	    if (!isNew) {
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			": serial number \"", buf, "\" found more than once", 
			(char *)NULL);
		goto error;
	    }
	    Tcl_SetHashValue(hPtr, (long)nextIndex);
	    strncpy(atom.atomName, line + 12, 4);
	    atom.atomName[4] = '\0';
	    strncpy(atom.residueName, line + 17, 3);
	    atom.residueName[3] = '\0';

	    if (GetCoordinates(interp, line, &atom) != TCL_OK) {
		goto error;
	    }
	    atom.symbolName[0] = '\0';
	    if (lineLength >= 77) {
		atom.symbolName[0] = line[76];
		atom.symbolName[1] = line[77];
		atom.symbolName[2] = '\0';
	    }
	    if (GetAtomicNumber(interp, &atom) != TCL_OK) {
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, verticesObjPtr, 
				     Tcl_NewIntObj(1));
	    Tcl_ListObjAppendElement(interp, verticesObjPtr, 
				     Tcl_NewIntObj(atom.serial));
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atom.x));
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atom.y));
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atom.z));
	    Tcl_ListObjAppendElement(interp, atomsObjPtr, 
				     Tcl_NewIntObj(atom.number));
	    nextIndex++;
	} else if ((c == 'C') && (strncmp(line, "CONECT", 6) == 0)) {
	    int a, i, n;
	    char buf[200];

	    strncpy(buf, line + 6, 5);
	    buf[5] = '\0';
	    if (lineLength < 11) {
		char *msg;

		msg = (char *)line;
		msg[lineLength] = '\0';
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			": bad CONECT record \"", msg, "\"",
			(char *)NULL);
		goto error;
	    }
	    if (SerialToIndex(interp, &serialTable, buf, &a) != TCL_OK) {
		goto error;
	    }
	    /* Allow basic maximum of 4 connections. */
	    for (n = 11, i = 0; i < 4; i++, n += 5) {
		ConnectKey key;
		int b;
		if (n >= lineLength) {
		    break;		
		}
		strncpy(buf, line + n, 5);
		buf[5] = '\0';
		if (IsSpaces(buf)) {
		    break;		/* No more entries */
		}
		if (SerialToIndex(interp, &serialTable, buf, &b) != TCL_OK) {
		    goto error;
		}
		if (a > b) {
		    key.from = b;
		    key.to = a;
		} else {
		    key.from = a;
		    key.to = b;
		}
		Tcl_CreateHashEntry(&conectTable, (char *)&key, &isNew);
	    }
	}
    }
    objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
    Tcl_AppendToObj(objPtr, "Converted from PDB format\n", -1);
    Tcl_AppendToObj(objPtr, "ASCII\n", -1);
    Tcl_AppendToObj(objPtr, "DATASET POLYDATA\n", -1);
    sprintf(mesg, "POINTS %d float\n", serialTable.numEntries);
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, pointsObjPtr);
    sprintf(mesg, "\n");
    Tcl_AppendToObj(objPtr, mesg, -1);

    if (conectTable.numEntries > 0) {
	sprintf(mesg, "LINES %d %d\n", conectTable.numEntries, 
		conectTable.numEntries * 3);
	Tcl_AppendToObj(objPtr, mesg, -1);
	for (hPtr = Tcl_FirstHashEntry(&conectTable, &iter); hPtr != NULL;
	     hPtr = Tcl_NextHashEntry(&iter)) {
	    ConnectKey *keyPtr;

	    keyPtr = (ConnectKey *)Tcl_GetHashKey(&conectTable, hPtr);
	    sprintf(mesg, "2 %d %d\n", keyPtr->from, keyPtr->to);
	    Tcl_AppendToObj(objPtr, mesg, -1);
	}
	sprintf(mesg, "\n");
	Tcl_AppendToObj(objPtr, mesg, -1);
    }
    
    sprintf(mesg, "VERTICES %d %d\n", serialTable.numEntries, 
	    serialTable.numEntries * 2);
    Tcl_AppendToObj(objPtr, mesg, -1);
    for (i = 0; i < serialTable.numEntries; i++) {
	sprintf(mesg, " 1 %d\n", i);
	Tcl_AppendToObj(objPtr, mesg, -1);
    }
    sprintf(mesg, "POINT_DATA %d\n", serialTable.numEntries);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SCALARS element int\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "LOOKUP_TABLE default\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, atomsObjPtr);
    Tcl_DeleteHashTable(&serialTable);
    Tcl_DeleteHashTable(&conectTable);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(atomsObjPtr);
    Tcl_DecrRefCount(verticesObjPtr);
    if (objPtr != NULL) {
	Tcl_SetObjResult(interp, objPtr);
    }
    return TCL_OK;
 error:
    Tcl_DeleteHashTable(&conectTable);
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
