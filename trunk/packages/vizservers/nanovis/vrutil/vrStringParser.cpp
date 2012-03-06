/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <vrutil/vrStringParser.h>

vrStringParser::vrStringParser(std::istream& stream)
: _stream(stream)
{
}

int vrStringParser::getToken()
{
	int index = 0;
	char ch = 0;
	if (_stream.eof()) return 0;

	while(!_stream.eof() && isWhiteChar(ch = _stream.get()));
	if (ch == 0 || isWhiteChar(ch)) return 0;

	do {
		_buff[index++] = ch;
	} while (!_stream.eof() && !isWhiteChar(ch = _stream.get()));

	_buff[index] = 0;

	return index;
}
