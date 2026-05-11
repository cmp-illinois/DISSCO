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

//----------------------------------------------------------------------------//
//
//	Spatializer.cpp
//
//----------------------------------------------------------------------------//

#ifndef __SPATIALIZER_CPP
#define __SPATIALIZER_CPP


//----------------------------------------------------------------------------//

#include "Spatializer.h"

using namespace std;

//----------------------------------------------------------------------------//


Spatializer::~Spatializer(){
  // nothing to do
}

MultiTrack* Spatializer::spatialize_Track(Track& t, int numTracks)
{


    Track* scaledTrack = new Track(t);
    scaledTrack->scale( 1.0/float(numTracks) );
    
    // create a new multitrack:
    MultiTrack* mt = new MultiTrack;
    
    // add copies of the scaled track to this multitrack
    for (int i=0; i<numTracks; i++)
    {
        mt->add( new Track(*scaledTrack) );
    }
    
    // delete the scaled track
    delete scaledTrack;
    
    // return:
    return mt;
}

/* ZIYUAN CHEN, July 2023 */
MultiTrack* Spatializer::spatialize_MultiTrack(MultiTrack& t, int numTracks,
                       m_sample_count_type sampleCount,
                       m_rate_type samplingRate)
{
    // create a new multitrack:
    MultiTrack* mt = new MultiTrack(numTracks, sampleCount, samplingRate);

    // this temporary multitrack holds each spatialized "component"
    MultiTrack* _tmp = new MultiTrack(numTracks, sampleCount, samplingRate);
    
    // superimpose each component to the output multitrack
    Iterator<Track*> it = t.iterator();
    while (it.hasNext())
    {
        _tmp = spatialize_Track( *(it.next()), numTracks );
        mt->composite( *_tmp );
    }
    
    // return:
    return mt;
}

//----------------------------------------------------------------------------//
Spatializer* Spatializer::clone()
{
    return new Spatializer(*this);
}

void Spatializer::xml_print( ofstream& xmlOutput )
{
	xmlOutput << "\t\t<spatializer>" << endl;
	
	xmlOutput << "\t\t</spatializer>" << endl;
}

//----------------------------------------------------------------------------//
#endif //__SPATIALIZER_CPP
