//
// class for RpField
//

#include "field.h"

RP_ERROR RpField::setMeshObj(RpSerializable* mesh)
{
	if (mesh) {
		m_mesh = mesh;
		return RP_SUCCESS;
	} 
	else {
		return RP_ERR_NULL_PTR;
	}
}

//
// serialize Field object 
// 16 bytes: 	Header (including RV identifier, version, object type id)
// 				e.g., RV-A-FIELD, RV-A-MESH3D
// 4 bytes:	N: number of bytes in object name (to follow)
// N bytes: 	object name (e.g., "output.grid(g1d))"
// 4 bytes:  	NB number of points in array
// NB bytes:   	data array (x x x...)
// 4 bytes:	M: number of bytes in mesh name (to follow)
// M bytes: 	mesh name (e.g., "output.mesh(m2d))"
//
char* 
RpField::serialize(int& nb)
{
	char* buf;
	int nbytes = numBytes();

	if ( (buf = new char[nbytes]) == NULL) {
		RpAppendErr("RpField::serialize: new char[] failed");
		RpPrintErr();
		return buf;
	}

	doSerialize(buf, nbytes);

	nb = nbytes;

#ifdef DEBUG
printf("RpField::serialize: %d bytes,buf=%x\n",nbytes,(unsigned)buf);
#endif
	return buf;
}

// 
// serialize object data in Little Endian byte order
//
RP_ERROR
RpField::doSerialize(char* buf, int nbytes)
{
	// check buffer 
	if (buf == NULL || nbytes < numBytes()) {
                RpAppendErr("RpField::serialize: invalid buffer");
		RpPrintErr();
		return RP_ERR_INVALID_ARRAY;
	}

	char * ptr = buf;

	// write object header (version and typename)
	writeRpHeader(ptr, RpCurrentVersion[FIELD], nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	// write object name and its length
	writeString(ptr, m_name);
	ptr += m_name.size() + sizeof(int);

	// write mesh name and its length
	writeString(ptr, m_meshName);
	ptr += m_meshName.size() + sizeof(int);

	// write number of points and array data
	writeArrayDouble(ptr, m_data, m_data.size());

	return RP_SUCCESS;
}

RP_ERROR 
RpField::deserialize(const char* buf)
{
	if (buf == NULL) {
		RpAppendErr("RpField::deserialize: null buf pointer");
		RpPrintErr();
		return RP_ERR_NULL_PTR;
	}

	char* ptr = (char*)buf;
	std::string header;
	int nbytes;

	readRpHeader(ptr, header, nbytes);
	ptr += HEADER_SIZE + sizeof(int);
	
	if (header == RpCurrentVersion[FIELD])
		return doDeserialize(ptr);

	// deal with older versions
	return RP_FAILURE;
}

// 
// parse out the buffer, 
// stripped off the version and total #bytes already.
//
RP_ERROR
RpField::doDeserialize(const char* buf)
{
	char* ptr = (char*)buf;

	// parse object name and store name in m_name

	readString(ptr, m_name);
	ptr += sizeof(int) + m_name.size();
	
	readString(ptr, m_meshName);
	ptr += sizeof(int) + m_meshName.size();
	
	int npts;
	readArrayDouble(ptr, m_data, npts);

	return RP_SUCCESS;
}

//
// mashalling object into xml string
//
void 
RpField::xmlString(std::string& textString)
{
	int i;
	int npts = m_data.size();
	char cstr[256];

	// clear input string
	textString.clear();

	sprintf(cstr, "<field id=\"%s\">\n", m_name.c_str());
	textString.append(cstr);

	textString.append("<component>\n");
	sprintf(cstr, "<mesh>%s</mesh>\n", m_meshName.c_str());
	textString.append(cstr);

	textString.append("<values>");

	for (i=0; i < npts; i++) {
		sprintf(cstr, "\t%.15f\n", m_data[i]);
		textString.append(cstr);
	}

	textString.append("</values>\n</component>\n</field>\n");
}

//
// print the xml string from the object
//
void 
RpField::print()
{
	string str;

	printf("object name: %s\n", m_name.c_str());

	xmlString(str);

	printf("%s", str.c_str());
}

//
// get object type
//
const char* RpField::objectType()
{
	return RpObjectTypes[FIELD];
}

//
// return total number of bytes when mashalling out as bytes
// 
int RpField::numBytes()
{
	int nbytes = RpGrid1d::numBytes()
		+ sizeof(int) // #bytes in mesh name
		+ m_meshName.size();

	return nbytes;
}

void RpField::clear()
{
	RpGrid1d::clear();
	m_meshName.clear();
}

RpField RpField::operator=(const RpField& f)
{
	(RpGrid1d&)*this = f;
	m_meshName = f.m_meshName;

	return (*this);
}
