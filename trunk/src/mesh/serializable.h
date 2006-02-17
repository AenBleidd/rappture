#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

//
// class implementing the serialization of various types
//

class RpSerializable {
public:
	// constructors
	RpSerializable();

	static void serialize(char& buf, int d);
	static void serialize(char& buf, double d);
	static void serialize(char& buf, double& buf, int nitems);
	static void serialize(char& buf, const char* str, int len);
};

#endif
