/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/**
 * @file vrvrTexture.h
 * @brief An abtract class for the texture
 * @author Insoo Woo(vrinside@gmail.com)
 * @author PhD research assistants in PURPL at Purdue University  
 */

#pragma once

#ifdef WIN32
#pragma pointers_to_members(full_generality, single_inheritance)
#endif

#include <vr3d/vr3d.h>
#include <vr3d/vrEnums.h>

class Vr3DExport vrTexture {
	friend class vrVRMLLoader;

protected :
	/**
	 * @brief texture target
	 * @brief Default value is 0
	 */
	TEXTARGET		_target;

	/**
	 * @brief vrTexture object id. 
	 * @brief Default value is 0
	 */
	unsigned int		_objectID;

	/**
	 * @brief texture type
	 * @brief Default value is GL_FLOAT
	 */
	DATATYPE		_type;

	/**
	 * @brief texture format
	 * @brief Default value is GL_RGBA
	 */
	COLORFORMAT	_colorFormat;

    /**
	 * @brief texture internal format
	 * @brief Default value is GL_RGBA
	 */
	COLORFORMAT	_internalColorFormat;


	/**
	 * @brief min filter
	 * @brief Default value is R2_LINEAR
	 */
	TEXFILTER		_minFilter;

	/**
	 * @brief mag filter
	 * @brief Default value is R2_LINEAR
	 */
	TEXFILTER		_magFilter;

	/**
	 * @brief component count
	 */
	int			_compCount;


public :
	vrTexture();
protected :
	virtual ~vrTexture();

public :
	/**
	 * @brief bind texture id
	 */
	void bind(int index) const;

	/**
	 * @brief unbind texture id
	 */
	void unbind() const;

	/**
	 * @brief update pixel data
	 * @param data pixel data
	 */
	virtual void updatePixels(void* data) = 0;

	/**
	 * @brief retur texture object id
	 */
	unsigned int getGraphicsObjectID() const;

	/**
	 * @brief return texture target
	 */
	TEXTARGET getTextureTarget() const;

	/**
	 * @brief set min filter
	 */
	void setMinFilter(TEXFILTER filter);


	/**
	 * @brief set mag filter
	 */
	void setMagFilter(TEXFILTER filter);

	/**
	 * @brief return min filter
	 */
	TEXFILTER getMinFilter() const;

	/**
	 * @brief return mag filter
	 */
	TEXFILTER getMagFilter() const;

	/**
	 * @brief return color format
	 */
	COLORFORMAT getColorFormat() const;

	/**
	 * @brief return data type
	 */
	DATATYPE getDataType() const;

	/**
	 * @brief component count
	 */
	int getCompCount() const;

};


inline void vrTexture::bind(int index) const
{
	glEnable(_target);
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(_target, _objectID);
}

inline void vrTexture::unbind() const
{
	glDisable(_target);
}

inline unsigned int vrTexture::getGraphicsObjectID() const
{
	return _objectID;
}

inline TEXTARGET vrTexture::getTextureTarget() const
{
	return _target;
}

inline void vrTexture::setMinFilter(TEXFILTER filter)
{
	_minFilter = filter;
}

inline void vrTexture::setMagFilter(TEXFILTER filter)
{
	_magFilter = filter;
}

inline TEXFILTER vrTexture::getMinFilter() const
{
	return _minFilter;
}

inline TEXFILTER vrTexture::getMagFilter() const
{
	return _magFilter;
}

inline COLORFORMAT vrTexture::getColorFormat() const
{
	return _colorFormat;
}

inline DATATYPE vrTexture::getDataType() const
{
	return _type;
}

inline int vrTexture::getCompCount() const
{
	return _compCount;
}

inline int GetNumComponent(COLORFORMAT format)
{
       switch (format)
       {
       case CF_LUMINANCE : return 1;
       case CF_RGB : return 3;
       case CF_RGBA : return 4;
       default :
               //R2Assert(0);
		   ;
       }
       return 0;
}

inline int SizeOf(DATATYPE format)
{
       switch (format)
       {
       case DT_UBYTE : return sizeof(unsigned char);
       case DT_FLOAT : return sizeof(float);
#ifndef OPENGLES
       //case DT_UINT : return sizeof(unsigned int);
       //case DT_INT : return sizeof(unsigned int);
#endif
       default :
               //R2Assert(0);
		   ;
       }
       return 0;
}

inline bool IsPowerOfTwo(int value)
{
       return ((value&(value-1))==0);
}

inline int GetNextPowerOfTwo(int value)
{
       int nextPOT = 1;
       while (nextPOT < value)
       {
               nextPOT <<= 1;
	   }

       return nextPOT;
}

