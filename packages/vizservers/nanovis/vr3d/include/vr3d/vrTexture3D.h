/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vr3d/vrTexture.h>

#ifndef OPENGLES

class Vr3DExport vrTexture3D : public vrTexture {


	/**
	 * @brief Three-dimensional texture width.
	 * @brief Default value is 0
	 */
	int			_width;

	/**
	 * @brief Three-dimensional texture height.
	 * @brief Default value is 0
	 */
	int			_height;

	/**
	 * @brief Three-dimensional texture depth.
	 * @brief Default value is 0
	 */
	int			_depth;

	/**
	 * @brief Wrap mode.
	 * @brief Default value is R2_CLAMP
	 */
	TEXWRAP		_wrapS;

	/**
	 * @brief Wrap mode.
	 * @brief Default value is R2_CLAMP
	 */
	TEXWRAP		_wrapT;
public :
	/**
	 * @brief constructuor
	 */
	vrTexture3D();

	/** 
	 * @brief destructor
	 */
	~vrTexture3D();

	/**
	 * @brief get texture size
	 * @return texture length
	 */
	int getWidth() const;

	/**
	 * @brief get texture height
	 * @return texture height
	 */
	int getHeight() const;

	/**
	 * @brief get texture height
	 * @return texture depth
	 */
	int getDepth() const;

	/**
	 * @brief set wrap mode
	 */
	void setWrapS(TEXWRAP wrap);

	/*
	 * @brief set wrap mode
	 */
	void setWrapT(TEXWRAP wrap);

	/**
	 * @brief set wrap mode
	 */
	TEXWRAP getWrapS() const;

	/*
	 * @brief set wrap mode
	 */
	TEXWRAP getWrapT() const;

	/**
	 * @brief set texture data
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(COLORFORMAT format, DATATYPE type, int width, int height, int depth, void* data);

    /**
	 * @brief set texture data
     * @param internalFormat texture internal data format 
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(TEXTARGET target, COLORFORMAT internalFormat, COLORFORMAT format, DATATYPE type, int width, int height, int depth, void* data);

    /**
	 * @brief set texture data (NV)
     * @param internalFormat internal colorformat
	 * @param format texture data format 
	 * @param type texture data type 
	 * @param width texture width
	 * @param data texture data
	 */
	void setPixels(COLORFORMAT internalFormat, COLORFORMAT format, DATATYPE type, int width, int height, int depth, void* data);

	/**
	 * @brief update pixel data
	 * @param data pixel data
	 */
	virtual void updatePixels(void* data);
};


inline int vrTexture3D::getWidth() const
{
	return _width;
}

inline int vrTexture3D::getHeight() const
{
	return _height;
}

inline int vrTexture3D::getDepth() const
{
	return _depth;
}

inline void vrTexture3D::setWrapS(TEXWRAP wrap)
{
	_wrapS = wrap;
}

inline void vrTexture3D::setWrapT(TEXWRAP wrap)
{
	_wrapT = wrap;
}

inline TEXWRAP vrTexture3D::getWrapS() const
{
	return _wrapS;
}

inline TEXWRAP vrTexture3D::getWrapT() const
{
	return _wrapT;
}

#endif 
