#ifndef __RP_TYPES_H__
#define __RP_TYPES_H__

#include <vector>

#define HEADER_SIZE 16

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

extern const char* RpObjectTypes[];
extern const char* RpCurrentVersion[];

//extern const int HEADER_SIZE;

/*
extern const char* RpGrid1d_current_version;
extern const char* RpGrid2d_current_version;
extern const char* RpMesh3d_current_version;
extern const char* RpField_current_version;
extern const char* RpCurve_current_version;
extern const char* RpElement_current_version;
extern const char* RpNode2d_current_version;
extern const char* RpNode3d_current_version;
*/

extern void readRpHeader(const char* buf, string& ver, int& nbytes);
extern void writeRpHeader(char* buf, const char* ver, int nbytes);
extern void readString(const char* buf, string& str);
extern void readString(const char* buf, string& str, int len);
extern void writeString(char* buf, string& str);
extern void writeString(char* buf, const char* str, int len);
extern void readInt(const char* buf, int& val);
extern void writeInt(char* buf, int val);
extern void readArrayInt(const char* buf, vector<int>& data, int& npts);
extern void writeArrayInt(char* buf, vector<int>& data, int npts);
extern void readArrayDouble(const char* buf, vector<double>& data, int& npts);
extern void writeArrayDouble(char* buf, vector<double>& data, int npts);

#endif
