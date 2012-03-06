/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vr3d/vrEnums.h>
#include <vr3d/vrTexture.h>

class Vr3DExport vrTexture1D : public vrTexture {

	/**
	 * @brief Two-dimensional texture width.
	 * @brief Default value is 0
	 */
	int			_width;

	/**
	 * @brief Wrap mode.
	 * @brief Default value is R2_CLAMP
	 */
	TEXWRAP		_wrapS;

public :
	/**
	 * @brief constructuor
	 */
	vrTexture1D(bool depth = false);

	/** 
	 * @brief destructor
	 */
	~vrTexture1D();

	/**
	 * @brief get texture size
	 * @return texture length
	 */
	int getWidth() const;

	/**
	 * @brief set wrap mode
	 */
	void setWrapS(TEXWRAP wrap);


	/**
	 * @brief set wrap mode
	 */
	TEXWRAP getWrapS() const;

	/**
	 * @brief set texture data
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(COLORFORMAT format, DATATYPE type, int width, void* data);

    /**
	 * @brief set texture data
     * @param internalFormat texture internal data format 
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(TEXTARGET target, COLORFORMAT internalFormat, COLORFORMAT format, DATATYPE type, int width, void* data);

    /**
	 * @brief set texture data (NV)
     * @param internalFormat internal colorformat
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(COLORFORMAT internalFormat, COLORFORMAT format, DATATYPE type, int width, void* data);

	/**
	 * @brief update pixel data
	 * @param data pixel data
	 */
	virtual void updatePixels(void* data);

};

inline int vrTexture1D::getWidth() const
{
	return _width;
}

inline void vrTexture1D::setWrapS(TEXWRAP wrap)
{
	_wrapS = wrap;
}


inline TEXWRAP vrTexture1D::getWrapS() const
{
	return _wrapS;
}

