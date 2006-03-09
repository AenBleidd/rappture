/*
typedef enum {
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
} RpObject_enum;
*/

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

//const int HEADER_SIZE = 16;

const char* RpGrid1d_current_version = "RV-A-GRID1D";
const char* RpGrid2d_current_version = "RV-A-GRID2D";
const char* RpMesh3d_current_version = "RV-A-MESH3D";
const char* RpField_current_version = "RV-A-FIELD";
const char* RpCurve_current_version = "RV-A-CURVE";
const char* RpElement_current_version = "RV-A-ELEMENT";
const char* RpNode2d_current_version = "RV-A-NODE2D";
const char* RpNode3d_current_version = "RV-A-NODE3D";

