#ifndef __RP_BYTEORDER_H__
#define __RP_BYTEORDER_H__

template<class ValType> class ByteOrder {
public:
	ByteOrder() { };
	~ByteOrder() { };

	// returns 1 if byte order is big endian
	static int IsBigEndian();

	// copy value from src to dst adjusting for endianess
	static void OrderCopy(const ValType& src, ValType& dst);

	// swap bytes in place, src altered for BE machine.
	static void Flip(ValType& src);

	//operator ValType () const;
	//ValType operator=(ValType val);

private:
	ValType m_val;
}; 

#endif
