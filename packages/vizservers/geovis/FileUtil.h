/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */
#ifndef GEOVIS_FILEUTIL_H
#define GEOVIS_FILEUTIL_H

#include <osgEarth/FileUtils>

namespace GeoVis {

class DirectoryVisitor : public osgEarth::DirectoryVisitor
{
public:
    DirectoryVisitor()
    {}

    virtual void handlePostDir(const std::string& path)
    {}

    virtual void traverse(const std::string& path);
};

class CollectFilesVisitor : public DirectoryVisitor
{
public:
    CollectFilesVisitor()
    {}

    virtual void handleFile(const std::string& path)
    {
        filenames.push_back(path);
    }

    virtual void handlePostDir(const std::string& path)
    {
        dirnames.push_back(path);
    }

    std::vector<std::string> filenames;
    std::vector<std::string> dirnames;
};

extern void removeDirectory(const char *path);

}

#endif
