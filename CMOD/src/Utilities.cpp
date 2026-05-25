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

//----------------------------------------------------------------------------//
//
//  Utilities.h
//  Created by Ming-ching Chiu 2012-2013
//
//  The Utilities object is designed to evaluate the XML strings to their
//  proper format, whether an Event, a number, an Envelope or other objects such
//  as Pattern, Sieve, etc.
//
//  The Utilities is also the interface between the CMOD Event and LASS Score.
//  CMOD Events add the Sound/Note objects they produce to LASS Score through
//  The Utilities.
//
//  Maintained by Fanbo Xiang 2018
//----------------------------------------------------------------------------//
#include "Utilities.h"
#include "Random.h"
#include "Event.h"
#include "Piece.h"
#include "Patter.h"
#include "ProbabilityEnvelope.h" // consider moving this into LASS.h
#include <memory>
#include <sstream>
#include <string>

Utilities::Utilities(pugi::xml_node root,
                     string _workingPath,
                     bool _soundSynthesis,
                     bool _outputParticel,
                     int _numThreads,
                     int _numChannels,
                     int _samplingRate,
                     Piece* _piece):
  soundSynthesis(_soundSynthesis),
  outputParticel(_outputParticel),
  numThreads(_numThreads),
  numChannels(_numChannels),
  samplingRate(_samplingRate),
  piece(_piece){

  // New LASS Score
  if (soundSynthesis){
    score = new Score (numThreads,  numChannels, _samplingRate );
    score->setClippingManagementMode(Score::CHANNEL_ANTICLIP);
  }
  else {
    score = NULL;
  }


  // Construct Envelope library
  pugi::xml_node envelopeLibraryElement = GNES(GNES(GFEC(root)));
  string envLibContent = XMLTranscode(envelopeLibraryElement);
  string fileString = "lib.temp";
  {
    std::ofstream file(fileString);
    file << envLibContent;
  }

  envelopeLibrary = new EnvelopeLibrary();
  envelopeLibrary->loadLibraryNewFormat((char*)fileString.c_str());
  std::error_code rmEc;
  std::filesystem::remove(fileString, rmEc);

  // Construct Markov Model Library
  pugi::xml_node markovModelLibraryElement = GNES(envelopeLibraryElement);
  string tagName = markovModelLibraryElement.name();
  if (tagName != "MarkovModelLibrary") {
    cout << "Project is outdated, please save the project in the latest version of DISSCO" << endl;
    exit(1);
  }
  string data = XMLTC(markovModelLibraryElement);
  std::stringstream ss(data);
  int size;
  ss >> size;
  markovModelLibrary.resize(size);
  string modelText, line;
  getline(ss, line, '\n');
  for (int i = 0; i < size; i++) {
    getline(ss, line, '\n');
    modelText = line + '\n';
    getline(ss, line, '\n');
    modelText += line + '\n';
    getline(ss, line, '\n');
    modelText += line + '\n';
    getline(ss, line, '\n');
    modelText += line;
    markovModelLibrary[i].from_str(modelText);
    markovModelLibrary[i].normalize();
  }


  //events and other objects

  pugi::xml_node eventElements = GNES(markovModelLibraryElement);
  pugi::xml_node thisEventElement = GFEC(eventElements);
  //Counters to assign numbers to the events. Experimental

   int topCounter = 0;
   int highCounter = 0;
   int midCounter = 0;
   int lowCounter = 0;
   int bottomCounter = 0;
   int spectrumCounter = 0;
   int envelopeCounter = 0;
   int sieveCounter = 0;
   int spatializationCounter = 0;
   int patternCounter = 0;
   int reverbCounter = 0;
   int filterCounter = 0;
   int notesCounter = 0;

   //put the pointer of events and objects into the proper map
   while(thisEventElement){
     int type = atoi(XMLTranscode(GFEC(thisEventElement)).c_str());
     string eventName=  XMLTranscode(GNES(GFEC(thisEventElement)));

     switch (type){
       case 0:
         topEventElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

         topEventnames.insert(
               pair<int, string>(topCounter, eventName));

         eventValues.insert(
               pair<string, double>(eventName, 0.0));

         break;
       case 1:
         highEventElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

         highEventnames.insert(
               pair<int, string>(highCounter, eventName));

         eventValues.insert(
               pair<string, double>(eventName, 0.0));

         break;
       case 2:
         midEventElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               midEventnames.insert(
                     pair<int, string>(midCounter, eventName));

                     eventValues.insert(
                           pair<string, double>(eventName, 0.0));

         break;
       case 3:
         lowEventElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               lowEventnames.insert(
                     pair<int, string>(lowCounter, eventName));

                     eventValues.insert(
                           pair<string, double>(eventName, 0.0));

         break;
       case 4:
         bottomEventElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               bottomEventnames.insert(
                     pair<int, string>(bottomCounter, eventName));

                     eventValues.insert(
                           pair<string, double>(eventName, 0.0));

         break;
       case 5:
         spectrumElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               spectrumEventnames.insert(
                     pair<int, string>(spectrumCounter, eventName));

                     eventValues.insert(
                           pair<string, double>(eventName, 0.0));

         break;
       case 6:
         envelopeElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               envelopeEventnames.insert(
                     pair<int, string>(envelopeCounter, eventName));
         break;
       case 7:
         sieveElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               sieveEventnames.insert(
                     pair<int, string>(sieveCounter, eventName));
         break;
       case 8:
         spatializationElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               spatializationEventnames.insert(
                     pair<int, string>(spatializationCounter, eventName));
         break;
       case 9:
         patternElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               patternEventnames.insert(
                     pair<int, string>(patternCounter, eventName));
         break;
       case 10:
         reverbElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               reverbEventnames.insert(
                     pair<int, string>(reverbCounter, eventName));
         break;
       case 12:
         notesElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               notesEventnames.insert(
                     pair<int, string>(notesCounter, eventName));
         break;
       case 13:
         filterElements.insert(
               pair<string, pugi::xml_node>(eventName, thisEventElement));

               filterEventnames.insert(
                     pair<int, string>(filterCounter, eventName));
         break;
     }
     thisEventElement = GNES(thisEventElement);
   }
 }



//----------------------------------------------------------------------------//

Utilities::~Utilities(){
  if (score != NULL){
    delete score;
  }
  delete envelopeLibrary;
}


//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getEventElement(EventType _type, string _eventName){
  map<string, pugi::xml_node>::iterator it;

  switch((int)_type){
    case 0:
      it = topEventElements.find(_eventName);
      break;
    case 1:
      it = highEventElements.find(_eventName);
      break;
    case 2:
      it = midEventElements.find(_eventName);
      break;
    case 3:
      it = lowEventElements.find(_eventName);
      break;
    case 4:
      it = bottomEventElements.find(_eventName);
      break;
    case 5:
      it = spectrumElements.find(_eventName);
      break;
    case 6:
      it = envelopeElements.find(_eventName);
      break;
    case 7:
      it = sieveElements.find(_eventName);
      break;
    case 8:
      it = spatializationElements.find(_eventName);
      break;
    case 9:
      it = patternElements.find(_eventName);
      break;
    case 10:
      it = reverbElements.find(_eventName);
      break;
    case 12:
      it = notesElements.find(_eventName);
      break;
    case 13:
      it = filterElements.find(_eventName);
      break;
  }
  return it->second;
}


//----------------------------------------------------------------------------//

string Utilities::XMLTranscode(pugi::xml_node _thisFunctionElement){

  // handle empty function element and empty string
  if (!_thisFunctionElement || !_thisFunctionElement.first_child()) {
    return "";
  }

  // If the element has only text children, return that text directly.
  bool hasElementChild = false;
  for (auto c = _thisFunctionElement.first_child(); c; c = c.next_sibling()) {
    if (c.type() == pugi::node_element) { hasElementChild = true; break; }
  }
  if (!hasElementChild) {
    return std::string(_thisFunctionElement.child_value());
  }

  // Otherwise serialize each child node (XML for elements, raw text for text nodes).
  std::ostringstream oss;
  for (auto c = _thisFunctionElement.first_child(); c; c = c.next_sibling()) {
    c.print(oss, "", pugi::format_raw | pugi::format_no_declaration);
  }
  return oss.str();
}


//----------------------------------------------------------------------------//

Sieve* Utilities::evaluateSieve(std::string _input, void* _object){
  return evaluateSieveFunction(_input, _object);
}


//----------------------------------------------------------------------------//

double Utilities::evaluate(std::string _input, void* _object){
  if (_input == "") return 0;

  string workingString = _input;
  // Test if there is any function in this string (look for <Fun>), if so,
  // replace the function with the evaluated number. Repeat until all the
  // functions are replaced by numbers.
  size_t locOfFun = workingString.find("<Fun>");
  int functionStringLength;
  while (locOfFun != string::npos){
    size_t locOfEndFun = findTheEndOfFirstFunction (workingString);
    functionStringLength = ((int) locOfEndFun) + 6 - ((int) locOfFun);//6 is the length of "</fun>"
    string functionString = workingString.substr(locOfFun, functionStringLength);
    string evaluatedFunction = evaluateFunction( functionString, _object);
    string front = workingString.substr(0, locOfFun);
    string back = workingString.substr(((int)locOfEndFun) +6);
    workingString = front + evaluatedFunction + back;
// cout << "utilities string: " << workingString << endl;
    // look for next function
    locOfFun = workingString.find("<Fun>");
  }
  // evaluate the expression to the final result
  mu::Parser p;
  p.SetExpr(workingString);
  double result;

  try {
    result = p.Eval();
//cout << "utilities result: " << result << endl;
  } catch (mu::ParserError) {
    cerr << "Oooops, we find a typo in your project." << endl;
    if (_object != NULL){
        cerr << "The string we see is located in Event (or Bottom): " << ((Event*)_object)->getEventName() << endl;
    }
    cerr << "The string we see is: " << _input << endl;
    exit(1);
  }

  return result;
}

//----------------------------------------------------------------------------//

bool Utilities::isSieveFunction(pugi::xml_node input)
{
  return (   (input = GFEC(input))
          && (input = GFEC(input))
          && (XMLTC(input) == "ChooseL" || XMLTC(input) == "ValuePick"));
}

//----------------------------------------------------------------------------//

void* Utilities::evaluateObject(string _input,
                                void* _object,
                                EventType _returnType){

  //remove any spaces
  string input =  removeSpaces( _input);

  // call the proper method
  if (_returnType == eventEnv){
    return getEnvelope(input, _object);
  }
  else if (_returnType ==eventPat){
    return (void*) getPattern( input, _object);
  }
  else if (_returnType ==eventSiv){
    return (void*) getSieve( input, _object);
  }
  // eventSpa / eventRev / eventFil / eventSpec return pugi::xml_node values;
  // callers should use evaluateSpa/evaluateRev/evaluateFil/evaluateSpectrumElement.
  return (void*)  NULL;
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::evaluateSpa(void* _object){
  return getSPAFunctionElement(_object);
}

pugi::xml_node Utilities::evaluateRev(void* _object){
  return getREVFunctionElement(_object);
}

pugi::xml_node Utilities::evaluateFil(void* _object){
  return getFILFunctionElement(_object);
}

pugi::xml_node Utilities::evaluateSpectrumElement(string _input, void* _object){
  return getSpectrum(removeSpaces(_input), _object);
}


//----------------------------------------------------------------------------//

std::vector<std::string> Utilities::listElementToStringVector(
            pugi::xml_node _listElement){
  std::vector<std::string> list;
  string listString = XMLTranscode(_listElement);
  bool doneProcessingListString = false;
  while (!doneProcessingListString){
    //  Step 1: Isolate the first element of the list and put its string
    //          representation in the vector.
    bool listElementLocated = false;
    size_t locationOfComma = listString.find(","); //this can be npos
    size_t locationOfFun = listString.find("<Fun>"); //this can be npos
    while (!listElementLocated){

      if (locationOfComma == string::npos){ //reach the end of the string
        listElementLocated =true;
      }
      else if (locationOfFun==string::npos || locationOfFun> locationOfComma){
        listElementLocated = true;
      }
      else { //find the end of this fun, and find comma and fun again
        size_t endOfThisFun =Utilities::findTheEndOfFirstFunction(listString.substr(locationOfFun)) + locationOfFun;

        locationOfComma = listString.find(",", endOfThisFun);
        locationOfFun = listString.find("<Fun>",endOfThisFun);
      }

    } // end of inner while

    //  Step 2: Cut the string from the listString
    if (locationOfComma ==string::npos){
      list.push_back(listString);
    }
    else {
      string thisListElement = listString.substr(0, locationOfComma);
      list.push_back(thisListElement);
      listString = listString.substr(locationOfComma +1);
    }
    //  Step 3: check if the listString is empty
    if (locationOfComma ==string::npos){
      doneProcessingListString = true;
    }
  }

  return list;
}

//----------------------------------------------------------------------------//

//this function assume that at least 1 "<Fun>" exists;
size_t Utilities::findTheEndOfFirstFunction(string _input){

  int depth = 1; // The depth of nested functions

  // When seeing a <Fun>, depth ++, seeing a </Fun>, depth --
  // When depth = 0, the outermost function has ended.
  size_t location = _input.find("<Fun>");
  while (depth!=0){
    size_t nextFun = _input.find("<Fun>", location + 1);
    size_t nextEndFun = _input.find("</Fun>", location + 1);

    if (nextFun < nextEndFun){
      depth ++;
      location = nextFun;
    }
    else {
      depth --;
      location = nextEndFun;
    }
  }
  return location;
}

//----------------------------------------------------------------------------//

void Utilities::addSound(Sound* _sound){
  if (!soundSynthesis){
    delete _sound;
    return;
  }
  else {
    score->add(_sound);
  }
}

//----------------------------------------------------------------------------//

MultiTrack* Utilities::doneCMOD(){
  if (score != NULL){
   return score->doneAddingSounds();
  }
  else return NULL;
}


//----------------------------------------------------------------------------//

string Utilities::removeSpaces(string _originalString){
  string input = _originalString;
  size_t index = 0;

  // Iterate through each instance of " " and remove
  while (true) {
    index = input.find(" ", index);
    if (index == string::npos) break;
    input.replace(index, 1, "");
    index ++;
  }
  return input;
}


//----------------------------------------------------------------------------//

string Utilities::evaluateFunction(string _functionString,void* _object){
  // convert the function string to a pugi::xml_node
  pugi::xml_document parsedDoc;
  parsedDoc.load_string(_functionString.c_str());
  pugi::xml_node root = parsedDoc.document_element();

  string resultString = "";

  pugi::xml_node functionNameElement = GFEC(root);
  string functionName = functionNameElement.child_value();

  // check the function name and call the proper method for evaluation
  if(functionName.compare("RandomInt")==0){
     resultString = function_RandomInt(root, _object);
  }

  else if (functionName.compare("RandomOrderInt")==0) {
    resultString = function_RandomOrderInt(root, _object);
  }

  else if (functionName.compare("Random")==0){
    resultString = function_Random(root, _object);
  }

  else if (functionName.compare("Select")==0){
    resultString = function_Select(root, _object);
  }

  else if (functionName.compare("GetPattern")==0){
    resultString = function_GetPattern(root, _object);
  }
  else if (functionName.compare("Markov") == 0) {
    resultString = function_Markov(root, _object);
  }

  else if (functionName.compare("Randomizer")==0){
    resultString = function_Randomizer(root, _object);
  }
  else if (functionName.compare("RandomDensity") == 0) {
    resultString = function_RandomDensity(root, _object);
  }
  else if (functionName.compare("ChooseL")==0){
    resultString = function_ChooseL(root, _object);
  }

  else if (functionName.compare("ValuePick")==0){
    resultString = function_ValuePick(root, _object);
  }

  else if (functionName.compare("Stochos")==0){
    resultString = function_Stochos(root, _object);
  }
  else if (functionName.compare("Decay")==0){
    resultString = function_Decay(root, _object);
  }

  else if (functionName.compare("Fibonacci")==0){
    resultString = function_Fibonacci(root, _object);
  }

  else if (functionName.compare("LN")==0){
    resultString = function_LN(root, _object);
  }
   else if (functionName.compare("Inverse")==0){
    resultString = function_Inverse(root, _object);
  }

  // static functions
  else if (functionName.compare("CURRENT_TYPE")==0){
    resultString = static_function_CURRENT_TYPE( _object);
  }

  else if (functionName.compare("CURRENT_CHILD_NUM")==0){
    resultString = static_function_CURRENT_CHILD_NUM( _object);
  }

  else if (functionName.compare("CURRENT_PARTIAL_NUM")==0){
    resultString = static_function_CURRENT_PARTIAL_NUM( _object);
  }
  else if (functionName.compare("CURRENT_DENSITY")==0){
    resultString = static_function_CURRENT_DENSITY( _object);
  }
  else if (functionName.compare("CURRENT_SEGMENT")==0){
    resultString = static_function_CURRENT_SEGMENT( _object);
  }
  else if (functionName.compare("AVAILABLE_EDU")==0){
    resultString = static_function_AVAILABLE_EDU( _object);
  }
  else if (functionName.compare("CURRENT_LAYER")==0){
    resultString = static_function_CURRENT_LAYER( _object);
  }
  else if (functionName.compare("PREVIOUS_CHILD_DURATION")==0){
    resultString = static_function_PREVIOUS_CHILD_DURATION( _object);
  }

  return resultString;
}


//---------------------------------------------------------------------------//
Sieve* Utilities::evaluateSieveFunction(string _functionString,void* _object){
  // convert the function string to a pugi::xml_node
  pugi::xml_document parsedDoc;
  parsedDoc.load_string(_functionString.c_str());
  pugi::xml_node root = parsedDoc.document_element();

  Sieve* resultSieve = nullptr;

  pugi::xml_node functionNameElement = GFEC(root);
  string functionName = functionNameElement.child_value();

  if (functionName.compare("ChooseL")==0){
    resultSieve = sieve_ChooseL(root, _object);
  }
  else if (functionName.compare("ValuePick")==0){
    resultSieve = sieve_ValuePick(root, _object);
  }

  return resultSieve;
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_TYPE(void* _object){
  if (_object !=NULL){
    double resultNum = ((Event*)_object)->getCurrentChildType();
    char result [50];
    sprintf(result, "%f",  resultNum);
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_TYPE has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_CHILD_NUM(void* _object){
  if (_object !=NULL){
    double resultNum = ((Event*)_object)->getCurrentChild();
    char result [50];
    sprintf(result, "%f",  resultNum);
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_NUM has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_PARTIAL_NUM(void* _object){
  if (_object !=NULL){
    double resultNum = ((Bottom*)_object)->getCurrPartialNum();
    char result [50];
    sprintf(result, "%f",  resultNum);
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_PARTIAL_NUM has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_DENSITY(void* _object){
  cout<<"Utilities:Warning! static_function_CURRENT_DENSITY is not implemented in CMOD 2.0 yet."<<endl;

  return "0";
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_SEGMENT(void* _object){
    //should recieve an envelope as the _object? --Ming-ching May 07 2013
    cout<<"Utilities:Warning! static_function_CURRENT_SEGMENT is not implemented in CMOD 2.0 yet."<<endl;

  return "0";
}

//----------------------------------------------------------------------------//

string Utilities::static_function_AVAILABLE_EDU(void* _object){
  if (_object !=NULL){
    double resultNum = ((Event*)_object)->getAvailableEDU();
    char result [50];
    sprintf(result, "%f",  resultNum);
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_AVAILABLE_EDU has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_LAYER(void* _object){
  if (_object !=NULL){
    double resultNum = ((Event*)_object)->getCurrentLayer();
    char result [50];
    sprintf(result, "%f",  resultNum);
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_LAYER has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_PREVIOUS_CHILD_DURATION(void* _object){
if (_object !=NULL){
    double resultNum = ((Event*)_object)->getPreviousChildEndTime();
    char result [50];
    sprintf(result, "%f",  resultNum);
//  cout << "Utilities::static_function_PREVIOUS_CHILD_DURATION - resultNum=" 
//       << resultNum << endl;
    return string(result);
  }
  else {
    cerr<<"Utilities:Warning! static_function_PREVIOUS_CHILD_DURATION has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::function_Inverse(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  double entry = evaluate(XMLTranscode(elementIter ),_object);

  double resultNum = ( 1. / entry );
  char result [50];
  sprintf(result, "%f",  resultNum);
  return string(result);
}


//---------------------------------------------------------------------------//

string Utilities::function_Markov(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  int entry = (int)evaluate(XMLTranscode(elementIter), _object);

  float resultNum = markovModelLibrary[entry].nextSample(Random::Rand());
  char result [50];
  sprintf(result, "%f", resultNum);
  return string(result);
}

//----------------------------------------------------------------------------//

string Utilities::function_LN(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  double entry = evaluate(XMLTranscode(elementIter ),_object);

  double resultNum = ( 1. / pow(2.71828, entry) );
  char result [50];
  sprintf(result, "%f",  resultNum);
  return string(result);
}

//----------------------------------------------------------------------------//

string Utilities::function_Fibonacci(pugi::xml_node _functionElement, void* _object){
  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  int entry = evaluate(XMLTranscode(elementIter ),_object);

  int numA = 1;
  int numB = 1;
  for (int i = 3; i <= entry; i++) {
    int swap = numB;
    numB += numA;
    numA = swap;
  }
  int resultNum = numB;
  char result [50];
  sprintf(result, "%i",  resultNum);
  return string(result);
}

//----------------------------------------------------------------------------//

string Utilities::function_Decay(pugi::xml_node _functionElement, void* _object){
//  <Fun>
//    <Name>Decay</Name>
//    <Base>base</Base>
//    <Type>EXPONENTIAL</Type>
//    <Rate>rate</Rate>
//    <Index>CURRENT_PARTIAL_NUM</Index>
//  </Fun>

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  double base = evaluate(XMLTranscode(elementIter ),_object);

  elementIter = GNES(elementIter);
  string type = XMLTranscode(elementIter);

  elementIter = GNES(elementIter);
  double rate = evaluate(XMLTranscode(elementIter ),_object);

  elementIter = GNES(elementIter);
  double index = evaluate(XMLTranscode(elementIter ),_object);


  double decay = 0.0;  // Initialize decay to avoid uninitialized variable warning

  if (type == "EXPONENTIAL") {
    decay = base * pow(rate, index);
  } else if (type == "LINEAR") {
    decay = base - (rate * index);
  }

  char result [50];
  sprintf(result, "%f",  decay);
  return string(result);
}

//----------------------------------------------------------------------------//

string Utilities::function_Stochos(pugi::xml_node _functionElement, void* _object){
//  <Fun>
//    <Name>Stochos</Name>
//    <Method>RANGE_DISTRIB</Method>
//    <Envelopes>
//      <Envelope>ENV</Envelope>
//      <Envelope>ENV</Envelope>
//      <Envelope>ENV</Envelope>
//    </Envelopes>
//    <Offset>3</Offset>
//  </Fun>

  // setup routine vars
  double checkpoint = 0;

  if (_object != NULL) {
    checkpoint = ((Event*)_object)->getCheckPoint();
    if (checkpoint < 0 || checkpoint > 1) {
      cerr << "Utilities:Stochos -- checkpoint error!" << endl;
      cerr << "   checkPoint = " << checkpoint;
      cerr << ", filename = " << ((Event*)_object)->getEventName();
      cerr << endl;
    }
  }

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  string method = XMLTC(elementIter);

  elementIter = GNES(elementIter);
  pugi::xml_node envElementIter = GFEC(elementIter);
  vector<Envelope*> envVect;
  while (envElementIter!=NULL) {
    //cou << MLTC(envElementIter)<<endl;
    envVect.push_back((Envelope*)evaluateObject(XMLTC(envElementIter), _object, eventEnv));
    envElementIter = GNES(envElementIter);
  }


  elementIter = GNES(elementIter);
  int offset = (int) evaluate ( XMLTC(elementIter), _object);
  float returnVal = 0.0;

  if(method == "FUNCTIONS") {
    float randomNumber;

    // stacked up envelopes: their values at the same moment add up to 1
    for (int i = 0; i < (int)envVect.size(); i++) {
      returnVal = envVect[i]->getValue(checkpoint, 1.);
      if(envVect.size() > 1) {                      // probability areas
        if(i == 0) randomNumber = Random::Rand();
        if (returnVal >= randomNumber) {
          returnVal = i;
          i = envVect.size(); // done: break out of the for loop
        }
      }
    }
  } else if (method == "RANGE_DISTRIB") {
    float limit[2];

    // distribution within given range; takes 3 envs: min, MAX, val in between
    if((int)envVect.size() <= 3 * offset) {
      cerr << "Error - Stochos - Not enough envelopes on the list: envVect.size="
           << envVect.size() << " 3*offset=" << 3 * offset << endl;
      if (_object != NULL) {
        cerr << "       in file " << ((Event*)_object)->getEventName() << endl;
      }
      exit(1);
    }
    for(int i = 0; i < 2; i++) {
      if(envVect[3 * offset + i] != NULL) {
        limit[i] = envVect[3 * offset + i]->getValue(checkpoint, 1);
      } else {
        cerr << "Stochos - NULL envelope. Trying to access envy["
             << 3 * offset + i<< "]=" <<  envVect[3 * offset + i] << endl;
      }
    }

    if(envVect[3 * offset + 2]) {
      returnVal = envVect[3 * offset + 2]->getValue(Random::Rand(), 1);
    } else {
      cerr << "Stochos - NULL envelope. Trying to access envVect["
           << 3 * offset + 2 << "]=" << envVect[ 3 * offset + 2] << endl;
    }

    returnVal *= (limit[1] - limit[0]);
    returnVal += limit[0];
  } else {
    cerr << "Stochos --- invalid method!  Use FUNCTIONS or RANGE_DISTRIB" << endl;
    exit(1);
  }

  for (unsigned i = 0; i < envVect.size(); i ++){
    delete envVect[i];
  }

  char result [50];
  sprintf(result, "%f",  returnVal);
  return string(result);

}

//----------------------------------------------------------------------------//

string Utilities::function_ValuePick(pugi::xml_node _functionElement, void* _object){
  Sieve* si = sieve_ValuePick(_functionElement, _object);
  int resultNum = si->ChooseL();
  delete si;
  char result [50];
  sprintf(result, "%i",  resultNum);
  return string(result);
}

//----------------------------------------------------------------------------//

Sieve* Utilities::sieve_ValuePick(pugi::xml_node _functionElement, void* _object){

//<Fun>
//  <Name>ValuePick</Name>
//  <Range>range</Range>
//  <Low>anEnv</Low>
//  <High>anEnv</High>
//  <Dist>anEnv</Dist>
//  <Method>MEANINGFUL</Method>
//  <Elements>1,2,3,4</Elements>
//  <WeightMethod> PERIODIC</WeightMethod>
//  <Weight>1,1,1,1</Weight>
//  <Type>VARIABLE</Type>
//  <Offsets>0,0,0,0</Offsets>
//</Fun>

//cout << "		Sieve* Utilities::sieve_ValuePick" << endl;
// setup routine vars
  double checkpoint = 0;

  if (_object != NULL) {
    checkpoint = ((Event*)_object)->getCheckPoint();
  }

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));

  //Range, low, high and distribution  envelopes
  double absRange =  evaluate (XMLTC(elementIter), _object);

  elementIter = GNES(elementIter);
  Envelope *envLow = (Envelope*)evaluateObject(XMLTC(elementIter), _object, eventEnv);

  elementIter = GNES(elementIter);
  Envelope *envHigh = (Envelope*)evaluateObject(XMLTC(elementIter), _object, eventEnv);

  elementIter = GNES(elementIter);
  Envelope *envDist = (Envelope*)evaluateObject(XMLTC(elementIter), _object, eventEnv);

  elementIter = GNES(elementIter);
  string eMethod = XMLTC(elementIter);

  elementIter = GNES(elementIter);
  vector<string> eArgs = listElementToStringVector( elementIter);
  vector<int> eArgVect;

  if (eMethod != "MODS") {
    for (unsigned i = 0; i < eArgs.size(); i ++){
      eArgVect.push_back((int)evaluate(eArgs[i], _object));
    }
  }
  elementIter = GNES(elementIter);
  string wMethod = XMLTC(elementIter);

  elementIter = GNES(elementIter);
  vector<string> wArgs = listElementToStringVector( elementIter);
  vector<int> wArgVect;
  for (unsigned i = 0; i < wArgs.size(); i ++){
    wArgVect.push_back((int)evaluate(wArgs[i], _object));
  }

  elementIter = GNES(elementIter);
  string modifyMethod = XMLTC(elementIter);

  elementIter = GNES(elementIter);
  vector<std::string> offsetString = listElementToStringVector( elementIter);
  vector<int> offsetVect;
  for (unsigned i = 0; i < offsetString.size(); i ++){
    offsetVect.push_back((int)evaluate(offsetString[i], _object));
  }


  int minVal = (int)floor( envLow->getScaledValueNew(checkpoint, 1) * absRange + 0.5);
  int maxVal = (int)floor( envHigh->getScaledValueNew(checkpoint, 1) * absRange + 0.5);

  Sieve si;
  if (eMethod == "MODS") {
//cout << "Sieve* Utilities::sieve_ValuePick - eMethod: " << eMethod << endl;
      si.BuildFromExpr(minVal, maxVal,
                         eMethod.c_str(), wMethod.c_str(),
                         eArgs[0], wArgVect,
                         offsetVect);

/*			added by Sever
  vector<int> attTimes;
  vector<double> attProbs;

  si.FillInVectors(attTimes, attProbs);
cout << "   sieve_ValuePick - attTimes.size()=" << attTimes.size() << endl;
  int beatEDUs;
  beatEDUs = _tempo.getEDUPerTimeSignatureBeat().Num();
cout << "      beatEDUs=" << beatEDUs << endl;

  for(int i = 0; i < attTimes.size(); i++){

    if(attTimes[i] > eArgs.size()){
      break;
    }

 // short_attime.push_back(attTimes[i]);
    cout << "   attTimes.size=" << attTimes.size() << endl;
    cout << attTimes[i] << " , ";
  }
  cout << endl;
*/

  } else {
    si.Build(minVal, maxVal, eMethod.c_str(), wMethod.c_str(), eArgVect, wArgVect, offsetVect);
  }
  
  si.Modify(envDist, modifyMethod);

  delete envLow;
  delete envHigh;
  delete envDist;
  return new Sieve(si);
}

//----------------------------------------------------------------------------//

Sieve* Utilities::sieve_ChooseL(pugi::xml_node _functionElement, void* _object) {
  string sivFunctionString =  XMLTC(GNES(GFEC(_functionElement))) ;
  return (Sieve*)evaluateObject(sivFunctionString, _object, eventSiv);
}

//----------------------------------------------------------------------------//

string Utilities::function_ChooseL(pugi::xml_node _functionElement, void* _object){
  //<Fun><Name>ChooseL</Name><Entry>SIV</Entry></Fun>
  string sivFunctionString =  XMLTC(GNES(GFEC(_functionElement))) ;
  Sieve* svPtr = (Sieve*)evaluateObject(sivFunctionString, _object, eventSiv);
  double resultNum = svPtr->ChooseL();
  delete svPtr;
  char result [50];
  sprintf(result, "%f",  resultNum);
  return string(result);
}

//----------------------------------------------------------------------------//

string Utilities::function_MakeList(pugi::xml_node _functionElement, void* _object){
  cout<<"Utilites: Make_List is not implemented yet."<<endl;
//<Fun><Name>MakeList</Name><Func></Func><Size></Size></Fun>

  pugi::xml_node listElement = GNES(GFEC(_functionElement));

  std::vector<std::string> stringList = listElementToStringVector(listElement);
  pugi::xml_node boundElement = GNES(listElement);

  int bound = (int)evaluate(XMLTranscode(boundElement), _object);
/*
  char result [50];
  sprintf(result, "%f",  evaluate( list[index], _object));
  return string(result);
*/
    vector<int> intList;

    for (unsigned i = 0; i < intList.size(); i ++){
      int num = (int) evaluate(stringList[i], _object);
      if (num <= bound) {
        intList.push_back(num);
      } else {
  return "intList";
      }
    }

  return "intList";
}

//----------------------------------------------------------------------------//

string Utilities::function_Randomizer(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node baseValElement = GNES(GFEC(_functionElement));
  pugi::xml_node percDevElement = GNES(baseValElement);

  double baseVal = evaluate(XMLTranscode(baseValElement ),_object);
  double percDev = evaluate(XMLTranscode(percDevElement), _object);

  double devVal = baseVal * percDev;
  double resultNum = Random::Rand(baseVal - devVal, baseVal + devVal);

  char result [50];
  sprintf(result, "%f",  resultNum);
  return string(result);

}

//----------------------------------------------------------------------------//

string Utilities::function_Random(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node lowBoundElement = GNES(GFEC(_functionElement));
  pugi::xml_node highBoundElement = GNES(lowBoundElement);

  double lowBound = evaluate(XMLTranscode(lowBoundElement ),_object);
  double highBound = evaluate(XMLTranscode(highBoundElement), _object);

  char result [50];
  sprintf(result, "%f",  Random::Rand(lowBound, highBound));
  return string(result);

}

//----------------------------------------------------------------------------//

string Utilities::function_Select(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node listElement = GNES(GFEC(_functionElement));
  pugi::xml_node indexElement = GNES(listElement);

  std::vector<std::string> list = listElementToStringVector(listElement);

  int index = (int)evaluate(XMLTranscode(indexElement), _object);
  char result [50];
/*
for(int i=0; i<list.size(); ++i)
std::cout << list[i] << ' ';
*/
  sprintf(result, "%f",  evaluate( list[index], _object));
/*
cout << "  result= ";
for(int i=0; i <= 2; i++ ) {
cout << result[i];
}
cout << endl;
*/
  return string(result);

}

//----------------------------------------------------------------------------//

string Utilities::function_SelectObject(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node listElement = GNES(GFEC(_functionElement));
  pugi::xml_node indexElement = GNES(listElement);
  std::vector<std::string> list = listElementToStringVector(listElement);
  int index = (int)evaluate(XMLTranscode(indexElement), _object);
  return list[index];
}

//----------------------------------------------------------------------------//

string Utilities::function_GetPattern(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));//method

  string method = XMLTranscode(elementIter);
  elementIter = GNES(elementIter);//origin
  int origin = (int)evaluate(XMLTranscode(elementIter), _object);

  string patternString = XMLTranscode(_functionElement);

  Patter* pattern = NULL;
  for (unsigned i = 0; i < ((Event*) _object)->patternStorage.size(); i++){

    if (patternString == (static_cast<Event*> (_object))->patternStorage[i]->getKey()){
      //cout<<"find existing pattern"<<endl;
      pattern = ((Event*) _object)->patternStorage[i]->getPattern();
      break;
    }
  }
  if (pattern ==NULL){ //this pattern doesn't exist, make a new pattern and put
                       //it in the _object's patternStorage. the _object is
                       //responsible for cleaning up the memory when it's done.

    pattern = (Patter*) (evaluateObject(XMLTranscode(GNES(elementIter)) , _object, eventPat));
    ((Event*) _object)->addPattern(patternString, pattern);

  }

  double returnValue = pattern->GetNextValue(method, origin);
  char result [50];
  sprintf(result, "%f", returnValue);
  return string(result);

}


//----------------------------------------------------------------------------/
string Utilities::function_RandomInt(pugi::xml_node _functionElement, void* _object){
  pugi::xml_node lowBoundElement = GNES(GFEC(_functionElement));
  pugi::xml_node highBoundElement = GNES(lowBoundElement);

  int lowBound = (int)evaluate(XMLTranscode(lowBoundElement), _object);
  int highBound = (int)evaluate(XMLTranscode(highBoundElement), _object);
  char result [50];
  sprintf(result, "%d",  Random::RandInt(lowBound, highBound));
  return string(result);
}

//---------------------------------------------------------------------------//

string Utilities::function_RandomOrderInt(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node lowBoundElement = GNES(GFEC(_functionElement));
  pugi::xml_node highBoundElement = GNES(lowBoundElement);
  pugi::xml_node idElement = GNES(highBoundElement);

  int lowBound = (int)evaluate(XMLTranscode(lowBoundElement), _object);
  int highBound = (int)evaluate(XMLTranscode(highBoundElement), _object);
  int id = (int) evaluate(XMLTranscode(idElement), _object);
  
  // Event* currentEvent = ((Event*)_object);
  // int numChildren = currentEvent->getNumberOfChildren();
  // string eventName = currentEvent->getEventName();

  // // Warn the user if the # of choices is less than # of children
  // if (highBound - lowBound + 1 < numChildren) {
  //   cout << "WARNING: number of choices in RandomOrderInt [" 
  //        << lowBound << ", " << highBound << "] is less than"
  //        << " number of children in event " << eventName << "."
  //        << " This will cause repeated values."
  //        << endl;
  // }

  char result [50];
  sprintf(result, "%d",  Random::RandOrderInt(lowBound, highBound, id));
  return string(result);
}

//---------------------------------------------------------------------------//

string Utilities::function_RandomDensity(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node envelopeNumberElement = GNES(GFEC(_functionElement));
  pugi::xml_node lowBoundElement = GNES(envelopeNumberElement);
  pugi::xml_node highBoundElement = GNES(lowBoundElement);

  int envelopeNumber = (int)evaluate(XMLTranscode(envelopeNumberElement), _object);
  double lowBound = evaluate(XMLTranscode(lowBoundElement), _object);
  double highBound = evaluate(XMLTranscode(highBoundElement), _object);

  ProbabilityEnvelope env(*(envelopeLibrary->getEnvelope(envelopeNumber)));
  // initialize the count table
  env.generateCountTable(1000);

  // sample from the count table
  double rand = Random::Rand(0, 1);
  double resultNumber = env.sample(rand) * (highBound - lowBound) + lowBound;
  // cout << "lowbound: " << lowBound << ", highbound: " << highBound << ", result: " << resultNumber << endl;

  char result [50];
  sprintf(result, "%lf", resultNumber);
  return string(result);
}

//----------------------------------------------------------------------------//


Sieve* Utilities::getSieve(string _functionString, void* _object){
  string toParse = "<root>" + _functionString + "</root>";
  pugi::xml_document doc;
  doc.load_string(toParse.c_str());
  pugi::xml_node root = doc.document_element();
  return getSieveHelper(_object, root);
}

//----------------------------------------------------------------------------//

Sieve* Utilities::getSieveHelper(void* _object, pugi::xml_node _SIVFunction){

  double checkpoint = 0;

  if (_object != NULL) {
    checkpoint = ((Event*)_object)->getCheckPoint();
  }
  //cout << "Utilities::getSieve. checkpoint: " << checkpoint << endl;
  // Get the function name
  pugi::xml_node functionNameElement = GFEC(GFEC(_SIVFunction));

  // If the function is ReadSIVFile:
  // Get the _SIVFunction from the filename, and recursively call itself
  // on that function.
  if (XMLTranscode(functionNameElement).compare("ReadSIVFile")==0){
    string fileName = XMLTranscode(GNES(functionNameElement));
    pugi::xml_node k = getEventElement(eventSiv, fileName);
    return getSieveHelper(_object, GNES(GNES(GFEC(k))));
  }

  // If the function is MakeSieve:
  // Read each of the values from _SIVFunction and create a new Sieve with
  // those values.
  else if (XMLTranscode(functionNameElement).compare("MakeSieve")==0){
  /*
    // Get minVal
    pugi::xml_node elementIter = GNES(GFEC(GFEC(_SIVFunction)));
    int minVal = evaluate(XMLTC(elementIter), _object);


    // Get maxVal
    elementIter = GNES(elementIter);
    int maxVal = evaluate(XMLTC(elementIter), _object);
  */
    // Get minVal
    pugi::xml_node elementIter = GNES(GFEC(GFEC(_SIVFunction)));

    Envelope *envLow = (Envelope*)evaluateObject(XMLTC(elementIter), _object, eventEnv);
    int minVal = (int)floor( envLow->getScaledValueNew(checkpoint, 1) + 0.5);

    // Get maxVal
    elementIter = GNES(elementIter);
    Envelope *envHigh = (Envelope*)evaluateObject(XMLTC(elementIter), _object, eventEnv);
    int maxVal = (int)floor( envHigh->getScaledValueNew(checkpoint, 1) + 0.5);
    //cout << "Utilities::getSieve. min: " << minVal << " max: " << maxVal << endl;
    // Get eMethod
    elementIter = GNES(elementIter);
    string eMethod = XMLTC(elementIter);

    // Get eArgInts
    elementIter = GNES(elementIter);
    vector<string> eArgs = listElementToStringVector( elementIter);
    vector<int> eArgInts;
    if (eMethod != "MODS") {
      for (unsigned i = 0; i < eArgs.size(); i ++){
        eArgInts.push_back((int)evaluate(eArgs[i], _object));
      }
    }

    // Get wMethod
    elementIter = GNES(elementIter);
    string wMethod = XMLTC(elementIter);

    // Get wArgInts
    elementIter = GNES(elementIter);
    vector<string> wArgs = listElementToStringVector( elementIter);
    vector<int> wArgInts;
    for (unsigned i = 0; i < wArgs.size(); i ++){
      wArgInts.push_back((int)evaluate(wArgs[i], _object));
    }

    // Get offsetVect
    elementIter = GNES(elementIter);
    vector<string> offsetString = listElementToStringVector( elementIter);
    vector<int> offsetVect;

    if (offsetString.size() == 1){ //only one number, so we need to copy it to form a vector to match the number elements
      for (unsigned i = 0; i < eArgs.size(); i ++){
        offsetVect.push_back((int)evaluate(offsetString[0], _object));
      }
    } else {
      for (unsigned i = 0; i < offsetString.size(); i ++){
        offsetVect.push_back((int)evaluate(offsetString[i], _object));
      }
    }

    // Build the Sieve
    Sieve* siv = new Sieve();
    if (eMethod == "MODS") {
      siv->BuildFromExpr(minVal, maxVal,
                         eMethod.c_str(), wMethod.c_str(),
                         eArgs[0], wArgInts,
                         offsetVect);
    } else {
      siv->Build(minVal, maxVal,
                 eMethod.c_str(), wMethod.c_str(),
                 eArgInts, wArgInts,
                 offsetVect);
    }
    return siv;
  }//end MakeSieve

  // If the function is Select:
  // Parse the Select function from _SIVFunction and recursively call itself
  // on the new function.
  else if (XMLTranscode(functionNameElement).compare("Select")==0){
    string selectedListElementString = "<root>" + function_SelectObject(GFEC(_SIVFunction), _object) + "</root>";
    pugi::xml_document doc;
    doc.load_string(selectedListElementString.c_str());
    pugi::xml_node root = doc.document_element();
    return getSieveHelper(_object, root);
  }

  // Otherwise, the function fails.
  cerr<<"Utilities::Warning! Sieve Construction Failed"<<endl;
  return NULL;
}



//----------------------------------------------------------------------------//

Patter* Utilities::getPattern(string _functionString, void* _object){
  string toParse = "<root>" + _functionString + "</root>";
  pugi::xml_document doc;
  doc.load_string(toParse.c_str());
  pugi::xml_node root = doc.document_element();
  return getPatternHelper(_object, root);
}

//----------------------------------------------------------------------------/e {


Patter* Utilities::getPatternHelper(void* _object, pugi::xml_node _PATFunction){
  // Get the function name
  pugi::xml_node functionNameElement = GFEC(GFEC(_PATFunction));

  // If the function is ReadPATFile:
  // Get the _PATFunction from the filename, and recursively call itself
  // on that function.
  if (XMLTranscode(functionNameElement).compare("ReadPATFile")==0){
    string fileName = XMLTranscode(GNES(functionNameElement));
    pugi::xml_node k = getEventElement(eventPat, fileName);

    return getPatternHelper(_object, GNES(GNES(GFEC(k))));

  }

  // If the function is MakePattern:
  // Read each of the values from _PATFunction and create a new Patter with
  // those values.
  else if (XMLTranscode(functionNameElement).compare("MakePattern")==0){

cout << "Patter* Utilities::getPatternHelper - MakePattern option" << endl;
    pugi::xml_node listElement = GNES(GFEC(GFEC(_PATFunction)));
cout << "	after listElement" << endl;
    vector<string> stringList =listElementToStringVector (listElement);
cout <<	"after ElementToString" << "  " << endl;
    vector<int> intList;
cout << "intList size: " << stringList.size() << "    " << endl;
    for (unsigned i = 0; i < stringList.size(); i ++){
      int num = (int) evaluate(stringList[i], _object);
      cout << "	i=" << i << " num=" << num << endl;
      intList.push_back(num);
    }

    Patter* newPattern = new Patter(0, intList);
    newPattern->SimplePat();
    return newPattern; //Utilities will put this pattern in _object's patternStorage

  }

  // If the function is ExpandPattern:
  // First find the parameters to expand by.
  // Recursively call itself on the pattern to expand.
  // Expand the pattern.
  //
  // TODO: Figure out if this is true.
  else if (XMLTranscode(functionNameElement).compare("ExpandPattern")==0){
    cout<<"see Expand pattern:"<<endl;
    cout<<XMLTranscode(GFEC(_PATFunction))<<endl;
    pugi::xml_node elementIter = GNES(GFEC(GFEC(_PATFunction)));
    string method = XMLTranscode(elementIter);

    // Find Expand parameters
    elementIter = GNES(elementIter);
    int mod = evaluate ( XMLTranscode( elementIter), _object);

    elementIter = GNES(elementIter);
    int low = evaluate ( XMLTranscode( elementIter), _object);

    elementIter = GNES(elementIter);
    int high = evaluate ( XMLTranscode( elementIter), _object);

    // Recurse on the pattern to expand
    elementIter = GNES(elementIter);
    Patter* pattern = getPatternHelper(_object, elementIter);
    pattern->Expand( method, mod, low, high );
    return pattern;

  }

  // If the function is Select:
  // Parse the Select function from _PATFunction and recursively call itself
  // on the new function.
  else if (XMLTranscode(functionNameElement).compare("Select")==0){
    string selectedListElementString = "<root>" + function_SelectObject(GFEC(_PATFunction), _object) + "</root>";

    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...
    auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();



    Patter* pat =  getPatternHelper(_object, root);
return pat;

  }
  cerr<<"Utilities::Warning! Pattern Construction Failed"<<endl;
  return NULL;
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getSPAFunctionElement(void* _object){
  return getSPAFunctionElementHelper(_object, pugi::xml_node(), true);
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getSPAFunctionElementHelper(void* _object, pugi::xml_node _SPAFunction, bool _initialCall){
  pugi::xml_node SPAElement;

  if (_initialCall ==true){
    Bottom* thisBottom = (Bottom*)_object;
    SPAElement = thisBottom->getSPAElement();
  }
  else {
    SPAElement = _SPAFunction;
  }
  pugi::xml_node functionNameElement = GFEC(GFEC(SPAElement));

  if (XMLTranscode(functionNameElement).compare("ReadSPAFile")==0){
    string functionString = XMLTC(SPAElement);
    string fileName;

    size_t select = functionString.find("Select", 0);
    if (select == string::npos){
      fileName = XMLTranscode(GNES(functionNameElement));
    }
    else{ //see select inside readSPA
      //cout<<functionString<<endl;
      string selectedListElementString = "<root><Fun><Name>ReadSPAFile</Name><File>" + function_SelectObject(GFEC(GNES(GFEC(GFEC(SPAElement)))), _object) + "</File></Fun></root>";
      // Here we need to push a temporary parser in the _object to stored the
      // parsed selectedListElementString...

      selectedListElementString = removeSpaces (selectedListElementString);


      auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();
      ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
      //after the statement above, the ownership of the parser is transfered to
      // the _object. the _object is responsible to clean up the memory of the
      // parser, which is done by the std::vector automatically.
      return getSPAFunctionElementHelper(_object, root, false);

    }//end see select inside readSPA


    fileName = removeSpaces (fileName);


    pugi::xml_node k = getEventElement(eventSpa, fileName);

    return getSPAFunctionElementHelper(_object, GNES(GNES(GFEC(k))), false);

  }
  else if (XMLTranscode(functionNameElement).compare("Select")==0){
    string selectedListElementString = "<root>" + function_SelectObject(GFEC(SPAElement), _object) + "</root>";

    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...
    auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();

    ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
    //after the statement above, the ownership of the parser is transfered to
    // the _object. the _object is responsible to clean up the memory of the
    // parser, which is done by the std::vector automatically.
    return getSPAFunctionElementHelper(_object, root, false);

  }
  else { //function name = SPA
    return GFEC(SPAElement);
  }
  return pugi::xml_node();
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getREVFunctionElement(void* _object){
  return getREVFunctionElementHelper(_object, pugi::xml_node(), true);
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getREVFunctionElementHelper(void* _object, pugi::xml_node _REVFunction, bool _initialCall){

  pugi::xml_node REVElement;

  if (_initialCall ==true){
    Bottom* thisBottom = (Bottom*)_object;
    REVElement = thisBottom->getREVElement();
  }
  else {
    REVElement = _REVFunction;
  }

  pugi::xml_node functionNameElement = GFEC(GFEC(REVElement));

  if (XMLTranscode(functionNameElement).compare("ReadREVFile")==0){
    string functionString = XMLTC(REVElement);
    string fileName;

    size_t select = functionString.find("Select", 0);
    if (select == string::npos){
      fileName = XMLTranscode(GNES(functionNameElement));
    }
    else{ //see select inside readREV
      //cout<<functionString<<endl;
      string selectedListElementString = "<root><Fun><Name>ReadREVFile</Name><File>" + function_SelectObject(GFEC(GNES(GFEC(GFEC(REVElement)))), _object) + "</File></Fun></root>";
    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...

      selectedListElementString = removeSpaces (selectedListElementString);


      auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();
      ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
      //after the statement above, the ownership of the parser is transfered to
      // the _object. the _object is responsible to clean up the memory of the
      // parser, which is done by the std::vector automatically.
      return getREVFunctionElementHelper(_object, root, false);

    }//end see select inside readSPA


    fileName = removeSpaces (fileName);


    pugi::xml_node k = getEventElement(eventRev, fileName);

    return getREVFunctionElementHelper(_object, GNES(GNES(GFEC(k))), false); ;

  }
  else if (XMLTranscode(functionNameElement).compare("Select")==0){
    string selectedListElementString = "<root>" + function_SelectObject(GFEC(REVElement), _object) + "</root>";

    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...
    auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();

    ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
    //after the statement above, the ownership of the parser is transfered to
    // the _object. the _object is responsible to clean up the memory of the
    // parser, which is done by the std::vector automatically.
    return getREVFunctionElementHelper(_object, root, false);

  }

  else { // simple or medium or advanced
    return GFEC(REVElement);
  }
  return pugi::xml_node();
}


//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getFILFunctionElement(void* _object){
  return getFILFunctionElementHelper(_object, pugi::xml_node(), true);
}

//----------------------------------------------------------------------------//

pugi::xml_node Utilities::getFILFunctionElementHelper(void* _object, pugi::xml_node _FILFunction, bool _initialCall){

  pugi::xml_node FILElement;

  if (_initialCall ==true){
    Bottom* thisBottom = (Bottom*)_object;
    FILElement = thisBottom->getFILElement();
  }
  else {
    FILElement = _FILFunction;
  }



  if (GFEC(FILElement)== pugi::xml_node() || GFEC(GFEC(FILElement))== pugi::xml_node()) return pugi::xml_node();
   pugi::xml_node functionNameElement = GFEC(GFEC(FILElement));

  if ( XMLTC(FILElement) ==""){
    cout<<"no filter"<<endl;
    return pugi::xml_node();
  }


  if (XMLTranscode(functionNameElement).compare("ReadFILFile")==0){
    string functionString = XMLTC(FILElement);
    string fileName;

    size_t select = functionString.find("Select", 0);
    if (select == string::npos){
      fileName = XMLTranscode(GNES(functionNameElement));
    }
    else{ //see select inside readFIL
      //cout<<functionString<<endl;
      string selectedListElementString = "<root><Fun><Name>ReadFILFile</Name><File>" + function_SelectObject(GFEC(GNES(GFEC(GFEC(FILElement)))), _object) + "</File></Fun></root>";
    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...

      selectedListElementString = removeSpaces (selectedListElementString);


      auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();
      ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
      //after the statement above, the ownership of the parser is transfered to
      // the _object. the _object is responsible to clean up the memory of the
      // parser, which is done by the std::vector automatically.
      return getFILFunctionElementHelper(_object, root, false);

    }//end see select inside readSPA


    fileName = removeSpaces (fileName);


    pugi::xml_node k = getEventElement(eventFil, fileName);

    return getFILFunctionElementHelper(_object, GNES(GNES(GFEC(k))), false); ;

  }
  else if (XMLTranscode(functionNameElement).compare("Select")==0){
    string selectedListElementString = "<root>" + function_SelectObject(GFEC(FILElement), _object) + "</root>";

    // Here we need to push a temporary parser in the _object to stored the
    // parsed selectedListElementString...
    auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(selectedListElementString.c_str());
      pugi::xml_node root = _docPtr->document_element();

    ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
    //after the statement above, the ownership of the parser is transfered to
    // the _object. the _object is responsible to clean up the memory of the
    // parser, which is done by the std::vector automatically.
    return getFILFunctionElementHelper(_object, root, false);

  }

  else {
    return GFEC(FILElement);
  }
  return pugi::xml_node();
}


//----------------------------------------------------------------------------//
pugi::xml_node Utilities::getSpectrum(string _functionString, void* _object){;
  auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(_functionString.c_str());
      pugi::xml_node _functionElement = _docPtr->document_element();

  return GNES(GFEC(_functionElement));
  }

//----------------------------------------------------------------------------//

Envelope* Utilities::getEnvelope(string _input, void* _object){

  auto _docPtr = std::make_unique<pugi::xml_document>();
      _docPtr->load_string(_input.c_str());
      pugi::xml_node root = _docPtr->document_element();

  pugi::xml_node functionNameElement = GFEC(root);
  string functionName = functionNameElement.child_value();

  Envelope* returnEnvelope;
  if(functionName.compare("EnvLib")==0){
      returnEnvelope = envLib(GNES(functionNameElement), _object);
  }
  else if (functionName.compare("MakeEnvelope")==0){
    returnEnvelope = makeEnvelope(GNES(functionNameElement), _object);
  }
  else if (functionName.compare("ReadENVFile")==0){
    returnEnvelope = readEnvFile(GNES(functionNameElement), _object);
  }
  else if (functionName.compare("Select")==0){
    string selectedListElementString = function_SelectObject(root, _object);
return getEnvelope(selectedListElementString,  _object);
  }
  else {
cout<<functionName<<endl;
    cout<<"warning: evaluating envelope failed."<<endl;
    return NULL;
  }
return returnEnvelope;
}

//----------------------------------------------------------------------------//

Envelope* Utilities::envLib(pugi::xml_node _functionElement, void* _object){
//  <Env>3</Env>
//  <Scale>1.0</Scale>
  int envelopeNumber = evaluate(XMLTranscode(_functionElement), _object);
  Envelope* env = envelopeLibrary->getEnvelope(envelopeNumber);
  //cout <<"EnvLib: #"<<envelopeNumber<<endl;
  double scale = evaluate(XMLTranscode(GNES(_functionElement)), _object);
  env->scale(scale);
  return env;

}

//----------------------------------------------------------------------------//

Envelope* Utilities::readEnvFile(pugi::xml_node _functionElement, void* _object){
  //<File>object name</File>

  pugi::xml_node file = getEventElement(eventEnv, XMLTranscode(_functionElement));

//  <Event orderInPalette=' 0'>
//      <EventType>6</EventType>
//      <Name>oeu</Name>
//      <EnvelopeBuilder><Fun><Name>EnvLib</Name><Env>1</Env><Scale>1.0</Scale></Fun></EnvelopeBuilder>
//    </Event>
//

  pugi::xml_node builder = GNES(GNES(GFEC(file)));

  return (Envelope*) evaluateObject(XMLTranscode(builder), _object, eventEnv);

}

//----------------------------------------------------------------------------//

Envelope* Utilities::makeEnvelope(pugi::xml_node _functionElement, void* _object){

//<Xs>
//  <X>0</X>
//  <X>0.5</X>
//  <X>1</X>
//</Xs>
//<Ys>
//  <Y>1</Y>
//  <Y>0</Y>
//  <Y>0.5</Y>
//</Ys>
//<Types>
//  <T>LINEAR</T>
//  <T>LINEAR</T>
//</Types>
//<Pros>
//  <P>FLEXIBLE</P>
//  <P>FLEXIBLE</P>
//</Pros>
//<Scale>1</Scale>

  pugi::xml_node x = GFEC(_functionElement);
  _functionElement = GNES(_functionElement);
  pugi::xml_node y = GFEC(_functionElement);
  _functionElement = GNES(_functionElement);
  pugi::xml_node t = GFEC(_functionElement);
  _functionElement = GNES(_functionElement);
  pugi::xml_node p = GFEC(_functionElement);

  double scale = evaluate(XMLTranscode(GNES(_functionElement)), _object);

  // create the collection of points
  Collection<xy_point> points;


  float prevYVal = 0;
  float prevXVal = 0;
  while (x!=NULL && y!=NULL) {
    xy_point xy;
    xy.x = evaluate(XMLTranscode(x), _object);
    xy.y = evaluate(XMLTranscode(y), _object);

    if (xy.x - prevXVal < 0) { // flag to keep previous xval
      xy.x = prevXVal * 1.01;
    }
    if (xy.y < 0) { // flag to keep previous yval
      xy.y = prevYVal;
    }
    prevXVal = xy.x;
    prevYVal = xy.y;

    points.add(xy);
    x = GNES(x);
    y = GNES(y);

  }

  // create the collection of segments
  Collection<envelope_segment> segments;
  while (t!=NULL && p!=NULL) {
    envelope_segment seg;

    if ( XMLTranscode(t).compare("LINEAR")==0) {
      seg.interType = LINEAR;
    }
    else if (XMLTranscode(t).compare("SPLINE")==0) {
      seg.interType = CUBIC_SPLINE;
    }
    else if (XMLTranscode(t).compare("EXPONENTIAL")==0) {
      seg.interType = EXPONENTIAL;
    }
    else {
      cerr << "Error in MakeEnvelope: Unrecognized interpolation type" << endl;
      exit(1);
    }

    if (XMLTranscode(p).compare("FIXED")==0) {
      seg.lengthType = FIXED;
    }
    else if (XMLTranscode(p).compare("FLEXIBLE")==0) {
      seg.lengthType = FLEXIBLE;
    }
    else {
      cerr << "Error in MakeEnvelope: Unrecognized envelope length stretch type." << endl;
      exit(1);
    }

    segments.add(seg);

    t = GNES(t);
    p = GNES(p);
  }

  // Create a new envelope given the points and segments defined
  Envelope* madeEnv = new Envelope(points, segments);
  madeEnv->scale(scale);

  // Clean up the temporary point and segment collections
  points.clear();
  segments.clear();

  return madeEnv;

}

//get the envelope shape in env lib corresponding to env_num
Envelope* Utilities::getEnvelopeshape(int env_num, double scale){
    Envelope* env = envelopeLibrary->getEnvelope(env_num);
    if (env == NULL){
      cout << "Error in getEnvelopeShape: env_num exceeds size of EnvLibrary" << endl;
      return NULL;
    }
    env->scale(scale);
    return env;
}
