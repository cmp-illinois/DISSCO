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
//	Sound.cpp
//
//----------------------------------------------------------------------------//

#ifndef __SOUND_CPP
#define __SOUND_CPP

//----------------------------------------------------------------------------//
#include "Sound.h"
#include "Score.h"
#include "Loudness.h"

using namespace std;

//----------------------------------------------------------------------------//
Sound::Sound()
{
    setParam(DURATION, 1.0);
    setParam(START_TIME, 0.0);
    setParam(LOUDNESS, 44100.0);
    setParam(LOUDNESS_RATE, 44100.0);
    setParam(DETUNE_SPREAD, 0.0);		//0.3
    setParam(DETUNE_DIRECTION, -1.0);		//0.0
    setParam(DETUNE_VELOCITY, 0.5);		//-1.0
    setParam(DETUNE_FUNDAMENTAL, 1);		//-1.0
    filterObj = NULL;
    reverbObj = NULL;
    spatializer_ = new Spatializer();
}
  
//----------------------------------------------------------------------------//
Sound::Sound(int numPartials, m_value_type baseFreq)
{
    
    spatializer_ = new Spatializer();
    spa_modified_ = false; /* ZIYUAN CHEN, July 2023 */
    if (numPartials < 1)
    {
        cerr << "ERROR: Sound: Creation with less than 1 partial." << endl;
        return;
    }
    
    // generate each subsequent partial
    for (int i=0; i<numPartials; i++)
    {
        Partial p;
        p.setParam(RELATIVE_AMPLITUDE, (1/pow(2.71828,i)));

        // INCREMENT FREQUENCY MULTIPLIER FOR GLISSANDO
        p.setParam(FREQUENCY, baseFreq * (i+1));
        p.setParam(PARTIAL_NUM, i);
        add(p);
    }

    setParam(DURATION, 1.0);
    setParam(START_TIME, 0.0);
    setParam(LOUDNESS, 44100.0);
    setParam(LOUDNESS_RATE, 44100.0);

    setParam(DETUNE_SPREAD, 0.0);
    setParam(DETUNE_DIRECTION, -1.0);		//-1.0
    setParam(DETUNE_VELOCITY, 0.5);
    setParam(DETUNE_FUNDAMENTAL, 1.0);		//0.0

    reverbObj = NULL;

}



//----------------------------------------------------------------------------//
void Sound::setPartialParam(PartialStaticParam p, m_value_type v)
{
    Iterator<Partial> it = iterator();
    while(it.hasNext())
        it.next().setParam(p,v);
}

//----------------------------------------------------------------------------//
void Sound::setPartialParam(PartialDynamicParam p, DynamicVariable& v)
{
    Iterator<Partial> it = iterator();
    while(it.hasNext())
        it.next().setParam(p,v);
}

//----------------------------------------------------------------------------//
void Sound::setPartialParam(PartialDynamicParam p, m_value_type v)
{
    Iterator<Partial> it = iterator();
    int i = 1;

    while(it.hasNext())
    {
        m_value_type mod_val = v;

        // FOR GLISSANDO
        if( p == FREQUENCY )
	    mod_val *= i;

        it.next().setParam(p,mod_val);
        i++;  
    }
}


//----------------------------------------------------------------------------//
void Sound::setDetune(double direction, double spread, double velocity){

  if ( !(direction != -1 || direction != 1 )){
    cerr << "ERROR: Sound: out of range for DETUNE_DIRECTION." << endl;
		return;
  }

  if (spread < 0 or spread > 1){ 		
    cerr << "ERROR: Sound: out of range DETUNE_SPREAD.Should be a percent" << endl;
		return;
  }

  if ( velocity < -1 or velocity > 1 ){
    cerr << "ERROR: Sound: out of range for DETUNE_VELOCITY" << endl;
		return;
  }

  if (spread == 0 and direction == 0 and velocity == 0){
		return;
  }

  setParam(DETUNE_DIRECTION,direction);
  setParam(DETUNE_SPREAD,spread);
  setParam(DETUNE_VELOCITY, velocity);
//   setParam(DETUNE_FUNDAMENTAL, 1);
}

//----------------------------------------------------------------------------//
void Sound::showDetune(){

  cout << "\t detune direction is " << getParam(DETUNE_DIRECTION) << endl;
  cout << "\t detune spread is " << getParam(DETUNE_SPREAD) << endl;
  cout << "\t detune velocity is... " << getParam(DETUNE_VELOCITY) << endl;
  cout << "\t detune fundamental is " << getParam(DETUNE_FUNDAMENTAL) << endl;
}

//----------------------------------------------------------------------------//
/*
void Sound::getPartialParamEnv(PartialDynamicParam p)
{
    Iterator<Partial> it = iterator();
    int i = 1;

    while(it.hasNext())
    {
        m_value_type mod_val = v;

        // FOR GLISSANDO
        if( p == FREQUENCY )
	    mod_val *= i;

        it.next().getParamEnv(p);
        i++;  
    }
}
*/
//----------------------------------------------------------------------------//
MultiTrack* Sound::render(
    int numChannels,
    m_rate_type samplingRate)
    // Remember, the default value for samplingRate (as defined in the
    //  .h file is DEFAULT_SAMPLING_RATE
{
    //------------------
    // calculate loudness for this sound.
    //------------------

    // negative loudness values signify that
    // loudness is not to be calculated for this sound.
    if (getParam(LOUDNESS) >= 0)
    {
        cout << "\t Calculating Loudness..." << endl;
        //m_rate_type loudnessRate = m_rate_type(getParam(LOUDNESS_RATE));
        //Loudness::calculate(*this, loudnessRate);
        Loudness::calculate(*this);
    }

    //------------------
    // do the detuning setup 
    //------------------
    if (size() != 0) {
      cout << "\t Creating Envelopes..." << endl;
      Iterator<Partial> iter = iterator();

      if(getParam(DETUNE_FUNDAMENTAL) == 1.0){
	cout << "\t using DETUNE" << endl;
//      showDetune();

        while(iter.hasNext()){
      // create the detuning envelope for this partial

          if (getParam( DETUNE_VELOCITY) == 0.5){
	    LinearInterpolator dv;
            setup_detuning_env(&dv);
            iter.next().setParam(DETUNING_ENV,dv);
  	  } else{
            ExponentialInterpolator detuning_env;
            setup_detuning_env(&detuning_env);
            iter.next().setParam(DETUNING_ENV,detuning_env);
	  }
        }
      }
    }

    //------------------
    // render each partial, and composite into a single track.
    //------------------

    cout << "\t Rendering..." << endl;

    /*
     * duration gets tricky when you've got partials and sounds, either or both
     * of which can have different reverb applied.  'duration' refers to the
     * 'dry' duration of this sound.  This is analogous to the length of time an
     * instrument is buzzing a noise.  This is the duration of time we are
     * generating sine waves in the partial.  After 'duration', each partial may
     * or may not have reverb applied to it, and the overall sound may have
     * reverb (potentially on top -- meaning, in addition to -- the partials'
     * reverb.
     *
     * in contrast, getTotalDuration() returns the total audible length of time
     * during which this sound, its partials, and any reverb will produce noise.
     */
    m_time_type duration = getParam(DURATION);

    // compute # of samples, taking into account space for reverb die-out
    m_sample_count_type sampleCount;
    sampleCount = (m_sample_count_type) ((m_time_type)getTotalDuration() * (m_time_type)samplingRate);

    /* ZIYUAN CHEN, July 2023: Partial::render() now returns (potentially spatialized) MultiTracks */
    MultiTrack* composite;

    if (size() == 0)
    {
        // there are no partials
        // create a new empty MultiTrack:
        composite = new MultiTrack(numChannels, sampleCount, samplingRate);
        // should we zero this memory out?
    }
    else
    {
        Iterator<Partial> iter = iterator();
        composite = iter.next().render(numChannels, sampleCount, duration, samplingRate);

        MultiTrack* tempTrack;
        while(iter.hasNext())
        {
            tempTrack = iter.next().render(numChannels, sampleCount, duration, samplingRate);
            composite->composite(*tempTrack);
            delete tempTrack;
        }
    }
  
    /* Chain of Conversion:
     * Track ---do_reverb_Track-->       Track        in Partial::render()
     * ...   ---spatialize_Track-->      MultiTrack   in Partial::render()
     * ...   ---do_filter_MultiTrack-->  MultiTrack   in Sound::render()
     * ...   ---do_reverb_MultiTrack-->  MultiTrack   in Sound::render()
     * ...   ---spatialize_MultiTrack--> MultiTrack   in Sound::render()
     */
  
    // do the filter
    if (filterObj != NULL){
      cout << "\t Applying Filter..." << endl;
      MultiTrack &filteredTrack = filterObj->do_filter_MultiTrack(*composite);
		delete composite;
		composite = &filteredTrack;
    }
    
  
    // do the reverb
    if (reverbObj != NULL) {
      cout << "\t Applying Reverb..." << endl;
      MultiTrack &reverbedTrack = reverbObj->do_reverb_MultiTrack(*composite);
      delete composite;

	//------------------
	// spatialize the sound into a MultiTrack object
	//------------------

  /* ZIYUAN CHEN, July 2023: On the default behavior of spatialization (Sound side)
   *   If a non-placeholder subclass of Spatializer (Pan/MultiPan) is set in the partials,
   *     "spatializer_" in the sound will be a placeholder and should be IGNORED.
   *   Otherwise, (indirectly) calling Spatializer::spatialize_Track() will cause each
   *     track to be averaged across all channels (default placeholder behavior) and
   *     OVERWRITE any spatialization performed by the partials!
   *   Compare Partial::render().
   */
	cout << "\t Spatializing..." << endl;

	if (!spa_modified_)
		return &reverbedTrack;

	MultiTrack* mt = spatializer_->spatialize_MultiTrack(reverbedTrack, numChannels, sampleCount, samplingRate);

	// delete the temporary track object that held the unspatialized reverbed sound
	delete &reverbedTrack;

	return mt;
     }
       else
     {
	//------------------
	// spatialize the sound into a MultiTrack object
	//------------------

  /* ZIYUAN CHEN, July 2023 - See above */
	cout << "\t Spatializing..." << endl;

	if (!spa_modified_)
		return composite;

	MultiTrack* mt = spatializer_->spatialize_MultiTrack(*composite, numChannels, sampleCount, samplingRate);
	delete composite;
	return mt;
      }
}

//----------------------------------------------------------------------------//
void Sound::setSpatializer(Spatializer& s)
{
    delete spatializer_;
    spatializer_ = s.clone();
    spa_modified_ = true;
}

//----------------------------------------------------------------------------//
void Sound::use_reverb(Reverb *newReverbObj)
{
  if (reverbObj != NULL){
    delete reverbObj;
  }
	reverbObj = newReverbObj;
}


void Sound::use_filter(Filter *newFilterObj){
  if (filterObj!= NULL){
    delete filterObj;
  }
  filterObj = newFilterObj;
}


//----------------------------------------------------------------------------//
void Sound::setup_detuning_env(ExponentialInterpolator *detuning_env)
{
//cout << "               EXPONENTIAL Interpolator" << endl;

  // it does no detune/tune, return
  if(getParam(DETUNE_FUNDAMENTAL) != 1.0) {
       		return;
  }

  double x[3], y[3], spread0, spread, vel;
  double randy = (float)std::rand() / (float)RAND_MAX;

  // determine the shape of the envelope
  vel = getParam(DETUNE_VELOCITY);
  x[0] = 0.0;
  y[0] = 1.0;

  x[1] = vel * 0.95 + randy * 0.05;
  y[1] = x[1]; 
//cout << "orig x1= " <<x[1] << endl;
  x[2] = 1.0;
  y[2] = 0.0;

  // scale by the height spread of the envelope
  spread0 = getParam(DETUNE_SPREAD);
  spread = spread0 * randy * 2.0 - spread0;
/*
cout << "    randy=" << randy << endl; 
cout << 	"Final spread= " << spread << endl;
*/
  y[0] *= spread;
  y[1] *= spread * (randy * 0.5); 

  // then offset to normalize the whole thing at 1.0
  y[0] += 1.0;
  y[1] += 1.0;
  y[2] += 1.0;
	
  if(getParam(DETUNE_DIRECTION) == -1.0) // divergence (detuning)
  {
//cout << "		diverging (detuning)" << endl;
  detuning_env->addEntry(x[0], y[2]);
  detuning_env->addEntry(x[1], y[1]);
  detuning_env->addEntry(x[2], y[0]);
/*
cout << "  x0=" << x[0] << " y2=" << y[2] << endl;
cout << "  x1=" << x[1] << " y1=" << y[1] << endl;
cout << "  x2=" << x[2] << " y0=" << y[0] << endl;
//int sever; cin >> sever;
*/
  } else if(getParam(DETUNE_DIRECTION) == 1.0) {      // convergence (tuning)
//cout << "		 converging (tuning)" << endl;
  detuning_env->addEntry(x[0], y[0]);
  detuning_env->addEntry(x[1], y[1]);
  detuning_env->addEntry(x[2], y[2]);
/*
cout << "    	x0=" << x[0] << " y0=" << y[0] << endl;
cout << "    	x1=" << x[1] << " y1=" << y[1] << endl;
cout << "   	x2=" << x[2] << " y2=" << y[2] << endl;
//int sever; cin >> sever;
*/
  }

}


//----------------------------------------------------------------------------//

void Sound::setup_detuning_env(LinearInterpolator *detuning_env){
	
//cout << "		LINEAR Interpolator" << endl;

// it does no detune/tune, return
  if(getParam(DETUNE_FUNDAMENTAL) != 1.0) {
                return;
  }

  float x[3], y[3], spread0, spread, vel;
  double randy = (float)std::rand() / (float)RAND_MAX;

  // determine the shape of the envelope
  vel = getParam(DETUNE_VELOCITY);
  x[0] = 0.0;
  y[0] = 1.0;

  x[1] = (((vel*0.95)+1.0)/2.0);
  y[1] = x[1];
  x[2] = 1.0;
  y[2] = 0.0;

  // scale by the height spread of the envelope
  spread0 = getParam(DETUNE_SPREAD);
  spread = spread0 * randy * 2.0 - spread0;
/*
cout << "    randy=" << randy << endl;
cout <<         "Final spread= " << spread << endl;
*/
  y[0] *= spread;
  y[1] *= spread * (randy * 0.5);

  // then offset to normalize the whole thing at 1.0
  y[0] += 1.0;
  y[1] += 1.0;
  y[2] += 1.0;

  if(getParam(DETUNE_DIRECTION) == -1.0) // divergence (detuning)
  {
//cout << "			divergence (detuning)" << endl;
  detuning_env->addEntry(x[0], y[2]);
  detuning_env->addEntry(x[1], y[1]);
  detuning_env->addEntry(x[2], y[0]);
/*
cout << "  x0=" << x[0] << " y2=" << y[2] << endl;
cout << "  x1=" << x[1] << " y1=" << y[1] << endl;
cout << "  x2=" << x[2] << " y0=" << y[0] << endl;
*/
  } else if(getParam(DETUNE_DIRECTION) == 1.0) {      // convergence (tuning)
  
//cout << "			convergence (tuning)" << endl;
  detuning_env->addEntry(x[0], y[0]);
  detuning_env->addEntry(x[1], y[1]);
  detuning_env->addEntry(x[2], y[2]);
/*
cout << "  x0=" << x[0] << " y0=" << y[0] << endl;
cout << "  x1=" << x[1] << " y1=" << y[1] << endl;
cout << "  x2=" << x[2] << " y2=" << y[2] << endl;
*/
  }
}
//----------------------------------------------------------------------------//
m_time_type Sound::getTotalDuration(void)
{
	m_time_type curDuration, maxDuration;
	Iterator<Partial> iter = iterator();

	maxDuration = getParam(DURATION);
	while(iter.hasNext())
	{
		curDuration = iter.next().getTotalDuration(getParam(DURATION));
		if(curDuration > maxDuration)
			maxDuration = curDuration;
	}

	maxDuration += ( (reverbObj != NULL) ? reverbObj->getDecay() : 0.0);

	return maxDuration;
}

//----------------------------------------------------------------------------//
void Sound::xml_print( ofstream& xmlOutput, list<Reverb*>& revObjs, list<DynamicVariable*>& dynObjs )
{
	xmlOutput << "\t<sound>" << endl;

	// Output reverb ID and update reverb collection if necessary
	xmlOutput << "\t\t<reverb_ptr id=\"" << (long)reverbObj << "\" />" << endl;
	list<Reverb*>::const_iterator revit;
	
	for( revit=revObjs.begin(); revit != revObjs.end(); revit++ )
	{
		if( (*revit) == reverbObj )
			break;
	}
	if( revit == revObjs.end() ){
		revObjs.push_back( reverbObj );
	}

	// Static Parameters
	xmlOutput << "\t\t<duration value=\"" << getParam(DURATION) << "\" />" << endl;	
	xmlOutput << "\t\t<start_time value=\"" << getParam(START_TIME) << "\" />" << endl;
	xmlOutput << "\t\t<loudness value=\"" << getParam(LOUDNESS) << "\" />" << endl;
	xmlOutput << "\t\t<loudness_rate value=\"" << getParam(LOUDNESS_RATE) << "\" />" << endl;
	xmlOutput << "\t\t<detune_spread value=\"" << getParam(DETUNE_SPREAD) << "\" />" << endl;
	xmlOutput << "\t\t<detune_direction value=\"" << getParam(DETUNE_DIRECTION) << "\" />" << endl;
	xmlOutput << "\t\t<detune_velocity value=\"" << getParam(DETUNE_VELOCITY) << "\" />" << endl;
	xmlOutput << "\t\t<detune_fundamental value=\"" << getParam(DETUNE_FUNDAMENTAL) << "\" />" << endl;

	//Dynamic Parameters
	spatializer_->xml_print( xmlOutput );
	
	// Output XML for partials
	Iterator<Partial> it = iterator();
	while(it.hasNext())
    {
		it.next().xml_print( xmlOutput, revObjs, dynObjs );
	}

	xmlOutput << "\t</sound>" << endl;
}

//----------------------------------------------------------------------------//
void Sound::xml_read(XmlReader::xmltag* soundtag, unordered_map<long, Reverb *>* reverbHash, unordered_map<long, DynamicVariable *>* dvHash)
{
	if(strcmp("sound",soundtag->name))
	{
		printf("Not a sound tag?!  This is a %s tag!\n", soundtag->name);
		return;
	}

	char *value;
	
	if((value=soundtag->findChildParamValue("reverb_ptr","id")) != 0)
		if(Reverb* temp = (*reverbHash)[atoi(value)])
			use_reverb(temp);
	if((value = soundtag->findChildParamValue("duration","value")) != 0)
		setParam(DURATION, atof(value));
	if((value = soundtag->findChildParamValue("start_time","value")) != 0)
		setParam(START_TIME, atof(value));
	if((value = soundtag->findChildParamValue("loudness","value")) != 0)
		setParam(LOUDNESS, atof(value));
	if((value = soundtag->findChildParamValue("loudness_rate","value")) != 0)
		setParam(LOUDNESS_RATE, atof(value));
	if((value = soundtag->findChildParamValue("detune_spread","value")) != 0)
		setParam(DETUNE_SPREAD, atof(value));
	if((value = soundtag->findChildParamValue("detune_direction","value")) != 0)
		setParam(DETUNE_DIRECTION, atof(value));
	if((value = soundtag->findChildParamValue("detune_velocity","value")) != 0)
		setParam(DETUNE_VELOCITY, atof(value));
	if((value = soundtag->findChildParamValue("detune_fundamental","value")) != 0)
		setParam(DETUNE_FUNDAMENTAL, atof(value));
	
	XmlReader::xmltag *partialtag;

	while((partialtag=soundtag->children->find("partial")) != 0)
	{
		Partial p;
		p.xml_read(partialtag, reverbHash, dvHash);
		add(p);
	}
}

//----------------------------------------------------------------------------//
Sound::~Sound(){
  if(reverbObj)delete reverbObj;
  if(filterObj)delete filterObj;
  if(spatializer_) delete spatializer_;
  
}


//----------------------------------------------------------------------------//
#endif //__SOUND_H
