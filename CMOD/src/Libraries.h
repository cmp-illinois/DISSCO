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

#ifndef __CMOD_LIBRARIES_H
#define __CMOD_LIBRARIES_H

//Several C++ standard library includes needed by CMOD
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <thread>

//Pseudo-standard, but widely used headers
#include <dirent.h>

//The LASS library for additive sound synthesis
#include "LASS.h"
#include "Score.h"

//Include the Rational number class here since it is a fundamental type and has
//no .cpp file that is actually compiled.
#include "Rational.h"

//Turn on the std namespace by default.
using namespace std;



// XML parsing via pugixml
#include <pugixml.hpp>
#include <cstring>
#include <stdexcept>

// this is the math expression parser : muparser
//#include "muParser.h"
#include "../../external-libs/muParser/muParser.h"

typedef enum {
  eventTop    = 0,
  eventHigh   = 1,
  eventMid    = 2,
  eventLow    = 3,
  eventBottom = 4,
  eventSound  = 5,
  eventEnv    = 6,
  eventSiv    = 7,
  eventSpa    = 8,
  eventPat    = 9,
	eventRev    = 10,
	eventFolder = 11,
	eventNote = 12,
	eventFil = 13,
  eventMea = 14,
  eventSpec = 15

} EventType;

typedef enum{
  tempoPrefixNone = 0,
  tempoPrefixDotted = 1,
  tempoPrefixDoubleDotted = 2,
  tempoPrefixTriple = 3,
  tempoPrefixQuintuple = 4,
  tempoPrefixSextuple = 5,
  tempoPrefixSeptuple = 6
} TempoPrefix;

typedef enum {
  tempoNoteValueWhole = 0,
  tempoNoteValueHalf =1,
  tempoNoteValueQuarter = 2,
  tempoNoteValueEighth = 3,
  tempoNoteValueSixteenth = 4,
  tempoNoteValueThirtySecond = 5
} TempoNoteValue;


// Element-only navigation helpers for pugi::xml_node. pugixml's first_child()
// and next_sibling() return any node type (including whitespace), so wrap them
// to skip non-element nodes, matching Xerces' getFirstElementChild semantics.
inline pugi::xml_node GFEC(pugi::xml_node n) {
  auto c = n.first_child();
  while (c && c.type() != pugi::node_element) c = c.next_sibling();
  return c;
}
inline pugi::xml_node GNES(pugi::xml_node n) {
  auto c = n.next_sibling();
  while (c && c.type() != pugi::node_element) c = c.next_sibling();
  return c;
}

// Recursive descendant lookup by tag name (Xerces getElementsByTagName first hit).
inline pugi::xml_node descendantByName(pugi::xml_node root, const char* tag) {
  if (!root) return pugi::xml_node();
  for (auto c = root.first_child(); c; c = c.next_sibling()) {
    if (c.type() == pugi::node_element && std::strcmp(c.name(), tag) == 0) return c;
    auto found = descendantByName(c, tag);
    if (found) return found;
  }
  return pugi::xml_node();
}

#define XMLTC Utilities::XMLTranscode

#endif
