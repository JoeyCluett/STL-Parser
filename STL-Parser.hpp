/*
    STL-Parser, .stl file parsing library
    Copyright (C) 2016  Joseph Cluett

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author(s):
        Joseph Cluett (main author)

    File Type: header/implementation, STL-Parser

    Date Created: 8/7/2016

    Date Last Modified: 8/7/2016

    Purpose:
        Parse simple CAD .stl files
        Uses objectParser library, which is also available on GitHub
        Produces Display Lists suitable for use in OpenGL rendering context

        initial compile: GCC 4.8.4 on Ubuntu 14.04.3

*/

#ifndef __JJC_STL_PARSER_HPP__
#define __JJC_STL_PARSER_HPP__

// need OpenGL typedefs for various data types
#include <GL/glx.h>
#include <GL/gl.h>

#include <fstream>
#include <iostream>
#include <objectParser.hpp>
//#include <string.h> // for strcmp()
#include <string>

namespace stl { // objectParser.hpp has many similarly named functions and so we use a different namespace to differentiate

    // stores 3 vertices, full color information and a normal vector for each face
    struct triFloat3 {
        objParse::GLfloat3 pts[3]; // .stl files only work with triangles
        objParse::GLfloat3 normal;
        GLfloat r_;
        GLfloat g_;
        GLfloat b_;
    };

    typedef std::vector<triFloat3*> Model;

    std::ifstream ifile; // starts out uninitialized
    bool fileOpened = false;

    void openFile(char* filename) {
        ifile.open(filename, ios_base::in);
        fileOpened = ifile.is_open();
    }

    /* parses .stl file containing description of object
        and makes a Model with it */
    Model* parseFile(void) {

        //#define ifile STL::ifile // easier for Joe (no longer needed, should really just delete it, but im sentimental like that :) )

        if(fileOpened == false) {
            std::cerr << "No file specified" << std::endl;
            exit(1);
        }

        Model* myModel = new Model;
        myModel->clear(); // STL::Model is just a vector

        int numFaces = 0;

        while(!ifile.eof()) { // while there is still unread data

            std::string* str = new std::string; // removing parentheses causes segfault immediately upon execution
            ifile >> *str;    // but keeping them causes compiler error, bit of a catch 22

            // count the number of faces on model and parse each facet
            if(*str == "facet") {
                numFaces++;

                // allocate space for each facet
                triFloat3* myFacet = new triFloat3;

                // normal vector comes first
                ifile >> *str; // shoudl be 'normal' each time

                // read next 3 items and convert to floats
                ifile >> *str;
                myFacet->normal.x_ = (GLfloat)atof(str->c_str());
                ifile >> *str;
                myFacet->normal.y_ = (GLfloat)atof(str->c_str());
                ifile >> *str;
                myFacet->normal.z_ = (GLfloat)atof(str->c_str());

                // skip next 2 items 'outer' and 'loop'
                ifile >> *str;
                ifile >> *str;

                // read 3 vectors from file
                for(int i = 0; i < 3; i++) {
                    ifile >> *str; // 'vertex' not needed

                    ifile >> *str;
                    myFacet->pts[i].x_ = (GLfloat)atof(str->c_str());
                    ifile >> *str;
                    myFacet->pts[i].y_ = (GLfloat)atof(str->c_str());
                    ifile >> *str;
                    myFacet->pts[i].z_ = (GLfloat)atof(str->c_str());
                }

                myModel->push_back(myFacet);

            }

        }

        std::cout << "Number of faces: " << numFaces << std::endl;
        std::cout << "Size of Model: " << myModel->size() << std::endl;

        return myModel;

        //#undef ifile // ifile is now global variable name

        //return NULL;

    }

    GLuint getBot(Model* myModel) {

        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        glBegin(GL_TRIANGLES);

            for(unsigned int i = 0; i < myModel->size(); i++) {
                // all triangles will be green
                glColor3f(0.0f, 1.0f, 0.0f);
                for(int j = 0; j < 3; j++) {
                    glVertex3f(myModel->at(i)->pts[j].x_, myModel->at(i)->pts[j].y_, myModel->at(i)->pts[j].z_);
                }
            }

        glEnd();
        glEndList();

        return nrmcBot;
    }

    GLuint getWireframe(Model* myModel) {
        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        for(unsigned int i = 0; i < myModel->size(); i++) {
            glBegin(GL_LINE_LOOP);
                // all triangles will be green
                glColor3f(0.0f, 0.0f, 0.0f);
                for(int j = 0; j < 3; j++) {
                    glVertex3f(myModel->at(i)->pts[j].x_, myModel->at(i)->pts[j].y_, myModel->at(i)->pts[j].z_);
                }
            glEnd();
        }
        glEndList();

        return nrmcBot;
    }

}

#endif // __JJC_STL_PARSER_HPP__

// I find that '#define struct union' is very helpful in memory contrained systems
