#ifndef __RP_OBJECT_TYPES_H__
#define __RP_OBJECT_TYPES_H__


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

#endif
