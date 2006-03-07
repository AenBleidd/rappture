#ifndef __RP_TYPES_H__
#define __RP_TYPES_H__


enum RpObject {
	BOOLEAN,
	NUMBER,
	STRING,
	FIELD,
	MESH3D,
	ELEMENT,
	NODE2D,
	NODE3D,
	GRID1D,
	GRID2D,
	GRID3D,
	CURVE2D
};

const char* RpObjectTypes[] = {
	"BOOLEAN",
	"NUMBER",
	"STRING",
	"FIELD",
	"MESH3D",
	"ELEMENT",
	"NODE2D",
	"NODE3D",
	"GRID1D",
	"GRID2D",
	"GRID3D",
	"CURVE2D"
};

const int HEADER_SIZE = 16;

const char* Grid1d_current_version = "RV-A-GRID1D";
const char* Grid2d_current_version = "RV-A-GRID2D";
const char* Mesh3d_current_version = "RV-A-MESH3D";
const char* Field_current_version = "RV-A-FIELD";
const char* Curve_current_version = "RV-A-CURVE";
const char* Element_current_version = "RV-A-ELEMENT";
const char* Node2d_current_version = "RV-A-NODE2D";
const char* Node3d_current_version = "RV-A-NODE3D";

#endif
