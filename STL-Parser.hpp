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

    Date Last Modified: 8/14/2016

    Purpose:
        Parse simple CAD .stl files
        Uses objectParser library, which is also available on GitHub
        Produces Display Lists suitable for use in OpenGL rendering context

        initial compile: GCC 4.8.4 on Ubuntu 14.04.3

    TODO: (DONE) add support for binary .stl files (shouldnt be too difficult)

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

    typedef std::vector<Model*> MultiModel;

    std::ifstream ifile; // starts out uninitialized
    bool fileOpened = false;
    char* _filename;

//-------------------------------------------------------------
// structs/unions/functions used when parsing binary .stl files

    union short_o {
        char byte[2];
        short short_;
    };

    union float_o {
        char byte[4];
        float float_; // floats are stored in little endian order
    };

    struct float3Union {
        float_o x_;
        float_o y_;
        float_o z_;
    };

    /* this struct has everthing needed to convert little endian numbers to big endian */
    struct triFloat3Union {
        float3Union pts[3];
        float3Union normal;
        GLfloat r_;
        GLfloat g_;
        GLfloat b_;
    };

    char header[80]; // contains file information we dont really care about

    // for binary stl files this is only used once but it is essential
    union int_o {
        char byte[4];
        int int_; // ints are stored in little endian order
    };

    void swapBytes(char arr[4]) {
        // swap bytes 0 and 3
        arr[0] = arr[0] ^ arr[3];
        arr[3] = arr[0] ^ arr[3];
        arr[0] = arr[0] ^ arr[3];

        // swap bytes 1 and 2
        arr[1] = arr[1] ^ arr[2];
        arr[2] = arr[1] ^ arr[2];
        arr[1] = arr[1] ^ arr[2];
    }

    // swap all things that need to be swapped
    void swapTriFloat3Union(triFloat3Union* tf3u) {
        for(int i = 0; i < 3; i++) {
            swapBytes(tf3u->pts[i].x_.byte);
            swapBytes(tf3u->pts[i].y_.byte);
            swapBytes(tf3u->pts[i].z_.byte);
        }
        swapBytes(tf3u->normal.x_.byte);
        swapBytes(tf3u->normal.y_.byte);
        swapBytes(tf3u->normal.z_.byte);
    }

    triFloat3* packTriFloat3(triFloat3Union* tf3u) {
        triFloat3* tf3 = new triFloat3;

        for(int i = 0; i < 3; i++) {
            tf3->pts[i].x_ = (GLfloat)tf3u->pts[i].x_.float_;
            tf3->pts[i].y_ = (GLfloat)tf3u->pts[i].y_.float_;
            tf3->pts[i].z_ = (GLfloat)tf3u->pts[i].z_.float_;
        }

        tf3->normal.x_ = (GLfloat)tf3u->normal.x_.float_;
        tf3->normal.y_ = (GLfloat)tf3u->normal.y_.float_;
        tf3->normal.z_ = (GLfloat)tf3u->normal.z_.float_;

        // typecasting not needed on these three values
        tf3->r_ = tf3u->r_;
        tf3->g_ = tf3u->g_;
        tf3->b_ = tf3u->b_;

        return tf3;
    }

//-------------------------------------------------------------

    void openFile(char* filename) {
        //ifile.open(filename, ios_base::in);
        //fileOpened = ifile.is_open();
        _filename = filename;
    }

    /* parses ascii .stl file containing description of object
        and makes a Model with it */
    Model* parseFileAscii(void) {

        //#define ifile STL::ifile // easier for Joe (no longer needed, should really just delete it, but im sentimental like that :) )

        ifile.open(_filename, ios_base::in);

        Model* myModel = new Model;
        myModel->clear(); // STL::Model is just a vector

        int numFaces = 0;

        while(!ifile.eof()) { // while there is still unread data

            std::string* str = new std::string;
            ifile >> *str;

            // count the number of faces on model and parse each facet
            if(*str == "facet") {
                numFaces++;

                // allocate space for each facet
                triFloat3* myFacet = new triFloat3;

                // normal vector comes first
                ifile >> *str; // should be 'normal' each time

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

        ifile.close();

        return myModel;

        //#undef ifile // ifile is now global variable name

        //return NULL;

    }

    /* same as function above but uses binary .stl files */
    Model* parseFileBinary(void) {

        std::ifstream ifile;

        ifile.open(_filename, ios::in | ios::binary);
        if(!ifile.is_open()) {
            std::cerr << "Invalid filename" << std::endl;
            exit(1);
        }

        Model* myModel = new Model;
        myModel->clear(); // STL::Model is just a vector
        int numFaces = 0;

        // start at beginning of file
        ifile.seekg(0, ios_base::beg);

        ifile.read(header, 80); // header is 80 bytes of stuff we dont really care about

        int_o intUnion;
        ifile.read(intUnion.byte, 4); // read 4 bytes in

        std::cout << "Pre-sort: " << intUnion.int_ << std::endl;
        //swapBytes(intUnion.byte);
        //std::cout << "Post-sort: " << intUnion.int_ << std::endl;
        //return 0;

        // one space for all measurements, values are then copied into triFloat3 and put into Model
        triFloat3Union preModel;

        short_o attrs;

        for(int i = 0; i < intUnion.int_; i++) {
            // read 3 floats into normal section of preModel
            ifile.read(preModel.normal.x_.byte, 4);
            ifile.read(preModel.normal.y_.byte, 4);
            ifile.read(preModel.normal.z_.byte, 4);

            // read 3 vectors into pts section of preModel
            for(int j = 0; j < 3; j++) {
                ifile.read(preModel.pts[j].x_.byte, 4);
                ifile.read(preModel.pts[j].y_.byte, 4);
                ifile.read(preModel.pts[j].z_.byte, 4);
            }

            // read to chars to skip attribute section
            ifile.read(attrs.byte, 2);

            /* attrs.short_  = attrs.short_ >> 1; // last bit isnt important
            preModel.r_ = 8.33f * (GLfloat)(attrs.short_ & 31);
            attrs.short_  = attrs.short_ >> 5;
            preModel.g_ = 8.33f * (GLfloat)(attrs.short_ & 31);
            attrs.short_  = attrs.short_ >> 5;
            preModel.b_ = 8.33f * (GLfloat)(attrs.short_ & 31);
            // swap all bytes that need to be swapped
            //swapTriFloat3Union(&preModel);
            */

            myModel->push_back(packTriFloat3(&preModel));

        }

        ifile.close();

        return myModel;

    }

    GLuint getBot(Model* myModel) {

        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        glBegin(GL_TRIANGLES);

            for(unsigned int i = 0; i < myModel->size(); i++) {
                // all triangles will be green
                glColor3f(0.0f, 1.0f, 0.0f);
                //glColor3f(myModel->at(i)->r_ / 255.0f, myModel->at(i)->g_ / 255.0f, myModel->at(i)->b_ / 255.0f);

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

    objParse::GLfloat3* getAABB_Center(Model* myModel) {
        objParse::GLfloat3 lesser;
        objParse::GLfloat3 larger;
        lesser.x_ = 1000000.0f; // start with a really big value that should only get smaller
        lesser.y_ = 1000000.0f; // --
        lesser.z_ = 1000000.0f; // --
        larger.x_ = -1000000.0f; // start with a really small value that should only get larger
        larger.y_ = -1000000.0f; // --
        larger.z_ = -1000000.0f; // --

        /* iterate through every point in every vertex to find largest and smallest xyz values */
        for(unsigned int i = 0; i < myModel->size(); i++) {
            for(int j = 0; j < 3; j++) {

                // test for low values
                if(myModel->at(i)->pts[j].x_ < lesser.x_) {
                    lesser.x_ = myModel->at(i)->pts[j].x_;
                }
                if(myModel->at(i)->pts[j].y_ < lesser.y_) {
                    lesser.y_ = myModel->at(i)->pts[j].y_;
                }
                if(myModel->at(i)->pts[j].z_ < lesser.z_) {
                    lesser.z_ = myModel->at(i)->pts[j].z_;
                }

                // test for high values
                if(myModel->at(i)->pts[j].x_ > larger.x_) {
                    larger.x_ = myModel->at(i)->pts[j].x_;
                }
                if(myModel->at(i)->pts[j].y_ > larger.y_) {
                    larger.y_ = myModel->at(i)->pts[j].y_;
                }
                if(myModel->at(i)->pts[j].z_ > larger.z_) {
                    larger.z_ = myModel->at(i)->pts[j].z_;
                }

            }
        }

        objParse::GLfloat3* myFloat3 = new objParse::GLfloat3;


        myFloat3->x_ = (lesser.x_ + larger.x_) / 2.0f; // mid x
        myFloat3->y_ = (lesser.y_ + larger.y_) / 2.0f; // mid y
        myFloat3->z_ = (lesser.z_ + larger.z_) / 2.0f; // mid z

        std::cout << "width: " << larger.x_ - lesser.x_ << " height: " << larger.y_ - lesser.y_ << " depth: " << larger.z_ - lesser.z_ << std::endl;

        return myFloat3;

    }

    GLuint getWireframe(Model* myModel, unsigned int start, unsigned int distance) {
        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        for(unsigned int i = start; i < myModel->size(); i += distance) {
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

    /* returns a pointer to a new tf3 with the same data */
    triFloat3* getNewtf3(triFloat3* tf3_o) {

        triFloat3* tf3_n = new triFloat3;

        // copy vertex data over
        for(int i = 0; i < 3; i++) {
            tf3_n->pts[i].x_ = tf3_o->pts[i].x_;
            tf3_n->pts[i].y_ = tf3_o->pts[i].y_;
            tf3_n->pts[i].z_ = tf3_o->pts[i].z_;
        }

        // copy whatever colors there are over
        tf3_n->r_ = tf3_o->r_;
        tf3_n->g_ = tf3_o->g_;
        tf3_n->b_ = tf3_o->b_;

        // copy normal vector definitions over
        tf3_n->normal.x_ = tf3_o->normal.x_;
        tf3_n->normal.y_ = tf3_o->normal.y_;
        tf3_n->normal.z_ = tf3_o->normal.z_;

        return tf3_n;
    }

    /* combine many smaller models (possibly from differnt files) into one larger Model */
    Model* packMultiModel(MultiModel* megaModel) {
        Model* myModel = new Model;

        for(unsigned int i = 0; i < megaModel->size(); i++) {
            for(unsigned int j = 0; j < megaModel->at(i)->size(); j++) {
                myModel->push_back(getNewtf3(megaModel->at(i)->at(j)));
            }
        }

        return myModel;
    }

}

#endif // __JJC_STL_PARSER_HPP__

// I find that '#define struct union' is very helpful in memory contrained systems
