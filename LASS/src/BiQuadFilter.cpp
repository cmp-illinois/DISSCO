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
//	BiQuadFilter.cpp
//
//----------------------------------------------------------------------------//

/* Best if viewed with 4-space tabs */

#ifndef __BI_QUAD_FILTER_CPP
#define __BI_QUAD_FILTER_CPP

#ifndef M_LN2
#define M_LN2	   0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

//----------------------------------------------------------------------------//

#include <math.h>
#include <stdlib.h>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <memory>
#ifdef LASS_WITH_CUDA
#include "../CUDA/BiquadCuda.h"
#endif
#include "SoundSample.h"
#include "Track.h"
#include "MultiTrack.h"
#include "Filter.h"
#include "BiQuadFilter.h"


//----------------------------------------------------------------------------//

/* set up the BiQuad Filter */
BiQuadFilter::BiQuadFilter(int type, m_sample_type dbGain, m_sample_type freq,m_sample_type srate, m_sample_type bandwidth)
{
	 
    	m_sample_type A, omega, sn, cs, alpha, beta;
    	m_sample_type a0, a1, a2, b0, b1, b2;

	
   	/* setup variables */
	A = static_cast<m_sample_type>(pow(10, dbGain /40));
	omega = static_cast<m_sample_type>(2 * M_PI * freq /srate);
    	sn = sin(omega);
    	cs = cos(omega);
	alpha = static_cast<m_sample_type>(sn * sinh(M_LN2 /2 * bandwidth * omega /sn));
    	beta = sqrt(A + A);


	    switch (type) {
	    case LPF:
	        b0 = (1 - cs) /2;
	        b1 = 1 - cs;
	        b2 = (1 - cs) /2;
	        a0 = 1 + alpha;
	        a1 = -2 * cs;
	        a2 = 1 - alpha;
	        break;
	    case HPF:
	        b0 = (1 + cs) /2;
	        b1 = -(1 + cs);
	        b2 = (1 + cs) /2;
	        a0 = 1 + alpha;
	        a1 = -2 * cs;
	        a2 = 1 - alpha;
	        break;
	    case BPF:
	        b0 = alpha;
	        b1 = 0;
	        b2 = -alpha;
	        a0 = 1 + alpha;
	        a1 = -2 * cs;
	        a2 = 1 - alpha;
	        break;
	    case NOTCH:
	        b0 = 1;
	        b1 = -2 * cs;
	        b2 = 1;
	        a0 = 1 + alpha;
	        a1 = -2 * cs;
	        a2 = 1 - alpha;
	        break;
	    case PEQ:
	        b0 = 1 + (alpha * A);
	        b1 = -2 * cs;
	        b2 = 1 - (alpha * A);
	        a0 = 1 + (alpha /A);
	        a1 = -2 * cs;
	        a2 = 1 - (alpha /A);
	        break;
	    case LSH:
	        b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
	        b1 = 2 * A * ((A - 1) - (A + 1) * cs);
	        b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
	        a0 = (A + 1) + (A - 1) * cs + beta * sn;
	        a1 = -2 * ((A - 1) + (A + 1) * cs);
	        a2 = (A + 1) + (A - 1) * cs - beta * sn;
	        break;
	    case HSH:
	        b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
	        b1 = -2 * A * ((A - 1) + (A + 1) * cs);
	        b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
	        a0 = (A + 1) - (A - 1) * cs + beta * sn;
	        a1 = 2 * ((A - 1) - (A + 1) * cs);
	        a2 = (A + 1) - (A - 1) * cs - beta * sn;
	        break;
	    default:
		 throw std::invalid_argument("BiQuadFilter type must be between 0 and 6.");
	    }

	cout << "The Filter type is " << type << ", the coefficients are:\na0: " << a0 << "\na1: " << a1 << "\nb0: " << b0 << "\nb1: " << b1 << "\nb2: " << b2 << endl;


	    /* precompute the coefficients */
    	ba0 = b0 /a0;
    	ba1 = b1 /a0;
    	ba2 = b2 /a0;
    	ba3 = a1 /a0;
    	ba4 = a2 /a0;

    	/* zero initial samples */
    	bx1 = bx2 = 0;
    	by1 = by2 = 0;

	// set parameters //////////////
	g = dbGain;

	cudaEligible_ = true;
}


//----------------------------------------------------------------------------//
BiQuadFilter::~BiQuadFilter()
{
}

//----------------------------------------------------------------------------//
m_sample_type BiQuadFilter::do_filter(m_sample_type sample)
{
	const m_sample_type result =
		ba0 * sample + ba1 * bx1 + ba2 * bx2 - ba3 * by1 - ba4 * by2;
	bx2 = bx1;
	bx1 = sample;
	by2 = by1;
	by1 = result;
	cudaEligible_ = false;
	return result;
}

//----------------------------------------------------------------------------//
void BiQuadFilter::reset()
{
	bx1 = bx2 = by1 = by2 = 0;
	cudaEligible_ = true;
}

MultiTrack* BiQuadFilter::try_filter_MultiTrack_cuda(MultiTrack& inWave)
{
#ifdef LASS_WITH_CUDA
	// Host/device transfers outweigh a CUDA launch for short sounds.
	constexpr m_sample_count_type kMinimumCudaSamples = 32768;
	m_sample_count_type totalSamples = 0;
	for (Track* track : inWave) {
		const m_sample_count_type count = track->getWave().getSampleCount();
		if (count < 0) return nullptr;
		if (totalSamples < kMinimumCudaSamples)
			totalSamples += std::min(count, kMinimumCudaSamples - totalSamples);
	}
	if (!cudaEligible_ || totalSamples < kMinimumCudaSamples ||
		!std::isfinite(ba0) || !std::isfinite(ba1) ||
		!std::isfinite(ba2) || !std::isfinite(ba3) ||
		!std::isfinite(ba4)) {
		return nullptr;
	}

	const lass_cuda::BiquadCoefficients coefficients{ba0, ba1, ba2, ba3, ba4};
	auto result = std::make_unique<MultiTrack>();
	for (Track* track : inWave) {
		SoundSample& input = track->getWave();
		const m_sample_count_type count = input.getSampleCount();
		auto output = std::make_unique<SoundSample>(count, input.getSamplingRate());
		if (count > 0 && !lass_cuda::filterBiquad(&input[0], &(*output)[0],
				static_cast<std::size_t>(count), coefficients)) {
			return nullptr;
		}
		auto newTrack = std::make_unique<Track>(output.get());
		output.release();
		result->add(newTrack.get());
		newTrack.release();
	}
	return result.release();
#else
	(void)inWave;
	return nullptr;
#endif
}

//----------------------------------------------------------------------------//
void BiQuadFilter::xml_print()
{

	cout << "\t\t<BiQuadFilter>" << endl;
	cout << "\t\t\t<g value=\"" << g << "\" />" << endl;
	cout << "\t\t</BiQuadFilter>" << endl;

}

//----------------------------------------------------------------------------//
#endif //__BI_QUAD_FILTER_CPP
