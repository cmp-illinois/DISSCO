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

//The MSVC compiler does not necessarily provide M_PI by default
#ifndef M_PI
  #define M_PI 3.14159265358979323846264338327950288
#endif

//Replace strcasecmp() and strncasecmp() with _stricmp() and _strnicmp() on Windows
//Replace index() and rindex() with strchr() and strrchr() on Windows
#ifdef _WIN32
  inline int strcasecmp(const char* s1, const char* s2) {
    return _stricmp(s1, s2);
  }

  inline int strncasecmp(const char* s1, const char* s2, size_t n) {
    return _strnicmp(s1, s2, n);
  }

  inline char* index(char* s, int c) {
    return ::strchr(s, c);
  }

  inline const char* index(const char* s, int c) {
    return ::strchr(s, c);
  }

  inline char* rindex(char* s, int c) {
    return ::strrchr(s, c);
  }

  inline const char* rindex(const char* s, int c) {
    return ::strrchr(s, c);
  }
#endif

/*On Windows, the standard C functions rand() / srand() are used as a 
temporary replacement for the POSIX functions random() / srandom().*/
#ifdef _WIN32
  inline long random() {
    return static_cast<long>(std::rand());
  }

  inline void srandom(unsigned int seed) {
    std::srand(seed);
  }
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string.h>

// Windows use <io.h> and <direct.h> instead of <unistd.h>
#include <ctime>
#ifndef _WIN32
  #include <unistd.h>
#else
  #include <io.h>
  #include <direct.h>
#endif

#include <iomanip>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>


/*The C++ standards committee only recently agreed on the new name
for "unordered_map". It used to be referred to as a hash_map, which
will be used here for now.*/

/* ext/hash_map is an old GCC extension; the modern C++ standard al
ternative is std::unordered_map*/
#ifdef _WIN32
  #include <unordered_map>

  namespace __gnu_cxx {
    template <
      class Key,
      class T,
      class Hash = std::hash<Key>,
      class Pred = std::equal_to<Key>,
      class Alloc = std::allocator<std::pair<const Key, T>>
    >
    using hash_map = std::unordered_map<Key, T, Hash, Pred, Alloc>;
  }

  using __gnu_cxx::hash_map;
#else
  #include <ext/hash_map>
  using __gnu_cxx::hash_map;
#endif

#define DISSCO_HASHMAP hash_map

/* C++0x Alternative (must compile with -std=c++0x):
  #include <unordered_map>
  #define DISSCO_HASHMAP unordered_map
*/

using namespace std;
using namespace __gnu_cxx; 

#endif

