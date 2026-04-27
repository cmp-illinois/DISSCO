/*
LASS (additive sound synthesis library)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)
Modified by Ming-ching Chiu 2013

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
//	Score.cpp
//
//----------------------------------------------------------------------------//

#ifndef __SCORE_CPP
#define __SCORE_CPP

//----------------------------------------------------------------------------//

#include "Score.h"
#include "Types.h"

#include <memory>

//----------------------------------------------------------------------------//

// This struct passes data between the main thread and the worker threads
struct ThreadEntry{
  //pointer to the score
  Score* score;
  int threadID;
  //pointer to the vector<Sound*> sounds of the score
  vector<Sound*, std::allocator<Sound*> >*  sounds;
  int numChannels;
  int samplingRate;
  std::mutex* mutexSoundVector;
  int* soundsRendered;
  int* soundObjectsCreated;

  Semaphore* semEmptySlotsSounds;
  Semaphore* semFullSlotsSounds;
  Semaphore* semEmptySlotsRendered;
  Semaphore* semFullSlotsRendered;

  MultiTrack** scoreMultiTrack;

};


//------------------------------------------------------------------------------
void Score::add(Sound* _sound){

  // To prevent unnecessary memory use, this function is temporarily blocked
  // when there are too many sounds objects waiting to be rendered.
  // 200 is just an arbitrary number.
  // This funcion can be upgraded to block the main thread in a fancier way, but
  // for now this is enough.
  //                                           --Ming-ching May 06, 2013

  // Rubin Du July 2024: Integrated the resource management, producer thread / consumer thread synchronization, and optimized concurrency by implementing semaphores

  semEmptySlotsSounds.wait();  // Wait for an empty slot in sounds vector

  {
    std::lock_guard<std::mutex> lock(mutexSoundVector);
    sounds.push_back(_sound);
    soundObjectsCreated ++;

    //update scoreEndTime
    m_time_type soundEndTime = _sound->getParam(START_TIME) +
                                     _sound->getTotalDuration();
    if (soundEndTime > scoreEndTime) {
      scoreEndTime = soundEndTime;
      //cout<<"Score End Time Updated: "<< scoreEndTime << "seconds"<<endl;
    }
      // figure in the reverb die-out period
    if(reverbObj != NULL)
    scoreEndTime += reverbObj->getDecay();
  }

  semFullSlotsSounds.post();  // Signal that there is a full slot in sounds vector

}

//------------------------------------------------------------------------------
/**
* This function is the entry of the worker threads.
* It gets the Sound objects from the score, renders them, and put all the
* rendered sounds in renderedThreadScore.
**/
void render(ThreadEntry* _threadEntry){
  // Take ownership of the heap-allocated entry passed from the main thread.
  std::unique_ptr<ThreadEntry> owned(_threadEntry);
  ThreadEntry& threadEntry = *owned;
  while(true){
    threadEntry.semFullSlotsSounds->wait(); // Wait for a full slot in sounds vector

    //Lock the sounds vector
    threadEntry.mutexSoundVector->lock();

      if (threadEntry.sounds->size()) {
          *(threadEntry.soundsRendered) = *(threadEntry.soundsRendered) + 1;
          cout<<"Thread #"<<threadEntry.threadID<<": Rendering sound #"<<
                *(threadEntry.soundsRendered)<<" of "<<
                *(threadEntry.soundObjectsCreated)<<endl;

          Sound* sound = threadEntry.sounds->back();
          threadEntry.sounds->pop_back();
          threadEntry.mutexSoundVector->unlock();

          // Render the sound outside the critical section
          MultiTrack* renderedSound = sound->render(threadEntry.numChannels,threadEntry.samplingRate);

          threadEntry.score->addRenderedSound(sound->getParam(START_TIME), renderedSound);

          delete sound;

          threadEntry.semEmptySlotsSounds->post();  // Signal that there is an empty slot in sounds vector
      }
      else {
          threadEntry.mutexSoundVector->unlock();
          threadEntry.semFullSlotsSounds->post();  // Put back the full slot since no sound was consumed
          if (threadEntry.score->isDoneGettingSoundObjects())   // If the main thread is done adding sounds, return
            return;
      }
  }// end of main while loop

}


void composite(Score* _score){
  _score->compositeRenderedSounds();
}

//----------------------------------------------------------------------------//
Score::Score(int _numThreads, int _numChannels, int _samplingRate )
    : cmm_(NONE),
    soundsRendered(0),
    soundObjectsCreated(0),
    doneGettingSoundObjects(false),
    workerThreadsAllJoined(false),
    semEmptySlotsSounds(MAX_SOUND_OBJECTS),
    semFullSlotsSounds(0),
    semEmptySlotsRendered(MAX_RENDERED_OBJECTS),
    semFullSlotsRendered(0),
    numChannels(_numChannels),
    samplingRate(_samplingRate)
{
  scoreEndTime = 1; //start with a small number
  scoreMultiTrackLength = scoreEndTime;
  m_sample_count_type newNumSamples =
        (m_sample_count_type) (scoreMultiTrackLength * float(samplingRate));
  scoreMultiTrack = new MultiTrack
        (numChannels,newNumSamples,samplingRate);


  reverbObj = NULL;
  numThreads = _numThreads;

  //create threads for sound rendering
  threads = new std::thread[_numThreads];

  for (int i = 0; i < _numThreads; i ++){
    ThreadEntry* threadEntry = new ThreadEntry;
    threadEntry->score = this;
    threadEntry->threadID = i;
    threadEntry->sounds = &sounds;
    threadEntry->numChannels = _numChannels;
    threadEntry->samplingRate = _samplingRate;
    threadEntry->mutexSoundVector=&mutexSoundVector;
    threadEntry->soundsRendered = &soundsRendered;
    threadEntry->soundObjectsCreated = &soundObjectsCreated;
    threadEntry->scoreMultiTrack =&scoreMultiTrack;
    threadEntry->semEmptySlotsSounds = &semEmptySlotsSounds;
    threadEntry->semFullSlotsSounds = &semFullSlotsSounds;
    threadEntry->semEmptySlotsRendered = &semEmptySlotsRendered;
    threadEntry->semFullSlotsRendered = &semFullSlotsRendered;
    threads[i] = std::thread(render, threadEntry);
  }

   //start the compositeThread
   compositeThread = std::thread(composite, this);

}


void Score::addRenderedSound(m_time_type _startTime, MultiTrack* _renderedSound){

  std::pair<m_time_type, MultiTrack*>* newPair = new pair<m_time_type, MultiTrack*>(_startTime, _renderedSound);
  // active waiting for the vector size to be below 20 (if the main thread can't
  // composite the rendered sound as fast the speed of worker threads produce
  // rendered sounds, there is no need for the worker threads to rush.
    semEmptySlotsRendered.wait();  // Wait for an empty slot in renderedSounds vector

    {
      std::lock_guard<std::mutex> lock(mutexVectorRenderedSound);
      renderedSounds.push_back(newPair);
    }

    semFullSlotsRendered.post();  // Signal that there is a full slot in renderedSounds vector

}


void Score::compositeRenderedSounds(){
  while (true){
    semFullSlotsRendered.wait();

    mutexVectorRenderedSound.lock();

    if(renderedSounds.size()){
      pair<m_time_type,MultiTrack*>* thisPair = renderedSounds.back();
      renderedSounds.pop_back();
      mutexVectorRenderedSound.unlock();
      semEmptySlotsRendered.post();
      checkScoreMultiTrackLength();
      scoreMultiTrack->composite(*(thisPair->second), thisPair->first);
      delete thisPair->second;
      delete thisPair;
    }
    else{
      mutexVectorRenderedSound.unlock();
      semFullSlotsRendered.post();
      if (workerThreadsAllJoined)
        return;
    }

  }



}
//------------------------------------------------------------------------------
MultiTrack* Score::joinThreadsAndMix(){
  //Join the threads
  for (int i = 0; i < numThreads; i ++){
    threads[i].join();
    cout<< "thread Joined: Thread #"<< i <<endl;
  }
  cout<<"=======================Threads all joined.==================="<<endl;
  delete [] threads;
  workerThreadsAllJoined = true;
  semFullSlotsRendered.post(); //unblock the composite thread for proper termination

  //join the composite thread
  compositeThread.join();

  // do the reverb
  if(reverbObj != NULL)
  {
    cout << "Applying reverb to the score..." << endl;
    MultiTrack *tmp = & reverbObj->do_reverb_MultiTrack(*scoreMultiTrack);
    delete scoreMultiTrack;
    scoreMultiTrack = tmp;
  }

  // perform Clipping management on the composite:
  cout << "Managing Clipping for the score..." << endl;
  manageClipping(scoreMultiTrack, cmm_);
  // return the composite:
  return scoreMultiTrack;
}


//----------------------------------------------------------------------------//

void Score::checkScoreMultiTrackLength(){

  if ( scoreEndTime > scoreMultiTrackLength ){
      scoreMultiTrackLength = scoreEndTime;
      m_sample_count_type newNumSamples =
        (m_sample_count_type) (scoreMultiTrackLength * float(samplingRate));
      MultiTrack* newScoreMultiTrack = new MultiTrack
        (numChannels,newNumSamples,samplingRate);

      newScoreMultiTrack->composite(*scoreMultiTrack, 0);
      delete scoreMultiTrack;
      scoreMultiTrack = newScoreMultiTrack;

      //pthread_mutex_unlock( &mutexVectorRenderedSound );
      cout<<"Get a longer score with length = " << scoreEndTime << " seconds."<<endl;

  }
}


//----------------------------------------------------------------------------//

void Score::setClippingManagementMode(ClippingManagementMode mode)
{
    cout << "Score::setClippingManagementMode - " << mode << endl;
    cmm_ = mode;
}

//----------------------------------------------------------------------------//

Score::ClippingManagementMode Score::getClippingManagementMode()
{
    cout << "Score::ClippingManagementMode - cmm_ " << cmm_ << endl;
    return cmm_;
}


//----------------------------------------------------------------------------//

void Score::manageClipping(MultiTrack* mt, ClippingManagementMode mode)
{
    cout << "Score::manageClipping - mode - " << mode << endl;
    switch (mode)
    {
        case NONE:		break;
        case CLIP: 		clip(mt); break;
        case SCALE : 		scale(mt); break;
        case CHANNEL_SCALE : 	channelScale(mt); break;
        case ANTICLIP :		anticlip(mt); break;
        case CHANNEL_ANTICLIP :	channelAnticlip(mt); break;
        default : 		break;
    }
}

//----------------------------------------------------------------------------//
void Score::use_reverb(Reverb *newReverbObj)
{
	reverbObj = newReverbObj;
}

//----------------------------------------------------------------------------//
// 	PRIVATE FUNCTIONS:
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
void Score::clip(MultiTrack* mt)
{
    cout << "Performing CLIP" << endl;

    // for each track
    for (int t=0; t<mt->size(); t++)
    {
        SoundSample& wave = mt->get(t)->getWave();
        SoundSample& amp = mt->get(t)->getAmp();

        // for each sample
        m_sample_count_type numSamples = wave.getSampleCount();
        for (m_sample_count_type s=0; s<numSamples; s++)
        {
            if (wave[s] >  1.0) wave[s] =  1.0;
            if (wave[s] < -1.0) wave[s] = -1.0;
            if (amp[s] >  1.0) amp[s] =  1.0;
            if (amp[s] < -1.0) amp[s] = -1.0;
        }

    }
}


//----------------------------------------------------------------------------//

void Score::scale(MultiTrack* mt)
{
    cout << "Performing SCALE" << endl;

    // -----
    // first, find the maximum amplitude:

    m_sample_type maxAmp = 0;
    // for each track
    for (int t=0; t<mt->size(); t++)
    {
        SoundSample& amp = mt->get(t)->getAmp();
        // for each sample
        m_sample_count_type numSamples = amp.getSampleCount();
        for (m_sample_count_type s=0; s<numSamples; s++)
        {
            if (amp[s] > maxAmp) maxAmp = amp[s];
        }
    }

    // -----
    // create a scaling factor:
    m_sample_type scalingFactor = 1.0 / maxAmp;

    // -----
    // scale every value by this factor

    // for each track
    for (int t=0; t<mt->size(); t++)
    {
        SoundSample& wave = mt->get(t)->getWave();
        SoundSample& amp = mt->get(t)->getAmp();

        // for each sample
        m_sample_count_type numSamples = wave.getSampleCount();
        for (m_sample_count_type s=0; s<numSamples; s++)
        {
            wave[s] *= scalingFactor;
            amp[s] *= scalingFactor;
        }
    }
}


//----------------------------------------------------------------------------//

void Score::channelScale(MultiTrack* mt)
{
    cout << "Performing CHANNEL_SCALE" << endl;

    // -----
    // for each track
    for (int t=0; t<mt->size(); t++)
    {
        // -----
        // find the maximum amplitude:
        SoundSample& wave = mt->get(t)->getWave();
        SoundSample& amp = mt->get(t)->getAmp();

        m_sample_type maxAmp = 0;

        // for each sample
        m_sample_count_type numSamples = wave.getSampleCount();
        for (m_sample_count_type s=0; s<numSamples; s++)
        {
            if (amp[s] > maxAmp) maxAmp = amp[s];
        }

        // -----
        // create a scaling factor:
        m_sample_type scalingFactor = 1.0 / maxAmp;

        // -----
        // scale every value by this factor
        for (m_sample_count_type s=0; s<numSamples; s++)
        {
            wave[s] *= scalingFactor;
            amp[s] *= scalingFactor;
        }

    }
}


//----------------------------------------------------------------------------//

void Score::anticlip(MultiTrack* mt)
{
    cout << "Performing ANTICLIP" << endl;

    // this is a dificult one, because we need fast cros-track access.
    // make our own data-structure for this one.
    vector<SoundSample*> wave;
    vector<SoundSample*> amp;
    int numTracks = mt->size();
    for (int i=0; i<numTracks; i++)
    {
        wave.push_back(& mt->get(i)->getWave());
        amp.push_back(& mt->get(i)->getAmp());
    }

    // now, for each sample:
    m_sample_count_type numSamples = wave[0]->getSampleCount();
    for (m_sample_count_type s=0; s<numSamples; s++)
    {
        // find the total amplitude across all tracks.
        m_sample_type totalAmp = 0.0;
        for (int t=0; t<numTracks; t++)
        {
            totalAmp += (*amp[t])[s];
        }

        // scale if necessary
        if (totalAmp > 1.0)
        {
            m_sample_type scalingFactor = 1.0 / totalAmp;
            for (int t=0; t<numTracks; t++)
            {
                (*amp[t] )[s] *= scalingFactor;
                (*wave[t])[s] *= scalingFactor;
            }
        }
    }
}


//----------------------------------------------------------------------------//

m_sample_type todB(m_sample_type x)
{
  return (m_sample_type)(log10((double)x) * 20.0);
}


//----------------------------------------------------------------------------//

m_sample_type fromdB(m_sample_type x)
{
  return (m_sample_type)pow(10.0, ((double)x * 0.05));
}


//----------------------------------------------------------------------------//

m_sample_type compressSound(m_sample_type x, m_sample_type peak,
  m_sample_type dBCompressionPoint)
{
/*				sever commented out 6/11/16
  m_sample_type xdB = todB(x);
  m_sample_type cdB = dBCompressionPoint;
  m_sample_type pdB = todB(peak);
*/

  if(x < fromdB(dBCompressionPoint))
    return x;

      m_sample_type c = fromdB(dBCompressionPoint);
      x = 1 - (peak - x) * (1 - c) / (peak - c);	//sever 6/11/16
//    x = fromdB((pdB - xdB) * (pdB * xdB - cdB * cdB) / ((cdB - pdB) * (cdB - pdB)));		//Andrew Burnson
      return x;

//return fromdB((pdB - xdB) * (pdB * xdB - cdB * cdB) /
//  ((cdB - pdB) * (cdB - pdB)));
}


//----------------------------------------------------------------------------//

void Score::channelAnticlip(MultiTrack* mt)
{
    cout << "Performing CHANNEL_ANTICLIP" << endl;

    // for each track
    m_sample_type maxAmplitude = 0.0;
    int peakPlace = 0;
    for (int t=0; t<mt->size(); t++)
    {
	// cout << "Score::channelAnticlip - first time track=" << t << endl;
        SoundSample& wave = mt->get(t)->getWave();
        SoundSample& amp = mt->get(t)->getAmp();

        // for each sample
        m_sample_count_type numSamples = wave.getSampleCount();
        for (m_sample_count_type s=0; s<numSamples; s++)
        {

            if (amp[s] > 1.0) {
/* 				deprecated
                scale this sample:
                wave[s] *= 1.0 / amp[s];
                amp[s] = 1.0;
*/
            }

            m_sample_type cur = wave[s];
            if(cur < 0) cur = -cur;
            if(cur > maxAmplitude)
            {
              maxAmplitude = cur;
              peakPlace = s;
            }
        }
    }

    if(maxAmplitude <= 0.99)
    {
      cout << "Peak at " << maxAmplitude << endl;
      //cout << "Normalizing to achieve better signal-to-noise ratio.";
    }
    else if(maxAmplitude >= 0.99)
    {
      cout << "Warning: peak is " << todB(maxAmplitude) << " dB at " <<
        ((double)peakPlace / (double)mt->get(0)->getWave().getSamplingRate())
           << " seconds. Compressing [-6, " << todB(maxAmplitude)
           << ") to [-6, 0) dB" << endl;
     maxAmplitude /= 0.99; //Never actually allow it to hit 0dB.

      //m_sample_type normalizeValue = maxAmplitude;
      for (int t=0; t<mt->size(); t++)
      {
          SoundSample& wave = mt->get(t)->getWave();
          SoundSample& amp = mt->get(t)->getAmp();

          // for each sample
          m_sample_count_type numSamples = wave.getSampleCount();
          for (m_sample_count_type s=0; s<numSamples; s++)
          {
              amp[s] = 1.0;

 	    if (wave[s] < 0) {
	      wave[s] *= -1;
              wave[s] = compressSound(wave[s], maxAmplitude, -6.0);
	      wave[s] *= -1;
            } else {
              wave[s] = compressSound(wave[s], maxAmplitude, -6.0);
	    }

/*
             cout << "Score::channelAnticlip - wave[" << s << "]= "
	      << wave[s] << endl;
`
              if(s == 0)
          cout << "Score::channelAnticlip Test - showing compression curve used:"
               << endl;
              wave[s] = compressSound((float)s / (float)numSamples * 2.f, 2.f, -6.0);
*/
          }
      }
    }
}


//----------------------------------------------------------------------------//

MultiTrack* Score::doneAddingSounds(){

  {
    std::lock_guard<std::mutex> lock(mutexSoundVector);
    doneGettingSoundObjects= true;
    semFullSlotsSounds.post(); // unblock the worker threads for proper termination
  }

  return joinThreadsAndMix();

}

//----------------------------------------------------------------------------//
#endif //__SCORE_CPP
