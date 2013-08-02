
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

#define BOND_NONE	(0)
#define BOND_CONECT	(1<<0)
#define BOND_COMPUTE	(1<<1)
#define BOND_BOTH	(BOND_CONECT|BOND_COMPUTE)

typedef struct {
    int from, to;
} ConnectKey;

typedef struct {
    double x, y, z;			/* Coordinates of atom. */
    int number;				/* Atomic number */
    int ordinal;			/* Index of the atom in VTK. */
} PdbAtom;

typedef struct {
    const char *symbol;			/* Atom symbol. */
    float radius;			/* Covalent radius of atom. */
} PeriodicElement;

static PeriodicElement elements[] = {
    {    "X",	    0.1f,	},	/* 0 */
    {    "H",	    0.32f,	},	/* 1 */		
    {    "He", 	    0.93f,	},	/* 2 */		
    {    "Li", 	    1.23f,	},	/* 3 */		
    {    "Be", 	    0.90f,	},	/* 4 */		
    {    "B", 	    0.82f,	},	/* 5 */		
    {    "C", 	    0.77f,	},	/* 6 */		
    {    "N", 	    0.75f,	},	/* 7 */		
    {    "O", 	    0.73f,	},	/* 8 */		
    {    "F", 	    0.72f,	},	/* 9 */		
    {    "Ne", 	    0.71f, 	},	/* 10 */	
    {    "Na", 	    1.54f, 	},	/* 11 */	
    {    "Mg", 	    1.36f, 	},	/* 12 */	
    {    "Al", 	    1.18f, 	},	/* 13 */	
    {    "Si", 	    1.11f, 	},	/* 14 */	
    {    "P", 	    1.06f, 	},	/* 15 */	
    {    "S", 	    1.02f, 	},	/* 16 */	
    {    "Cl", 	    0.99f, 	},	/* 17 */	
    {    "Ar", 	    0.98f, 	},	/* 18 */	
    {    "K", 	    2.03f, 	},	/* 19 */	
    {    "Ca", 	    1.74f, 	},	/* 20 */	
    {    "Sc", 	    1.44f, 	},	/* 21 */	
    {    "Ti", 	    1.32f, 	},	/* 22 */	
    {    "V", 	    1.22f, 	},	/* 23 */	
    {    "Cr", 	    1.18f, 	},	/* 24 */	
    {    "Mn", 	    1.17f, 	},	/* 25 */	
    {    "Fe", 	    1.17f, 	},	/* 26 */	
    {    "Co", 	    1.16f, 	},	/* 27 */	
    {    "Ni", 	    1.15f, 	},	/* 28 */	
    {    "Cu", 	    1.17f, 	},	/* 29 */	
    {    "Zn", 	    1.25f, 	},	/* 30 */	
    {    "Ga", 	    1.26f, 	},	/* 31 */	
    {    "Ge", 	    1.22f, 	},	/* 32 */	
    {    "As", 	    1.20f, 	},	/* 33 */	
    {    "Se", 	    1.16f, 	},	/* 34 */	
    {    "Br", 	    1.14f, 	},	/* 35 */	
    {    "Kr", 	    1.12f, 	},	/* 36 */	
    {    "Rb", 	    2.16f, 	},	/* 37 */	
    {    "Sr", 	    1.91f, 	},	/* 38 */	
    {    "Y", 	    1.62f, 	},	/* 39 */	
    {    "Zr", 	    1.45f, 	},	/* 40 */	
    {    "Nb", 	    1.34f, 	},	/* 41 */	
    {    "Mo", 	    1.30f, 	},	/* 42 */	
    {    "Tc", 	    1.27f, 	},	/* 43 */	
    {    "Ru", 	    1.25f, 	},	/* 44 */	
    {    "Rh", 	    1.25f, 	},	/* 45 */	
    {    "Pd", 	    1.28f, 	},	/* 46 */	
    {    "Ag", 	    1.34f, 	},	/* 47 */	
    {    "Cd", 	    1.48f, 	},	/* 48 */	
    {    "In", 	    1.44f, 	},	/* 49 */	
    {    "Sn", 	    1.41f, 	},	/* 50 */	
    {    "Sb", 	    1.40f, 	},	/* 51 */	
    {    "Te", 	    1.36f, 	},	/* 52 */	
    {    "I", 	    1.33f, 	},	/* 53 */	
    {    "Xe", 	    1.31f, 	},	/* 54 */	
    {    "Cs", 	    2.35f, 	},	/* 55 */	
    {    "Ba", 	    1.98f, 	},	/* 56 */	
    {    "La", 	    1.69f, 	},	/* 57 */	
    {    "Ce", 	    1.65f, 	},	/* 58 */	
    {    "Pr", 	    1.65f, 	},	/* 59 */	
    {    "Nd", 	    1.64f, 	},	/* 60 */	
    {    "Pm", 	    1.63f, 	},	/* 61 */	
    {    "Sm", 	    1.62f, 	},	/* 62 */	
    {    "Eu", 	    1.85f, 	},	/* 63 */	
    {    "Gd", 	    1.61f, 	},	/* 64 */	
    {    "Tb", 	    1.59f, 	},	/* 65 */	
    {    "Dy", 	    1.59f, 	},	/* 66 */	
    {    "Ho", 	    1.58f, 	},	/* 67 */	
    {    "Er", 	    1.57f, 	},	/* 68 */	
    {    "Tm", 	    1.56f, 	},	/* 69 */	
    {    "Yb", 	    1.74f, 	},	/* 70 */	
    {    "Lu", 	    1.56f, 	},	/* 71 */	
    {    "Hf", 	    1.44f, 	},	/* 72 */	
    {    "Ta", 	    1.34f, 	},	/* 73 */	
    {    "W", 	    1.30f, 	},	/* 74 */	
    {    "Re", 	    1.28f, 	},	/* 75 */	
    {    "Os", 	    1.26f, 	},	/* 76 */	
    {    "Ir", 	    1.27f, 	},	/* 77 */	
    {    "Pt", 	    1.30f, 	},	/* 78 */	
    {    "Au", 	    1.34f, 	},	/* 79 */	
    {    "Hg", 	    1.49f, 	},	/* 80 */	
    {    "Tl", 	    1.48f, 	},	/* 81 */	
    {    "Pb", 	    1.47f, 	},	/* 82 */	
    {    "Bi", 	    1.46f, 	},	/* 83 */	
    {    "Po", 	    1.46f, 	},	/* 84 */	
    {    "At", 	    1.45f, 	},	/* 85 */	
    {    "Rn", 	    1.50f, 	},	/* 86 */	
    {    "Fr", 	    2.60f, 	},	/* 87 */	
    {    "Ra", 	    2.21f, 	},	/* 88 */	
    {    "Ac", 	    2.15f, 	},	/* 89 */	
    {    "Th", 	    2.06f, 	},	/* 90 */	
    {    "Pa", 	    2.00f, 	},	/* 91 */	
    {    "U", 	    1.96f, 	},	/* 92 */	
    {    "Np", 	    1.90f, 	},	/* 93 */	
    {    "Pu", 	    1.87f, 	},	/* 94 */	
    {    "Am", 	    1.80f, 	},	/* 95 */	
    {    "Cm", 	    1.69f, 	},	/* 96 */	
    {    "Bk", 	    0.1f, 	},	/* 97 */	
    {    "Cf", 	    0.1f, 	},	/* 98 */	
    {    "Es", 	    0.1f, 	},	/* 99 */	
    {    "Fm", 	    0.1f, 	},	/* 100 */	
    {    "Md", 	    0.1f, 	},	/* 101 */	
    {    "No", 	    0.1f, 	},	/* 102 */	
    {    "Lr", 	    0.1f, 	},	/* 103 */	
    {    "Rf", 	    0.1f, 	},	/* 104 */	
    {    "Db", 	    0.1f, 	},	/* 105 */	
    {    "Sg", 	    0.1f, 	},	/* 106 */	
    {    "Bh", 	    0.1f, 	},	/* 107 */	
    {    "Hs", 	    0.1f, 	},	/* 108 */	
    {    "Mt", 	    0.1f, 	},	/* 109 */	
    {    "Ds",	    0.1f, 	},	/* 110 */	
    {    "Rg",	    0.1f, 	},	/* 111 */	
    {    "Cn",	    0.1f, 	},	/* 112 */	
    {    "Uut",	    0.1f, 	},	/* 113 */	
    {    "Fl",	    0.1f, 	},	/* 114 */	
    {    "Uup",	    0.1f, 	},	/* 115 */	
    {    "Lv", 	    0.1f, 	},	/* 116 */	
    {    "Uus",	    0.1f, 	},	/* 117 */	
    {    "Uuo",     0.1f,	}	/* 118 */	
};

static int numPeriodicElements = sizeof(elements) / sizeof(PeriodicElement);

static int lineNum = 0;

static const char *
Itoa(int number) 
{
    static char buf[200];
    sprintf(buf, "%d", number);
    return buf;
}

static void
ComputeBonds(Tcl_HashTable *atomTablePtr, Tcl_HashTable *conectTablePtr)
{
    PdbAtom **array;
    int i;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

#define TOLERANCE 0.45			/* Fuzz factor for comparing distances
					 * (in angstroms) */
    array = calloc(atomTablePtr->numEntries, sizeof(PdbAtom *));
    if (array == NULL) {
	return;
    }
    for (i = 0, hPtr = Tcl_FirstHashEntry(atomTablePtr, &iter); hPtr != NULL;
	 hPtr = Tcl_NextHashEntry(&iter), i++) {
	PdbAtom *atomPtr;

	atomPtr = Tcl_GetHashValue(hPtr);
	array[i] = atomPtr;
    }
    for (i = 0; i < atomTablePtr->numEntries; i++) {
	PdbAtom *atom1Ptr;
	double r1;
	int j;

	atom1Ptr = array[i];
        r1 = elements[atom1Ptr->number].radius;
        for (j = i+1; j < atomTablePtr->numEntries; j++) {
	    PdbAtom *atom2Ptr;
            double ds2, cut;
	    double r2;

	    atom2Ptr  = array[j];
            if ((atom2Ptr->number == 1) && (atom1Ptr->number == 1)) {
                continue;
	    }
	    r2 = elements[atom2Ptr->number].radius;
	    cut = (r1 + r2 + TOLERANCE);
	    ds2 = (((atom1Ptr->x - atom2Ptr->x) * (atom1Ptr->x - atom2Ptr->x)) +
		   ((atom1Ptr->y - atom2Ptr->y) * (atom1Ptr->y - atom2Ptr->y)) +
		   ((atom1Ptr->z - atom2Ptr->z) * (atom1Ptr->z - atom2Ptr->z)));

	    // perform distance test, but ignore pairs between atoms
	    // with nearly identical coords
	    if (ds2 < 0.001) {
		continue;
	    }
	    if (ds2 < (cut*cut)) {
		ConnectKey key;
		int isNew;

		if (atom1Ptr->ordinal > atom2Ptr->ordinal) {
		    key.from = atom2Ptr->ordinal;
		    key.to = atom1Ptr->ordinal;
		} else {
		    key.from = atom1Ptr->ordinal;
		    key.to = atom2Ptr->ordinal;
		}
		Tcl_CreateHashEntry(conectTablePtr, (char *)&key, &isNew);
            }
        }
    }
    free(array);
}

int 
GetAtomicNumber(Tcl_Interp *interp, PdbAtom *atomPtr, const char *atomName, 
		const char *symbolName, const char *residueName)
{
    char name[5], *namePtr;
    int elemIndex, symbolIndex;
    int count;
    char *p;

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
    count = 0;
    for (p = atomName; *p != '\0'; p++) {
	if (isspace(*p) || isdigit(*p)) {
	    continue;
	}
	name[count] = *p;
	count++;
    }
    name[count] = '\0';
    if (count > 0) {
	int i;

	namePtr = name;
	if (name[0] == ' ') {
	    namePtr++;
	}
	if (name[1] == ' ') {
	    name[1] = '\0';
	}
	for (i = 0; i < numPeriodicElements; i++) {
	    if (strcasecmp(namePtr, elements[i].symbol) == 0) {
		elemIndex = i;
		break;
	    }
	}
	if ((elemIndex == -1) && (count > 1)) {
	    name[1] = '\0';
	    for (i = 0; i < numPeriodicElements; i++) {
		if (strcasecmp(namePtr, elements[i].symbol) == 0) {
		    elemIndex = i;
		    break;
		}
	    }
	}
    }
    name[0] = symbolName[0];
    name[1] = symbolName[1];
    name[2] = '\0';
    if (isdigit(name[1])) {
	sscanf(name, "%d", &atomPtr->number);
	return TCL_OK;
    }
    if ((name[0] != ' ') || (name[1] != ' ')) {
	int i;

	namePtr = name;
	if (name[0] == ' ') {
	    namePtr++;
	}
	if (name[1] == ' ') {
	    name[1] = '\0';
	}
	for (i = 0; i < numPeriodicElements; i++) {
	    if (strcasecmp(namePtr, elements[i].symbol) == 0) {
		symbolIndex = i;
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
		    atomName, symbolName);
	    atomPtr->number = symbolIndex;
	    return TCL_OK;
	}
	atomPtr->number = elemIndex;
	return TCL_OK;
    } else if (elemIndex > 0) {
	atomPtr->number = elemIndex;
	return TCL_OK;
    }
    Tcl_AppendResult(interp, "line ", Itoa(lineNum), ": bad atom \"", 
		     atomName, "\" or element \"", 
		     symbolName, "\"", (char *)NULL);
    return TCL_ERROR;
}

static PdbAtom *
NewAtom(Tcl_Interp *interp, int ordinal, const char *line, int lineLength)
{
    PdbAtom *atomPtr;
    char buf[200];
    char atomName[6];			/* Atom name.  */
    char symbolName[3];			/* Symbol name.  */
    char residueName[4];		/* Residue name. */
    double value;

    atomPtr = calloc(1, sizeof(PdbAtom));
    if (atomPtr == NULL) {
	return NULL;
    }
    atomPtr->ordinal = ordinal;
    strncpy(atomName, line + 12, 4);
    atomName[4] = '\0';
    strncpy(residueName, line + 17, 3);
    residueName[3] = '\0';

    strncpy(buf, line + 30, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &value) != TCL_OK) {
	Tcl_AppendResult(interp, "bad x-coordinate \"", buf,
			 "\"", (char *)NULL);
	return NULL;
    }
    atomPtr->x = value;
    strncpy(buf, line + 38, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &value) != TCL_OK) {
	Tcl_AppendResult(interp, "bad y-coordinate \"", buf,
			 "\"", (char *)NULL);
	return NULL;
    }
    atomPtr->y = value;
    strncpy(buf, line + 46, 8);
    buf[8] = '\0';
    if (Tcl_GetDouble(interp, buf, &value) != TCL_OK) {
	Tcl_AppendResult(interp, "bad z-coordinate \"", buf,
			 "\"", (char *)NULL);
	return NULL;
    }
    atomPtr->z = value;
    symbolName[2] = symbolName[1]  = symbolName[0] = '\0';
    if (lineLength >= 77) {
	symbolName[0] = line[76];
	symbolName[1] = line[77];
	symbolName[2] = '\0';
    }
    if (GetAtomicNumber(interp, atomPtr, atomName, symbolName, residueName) 
	!= TCL_OK) {
	return NULL;
    }
    return atomPtr;
}

static void
FreeAtoms(Tcl_HashTable *tablePtr)
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    for (hPtr = Tcl_FirstHashEntry(tablePtr, &iter); hPtr != NULL;
	 hPtr = Tcl_NextHashEntry(&iter)) {
	PdbAtom *atomPtr;
	
	atomPtr = Tcl_GetHashValue(hPtr);
	free(atomPtr);
    }
    Tcl_DeleteHashTable(tablePtr);
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
    *lengthPtr = (p - line) - 1;
    *stringPtr = p;
    return line;
}


static int 
SerialToOrdinal(Tcl_Interp *interp, Tcl_HashTable *tablePtr, const char *string,
	      int *ordPtr)
{
    int serial;
    PdbAtom *atomPtr;
    long lserial;
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
    atomPtr = Tcl_GetHashValue(hPtr);
    *ordPtr = atomPtr->ordinal;
    return TCL_OK;
}

/* 
 *  PdbToVtk string ?-bonds none|both|auto|conect?
 */
static int
PdbToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv) 
{
    Tcl_Obj *objPtr, *pointsObjPtr, *atomsObjPtr;
    const char *p, *pend;
    const char *string;
    char mesg[2000];
    int length, nextOrdinal;
    Tcl_HashTable atomTable, conectTable;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    int isNew;
    int bondFlags;

    bondFlags = BOND_NONE;
    lineNum = nextOrdinal = 0;
    if ((objc != 2) && (objc != 4)) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		Tcl_GetString(objv[0]), 
		" string ?-bonds none|both|auto|conect\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 4) {
	const char *string;
	char c;

	string = Tcl_GetString(objv[2]);
	if ((string[0] != '-') || (strcmp(string, "-bonds") != 0)) {
	    Tcl_AppendResult(interp, "bad switch \"", string, 
			     "\": should be \"-bonds\"", (char *)NULL);
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[3]);
	c = string[0];
	if ((c == 'n') && (strcmp(string, "none") == 0)) {
	    bondFlags = BOND_NONE;
	} else if ((c == 'b') && (strcmp(string, "both") == 0)) {
	    bondFlags = BOND_BOTH;
	} else if ((c == 'a') && (strcmp(string, "auto") == 0)) {
	    bondFlags = BOND_COMPUTE;
	} else if ((c == 'c') && (strcmp(string, "conect") == 0)) {
	    bondFlags = BOND_CONECT;
	} else {
	    Tcl_AppendResult(interp, "bad value \"", string, 
		"\" for -bonds switch: should be none, both, manual, or auto",
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    Tcl_InitHashTable(&atomTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&conectTable, sizeof(ConnectKey) / sizeof(int));
    string = Tcl_GetStringFromObj(objv[1], &length);
    pointsObjPtr = Tcl_NewStringObj("", -1);
    atomsObjPtr = Tcl_NewStringObj("", -1);
    Tcl_IncrRefCount(pointsObjPtr);
    Tcl_IncrRefCount(atomsObjPtr);
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
	    int serialNum;
	    long lserial;
	    PdbAtom *atomPtr;

	    if (lineLength < 47) {
		char *msg;

		msg = (char *)line;
		msg[lineLength] = '\0';
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			" short ATOM line \"", line, "\"", (char *)NULL);
		return TCL_ERROR;
	    }		
	    strncpy(buf, line + 6, 5);
	    buf[5] = '\0';
	    if (Tcl_GetInt(interp, buf, &serialNum) != TCL_OK) {
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			": bad serial number \"", buf, "\"", (char *)NULL);
		goto error;
	    }
	    lserial = (long)serialNum;
	    hPtr = Tcl_CreateHashEntry(&atomTable, (char *)lserial, &isNew);
	    if (!isNew) {
		Tcl_AppendResult(interp, "line ", Itoa(lineNum), 
			": serial number \"", buf, "\" found more than once", 
			(char *)NULL);
		goto error;
	    }
	    atomPtr = NewAtom(interp, nextOrdinal, line, lineLength);
	    if (atomPtr == NULL) {
		goto error;
	    }
	    Tcl_SetHashValue(hPtr, atomPtr);
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atomPtr->x));
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atomPtr->y));
	    Tcl_ListObjAppendElement(interp, pointsObjPtr, 
				     Tcl_NewDoubleObj(atomPtr->z));
	    Tcl_ListObjAppendElement(interp, atomsObjPtr, 
				     Tcl_NewIntObj(atomPtr->number));
	    nextOrdinal++;
	} else if ((c == 'C') && (strncmp(line, "CONECT", 6) == 0)) {
	    int a, i, n;
	    char buf[200];

	    if ((bondFlags & BOND_CONECT) == 0) {
		continue;
	    }
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
	    if (SerialToOrdinal(interp, &atomTable, buf, &a) != TCL_OK) {
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
		if (SerialToOrdinal(interp, &atomTable, buf, &b) != TCL_OK) {
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
    if (bondFlags & BOND_COMPUTE) {
	ComputeBonds(&atomTable, &conectTable);
    }
    objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
    Tcl_AppendToObj(objPtr, "Converted from PDB format\n", -1);
    Tcl_AppendToObj(objPtr, "ASCII\n", -1);
    Tcl_AppendToObj(objPtr, "DATASET POLYDATA\n", -1);
    sprintf(mesg, "POINTS %d float\n", atomTable.numEntries);
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
    sprintf(mesg, "POINT_DATA %d\n", atomTable.numEntries);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SCALARS element int\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "LOOKUP_TABLE default\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, atomsObjPtr);
    FreeAtoms(&atomTable);
    Tcl_DeleteHashTable(&conectTable);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(atomsObjPtr);
    if (objPtr != NULL) {
	Tcl_SetObjResult(interp, objPtr);
    }
    return TCL_OK;
 error:
    Tcl_DeleteHashTable(&conectTable);
    FreeAtoms(&atomTable);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(atomsObjPtr);
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
