/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <cstring>
#include <vrutil/vrColorBrewer.h>


vrColorBrewer::vrColorBrewer(int size, char label[20])
{
	this->size = size;
	
	colorScheme =  new CBColor[size];

	colorKey = new float[size];
	defaultOpacity = new float[size];
	for (int i = 0; i < size; ++i)
	{
		colorKey[i] = float(i) / (size - 1);
		
		if (i < 10)
			defaultOpacity[i] = 0.0;
		else
			defaultOpacity[i] = 0.5;
		
	}
	this->label = label;
}

vrColorBrewer::~vrColorBrewer(void)
{
	delete [] colorScheme;
	delete [] colorKey;
	delete [] defaultOpacity;
}

void vrColorBrewer::setColor(int index, float r, float g, float b)
{
	if (index <size)
	{
		colorScheme[index].r = r;
		colorScheme[index].g = g;
		colorScheme[index].b = b;
	}
}
