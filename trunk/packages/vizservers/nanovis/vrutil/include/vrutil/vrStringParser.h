/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <string>
#include <iostream>
#include <vrutil/vrUtil.h>

class VrUtilExport vrStringParser {
	std::string _whitechars;
	std::istream& _stream;
	char _buff[256];

public :
	vrStringParser(std::istream& stream);
	int getToken();
	void ungetToken();
	bool isWhiteChar(char ch);
	void setWhiteCharacters(const std::string& whitechars);
	char* getBuffer();

};

inline char* vrStringParser::getBuffer()
{
	return _buff;
}

inline void vrStringParser::setWhiteCharacters(const std::string& whitechars)
{
	_whitechars = whitechars;
}

inline bool vrStringParser::isWhiteChar(char ch)
{
	for (size_t i = 0; i < _whitechars.size(); ++i)
	{
		if (ch == _whitechars[i]) return true;
	}

	return false;
}

