/*
 * ----------------------------------------------------------------------
 *  Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "scew.h"
#include "scew_extras.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

/* indentation size (in whitespaces) */
#define INDENT_SIZE 4

#ifndef _RpLIBRARY_H 
#define _RpLIBRARY_H

class RpLibrary
{
    public:

        // users member fxns

        RpLibrary* element (std::string path = "", std::string as = "object");

        // should return RpObject& but for simplicity right now it doesnt
        // RpObject will either be an Array, RpString, RpNumber ...

        RpLibrary*  children (  std::string path = "",
                                RpLibrary* rpChildNode = NULL,
                                std::string type = "",
                                int* childCount = NULL  );

        RpLibrary*  get (std::string path = "");
        std::string getString (std::string path = "");
        double      getDouble (std::string path = "");

        RpLibrary& put (    std::string path,
                            std::string value,
                            std::string id = "",
                            int append = 0  );

        RpLibrary& put (    std::string path,
                            double value,
                            std::string id = "",
                            int append = 0  );

        RpLibrary& remove (std::string path = "");

        std::string xml();

        std::string nodeType();
        std::string nodeId();
        std::string nodeComp();

        void result();
        const char* nodeTypeC();
        const char* nodeIdC();
        const char* nodeCompC();

        // no arg constructor
        // used when we dont want to read an xml file to populate the xml tree
        // we are building a new xml structure
        RpLibrary ()
            :   parser      (NULL),
                tree        (NULL),
                root        (NULL)
        {
            tree = scew_tree_create();
            freeTree = 1;
            root = scew_tree_add_root(tree, "run");
        }

        RpLibrary (
                    std::string filePath
                )
            :   parser      (NULL),
                tree        (NULL),
                root        (NULL)
        {

            if (filePath.length() != 0) {
                // file path should not be null or empty string unless we are
                // creating a new xml file

                parser = scew_parser_create();

                scew_parser_ignore_whitespaces(parser, 1);

                /* Loads an XML file */
                if (!scew_parser_load_file(parser, filePath.c_str()))
                {
                    scew_error code = scew_error_code();
                    printf("Unable to load file (error #%d: %s)\n", code,
                           scew_error_string(code));

                    /*
                    std::cout << "Unable to load file (error #" \
                              << code << ": " << scew_error_string(code) \
                              << ")\n" << std::endl;
                    */

                    if (code == scew_error_expat)
                    {
                        enum XML_Error expat_code =
                            scew_error_expat_code(parser);
                        printf("Expat error #%d (line %d, column %d): %s\n",
                               expat_code,
                               scew_error_expat_line(parser),
                               scew_error_expat_column(parser),
                               scew_error_expat_string(expat_code));
                    }
                    // should probably exit program or something
                    // return EXIT_FAILURE;

                }

                tree = scew_parser_tree(parser);
                freeTree = 0;
                root = scew_tree_root(tree);
            }
            else {
                // create a new xml (from an empty file)
            }
        }


        // copy constructor
        // for some reason making this a const gives me problems when calling xml()
        // need help looking into this
        // RpLibrary ( const RpLibrary& other )
        RpLibrary ( RpLibrary& other )
            : parser    (NULL),
              tree      (NULL),
              root      (NULL)
        {
            std::string buffer;
            int buffLen;

            // fill in the current RpLibrary's data with other's data
            parser = scew_parser_create();
            scew_parser_ignore_whitespaces(parser, 1);

            // Loads the XML from other
            // the length cannot be 0 because xml() should not be returning 
            // empty strings
            buffer = other.xml();
            buffLen = buffer.length();

            if (buffLen > 0) {
                if (!scew_parser_load_buffer(parser,buffer.c_str(),buffLen))
                {
                    // there was an error loading the buffer
                    // how do you tell the user, you couldn't make a copy?
                    scew_error code = scew_error_code();
                    printf("Unable to load buffer (error #%d: %s)\n", code,
                           scew_error_string(code));

                    if (code == scew_error_expat)
                    {
                        enum XML_Error expat_code =
                            scew_error_expat_code(parser);
                        printf("Expat error #%d (line %d, column %d): %s\n",
                               expat_code,
                               scew_error_expat_line(parser),
                               scew_error_expat_column(parser),
                               scew_error_expat_string(expat_code));
                    }

                    // return an empty RpLibrary?
                    // return EXIT_FAILURE;

                    parser = NULL;
                }
                else {

                    // parsing the buffer was successful
                    // populate the new data members.

                    tree = scew_parser_tree(parser);
                    freeTree = 0;
                    root = scew_tree_root(tree);

                }

            } // end if (buffer.length() != 0) {
        }// end copy constructor

        // for some reason making this a const gives me problems when calling xml()
        // need help looking into this
        // RpLibrary& operator= (const RpLibrary& other) {
        RpLibrary& operator= (RpLibrary& other) {

            std::string buffer;
            int buffLen;

            scew_parser* tmp_parser;
            scew_tree* tmp_tree;
            scew_element* tmp_root;
            int tmp_freeTree;

            if (this != &other) {

                tmp_parser   = parser;
                tmp_tree     = tree;
                tmp_root     = root;
                tmp_freeTree = freeTree;

                // fill in the current RpLibrary's data with other's data
                parser = scew_parser_create();
                scew_parser_ignore_whitespaces(parser, 1);

                // Loads the XML from other
                // the length cannot be 0 because xml() should not be returning
                // empty strings
                buffer = other.xml();
                buffLen = buffer.length();

                if (buffLen > 0) {
                    if (!scew_parser_load_buffer(parser,buffer.c_str(),buffLen))
                    {
                        // there was an error loading the buffer
                        // how do you tell the user, you couldn't make a copy?
                        scew_error code = scew_error_code();
                        printf("Unable to load buffer (error #%d: %s)\n", code,
                               scew_error_string(code));

                        if (code == scew_error_expat)
                        {
                            enum XML_Error expat_code = 
                                scew_error_expat_code(parser);
                            printf("Expat error #%d (line %d, column %d): %s\n",
                                   expat_code,
                                   scew_error_expat_line(parser),
                                   scew_error_expat_column(parser),
                                   scew_error_expat_string(expat_code));
                        }

                        // return things back to the way they used to be
                        // or maybe return an empty RpLibrary?
                        // return EXIT_FAILURE;

                        // return this object to its previous state.
                        parser = tmp_parser;
                    }
                    else {

                        // parsing the buffer was successful
                        // populate the new data members.

                        tree = scew_parser_tree(parser);
                        freeTree = 0;
                        root = scew_tree_root(tree);

                        // free the current RpLibrary's data
                        // we do the free so far down so we can see if
                        // parsing the other object's xml fails.
                        // if the parsing fails, we can still return this
                        // object to its previous state.
                        if (tmp_tree && tmp_freeTree) {
                            scew_tree_free(tmp_tree);
                            tmp_tree = NULL;
                        }
                        if (tmp_parser) {
                            scew_parser_free(tmp_parser);
                            tmp_parser = NULL;
                        }
                        if (tmp_root) {
                            tmp_root = NULL;
                        }
                    }

                } // end if (buffer.length() != 0) {
            } // end if (this != &other)

            return *this;
        } // end operator=


        // default destructor
        virtual ~RpLibrary ()
        {
            // clean up dynamic memory

            if (tree && freeTree) {
                scew_tree_free(tree);
                tree = NULL;
            }
            if (parser) {
                scew_parser_free(parser);
                parser = NULL;
            }
            if (root) {
                // scew_element_free(root);
                root = NULL;
            }
        }

    private:

        scew_parser* parser;
        scew_tree* tree;
        scew_element* root;

        // flag to tell if we are responsible for calling scew_tree_free
        // on the tree structure. if we get our tree by using the
        // scew_tree_create
        // fxn, we need to free it. if we get our tree using the
        // scew_parser_tree
        // fxn, then it will be free'd when the parser is free'd.
        int freeTree;


        RpLibrary ( scew_element* node)
            :   parser      (NULL),
                tree        (NULL),
                root        (node)

        {}


        std::string _get_attribute(scew_element* element, std::string attributeName);
        int _path2list (std::string& path, std::string** list, int listLen);
        std::string _node2name (scew_element* node);
        std::string _node2comp (scew_element* node);
        int _splitPath (std::string& path,
                        std::string& tagName,
                        int* idx,
                        std::string& id );
        scew_element* _find (std::string path, int create);
        void print_indent (unsigned int indent, std::stringstream& outString);
        void print_attributes (scew_element* element, std::stringstream& outString);
        void print_element( scew_element* element,
                            unsigned int indent,
                            std::stringstream& outString    );


};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
