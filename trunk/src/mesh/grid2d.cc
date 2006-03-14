//
// class for 2D grid
//

#include "grid2d.h"

// Constructors
RpGrid2d::RpGrid2d()
{
}

RpGrid2d::RpGrid2d(int npoints)
{
	m_data.reserve(npoints*2);
}

// constructor
// Input: 
// 	val: array of x y values (x1 y1 x2 y2 ...)
// 	number of points
//
RpGrid2d::RpGrid2d(DataValType* val, int npoints)
{
	if (val == NULL)
		return;

	for (int i=0; i<2*npoints; i++)
		m_data.push_back(val[i]);
}

//
// constructor
// input: array of pointers to points
//
RpGrid2d::RpGrid2d(DataValType* val[], int npoints)
{
	if (val == NULL)
		return;

	for (int i=0; i < npoints; i++) {
		m_data.push_back(val[i][0]);
		m_data.push_back(val[i][1]);
	}
}

//
// constructor 
// Input: 
// 	spec for a uniform 2d grid
// 	X: origin, max, delta
// 	Y: origin, max, delta
// Result:
// 	expand points and add them to m_data
//
RpGrid2d::RpGrid2d(DataValType x_min, DataValType x_max, DataValType x_delta,
		   DataValType y_min, DataValType y_max, DataValType y_delta)
{
	setUniformGrid(x_min, x_max, x_delta, y_min, y_max, y_delta);
}

RP_ERROR
RpGrid2d::setUniformGrid(DataValType x_min, DataValType x_max, DataValType x_delta,
	   DataValType y_min, DataValType y_max, DataValType y_delta)
{
	// expand array, inclusive of origin and max
	for (int i=0; i <= (int)((x_max-x_min)/x_delta); i++) {
		for (int j=0; j <= (int)((y_max-y_min)/y_delta); j++) {
			m_data.push_back(x_min + i*x_delta);
			m_data.push_back(y_min + j*y_delta);
		}
	}

#ifdef DEBUG
printf("RpGrid2d uniform grid constructor: num=%d\n", m_data.size()/2);
#endif
	return RP_FAILURE;
}

//
// constructor for rectilinear 2d grid
//
RpGrid2d::RpGrid2d(DataValType* x, int xdim, DataValType* y, int ydim)
{
	setRectGrid(x, xdim, y, ydim);
}

// 
// add a rectilinear 2d grid
// Input:
// 	x: points on x axis
//   xdim: number of points on X
// 	y: points on y axis
//   ydim: number of points on Y
//
// Result: 
// 	expand data to (xdim * ydim) points
//
RP_ERROR 
RpGrid2d::setRectGrid(DataValType* x, int xdim, 
		                DataValType* y, int ydim)
{
	if (x == NULL || y == NULL) {
		RpAppendErr("RpGrid2d::setRectGrid: null input ptrs");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	for (int i=0; i < xdim; i++)
		for (int j=0;  j < ydim; j++) {
			m_data.push_back(x[i]);
			m_data.push_back(y[j]);
		}

	return RP_SUCCESS;
}

//
// add a point (x, y) pair to grid
//
void RpGrid2d::addPoint(DataValType x, DataValType y)
{
	m_data.push_back(x);
	m_data.push_back(y);
}

//
// add all points to grid
//
RP_ERROR 
RpGrid2d::addAllPoints(DataValType* points, int npts)
{
	return (RpGrid1d::addAllPoints(points, 2*npts));
}

// 
// get pointer to array of points
//
DataValType*
RpGrid2d::getData()
{
	return RpGrid1d::getData();
}

//
// retrieve x y at index
//
RP_ERROR 
RpGrid2d::getData(DataValType& x, DataValType& y, int index)
{
	x = m_data.at(index);
	y = m_data.at(index+1);

	return RP_SUCCESS;
}

//
// serialization
//
RP_ERROR
RpGrid2d::doSerialize(char* buf, int nbytes)
{
	RP_ERROR err;

	// call base class doSerialize()
	err = RpGrid1d::doSerialize(buf, nbytes);

	if (err == RP_SUCCESS) {
		// now, override the header with correct object type
		writeRpHeader(buf, RpCurrentVersion[GRID2D], nbytes);
	}

	return err;

}

//
// unmarshalling object from byte stream pointed by 'buf'
//
//
RP_ERROR 
RpGrid2d::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpGrid2d::deserialize: null buf pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char* ptr = (char*)buf;
	std::string header;
	int nbytes;

	readRpHeader(ptr, header, nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	if (header == RpCurrentVersion[GRID2D])
		return doDeserialize(ptr);

	// deal with older versions
	return RP_FAILURE;
}

//
//// mashalling object into xml string
////
void
RpGrid2d::xmlString(std::string& textString)
{
	char cstr[256];

	// clear input string
	textString.clear();

	textString.append("<points>");

	for (int i=0; i < (signed)m_data.size(); i++) {
		sprintf(cstr, "\t%.15f\t%.15f\n", m_data.at(i), m_data.at(i+1));
		textString.append(cstr);
		i++;
	}
	textString.append("</points>\n");
}

//
// print the xml string from the object
//
void 
RpGrid2d::print()
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
RpGrid2d::objectType()
{
        return RpObjectTypes[GRID2D];
}

