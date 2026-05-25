/*
CMOD (composition module)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)
                                                                                
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
                                                                                
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
                                                                                
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef OUTPUT_H
#define OUTPUT_H

#include "Libraries.h"
#include "Tempo.h"
#include "NotationScore.h"

/**
 * @file Output.h
 * @brief Side-band result tree CMOD builds while expanding a Piece.
 *
 * As events resolve into sounds and notes, the @ref Output static class
 * accumulates an @ref OutputNode tree that mirrors the event hierarchy.
 * Each node carries (name, value, unit) properties and a list of child
 * nodes; the tree is serialized to the project's `.particel` XML report
 * when synthesis finishes.
 */

/**
 * @brief One node in the build-time output tree.
 *
 * Owns its child OutputNode pointers and frees them on destruction.
 * Property lookup is linear in the property count — callers add a small,
 * fixed set per node so this is acceptable.
 */
struct OutputNode
{
  string nodeName;
  
  vector<string> propertyNames;
  vector<string> propertyValues;
  vector<string> propertyUnits;
  
  vector<OutputNode*> subNodes;  //vector of pointers to the class itself
  
  OutputNode(string name);
  ~OutputNode();
  
  void addProperty(string name, string value, string units);
  string getProperty(string name);
  bool isBottom(void);
  bool isNote(void);
  bool isBuildPhase(void);
  string getXML(void);
  
  static string findAndReplace(string in, string needle, string replace);

  /**
   *  Sanitize - remove illegal symbols
   **/
  static string sanitize(string name);
};

/**
 * @brief Process-global facade for accumulating and emitting CMOD output.
 *
 * Output is purely static — there is one tree per CMOD run, rooted at
 * @c top. Event-expansion code pushes nodes as it descends and pops as it
 * ascends, building the same shape as the event tree itself. When the
 * piece finishes, the tree is written to the project's `.particel` XML
 * report so the user can audit every choice CMOD made.
 */
class Output
{
  static OutputNode* top;
  
  static ofstream *particelFile;
  static int level;
  
  static void writeLineToParticel(string line);
  
  protected:
  static string getLevelIndentation(bool isProperty, bool isEndLevel);
  static string getPropertyIndentation(void);
 
  public:
  static NotationScore notation_score_;

  static void initialize(string particelFilename);
  static void free(void);
  static OutputNode* getCurrentLevelNode(void);
  static void beginSubLevel(string name);
  static void addProperty(string name, string value, string units="");
  template <class T>
  static void addProperty(string name, T value, string units="") {
    stringstream oss;
    oss.setf(ios::fixed, ios::floatfield);
    oss.precision(2);
    oss << value;
    addProperty(name, string(oss.str()), units);
  }
  static void endSubLevel(void);
};

#endif
