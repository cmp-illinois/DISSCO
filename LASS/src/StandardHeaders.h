/*
LASS (additive sound synthesis library)
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

/**
 * @file StandardHeaders.h
 * @brief Single pull-in for the C/C++ standard library headers LASS uses.
 *
 * Included transitively by every LASS public header (via @ref LASS.h) so
 * downstream code does not have to remember which `<algorithm>`, `<cmath>`,
 * `<vector>`, ... it needs. Also picks the hash-map implementation
 * (currently the libstdc++ `ext/hash_map`, aliased as @c DISSCO_HASHMAP)
 * and applies the `using namespace std` / `using namespace __gnu_cxx`
 * declarations the rest of the library is written against.
 */

#ifndef __STANDARDHEADERS_H
#define __STANDARDHEADERS_H

//----------------------------------------------------------------------------//

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

#endif

