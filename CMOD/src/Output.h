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

struct OutputNode
{
  std::string nodeName;
  
  std::vector<std::string> propertyNames;
  std::vector<std::string> propertyValues;
  std::vector<std::string> propertyUnits;
  
  std::vector<OutputNode*> subNodes;  //std::vector of pointers to the class itself
  
  OutputNode(std::string name);
  ~OutputNode();
  
  void addProperty(std::string name, std::string value, std::string units);
  std::string getProperty(std::string name);
  bool isBottom(void);
  bool isNote(void);
  bool isBuildPhase(void);
  std::string getXML(void);
  
  static std::string findAndReplace(std::string in, std::string needle, std::string replace);

  /**
   *  Sanitize - remove illegal symbols
   **/
  static std::string sanitize(std::string name);
};

// The static output class.
class Output
{
  static OutputNode* top;
  
  static std::ofstream *particelFile;
  static int level;
  
  static void writeLineToParticel(std::string line);
  
  protected:
  static std::string getLevelIndentation(bool isProperty, bool isEndLevel);
  static std::string getPropertyIndentation(void);
 
  public:
  static NotationScore notation_score_;

  static void initialize(std::string particelFilename);
  static void free(void);
  static OutputNode* getCurrentLevelNode(void);
  static void beginSubLevel(std::string name);
  static void addProperty(std::string name, std::string value, std::string units="");
  template <class T>
  static void addProperty(std::string name, T value, std::string units="") {
    std::stringstream oss;
    oss.setf(ios::fixed, ios::floatfield);
    oss.precision(2);
    oss << value;
    addProperty(name, std::string(oss.str()), units);
  }
  static void endSubLevel(void);
};

#endif
