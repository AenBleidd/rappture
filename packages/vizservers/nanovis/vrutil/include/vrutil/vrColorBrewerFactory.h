/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef VRCOLORBREWERFACTORY_H
#define VRCOLORBREWERFACTORY_H

#include <vector>
#include <string>

#include <vrutil/vrUtil.h>

#define COLOR_SCHEME_START      3
#define COLOR_SCHEME_SEQ_END    9
#define COLOR_SCHEME_DIV_END    11

class vrColorBrewer;

class vrColorBrewerFactory
{
public:
    void loadColorBrewerList();

    int getColorMapCount() const;

    vrColorBrewer *getColorMap(int id);

    vrColorBrewer *chooseColorScheme(const std::string& scheme, int size);

    static vrColorBrewerFactory* getInstance();

protected:
    vrColorBrewerFactory();

private:
    std::vector<vrColorBrewer *> colorList;
    vrColorBrewer *_currentColorBrewer;

    static vrColorBrewerFactory *_instance;
};

inline vrColorBrewer* vrColorBrewerFactory::getColorMap(int id)
{
    return colorList[id];
}

inline int vrColorBrewerFactory::getColorMapCount() const
{
    return (int) colorList.size();
}

#endif
