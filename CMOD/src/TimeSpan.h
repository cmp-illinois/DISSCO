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

//----------------------------------------------------------------------------//
//
//  TimeSpan.h
//
//----------------------------------------------------------------------------//

#ifndef TIMESPAN_H
#define TIMESPAN_H

//CMOD includes
#include "Libraries.h"
#include "Rational.h"

/**
 * @file TimeSpan.h
 * @brief Dual-representation [start, end] interval used by every Event.
 *
 * Each event lives on two clocks at once: the audible / wall-clock seconds
 * used by LASS for synthesis, and the rational EDU grid used by CMOD for
 * notated rhythm. TimeSpan carries both, plus the absolute EDU offset
 * needed when stitching multiple events onto a single global timeline.
 */

//----------------------------------------------------------------------------//

/**
 * @brief Interval expressed simultaneously in seconds and in EDUs.
 *
 * `start` / `duration` / `end` are seconds; `startEDU` / `durationEDU` /
 * `endEDU` are exact rationals on the EDU grid. `startEDUAbsolute` is the
 * cumulative integer EDU offset from the start of the piece.
 */
struct TimeSpan {
  float start;
  float duration;
  float end;
  Ratio startEDU;
  unsigned long long int startEDUAbsolute;
  Ratio durationEDU;
  Ratio endEDU;
  
  TimeSpan() : start(0), duration(0), end(0), startEDU(0, 0), startEDUAbsolute(0), 
  durationEDU(0, 0), endEDU(0, 0) {}
};
#endif /* TIMESPAN_H */

