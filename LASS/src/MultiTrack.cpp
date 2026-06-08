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
//	MultiTrack.cpp
//
//----------------------------------------------------------------------------//

#ifndef __MULTI_TRACK_CPP
#define __MULTI_TRACK_CPP

//----------------------------------------------------------------------------//

#include "MultiTrack.h"

//----------------------------------------------------------------------------//
MultiTrack::MultiTrack()
{
    // nothing
}
    
//----------------------------------------------------------------------------//
MultiTrack::MultiTrack(
        int channels,
        m_sample_count_type numSamples,
        m_rate_type samplingRate)
{
    for (int i=0; i<channels; i++)
    {
        // create a blank track: (zero'd out)
        Track* t = new Track(numSamples, samplingRate, true);
        // add this blank track:
        add(t);
    }

}

//----------------------------------------------------------------------------//
MultiTrack::MultiTrack(const MultiTrack& mt)
{
    // copy every track from mt to this object.
    for (int i = 0; i < mt.size(); i++)
        add(new Track(*mt.get(i)));
}

//----------------------------------------------------------------------------//
MultiTrack& MultiTrack::operator=(const MultiTrack& mt)
{
    if ( this != &mt) // beware of self assignment
    {
        // delete any tracks:
        for (Track* track : tracks_)
            delete track;

        // clear this object
        tracks_.clear();

        // copy every track from mt to this object.
        for (int i = 0; i < mt.size(); i++)
            add(new Track(*mt.get(i)));
    }

    return *this;
}

//----------------------------------------------------------------------------//
MultiTrack::~MultiTrack()
{
        // delete any tracks:
        for (Track* track : tracks_)
            delete track;

        // clear this object
        tracks_.clear();
}

//----------------------------------------------------------------------------//
void MultiTrack::composite(MultiTrack& mt, m_time_type startTime)
{
    for (int i = 0; i < size() && i < mt.size(); i++)
        get(i)->composite(*mt.get(i), startTime);

    // we should do some warning if sizes aren't equal and such.
}

//----------------------------------------------------------------------------//
void MultiTrack::add(Track* track)
{
    tracks_.push_back(track);
}

//----------------------------------------------------------------------------//
Track* MultiTrack::get(int index) const
{
    return tracks_[index];
}

//----------------------------------------------------------------------------//
int MultiTrack::size() const
{
    return (int) tracks_.size();
}

//----------------------------------------------------------------------------//
vector<Track*>::iterator MultiTrack::begin()
{
    return tracks_.begin();
}

//----------------------------------------------------------------------------//
vector<Track*>::iterator MultiTrack::end()
{
    return tracks_.end();
}

//----------------------------------------------------------------------------//
#endif //__MULTI_TRACK_CPP
