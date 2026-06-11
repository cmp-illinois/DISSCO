/*
   CMOD (composition module)
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
//   Bottom.cpp
//
//----------------------------------------------------------------------------//

#include "Bottom.h"
#include "Random.h"
#include "Output.h"
static int test=0;


//----------------------------------------------------------------------------//

Bottom::Bottom(pugi::xml_node _element,
               TimeSpan _timeSpan,
               int _type,
               Tempo _tempo,
               Utilities* _utilities,
               pugi::xml_node _ancestorSpa,
               pugi::xml_node _ancestorRev,
               pugi::xml_node _ancestorFil,
               pugi::xml_node _ancestorModifiers):
  Event(_element, _timeSpan,_type, _tempo, _utilities, {},{},{},{}),
  ancestorModifiersElement(_ancestorModifiers){


  pugi::xml_node extraInfo = descendantByName(_element, "ExtraInfo");

  /*
  <ExtraInfo>
    <FrequencyInfo>
      <FrequencyFlag>0</FrequencyFlag>
      <FrequencyContinuumFlag>0</FrequencyContinuumFlag>
      <FrequencyEntry1>3</FrequencyEntry1>
      <FrequencyEntry2/>
    </FrequencyInfo>
    <Loudness>4</Loudness>
    <Spatialization>5</Spatialization>
    <Reverb>6</Reverb>
    <Filter>f</Filter>
    <ModifierGroup></ModifierGroup>
    <Modifiers>
    </Modifiers>
  </ExtraInfo>
  */

  frequencyElement = GFEC(extraInfo);
  loudnessElement = GNES(frequencyElement);
  if (_ancestorSpa != NULL){
    spatializationElement = _ancestorSpa;
  }
  else {
    spatializationElement = GNES(loudnessElement);
  }

  if (_ancestorRev != NULL){
    reverberationElement = _ancestorRev;
  }
  else {
    reverberationElement = GNES(GNES(loudnessElement));
  }

  if (_ancestorFil != NULL){
    filterElement = _ancestorFil;
  }
  else {
    filterElement = GNES(GNES(GNES(loudnessElement)));
  }

  /* ZIYUAN CHEN, July 2023 */
  modifierGroupElement = GNES(GNES(GNES(GNES(loudnessElement))));

  modifiersElement = GNES(GNES(GNES(GNES(GNES(loudnessElement)))));

}


//----------------------------------------------------------------------------//

Bottom::~Bottom() {
  //do nothing
}

//----------------------------------------------------------------------------//

void Bottom::buildChildren(){

  if (utilities->getOutputParticel()){
    //Begin this sub-level in the output and write out its properties.
    Output::beginSubLevel(name);
    outputProperties();
  }
  //Build the event's children.

  string method = XMLTC(methodFlagElement);

  //Set the number of possible restarts (for buildDiscrete)
  restartsRemaining = restartsNormallyAllowed;

  //Make sure that the temporary child events array is clear.
  if(temporaryChildEvents.size() > 0) {
    cerr << "WARNING: temporaryChildEvents should not contain data." << endl;
    cerr << "There may be a bug in the code. Please report." << endl;
    exit(1);
  }

  //Create the child events.
  for (currChildNum = 0; currChildNum < numChildren; currChildNum++) {
    if (method == "0") //continuum
      checkEvent(buildContinuum());
    else if (method == "1"){ //sweep

//      bool buildSweepSuccess = buildSweep();
//      cout<<endl;  // Ming-ching: I don't know why the program crash without this line... very odd..
//      checkEvent(buildSweepSuccess);

      //the three lines above is simply:
      checkEvent(buildSweep());

    }
    else if (method == "2") {  //discrete
      checkEvent(buildDiscrete());
    }
    else {
      cerr << "Unknown build method: " << method << endl << "Aborting." << endl;
      exit(1);
    }
  }

  //Using the temporary events that were created, construct the actual children.
  //The code below is different from buildchildren in Event class.
  for (unsigned i = 0; i < childSoundsAndNotes.size(); i++) {
    SoundAndNoteWrapper* thisChild = childSoundsAndNotes[i];
    //Increment the static current child number.
    currChildNum = i;
    constructChild(thisChild);
    delete thisChild;
  }

/*
  //Clear the temporary event list.
  childSoundsAndNotes.clear();
*/

  if (utilities->getOutputParticel()){
  //End this output sublevel.
    Output::addProperty("Updated Tempo Start Time", tempo.getStartTime());
    Output::endSubLevel();
  }
}

//---------------------------------------------------------------------------//

void Bottom::modifyChildren(){            //Incomplete Override

  //Randomly modify elements

   //const short unsigned num = 15 ;
   //const short unsigned* loudness_val = &num;


   //loudnessElement->setNodeValue(loudness_val);


}

//---------------------------------------------------------------------------//

//----------------------------------------------------------------------------//

void Bottom::constructChild(SoundAndNoteWrapper* _soundNoteWrapper) {
  //Just to get the checkpoint. Not used any other time.
  checkPoint = (_soundNoteWrapper->ts.start - ts.start) / ts.duration;
  if (name.substr(0,1) == "s"){
    // buildNote(_soundNoteWrapper);
    buildSound(_soundNoteWrapper);
    return;
  }
  else if (name.substr(0,1) == "n"){
    buildNote(_soundNoteWrapper);
    return;
  }
  // else if (name.substr(0,2) == "ns" || name.substr(0,2) == "sn"){
  //   buildSound(_soundNoteWrapper);
  //   buildNote(_soundNoteWrapper);
  //   return;
  // }
}

//----------------------------------------------------------------------------//

void Bottom::buildSound(SoundAndNoteWrapper* _soundNoteWrapper) {
  //Create a new sound object.
  Sound* newSound = new Sound();

  if (utilities->getOutputParticel()){
    //Output sound related properties.
    Output::beginSubLevel("Sound");
    Output::addProperty("Name", _soundNoteWrapper->name);
    Output::addProperty("Type", _soundNoteWrapper->type);
    Output::addProperty("Start Time", _soundNoteWrapper->ts.start, "sec.");
      Output::addProperty("End Time", _soundNoteWrapper-> ts.start +
        _soundNoteWrapper->ts.duration, "sec.");
      Output::addProperty("Duration",_soundNoteWrapper-> ts.duration, "sec.");
  }

  //Set the start time and duration from the timespan.
  newSound->setParam(START_TIME, _soundNoteWrapper->ts.start);
  newSound->setParam(DURATION, _soundNoteWrapper->ts.duration);

  //Set the frequency.
  float baseFrequency = computeBaseFreq();
  if (utilities->getOutputParticel())Output::addProperty("Base Frequency", baseFrequency, "Hz");

  //Set the loudness.
  float loudSones = computeLoudness();
  newSound->setParam(LOUDNESS, loudSones);
  if (utilities->getOutputParticel())Output::addProperty("Loudness", loudSones, "sones");

  int numPartials = computeNumPartials( baseFrequency ,_soundNoteWrapper->element );

 //Element for GenSpectrum
  pugi::xml_node specElement = GNES(GNES(GNES(GNES(GFEC(_soundNoteWrapper->element)))));

  //if there is no spec element,go to partial routine;else, go to gen_spectrum
  if (GFEC(specElement) != NULL){
    pugi::xml_node partialEnvElement = utilities->evaluateSpectrumElement(XMLTC(specElement), (void*)this);
    pugi::xml_node distanceElement = GNES(partialEnvElement);

    Envelope* waveShape = (Envelope*) utilities->evaluateObject(XMLTC(partialEnvElement),(void*)this, eventEnv );
    float distance = utilities->evaluate(XMLTC(distanceElement),(void*)this);

    //instead of reading partials, generate spectrum envelope and add to sound
    generatePartials(newSound, baseFrequency, loudSones, distance, waveShape);
    delete waveShape;
  }
  else{
      //Set the number of partials.
      if (utilities->getOutputParticel())Output::beginSubLevel("Partials");
      if (utilities->getOutputParticel())
    	Output::addProperty("Deviation",
    		computeDeviation(_soundNoteWrapper->element), "normalized");

      for (int i = 0; i < numPartials; i++) {
        currPartialNum = i; //added by ming 20130425

        //Create the next partial object.
        Partial partial;

        //Set the partial number of the partial based on the current index.
        partial.setParam(PARTIAL_NUM, i);

        //Compute the deviation for partials above the fundamental.
        double deviation = 0;

        if(i != 0)
          deviation = computeDeviation(_soundNoteWrapper->element );

        //Set the frequencies for each partial.
        float actualFrequency = setPartialFreq(
          partial, deviation, baseFrequency, i);

        //Report the actual frequency.
        stringstream ss; if(i != 0) ss << "Partial " << i; else ss << "Fundamental";
        if (utilities->getOutputParticel())
    	Output::addProperty(ss.str(), actualFrequency, "Hz");
        //Set the spectrum for this partial.
        setPartialSpectrum(partial, i, _soundNoteWrapper->element);

        //Add the partial to the sound.
        newSound->add(partial);

      }
   }

  if (utilities->getOutputParticel())Output::endSubLevel();

  //Apply the modifiers to the sound.
  applyModifiers(newSound, numPartials);

  //Apply the spatialization to the sound.
  applySpatialization(newSound, numPartials);

  //apply the filter to the sound
  applyFilter(newSound);

  //Apply the reverberation to the sound.
  //applyReverberation(newSound);
  // EXPERIMENTAL: Added option of applying reverb by partial
  applyReverberation(newSound, numPartials);

  if (utilities->getOutputParticel())Output::endSubLevel();

  // at this point the ownership of the sound is given to the utilities.
  // utiliise will then transfer it to the Score object.
  utilities->addSound(newSound);

}

//----------------------------------------------------------------------------//

void Bottom::buildNote(SoundAndNoteWrapper* _soundNoteWrapper) {
  //Create the note.
  Note* newNote = new Note(tsChild, tempo.getRootExactAncestor());
  if (utilities->getOutputParticel()){
  //Output note-related properties.
    Output::beginSubLevel("Note");
    Output::addProperty("Name", _soundNoteWrapper->name);
    Output::addProperty("Type", _soundNoteWrapper->type);
    Output::addProperty("Start Time", _soundNoteWrapper->ts.start, "sec.");
    Output::addProperty("End Time", _soundNoteWrapper-> ts.start +
	                      _soundNoteWrapper->ts.duration, "sec.");
    Output::addProperty("Duration",_soundNoteWrapper-> ts.duration, "sec.");
    Output::addProperty("Tempo Start Time",
	                      _soundNoteWrapper->tempo.getStartTime(), "sec.");
    Output::addProperty("EDU Start Time",
	                      _soundNoteWrapper->ts.startEDU.toPrettyString(), "EDU");
    Output::addProperty("EDU Start Time Absolute", 
                        _soundNoteWrapper->ts.startEDUAbsolute, "EDU");
    Output::addProperty("EDU Duration",
	                      _soundNoteWrapper->ts.durationEDU.toPrettyString(), "EDU");
  }

//set loudness
  float loudfloat = computeLoudness();
  newNote->setLoudnessSones(loudfloat);

// multistaffs
  //<NoteInfo>
  //  <Staffs>...<\staffs>
  //  <Modifiers>...<\Modifiers>
  //<\NoteInfo>
  // multistaffs
  pugi::xml_node noteInfo = GNES(GNES(GFEC(_soundNoteWrapper->element)));
  pugi::xml_node staffsInfo = GFEC(noteInfo);
  pugi::xml_node modifiersInfo = GNES(GFEC(noteInfo));
//set modifiers
  vector<string> noteMods = applyNoteModifiers(modifiersInfo);
  newNote->setModifiers(noteMods);
// set childStaff
  int noteStaff = utilities->evaluate(XMLTC(staffsInfo),(void*)this);
  // int noteStaff = applyNoteStaffs(_soundNoteWrapper->element);
  newNote->setStaffNum(noteStaff);

  //Set the pitch.
  float baseFrequency = computeBaseFreq();

  int absPitchNum;

  if(wellTempPitch <= 0) { 		//if frequency is in Hertz
    absPitchNum = newNote->HertzToPitch(baseFrequency);
  } else {
    absPitchNum = wellTempPitch;
  }
  newNote->setPitchWellTempered(absPitchNum);

  // Set notation start, start absolute, and end times in edus
  newNote->setStartTime(_soundNoteWrapper->ts.startEDU.To<int>());
  newNote->setEndTime(
    _soundNoteWrapper->ts.startEDU.To<int>() + 
      _soundNoteWrapper->ts.durationEDU.To<int>());
  // Initialize the parameter split before the arrangement
  newNote->initSplit();
  
  // multistaffs
  //Output::notation_score_.RegisterTempo(tempo);
  Output::notation_score_.RegisterTempo(tempo,newNote->getStaffNum());
  Output::notation_score_.InsertNote(newNote);

  if (utilities->getOutputParticel()){
      Output::endSubLevel();
  }

  childNotes.push_back(newNote);
}


//----------------------------------------------------------------------------//

list<Note> Bottom::getNotes() {
  list<Note> result;
  for(unsigned i = 0; i < childNotes.size(); i++)
    result.push_back(*childNotes[i]);
  return result;
}

//----------------------------------------------------------------------------//

float Bottom::computeBaseFreq() {

  float baseFreqResult;
  pugi::xml_node freqFlagElement = GFEC(frequencyElement);
  pugi::xml_node continuumFlagElement = GNES(freqFlagElement);
  pugi::xml_node valueElement = GNES(continuumFlagElement);
  pugi::xml_node valueElement2 = GNES(valueElement);
  if (utilities->evaluate(XMLTC(freqFlagElement),(void*) this)==2) {//contiruum
    /* 2nd arg is a string (HERTZ or POW2) */

    if (utilities->evaluate(XMLTC(continuumFlagElement), NULL)==0) { //Hertz
      baseFreqResult = utilities->evaluate(XMLTC(valueElement), (void*)this);
      /* 3rd arg is a float (baseFreq in Hz) */
    }
    else  {//power of 2
      /* 3rd arg is a float (power of 2) */
      float step = utilities->evaluate(XMLTC(valueElement), (void*)this);
      if(step <= log2(MINFREQ/C0) || step >= log2(CEILING/C0)) {
        cerr << "BaseFreq: power of 2 out of range: " << step << endl;
        cerr << "	log2(MINFREQ/C0) > step < log2(CEILING/C0)" << endl;
      }
      baseFreqResult = C0 * pow(2, step);
    }
  } else if (utilities->evaluate(XMLTC(freqFlagElement), (void*) this)==0) { //equal tempered
    /* 2nd arg is an int */
    wellTempPitch = utilities->evaluate(XMLTC(valueElement), (void*)this);
    baseFreqResult = C0 * pow(WELL_TEMP_INCR, wellTempPitch);
  } else  {// fundamental
    /* 2nd arg is (float)fundamental_freq, 3rd arg is (int)overtone_num */
    float fund_freq = utilities->evaluate(XMLTC(valueElement), (void*)this);
    int overtone_step = utilities->evaluate(XMLTC(valueElement2), (void*)this);
    baseFreqResult = fund_freq * overtone_step;
  }
  return baseFreqResult;
}

//----------------------------------------------------------------------------//

float Bottom::computeLoudness() {
//   float expVal = 0;
//   for(int i = 0; i < 10; i++){
//       expVal += utilities->evaluate(XMLTC(loudnessElement), (void*)this);
//   }
  // expVal /= 10;
  float loudval = utilities->evaluate(XMLTC(loudnessElement), (void*)this);
  // float diff = loudval - expVal;
  // loudval -= 0.4 * diff;
  // cout << "bottom loudness: " << loudval << endl;
  // cout << "bottom expval: " << expVal << endl;
  return loudval;
}

//----------------------------------------------------------------------------//

int Bottom::computeNumPartials(float baseFreq, pugi::xml_node _spectrum) {

  pugi::xml_node numPartialElement = GNES(GNES(GFEC(_spectrum)));
  int numPartsResult = utilities->evaluate(XMLTC(numPartialElement), (void*) this);

  // Decrease numPartials until p < CEILING
  // (CEILING is a global def from define.h)
  while(numPartsResult * baseFreq > CEILING) {
    numPartsResult--;
  }

  if(numPartsResult <= 0) {
    cerr << "Error: Bottom::computeNumPartials got 0, baseFrequency="
         << baseFreq << endl;
    exit(1);
  }

  return numPartsResult;
}

//----------------------------------------------------------------------------//

float Bottom::computeDeviation( pugi::xml_node _spectrum) {
  pugi::xml_node devElement = GNES(GNES(GNES(GFEC(_spectrum))));
  return utilities->evaluate(XMLTC(devElement), (void*)this);
}

//----------------------------------------------------------------------------//

float Bottom::setPartialFreq(Partial& part, float deviation, float baseFreq, int partNum) {

  // assign frequency to each partial
  float pDev = deviation * (Random::Rand() - 0.5) * 2;
  float pFreq = baseFreq * ((partNum + 1) + pDev);

  // if pFreq is out of range then set it to the closer of the max or min value
  if(pFreq < MINFREQ) {
    pFreq = MINFREQ;
  } else if(pFreq > CEILING) {
    pFreq = CEILING;
  }

  part.setParam(FREQUENCY, pFreq);
  return pFreq;
}

//----------------------------------------------------------------------------//

void Bottom::setPartialSpectrum(Partial& part, int partNum, pugi::xml_node _element) {

  pugi::xml_node partialEnvElement = GFEC(GNES(GNES(GNES(GNES(GNES(GFEC(_element)))))));

    int counter = partNum;
    while(counter != 0){
      partialEnvElement=GNES(partialEnvElement);
      counter--;
    }
    Envelope* waveShape = (Envelope*) utilities->evaluateObject(XMLTC(partialEnvElement),(void*)this, eventEnv );
    part.setParam(WAVE_SHAPE, *waveShape );
    delete waveShape;
}

//----------------------------------------------------------------------------//

void Bottom::applySpatialization(Sound* s, int numPartials) {

  pugi::xml_node SPAElement = utilities->evaluateSpa((void*) this); //this call will return a spa function, just in case users use "select" here.


//  <Fun>
//    <Name>SPA</Name>
//    <Method>STEREO</Method>
//    <Apply>SOUND</Apply>
//    <Channels>  bla bla bla ... </Channels>
//  </Fun>
  pugi::xml_node methodElement = GNES(GFEC(SPAElement));
  string method = XMLTC(methodElement);

  pugi::xml_node applyHowElement = GNES(methodElement);
  string apply = XMLTC(applyHowElement);

  pugi::xml_node channelsElement = GNES(applyHowElement);

  if (method.compare("STEREO")==0) {
    //will be a list of envs, of length 1 if applyhow == SOUND, or
    //  else if applyhow == PARTIAL, it will be numpartials in length
    spatializationStereo(s, channelsElement, apply, numPartials);

  } else if (method.compare("MULTI_PAN")==0) {
    //will be a list of lists of envs ... number of items in
    // outerlist = NumChannels, items in inner lists = NumPartials

    spatializationMultiPan(s, channelsElement, apply, numPartials);

  } else if (method.compare("POLAR")==0) {
    //will be 2 lists of envelopes, both NumPartials in length

    spatializationPolar(s, channelsElement, apply, numPartials);

  } else {
    cout << "spat_method = " << method << endl;
    cout << "SOUND_SPATIALIZATION has invalid method!  Use STEREO, MULTI_PAN, or POLAR" << endl;
    exit(1);
  }

}

//----------------------------------------------------------------------------//

void Bottom::spatializationStereo(Sound *s,
                                  pugi::xml_node _channels,
                                  string applyHow,
                                  int numParts) {
//  <Channels>
//    <Partials>
//      <P><Fun><Name>EnvLib</Name><Env>1</Env><Scale>1.0</Scale></Fun></P>
//    </Partials>
//  </Channels>

  pugi::xml_node envelopeElement = GFEC(GFEC(_channels));
  string envstr;

  if (applyHow == "SOUND") {
    envstr = XMLTC(envelopeElement);
    if (envstr == "") {
      cerr << "WARNING: spatializationStereo got empty envelope for sound; ignoring" << endl;
        // ^ this cannot be wrapped into computeSpatializationStereo
        // since returning an empty Pan object is ambiguous
        // but refactoring the function to return a pointer introduces memory hazards
    } else {
      Pan stereoPan = computeSpatializationStereo(envstr);
      s->setSpatializer(stereoPan);
    }
  }

  else if (applyHow == "PARTIAL") {
    for (int i = 0; i < numParts; i++) {
      envstr = XMLTC(envelopeElement);
      if (envstr == "") {
        cerr << "WARNING: spatializationStereo got empty envelope for partial " << i << "; ignoring" << endl;
      } else {
        Pan stereoPan = computeSpatializationStereo(envstr);
        s->get(i).setSpatializer(stereoPan);
        // ^ this cannot be wrapped into computeSpatializationStereo
        // since Sound and Partial does not inherit each other
      }
      envelopeElement = GNES(envelopeElement);
    }

  }
  else {
    cerr << "Error: " << applyHow << " is an invalid way to apply spatialization! "
         << "Use SOUND or PARTIAL" << endl;

  }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
Pan Bottom::computeSpatializationStereo(string envstr) {

  Envelope* panning = (Envelope*)utilities->evaluateObject(envstr, (void*)this, eventEnv);
  Pan stereoPan(*panning);
  delete panning;
  return stereoPan;

}

//----------------------------------------------------------------------------//

void Bottom::spatializationMultiPan(Sound *s,
                                    pugi::xml_node _channels,
                                    string applyHow,
                                    int numParts) {


//  <Channels>
//    <Partials> <!-- channel 1 -->
//      <P><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></P>
//    </Partials>
//    <Partials> <!-- channel 2 -->
//      <P><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></P>
//    </Partials>
//    <Partials> <!-- channel 3 -->
//      <P><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></P>
//    </Partials>
//  </Channels>

  vector< vector<Envelope*> > mults;
  vector<bool> isPartialValid;
  string envstr;
  Envelope* env;
  pugi::xml_node partials = GFEC(_channels);
  pugi::xml_node envElement;

  unsigned j; // index of partials

  // populate mults, essentially "transposing" the grid of envelopes
  while (partials!=NULL){
    envElement = GFEC(partials);
    j = 0;
    while (envElement != NULL) { // for each partial
      if (j >= mults.size()) {
        // populate mults with "bins", only effective in the first go
        mults.push_back(vector<Envelope*>());
        isPartialValid.push_back(true); // initialize the flags
      }
      envstr = XMLTC(envElement);
      if (envstr == "") {
        isPartialValid.at(j) = false; // missing one channel disables the entire partial
      } else {
        env = (Envelope*)utilities->evaluateObject(envstr, (void*)this, eventEnv);
        mults.at(j).push_back(env);
      }
      envElement = GNES(envElement);
      j++;
    }
    partials = GNES(partials);
  }

  if (applyHow == "SOUND") {
    if (isPartialValid.at(0) == false) {
      cerr << "WARNING: spatializationMultiPan got empty envelope for sound; ignoring" << endl;
      return;
    }
    MultiPan multipan = computeSpatializationMultiPan(mults.at(0));
    s->setSpatializer(multipan);

  } else if (applyHow == "PARTIAL") {
    for (unsigned i = 0; (int)i < numParts; i++) { // apply multipan to each partial
      if (mults.size() <= i){
        cout << "WARNING: spatializationMultiPan got empty envelopes for partial " << i << " and onwards; ignoring" << endl;
        break;
      }
      if (isPartialValid.at(i) == false) {
        cerr << "WARNING: spatializationMultiPan got empty envelope for partial " << i << "; ignoring" << endl;
        continue;
      }
      MultiPan multipan = computeSpatializationMultiPan(mults.at(i));
      s->get(i).setSpatializer(multipan);
    }

  }
  else {
    cerr << "Error: " << applyHow << " is an invalid way to apply spatialization! "
         << "Use SOUND or PARTIAL" << endl;

  }
}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
MultiPan Bottom::computeSpatializationMultiPan(vector<Envelope*> mult) {

  MultiPan multipan(mult.size(), mult);

  for (unsigned i = 0; i < mult.size(); i++) {
    delete mult[i];
  }

  return multipan;

}

//----------------------------------------------------------------------------//

void Bottom::spatializationPolar(Sound *s,
                                 pugi::xml_node _channels,
                                 string applyHow,
                                 int numParts) {
//<Channels>
//  <Partials>         This element is actually the Theta
//    <P><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></P>
//  </Partials>
//  <Partials>         This element is the Radius
//    <P><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></P>
//  </Partials>
//</Channels>

  pugi::xml_node thetaElement = GFEC(GFEC(_channels));
  pugi::xml_node radiusElement = GFEC(GNES(GFEC(_channels)));
  string theta, radius;

  if (applyHow == "SOUND") {
    theta = XMLTC(thetaElement);
    radius = XMLTC(radiusElement);
    if (theta == "" || radius == "") {
      cerr << "WARNING: spatializationPolar got empty envelope for sound; ignoring" << endl;
    } else {
      MultiPan multipan = computeSpatializationPolar(theta, radius);
      s->setSpatializer(multipan);
    }
  }

  else if (applyHow == "PARTIAL") {
    for (int i = 0; i < numParts; i++) {
      theta = XMLTC(thetaElement);
      radius = XMLTC(radiusElement);
      if (theta == "" || radius == "") {
        cerr << "WARNING: spatializationPolar got empty envelope for partial " << i << "; ignoring" << endl;
      } else {
        MultiPan multipan = computeSpatializationPolar(theta, radius);
        s->get(i).setSpatializer(multipan);
      }
      thetaElement = GNES(thetaElement);
      radiusElement = GNES(radiusElement);
    }

  }
  else {
    cerr << "Error: " << applyHow << " is an invalid way to apply spatialization! "
         << "Use SOUND or PARTIAL" << endl;

  }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
MultiPan Bottom::computeSpatializationPolar(string thetaEnvStr, string radiusEnvStr) {

  Envelope* thetaEnv;
  Envelope* radiusEnv;

  thetaEnv = (Envelope*)utilities->evaluateObject(thetaEnvStr, (void*)this,eventEnv);
  radiusEnv = (Envelope*)utilities->evaluateObject(radiusEnvStr, (void*)this,eventEnv);

  MultiPan multipan(utilities->getNumberOfChannels());
  float time, theta, radius;

    // take 100 samples of the envelopes and apply them to the sound
    // (this should be enough to catch the important parts of the env)
    int numPolarSamples = 100;
    //cout << "TIME    THETA   RADIUS" << endl;
    for (int i = 0; i <= numPolarSamples; i++) {
      time = (float)i / numPolarSamples;
      theta = PI * thetaEnv->getScaledValueNew(time, 1.0);
      radius = radiusEnv->getScaledValueNew(time, 1.0);

      multipan.addEntryLocation(time, theta, radius);
      //if (i % 5 == 0) {
      //  cout << setw(6) << time << "  ";
      //  cout << setw(6) << theta << " ";
      //  cout << setw(6) << radius << "  " << endl;
      //}
    }

    multipan.doneAddEntryLocation();

  delete thetaEnv;
  delete radiusEnv;

  return multipan;

}

//----------------------------------------------------------------------------//

void Bottom::applyFilter(Sound* s){
  pugi::xml_node filterElement = utilities->evaluateFil((void*) this);
  if (filterElement == NULL) return; //no filter

//  <Fun>
//    <Name>MakeFilter</Name>
//    <Type>HPF</Type>
//    <Frequency>2000</Frequency>
//    <BandWidth>4.5</BandWidth>
//    <dBGain/>
//  </Fun>
  pugi::xml_node it = GNES(GFEC(filterElement));
  string type = XMLTC(it);
  it = GNES(it);
  double frequency = utilities->evaluate(XMLTC(it), (void*)this);
  it = GNES(it);
  double bandWidth = utilities->evaluate(XMLTC(it), (void*)this);
  it = GNES(it);
  double gain = utilities->evaluate(XMLTC(it), (void*)this);

  int typeInt;
  if (type =="LPF") typeInt = 0;
  else if (type == "HPF") typeInt =1;
  else if (type == "BPF") typeInt =2;
  else if (type == "NF") typeInt =3;
  else if (type == "PBEQF") typeInt =4;
  else if (type == "LSF") typeInt =5;
  else if (type == "HSF") typeInt =6;
  else {
    cout<<"Filter Type not recognized."<<endl;
    return;
  }

  /**
   *Usage:(From Mert Bay's BiQuadFilter.cpp)
  **/
  /**
   *	BiQuadFilter(int type, m_sample_type dbGain,  m_sample_type freq,  m_sample_type srate,   m_sample_type bandwidth);
   * Where arguments are
   * 1)Filter type: 0-6:
	*0-Low Pass Filter,
	*1-High Pass Filter
	*2-Band Pass Filter
	*3-Notch Filter
	*4-Peaking Band EQ filter
	*5-Low Shelf Filter
	*6-High Shelf Filter
   * 2) dbGain: Filters gain (dB) for peaking and shelving filters only
   * 3) Cutoff Frequency (hz)
   * 4) Sampling Rate (samples/sc)
   * 5) Bandwidth  (in octaves)
   *
  **/

  BiQuadFilter *filterObj= new BiQuadFilter(
        typeInt,
        gain,
        frequency,
        utilities->getSamplingRate(),
        bandWidth);

  s->use_filter(filterObj);
}


//----------------------------------------------------------------------------//

// TEJUS, ZIYUAN CHEN: applyReverberation, for sound or partials
void Bottom::applyReverberation(Sound *s, int numPartials) {
  // Sample XML string:
  // <Reverb>
  //   <Fun>
  //     <Name>REV_Simple</Name>
  //     <Apply>PARTIAL</Apply>
  //     <Sizes>
  //       <Size><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></Size>
  //       <Size>0.2</Size>
  //       <Size>0.3</Size>
  //     </Sizes>
  //   </Fun>
  // </Reverb>
	
  // May need to modify utilities...
  pugi::xml_node reverbElement = utilities->evaluateRev((void*) this);

//this call will return a rev function, just in case users use "select" here.
//The string here is just a dummy since the callee will find the right rev
//element  within "this".

  // Assume this correctly gets <Name>
  string rev_method =  XMLTC(GFEC(reverbElement));

  pugi::xml_node applyHowElement = GNES(GFEC(reverbElement));
  string rev_apply = XMLTC(applyHowElement);

	// Number of parameters varies between methods. But unlike spatialization,
	// this "everything else" part is NOT enclosed in a <Channel> element;
	// instead, they are listed in the same level under "Fun"
  pugi::xml_node paramsElement = GNES(applyHowElement);

  if (rev_method.compare("REV_Simple") == 0) {
    reverberationSimple(s, paramsElement, rev_apply, numPartials);
  }

  else if (rev_method.compare("REV_Medium") == 0) {
    reverberationMedium(s, paramsElement, rev_apply, numPartials);
  }

  else if (rev_method.compare("REV_Advanced") == 0) {
    reverberationAdvanced(s, paramsElement, rev_apply, numPartials);
  }

	else {
    cerr << "WARNING: Invalid method/syntax in reverb!" << endl;
    cerr << "   Method = " << rev_method << endl;
  }

}

//----------------------------------------------------------------------------//

void Bottom::reverberationSimple(Sound *s,
                                 pugi::xml_node paramsElement,
                                 string applyHow,
                                 int numPartials) {

    //     <Sizes>
    //       <Size><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></Size>
    //       <Size>0.2</Size>
    //       <Size>0.3</Size>
    //     </Sizes>

    pugi::xml_node sizeElement = GFEC(paramsElement); // First <Size>
    if (applyHow == "SOUND") {
        Reverb* reverbObj = computeReverberationSimple(sizeElement, -1);
        s->use_reverb(reverbObj);
    } else if (applyHow == "PARTIAL") {
    	// Now apply the reverb to each partial
    	for (int i = 0; i < numPartials; i++) {
        Reverb* reverbObj = computeReverberationSimple(sizeElement, i);
        // Add the reverb obj to the partial. It appears that this is already implemented in LASS/src/Partial.cpp.
        s->get(i).use_reverb(reverbObj);
        sizeElement = GNES(sizeElement);
        if (!sizeElement) {
          cerr << "WARNING: reverberationSimple parameters undefined since partial "
            << i + 1 << "; ignoring" << endl;
          break;
        }
	    }
      if (sizeElement) {
        cerr << "WARNING: reverberationSimple parameters defined beyond partial "
          << numPartials - 1 << "; ignoring" << endl;
      }
    } else {
    	cout << "WARNING: No <Apply> specifier for reverb, cannot apply." << endl;
    }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
Reverb* Bottom::computeReverberationSimple(pugi::xml_node sizeElement, int iPartial) {

  float roomSize;

  string envstr = XMLTC(sizeElement);
  if (envstr == "") {
    cerr << "WARNING: Fewer partials set in reverb string than configured in spectrum. Defaulting ";
    if (iPartial == -1) cerr << "sound to room size 0" << endl;
    else cerr << "partial " << iPartial << " to room size 0" << endl;
    roomSize = 0.0;
  } else {
    roomSize = utilities->evaluate(envstr, (void*)this);
  }

  Reverb* reverbObj = new Reverb(roomSize, SAMPLING_RATE);
  return reverbObj;

}

//----------------------------------------------------------------------------//

void Bottom::reverberationMedium(Sound *s,
                                 pugi::xml_node paramsElement,
                                 string applyHow,
                                 int numPartials) {

//    <Percents>
//      <Percent>
//        <Fun><Name>EnvLib</Name><Env>1</Env><Scale>1.0</Scale></Fun>
//      </Percent>
//    </Percents>
//    <Spreads><Spread>  0.5</Spread></Spreads>
//    <AllPasses><AllPass>0.5</AllPass></AllPasses>
//    <Delays><Delay>0.5</Delay></Delays>

  pugi::xml_node percentElement = GFEC(paramsElement);
  pugi::xml_node spreadElement  = GFEC(GNES(paramsElement));
  pugi::xml_node allPassElement = GFEC(GNES(GNES(paramsElement)));
  pugi::xml_node delayElement   = GFEC(GNES(GNES(GNES(paramsElement))));

  if (applyHow == "SOUND") {

    Reverb* reverbObj = computeReverberationMedium(percentElement,
      spreadElement, allPassElement, delayElement, -1);
    s->use_reverb(reverbObj);

  } else if (applyHow == "PARTIAL") {
    for (int i = 0; i < numPartials; i++) {

      Reverb* reverbObj = computeReverberationMedium(percentElement,
        spreadElement, allPassElement, delayElement, i);
      s->get(i).use_reverb(reverbObj);

      percentElement = GNES(percentElement);
      spreadElement  = GNES(spreadElement);
      allPassElement = GNES(allPassElement);
      delayElement   = GNES(delayElement);

      if (!percentElement || !spreadElement || !allPassElement || !delayElement) {
        cerr << "WARNING: reverberationMedium parameters undefined since partial "
          << i + 1 << "; ignoring" << endl;
        break;
      }
    }

    if (percentElement || spreadElement || allPassElement || delayElement) {
      cerr << "WARNING: reverberationMedium parameters defined beyond partial "
        << numPartials - 1 << "; ignoring" << endl;
    }

  } else {
    cout << "WARNING: No <Apply> specifier for reverb, cannot apply." << endl;
  }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
Reverb* Bottom::computeReverberationMedium(pugi::xml_node percentElement,
  pugi::xml_node spreadElement, pugi::xml_node allPassElement,
  pugi::xml_node delayElement, int iPartial) {

    //second input is percent reverb envelope
    string envstr = XMLTC(percentElement);
    if (envstr == "") {
      cerr << "WARNING: reverberationMedium got empty envelope for ";
      if (iPartial == -1) cerr << "sound; ignoring" << endl;
      else cerr << "partial " << iPartial << "; ignoring" << endl;
      return NULL;
    }
    Envelope* percent_rev =
	 (Envelope*) utilities->evaluateObject(envstr, this, eventEnv);

    //3 floats:  hi/low spread, gain all pass, delay
    float hi_low_spread = utilities->evaluate(XMLTC(spreadElement),this);
    float gain_all_pass = utilities->evaluate(XMLTC(allPassElement),this);
    float delay = utilities->evaluate(XMLTC(delayElement),this);

    if (delay == 0) {
      cerr << "WARNING: reverberationMedium got 0 delay for ";
      if (iPartial == -1) cerr << "sound; ignoring" << endl;
      else cerr << "partial " << iPartial << "; ignoring" << endl;
      return NULL;
    }

    Reverb* reverbObj = new Reverb(percent_rev, hi_low_spread, gain_all_pass,
      delay, SAMPLING_RATE);
    delete percent_rev;
    return reverbObj;

}

//----------------------------------------------------------------------------//

void Bottom::reverberationAdvanced(Sound *s,
                                   pugi::xml_node paramsElement,
                                   string applyHow,
                                   int numPartials) {

//  <Percents>
//	  <Percent>
//	    <Fun><Name>EnvLib</Name><Env>1</Env><Scale>1.0</Scale></Fun>
//	  </Percent>
//  </Percents>
//	<CombGainLists><CombGainList>0.46, 0.48, 0.50, 0.52, 0.53, 0.55</CombGainList></CombGainLists>
//	<LPGainLists><LPGainList>0.05, 0.06, 0.07, 0.05, 0.04, 0.02</LPGainList></LPGainLists>
//	<AllPasses><AllPass></AllPass></AllPasses>
//	<Delays><Delay></Delay></Delays>

  pugi::xml_node percentElement = GFEC(paramsElement);
  pugi::xml_node combGainListElement = GFEC(GNES(paramsElement));
  pugi::xml_node lpGainListElement   = GFEC(GNES(GNES(paramsElement)));
  pugi::xml_node allPassElement = GFEC(GNES(GNES(GNES(paramsElement))));
  pugi::xml_node delayElement   = GFEC(GNES(GNES(GNES(GNES(paramsElement)))));

  if (applyHow == "SOUND") {

    Reverb* reverbObj = computeReverberationAdvanced(percentElement,
      combGainListElement, lpGainListElement, allPassElement, delayElement, -1);
    s->use_reverb(reverbObj);

  } else if (applyHow == "PARTIAL") {
    for (int i = 0; i < numPartials; i++) {

      Reverb* reverbObj = computeReverberationAdvanced(percentElement,
        combGainListElement, lpGainListElement, allPassElement, delayElement, i);
      s->get(i).use_reverb(reverbObj);

      percentElement = GNES(percentElement);
      combGainListElement = GNES(combGainListElement);
      lpGainListElement   = GNES(lpGainListElement);
      allPassElement = GNES(allPassElement);
      delayElement   = GNES(delayElement);

      if (!percentElement || !combGainListElement || !lpGainListElement ||
          !allPassElement || !delayElement) {
        cerr << "WARNING: reverberationAdvanced parameters undefined since partial "
          << i + 1 << "; ignoring" << endl;
        break;
      }
    }

    if (percentElement || combGainListElement || lpGainListElement ||
        allPassElement || delayElement) {
      cerr << "WARNING: reverberationAdvanced parameters defined beyond partial "
        << numPartials - 1 << "; ignoring" << endl;
    }

  } else {
    cout << "WARNING: No <Apply> specifier for reverb, cannot apply." << endl;
  }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
Reverb* Bottom::computeReverberationAdvanced(pugi::xml_node percentElement,
  pugi::xml_node combGainListElement, pugi::xml_node lpGainListElement,
  pugi::xml_node allPassElement, pugi::xml_node delayElement, int iPartial) {

    //second input is percent reverb envelope
    string envstr = XMLTC(percentElement);
    if (envstr == "") {
      cerr << "WARNING: reverberationAdvanced got empty envelope for ";
      if (iPartial == -1) cerr << "sound; ignoring" << endl;
      else cerr << "partial " << iPartial << "; ignoring" << endl;
      return NULL;
    }
    Envelope* percent_rev =
         (Envelope*) utilities->evaluateObject(envstr, this, eventEnv);

//    //list of EXACTLY 6 comb gain filters
    vector<std::string> stringListC =
			utilities->listElementToStringVector(combGainListElement);
    if (stringListC.size() != 6) {
      cerr << "WARNING: reverb comb gain list for ";
      if (iPartial == -1) cerr << "sound must contain 6 items!" << endl;
      else cerr << "partial " << iPartial << " must contain 6 items!" << endl;
      return NULL;
    }
    vector<float> comb_gain_list;

    for (unsigned i = 0; i < stringListC.size(); i ++){
      float num = (float) utilities->evaluate(stringListC[i], this);
      comb_gain_list.push_back(num);
    }

    //list of EXACTLY 6 lp gain filters
    vector<std::string> stringListG =
              		utilities->listElementToStringVector(lpGainListElement);
    if (stringListG.size() != 6) {
      cerr << "WARNING: reverb lp gain list for ";
      if (iPartial == -1) cerr << "sound must contain 6 items!" << endl;
      else cerr << "partial " << iPartial << " must contain 6 items!" << endl;
      return NULL;
    }
    vector<float> lp_gain_list;

    for (unsigned i = 0; i < stringListG.size(); i ++){
      float num = (float) utilities ->evaluate(stringListG[i], this);
      lp_gain_list.push_back(num);
    }

    //2 floats:  gain all pass, delay
    float gain_all_pass = utilities->evaluate(XMLTC(allPassElement),this);
    float delay = utilities->evaluate(XMLTC(delayElement),this);

    if (delay == 0) {
      cerr << "WARNING: reverberationAdvanced got 0 delay for ";
      if (iPartial == -1) cerr << "sound; ignoring" << endl;
      else cerr << "partial " << iPartial << "; ignoring" << endl;
      return NULL;
    }

    Reverb* reverbObj = new Reverb(percent_rev,
			           &comb_gain_list[0],
				   &lp_gain_list[0],
				   gain_all_pass, delay, SAMPLING_RATE);

    delete percent_rev;
    return reverbObj;

}

//-----------------------------------------------------------------------------/

void Bottom::applyModifiers(Sound *s, int numPartials) {
  map<string, vector<Modifier> > modGroups; // ZIYUAN CHEN, July 2023 - map group names to the mods


  pugi::xml_document mergedModifiersDoc;
  pugi::xml_node modifiersIncludingAncestorsElement =
      mergedModifiersDoc.append_copy(modifiersElement);
  if (ancestorModifiersElement) {
    for (auto a = GFEC(ancestorModifiersElement); a; a = GNES(a)) {
      modifiersIncludingAncestorsElement.append_copy(a);
    }
  }

  //cout<<"Bottom-"<<name<<": Modifiers after merge:"<<XMLTC(modifiersIncludingAncestorsElement)<<endl<<endl<<"============="<<endl;



  pugi::xml_node modifierElement = GFEC(modifiersIncludingAncestorsElement);
  while (modifierElement!=NULL) {

//    <Modifier>
//      <Type>7</Type>
//      <ApplyHow>0</ApplyHow>
//      <Probability><Fun><Name>EnvLib</Name><Env>1</Env><Scale>1.0</Scale></Fun></Probability>
//      <Amplitude><Fun><Name>EnvLib</Name><Env>2</Env><Scale>1.0</Scale></Fun></Amplitude>
//      <Rate></Rate>
//      <Width></Width>
//	<DetuneSpread></DetuneSpread>
//	<DetuneDirection></DetuneDirection>
//	<DetuneVelocity></DetuneVelocity>
//      <GroupName></GroupName>
//      <PartialResultString></PartialResultString>
//    </Modifier>

    pugi::xml_node arg = GFEC(modifierElement);
    string modType;
    switch ((int)utilities->evaluate(XMLTC(arg), this)){
      case 0: modType = "TREMOLO"; break;
      case 1: modType = "VIBRATO"; break;
      case 2: modType = "GLISSANDO"; break;
      // case 3: modType = "BEND"; break;
      case 3: modType = "DETUNE"; break;
      case 4: modType = "AMPTRANS"; break;
      case 5: modType = "FREQTRANS"; break;
      case 6: modType = "WAVE_TYPE"; break;
    }
    //cout<<"Mod Type: "<<modType<<endl;
    arg = GNES(arg);
    string applyHow = ((int)utilities->evaluate(XMLTC(arg), this)==0)?"SOUND":"PARTIAL";

    arg = GNES(arg);

    Envelope* probEnv = NULL;
    pugi::xml_node ampElement, spreadElement, directionElement, velocityElement, rateElement, widthElement,
                   partialResultStringElement;
    string ampStr, spreadStr, directionStr, velocityStr, rateStr, widthStr, probStr, partialResultStr;

    // Only evaluate the envelope if we apply by SOUND. Otherwise, may segfault on empty probability envelopes.
    if (applyHow == "SOUND") {
      probEnv = (Envelope*)utilities->evaluateObject(XMLTC(arg), this, eventEnv);
      probStr = XMLTC(arg);
    }

    ampElement = GNES(arg);
    rateElement = GNES(ampElement);
    widthElement = GNES(rateElement);
    spreadElement = GNES(GNES(GNES(ampElement)));
    directionElement = GNES(spreadElement);
    velocityElement = GNES(directionElement);
    partialResultStringElement = GNES(GNES(velocityElement));	
    
    ampStr = XMLTC(ampElement);
    spreadStr = XMLTC(spreadElement);
    directionStr = XMLTC(directionElement);
    velocityStr = XMLTC(velocityElement);
    rateStr = XMLTC(rateElement);
    widthStr = XMLTC(widthElement);
    partialResultStr = XMLTC(partialResultStringElement);

    // ADDED BY TEJUS
    // skip group name
    // pugi::xml_node partialResultStringElement = GNES(GNES(widthElement));

    // TEJUS 2/8
    // and apply modifiers one by one via their partial number.
    // TODO: Validate the partial num to make sure that it does not go out of range.

    if (applyHow == "SOUND") {
      Modifier newMod(modType, probEnv, applyHow);

      // TEST
      if (ampStr!=""){
        Envelope* env =  (Envelope*)utilities->evaluateObject(ampStr, this, eventEnv );
        newMod.addValueEnv(env);
        delete env;
      }
      if (spreadStr != ""){
        newMod.addSpread(atof(spreadStr.c_str()));
      }
      if (directionStr != ""){
        newMod.addDirection(atof(directionStr.c_str()));
      }
      if (velocityStr != ""){
        newMod.addVelocity(atof(velocityStr.c_str()));
float vel;
      }
      if (rateStr!=""){
        Envelope* env =  (Envelope*)utilities->evaluateObject(rateStr, this, eventEnv );
        newMod.addValueEnv(env);
        delete env;
      }
      if (widthStr!=""){
        Envelope* env =  (Envelope*)utilities->evaluateObject(widthStr, this, eventEnv );
        newMod.addValueEnv(env);
        delete env;
      }

      /* ZIYUAN CHEN, July 2023 - Categorizing a modifier into groups */
      arg = GNES(velocityElement);//group name
      std::stringstream ss(XMLTC(arg));
      std::string groupName;
      while (std::getline(ss, groupName, ',')) {
        /* strip leading and trailing whitespaces to be compatible with
           <List>Name1, Name2, Name3</List> in Select - RandomInt function */
        groupName.erase(0, groupName.find_first_not_of(' '));
        groupName.erase(groupName.find_last_not_of(' ') + 1);
        modGroups[groupName].push_back(newMod);
      }
      delete probEnv;
    }
    else if (applyHow == "PARTIAL") {
      pugi::xml_document partialResultDoc;
      partialResultDoc.load_string(partialResultStr.c_str());
      pugi::xml_node root = partialResultDoc.document_element();
      pugi::xml_node thisElement = GFEC(root);    //start of envelopes
      thisElement = GNES(thisElement);    //envelopes

      pugi::xml_node envelopeElement = GFEC(thisElement);//first envelope
      for (int i = 0; i <numPartials; i ++){
        // make envelopes for all the partials
        if (envelopeElement == NULL){
          cout << "WARNING: Fewer partials set in partial result string string than configured in spectrum."
          << " Correct partial number is   " << numPartials << endl;
          return;
        }
        Envelope* probEnv = (Envelope*)utilities->evaluateObject(XMLTC(envelopeElement), this, eventEnv);
        
        probStr = XMLTC(envelopeElement);

       
   
      	// Make a new modifier 
      	Modifier newPartialMod(modType, probEnv, applyHow, i);
        envelopeElement = GNES(envelopeElement);
        ampStr = XMLTC(envelopeElement);
       // return;
       envelopeElement = GNES(envelopeElement);
        widthStr = XMLTC(envelopeElement);
        envelopeElement = GNES(envelopeElement);
        rateStr = XMLTC(envelopeElement);
        if (ampStr!="" && ampStr!="N/A"){
          Envelope* env =  (Envelope*)utilities->evaluateObject(ampStr, this, eventEnv );
          newPartialMod.addValueEnv(env);
          delete env;
        }
        if (widthStr!="" && widthStr!="N/A"){
           Envelope* env =  (Envelope*)utilities->evaluateObject(widthStr, this, eventEnv );
           newPartialMod.addValueEnv(env);
           delete env;
         }
          if (rateStr!="" && rateStr!="N/A"){
           Envelope* env =  (Envelope*)utilities->evaluateObject(rateStr, this, eventEnv );
           newPartialMod.addValueEnv(env);
           delete env;
         }
        //return;
        

	// delete probEnv;
        arg = GNES(velocityElement);//group name
        std::stringstream ss(XMLTC(arg));
        std::string groupName;
        while (std::getline(ss, groupName, ',')) {
          groupName.erase(0, groupName.find_first_not_of(' '));
          groupName.erase(groupName.find_last_not_of(' ') + 1);
          modGroups[groupName].push_back(newPartialMod);
        }
        envelopeElement = GNES(envelopeElement);
      }
    }

    modifierElement = GNES(modifierElement); // go to the next MOD in the list
  } // end of the main while loop

  /* ZIYUAN CHEN, July 2023 -
    Here, we evaluate the target "Modifier Group" name to be applied.
    The user may put a string (e.g., "Apple") or a function in this field.
    The function is always a random selection between strings with the following syntax:
      <Fun>
        <Name>Select</Name>
        <List>Apple,Boy,Cat</List>
        <Index>
          <Fun>
            <Name>RandomInt</Name>
            <LowBound>0</LowBound>
            <HighBound>2</HighBound>
          </Fun>
        </Index>
      </Fun>
    Since utilities->evaluate() returns a double everywhere else (and correspondingly,
      <List> holds numbers instead of strings), a special mechanism is implemented here
      to (1) evaluate <Index> as a "RandomInt" function and (2) manually extract the
      desired <List> element, instead of rewriting utilities->evaluate().
  */

  string targetModGroupName = XMLTC(modifierGroupElement);

  if (targetModGroupName.find("<Fun>") != string::npos) { // evaluate if it's function string

    pugi::xml_node modifierGroupListElement = GNES(GFEC(GFEC(modifierGroupElement))); // <List>
    pugi::xml_node modifierGroupIndexFunElement = GNES(modifierGroupListElement); // <Index>
    int targetModGroupIndex = (int)utilities->evaluate(XMLTC(modifierGroupIndexFunElement), this);

    std::stringstream ss(XMLTC(modifierGroupListElement));
    while (std::getline(ss, targetModGroupName, ',') && targetModGroupIndex > 0) {
      targetModGroupName.erase(0, targetModGroupName.find_first_not_of(' '));
      targetModGroupName.erase(targetModGroupName.find_last_not_of(' ') + 1);
      targetModGroupIndex--;
    }

  }

  // ZIYUAN CHEN, July 2023 - apply the specified one (1) group of modifiers
  if (modGroups.find(targetModGroupName) != modGroups.end()) {
    vector<Modifier> modGroup = modGroups[targetModGroupName];
    for (unsigned i = 0; i < modGroup.size(); i++) {
      modGroup[i].applyModifier(s);
    }
  } else {
    cerr << "WARNING: Specified modifier group " << targetModGroupName << " not found!" << endl;
  }

  //delete modifiersIncludingAncestorsElement;
}

//----------------------------------------------------------------------------//

vector<string> Bottom::applyNoteModifiersOld() {
  vector<string> modNames;

  vector<Modifier> modNoDep;  //mods without dependencies
  map<string, vector<Modifier> > modMutEx; // map mutex group names to the mods
  cout << "Bottom::applyNoteModifiers begin" << endl;


  pugi::xml_document mergedModifiersDoc;
  pugi::xml_node modifiersIncludingAncestorsElement =
      mergedModifiersDoc.append_copy(modifiersElement);
  if (ancestorModifiersElement) {
    for (auto a = GFEC(ancestorModifiersElement); a; a = GNES(a)) {
      modifiersIncludingAncestorsElement.append_copy(a);
    }
  }

  cout << "Bottom-"<<name<<": Modifiers after merge:"
       << XMLTC(modifiersIncludingAncestorsElement) << endl << endl
       << "=============" << endl;


/* needs to be rewrite to remove filevalue
  list<FileValue>* modList = modifiersFV->getListPtr(this);
  list<FileValue>::iterator modIter = modList->begin();
*/

  pugi::xml_node modifierElement = GFEC(modifiersIncludingAncestorsElement);
/*
  pugi::xml_node modifierElement = GFEC(modifiersElement);

    cout<<"modifierElement: "<<XMLTC(modifierElement)<<endl;
   int sever;  cin >> sever;
*/

  while (modifierElement != NULL) {

    pugi::xml_node arg = GFEC(modifierElement);
    string modType;
    switch ((int)utilities->evaluate(XMLTC(arg), this)){
      case 0: modType = "TREMOLO"; break;
      case 1: modType = "VIBRATO"; break;
      case 2: modType = "GLISSANDO"; break;
      // case 3: modType = "BEND"; break;
      case 3: modType = "DETUNE"; break;
    }
    //cout<<"Mod Type: "<<modType<<endl;

    arg = GNES(arg);
    string applyHow =
           ((int)utilities->evaluate(XMLTC(arg), this) == 0)? "SOUND":"PARTIAL";

    arg = GNES(arg);
    Envelope* probEnv =
	   (Envelope*)utilities->evaluateObject(XMLTC(arg), this, eventEnv);

    pugi::xml_node ampElement = GNES(arg);
    pugi::xml_node rateElement = GNES(ampElement);

    string ampStr = XMLTC(ampElement);
    string rateStr = XMLTC(rateElement);
    cout << "Bottom::applyNoteModifiers - rateStr: " << rateStr << endl;

    Modifier newMod(modType, probEnv, applyHow);

/* needs to be rewrite to remove filevalue
  while (modIter != modList->end()) {
    // create the modifier and add it to the proper group
    list<FileValue>::iterator currMod = modIter->getListPtr(this)->begin();
    // 1st arg is string, 2nd is env
    string modType = currMod->getString(this);
    currMod++;
    Envelope* probEnv = currMod->getEnvelope(this);
    currMod++;

    Modifier newMod(modType, probEnv, "SOUND");
*/

/* needs to be rewrite to remove filevalue
    string mutExGroup = "";

    while (mutExGroup == "" && currMod != modIter->getListPtr(this)->end()) {
      FileValue mutExFV = currMod->getListPtr(this)->back();
      if (mutExFV.getReturnType() == FVAL_STRING) {
        mutExGroup = mutExFV.getString(this);
      } else {
        cerr << "Bottom::applyModifiers error: invalid syntax for MUT_EX group!" << endl;
        exit(1);
      }
*/

    arg = GNES(rateElement);//group name (MUT_EX)
    string mutExGroup = XMLTC(arg);

    if (applyHow == "SOUND") {

      if (ampStr!=""){
        Envelope* env =
		 (Envelope*)utilities->evaluateObject(ampStr, this, eventEnv );
        newMod.addValueEnv(env);
        delete env;
      }

      if (rateStr!=""){
        Envelope* env =
		 (Envelope*)utilities->evaluateObject(rateStr, this, eventEnv );
        newMod.addValueEnv(env);
        delete env;
      }
    }
    else if (applyHow == "PARTIAL") {
      cout << "ERROR: Note does not use PARTIAL" << endl;
    }

    if (mutExGroup == "") {
      // not MUT_EX
      modNoDep.push_back(newMod);
    } else {
      // mutually exclusive
      modMutEx[mutExGroup].push_back(newMod);
    }
/*    arg = GNES(widthElement);//group name (MUT_EX)
    string mutExGroup = XMLTC(arg);

    modIter++; // go to the next MOD in the list
  }
*/
    delete probEnv;
    modifierElement = GNES(modifierElement); // go to the next MOD in the list
  } // end of the main while loop


  // go through the non-exclusive mods
  for (unsigned i = 0; i < modNoDep.size(); i++) {
    if (modNoDep[i].willOccur(checkPoint)) {
      modNames.push_back( modNoDep[i].getModName() );
    }
  }

  // go through the exclusive mods
  map<string, vector<Modifier> >::iterator iter = modMutEx.begin();
  if (iter != modMutEx.end()) {
    vector<Modifier> modGroup = (*iter).second;

    //go through the group, and apply 1 at most
    bool appliedMod = false;
    for (unsigned i = 0; i < modGroup.size() && !appliedMod; i++) {
      if (modGroup[i].willOccur(checkPoint)) {
        modNames.push_back( modGroup[i].getModName() );
        appliedMod = true;
      }
    }
    iter++;
  }

  //delete modifiersIncludingAncestorsElement;

  return modNames;
}

// int applyNoteStaffs(pugi::xml_node _playingMethods){
//   int noteStaff;

//   pugi::xml_node noteInfo = GNES(GNES(GFEC(_playingMethods)));
//   pugi::xml_node techniqueElement = GFEC(noteInfo);

//   noteStaff = utilities->evaluate(XMLTC(techniqueElement),(void*)this);

//   return noteStaff;
// }

//----------------------------------------------------------------------------//

vector<string> Bottom::applyNoteModifiers( pugi::xml_node _playingMethods) {

  vector<string> modNames;

  // pugi::xml_node noteInfo = GNES(GNES(GFEC(_playingMethods)));
  // pugi::xml_node techniqueElement = GNES(GFEC(noteInfo));

//  string name0 = XMLTC(techniqueElement);
//  cout << "name0: " << name0 << endl;

  // pugi::xml_node currentTechnique = GFEC(techniqueElement);
  pugi::xml_node currentTechnique = GFEC(_playingMethods);

  // do {
  //   string name = XMLTC(currentTechnique);
  //   modNames.push_back(name);
  //   currentTechnique = GNES(currentTechnique);
  // } while ( currentTechnique != NULL);

  while (currentTechnique != NULL) {
    string name = XMLTC(currentTechnique);
    modNames.push_back(name);
    currentTechnique = GNES(currentTechnique);
  }

  return modNames;
}

//----------------------------------------------------------------------------//

void Bottom::generatePartials(Sound* newsound, float frequency, float loudness, float distance, Envelope* waveShape){
  float strength = loudness*distance/256*2;   //strength is normalized between 0 and 2

  if ((frequency < 233) || frequency > 932){
    cout << "Error in genratePartials: frequency out of range" << endl;
  }
  //calculate scale for Partials
  double scaleTable[3][3][20] = {
    { //Bb3,p,m,f     233 hz
      {0.886, 0.014, 1, 0.072, 0.367, 0.193, 0.183, 0.036, 0.058, 0.010, 0.028,
        0.021, 0.011, 0.016, 0.003, 0.003, 0.002, 0.002, 0.002, 0.002},
      {0.756, 0.014, 1, 0.037, 0.521, 0.217, 0.234, 0.123, 0.306, 0.066, 0.142,
      0.040, 0.053, 0.044, 0.053, 0.039, 0.033, 0.043, 0.004, 0.012},
      {0.832, 0.019, 1.000, 0.043, 0.481, 0.226, 0.145, 0.119, 0.384, 0.091,
        0.182, 0.048, 0.080, 0.094, 0.077, 0.053, 0.044, 0.057, 0.009, 0.021}
      },
    {//Bb4,p,m,f     466 hz
      {1.000, 0.115, 0.834, 0.079, 0.233, 0.031, 0.027, 0.015, 0.005, 0.004,
      0.008, 0.002, 0.001, 0.0017, 0.001, 0.001, 0.001, 0.001, 0.0006, 0.001},
      {0.938, 0.072, 1.000, 0.318, 0.096, 0.096, 0.036, 0.109, 0.023, 0.010,
      0.007, 0.007, 0.002, 0.010, 0.002, 0.0015, 0.002, 0.002, 0.002, 0.0015},
      {0.939, 0.036, 1.000, 0.252, 0.220, 0.078, 0.057, 0.045, 0.027, 0.016,
      0.013, 0.007, 0.003, 0.008, 0.004, 0.001, 0.002, 0.001, 0.0022, 0.0018}
    },
    {//Bb5,p,m,f     932 hz
      {1.000, 0.269, 0.227, 0.040, 0.004, 0.006, 0.003, 0.0007, 0.002, 0.0007,
      0.0007, 0.003, 0.0007, 0.001, 0.0014, 0.0005, 0.0007, 0.0005, 0.0007, 0.0005},
      {1.00, 0.507, 0.132, 0.160, 0.030, 0.002, 0.001, 0.0008, 0.0016, 0.003, 0.002,
      0.001, 0.001, 0.001, 0.0005, 0.0005, 0.001, 0.0005, 0.001, 0.00064},
      {1.00, 0.354, 0.198, 0.102, 0.054, 0.012, 0.005, 0.002, 0.005, 0.003, 0.003,
      0.002, 0.001, 0.002, 0.004, 0.0015, 0.002, 0.002, 0.003, 0.0013}
    }
  };
  //Set the number of partials.
  int numPartials = 20;
  //For each partial, create and add to sound.
  for (int i = 0; i < numPartials; i++) {

    //Create the next partial object.
    Partial partial;

    //Set the partial number of the partial based on the current index.
    partial.setParam(PARTIAL_NUM, i);

    //Set the frequencies for each partial.
    float actualFrequency = setPartialFreq(
      partial, 0, frequency, i);

    //Report the actual frequency.
    stringstream ss; if(i != 0) ss << "Partial " << i; else ss << "Fundamental";
    if (utilities->getOutputParticel())
  Output::addProperty(ss.str(), actualFrequency, "Hz");
    //Set the spectrum for this partial.
    //interpolate the scale from scale table
    // double scale;
    // if (frequency <= 466){
    //   if (strength <= 1){
    //     double scale_strength1 = calculateFreqPartial(233,scaleTable[0][0][i],466,scaleTable[1][0][i],frequency);
    //     double scale_strength2 = calculateFreqPartial(233,scaleTable[0][1][i],466,scaleTable[1][1][i],frequency);
    //     scale = strength * scale_strength1 + (1 - strength) * scale_strength2;
    //   }
    //   else{
    //     double scale_strength1 = calculateFreqPartial(233,scaleTable[0][1][i],466,scaleTable[1][1][i],frequency);
    //     double scale_strength2 = calculateFreqPartial(233,scaleTable[0][2][i],466,scaleTable[1][2][i],frequency);
    //     scale = (2 - strength) * scale_strength1 + (strength - 1) * scale_strength2;
    //   }
    // }
    // else{
    //   if (strength <= 1){
    //     double scale_strength1 = calculateFreqPartial(466,scaleTable[1][0][i],932,scaleTable[2][0][i],frequency);
    //     double scale_strength2 = calculateFreqPartial(466,scaleTable[1][1][i],932,scaleTable[2][1][i],frequency);
    //     scale = strength * scale_strength1 + (1 - strength) * scale_strength2;
    //   }
    //   else{
    //     double scale_strength1 = calculateFreqPartial(466,scaleTable[1][1][i],932,scaleTable[2][1][i],frequency);
    //     double scale_strength2 = calculateFreqPartial(466,scaleTable[1][2][i],932,scaleTable[2][2][i],frequency);
    //     scale = (2 - strength) * scale_strength1 + (strength - 1) * scale_strength2;
    //   }
    // }

    partial.setParam(WAVE_SHAPE, *waveShape);
    //Add the partial to the sound.
    newsound->add(partial);
  }
}

//----------------------------------------------------------------------------//

double Bottom::calculateFreqPartial(double x1, double y1, double x2, double y2, double x){
  //return the amplitude (ratio) of the generated partial
  //the curve between two points (x1,y1) and (x2,y2) is modeled by y=a*2^(b*(x-x1))
  //assuming y2>y1
  if ((x > x2) || (x < x1)){
    cout << "Bottom::error in calculateFreqPartials" << endl;
    return 0;
  }
  double a, b, y;
  if (y2 >= y1){
    a = y1;
    b = (log2(y2 / y1)) / (x2 - x1);
    y = a * pow(2, b * (x-x1) );
  }
  else{
    //the downward curve is supposed to be symmetric (with respect to the
    //horizontal line y1) to the above case. Model the curve as if y2>y1 and
    //take the symmetric result
    double y2_temp = y1 + (y1 - y2);
    a = y1;
    b = (log2(y2_temp / y1)) / (x2 - x1);
    double y_temp = a * pow(2, b * (x-x1) );
    y = y1 - (y_temp - y1);
  }
  return y;
}
