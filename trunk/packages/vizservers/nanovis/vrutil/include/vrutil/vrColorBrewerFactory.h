/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <vrutil/vrUtil.h>

class vrColorBrewer;

#include <vector>
#include <string>

#define COLOR_SCHEME_START    3
#define COLOR_SCHEME_SEQ_END    9
#define COLOR_SCHEME_DIV_END    11

class vrColorBrewerFactory {
	std::vector<vrColorBrewer*> colorList;
	
	vrColorBrewer* _currentColorBrewer;

	static vrColorBrewerFactory* _instance;
protected :
	vrColorBrewerFactory();
public :
	static vrColorBrewerFactory* getInstance();

public :
	void loadColorBrewerList();
	int getColorMapCount() const;

	vrColorBrewer* getColorMap(int id);

	vrColorBrewer* chooseColorScheme(const std::string& scheme, int size);
};

inline vrColorBrewer* vrColorBrewerFactory::getColorMap(int id)
{
	return colorList[id];
}

inline int vrColorBrewerFactory::getColorMapCount() const
{
	return (int) colorList.size();
}
