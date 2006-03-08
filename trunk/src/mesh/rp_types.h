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

extern const char* RpObjectTypes[];

extern const int HEADER_SIZE;

extern const char* RpGrid1d_current_version;
extern const char* RpGrid2d_current_version;
extern const char* RpMesh3d_current_version;
extern const char* RpField_current_version;
extern const char* RpCurve_current_version;
extern const char* RpElement_current_version;
extern const char* RpNode2d_current_version;
extern const char* RpNode3d_current_version;

#endif
