#ifndef __RP_BYTEORDER_H__
#define __RP_BYTEORDER_H__

//#include <stdio.h>
#include <string.h>

template<class ValType> class ByteOrder {
public:
	ByteOrder() { };
	~ByteOrder() { };

	// returns 1 if byte order is big endian
	static int IsBigEndian() 
	{
		unsigned x = 1;
		return !(*(char *)(&x));
	};

	// copy value from src to dst adjusting for endianess
	// If the machine calling this function uses big endian byte order,
	// bytes are flipped.
	static void OrderCopy(const ValType* src_val, ValType* dst_val)
	{
		unsigned char* src = (unsigned char *)src_val;
		unsigned char* dst = (unsigned char *)dst_val;
	
		// our software will use little endian
		// swap bytes if reading on a big endian machine
		//
		if (IsBigEndian()) {
			switch(sizeof(ValType)) {
				case 1:
	                		dst[0] = src[0];
					break;
				case 2: 
	                		dst[1] = src[0];
	                		dst[0] = src[1];
					break;
				case 4:
					dst[3] = src[0];
	                		dst[2] = src[1];
	                		dst[1] = src[2];
	                		dst[0] = src[3];
				default:
	                		dst += sizeof(ValType)-1;
	                		for(int i=sizeof(ValType); i>0; i-- )
	                			*(dst--) = *(src++);
			}
	    	}
		else {
			// LE: straight copy 
			unsigned int i;
			for (i=0; i<sizeof(ValType); i++) 
				memcpy((void*)dst, (void*)src, sizeof(ValType));
		}
	};

	// copy an array of ValType elements
	static void OrderCopyArray(const ValType* src_val, ValType* dst_val, int nitems)
	{
		for (int i=0; i < nitems; i++)
			OrderCopy(&(src_val[i]), &(dst_val[i]));
	};

	// swap bytes in place, src altered for BE machine.
	static void Flip(ValType* src_val)
	{
		ValType val;
		Copy(src_val, &val);
		memcpy((void*)src_val, (void*)&val, sizeof(ValType));
	};

	static void FlipArray(ValType* src_val, int nitems)
	{
		for (int i=0; i < nitems; i++)
			Flip(&(src_val[i]));
	}

	operator ValType () const 
	{
		ValType l_value;
		OrderCopy(m_val, l_value);
		return l_value;
	};

	ValType operator=(ValType in_val)
	{ 
		OrderCopy(in_val, m_val); 
		return in_val;
	};

private:
	ValType m_val;
}; 

#endif
