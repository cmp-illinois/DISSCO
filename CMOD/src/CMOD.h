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

#ifndef __CMOD_H
#define __CMOD_H

/**
 * @file CMOD.h
 * @brief Umbrella header for the Composition Module (CMOD).
 *
 * CMOD is the engine half of DISSCO: it reads a `.dissco` project file,
 * expands its hierarchical Event tree (top → high → mid → low → bottom),
 * and emits both rendered sound (via the LASS audio library) and a
 * notated score. This header pulls in everything a translation unit
 * outside CMOD needs in order to drive a Piece end-to-end.
 *
 * @see Piece for the project-root model
 * @see Event for the layer base class
 * @see Bottom for the leaf event that produces Notes and Sounds
 */

#include "Libraries.h"

#include "Bottom.h"
#include "Define.h"
#include "Event.h"
#include "Matrix.h"
#include "Modifier.h"
#include "Note.h"
#include "Patter.h"
#include "Piece.h"
#include "Random.h"
#include "Rational.h"
#include "Sieve.h"
#include "Tempo.h"





#endif

