/*
    objectParser, XML-based object parsing library
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

    File Type: header/implementation, objectParser

    Date Created: 6/8/2016

    Date Last Modified: 8/5/2016

    Purpose:
        Parse model files written in custom xml-based object description language
        Produces Display Lists suitable for use in OpenGL rendering context

    Misc. Notes:
        Uses many other non-standard libraries, all of which are freely available on GitHub

        initial compile: GCC 4.8.4 on Ubuntu 14.04.3

*/

#ifndef JJC_OBJECT_PARSER_HPP
#define JJC_OBJECT_PARSER_HPP

#include <stdlib.h>
#include <GL/glx.h>
#include <GL/gl.h>

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>

#include <iostream>

// places to hold displayList values
#include <vector>

// for strcmp function
#include <string.h>

// should use a typedef instead
#define ObjModel vector<Quadfloat3*>*

// allow user to optionally define a different scaling factor
#ifndef _SCALE_
    #define _SCALE_ (GLfloat)1.0
#endif // _SCALE_

using namespace std;

namespace objParse {

    // struct describes individual vertices
    struct GLfloat3 {
        GLfloat x_;
        GLfloat y_;
        GLfloat z_;
    };

    // 4 points and a name to track
    struct Quadfloat3 {
        char* name;
        GLfloat3 pts[4];
        GLfloat r_;
        GLfloat g_;
        GLfloat b_;

        // constructor that doesnt do any constructing
        Quadfloat3(void) {
            ;
        }

        Quadfloat3(Quadfloat3* oldquad) {

            name = oldquad->name;
            r_ = oldquad->r_;
            g_ = oldquad->g_;
            b_ = oldquad->b_;

            for(int i = 0; i < 4; i++) {
                pts[i].x_ = oldquad->pts[i].x_;
                pts[i].y_ = oldquad->pts[i].y_;
                pts[i].z_ = oldquad->pts[i].z_;
            }
        }

    };

    typedef vector<Quadfloat3*> Model;

    Model* GLfloatVec = NULL;

    /* parses xml file containing physical description of robot */
    void parseBotFile(char* filename) { // expects Model to be empty

        //GLfloatVec = new vector<Quadfloat3*>;
        GLfloatVec = new Model;
        GLfloatVec->clear();

        // parse file containing description of robot
        cout << "Creating xml document object" << endl;
        rapidxml::xml_document<> doc; // create xml document object

        cout << "Creating xml file object" << endl;
        ifstream myfile(filename);
        vector<char> fileBuffer((istreambuf_iterator<char>(myfile)), istreambuf_iterator<char>( ));
        fileBuffer.push_back('\0');

        doc.parse<rapidxml::parse_trim_whitespace>(&fileBuffer[0]); // parse the contents of the file
        rapidxml::xml_node<>* root = doc.first_node("body"); // find our root node

        if(root == NULL) {
            cerr << "No 'body' tag found" << endl;
            exit(1);
        }

        rapidxml::xml_attribute<>* attr = root->first_attribute("name");
        if(attr != NULL) {
            cout << "object name: " << attr->value() << endl;
        } else {
            cerr << "Object name not given" << endl;
            exit(1);
        }

        attr = root->first_attribute("numParts");
        if(attr == NULL) {
            cerr << "number of parts in object not given" << endl;
            exit(1);
        }
        GLsizei numParts = (GLsizei)atoi(attr->value());
        cout << "number of parts: " << numParts << endl;

        rapidxml::xml_node<>* part;
        if(numParts > 0) {
            part = root->first_node("part");
            if(part == NULL) {
                cerr << "'part' tag missing" << endl;
                exit(1);
            }
        } else {
            cerr << "minimum one part per object" << endl;
            exit(1);
        }

        // part is pointing to first 'part' tag
        for(GLsizei i = 0; i < numParts; i++) {

            rapidxml::xml_node<>* rect = part->first_node("rect");
            if(rect == NULL) {
                cerr << "no rect vertices defined" << endl;
                exit(1);
            }

            // iterate through all 'rect' tags
            do {
                // rect is either original or uses a predefined rect
                attr = rect->first_attribute("name");
                if(attr != NULL) { // rect is original ploygon definition
                    cout << "Name of rectangle is: " << attr->value() << endl;
                    Quadfloat3* myquad = new Quadfloat3;
                    myquad->name = attr->value();
                    rapidxml::xml_node<>* vertex = rect->first_node("vertex");
                    if(vertex != NULL) {
                        /* retrieve and test that values exist for xyz for each vertex, failure to give correct number of vertices will
                           either result in undefined but legal behavior or will cause system to throw segfault */
                        int numV = 0;
                        do {

                            rapidxml::xml_attribute<>* attrVertex = vertex->first_attribute("x");
                            if(attrVertex != NULL) {
                                // x attribute exists
                                myquad->pts[numV].x_ = (GLfloat)atof(attrVertex->value()) / _SCALE_;
                            }
                            attrVertex = vertex->first_attribute("y");
                            if(attrVertex != NULL) {
                                // y attribute exists
                                myquad->pts[numV].y_ = (GLfloat)atof(attrVertex->value()) / _SCALE_;
                            }
                            attrVertex = vertex->first_attribute("z");
                            if(attrVertex != NULL) {
                                // z attribute exists
                                myquad->pts[numV].z_ = (GLfloat)atof(attrVertex->value()) / _SCALE_;
                            }

                            numV++;
                        } while(vertex = vertex->next_sibling("vertex"));

                        // now retrieve the shift offsets
                        rapidxml::xml_node<>* shift = rect->first_node("shift");
                        if(shift != NULL) {
                            rapidxml::xml_attribute<>* attrShift = shift->first_attribute("x");
                            if(attrShift != NULL) {
                                GLfloat xshift = (GLfloat)atof(attrShift->value()) / _SCALE_;
                                for(int i = 0; i < 4; i++) {
                                    myquad->pts[i].x_ += xshift;
                                }
                            }
                            attrShift = shift->first_attribute("y");
                            if(attrShift != NULL) {
                                GLfloat yshift = (GLfloat)atof(attrShift->value()) / _SCALE_;
                                for(int i = 0; i < 4; i++) {
                                    myquad->pts[i].y_ += yshift;
                                }
                            }
                            attrShift = shift->first_attribute("z");
                            if(attrShift != NULL) {
                                GLfloat zshift = (GLfloat)atof(attrShift->value()) / _SCALE_;
                                for(int i = 0; i < 4; i++) {
                                    myquad->pts[i].z_ += zshift;
                                }
                            }

                        } else {
                            cerr << "shifted values not given" << endl;
                            exit(1);
                        }

                        // get rgb color information for the rectangle
                        rapidxml::xml_node<>* color = rect->first_node("color");
                        if(color != NULL) {

                            rapidxml::xml_attribute<>* attrColorR = color->first_attribute("r");
                            rapidxml::xml_attribute<>* attrColorG = color->first_attribute("g");
                            rapidxml::xml_attribute<>* attrColorB = color->first_attribute("b");

                            if(attrColorR && attrColorG && attrColorB) {

                                // put rgb values in rectangle
                                GLfloat r = (GLfloat)atof(attrColorR->value());
                                GLfloat g = (GLfloat)atof(attrColorG->value());
                                GLfloat b = (GLfloat)atof(attrColorB->value());

                                myquad->r_ = r / (GLfloat)255;
                                myquad->g_ = g / (GLfloat)255;
                                myquad->b_ = b / (GLfloat)255;

                            } else {
                                cerr << "one or more rgb values missing" << endl;
                                exit(1);
                            }
                        } else {
                            cerr << "color information not given" << endl;
                            exit(1);
                        }

                        GLfloatVec->push_back(myquad);
                    } else {
                        cerr << "Vertices not given" << endl;
                        exit(1);
                    }


                }

                attr = rect->first_attribute("uses");
                if(attr != NULL) { // rect is a copy of existing rectangle
                    cout << "reusing rect: " << attr->value() << endl;

                    // copy correct rectangle information into new rectangle struct
                    Quadfloat3* usesOld;
                    for(int i = 0; i < GLfloatVec->size(); i++) {
                        if(strcmp(GLfloatVec->at(i)->name, attr->value()) == 0) {
                            cout << "original found at index " << i << endl;
                            usesOld = new Quadfloat3(GLfloatVec->at(i));
                            break; // done with scanning loop
                        }
                    }

                    rapidxml::xml_node<>* shift = rect->first_node("shift");
                    if(shift != NULL) {

                        rapidxml::xml_attribute<>* attrShiftX = shift->first_attribute("x");
                        rapidxml::xml_attribute<>* attrShiftY = shift->first_attribute("y");
                        rapidxml::xml_attribute<>* attrShiftZ = shift->first_attribute("z");

                        if(attrShiftX && attrShiftY && attrShiftZ) {

                            for(int i = 0; i < 4; i++) {
                                usesOld->pts[i].x_ += (GLfloat)atof(attrShiftX->value()) / _SCALE_;
                                usesOld->pts[i].y_ += (GLfloat)atof(attrShiftY->value()) / _SCALE_;
                                usesOld->pts[i].z_ += (GLfloat)atof(attrShiftZ->value()) / _SCALE_;
                            }

                        } else {
                            cerr << "shift value missing" << endl;
                            exit(1);
                        }

                    } else {
                        cerr << "shift not given for copy" << endl;
                        exit(1);
                    }

                    rapidxml::xml_node<>* color = rect->first_node("color");
                    if(color != NULL) {
                        rapidxml::xml_attribute<>* attrColorR = color->first_attribute("r");
                        rapidxml::xml_attribute<>* attrColorG = color->first_attribute("g");
                        rapidxml::xml_attribute<>* attrColorB = color->first_attribute("b");

                        if(attrColorR && attrColorG && attrColorB) {
                            // put rgb values in rectangle
                            GLfloat r = (GLfloat)atof(attrColorR->value());
                            GLfloat g = (GLfloat)atof(attrColorG->value());
                            GLfloat b = (GLfloat)atof(attrColorB->value());

                            usesOld->r_ = r / (GLfloat)255;
                            usesOld->g_ = g / (GLfloat)255;
                            usesOld->b_ = b / (GLfloat)255;

                        } else {
                            cerr << "one or more rgb values missing" << endl;
                            exit(1);
                        }
                    } else {
                        cout << "warning: color not given for cold rect" << endl;
                    }

                    GLfloatVec->push_back(usesOld);

                }

            } while(rect = rect->next_sibling("rect"));

            part = part->next_sibling("part"); // currently only supports one part

        }

        // invert every x-coordinate, seems that GLUT/OpenGL doesn't like the human view of the world
        for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
            for(int j = 0; j < 4; j++) {
                GLfloatVec->at(i)->pts[j].x_ *= -1;
            }
        }

        return;
    }

    /* returns a model of the robot in its original position */
    GLuint getBot(Model* GLfloatVec) {
        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        glBegin(GL_QUADS);

            for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
                Quadfloat3* myquad = GLfloatVec->at(i);
                glColor3f(myquad->r_, myquad->g_, myquad->b_);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(myquad->pts[j].x_, myquad->pts[j].y_, myquad->pts[j].z_);
                }
            }

        glEnd();
        glEndList();

        return nrmcBot;
    }

    /* returns a wireframe model of the robot in its original position */
    GLuint getWireframe(Model* GLfloatVec) {
        GLuint myObj = glGenLists(1);
        glNewList(myObj, GL_COMPILE);

        for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
        glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_STRIP);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(GLfloatVec->at(i)->pts[j].x_, GLfloatVec->at(i)->pts[j].y_, GLfloatVec->at(i)->pts[j].z_);
                }
            glEnd();
        }

        glEndList();

        return myObj;
    }

    /* returns a model of the robot shifted some distance along each axis */
    GLuint getBotShifted(GLfloat xShift, GLfloat yShift, GLfloat zShift) {
        GLuint nrmcBot = glGenLists(1);

        glNewList(nrmcBot, GL_COMPILE);
        glBegin(GL_QUADS);

            for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
                Quadfloat3* myquad = GLfloatVec->at(i);
                glColor3f(myquad->r_, myquad->g_, myquad->b_);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(myquad->pts[j].x_ + xShift, myquad->pts[j].y_ + yShift, myquad->pts[j].z_ + zShift);
                }
            }

        glEnd();
        glEndList();

        return nrmcBot;
    }

    /* tells OpenGL pipeline about various points to render */
    // TODO solve (not hack) the x-axis flipping problem
    void drawBot(Model* GLfloatVec) {
        glBegin(GL_QUADS);
            for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
                Quadfloat3* myquad = GLfloatVec->at(i);
                glColor3f(myquad->r_, myquad->g_, myquad->b_);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(myquad->pts[j].x_, myquad->pts[j].y_, myquad->pts[j].z_);
                }
            }
        glEnd();
    }

    void drawWireframe(Model* GLfloatVec) {
        glColor3f(0.0f, 0.0f, 0.0f);
        for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
            glBegin(GL_LINE_STRIP);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(GLfloatVec->at(i)->pts[j].x_, GLfloatVec->at(i)->pts[j].y_, GLfloatVec->at(i)->pts[j].z_);
                }
            glEnd();
        }

    }

    /* just like drawBot but shifts values before sending them to OpenGL pipeline */
    void drawBotShifted(GLfloat xShift, GLfloat yShift, GLfloat zShift) {
        glBegin(GL_QUADS);
            for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
                Quadfloat3* myquad = GLfloatVec->at(i);
                glColor3f(myquad->r_, myquad->g_, myquad->b_);
                for(int j = 0; j < 4; j++) {
                    glVertex3f(myquad->pts[j].x_ + xShift, myquad->pts[j].y_ + yShift, myquad->pts[j].z_ + zShift);
                }
            }
        glEnd();
    }

    /* allows user to retrieve center point of bot */
    GLfloat3* getCenterPoint(GLfloat xShift, GLfloat yShift, GLfloat zShift) {
        GLfloat3* tempFloat3 = new GLfloat3;
        tempFloat3->x_ = 0.0f;
        tempFloat3->y_ = 0.0f;
        tempFloat3->z_ = 0.0f;

        // add all xyz values seperately
        for(unsigned int i = 0; i < GLfloatVec->size(); i++) {
            for(int j = 0; j < 4; j++) {
                //glVertex3f(myquad->pts[j].x_ + xShift, myquad->pts[j].y_ + yShift, myquad->pts[j].z_ + zShift);
                tempFloat3->x_ += GLfloatVec->at(i)->pts[j].x_;
                tempFloat3->y_ += GLfloatVec->at(i)->pts[j].y_;
                tempFloat3->z_ += GLfloatVec->at(i)->pts[j].z_;
            }
        }

        tempFloat3->x_ /= GLfloatVec->size() * 4;
        tempFloat3->y_ /= GLfloatVec->size() * 4;
        tempFloat3->z_ /= GLfloatVec->size() * 4;

        tempFloat3->x_ += xShift;
        tempFloat3->y_ += yShift;
        tempFloat3->z_ += zShift;

        return tempFloat3;
    }

}


#endif // JJC_OBJECT_PARSER_HPP


