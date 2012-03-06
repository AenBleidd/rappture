/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <string>
#include <vrutil/vrUtil.h>

class VrUtilExport CBColor {
public :
	float r, g, b;
};


class VrUtilExport vrColorBrewer {
public:
	CBColor *colorScheme;
	float* colorKey;
	float* defaultOpacity;
	int size;
	std::string label;

public:
	vrColorBrewer();
	vrColorBrewer(int size, char label[20]);
	~vrColorBrewer(void);
	
public :
	void setColor(int index, float r, float g, float b);
};

