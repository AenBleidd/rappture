#ifndef __RP_ENDIAN_H__
#define __RP_ENDIAN_H__

template <class ValType > class ByteOrder
{
public:
	static bool IsBigEndian();
	static void OrderRead(const ValType& src, ValType& dst);
	static void OrderWrite(const ValType& src, ValType& dst);
	operator ValType () const;
	ValType operator=(ValType val);

	ValType m_val;
}; 

#endif
