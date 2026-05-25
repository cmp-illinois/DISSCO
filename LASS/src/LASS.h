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
 * @file LASS.h
 * @brief Umbrella header for the LASS additive sound synthesis library.
 *
 * LASS is the DSP half of DISSCO. CMOD calls into it to turn an expanded
 * Event tree into rendered audio: a @ref Score holds @ref Sound objects,
 * each of which decomposes into @ref Partial objects, which are summed and
 * shaped by @ref Filter / @ref Reverb / @ref Spatializer to produce one
 * or more @ref Track samples inside a @ref MultiTrack. @ref Envelope and
 * its iterators drive time-varying parameters, and @ref AuWriter writes
 * the final mix to disk.
 *
 * Including this single header pulls in everything a downstream
 * translation unit needs from the library; it is also the canonical entry
 * point referenced from CMOD.
 */

#include "StandardHeaders.h"

#include "AbstractIterator.h"
#include "AllPassFilter.h"
#include "AuWriter.h"
#include "Collection.h"
#include "Constant.h"
#include "DynamicVariable.h"
#include "DynamicVariableSequence.h"
#include "DynamicVariableSequenceIterator.h"
#include "Envelope.h"
#include "EnvelopeIterator.h"
#include "EnvelopeLibrary.h"
#include "Filter.h"
#include "Interpolator.h"
#include "InterpolatorIterator.h"
#include "InterpolatorTypes.h"
#include "Iterator.h"
#include "LPCombFilter.h"
#include "Loudness.h"
#include "LowPassFilter.h"
#include "MultiPan.h"
#include "MultiTrack.h"
#include "Pan.h"
#include "ParameterLib.h"
#include "Partial.h"
#include "Reverb.h"
#include "Score.h"
#include "Sound.h"
#include "SoundSample.h"
#include "Spatializer.h"
#include "Track.h"
#include "Types.h"
#include "XmlReader.h"
#include "BiQuadFilter.h"
#include "MarkovModel.h"
