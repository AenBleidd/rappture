//
// class for 3D grid
//

#include "grid3d.h"

// Constructors
RpGrid3d::RpGrid3d()
{
}

RpGrid3d::RpGrid3d(const char* name)
{
	objectName(name);
}

RpGrid3d::RpGrid3d(int npoints)
{
	m_data.reserve(npoints*3);
}

RpGrid3d::RpGrid3d(const char* name, int npts)
{
	objectName(name);
	m_data.reserve(npts*3);
}

// constructor
// Input: 
// 	val: array of x y z values (x1 y1 z1 x2 y2 z2 ...)
// 	number of points
//
RpGrid3d::RpGrid3d(DataValType* val, int npoints)
{
	if (val == NULL)
		return;

	copyArray(val, npoints*3, m_data);
}

//
// constructor
// input: array of pointers to values
//
RpGrid3d::RpGrid3d(DataValType* val[], int npoints)
{
	if (val == NULL)
		return;

	for (int i=0; i < npoints; i++) {
		m_data.push_back(val[i][0]);
		m_data.push_back(val[i][1]);
		m_data.push_back(val[i][2]);
	}
}

//
// constructor 
// Input: 
// 	spec for a uniform 3d grid
// 	X: origin, max, delta
// 	Y: origin, max, delta
// 	Z: origin, max, delta
// Result:
// 	expand points and add them to m_data
//
RpGrid3d::RpGrid3d(DataValType x_min, DataValType x_max, DataValType x_delta,
		   DataValType y_min, DataValType y_max, DataValType y_delta,
		   DataValType z_min, DataValType z_max, DataValType z_delta)
{
	setUniformGrid(x_min, x_max, x_delta, 
			y_min, y_max, y_delta,
			z_min, z_max, z_delta);
}

RP_ERROR
RpGrid3d::setUniformGrid(
		DataValType x_min, DataValType x_max, DataValType x_delta,
	   	DataValType y_min, DataValType y_max, DataValType y_delta,
	   	DataValType z_min, DataValType z_max, DataValType z_delta)
{
	// expand array, inclusive of origin and max
	for (int i=0; i <= (int)((x_max-x_min)/x_delta); i++) {
	    for (int j=0; j <= (int)((y_max-y_min)/y_delta); j++) {
		for (int k=0; k <= (int)((z_max-z_min)/y_delta); k++) {
			m_data.push_back(x_min + i*x_delta);
			m_data.push_back(y_min + j*y_delta);
			m_data.push_back(z_min + k*z_delta);
		}
	    }
	}

#ifdef DEBUG
printf("RpGrid3d uniform grid constructor: num=%d\n", m_data.size()/3);
#endif
	return RP_FAILURE;
}

//
// constructor for rectilinear 3d grid
//
RpGrid3d::RpGrid3d(DataValType* x, int xdim, DataValType* y, int ydim,
		DataValType* z, int zdim)
{
	setRectGrid(x, xdim, y, ydim, z, zdim);
}

// 
// add a rectilinear 3d grid
// Input:
// 	x: points on x axis
//   xdim: number of points on X
// 	y: points on y axis
//   ydim: number of points on Y
// 	z: points on z axis
//   zdim: number of points on Z
//
// Result: 
// 	expand data to (xdim * ydim * zdim) points
//
RP_ERROR 
RpGrid3d::setRectGrid(DataValType* x, int xdim, 
		                DataValType* y, int ydim,
		                DataValType* z, int zdim)
{
	if (x == NULL || y == NULL || z == NULL) {
		RpAppendErr("RpGrid3d::setRectGrid: null input ptrs");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	for (int i=0; i < xdim; i++)
	    for (int j=0;  j < ydim; j++) {
		   for (int k=0;  k < zdim; k++) {
			m_data.push_back(x[i]);
			m_data.push_back(y[j]);
			m_data.push_back(z[k]);
		   }
	    }

	return RP_SUCCESS;
}

//
// add a point (x, y) pair to grid
//
void RpGrid3d::addPoint(DataValType x, DataValType y, DataValType z)
{
	m_data.push_back(x);
	m_data.push_back(y);
	m_data.push_back(z);
}

//
// add all points to grid
//
RP_ERROR 
RpGrid3d::addAllPoints(DataValType* points, int npts)
{
	return (RpGrid1d::addAllPoints(points, 2*npts));
}

// 
// get pointer to array of points
//
DataValType*
RpGrid3d::getData()
{
	return RpGrid1d::getData();
}

//
// retrieve x y at index
//
RP_ERROR 
RpGrid3d::getData(DataValType& x, DataValType& y, DataValType& z, int index)
{
	x = m_data.at(index);
	y = m_data.at(index+1);
	z = m_data.at(index+2);

	return RP_SUCCESS;
}

//
// serialization
//
RP_ERROR
RpGrid3d::doSerialize(char* buf, int nbytes)
{
	RP_ERROR err;

	// call base class doSerialize()
	err = RpGrid1d::doSerialize(buf, nbytes);

	if (err == RP_SUCCESS) {
		// now, override the header with correct object type
		writeRpHeader(buf, RpCurrentVersion[GRID3D], nbytes);
	}

	return err;

}

//
// unmarshalling object from byte stream pointed by 'buf'
//
//
RP_ERROR 
RpGrid3d::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpGrid3d::deserialize: null buf pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char* ptr = (char*)buf;
	std::string header;
	int nbytes;

	readRpHeader(ptr, header, nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	if (header == RpCurrentVersion[GRID3D])
		return doDeserialize(ptr);

	// deal with older versions
	return RP_FAILURE;
}

//
//// mashalling object into xml string
////
void
RpGrid3d::xmlString(std::string& textString)
{
	char cstr[256];

	// clear input string
	textString.clear();

	textString.append("<points>");

	for (int i=0; i < (signed)m_data.size(); i++) {
		sprintf(cstr, "\t%.15f\t%.15f\t%.15f\n", 
				m_data.at(i), m_data.at(i+1), m_data.at(i+2));
		textString.append(cstr);
		i += 2;
	}
	textString.append("</points>\n");
}

//
// print the xml string from the object
//
void 
RpGrid3d::print()
{
	string str;

	printf("object name: %s\n", m_name.c_str());

	xmlString(str);

	printf("%s", str.c_str());
}

//
// get object type
//
const char* 
RpGrid3d::objectType()
{
        return RpObjectTypes[GRID3D];
}

