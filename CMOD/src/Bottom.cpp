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
#include "CmodError.h"
#include "ModifierUsage.hpp"
#include "Random.h"
#include "Output.h"
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

static int test=0;

struct Bottom::ModifierUsageRuntime {
  std::optional<dissco::modifier_usage::Program> program;
  std::vector<dissco::modifier_usage::ModifierId> modifierIds;
};

namespace {

void warnBottom(const string& name, const string& field,
                const string& message, const string& suggestion) {
  cerr << "CMOD warning: " << message << '\n'
       << "Context: Bottom '" << name << "' / " << field << '\n'
       << "Suggestion: " << suggestion << endl;
}

bool parseStrictDouble(const char* text, double& value) {
  if (text == NULL || *text == '\0') {
    return false;
  }

  errno = 0;
  char* end = NULL;
  value = std::strtod(text, &end);
  if (end == text || errno == ERANGE) {
    return false;
  }
  while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  return *end == '\0';
}

bool isUnavailable(const string& value) {
  std::size_t first = 0;
  while (first < value.size()
         && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }

  std::size_t last = value.size();
  while (last > first
         && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }

  if (first == last) {
    return true;
  }
  if (last - first != 3) {
    return false;
  }
  return std::tolower(static_cast<unsigned char>(value[first])) == 'n'
      && value[first + 1] == '/'
      && std::tolower(static_cast<unsigned char>(value[first + 2])) == 'a';
}

} // namespace


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
    <Phase>0</Phase>
    <Spatialization>5</Spatialization>
    <Reverb>6</Reverb>
    <Filter>f</Filter>
    <ModifierUsage version="1" samplingScope="per-sound"/>
    <Modifiers>
    </Modifiers>
  </ExtraInfo>
  */

  // ExtraInfo used to be parsed by sibling position.  Phase is optional for
  // backward compatibility, so resolve every field by name instead: otherwise
  // inserting <Phase> would shift Spatialization/Reverb/Filter/Modifiers.
  frequencyElement = extraInfo.child("FrequencyInfo");
  loudnessElement = extraInfo.child("Loudness");
  phaseElement = extraInfo.child("Phase");
  if (_ancestorSpa != NULL){
    spatializationElement = _ancestorSpa;
  }
  else {
    spatializationElement = extraInfo.child("Spatialization");
  }

  if (_ancestorRev != NULL){
    reverberationElement = _ancestorRev;
  }
  else {
    reverberationElement = extraInfo.child("Reverb");
  }

  if (_ancestorFil != NULL){
    filterElement = _ancestorFil;
  }
  else {
    filterElement = extraInfo.child("Filter");
  }

  modifiersElement = extraInfo.child("Modifiers");

  // Modifier Usage is now the only runtime path. Files without the marker are
  // adapted below with explicit, deterministic best-effort defaults.
  initializeModifierUsage(extraInfo.child("ModifierUsage"));

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

  // A Bottom placed in Seconds or as a Fraction (an inexact start) begins a
  // new tempo section at its own start, whatever unit its children use.
  // Event::checkEvent does this for exact children; children in Seconds or
  // Fraction need it too, or their notes are measured from the inherited
  // section start. Only the score reads the tempo start, not the sound.
  if (!ts.startEDU.isDeterminate()) {
    tempo.setStartTime(ts.start);
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
      throw CmodError(CmodError::Kind::Project,
                      "Unknown child build method '" + method + "'.",
                      "Bottom '" + name + "' / Child Event Definition / DefinitionFlag",
                      "Choose Continuum (0), Sweep (1), or Discrete (2) as the child build method.");
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

void Bottom::constructChild(SoundAndNoteWrapper* _soundNoteWrapper) try {
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
  throw CmodError(CmodError::Kind::Project,
                  "Bottom name '" + name + "' does not identify a sound or note event.",
                  "Bottom Name",
                  "Start sound Bottom names with 's' and note Bottom names with 'n', then update references to the renamed event.");
  // else if (name.substr(0,2) == "ns" || name.substr(0,2) == "sn"){
  //   buildSound(_soundNoteWrapper);
  //   buildNote(_soundNoteWrapper);
  //   return;
  // }
} catch (CmodError& error) {
  error.addContext("Bottom '" + name + "' / child #"
      + std::to_string(currChildNum + 1) + " ('" + _soundNoteWrapper->name + "')");
  throw;
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

  // Fixed initial phase offset.  LASS represents oscillator phase in cycles,
  // so no degree/radian conversion is needed here.
  float carrierPhase = computeCarrierPhase();
  if (utilities->getOutputParticel())
    Output::addProperty("Carrier Phase", carrierPhase, "cycle");

  int numPartials = computeNumPartials( baseFrequency ,_soundNoteWrapper->element );

 //Element for GenSpectrum
  pugi::xml_node specElement = GNES(GNES(GNES(GNES(GFEC(_soundNoteWrapper->element)))));

  //if there is no spec element,go to partial routine;else, go to gen_spectrum
  if (GFEC(specElement) != NULL){
    pugi::xml_node partialEnvElement = utilities->evaluateSpectrumElement(XMLTC(specElement), (void*)this);
    pugi::xml_node distanceElement = GNES(partialEnvElement);

    Envelope* waveShape = (Envelope*) utilities->evaluateObject(XMLTC(partialEnvElement),(void*)this, eventEnv );
    float distance = static_cast<float>(utilities->evaluate(XMLTC(distanceElement),(void*)this));

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
        partial.setParam(PARTIAL_NUM, static_cast<m_value_type>(i));

        //Compute the deviation for partials above the fundamental.
        double deviation = 0;

        if(i != 0)
          deviation = computeDeviation(_soundNoteWrapper->element );

        //Set the frequencies for each partial.
        float actualFrequency = setPartialFreq(
          partial, static_cast<float>(deviation), baseFrequency, i);

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

  // Apply after either partial-creation path so generated-spectrum partials
  // receive the same carrier offset as explicitly configured partials.
  newSound->setPartialParam(CARRIER_PHASE, carrierPhase);

  // Generated spectra currently create a fixed number of partials.  Use the
  // actual Sound size for all subsequent per-partial operations.
  numPartials = newSound->size();

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
  int noteStaff = static_cast<int>(utilities->evaluate(XMLTC(staffsInfo),(void*)this));
  // int noteStaff = applyNoteStaffs(_soundNoteWrapper->element);
  newNote->setStaffNum(noteStaff);

  //Set the pitch.
  float baseFrequency = computeBaseFreq();

  int notePitch;

  if(wellTempPitch <= 0) { 		//if frequency is in Hertz
    notePitch = newNote->HertzToPitch(baseFrequency);
  } else {
    notePitch = wellTempPitch;
  }
  newNote->setPitchWellTempered(notePitch);

  // Set notation start and end times in edus from the start of the tempo
  // section. A start or duration given in Seconds or as a Fraction has no
  // exact edu value (Ratio(0,0)), so it is placed by its global time in
  // seconds, converted with the tempo and rounded to the nearest edu.
  TimeSpan& noteTs = _soundNoteWrapper->ts;
  int noteStartEDU;
  if (noteTs.startEDU.isDeterminate()) {
    noteStartEDU = noteTs.startEDU.To<int>();
  } else {
    noteStartEDU = tempo.convertSecondsToEDUs(noteTs.start - tempo.getStartTime());
  }
  int noteEndEDU;
  if (noteTs.durationEDU.isDeterminate()) {
    noteEndEDU = noteStartEDU + noteTs.durationEDU.To<int>();
  } else {
    noteEndEDU = tempo.convertSecondsToEDUs(
      noteTs.start + noteTs.duration - tempo.getStartTime());
  }
  newNote->setStartTime(noteStartEDU);
  newNote->setEndTime(noteEndEDU);
  // Initialize the parameter split before the arrangement
  newNote->initSplit();
  
  // multistaffs
  // Only the score uses the notation score; without score output the
  // tempo (whose time signature may not be notatable) is not registered
  if (utilities->getScorePrinting()) {
    //Output::notation_score_.RegisterTempo(tempo);
    Output::notation_score_.RegisterTempo(tempo,newNote->getStaffNum());
    Output::notation_score_.InsertNote(newNote, tempo);
  }

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
  pugi::xml_node freqFlagElement = frequencyElement.child("FrequencyFlag");
  pugi::xml_node continuumFlagElement = frequencyElement.child("FrequencyContinuumFlag");
  pugi::xml_node valueElement = frequencyElement.child("FrequencyEntry1");
  pugi::xml_node valueElement2 = frequencyElement.child("FrequencyEntry2");
  const string context = "Bottom '" + name + "' / Frequency";
  const double frequencyMode = utilities->evaluate(XMLTC(freqFlagElement), this);
  if (frequencyMode != 0 && frequencyMode != 1 && frequencyMode != 2) {
    throw CmodError(CmodError::Kind::Project,
                    "Unknown FrequencyFlag value " + std::to_string(frequencyMode) + ".",
                    context,
                    "Choose Equal Temperament (0), Fundamental (1), or Continuum (2) in the Bottom event's Frequency settings.");
  }
  if (frequencyMode == 2) {//continuum
    /* 2nd arg is a string (HERTZ or POW2) */

    if (utilities->evaluate(XMLTC(continuumFlagElement), NULL)==0) { //Hertz
      baseFreqResult = static_cast<float>(utilities->evaluate(XMLTC(valueElement), (void*)this));
      /* 3rd arg is a float (baseFreq in Hz) */
    }
    else  {//power of 2
      /* 3rd arg is a float (power of 2) */
      float step = static_cast<float>(utilities->evaluate(XMLTC(valueElement), (void*)this));
      if(step <= log2(MINFREQ/C0) || step >= log2(CEILING/C0)) {
        cerr << "CMOD warning: " << context << " / FrequencyEntry1: power-of-two step "
             << step << " is outside the audible range ("
             << log2(MINFREQ/C0) << " to " << log2(CEILING/C0) << "). "
             << "Check the power-of-two expression; partial frequencies outside "
             << MINFREQ << " to " << CEILING << " Hz are clamped." << endl;
      }
      baseFreqResult = static_cast<float>(C0 * pow(2, step));
    }
  } else if (frequencyMode == 0) { //equal tempered
    /* 2nd arg is an int */
    const double pitch = utilities->evaluate(XMLTC(valueElement), this);
    if (pitch < std::numeric_limits<int>::min()
        || pitch > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "Equal-tempered pitch " + std::to_string(pitch) + " is outside the integer range.",
                      context + " / FrequencyEntry1",
                      "Check the pitch expression and choose a pitch that produces a finite, positive frequency.");
    }
    wellTempPitch = static_cast<int>(pitch);
    baseFreqResult = static_cast<float>(C0 * pow(WELL_TEMP_INCR, wellTempPitch));
  } else  {// fundamental
    /* 2nd arg is (float)fundamental_freq, 3rd arg is (int)overtone_num */
    float fund_freq = static_cast<float>(utilities->evaluate(XMLTC(valueElement), (void*)this));
    const double overtone = utilities->evaluate(XMLTC(valueElement2), this);
    if (overtone < 1 || overtone > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "Overtone number evaluated to " + std::to_string(overtone) + ".",
                      context + " / FrequencyEntry2",
                      "Set the overtone number to a positive count within the integer range.");
    }
    int overtone_step = static_cast<int>(overtone);
    baseFreqResult = fund_freq * overtone_step;
  }
  if (!std::isfinite(baseFreqResult) || baseFreqResult <= 0) {
    throw CmodError(CmodError::Kind::Project,
                    "Base frequency evaluated to " + std::to_string(baseFreqResult) + " Hz.",
                    context + " / FrequencyEntry1: " + XMLTC(valueElement),
                    "Choose a finite frequency greater than zero; check the pitch, power-of-two, or fundamental/overtone expression.");
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
  float loudval = static_cast<float>(utilities->evaluate(XMLTC(loudnessElement), (void*)this));
  // float diff = loudval - expVal;
  // loudval -= 0.4 * diff;
  // cout << "bottom loudness: " << loudval << endl;
  // cout << "bottom expval: " << expVal << endl;
  if (!std::isfinite(loudval) || loudval < 0) {
    throw CmodError(CmodError::Kind::Project,
                    "Loudness evaluated to " + std::to_string(loudval) + " sones.",
                    "Bottom '" + name + "' / Loudness: " + XMLTC(loudnessElement),
                    "Use a finite, non-negative loudness in sones and check the expression that computes it.");
  }
  return loudval;
}

//----------------------------------------------------------------------------//

float Bottom::computeCarrierPhase() {
  const string phaseExpression = XMLTC(phaseElement);
  if (phaseExpression.empty()) {
    return 0.0f;
  }

  float phase = static_cast<float>(utilities->evaluate(phaseExpression, (void*)this));
  if (!std::isfinite(phase)) {
    cerr << "WARNING: Carrier Phase for Bottom " << name
         << " is not finite; using 0 cycle." << endl;
    return 0.0f;
  }

  // One full cycle is equivalent to zero.  Wrapping also gives a defined,
  // periodic result for expressions that evaluate outside the UI's 0..1 range.
  if (phase < 0.0f || phase >= 1.0f) {
    const float originalPhase = phase;
    phase = std::fmod(phase, 1.0f);
    if (phase < 0.0f) phase += 1.0f;
    if (originalPhase != 1.0f) {
      cerr << "WARNING: Carrier Phase should be between 0 and 1 cycle; wrapping "
           << originalPhase << " to " << phase << " in Bottom " << name << "." << endl;
    }
  }

  return phase;
}

//----------------------------------------------------------------------------//

int Bottom::computeNumPartials(float baseFreq, pugi::xml_node _spectrum) {

  pugi::xml_node numPartialElement = _spectrum.child("NumberOfPartials");
  const double requested = utilities->evaluate(XMLTC(numPartialElement), this);
  const string context = "Bottom '" + name + "' / spectrum '"
      + XMLTC(_spectrum.child("Name")) + "' / Number of Partials";
  if (!std::isfinite(requested) || requested < 1
      || requested > std::numeric_limits<int>::max()) {
    throw CmodError(CmodError::Kind::Project,
                    "Number of Partials evaluated to " + std::to_string(requested) + ".",
                    context,
                    "Set Number of Partials to a positive count within the integer range, and check the expression that computes it.");
  }
  int numPartsResult = static_cast<int>(requested);

  // Decrease numPartials until p < CEILING
  // (CEILING is a global def from define.h)
  while(numPartsResult * baseFreq > CEILING) {
    numPartsResult--;
  }

  if(numPartsResult <= 0) {
    throw CmodError(CmodError::Kind::Project,
                    "No partials fit below CMOD's frequency ceiling of "
                        + std::to_string(static_cast<double>(CEILING))
                        + " Hz; the base frequency is " + std::to_string(baseFreq) + " Hz.",
                    context,
                    "Lower the Bottom event's Frequency so at least its fundamental partial fits below the frequency ceiling.");
  }

  return numPartsResult;
}

//----------------------------------------------------------------------------//

float Bottom::computeDeviation( pugi::xml_node _spectrum) {
  pugi::xml_node devElement = GNES(GNES(GNES(GFEC(_spectrum))));
  return static_cast<float>(utilities->evaluate(XMLTC(devElement), (void*)this));
}

//----------------------------------------------------------------------------//

float Bottom::setPartialFreq(Partial& part, float deviation, float baseFreq, int partNum) {

  // assign frequency to each partial
  float pDev = static_cast<float>(deviation * (Random::Rand() - 0.5) * 2);
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
    const string envelope = XMLTC(partialEnvElement);
    if (envelope.empty()) {
      throw CmodError(CmodError::Kind::Project,
                      "Partial #" + std::to_string(partNum + 1) + " has no wave-shape envelope.",
                      "Bottom '" + name + "' / spectrum '" + XMLTC(_element.child("Name")) + "' / Spectrum",
                      "Provide an envelope for each requested partial, or lower Number of Partials to match the configured envelopes.");
    }
    Envelope* waveShape = (Envelope*) utilities->evaluateObject(envelope, this, eventEnv);
    if (waveShape == NULL) {
      throw CmodError(CmodError::Kind::Project,
                      "Partial #" + std::to_string(partNum + 1) + " did not resolve to an envelope.",
                      "Bottom '" + name + "' / spectrum '" + XMLTC(_element.child("Name")) + "' / Spectrum",
                      "Check the partial's envelope expression and its referenced envelope definition.");
    }
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
  if (apply != "SOUND" && apply != "PARTIAL") {
    throw CmodError(CmodError::Kind::Project,
                    "Invalid spatialization Apply value '" + apply + "'.",
                    "Bottom '" + name + "' / Spatialization / Apply",
                    "Choose SOUND to spatialize the whole sound or PARTIAL to spatialize individual partials.");
  }

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
    throw CmodError(CmodError::Kind::Project,
                    "Unknown spatialization Method '" + method + "'.",
                    "Bottom '" + name + "' / Spatialization / Method",
                    "Choose STEREO, MULTI_PAN, or POLAR and provide the corresponding channel envelopes.");
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
      warnBottom(name, "Spatialization / STEREO / Channels",
                 "The sound has no panning envelope; stereo spatialization is skipped.",
                 "Add the sound's panning envelope to the spatialization definition.");
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
        warnBottom(name, "Spatialization / STEREO / Channels / partial #" + std::to_string(i + 1),
                   "This partial has no panning envelope; its spatialization is skipped.",
                   "Add a panning envelope for this partial if it should be spatialized.");
      } else {
        Pan stereoPan = computeSpatializationStereo(envstr);
        s->get(i).setSpatializer(stereoPan);
        // ^ this cannot be wrapped into computeSpatializationStereo
        // since Sound and Partial does not inherit each other
      }
      envelopeElement = GNES(envelopeElement);
    }

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
    if (isPartialValid.empty()) {
      throw CmodError(CmodError::Kind::Project,
                      "MULTI_PAN has no channel envelopes for the sound.",
                      "Bottom '" + name + "' / Spatialization / Channels",
                      "Add a non-empty envelope for each channel in the MULTI_PAN spatialization definition.");
    }
    if (isPartialValid.at(0) == false) {
      warnBottom(name, "Spatialization / MULTI_PAN / Channels",
                 "At least one channel envelope is empty; spatialization of the sound is skipped.",
                 "Provide a non-empty envelope for every channel of the sound.");
      return;
    }
    MultiPan multipan = computeSpatializationMultiPan(mults.at(0));
    s->setSpatializer(multipan);

  } else if (applyHow == "PARTIAL") {
    for (unsigned i = 0; (int)i < numParts; i++) { // apply multipan to each partial
      if (mults.size() <= i){
        warnBottom(name, "Spatialization / MULTI_PAN / Channels / partial #" + std::to_string(i + 1),
                   "No channel envelopes remain; spatialization of this and later partials is skipped.",
                   "Add channel envelopes for any remaining partials that should be spatialized.");
        break;
      }
      if (isPartialValid.at(i) == false) {
        warnBottom(name, "Spatialization / MULTI_PAN / Channels / partial #" + std::to_string(i + 1),
                   "A channel envelope is empty; spatialization of this partial is skipped.",
                   "Provide a non-empty envelope for every channel of this partial.");
        continue;
      }
      MultiPan multipan = computeSpatializationMultiPan(mults.at(i));
      s->get(i).setSpatializer(multipan);
    }

  }
}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
MultiPan Bottom::computeSpatializationMultiPan(vector<Envelope*> mult) {

  MultiPan multipan(static_cast<int>(mult.size()), mult);

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
      warnBottom(name, "Spatialization / POLAR / Channels",
                 "The sound is missing a theta or radius envelope; polar spatialization is skipped.",
                 "Provide both theta and radius envelopes in the spatialization definition.");
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
        warnBottom(name, "Spatialization / POLAR / Channels / partial #" + std::to_string(i + 1),
                   "This partial is missing a theta or radius envelope; its spatialization is skipped.",
                   "Provide both envelopes for this partial if it should be spatialized.");
      } else {
        MultiPan multipan = computeSpatializationPolar(theta, radius);
        s->get(i).setSpatializer(multipan);
      }
      thetaElement = GNES(thetaElement);
      radiusElement = GNES(radiusElement);
    }

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
      theta = static_cast<float>(PI * thetaEnv->getScaledValueNew(time, 1.0));
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
  pugi::xml_node evaluatedFilter = utilities->evaluateFil((void*) this);
  if (evaluatedFilter == NULL) return; //no filter

//  <Fun>
//    <Name>MakeFilter</Name>
//    <Type>HPF</Type>
//    <Frequency>2000</Frequency>
//    <BandWidth>4.5</BandWidth>
//    <dBGain/>
//  </Fun>
  pugi::xml_node it = GNES(GFEC(evaluatedFilter));
  string filterType = XMLTC(it);
  it = GNES(it);
  double frequency = utilities->evaluate(XMLTC(it), (void*)this);
  it = GNES(it);
  double bandWidth = utilities->evaluate(XMLTC(it), (void*)this);
  it = GNES(it);
  double gain = utilities->evaluate(XMLTC(it), (void*)this);

  int typeInt;
  if (filterType =="LPF") typeInt = 0;
  else if (filterType == "HPF") typeInt =1;
  else if (filterType == "BPF") typeInt =2;
  else if (filterType == "NF") typeInt =3;
  else if (filterType == "PBEQF") typeInt =4;
  else if (filterType == "LSF") typeInt =5;
  else if (filterType == "HSF") typeInt =6;
  else {
    throw CmodError(CmodError::Kind::Project,
                    "Unknown filter Type '" + filterType + "'.",
                    "Bottom '" + name + "' / Filter / Type",
                    "Choose LPF, HPF, BPF, NF, PBEQF, LSF, or HSF, or remove the filter if it is not needed.");
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
        static_cast<m_sample_type>(gain),
        static_cast<m_sample_type>(frequency),
        static_cast<m_sample_type>(utilities->getSamplingRate()),
        static_cast<m_sample_type>(bandWidth));

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
  if (!reverbElement) return; // Reverberation is optional.

//this call will return a rev function, just in case users use "select" here.
//The string here is just a dummy since the callee will find the right rev
//element  within "this".

  // Assume this correctly gets <Name>
  string rev_method =  XMLTC(GFEC(reverbElement));

  pugi::xml_node applyHowElement = GNES(GFEC(reverbElement));
  string rev_apply = XMLTC(applyHowElement);
  if (rev_apply != "SOUND" && rev_apply != "PARTIAL") {
    throw CmodError(CmodError::Kind::Project,
                    "Invalid reverb Apply value '" + rev_apply + "'.",
                    "Bottom '" + name + "' / Reverb / Apply",
                    "Choose SOUND to reverberate the whole sound or PARTIAL for per-partial reverberation.");
  }

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
    throw CmodError(CmodError::Kind::Project,
                    "Unknown reverb method '" + rev_method + "'.",
                    "Bottom '" + name + "' / Reverb / Method",
                    "Choose REV_Simple, REV_Medium, or REV_Advanced, or remove reverberation if it is not needed.");
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
        if (i + 1 < numPartials && !sizeElement) {
          warnBottom(name, "Reverb / REV_Simple / Sizes",
                     "Room sizes are missing for partial #" + std::to_string(i + 2)
                         + " and later partials; their reverberation is skipped.",
                     "Add room-size entries for the remaining partials if they should have reverberation.");
          break;
        }
	    }
      if (sizeElement) {
        warnBottom(name, "Reverb / REV_Simple / Sizes",
                   "Extra room-size entries beyond the " + std::to_string(numPartials) + " sound partials are ignored.",
                   "Remove unused entries or increase Number of Partials if those entries were intended to be used.");
      }
    }

}

//----------------------------------------------------------------------------//

/* ZIYUAN CHEN, July 2023 */
Reverb* Bottom::computeReverberationSimple(pugi::xml_node sizeElement, int iPartial) {

  float roomSize;

  string envstr = XMLTC(sizeElement);
  if (envstr == "") {
    warnBottom(name, "Reverb / REV_Simple / Room Size / "
                   + (iPartial < 0 ? string("sound") : "partial #" + std::to_string(iPartial + 1)),
               "Room Size is empty; using the default value 0.",
               "Set Room Size in the reverb definition if a non-default room is intended.");
    roomSize = 0.0;
  } else {
    roomSize = static_cast<float>(utilities->evaluate(envstr, (void*)this));
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

      if (i + 1 < numPartials
          && (!percentElement || !spreadElement || !allPassElement || !delayElement)) {
        warnBottom(name, "Reverb / REV_Medium / partial #" + std::to_string(i + 2),
                   "Reverb parameters are missing for this and later partials; their reverberation is skipped.",
                   "Provide a Percent envelope, Spread, All Pass gain, and Delay for each remaining partial that should have reverb.");
        break;
      }
    }

    if (percentElement || spreadElement || allPassElement || delayElement) {
      warnBottom(name, "Reverb / REV_Medium",
                 "Extra parameter entries beyond the " + std::to_string(numPartials) + " sound partials are ignored.",
                 "Match the reverb parameter lists to the intended number of partials.");
    }

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
      warnBottom(name, "Reverb / REV_Medium / Percent / "
                     + (iPartial < 0 ? string("sound") : "partial #" + std::to_string(iPartial + 1)),
                 "The Percent envelope is empty; this reverberation effect is skipped.",
                 "Provide a Percent envelope if this sound or partial should have reverberation.");
      return NULL;
    }
    Envelope* percent_rev =
	 (Envelope*) utilities->evaluateObject(envstr, this, eventEnv);

    //3 floats:  hi/low spread, gain all pass, delay
    float hi_low_spread = static_cast<float>(utilities->evaluate(XMLTC(spreadElement),this));
    float gain_all_pass = static_cast<float>(utilities->evaluate(XMLTC(allPassElement),this));
    float delay = static_cast<float>(utilities->evaluate(XMLTC(delayElement),this));

    if (!std::isfinite(delay) || delay < 0) {
      delete percent_rev;
      throw CmodError(CmodError::Kind::Project,
                      "Reverb Delay evaluated to " + std::to_string(delay) + " seconds.",
                      "Bottom '" + name + "' / Reverb / REV_Medium / Delay",
                      "Use a finite, non-negative Delay in seconds; zero disables this reverberation effect.");
    }
    if (delay == 0) {
      warnBottom(name, "Reverb / REV_Medium / Delay / "
                     + (iPartial < 0 ? string("sound") : "partial #" + std::to_string(iPartial + 1)),
                 "Delay is zero; this reverberation effect is skipped.",
                 "Use a positive Delay in seconds if reverberation is intended.");
      delete percent_rev;
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

      if (i + 1 < numPartials && (!percentElement || !combGainListElement
          || !lpGainListElement || !allPassElement || !delayElement)) {
        warnBottom(name, "Reverb / REV_Advanced / partial #" + std::to_string(i + 2),
                   "Reverb parameters are missing for this and later partials; their reverberation is skipped.",
                   "Provide Percent, Comb Gain List, LP Gain List, All Pass gain, and Delay entries for each remaining partial that should have reverb.");
        break;
      }
    }

    if (percentElement || combGainListElement || lpGainListElement ||
        allPassElement || delayElement) {
      warnBottom(name, "Reverb / REV_Advanced",
                 "Extra parameter entries beyond the " + std::to_string(numPartials) + " sound partials are ignored.",
                 "Match the reverb parameter lists to the intended number of partials.");
    }

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
      warnBottom(name, "Reverb / REV_Advanced / Percent / "
                     + (iPartial < 0 ? string("sound") : "partial #" + std::to_string(iPartial + 1)),
                 "The Percent envelope is empty; this reverberation effect is skipped.",
                 "Provide a Percent envelope if this sound or partial should have reverberation.");
      return NULL;
    }
    Envelope* percent_rev =
         (Envelope*) utilities->evaluateObject(envstr, this, eventEnv);

//    //list of EXACTLY 6 comb gain filters
    vector<std::string> stringListC =
			utilities->listElementToStringVector(combGainListElement);
    if (stringListC.size() != 6) {
      delete percent_rev;
      throw CmodError(CmodError::Kind::Project,
                      "Comb Gain List has " + std::to_string(stringListC.size()) + " entries; expected 6.",
                      "Bottom '" + name + "' / Reverb / REV_Advanced / Comb Gain List",
                      "Provide exactly six comma-separated comb-filter gain values.");
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
      delete percent_rev;
      throw CmodError(CmodError::Kind::Project,
                      "LP Gain List has " + std::to_string(stringListG.size()) + " entries; expected 6.",
                      "Bottom '" + name + "' / Reverb / REV_Advanced / LP Gain List",
                      "Provide exactly six comma-separated low-pass gain values.");
    }
    vector<float> lp_gain_list;

    for (unsigned i = 0; i < stringListG.size(); i ++){
      float num = (float) utilities ->evaluate(stringListG[i], this);
      lp_gain_list.push_back(num);
    }

    //2 floats:  gain all pass, delay
    float gain_all_pass = static_cast<float>(utilities->evaluate(XMLTC(allPassElement),this));
    float delay = static_cast<float>(utilities->evaluate(XMLTC(delayElement),this));

    if (!std::isfinite(delay) || delay < 0) {
      delete percent_rev;
      throw CmodError(CmodError::Kind::Project,
                      "Reverb Delay evaluated to " + std::to_string(delay) + " seconds.",
                      "Bottom '" + name + "' / Reverb / REV_Advanced / Delay",
                      "Use a finite, non-negative Delay in seconds; zero disables this reverberation effect.");
    }
    if (delay == 0) {
      warnBottom(name, "Reverb / REV_Advanced / Delay / "
                     + (iPartial < 0 ? string("sound") : "partial #" + std::to_string(iPartial + 1)),
                 "Delay is zero; this reverberation effect is skipped.",
                 "Use a positive Delay in seconds if reverberation is intended.");
      delete percent_rev;
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

void Bottom::initializeModifierUsage(pugi::xml_node modifierUsageElement) {
  using namespace dissco::modifier_usage;

  modifierUsageRuntime = std::make_unique<ModifierUsageRuntime>();

  vector<string> adapterDiagnostics;
  Config config;
  const bool useLegacyDefaults = !modifierUsageElement;

  if (useLegacyDefaults) {
    cerr << "WARNING: Bottom '" << name
         << "' has no <ModifierUsage> marker. Legacy <ModifierGroup> is "
            "ignored; using best-effort Modifier Usage with per-sound "
            "sampling, default ON chance 1, and stable synthetic IDs for "
            "modifiers without <Usage> metadata."
         << endl;
    config.scope = SamplingScope::PerSound;
  } else {
    const string version = modifierUsageElement.attribute("version").value();
    if (version != "1") {
      adapterDiagnostics.push_back(
          "unsupported or missing ModifierUsage version '" + version + "'.");
    }

    const string samplingScope =
        modifierUsageElement.attribute("samplingScope").value();
    if (samplingScope == "per-sound") {
      config.scope = SamplingScope::PerSound;
    } else if (samplingScope == "per-bottom") {
      config.scope = SamplingScope::PerBottom;
    } else {
      config.scope = static_cast<SamplingScope>(-1);
      adapterDiagnostics.push_back(
          "samplingScope must be 'per-sound' or 'per-bottom'.");
    }
  }

  pugi::xml_document mergedModifiersDoc;
  pugi::xml_node mergedModifiers =
      mergedModifiersDoc.append_child("Modifiers");
  if (modifiersElement) {
    for (auto modifier = GFEC(modifiersElement); modifier;
         modifier = GNES(modifier)) {
      mergedModifiers.append_copy(modifier);
    }
  }
  if (ancestorModifiersElement) {
    for (auto modifier = GFEC(ancestorModifiersElement); modifier;
         modifier = GNES(modifier)) {
      mergedModifiers.append_copy(modifier);
    }
  }

  // Reserve every explicit ID before generating any fallback IDs. This keeps
  // synthesis deterministic while avoiding a collision with a later modifier.
  std::unordered_set<ModifierId> reservedModifierIds;
  for (auto modifierElement = mergedModifiers.child("Modifier");
       modifierElement;
       modifierElement = modifierElement.next_sibling("Modifier")) {
    const ModifierId explicitId =
        modifierElement.child("Usage").attribute("id").value();
    if (!explicitId.empty()) {
      reservedModifierIds.insert(explicitId);
    }
  }

  int modifierPosition = 0;
  for (auto modifierElement = mergedModifiers.child("Modifier");
       modifierElement;
       modifierElement = modifierElement.next_sibling("Modifier")) {
    ++modifierPosition;
    Entry entry;
    pugi::xml_node usageElement = modifierElement.child("Usage");
    if (!usageElement) {
      if (useLegacyDefaults) {
        string syntheticId =
            "__legacy_modifier_" + std::to_string(modifierPosition);
        int collisionSuffix = 2;
        while (reservedModifierIds.find(syntheticId)
               != reservedModifierIds.end()) {
          syntheticId = "__legacy_modifier_"
              + std::to_string(modifierPosition) + "_"
              + std::to_string(collisionSuffix++);
        }
        reservedModifierIds.insert(syntheticId);
        entry.id = std::move(syntheticId);
        entry.defaultOnChance = 1.0;
      } else {
        adapterDiagnostics.push_back(
            "modifier #" + std::to_string(modifierPosition)
            + " has no appended <Usage> metadata.");
        // Keep one invalid entry in the configured order so the shared
        // compiler can also report the empty identity.
        entry.defaultOnChance = std::numeric_limits<double>::quiet_NaN();
      }
      modifierUsageRuntime->modifierIds.push_back(entry.id);
      config.orderedModifiers.push_back(std::move(entry));
      continue;
    }

    entry.id = usageElement.attribute("id").value();
    if (useLegacyDefaults && entry.id.empty()) {
      string syntheticId =
          "__legacy_modifier_" + std::to_string(modifierPosition);
      int collisionSuffix = 2;
      while (reservedModifierIds.find(syntheticId)
             != reservedModifierIds.end()) {
        syntheticId = "__legacy_modifier_"
            + std::to_string(modifierPosition) + "_"
            + std::to_string(collisionSuffix++);
      }
      reservedModifierIds.insert(syntheticId);
      entry.id = std::move(syntheticId);
    }
    modifierUsageRuntime->modifierIds.push_back(entry.id);
    if (!parseStrictDouble(usageElement.attribute("defaultOn").value(),
                           entry.defaultOnChance)) {
      entry.defaultOnChance = std::numeric_limits<double>::quiet_NaN();
      adapterDiagnostics.push_back(
          "modifier '" + entry.id + "' has an invalid defaultOn value.");
    }

    pugi::xml_node exceptionsElement = usageElement.child("Exceptions");
    for (auto exceptionElement = exceptionsElement.child("Exception");
         exceptionElement;
         exceptionElement = exceptionElement.next_sibling("Exception")) {
      Rule rule;
      if (!parseStrictDouble(exceptionElement.attribute("onChance").value(),
                             rule.onChance)) {
        rule.onChance = std::numeric_limits<double>::quiet_NaN();
        adapterDiagnostics.push_back(
            "modifier '" + entry.id
            + "' has an exception with an invalid onChance value.");
      }

      for (auto whenElement = exceptionElement.child("When");
           whenElement;
           whenElement = whenElement.next_sibling("When")) {
        Predicate predicate;
        predicate.modifierId =
            whenElement.attribute("modifierId").value();
        const string state = whenElement.attribute("state").value();
        if (state == "on") {
          predicate.requiredOn = true;
        } else if (state == "off") {
          predicate.requiredOn = false;
        } else {
          adapterDiagnostics.push_back(
              "modifier '" + entry.id
              + "' has a condition whose state is not 'on' or 'off'.");
        }
        if (predicate.modifierId.empty()) {
          adapterDiagnostics.push_back(
              "modifier '" + entry.id
              + "' has a condition with no modifierId.");
        }
        rule.when.push_back(std::move(predicate));
      }
      entry.rules.push_back(std::move(rule));
    }
    config.orderedModifiers.push_back(std::move(entry));
  }

  CompileOptions compileOptions;
  compileOptions.overallUsageMode = OverallUsageMode::Skip;
  CompileResult compiled = compile(std::move(config), compileOptions);
  string diagnostics;
  for (const string& diagnostic : adapterDiagnostics) {
    diagnostics += diagnostic + " ";
  }
  for (const Diagnostic& diagnostic : compiled.diagnostics) {
    diagnostics += diagnostic.message + " ";
  }

  if (!diagnostics.empty() || !compiled.program.has_value()) {
    throw CmodError(CmodError::Kind::Project,
                    "Invalid Modifier Usage configuration: " + diagnostics,
                    "Bottom '" + name + "' / Modifier Usage",
                    "Correct the listed Modifier Usage settings: version 1, per-sound or per-bottom scope, unique IDs, and ON chances between 0 and 1.");
  }
  modifierUsageRuntime->program.emplace(std::move(*compiled.program));
}

//-----------------------------------------------------------------------------/

void Bottom::applyModifierUsage(Sound *s, int numPartials) {
  using dissco::modifier_usage::ModifierId;
  using dissco::modifier_usage::Selection;

  struct RuntimeModifier {
    std::unique_ptr<Modifier> effect;
    bool applyByPartial = false;
  };

  std::unordered_map<ModifierId, vector<RuntimeModifier>> modifiersById;
  auto runtimeError = [this](const string& message) {
    throw CmodError(CmodError::Kind::Project,
                    message,
                    "Bottom '" + name + "' / Modifiers",
                    "Correct the named modifier's type, application mode, or required parameters in the Bottom event (including inherited modifiers), then run again.");
  };

  pugi::xml_document mergedModifiersDoc;
  pugi::xml_node mergedModifiers =
      mergedModifiersDoc.append_child("Modifiers");
  if (modifiersElement) {
    for (auto modifier = GFEC(modifiersElement); modifier;
         modifier = GNES(modifier)) {
      mergedModifiers.append_copy(modifier);
    }
  }
  if (ancestorModifiersElement) {
    for (auto modifier = GFEC(ancestorModifiersElement); modifier;
         modifier = GNES(modifier)) {
      mergedModifiers.append_copy(modifier);
    }
  }

  std::size_t modifierPosition = 0;
  for (auto modifierElement = mergedModifiers.child("Modifier");
       modifierElement;
       modifierElement = modifierElement.next_sibling("Modifier")) {
    if (modifierPosition >= modifierUsageRuntime->modifierIds.size()) {
      runtimeError("the compiled modifier order does not match the XML.");
      break;
    }
    const ModifierId& usageId =
        modifierUsageRuntime->modifierIds[modifierPosition++];
    if (usageId.empty()) {
      runtimeError("a modifier has no usable Modifier Usage ID.");
      continue;
    }

    const string typeExpression = XMLTC(modifierElement.child("Type"));
    const double typeValue = utilities->evaluate(typeExpression, this);
    if (!std::isfinite(typeValue) || typeValue < 0 || typeValue > 7
        || std::floor(typeValue) != typeValue) {
      std::ostringstream value;
      value << typeValue;
      runtimeError("modifier '" + usageId + "' has invalid Type " + value.str()
                   + " (expression: " + typeExpression + "); choose an integer type "
                     "from 0 (TREMOLO) through 7 (PHASE_MOD).");
    }
    const int modTypeCode = static_cast<int>(typeValue);
    string modType;
    switch (modTypeCode) {
      case 0: modType = "TREMOLO"; break;
      case 1: modType = "VIBRATO"; break;
      case 2: modType = "GLISSANDO"; break;
      case 3: modType = "DETUNE"; break;
      case 4: modType = "AMPTRANS"; break;
      case 5: modType = "FREQTRANS"; break;
      case 6: modType = "WAVE_TYPE"; break;
      case 7: modType = "PHASE_MOD"; break;
      default:
        runtimeError("modifier '" + usageId
                     + "' has unknown Type " + std::to_string(modTypeCode)
                     + "; choose a modifier type from 0 (TREMOLO) through 7 (PHASE_MOD).");
        continue;
    }

    const string applyExpression = XMLTC(modifierElement.child("ApplyHow"));
    const double applyValue = utilities->evaluate(applyExpression, this);
    if (!std::isfinite(applyValue) || (applyValue != 0 && applyValue != 1)) {
      std::ostringstream value;
      value << applyValue;
      runtimeError("modifier '" + usageId + "' has invalid ApplyHow " + value.str()
                   + " (expression: " + applyExpression + "); use 0 (SOUND) or 1 (PARTIAL).");
    }
    const int applyHowCode = static_cast<int>(applyValue);
    const bool applyByPartial = applyHowCode == 1;

    const string ampStr = XMLTC(modifierElement.child("Amplitude"));
    const string rateStr = XMLTC(modifierElement.child("Rate"));
    const string widthStr = XMLTC(modifierElement.child("Width"));
    const string spreadStr = XMLTC(modifierElement.child("DetuneSpread"));
    const string directionStr = XMLTC(modifierElement.child("DetuneDirection"));
    const string velocityStr = XMLTC(modifierElement.child("DetuneVelocity"));

    double detuneSpread = 0.0;
    double detuneDirection = 0.0;
    double detuneVelocity = 0.0;
    if (!applyByPartial && modType == "DETUNE") {
      if (isUnavailable(spreadStr) || isUnavailable(directionStr)
          || isUnavailable(velocityStr)) {
        runtimeError("DETUNE modifier '" + usageId
                     + "' requires spread, direction, and velocity.");
        continue;
      }
      if (!parseStrictDouble(spreadStr.c_str(), detuneSpread)
          || !std::isfinite(detuneSpread)
          || detuneSpread < 0.0 || detuneSpread > 1.0) {
        runtimeError("DETUNE modifier '" + usageId
                     + "' has an invalid spread; expected a finite value "
                       "between 0 and 1.");
        continue;
      }
      if (!parseStrictDouble(directionStr.c_str(), detuneDirection)
          || !std::isfinite(detuneDirection)
          || detuneDirection == 0.0) {
        runtimeError("DETUNE modifier '" + usageId
                     + "' has an invalid direction; expected a finite, "
                       "non-zero value.");
        continue;
      }
      if (!parseStrictDouble(velocityStr.c_str(), detuneVelocity)
          || !std::isfinite(detuneVelocity)
          || detuneVelocity < -1.0 || detuneVelocity > 1.0) {
        runtimeError("DETUNE modifier '" + usageId
                     + "' has an invalid velocity; expected a finite value "
                       "between -1 and 1.");
        continue;
      }
      detuneDirection = detuneDirection < 0.0 ? -1.0 : 1.0;
    }

    auto addEnvelope = [this, &runtimeError, &usageId](
                           Modifier& modifier,
                           const string& value,
                           const char* fieldName) {
      if (isUnavailable(value)) {
        return false;
      }
      Envelope* envelope = static_cast<Envelope*>(
          utilities->evaluateObject(value, this, eventEnv));
      if (envelope == NULL) {
        runtimeError("modifier '" + usageId + "' has an invalid "
                     + fieldName + " envelope.");
        return false;
      }
      modifier.addValueEnv(envelope);
      delete envelope;
      return true;
    };

    if (!applyByPartial) {
      auto effect = std::make_unique<Modifier>(modType, nullptr, "SOUND");
      int envelopeCount = 0;
      envelopeCount += addEnvelope(*effect, ampStr, "Amplitude") ? 1 : 0;
      if (modType == "DETUNE") {
        effect->addSpread(detuneSpread);
        effect->addDirection(detuneDirection);
        effect->addVelocity(detuneVelocity);
      }
      envelopeCount += addEnvelope(*effect, rateStr, "Rate") ? 1 : 0;
      envelopeCount += addEnvelope(*effect, widthStr, "Width") ? 1 : 0;

      int requiredEnvelopeCount = 0;
      if (modType == "TREMOLO" || modType == "VIBRATO"
          || modType == "PHASE_MOD") {
        requiredEnvelopeCount = 2;
      } else if (modType == "GLISSANDO" || modType == "WAVE_TYPE") {
        requiredEnvelopeCount = 1;
      } else if (modType == "AMPTRANS" || modType == "FREQTRANS") {
        requiredEnvelopeCount = 3;
      }
      if (envelopeCount < requiredEnvelopeCount) {
        runtimeError("modifier '" + usageId
                     + "' (" + modType + ") is missing a required parameter envelope: "
                     + (isUnavailable(ampStr) ? "Amplitude " : "")
                     + (requiredEnvelopeCount >= 2 && isUnavailable(rateStr) ? "Rate " : "")
                     + (requiredEnvelopeCount >= 3 && isUnavailable(widthStr) ? "Width " : ""));
        continue;
      }
      modifiersById[usageId].push_back(
          RuntimeModifier{std::move(effect), false});
      continue;
    }

    const string partialResultStr =
        XMLTC(modifierElement.child("PartialResultString"));
    pugi::xml_document partialResultDoc;
    const pugi::xml_parse_result parseResult =
        partialResultDoc.load_string(partialResultStr.c_str());
    const pugi::xml_node root = partialResultDoc.document_element();
    pugi::xml_node envelopeElement =
        root.child("Envelopes").child("Envelope");
    if (!parseResult || !root || !envelopeElement) {
      runtimeError("PARTIAL modifier '" + usageId
                   + "' has an invalid PartialResultString.");
      continue;
    }

    // A logical PARTIAL modifier may intentionally have no enabled rows (for
    // example, every Probability slot is N/A). Keep the ID represented so it
    // remains a valid no-op when selected.
    vector<RuntimeModifier>& runtimeModifiers = modifiersById[usageId];
    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex) {
      // Fewer configured rows than spectrum partials is intentional: the
      // remaining partials simply do not receive this modifier. Once a row
      // starts, however, the legacy wire format still requires all four slots.
      if (!envelopeElement) {
        break;
      }
      pugi::xml_node probabilityElement = envelopeElement;
      pugi::xml_node amplitudeElement =
          probabilityElement.next_sibling("Envelope");
      pugi::xml_node widthElement =
          amplitudeElement.next_sibling("Envelope");
      pugi::xml_node rateElement =
          widthElement.next_sibling("Envelope");
      if (!probabilityElement || !amplitudeElement
          || !widthElement || !rateElement) {
        runtimeError("PARTIAL modifier '" + usageId
                     + "' contains fewer than four envelopes per partial.");
        break;
      }

      const string probabilityStr = XMLTC(probabilityElement);
      if (isUnavailable(probabilityStr)) {
        envelopeElement = rateElement.next_sibling("Envelope");
        continue;
      }

      Envelope* probabilityEnvelope = static_cast<Envelope*>(
          utilities->evaluateObject(probabilityStr, this, eventEnv));
      if (probabilityEnvelope == NULL) {
        runtimeError("PARTIAL modifier '" + usageId
                     + "' has an invalid Probability envelope.");
      }

      auto effect = std::make_unique<Modifier>(
          modType, probabilityEnvelope, "PARTIAL", partialIndex);
      delete probabilityEnvelope;

      int envelopeCount = 0;
      envelopeCount += addEnvelope(
          *effect, XMLTC(amplitudeElement), "partial Amplitude") ? 1 : 0;
      envelopeCount += addEnvelope(
          *effect, XMLTC(rateElement), "partial Rate") ? 1 : 0;
      if (modType != "PHASE_MOD") {
        envelopeCount += addEnvelope(
            *effect, XMLTC(widthElement), "partial Width") ? 1 : 0;
      }

      int requiredEnvelopeCount = 0;
      if (modType == "TREMOLO" || modType == "VIBRATO"
          || modType == "PHASE_MOD") {
        requiredEnvelopeCount = 2;
      } else if (modType == "GLISSANDO" || modType == "DETUNE"
                 || modType == "WAVE_TYPE") {
        requiredEnvelopeCount = 1;
      } else if (modType == "AMPTRANS" || modType == "FREQTRANS") {
        requiredEnvelopeCount = 3;
      }
      if (envelopeCount < requiredEnvelopeCount) {
        runtimeError("PARTIAL modifier '" + usageId
                     + "' is missing a required parameter envelope.");
        break;
      }

      runtimeModifiers.push_back(RuntimeModifier{std::move(effect), true});
      envelopeElement = rateElement.next_sibling("Envelope");
    }
  }

  // Program compilation and runtime effect parsing must describe the same
  // ordered logical modifiers. Check this before drawing or applying anything.
  for (const ModifierId& modifierId : modifierUsageRuntime->modifierIds) {
    const auto found = modifiersById.find(modifierId);
    if (found == modifiersById.end()) {
      runtimeError("compiled modifier '" + modifierId
                   + "' has no runtime effect.");
    }
  }
  Selection selection;
  try {
    selection = modifierUsageRuntime->program->select(
        []() { return Random::Rand(); });
  } catch (const CmodError&) {
    throw;
  } catch (const std::exception& error) {
    throw CmodError(CmodError::Kind::Internal,
                    "Modifier selection failed: " + string(error.what()),
                    "Bottom '" + name + "' / Modifier Usage selection",
                    "Report this diagnostic to the DISSCO developers with the project and seed.");
  }

  // Selection IDs are already in Program order. A selected PARTIAL logical
  // modifier still uses its existing per-partial Probability envelope as a
  // second-stage gate.
  for (const ModifierId& selectedId : selection.orderedOnIds) {
    auto found = modifiersById.find(selectedId);
    if (found == modifiersById.end()) {
      runtimeError("selection returned unknown modifier '" + selectedId + "'.");
      return;
    }
    for (RuntimeModifier& runtimeModifier : found->second) {
      Modifier& effect = *runtimeModifier.effect;
      if (runtimeModifier.applyByPartial) {
        if (effect.willOccur(checkPoint)) {
          effect.applyModifier(s);
        }
      } else {
        effect.setCheckPoint(checkPoint);
        effect.applyModifier(s);
      }
    }
  }
}

//-----------------------------------------------------------------------------/

void Bottom::applyModifiers(Sound *s, int numPartials) {
  applyModifierUsage(s, numPartials);
}

//----------------------------------------------------------------------------//

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
    string techniqueName = XMLTC(currentTechnique);
    modNames.push_back(techniqueName);
    currentTechnique = GNES(currentTechnique);
  }

  return modNames;
}

//----------------------------------------------------------------------------//

void Bottom::generatePartials(Sound* newsound, float frequency, float, float, Envelope* waveShape){

  if ((frequency < 233) || frequency > 932){
    warnBottom(name, "Generate Spectrum / Frequency",
               "Frequency is " + std::to_string(frequency)
                   + " Hz, outside Spectrum_Gen's reference range of 233 to 932 Hz; generation continues.",
               "Check the Bottom event's Frequency, or use explicit spectrum partial envelopes if this wider range is intentional.");
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
    partial.setParam(PARTIAL_NUM, static_cast<m_value_type>(i));

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
