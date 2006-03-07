#include "serializable.h"
#include "obj_types.h"

//
// factory method - creating object of 'type'
//
RpSerializable * RpSerializable::create(const char* objectType)
{
	if (!strcmp(objectType, RpObjectTypes[FIELD_OBJ]) )
		return new RpField;

	// todo
	
}
