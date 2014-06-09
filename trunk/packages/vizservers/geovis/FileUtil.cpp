/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <string>

#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include "FileUtil.h"
#include "Trace.h"

using namespace GeoVis;

void DirectoryVisitor::traverse(const std::string& path)
{
    if (osgDB::fileType(path) == osgDB::DIRECTORY) {
        if (handleDir(path)) {
            osgDB::DirectoryContents files = osgDB::getDirectoryContents(path);
            for (osgDB::DirectoryContents::const_iterator f = files.begin(); f != files.end(); ++f) {
                if (f->compare(".") == 0 || f->compare("..") == 0)
                    continue;

                std::string filepath = osgDB::concatPaths(path, *f);
                traverse(filepath);
            }
        }
        handlePostDir(path);
    } else if (osgDB::fileType(path) == osgDB::REGULAR_FILE) {
        handleFile(path);
    }
}

void GeoVis::removeDirectory(const char *path)
{
    TRACE("%s", path);

    CollectFilesVisitor cfv;
    cfv.traverse(std::string(path));
    for (std::vector<std::string>::const_iterator itr = cfv.filenames.begin();
         itr != cfv.filenames.end(); ++itr) {
        if (remove(itr->c_str()) < 0) {
            ERROR("remove: %s", strerror(errno));
        }
    }
    for (std::vector<std::string>::const_iterator itr = cfv.dirnames.begin();
         itr != cfv.dirnames.end(); ++itr) {
        if (remove(itr->c_str()) < 0) {
            ERROR("remove: %s", strerror(errno));
        }
    }
}
