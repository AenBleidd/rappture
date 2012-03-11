/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRSTRINGPARSER_H
#define VRSTRINGPARSER_H

#include <string>
#include <iostream>

#include <vrutil/vrUtil.h>

class vrStringParser
{
public:
    vrStringParser(std::istream& stream);

    int getToken();

    void ungetToken();

    bool isWhiteChar(char ch);

    void setWhiteCharacters(const std::string& whitechars);

    char *getBuffer();

private:
    std::string _whitechars;
    std::istream& _stream;
    char _buff[256];
};

inline char *vrStringParser::getBuffer()
{
    return _buff;
}

inline void vrStringParser::setWhiteCharacters(const std::string& whitechars)
{
    _whitechars = whitechars;
}

inline bool vrStringParser::isWhiteChar(char ch)
{
    for (size_t i = 0; i < _whitechars.size(); ++i) {
        if (ch == _whitechars[i]) return true;
    }

    return false;
}

#endif

