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
//  Event.cpp
//
//----------------------------------------------------------------------------//

#include "Event.h"
#include "Output.h"
#include "Sieve.h"
#include "Random.h" 
#include "Bottom.h"
#include "CmodError.h"
#include "TimeSignature.h"
#include <cmath>
#include <limits>

//----------------------------------------------------------------------------//
//Checked
static Sieve* sieveSweep = NULL;
static vector<int> attackSweep;


Event::Event(pugi::xml_node _element,
             TimeSpan _timeSpan,
             int _type,
             Tempo _tempo,
             Utilities* _utilities,
             pugi::xml_node _ancestorSpa,
             pugi::xml_node _ancestorRev,
             pugi::xml_node _ancestorFil,
             pugi::xml_node _ancestorModifiers):
  type(_type),
  modifiersIncludingAncestorsElement(NULL),
  maxChildDur(0), checkPoint(0),
  previousChildEndTime(0.0f), sieveAligned(false),
  numChildren(0),
  restartsRemaining(0),
  currChildNum(0), childType(0),
  matrix(0),
  utilities(_utilities)
   {

  //Initialize parameters
  pugi::xml_node thisEventElement = GFEC(_element);
  string typeString =XMLTC(thisEventElement);
  type = atoi(typeString.c_str());

  thisEventElement = GNES(thisEventElement);
  name = XMLTC(thisEventElement);
  ts = _timeSpan;
  tempo = _tempo;

  thisEventElement = GNES(thisEventElement);
  maxChildDur = (float)utilities->evaluate(XMLTC(thisEventElement), (void*)this);

  thisEventElement = GNES(thisEventElement);
  const double eduPerBeat = utilities->evaluate(XMLTC(thisEventElement),(void*)this);
  if (!std::isfinite(eduPerBeat) || eduPerBeat < 1 || eduPerBeat > std::numeric_limits<int>::max()) {
    throw CmodError(CmodError::Kind::Project,
                    "EDU Per Beat must produce a positive integer within CMOD's timing range.",
                    "Event '" + name + "' -> EDU Per Beat: " + to_string(eduPerBeat),
                    "Set EDU Per Beat to a positive integer, such as 60; zero cannot define the timing grid.");
  }
  int newEDUPerBeat = static_cast<int>(eduPerBeat);
  Ratio k(newEDUPerBeat,1);
  Tempo fvTempo; // File-Value Tempo

  fvTempo.setEDUPerTimeSignatureBeat(k);

  thisEventElement = GNES(thisEventElement);
  fvTempo.setTimeSignature(getTimeSignatureStringFromDOMElement(thisEventElement));

  thisEventElement = GNES(thisEventElement);
  fvTempo.setTempo(getTempoStringFromDOMElement(thisEventElement));

  // Ratio<int> multiplies before reducing. Validate the same intermediate
  // products in a wider type before timing or score code evaluates them.
  const Ratio timingEDUs = fvTempo.getEDUPerTimeSignatureBeat();
  const Ratio timingBeatsPerBar = fvTempo.getTimeSignatureBeatsPerBar();
  const Ratio timingBeat = fvTempo.getTimeSignatureBeat();
  const Ratio timingTempoBeat = fvTempo.getTempoBeat();
  const Ratio timingBPM = fvTempo.getTempoBeatsPerMinute();
  const string timingContext = "Event '" + name + "' -> Time Signature: " +
    fvTempo.getTimeSignature() + "; EDU Per Beat: " + timingEDUs.toPrettyString() +
    "; Tempo: " + timingBPM.toPrettyString();
  const auto checkedTimingRatio = [&timingContext](Ratio left, Ratio right,
                                                  bool divide, const string& field) {
    const long long numerator = static_cast<long long>(left.Num()) *
                                (divide ? right.Den() : right.Num());
    const long long denominator = static_cast<long long>(left.Den()) *
                                  (divide ? right.Num() : right.Den());
    if (numerator <= 0 || denominator <= 0 ||
        numerator > std::numeric_limits<int>::max() ||
        denominator > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "The derived " + field + " is outside CMOD's supported integer timing range.",
                      timingContext + " -> " + field + ": " + to_string(numerator) + "/" + to_string(denominator),
                      "Reduce EDU Per Beat or the Time Signature values, or simplify the Tempo fraction. "
                      "Each intermediate timing numerator and denominator must fit within 1 to " +
                      to_string(std::numeric_limits<int>::max()) + ".");
    }
    return Ratio(static_cast<int>(numerator), static_cast<int>(denominator));
  };
  checkedTimingRatio(timingEDUs, timingBeatsPerBar, false, "EDU per bar");
  const Ratio beatsPerTempoBeat = checkedTimingRatio(timingTempoBeat, timingBeat, true, "beats per tempo beat");
  const Ratio tempoBeatsPerBeat = checkedTimingRatio(timingBeat, timingTempoBeat, true, "tempo beats per beat");
  checkedTimingRatio(timingBeatsPerBar, tempoBeatsPerBeat, false, "tempo beats per bar");
  const Ratio beatsPerMinute = checkedTimingRatio(timingBPM, beatsPerTempoBeat, false, "beats per minute");
  checkedTimingRatio(Ratio(60), timingBPM, true, "tempo beat duration");
  const Ratio secondsPerBeat = checkedTimingRatio(Ratio(60), beatsPerMinute, true, "time-signature beat duration");
  checkedTimingRatio(timingEDUs, beatsPerTempoBeat, false, "EDU per tempo beat");
  const Ratio edusPerMinute = checkedTimingRatio(timingEDUs, beatsPerMinute, false, "EDU per minute");
  checkedTimingRatio(edusPerMinute, Ratio(60), true, "EDU per second");
  checkedTimingRatio(secondsPerBeat, timingEDUs, true, "EDU duration");

  fvTempo.setStartTime(tempo.getStartTime());


  if(tempo.getStartTime() == 0){
    tempo = fvTempo;
  }
  else if(!tempo.isTempoSameAs(fvTempo)) { //Warn if different tempi
    cout << endl << "WARNING: the tempo of this exact event differs from" << endl
      << "that of its exact parent." << endl;
    cout << "Parent: " << endl;
    cout << "  " << tempo.getTempoBeatsPerMinute() << endl;
    cout << "  " << tempo.getTempoBeat() << endl;
    cout << "  " << tempo.getTimeSignatureBeat() << endl;
    cout << "  " << tempo.getTimeSignatureBeatsPerBar() << endl;
    cout << "  " << tempo.getEDUPerTimeSignatureBeat() << endl;
    cout << "  " << tempo.getStartTime() << endl;
    cout << "File-Value Tempo: " << endl;
    cout << "  " << fvTempo.getTempoBeatsPerMinute() << endl;
    cout << "  " << fvTempo.getTempoBeat() << endl;
    cout << "  " << fvTempo.getTimeSignatureBeat() << endl;
    cout << "  " << fvTempo.getTimeSignatureBeatsPerBar() << endl;
    cout << "  " << fvTempo.getEDUPerTimeSignatureBeat() << endl;
    cout << "  " << fvTempo.getStartTime() << endl;
  }

  tempo.setRootExactAncestor(this);

  //This part initializes childnum and childnames

  pugi::xml_node numChildrenElement = GNES(thisEventElement);

  //store the pointer to be used in buildChildren()
  childEventDefElement = GNES(numChildrenElement);
  childStartTimeElement = GFEC(childEventDefElement);
  childTypeElement = GNES(childStartTimeElement);
  childDurationElement = GNES(childTypeElement);
  AttackSieveElement = GNES(childDurationElement);
  DurationSieveElement = GNES(AttackSieveElement);
  methodFlagElement = GNES(DurationSieveElement);
  childStartTypeFlag = GNES(methodFlagElement);
  childDurationTypeFlag = GNES(childStartTypeFlag);

  //layers, initialize child names
  thisEventElement = GNES(childEventDefElement);
  pugi::xml_node layerElement = GFEC(thisEventElement);
  while (layerElement){

    layerElements.push_back(layerElement);
    pugi::xml_node childPackage = GFEC(GNES(GFEC(layerElement)));
    vector<string> layerNames;

    while(childPackage){
      childTypeElements.push_back(childPackage);
      layerNames.push_back(XMLTC(GFEC(childPackage)));
      childPackage = GNES(childPackage);
    }
    layerVect.push_back(layerNames);
    layerElement = GNES(layerElement);
  }


  const auto checkedChildCount = [this](double value) {
    if (!std::isfinite(value) || value < 0 || value > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "The number of children is outside the supported range.",
                      "Event '" + name + "' -> NumberOfChildren: " + to_string(value),
                      "Use a finite value between 0 and " + to_string(std::numeric_limits<int>::max()) + ".");
    }
    return static_cast<int>(value);
  };
  pugi::xml_node flagElement = GFEC(numChildrenElement);
  if (XMLTC(flagElement) =="0"){ // Continuum
    pugi::xml_node entry1Element = GNES(flagElement);
    if (XMLTC(entry1Element)==""){
      numChildren = checkedChildCount(static_cast<double>(childTypeElements.size()));
    }
    else {
      numChildren = checkedChildCount(utilities->evaluate(XMLTC(entry1Element), (void*)this));
    }
  }
  else if (XMLTC(flagElement) == "1"){ // Densitiy
    pugi::xml_node densityElement = GNES(GFEC(numChildrenElement));
    pugi::xml_node areaElement = GNES(densityElement);
    pugi::xml_node underOneElement = GNES(areaElement);
    double density = utilities->evaluate( XMLTC(densityElement),(void*)this);
    double area = utilities->evaluate( XMLTC(areaElement),(void*)this);
    if (area == 0) {
      throw CmodError(CmodError::Kind::Project,
                      "The Density calculation divides by an Area of zero.",
                      "Event '" + name + "' -> Number of Children -> Density -> Area: 0",
                      "Set Area to a nonzero value, or choose a different method for Number of Children.");
    }
//  cout << "areaElement=" << areaElement << endl;
    double underOne = utilities->evaluate( XMLTC(underOneElement),(void*)this);
    double soundsPsec = pow(2, density * area - underOne); //this can't be right..
//  cout<<"density:"<< density<<", area:"<<area<<", underOne:"<<underOne<<endl;

    //not sure which version is the correct one. ask sever
    numChildren = checkedChildCount(soundsPsec * ts.duration + underOne/area);
//  cout << "     numChildren=" << numChildren << endl;
    //numChildren = (int)(soundsPsec * layerElements * ts.duration + 0.5);

  }
  else {// by layer
  numChildren = 0;
    for (unsigned i = 0; i < layerElements.size(); i ++){
      const int layerCount = checkedChildCount(utilities->evaluate(XMLTC(GFEC(layerElements[i])),(void*)this));
      numChildren = checkedChildCount(static_cast<double>(numChildren) + layerCount);
    }
  }

  if (numChildren > 0 && childTypeElements.empty()) {
    throw CmodError(CmodError::Kind::Project,
                    "Children were requested, but no child events are listed in Layers.",
                    "Event '" + name + "' -> NumberOfChildren",
                    "Add child events to this event's Layers, or set Number of Children to zero.");
  }

  if (numChildren > 0 && XMLTC(methodFlagElement) != "2") {
    const auto validateUnit = [this](pugi::xml_node element, const string& field) {
      const string value = XMLTC(element);
      if (value != "0" && value != "1" && value != "2") {
        throw CmodError(CmodError::Kind::Project,
                        "The " + field + " is missing or not recognized.",
                        "Event '" + name + "' -> Child Event Definition -> " + field + ": '" + value + "'",
                        "Choose Fraction (0), EDU (1), or Seconds (2) for this timing field.");
      }
    };
    validateUnit(childStartTypeFlag, "Start Time Unit");
    validateUnit(childDurationTypeFlag, "Duration Unit");
  }

  if (type <=3){ //top, high, mid, low

    thisEventElement = GNES(thisEventElement);
    if (_ancestorSpa != NULL) {
      spatializationElement = _ancestorSpa;
    }
    else if (Utilities::removeSpaces(XMLTC(thisEventElement)) =="") {
        spatializationElement = pugi::xml_node();
    }
    else {
      spatializationElement = thisEventElement;
    }

    thisEventElement = GNES(thisEventElement);
    if (_ancestorRev != NULL) {
      reverberationElement = _ancestorRev;
    }
    else if (Utilities::removeSpaces(XMLTC(thisEventElement)) =="") {
        reverberationElement = pugi::xml_node();
    }
    else {
      reverberationElement = thisEventElement;
    }

    thisEventElement = GNES(thisEventElement);
    if (_ancestorFil != NULL) {
      filterElement = _ancestorFil;
    }
    else if (Utilities::removeSpaces(XMLTC(thisEventElement)) =="") {
        filterElement = pugi::xml_node();
    }
    else {
      filterElement = thisEventElement;
    }


    thisEventElement = GNES(thisEventElement);
    modifiersElement = thisEventElement;

    // Build a private document holding this event's Modifiers element with any
    // ancestor modifier children appended onto it. The Event owns this doc;
    // children receive the resulting node as their _ancestorModifiers handle.
    modifiersDoc = std::make_unique<pugi::xml_document>();
    modifiersIncludingAncestorsElement = modifiersDoc->append_copy(modifiersElement);
    if (_ancestorModifiers) {
      for (auto a = GFEC(_ancestorModifiers); a; a = GNES(a)) {
        modifiersIncludingAncestorsElement.append_copy(a);
      }
    }
  } // end event type = 0, 1, 2, 3

}


//----------------------------------------------------------------------------//

string Event::getTempoStringFromDOMElement(pugi::xml_node _element){

  /*
  <Tempo>
        <MethodFlag>0</MethodFlag>
        <Prefix>0</Prefix>
        <NoteValue>2</NoteValue>
        <FractionEntry1></FractionEntry1>
        <FractionEntry2></FractionEntry2>
        <ValueEntry>60</ValueEntry>
      </Tempo>
  */
  pugi::xml_node thisElement = GFEC(_element);
  string stringbuffer = "";
  string methodFlag = XMLTC(thisElement); //it's either 0 or 1

  thisElement = GNES(thisElement);
  string prefix =  XMLTC(thisElement);

  thisElement = GNES(thisElement);
  string noteValue = XMLTC(thisElement);

  thisElement = GNES(thisElement);
  double fractionEntry1 = utilities->evaluate(XMLTC(thisElement),(void*)this);

  thisElement = GNES(thisElement);
  utilities->evaluate(XMLTC(thisElement),(void*)this);

  thisElement = GNES(thisElement);
  double valueEntry = utilities->evaluate(XMLTC(thisElement),(void*)this);

  const auto checkTempoValue = [this](double value, const string& field) {
    if (!std::isfinite(value) || value <= 0 || value > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "A Tempo value is zero, negative, or too large for CMOD's timing representation.",
                      "Event '" + name + "' -> Tempo -> " + field + ": " + to_string(value),
                      "Use a positive finite Tempo value no larger than " +
                      to_string(std::numeric_limits<int>::max()) + ".");
    }
  };
  checkTempoValue(valueEntry, "Value");
  if (methodFlag != "0" && methodFlag != "1") {
    throw CmodError(CmodError::Kind::Project,
                    "The Tempo method is not recognized.",
                    "Event '" + name + "' -> Tempo -> MethodFlag: '" + methodFlag + "'",
                    "Choose a note-value Tempo (0) or a fractional Tempo (1).");
  }

  if (prefix == "1"){
      stringbuffer = stringbuffer + "dotted ";
    }
    else if (prefix == "2" ){
      stringbuffer = stringbuffer + "double dotted ";
    }
    else if (prefix =="3"){
      stringbuffer = stringbuffer + "triple ";
    }
    else if (prefix =="4"){
      stringbuffer = stringbuffer + "quintuple ";
    }
    else if (prefix =="5"){
      stringbuffer = stringbuffer + "sextuple ";
    }
    else if (prefix =="6"){
      stringbuffer = stringbuffer + "septuple ";
    }

    if (noteValue == "0"){
      stringbuffer = stringbuffer + "whole = ";
    }
    else if (noteValue == "1"){
      stringbuffer = stringbuffer + "half = ";
    }
    else if (noteValue == "2"){
      stringbuffer = stringbuffer + "quarter = ";
    }
    else if (noteValue == "3"){
      stringbuffer = stringbuffer + "eighth = ";
    }
    else if (noteValue == "4"){
      stringbuffer = stringbuffer + "sixteenth = ";
    }
    else if (noteValue == "5"){
      stringbuffer = stringbuffer + "thirtysecond = ";
    }

  // Preserve the existing six-decimal precision without overflowing fixed
  // buffers or the integer numerator while formatting a Tempo.
  long long numerator = std::llround(valueEntry * 1000000.0);
  long long denominator = 1000000;
  if (methodFlag == "1") {
    checkTempoValue(fractionEntry1, "Fraction numerator");
    numerator = std::llround(fractionEntry1 * 60.0 * 1000000.0);
    denominator = std::llround(valueEntry * 1000000.0);
  }
  Rational<long long> ratio(numerator, denominator);
  if (numerator <= 0 || denominator <= 0 ||
      ratio.Num() > std::numeric_limits<int>::max() ||
      ratio.Den() > std::numeric_limits<int>::max()) {
    throw CmodError(CmodError::Kind::Project,
                    "The Tempo cannot be represented by CMOD's integer timing ratios.",
                    "Event '" + name + "' -> Tempo -> Value: " + to_string(valueEntry),
                    "Use a positive Tempo with fewer decimal places and smaller fractional values; "
                    "the value must remain positive when rounded to six decimal places.");
  }
  stringbuffer += ratio.toPrettyString();
  return stringbuffer;
}


//----------------------------------------------------------------------------//

string Event::getTimeSignatureStringFromDOMElement(pugi::xml_node _element){
/*
<TimeSignature>
        <Entry1>4</Entry1>
        <Entry2>4</Entry2>
      </TimeSignature>
  */

  const auto signatureEntry = [this](pugi::xml_node element, const string& field) {
    const double value = utilities->evaluate(XMLTC(element),(void*)this);
    if (!std::isfinite(value) || value < 1 || value > std::numeric_limits<int>::max()) {
      throw CmodError(CmodError::Kind::Project,
                      "The Time Signature must have a positive numerator and denominator.",
                      "Event '" + name + "' -> Time Signature -> " + field + ": " + to_string(value),
                      "Use positive integers for both Time Signature fields, such as 4/4.");
    }
    return static_cast<int>(value);
  };
  pugi::xml_node thisElement = GFEC(_element);
  int entry1 = signatureEntry(thisElement, "Numerator");

  char charbuffer[20];
  sprintf(charbuffer, "%d", entry1);
  string stringbuffer =  string(charbuffer);

  thisElement = GNES(thisElement);
  int entry2 = signatureEntry(thisElement, "Denominator");
  sprintf(charbuffer, "%d", entry2);
  string returnString = stringbuffer + "/"+ string(charbuffer);

  // The denominator is the note value of one beat, which the score must
  // write as a note value (1 = whole, 2 = half, 4 = quarter, ...). Sound
  // alone can use any beat, so only projects with score output need this.
  if (utilities->getScorePrinting() && !TimeSignature::IsPowerOf2(entry2)) {
    throw CmodError(CmodError::Kind::Project,
                    "The Time Signature " + returnString + " cannot be notated: its denominator must be a power of two.",
                    "Event '" + name + "' -> Time Signature: " + returnString,
                    "Use a denominator of 1, 2, 4, 8, 16, ... (such as 3/4 or 3/8), "
                    "and write triplets or other tuplets through EDU Per Beat instead.");
  }

  return returnString;
}


//----------------------------------------------------------------------------//

void Event::buildChildren() {
  if (utilities->getOutputParticel()){
  //Begin this sub-level in the output and write out its properties.
    Output::beginSubLevel(name);
    outputProperties();
  }

  string eventclass;
  switch (type){
    case 0: eventclass = "T         : ";break;
    case 1: eventclass = "--H       : ";break;
    case 2: eventclass = "----M     : ";break;
    case 3: eventclass = "------L   : ";break;
  }
  //Build the event's children.

  //Create the event definition iterator.
        /*
        <Entry1>1</Entry1>
        <Entry2>2</Entry2>
        <Entry3>3</Entry3>
        <AttackSieve>4</AttackSieve>
        <DurationSieve>5</DurationSieve>
        <DefinitionFlag>1</DefinitionFlag>
        <StartTypeFlag>2</StartTypeFlag>
        <DurationTypeFlag>2</DurationTypeFlag>
        */
  string method = XMLTC(methodFlagElement);


  //Set the number of possible restarts (for buildDiscrete)
  restartsRemaining = restartsNormallyAllowed;

  //Make sure that the temporary child events array is clear.
  if(temporaryChildEvents.size() > 0) {
    throw CmodError(CmodError::Kind::Internal,
                    "Child generation started with unfinished temporary events.",
                    "Event '" + name + "' -> child generation",
                    "Report this error with the project file and the complete diagnostic.");
  }

  /*  old code. --Ming-ching May 06, 2013
  //Make sure the childType indexes correctly.
  if (childType >= typeVect.size() || typeVect[childType] == "") {
    cerr << "There is a mismatch between childType and typeVect." << endl;
    exit(1);
  }
  */
  //Create the child events.
  for (currChildNum = 0; currChildNum < numChildren; currChildNum++) {
    if (method == "0")
      checkEvent(buildContinuum());
    else if (method == "1")
      checkEvent(buildSweep());

    else if (method == "2")
      checkEvent(buildDiscrete());
    else {
      throw CmodError(CmodError::Kind::Project,
                      "The child generation method is not recognized.",
                      "Event '" + name + "' -> Child Event Definition -> DefinitionFlag: '" + method + "'",
                      "Choose Continuum (0), Sweep (1), or Discrete (2) as the child generation method.");
    }
  }

  //Using the temporary events that were created, construct the actual children.
  for (unsigned i = 0; i < temporaryChildEvents.size(); i++) {

    //Increment the static current child number.
    currChildNum = i;

    //build child.
    Event *e = temporaryChildEvents[currChildNum];
    childEvents.push_back(e);
  }

  //Clear the temporary event list.
  temporaryChildEvents.clear();

  //For each child that was created, build its children.
  for(unsigned i = 0; i < childEvents.size(); i++){
    //overload in bottom
	/*
		RISKY METHOD 1

		If we are to stop the generation here
		call new method childEvents[i]->buildEmptyChildren()
			in place of buildChildren,
		then

		!!! REFACTOR THIS INTO A SINGLE CALLABLE METHOD !!!
		if (soundSynthesis){
    		MultiTrack* renderedScore = utilities->doneCMOD();
			!!! REPLACE GET NEXT SOUND FILE WITH A NEW NAMING METHOD !!!
			string soundFilename = getNextSoundFile();
    		//Write to file.
    		AuWriter::write(*renderedScore, soundFilename);
    		delete renderedScore;
  		}
	*/
    childEvents[i]->buildChildren();
    //TODO: DELETE ELSEWHERE. Cleanup should probably be done after execution.
		delete childEvents[i];
  }
  if (utilities->getOutputParticel()){
  //End this output sublevel.
    Output::addProperty("Updated Tempo Start Time", tempo.getStartTime());
    Output::endSubLevel();
  }
}

//---------------------------------------------------------------------------//

void Event::modifyChildren(){            //Incomplete

  //Randomly modify reverb and spatial elements

  //cout<<childEvents.size();

  for(unsigned i = 0; i < childEvents.size(); i++){
     childEvents[i]->modifyChildren();
  }

}

//---------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
void Event::findLeafChildren(vector<Event*> & leafChildren){
	if(childEvents.size() == 0){
		cout << "FOUND A LEAF: " << name << endl;
		leafChildren.push_back(this);
	}else{
		cout << "numchildren: " << childEvents.size()
                     << "----Non-Leaf----: " << name << endl;
	}
	for(unsigned i = 0; i < childEvents.size(); i++){
		cout << "Before a dereference in findLeafChildren in " << name << endl;
		if(childEvents[i]!=NULL)
			childEvents[i]->findLeafChildren(leafChildren);
		cout << "Finished a dereference line. " << endl;
	}
}


//-----------------------------------------------------------------------------

int Event::checkedChildType(double value) const {
  if (!std::isfinite(value) || value < 0 || value >= childTypeElements.size() ||
      value > std::numeric_limits<int>::max()) {
    throw CmodError(CmodError::Kind::Project,
                    "The child Type selects an event that is not listed in Layers.",
                    "Event '" + name + "' -> child " + to_string(currChildNum + 1) +
                    " of " + to_string(numChildren) +
                    " -> Child Event Definition -> Type: " + to_string(value),
                    "Choose a Type index from 0 to " +
                    to_string(static_cast<int>(childTypeElements.size()) - 1) +
                    " for the " + to_string(childTypeElements.size()) +
                    " child events listed in Layers, or add the missing child event.");
  }
  return static_cast<int>(value);
}

bool Event::buildContinuum() {
  string startType = XMLTC(childStartTypeFlag);
  string durType = XMLTC(childDurationTypeFlag);
  string childName;

  // Whether we should align notes to sieves
  bool align = (   sieveAligned
                && Utilities::isSieveFunction(childStartTimeElement)
                && Utilities::isSieveFunction(childDurationElement)
                && startType == "1"
                && durType == "1");

  if (currChildNum == 0) {
    checkPoint = 0;
  }
  else {
    checkPoint = Random::Rand();
  }

  // get the start time
  float rawChildStartTime = 0.0;
  float rawChildDuration = 0.0;

  if (align) {
    if (matrix == NULL) buildMatrix(false);
    MatPoint childPt = matrix->chooseContinuum();

    rawChildStartTime = static_cast<float>(childPt.stime);
    tsChild.startEDU = childPt.stime;
    tsChild.start = childPt.stime * tempo.getEDUDurationInSeconds().To<float>();

    rawChildDuration = static_cast<float>(childPt.dur);
    tsChild.durationEDU = childPt.dur;
    tsChild.duration = childPt.dur * tempo.getEDUDurationInSeconds().To<float>();
  } else {
    rawChildStartTime = static_cast<float>(utilities->evaluate(XMLTC(childStartTimeElement),(void*)this));
    // how to process start time: EDU, SECONDS or PERCENTAGE
    if (startType == "1" ) { //"EDU"
      tsChild.start = rawChildStartTime *
        tempo.getEDUDurationInSeconds().To<float>();
      tsChild.startEDU = Ratio((int)rawChildStartTime, 1);
    } else if (startType == "2") { //second
      tsChild.start = rawChildStartTime; // no conversion needed
      tsChild.startEDU = Ratio(0, 0);  // floating point is not exact: NaN
    } else if (startType == "0") { //fraction
      tsChild.start = rawChildStartTime * ts.duration; // convert to seconds
      tsChild.startEDU = Ratio(0, 0);  // floating point is not exact: NaN
    } else {
      throw CmodError(CmodError::Kind::Project,
                      "The child Start Time unit is missing or not recognized.",
                      "Event '" + name + "' -> Child Event Definition -> Start Time Unit: '" + startType + "'",
                      "Choose Fraction (0), EDU (1), or Seconds (2) for Start Time.");
    }

    // get the type
    childType = checkedChildType(utilities->evaluate(XMLTC(childTypeElement),(void*)this));
    childName = XMLTC(GFEC(childTypeElements[childType]));

    // get the duration
    rawChildDuration = static_cast<float>(utilities->evaluate(XMLTC(childDurationElement),(void*)this));

    // assign previousChild Duration here so that the next child can use it
    //  a MISNOMER, actually the ENDTIME of present child
//  previousChildDuration = rawChildDuration;
    previousChildEndTime = rawChildStartTime + rawChildDuration;

    // pre-quantize the duration in case "EDU" is used
    int rawChildDurationInt = (int)rawChildDuration;
    int maxChildDurInt = (int)maxChildDur;
    if(rawChildDurationInt > maxChildDurInt)
        rawChildDurationInt = maxChildDurInt;

    // how to process duration: EDU, SECONDS or PERCENTAGE
    if (durType == "1") {//EDU
      tsChild.durationEDU = Ratio(rawChildDurationInt, 1);
      tsChild.duration = // convert to seconds
        (float)rawChildDurationInt * tempo.getEDUDurationInSeconds().To<float>();
    } else if (durType == "2") {//seconds
      tsChild.duration = rawChildDuration;
      if(tsChild.duration > maxChildDur)
        tsChild.duration = maxChildDur; // enforce limit
      tsChild.durationEDU = Ratio(0, 0); // floating point is not exact: NaN
    } else if (durType == "0") {//fraction
      tsChild.duration = rawChildDuration * ts.duration; // convert to seconds
      if(tsChild.duration > maxChildDur)
        tsChild.duration = maxChildDur; // enforce limit
      tsChild.durationEDU = Ratio(0, 0); // floating point is not exact: NaN
    } else {
      throw CmodError(CmodError::Kind::Project,
                      "The child Duration unit is missing or not recognized.",
                      "Event '" + name + "' -> Child Event Definition -> Duration Unit: '" + durType + "'",
                      "Choose Fraction (0), EDU (1), or Seconds (2) for Duration.");
    }
  }

/*	copied from Sweep - do not use Continuum if using EDUs !!      sever7/11/22
  if(startType == "1" && durType == "1") {

    endTime = Event::verify_valid(previousChildEndTime);        //missnomer !!

//  tsChild.start = endTime *                             SEVER 5/19 2022 
    tsChild.start = rawChildStartTime *                 //SEVER 5/19 2022
        tempo.getEDUDurationInSeconds().To<float>();
    tsChild.startEDU = Ratio((int)rawChildStartTime, 1);
//cout << "FIN: tsChild.start=" << tsChild.start << " endTime=" << endTime << endl;
//cin >> sT; 

    rawChildDuration = endTime - rawChildStartTime;
    int rawChildDurationInt = (int)rawChildDuration;
    tsChild.durationEDU = Ratio(rawChildDurationInt, 1);
    tsChild.duration =                        // convert to seconds
        (float)rawChildDurationInt * tempo.getEDUDurationInSeconds().To<float>();

    previousChildEndTime = endTime;
cout << "rawChildStartTime="<< rawChildStartTime << " rawChildDuration=" 
     <<rawChildDuration << endl;
cout << "tsChild.start=" << tsChild.start << " tsChild.duration=" 
     << tsChild.duration << endl;
cout << "	endTime=" << endTime << endl;
int sever; cin >> sever;
  }

  tsPrevious.end = tsChild.start + tsChild.duration;    //same as EndTime above
  tsPrevious.endEDU = tsChild.startEDU + tsChild.durationEDU;;
*/

  // set checkpoint to the start of this child event
  checkPoint = (double)tsChild.start / ts.duration;

//cout << "EVENT::buildContinuum - childName: " << childName << endl;

  if (utilities->getOutputParticel()){
  //Output parameters in the different units available.
    Output::beginSubLevel("Continuum");
    Output::addProperty("Name", childName);
    Output::beginSubLevel("Parameters");
      Output::addProperty("Start", rawChildStartTime, unitTypeToUnits(startType));
      Output::addProperty("Duration", rawChildDuration, unitTypeToUnits(durType));
      if(unitTypeToUnits(startType) == "EDU")
        Output::addProperty("Max Duration", maxChildDur, "EDU");
      else
        Output::addProperty("Max Duration", maxChildDur, "sec.");
    Output::endSubLevel();
    Output::beginSubLevel("Seconds");
      Output::addProperty("Start", tsChild.start, "sec.");
      Output::addProperty("Duration", tsChild.duration, "sec.");
    Output::endSubLevel();
    Output::beginSubLevel("EDU");
      Output::addProperty("Start", tsChild.startEDU.toPrettyString(), "EDU");
      Output::addProperty("Duration", tsChild.durationEDU.toPrettyString(), "EDU");
    Output::endSubLevel();
    Output::addProperty("Checkpoint", checkPoint, "of parent");
    Output::endSubLevel();
  }
  return true; //success!
}


//----------------------------------------------------------------------------//
bool Event::buildSweep() { 
  string startType = XMLTC(childStartTypeFlag);
  string durType = XMLTC(childDurationTypeFlag);
  string childName;

  // Whether we should align notes to sieves
  bool align = (   sieveAligned
                && Utilities::isSieveFunction(childStartTimeElement)
                && Utilities::isSieveFunction(childDurationElement)
                && startType == "1"
                && durType == "1");

  // find start time and dur of last child
  if (currChildNum == 0) {
    tsPrevious.end = 0;			//IS THIS NECESSARY ?
    tsPrevious.endEDU = 0;
  }

  // Set checkpoint to the endpoint of the last event
  checkPoint = tsPrevious.end / ts.duration;

  if (checkPoint > 1) {
    throw CmodError(CmodError::Kind::Project,
                    "Sweep cannot place the next child because the preceding children extend beyond the parent duration.",
                    "Event '" + name + "' -> Sweep -> child " + to_string(currChildNum + 1) +
                    " of " + to_string(numChildren) + " -> previous end: " + to_string(tsPrevious.end) +
                    " seconds; parent duration: " + to_string(ts.duration) + " seconds",
                    "Reduce Number of Children to Create or the child Duration, or extend the parent duration. "
                    "Check that the Start Time and Duration units match the entered values.");
  }

  // get the start time
  float rawChildStartTime = 0.0;	//Is this necessary for DISCRETE ?
  float rawChildDuration = 0.0;
  int endTime = 0;

  if (align) {
    if (matrix == NULL) buildMatrix(false);
    MatPoint childPt = matrix->chooseSweep(numChildren - currChildNum - 1);

    rawChildStartTime = static_cast<float>(childPt.stime);
    tsChild.startEDU = childPt.stime;
    tsChild.start = childPt.stime * tempo.getEDUDurationInSeconds().To<float>();

    rawChildDuration = static_cast<float>(childPt.dur);
    tsChild.durationEDU = childPt.dur;
    tsChild.duration = childPt.dur * tempo.getEDUDurationInSeconds().To<float>();
  } else {
    // get the start time
//  rawChildStartTime =
//    utilities->evaluate(XMLTC(childStartTimeElement),(void*)this);

    rawChildStartTime = static_cast<float>(previousChildEndTime);			//actually endTime
//cout << "Event::buildSweep - rawChildStartTime=" << rawChildStartTime << endl;

    // The child starts where the previous child ends, whatever units its
    // times are given in. In EDU that is the previous child's exact end. In
    // Seconds or as a Fraction it is the sum of the entered durations,
    // converted to seconds exactly as CMOD has always computed it, so the
    // sound of existing projects does not move by even one sample. When the
    // start and the durations use different units, their sum means nothing,
    // so the child starts at the previous child's end in seconds.
    if (startType == "1" && tsPrevious.endEDU.isDeterminate()) {	//EDU
      tsChild.startEDU = tsPrevious.endEDU;
      tsChild.start = tsChild.startEDU.To<float>() *
        tempo.getEDUDurationInSeconds().To<float>();
    } else if (startType == durType && (startType == "2" || startType == "0")) {
      if (startType == "2") {					//seconds
        tsChild.start = rawChildStartTime; 	// no conversion needed
      } else {							//fraction
        tsChild.start = rawChildStartTime * ts.duration; 	// convert to seconds
      }
      if (tsChild.start < tsPrevious.end) // Prevent events from overlapping
        tsChild.start = tsPrevious.end;
      tsChild.startEDU = Ratio(0, 0); // floating point is not exact: NaN
    } else {		//start and durations in different units
      tsChild.start = tsPrevious.end;
      tsChild.startEDU = Ratio(0, 0); // floating point is not exact: NaN
    }

  // get the type
  childType = checkedChildType(utilities->evaluate(XMLTC(childTypeElement),(void*)this));
  childName = XMLTC(GFEC(childTypeElements[childType]));

    // get the duration
    rawChildDuration = static_cast<float>(utilities->evaluate(XMLTC(childDurationElement),(void*)this));

    //assign previousChild Duration here so that the next child can use it
   // this is a MISNOMER actually the endTime of the present child
    previousChildEndTime = rawChildStartTime + rawChildDuration;
/*
    cout << "	previousChildEndTime=" << previousChildEndTime 
	<< " rawChildDuration=" << rawChildDuration << endl; 	
*/
    // pre-quantize the duration in case "EDU" is used
    int rawChildDurationInt = (int)rawChildDuration;
    int maxChildDurInt = (int)maxChildDur;

    if(rawChildDurationInt > maxChildDurInt)
        rawChildDurationInt = maxChildDurInt;		//enforce limit

    if (durType == "1") {					//EDU
      tsChild.durationEDU = Ratio(rawChildDurationInt, 1);
      tsChild.duration = 			// convert to seconds
        (float)rawChildDurationInt * tempo.getEDUDurationInSeconds().To<float>();
    } else if (durType == "2") {				//seconds
      tsChild.duration = rawChildDuration;
      if(tsChild.duration > maxChildDur)
        tsChild.duration = maxChildDur; // enforce limit
      tsChild.durationEDU = Ratio(0, 0); // floating point is not exact: NaN
    } else if (durType == "0") {				//fraction
      tsChild.duration = rawChildDuration * ts.duration; // convert to seconds
      if(tsChild.duration > maxChildDur)
        tsChild.duration = maxChildDur; // enforce limit
      tsChild.durationEDU = Ratio(0, 0); // floating point is not exact: NaN
    }
  }

  if(startType == "1" && durType == "1") {
    endTime = Event::verify_valid(static_cast<int>(previousChildEndTime));		//missnomer !

    tsChild.start = rawChildStartTime *			       	    //SEVER 5/19 2022
        tempo.getEDUDurationInSeconds().To<float>();
    tsChild.startEDU = Ratio((int)rawChildStartTime, 1);

    rawChildDuration = endTime - rawChildStartTime;
    int rawChildDurationInt = (int)rawChildDuration;
    tsChild.durationEDU = Ratio(rawChildDurationInt, 1);
    tsChild.duration =                        		// convert to seconds
        (float)rawChildDurationInt * tempo.getEDUDurationInSeconds().To<float>();

    previousChildEndTime = endTime;
  }

  tsPrevious.end = tsChild.start + tsChild.duration;	//same as EndTime above
  tsPrevious.endEDU = tsChild.startEDU + tsChild.durationEDU;

  // Max Child Duration shortens the sound of a child timed in Seconds or as a
  // Fraction, not its place in the Sweep: the next child still moves by the
  // entered duration, as it always has, and the gap becomes a rest. (When the
  // start uses the same unit, the start above already sums entered durations.)
  if ((durType == "2" || durType == "0") && startType != durType) {
    float enteredDuration = rawChildDuration;			//seconds
    if (durType == "0") enteredDuration *= ts.duration;	//fraction
    if (enteredDuration > tsChild.duration)
      tsPrevious.end = tsChild.start + enteredDuration;
  }
/*
 cout << "   " << endl;
 cout << "Event:buildSweep - rawChildStartTime=" << rawChildStartTime << endl;
 cout << "                   tsChild.start=" << tsChild.start << endl;
 cout << "                   rawChildDuration=" << rawChildDuration << endl;
 cout << "                   tsChild.duration=" << tsChild.duration << endl;
 cout << "                 - endTime=" << endTime << endl;
 cout << "                 - tsPrevious.end=" << tsPrevious.end << endl;
 cout << "                 - tsPrevious.endEDU=" << tsPrevious.endEDU << endl;
 cout << "                 - previousChildEndTime=" << previousChildEndTime << endl;
     int sever; cin >> sever;
*/

  // set checkpoint to the start of this child event
  checkPoint = tsChild.start / ts.duration;

  if (checkPoint > 1) {
    throw CmodError(CmodError::Kind::Project,
                    "A Sweep child starts after the parent event has ended.",
                    "Event '" + name + "' -> Sweep -> child " + to_string(currChildNum + 1) +
                    " of " + to_string(numChildren) + " -> start: " + to_string(tsChild.start) +
                    " seconds; parent duration: " + to_string(ts.duration) + " seconds",
                    "Reduce Number of Children to Create or the child Duration, or extend the parent duration. "
                    "Check the Start Time and Duration units.");
  }

  if (utilities->getOutputParticel()){
    //Output parameters in the different units available.
    Output::beginSubLevel("Sweep");
    Output::addProperty("Name", childName);
    Output::beginSubLevel("Parameters");
      Output::addProperty("Start", rawChildStartTime, unitTypeToUnits(startType));
      Output::addProperty("Duration", rawChildDuration, unitTypeToUnits(durType));
      if(unitTypeToUnits(startType) == "EDU")
        Output::addProperty("Max Duration", maxChildDur, "EDU");
      else
        Output::addProperty("Max Duration", maxChildDur, "sec.");
    Output::endSubLevel();
    Output::beginSubLevel("Seconds");
      Output::addProperty("Start", tsChild.start, "sec.");
      Output::addProperty("Duration", tsChild.duration, "sec.");
      Output::addProperty("Previous", tsPrevious.end, "sec.");
    Output::endSubLevel();
    Output::beginSubLevel("EDU");
      Output::addProperty("Start", tsChild.startEDU, "EDU");
      Output::addProperty("Duration", tsChild.durationEDU, "EDU");
      Output::addProperty("Previous", tsPrevious.endEDU, "EDU");
    Output::endSubLevel();
    Output::addProperty("Checkpoint", checkPoint, "of parent");
    Output::endSubLevel();
  }
  return true; // success!
}


//----------------------------------------------------------------------------//

void Event::addTemporaryXMLDocument(std::unique_ptr<pugi::xml_document> _doc){
  temporaryXMLDocuments.push_back(std::move(_doc));
}

PatternPair::~PatternPair(){
  if (pattern)delete pattern;
}

void Event::addPattern(std::string _string, Patter* _pat){
  PatternPair* n = new PatternPair(_string, _pat);
  patternStorage.push_back(n);
}


//---------------------------------------------------------------------------//

Event::~Event() {
  // temporaryXMLDocuments and modifiersDoc are unique_ptrs that auto-clean up.

  for (auto* p : patternStorage)
    delete p;

  if (matrix != NULL) delete matrix;
}


//----------------------------------------------------------------------------//
//Checked

void Event::tryToRestart(void) {

  // Retry random placements, but never change the requested child count.
  if(restartsRemaining > 0) {
    restartsRemaining--;
    cout << "Retrying Discrete generation for event '" << name
         << "': cannot place child " << currChildNum + 1 << " of " << numChildren
         << ". " << restartsRemaining << " retries remain." << endl;
  } else {
    throw CmodError(CmodError::Kind::Project,
                    "Discrete generation could not place all requested children after retrying.",
                    "Event '" + name + "' -> child " + to_string(currChildNum + 1) +
                    " of " + to_string(numChildren) + " -> parent duration: " +
                    to_string(ts.duration) + " seconds",
                    "Reduce Number of Children to Create, shorten the Duration Sieve values, "
                    "or provide more allowed Attack Sieve positions within the parent duration. "
                    "Check that the layer weights and probability envelopes allow these placements.");
  }

  //Start over by clearing the event arrays and resetting the for-loop index.
	// NOTE: SHOULD BE -1
  currChildNum = -1;
  for (unsigned i = 0; i < temporaryChildEvents.size(); i++)
    delete temporaryChildEvents[i];
  temporaryChildEvents.clear();

  //Clear the temporary event list.
  for (auto* child : childSoundsAndNotes) delete child;
  childSoundsAndNotes.clear();

  patternStorage.clear();
}


//----------------------------------------------------------------------------//
//Checked

void Event::checkEvent(bool buildResult) {
  //If the build failed, restart if necessary.
  if (!buildResult) {
    tryToRestart();
    return;
  }

  /*Up to now the child start time is an *offset*, that is, it has no context
  yet within the piece. The following section uses the start time/tempo rules to
  determine the correct exact and inexact start times, in some cases leading to
  a new tempo.*/

  /*Inexact start time is global. That is, it *always* refers to the position in
  time relative to the beginning of the piece. Thus the child start time is
  merely the child offset added of the parent start time.*/
  tsChild.start += ts.start;

  /*The next part of the code deals with exactness issues since inexact and
  exact events may be nested inside each other.*/

  /*The following graphic attempts to show the many possibilities of nested
  exact and inexact offsets.

  i = inexact start offset
  e = exact start offset
  T = tempo start time

  0=============================================================================
  |            EVENT 1-----------------------------------------------------. . .
  |            |           EVENT 2-----------------------------------------. . .
  |            |           |          EVENT 3------------------------------. . .
  |            |           |          |         EVENT 4--------------------. . .
  |            |           |          |         |         EVENT 5----------. . .
  |            |           |          |         |         |       EVENT 6--. . .
  |            |           |          |         |         |       |
  |      +     i1    +     e2    +    e3   +    i4   +    i5  +   e6
  |            |           |          |         |         |       |
  .            .           .          .         .         .       .
  .            .           .          .         .         .       .
  .            .           .          .         .         .       .
  |    TEMPO   |   START   |  TIMES   |         |         |       |
               \\                                         \\
               T1         (T1)       (T1)                 T5     (T5)

  Inexact Start Times (~ means the exact value is truncated to floating point):
  Event 1 = i1
  Event 2 = i1 + ~e2
  Event 3 = i1 + ~e2 + ~e3
  Event 4 = i1 + ~e2 + ~e3 + i4
  Event 5 = i1 + ~e2 + ~e3 + i4 + i5
  Event 6 = i1 + ~e2 + ~e3 + i4 + i5 + ~e6

  Exact Start Times:
  Event 1 = (not applicable)
  Event 2 = T1 + e2
  Event 3 = T1 + (e2 + e3)
  Event 4 = (not applicable)
  Event 5 = (not applicable)
  Event 6 = T5 + e6

  Note that Event 4 ignores tempo information altogether since its child is
  inexact.

  Possible combinations:
  1) Parent inexact, child inexact (Events 4-5)
  Since both are inexact, nothing further is to be done. They will both only
  have global inexact time offsets.*/
  if(!ts.startEDU.isDeterminate() && !tsChild.startEDU.isDeterminate()) {
    tsChild.startEDUAbsolute = ts.startEDUAbsolute + tempo.convertSecondsToEDUs(tsChild.start);
  }

  /*2) Parent exact, child inexact (Events 3-4)
  Since the child is inexact, nothing further is to be done. The child will
  simply have a global inexact time offset. The parent will already have
  calculated its tempo start time.*/
  if(ts.startEDU.isDeterminate() && !tsChild.startEDU.isDeterminate()) {
    tsChild.startEDUAbsolute = ts.startEDUAbsolute + tempo.convertSecondsToEDUs(tsChild.start);
  }

  /*3) Parent exact, child exact (Events 2-3)
  Since the both are exact, the child inherits the tempo of the parent. Its
  exact offset is calculated by adding the exact parent start time offset.

  Important Note:
  If the child attempts to override the parent tempo, it will be ignored and the
  above calculation. This is to prevent implicitly nested tempos, which are
  better handled explicitly at the moment. For example it would be very
  difficult to properly render "4/4 for 3 1/4 beats, then change to 5/8 for
  3 beats as a child tempo." If this nesting were allowed, it would be very
  ambiguous as to how to return back to 4/4. Even if the 5/8 were to trigger a
  new tempo start time, in the score this would be misleading making it appear
  that the two sections were not rhythmically related, even though they
  inherently are by virtue of them both being exact.*/
  if(ts.startEDU.isDeterminate() && tsChild.startEDU.isDeterminate()) {
    tsChild.startEDUAbsolute = ts.startEDUAbsolute + tsChild.startEDU.To<int>();
    tsChild.startEDU += ts.startEDU;
    /*We need to force child to have the same tempo, so that weird things do not
    happen. This is done below by explictly setting the tempo of the child. This
    will in turn be honored by initDiscreteInfo which will not override the
    given parent tempo. Note in order for this to be done, the tempo is passed
    to buildChildEvents, to constructChild, to EventFactory::Build, and finally
    to initDiscreteInfo.*/
  }

  /*4) Parent inexact, child exact (Events 1-2, 5-6)
  In this case, since the parent did not have an exact offset from the
  grandparent, the exact child needs a new reference point. This triggers the
  creation of a new tempo start time *for the parent*. Since the child is
  offset an exact amount from the parent, the parent is the new tempo reference.

  This could easily be the source of confusion: when a parent offset is inexact,
  and a child offset is exact, it is the parent which takes on the new tempo.
  Note that this implies that the child's siblings will refer to the same new
  tempo start time.*/
  if(!ts.startEDU.isDeterminate() && tsChild.startEDU.isDeterminate()) {
    /*The offset is the new start time, so nothing needs to be done to
    tsChild.startEDU. Instead we need to trigger a new tempo start for the
    parent. If this is the second exact child of a parent, then it will merely
    set the start time to the same thing.*/
    tempo.setStartTime(ts.start);
    tsChild.startEDUAbsolute = ts.startEDUAbsolute + tsChild.startEDU.To<int>();
    //We need to force child to have the same tempo. See statement for 3).
  }

  //Make sure the childType indexes correctly.
  if (childType < 0 || childType >= (int)childTypeElements.size() ) {
    throw CmodError(CmodError::Kind::Internal,
                    "Child generation returned an invalid Type index.",
                    "Event '" + name + "' -> child " + to_string(currChildNum + 1) +
                    " -> Type: " + to_string(childType),
                    "Report this error with the project file and the complete diagnostic.");
  }

  //Create new event.
  pugi::xml_node discretePackage = childTypeElements[childType];
  EventType childEventType = (EventType) utilities->evaluate(XMLTC(GNES(GFEC(discretePackage))),(void*)this);

  string childEventName = XMLTC(GFEC(discretePackage));
  pugi::xml_node childElement = utilities->getEventElement(childEventType, childEventName);

  Event* e = NULL;
  if (childEventType == eventBottom){
    e = (Event*) new Bottom(childElement, tsChild, childType, tempo, utilities, spatializationElement, 
                            reverberationElement, filterElement, modifiersIncludingAncestorsElement);
    if(tsChild.startEDU.isDeterminate()){
      e->tempo = tempo;
    }
  } else if (childEventType == eventSound || childEventType == eventNote){
    childSoundsAndNotes.push_back(new SoundAndNoteWrapper
		  (childElement, tsChild, childEventName, childType, tempo));
  } else {
    e = new Event( childElement, tsChild, childType, tempo, utilities,
                  spatializationElement, reverberationElement, filterElement,
		    modifiersIncludingAncestorsElement);
    if(tsChild.startEDU.isDeterminate()){
      e->tempo = tempo;
    }
  }

  if (e != NULL) temporaryChildEvents.push_back(e);
}


//----------------------------------------------------------------------------//
//Checked

void Event::outputProperties() {
  Output::addProperty("Type", type);
  Output::addProperty("Start Time", ts.start, "sec.");
  Output::addProperty("Start Absolute", tsChild.startEDUAbsolute, "EDU");
  Output::addProperty("Duration", ts.duration, "sec.");
  Output::addProperty("Tempo Start Time", tempo.getStartTime());
  Output::addProperty("Tempo",
    tempo.getTempoBeatsPerMinute().toPrettyString(), "BPM");
  Output::addProperty("Tempo Beat", tempo.getTempoBeat().toPrettyString(), "of whole note");
  Output::addProperty("Time Signature", tempo.getTimeSignature());
  Output::addProperty("Divisions",
    tempo.getEDUPerTimeSignatureBeat().toPrettyString(), "EDU/beat");
  Output::addProperty("Available EDU", getAvailableEDU());
  Output::addProperty("Available EDU is Exact", getEDUDurationExactness());
  Output::addProperty("EDU Duration", tempo.getEDUDurationInSeconds().toPrettyString(), "sec.");
}


//----------------------------------------------------------------------------//
//Checked

list<Note> Event::getNotes() {
  list<Note> result;
  for (unsigned i = 0; i < childEvents.size(); i++) {
    list<Note> append = childEvents[i]->getNotes();
    list<Note>::iterator iter = append.begin();
    while (iter != append.end()) {
      result.push_back(*iter);
      iter++;
    }
  }
  return result;
}


//----------------------------------------------------------------------------//
//Checked

int Event::getCurrentLayer() {
  int countInLayer = 0;
  for(unsigned i = 0; i < layerVect.size(); i++) {
    countInLayer = static_cast<int>(countInLayer + layerVect[i].size());
    if(childType >= 0 && childType < countInLayer)
      return i;
  }
  throw CmodError(CmodError::Kind::Project,
                  "CURRENT_LAYER has no available child event to look up.",
                  "Event '" + name + "' -> CURRENT_LAYER -> Type: " + to_string(childType),
                  "Add child events to Layers and use CURRENT_LAYER in a child-generation expression "
                  "after the child Type is available.");
}


//----------------------------------------------------------------------------//
//Checked

int Event::getAvailableEDU()
{
  //Return exact duration if it is already apparent.
  if(ts.startEDU.isDeterminate() && ts.startEDU != Ratio(0, 1))
    return ts.startEDU.To<int>();

  //The duration is not exact.
  int myDurationInt = (int)ts.duration;
  Ratio EDUs;
  float durationScalar;

  if(ts.duration == (float)myDurationInt)
  {
    //Since duration is an integer, it may still be possible to have exact EDUs.
    EDUs = tempo.getEDUPerSecond() * Ratio(myDurationInt, 1);
    if(EDUs.Den() == 0)//This shouldn't happen.
      return 0;
    else if(EDUs.Den() != 0 && EDUs.Den() != 1) //We have exact EDUs
      return EDUs.To<int>();
    else //Implied EDUs.Den() == 1
      durationScalar = 1;
  }
  else
  {
    EDUs = tempo.getEDUPerSecond();
    if(EDUs.Den() == 0)
      return 0; //This shouldn't happen.
    else //Implied EDUs.Den() != 0
      durationScalar = ts.duration;
  }

  //The duration is not exact, so the available EDUs must be quantized.
  float approximateEDUs = EDUs.To<float>() * durationScalar;
  int quantizedEDUs = (int)(approximateEDUs + 0.001f);
  if(abs((float)quantizedEDUs - approximateEDUs) > 0.001f) {
    cout << "WARNING: quantizing AVAILABLE_EDU from ";
    cout << approximateEDUs << " to " << quantizedEDUs << endl;
  }
  return quantizedEDUs;
};


//----------------------------------------------------------------------------//
//Checked

string Event::getEDUDurationExactness(void) {
  float actualEDUDuration =
    (Ratio(getAvailableEDU(), 1) * tempo.getEDUDurationInSeconds()).To<float>();

  if(actualEDUDuration == ts.duration)
    return "Yes";
  else if(fabs(actualEDUDuration / ts.duration - 1.0f) < 0.01f)
    return "Almost";
  else
    return "No";
}


//----------------------------------------------------------------------------//
//Checked

string Event::unitTypeToUnits(string unitType) {
  if(unitType == "UNITS" || unitType == "EDU")
    return "EDU";
  else if(unitType == "SECONDS")
    return "sec.";
  else if(unitType == "PERCENTAGE")
    return "normalized";
  else
    return "";
}


//----------------------------------------------------------------------------//

bool Event::buildDiscrete() {

  if (currChildNum == 0) {
    checkPoint = 0;
  }
  MatPoint childPt;

  if (matrix == NULL) buildMatrix(true);

  // get something out of the matrix
  childPt  = matrix->chooseDiscrete(numChildren - currChildNum - 1);

  // check to see if we ran out of space in matrix --- (type=-1 is a flag)
  if (childPt.type == -1) {
    delete matrix; // delete the matrix so it gets recreated on a retry
    matrix = NULL;
    return false; // failure!
  }

  int stimeEDU = childPt.stime;
  int durEDU = childPt.dur;
  childType = childPt.type;
  string childName = XMLTC(GFEC(childTypeElements[childType]));

  if(durEDU > (int)maxChildDur)
    durEDU = static_cast<int>(maxChildDur);
  tsChild.startEDU = stimeEDU;
  tsChild.durationEDU = durEDU;

  tsChild.start = (float)stimeEDU *
    tempo.getEDUDurationInSeconds().To<float>();
  tsChild.duration = (float)durEDU *
    tempo.getEDUDurationInSeconds().To<float>();

  // using edu
  checkPoint = (double)tsChild.start / ts.duration;

  if (utilities->getOutputParticel()){
  //Output parameters in the different units available.
    Output::beginSubLevel("Discrete");
    Output::addProperty("Name", childName);
    Output::beginSubLevel("Parameters");
      Output::addProperty("Max Duration", maxChildDur, "EDU");
    Output::endSubLevel();
    Output::beginSubLevel("Seconds");
      Output::addProperty("Start", tsChild.start, "sec.");
      Output::addProperty("Duration", tsChild.duration, "sec.");
    Output::endSubLevel();
    Output::beginSubLevel("EDU");
      Output::addProperty("Start", tsChild.startEDU, "EDU");
      Output::addProperty("Duration", tsChild.durationEDU, "EDU");
    Output::endSubLevel();
    Output::endSubLevel();
  }

  return true; // success!
}


//---------------------------------------------------------------------------//

void Event::buildMatrix(bool discrete) {
  // first time called --- create the matrix!
  Sieve* attackSiv;
  Sieve* durSiv;
  vector<double> typeProbs;
  vector<Envelope*> attackEnvs;
  vector<Envelope*> durEnvs;
  vector<int> numTypesInLayers;

  const auto readSieve = [this, discrete](pugi::xml_node element, const string& field) {
    try {
      return discrete
        ? static_cast<Sieve*>(utilities->evaluateObject(XMLTC(element), this, eventSiv))
        : utilities->evaluateSieve(XMLTC(element), this);
    } catch (CmodError& error) {
      error.addContext("Event '" + name + "' -> Child Event Definition -> " + field);
      throw;
    }
  };
  attackSiv = readSieve(discrete ? AttackSieveElement : childStartTimeElement, "Attack Sieve");
  durSiv = readSieve(discrete ? DurationSieveElement : childDurationElement, "Duration Sieve");

  if (attackSiv == NULL || attackSiv->GetNumItems() == 0 ||
      durSiv == NULL || durSiv->GetNumItems() == 0) {
    const string field = attackSiv == NULL || attackSiv->GetNumItems() == 0
                         ? "Attack Sieve" : "Duration Sieve";
    delete attackSiv;
    delete durSiv;
    throw CmodError(CmodError::Kind::Project,
                    "The " + field + " contains no usable values.",
                    "Event '" + name + "' -> Child Event Definition -> " + field,
                    "Check the sieve's Low/High limits, Elements, and Offset so at least one value remains.");
  }

  double weightSum = 0;
  for (unsigned i = 0; i < childTypeElements.size(); i ++){
    double prob = utilities->evaluate(XMLTC(GNES(GNES(GFEC(childTypeElements[i])))), (void*) this);
    if (!std::isfinite(prob) || prob < 0) {
      delete attackSiv;
      delete durSiv;
      throw CmodError(CmodError::Kind::Project,
                      "A child event's probability Weight is negative or non-finite.",
                      "Event '" + name + "' -> Layers -> child Type " + to_string(i) +
                      " -> Weight: " + to_string(prob),
                      "Use finite, nonnegative weights, with at least one positive child weight.");
    }
    typeProbs.push_back(prob);
    weightSum += prob;
  }
  if (!std::isfinite(weightSum) || weightSum <= 0) {
    delete attackSiv;
    delete durSiv;
    throw CmodError(CmodError::Kind::Project,
                    "The child event probability weights have no positive finite total.",
                    "Event '" + name + "' -> Layers -> Weight total: " + to_string(weightSum),
                    "Give at least one child event a positive Weight and keep all weights finite and nonnegative.");
  }
  for (unsigned i = 0; i < typeProbs.size(); i ++){
    typeProbs[i] = typeProbs[i] / weightSum;

  }

  if (discrete) {
    for (unsigned i = 0; i < childTypeElements.size(); i ++){
      // attack env
      pugi::xml_node elementIter = GNES(GNES(GNES(GFEC(childTypeElements[i]))));

      string attackEnvString = XMLTC(elementIter);
      elementIter = GNES(elementIter);
      string attackEnvScaleString = XMLTC(elementIter);

      string attackFunctionString =
              "<Fun><Name>EnvLib</Name><Env>" +
              attackEnvString +
              "</Env><Scale>" +
              attackEnvScaleString +
              "</Scale></Fun>";

      elementIter = GNES(elementIter);
      string durationEnvString = XMLTC(elementIter);

      elementIter = GNES(elementIter);
      string durationEnvScaleString = XMLTC(elementIter);

      string durationFunctionString =
              "<Fun><Name>EnvLib</Name><Env>" +
              durationEnvString +
              "</Env><Scale>" +
              durationEnvScaleString +
              "</Scale></Fun>";

      attackEnvs.push_back((Envelope*)
        utilities->evaluateObject(attackFunctionString, this, eventEnv));

      durEnvs.push_back((Envelope*)
        utilities->evaluateObject(durationFunctionString, this, eventEnv));
    }
  }
  for (unsigned i = 0; i < layerElements.size(); i ++){
    int numOfDiscretePackages = 0;
    pugi::xml_node elementIter = GFEC(GNES(GFEC(layerElements[i])));
    while (elementIter!= NULL){
      numOfDiscretePackages++;
      elementIter = GNES(elementIter);
    }
    numTypesInLayers.push_back (numOfDiscretePackages);
  }

  int parentEDUs = static_cast<int>(Note::str_to_int(tempo.getEDUPerSecond().toPrettyString()) * ts.duration);

  matrix = new Matrix(static_cast<int>(childTypeElements.size()), attackSiv->GetNumItems(),
       durSiv->GetNumItems(),  numTypesInLayers, parentEDUs, tempo, sieveAligned);

  if (discrete) {
    matrix->setAttacks(attackSiv, attackEnvs);
    matrix->setDurations(durSiv, parentEDUs, durEnvs);
  } else {
    matrix->setAttacks(attackSiv);
    matrix->setDurations(durSiv, parentEDUs);
  }
  matrix->setTypeProbs(typeProbs);
//matrix->printMatrix(false);

  delete attackSiv;
  delete durSiv;

  for (unsigned i = 0 ; i < attackEnvs.size(); i++){
    delete attackEnvs[i];
  }
  attackEnvs.clear();
  for (unsigned i = 0 ; i< durEnvs.size(); i++){
    delete durEnvs[i];
  }
  durEnvs.clear();


}

//----------------------------------------------------------------------------//

int Event::verify_valid(int endTime){
  // Numeric EDU timing is already exact; sieve alignment is optional.
  if (!Utilities::isSieveFunction(childStartTimeElement)) return endTime;
  
  int beatEDUs = tempo.getEDUPerTimeSignatureBeat().Num();
//cout << "     beatEDUs=" << beatEDUs << endl;

  if (sieveSweep == NULL){
     sieveSweep = utilities->evaluateSieve(XMLTC(childStartTimeElement), (void*) this);
     if (sieveSweep == NULL || sieveSweep->GetNumItems() == 0) {
       throw CmodError(CmodError::Kind::Project,
                       "The Sweep Start Time sieve contains no usable values.",
                       "Event '" + name + "' -> Sweep -> Child Event Definition -> Start Time",
                       "Check the sieve's Low/High limits, Elements, and Offset so at least one value remains.");
     }
     vector<double> attProbs;
     vector<int> attTimes;
     sieveSweep->FillInVectors(attTimes, attProbs);
     attackSweep.clear();

     for (unsigned i = 0; i < attTimes.size(); i++){
     	if (beatEDUs <= attTimes[i]) break;
//cout << "Event::verify_valid - attTimes[" << i << "]: "<< attTimes[i] << " ";
	    attackSweep.push_back(attTimes[i]);
     }
//cout << " " << endl;
  }

  int length = static_cast<int>(attackSweep.size());
  if (length == 0) {
    return endTime;
  }
  int low = 0;
  int high = length - 1;
  int eTime = endTime % beatEDUs;
//cout << "	length=" << length << " beatEDUs=" << beatEDUs << endl;

  while(high > low+1){
    int mid = (high+low) / 2;
    if(attackSweep[mid] < eTime){
      low = mid;
    } else if(attackSweep[mid] > eTime) {
      high = mid;
    } else {
      return endTime;
    }
  }
//cout << "endTime: " << endTime<< " low=" << low << " high=" << high 
//     << " eTime=" << eTime << endl;
  int offset = attackSweep[high] - eTime <= eTime - attackSweep[low] ? attackSweep[high] - eTime : attackSweep[low] - eTime;
//cout << "endTime: " << endTime << " choose: " << endTime + offset << endl;
  return endTime + offset;
}
