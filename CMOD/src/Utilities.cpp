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
#include "CmodError.h"
#include "Random.h"
#include "Event.h"
#include "Piece.h"
#include "Patter.h"
#include "ProbabilityEnvelope.h" // consider moving this into LASS.h
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <limits>
#include <sstream>
#include <string>

static int checkedEnvelopeNumber(double number, int size,
                                 const string& function) {
  if (number < 1 || number >= static_cast<double>(size) + 1) {
    throw CmodError(CmodError::Kind::Project,
                    function + " envelope number " + to_string(number)
                        + " is outside the library of " + to_string(size) + " envelopes.",
                    "Function: " + function + " -> Envelope number",
                    "Choose an existing envelope from the library. Envelope numbers start at 1; add an envelope if the library is empty.");
  }
  return static_cast<int>(number);
}

class ObjectReferenceGuard {
public:
  ObjectReferenceGuard(std::vector<pugi::xml_node>& references,
                       pugi::xml_node node, const string& kind, const string& name)
      : references_(references) {
    if (std::find(references.begin(), references.end(), node) != references.end()) {
      throw CmodError(CmodError::Kind::Project,
                      "A circular " + kind + " reference reaches '" + name + "' again.",
                      kind + " object: " + name,
                      "Remove the reference back to this object so its definition can be evaluated without a cycle.");
    }
    references_.push_back(node);
  }
  ~ObjectReferenceGuard() { references_.pop_back(); }
private:
  std::vector<pugi::xml_node>& references_;
};

static string requiredFunctionArgument(pugi::xml_node node,
                                       const string& function, const string& field) {
  const string expression = Utilities::XMLTranscode(node);
  if (!node || expression.find_first_not_of(" \t\r\n") == string::npos) {
    throw CmodError(CmodError::Kind::Project,
                    function + " is missing its " + field + " argument.",
                    "Function: " + function + " -> " + field,
                    "Enter a value or expression for this argument in the function editor.");
  }
  return expression;
}

static int checkedIntegerArgument(double value, const string& function,
                                  const string& field) {
  const double integer = std::trunc(value);
  if (!std::isfinite(value) || integer < std::numeric_limits<int>::min()
      || integer > std::numeric_limits<int>::max()) {
    std::ostringstream originalValue;
    originalValue << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    throw CmodError(CmodError::Kind::Project,
                    function + " " + field + " value " + originalValue.str()
                        + " is outside the supported integer range "
                        + to_string(std::numeric_limits<int>::min()) + " through "
                        + to_string(std::numeric_limits<int>::max()) + ".",
                    "Function: " + function + " -> " + field,
                    "Use a finite value whose integer part is within this range; fractional parts are truncated toward zero.");
  }
  return static_cast<int>(integer);
}

Utilities::Utilities(pugi::xml_node root,
                     string,
                     bool _soundSynthesis,
                     bool _scorePrinting,
                     bool _outputParticel,
                     int _numThreads,
                     int _numChannels,
                     int _samplingRate,
                     Piece* _piece):
  soundSynthesis(_soundSynthesis),
  scorePrinting(_scorePrinting),
  outputParticel(_outputParticel),
  numThreads(_numThreads),
  numChannels(_numChannels),
  samplingRate(_samplingRate),
  piece(_piece){

  for (const char* section : {"EnvelopeLibrary", "MarkovModelLibrary", "Events"}) {
    if (!root.child(section)) {
      throw CmodError(CmodError::Kind::Project,
                      "Missing required project section '" + string(section) + "'.",
                      "ProjectRoot/" + string(section),
                      "Restore the missing section, or resave the original project in LASSIE.");
    }
  }

  // New LASS Score
  if (soundSynthesis){
    score = new Score (numThreads,  numChannels, _samplingRate );
    score->setClippingManagementMode(Score::CHANNEL_ANTICLIP);
  }
  else {
    score = NULL;
  }


  // Construct Envelope library
  pugi::xml_node envelopeLibraryElement = root.child("EnvelopeLibrary");
  string envLibContent = XMLTranscode(envelopeLibraryElement);
  string fileString = "lib.temp";
  FILE* file  = fopen(fileString.c_str(), "w");
  if (file == NULL) {
    throw CmodError(CmodError::Kind::Output,
                    "Cannot create the temporary envelope library file.",
                    "File: " + fileString,
                    "Check that the project folder is writable and that lib.temp is not a directory.");
  }
  const bool writeSucceeded = fputs(envLibContent.c_str(), file) >= 0;
  const bool closeSucceeded = fclose(file) == 0;
  if (!writeSucceeded || !closeSucceeded) {
    throw CmodError(CmodError::Kind::Output,
                    "Cannot write the temporary envelope library file.",
                    "File: " + fileString,
                    "Check free disk space and write permissions for the project folder.");
  }

  envelopeLibrary = new EnvelopeLibrary();
  envelopeLibrary->loadLibraryNewFormat((char*)fileString.c_str());
  std::error_code removalError;
  std::filesystem::remove(fileString, removalError);

  // Construct Markov Model Library
  pugi::xml_node markovModelLibraryElement = root.child("MarkovModelLibrary");
  string data = XMLTC(markovModelLibraryElement);
  std::stringstream ss(data);
  string countText;
  ss >> countText;
  std::istringstream countInput(countText);
  int size = 0;
  if (!(countInput >> size) || !countInput.eof() || size < 0) {
    throw CmodError(CmodError::Kind::Project,
                    "MarkovModelLibrary model count must be a nonnegative integer.",
                    "MarkovModelLibrary: " + countText,
                    "Set the model count to the number of saved Markov models, or resave the project in LASSIE.");
  }
  string modelText, line;
  getline(ss, line, '\n');
  for (int i = 0; i < size; i++) {
    if (!getline(ss, line, '\n')) {
      throw CmodError(CmodError::Kind::Project,
                      "Markov model " + to_string(i) + " is missing from the declared library.",
                      "MarkovModelLibrary -> model count " + to_string(size),
                      "Restore the missing model data, or correct the model count by resaving the library in LASSIE.");
    }
    std::istringstream stateCountInput(line);
    int stateCount = 0;
    if (!(stateCountInput >> stateCount) || stateCount < 0
        || (stateCountInput >> std::ws, !stateCountInput.eof())) {
      throw CmodError(CmodError::Kind::Project,
                      "Markov model " + to_string(i) + " has an invalid or missing state count.",
                      "MarkovModelLibrary -> model " + to_string(i),
                      "Restore the model's state count and data, or resave the model in LASSIE.");
    }
    modelText = line + '\n';
    const auto validateModelLine = [i](const string& text, unsigned long long expected,
                                      const string& field, bool probability) {
      std::istringstream values(text);
      for (unsigned long long j = 0; j < expected; ++j) {
        double value = 0;
        if (!(values >> value) || !std::isfinite(value) || (probability && value < 0)
            || (!probability && std::abs(value) > std::numeric_limits<float>::max())) {
          throw CmodError(CmodError::Kind::Project,
                          "Markov model " + to_string(i) + " has missing or invalid " + field
                              + " at entry " + to_string(j + 1) + ".",
                          "MarkovModelLibrary -> model " + to_string(i) + " -> " + field,
                          "Provide one finite value per state and a complete matrix of nonnegative transition probabilities in the Markov model editor.");
        }
      }
      values >> std::ws;
      if (!values.eof()) {
        throw CmodError(CmodError::Kind::Project,
                        "Markov model " + to_string(i) + " has extra or invalid data after its " + field + ".",
                        "MarkovModelLibrary -> model " + to_string(i) + " -> " + field,
                        "Make the model's state count, values, initial probabilities, and transition matrix dimensions agree.");
      }
    };
    line.clear();
    getline(ss, line, '\n');
    validateModelLine(line, stateCount, "state values", false);
    modelText += line + '\n';
    line.clear();
    getline(ss, line, '\n');
    validateModelLine(line, stateCount, "initial probabilities", true);
    modelText += line + '\n';
    line.clear();
    getline(ss, line, '\n');
    validateModelLine(line, static_cast<unsigned long long>(stateCount) * stateCount,
                      "transition probabilities", true);
    modelText += line;
    markovModelLibrary.emplace_back();
    markovModelLibrary[i].from_str(modelText);
    markovModelLibrary[i].normalize();
  }


  //events and other objects

  pugi::xml_node eventElements = root.child("Events");
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
  const auto lookup = [_type, &_eventName](
      const map<string, pugi::xml_node>& elements) {
    const auto it = elements.find(_eventName);
    if (it == elements.end()) {
      throw CmodError(CmodError::Kind::Project,
                      "Cannot find event or object '" + _eventName
                          + "' of type " + to_string(static_cast<int>(_type)) + ".",
                      "Event/object reference",
                      "Check the referenced name and type in LASSIE, or restore the missing object.");
    }
    return it->second;
  };

  switch((int)_type){
    case 0:
      return lookup(topEventElements);
    case 1:
      return lookup(highEventElements);
    case 2:
      return lookup(midEventElements);
    case 3:
      return lookup(lowEventElements);
    case 4:
      return lookup(bottomEventElements);
    case 5:
      return lookup(spectrumElements);
    case 6:
      return lookup(envelopeElements);
    case 7:
      return lookup(sieveElements);
    case 8:
      return lookup(spatializationElements);
    case 9:
      return lookup(patternElements);
    case 10:
      return lookup(reverbElements);
    case 12:
      return lookup(notesElements);
    case 13:
      return lookup(filterElements);
  }
  throw CmodError(CmodError::Kind::Project,
                  "Unsupported event/object type " + to_string(static_cast<int>(_type))
                      + " for '" + _eventName + "'.",
                  "Event/object reference",
                  "Use a supported event/object type and recreate the reference in LASSIE.");
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

  const auto errorContext = [this, _object, &_input]() {
    string context = "Expression: " + _input;
    if (_object != NULL && _object != piece) {
      context = "Event '" + static_cast<Event*>(_object)->getEventName()
              + "' -> " + context;
    }
    return context;
  };
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
    string evaluatedFunction;
    try {
      evaluatedFunction = evaluateFunction(functionString, _object);
    } catch (CmodError& error) {
      error.addContext(errorContext());
      throw;
    }
    string front = workingString.substr(0, locOfFun);
    string back = workingString.substr(((int)locOfEndFun) +6);
    workingString = front + evaluatedFunction + back;
// cout << "utilities string: " << workingString << endl;
    // look for next function
    locOfFun = workingString.find("<Fun>");
  }
  // evaluate the expression to the final result
  mu::Parser p;
  double result;

  try {
    p.SetExpr(workingString);
    result = p.Eval();
//cout << "utilities result: " << result << endl;
  } catch (const mu::ParserError& error) {
    throw CmodError(CmodError::Kind::Project,
                    "Cannot evaluate numeric expression: " + error.GetMsg(),
                    errorContext(),
                    "Check the expression's operators, parentheses, and function arguments in LASSIE.");
  }

  if (!std::isfinite(result)) {
    throw CmodError(CmodError::Kind::Project,
                    "Numeric expression produced a non-finite value.",
                    errorContext(),
                    "Check for division by zero and invalid function inputs; the result must be finite.");
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

  const bool needsEvent = functionName == "GetPattern"
      || functionName == "CURRENT_TYPE"
      || functionName == "CURRENT_CHILD_NUM"
      || functionName == "CURRENT_PARTIAL_NUM"
      || functionName == "AVAILABLE_EDU"
      || functionName == "CURRENT_LAYER"
      || functionName == "PREVIOUS_CHILD_DURATION";
  if (needsEvent && (_object == NULL || _object == piece)) {
    throw CmodError(CmodError::Kind::Project,
                    "Function '" + functionName + "' requires an event context.",
                    "Function: " + _functionString,
                    "Use this function in an event, or replace it with a constant in Project Properties.");
  }

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
  else if (functionName == "MakeEnvelope") {
    // Numeric fields sample the curve at the current event checkpoint,
    // exactly like Stochos FUNCTIONS with a single envelope. Object fields
    // still use getEnvelope() to obtain the entire curve.
    std::unique_ptr<Envelope> envelope(getEnvelope(_functionString, _object));
    const double checkpoint = (_object != NULL && _object != piece)
        ? static_cast<Event*>(_object)->getCheckPoint() : 0.0;
    resultString = to_string(envelope->getValue(static_cast<m_value_type>(checkpoint), 1.));
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
  else {
    throw CmodError(CmodError::Kind::Project,
                    "Unknown numeric function '" + functionName + "'.",
                    "Function: " + _functionString,
                    "Choose a supported numeric function in LASSIE and check its name.");
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
    return to_string(resultNum);
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
    return to_string(resultNum);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_NUM has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_PARTIAL_NUM(void* _object){
  if (_object !=NULL){
    double resultNum = static_cast<Event*>(_object)->getCurrPartialNum();
    return to_string(resultNum);
  }
  else {
    cerr<<"Utilities:Warning! static_function_CURRENT_PARTIAL_NUM has no object to look up."<<endl;
    return "0";
  }
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_DENSITY(void* _object){
  cerr << "CMOD warning: CURRENT_DENSITY is not implemented in this CMOD version; using legacy value 0.\n"
       << "Context: ";
  if (_object != NULL && _object != piece)
    cerr << "Event '" << static_cast<Event*>(_object)->getEventName() << "' -> ";
  cerr << "Function: CURRENT_DENSITY\n"
       << "Suggestion: Replace it with a constant or a supported expression for the desired density.\n";
  return "0";
}

//----------------------------------------------------------------------------//

string Utilities::static_function_CURRENT_SEGMENT(void* _object){
  cerr << "CMOD warning: CURRENT_SEGMENT is not implemented in this CMOD version; using legacy value 0.\n"
       << "Context: ";
  if (_object != NULL && _object != piece)
    cerr << "Event '" << static_cast<Event*>(_object)->getEventName() << "' -> ";
  cerr << "Function: CURRENT_SEGMENT\n"
       << "Suggestion: Replace it with a constant or a supported expression for the desired segment.\n";
  return "0";
}

//----------------------------------------------------------------------------//

string Utilities::static_function_AVAILABLE_EDU(void* _object){
  if (_object !=NULL){
    double resultNum = ((Event*)_object)->getAvailableEDU();
    return to_string(resultNum);
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
    return to_string(resultNum);
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
//  cout << "Utilities::static_function_PREVIOUS_CHILD_DURATION - resultNum=" 
//       << resultNum << endl;
    return to_string(resultNum);
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
  return to_string(resultNum);
}


//---------------------------------------------------------------------------//

string Utilities::function_Markov(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  const double modelIndex = evaluate(XMLTranscode(elementIter), _object);
  if (modelIndex < 0 || modelIndex >= markovModelLibrary.size()) {
    throw CmodError(CmodError::Kind::Project,
                    "Markov model index " + to_string(modelIndex)
                        + " is outside the library of " + to_string(markovModelLibrary.size()) + " models.",
                    "Function: Markov -> model index",
                    "Choose a saved Markov model in LASSIE. Model indices start at 0; add a model if the library is empty.");
  }
  const size_t entry = static_cast<size_t>(modelIndex);
  if (markovModelLibrary[entry].getStateSize() == 0) {
    throw CmodError(CmodError::Kind::Project,
                    "Markov model " + to_string(entry) + " has no states to sample.",
                    "Function: Markov -> model index",
                    "Add states and probabilities to this model, or select a populated model.");
  }

  float resultNum = markovModelLibrary[entry].nextSample(Random::Rand());
  return to_string(resultNum);
}

//----------------------------------------------------------------------------//

string Utilities::function_LN(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  double entry = evaluate(XMLTranscode(elementIter ),_object);

  double resultNum = ( 1. / pow(2.71828, entry) );
  return to_string(resultNum);
}

//----------------------------------------------------------------------------//

string Utilities::function_Fibonacci(pugi::xml_node _functionElement, void* _object){
  pugi::xml_node elementIter = GNES(GFEC(_functionElement));
  const double value = evaluate(XMLTranscode(elementIter), _object);
  if (value >= 47) {
    throw CmodError(CmodError::Kind::Project,
                    "Fibonacci entry " + to_string(value) + " exceeds the supported maximum of 46.",
                    "Function: Fibonacci -> Entry",
                    "Use an entry at most 46; larger Fibonacci values exceed CMOD's integer range.");
  }
  if (value <= 2) return "1";
  const int entry = static_cast<int>(value);

  int numA = 1;
  int numB = 1;
  for (int i = 3; i <= entry; i++) {
    int swap = numB;
    numB += numA;
    numA = swap;
  }
  int resultNum = numB;
  return to_string(resultNum);
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
  } else {
    throw CmodError(CmodError::Kind::Project,
                    "Decay type '" + type + "' is not supported.",
                    "Function: Decay -> Type",
                    "Choose EXPONENTIAL or LINEAR in the Decay function editor.");
  }

  return to_string(decay);
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
  if (method != "FUNCTIONS" && method != "RANGE_DISTRIB") {
    throw CmodError(CmodError::Kind::Project,
                    "Stochos method '" + method + "' is not supported.",
                    "Function: Stochos -> Method",
                    "Choose FUNCTIONS or RANGE_DISTRIB in the Stochos function editor.");
  }

  elementIter = GNES(elementIter);
  pugi::xml_node envElementIter = GFEC(elementIter);
  vector<std::unique_ptr<Envelope>> envVect;
  while (envElementIter!=NULL) {
    envVect.emplace_back((Envelope*)evaluateObject(XMLTC(envElementIter), _object, eventEnv));
    envElementIter = GNES(envElementIter);
  }
  if (envVect.empty()) {
    throw CmodError(CmodError::Kind::Project,
                    "Stochos has no envelopes to evaluate.",
                    "Function: Stochos -> Envelopes",
                    "Add at least one envelope for FUNCTIONS, or three envelopes per RANGE_DISTRIB group.");
  }

  elementIter = GNES(elementIter);
  const double offsetValue = evaluate(XMLTC(elementIter), _object);
  float returnVal = 0.0;

  if(method == "FUNCTIONS") {
    float randomNumber = 0.0f;

    // stacked up envelopes: their values at the same moment add up to 1
    for (int i = 0; i < (int)envVect.size(); i++) {
      returnVal = envVect[i]->getValue(static_cast<m_value_type>(checkpoint), 1.);
      if(envVect.size() > 1) {                      // probability areas
        if(i == 0) randomNumber = static_cast<float>(Random::Rand());
        if (returnVal >= randomNumber) {
          returnVal = static_cast<float>(i);
          break;
        }
      }
    }
  } else if (method == "RANGE_DISTRIB") {
    float limit[2];

    // distribution within given range; takes 3 envs: min, MAX, val in between
    if (offsetValue < 0 || offsetValue >= envVect.size() / 3) {
      throw CmodError(CmodError::Kind::Project,
                      "Stochos offset " + to_string(offsetValue)
                          + " does not select a complete group of 3 envelopes from the list of "
                          + to_string(envVect.size()) + " envelopes.",
                      "Function: Stochos -> RANGE_DISTRIB -> Offset",
                      "Use a nonnegative group offset (starting at 0) and supply minimum, maximum, and distribution envelopes for that group.");
    }
    const size_t offset = static_cast<size_t>(offsetValue);
    for(int i = 0; i < 2; i++) {
      limit[i] = envVect[3 * offset + i]->getValue(static_cast<m_value_type>(checkpoint), 1);
    }

    returnVal = envVect[3 * offset + 2]->getValue(static_cast<m_value_type>(Random::Rand()), 1);

    returnVal *= (limit[1] - limit[0]);
    returnVal += limit[0];
  }

  return to_string(returnVal);

}

//----------------------------------------------------------------------------//

string Utilities::function_ValuePick(pugi::xml_node _functionElement, void* _object){
  Sieve* si = sieve_ValuePick(_functionElement, _object);
  int resultNum = si->ChooseL();
  delete si;
  return to_string(resultNum);
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
  return to_string(resultNum);
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

  return to_string(resultNum);

}

//----------------------------------------------------------------------------//

string Utilities::function_Random(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node lowBoundElement = GNES(GFEC(_functionElement));
  pugi::xml_node highBoundElement = GNES(lowBoundElement);

  double lowBound = evaluate(requiredFunctionArgument(lowBoundElement, "Random", "Low"), _object);
  double highBound = evaluate(requiredFunctionArgument(highBoundElement, "Random", "High"), _object);

  return to_string(Random::Rand(lowBound, highBound));

}

//----------------------------------------------------------------------------//

static size_t checkedSelectIndex(double value, size_t listSize) {
  if (!std::isfinite(value) || value < 0 || value >= listSize) {
    throw CmodError(CmodError::Kind::Project,
                    "Select index " + to_string(value)
                        + " is outside the list of " + to_string(listSize) + " entries.",
                    "Function: Select",
                    "Use a nonnegative index smaller than the number of entries in the Select list.");
  }
  return static_cast<size_t>(value);
}

string Utilities::function_Select(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node listElement = GNES(GFEC(_functionElement));
  pugi::xml_node indexElement = GNES(listElement);
  if (XMLTranscode(listElement).find_first_not_of(" \t\r\n") == string::npos) {
    throw CmodError(CmodError::Kind::Project,
                    "Select cannot choose from an empty list.",
                    "Function: Select -> List",
                    "Add at least one value or object to the Select list.");
  }

  std::vector<std::string> list = listElementToStringVector(listElement);

  const size_t index = checkedSelectIndex(evaluate(requiredFunctionArgument(indexElement, "Select", "Index"), _object), list.size());
  return to_string(evaluate(list[index], _object));

}

//----------------------------------------------------------------------------//

string Utilities::function_SelectObject(pugi::xml_node _functionElement, void* _object){

  pugi::xml_node listElement = GNES(GFEC(_functionElement));
  pugi::xml_node indexElement = GNES(listElement);
  if (XMLTranscode(listElement).find_first_not_of(" \t\r\n") == string::npos) {
    throw CmodError(CmodError::Kind::Project,
                    "Select cannot choose from an empty list.",
                    "Function: Select -> List",
                    "Add at least one value or object to the Select list.");
  }
  std::vector<std::string> list = listElementToStringVector(listElement);
  const size_t index = checkedSelectIndex(evaluate(requiredFunctionArgument(indexElement, "Select", "Index"), _object), list.size());
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
  return to_string(returnValue);

}


//----------------------------------------------------------------------------/
string Utilities::function_RandomInt(pugi::xml_node _functionElement, void* _object){
  pugi::xml_node lowBoundElement = _functionElement.child("Low");
  pugi::xml_node highBoundElement = _functionElement.child("High");
  if (!lowBoundElement) lowBoundElement = _functionElement.child("LowBound");
  if (!highBoundElement) highBoundElement = _functionElement.child("HighBound");

  const int lowBound = checkedIntegerArgument(
      evaluate(requiredFunctionArgument(lowBoundElement, "RandomInt", "Low"), _object), "RandomInt", "Low");
  const int highBound = checkedIntegerArgument(
      evaluate(requiredFunctionArgument(highBoundElement, "RandomInt", "High"), _object), "RandomInt", "High");
  return to_string(Random::RandInt(lowBound, highBound));
}

//---------------------------------------------------------------------------//

string Utilities::function_RandomOrderInt(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node lowBoundElement = _functionElement.child("Low");
  pugi::xml_node highBoundElement = _functionElement.child("High");
  if (!lowBoundElement) lowBoundElement = _functionElement.child("LowBound");
  if (!highBoundElement) highBoundElement = _functionElement.child("HighBound");
  pugi::xml_node idElement = _functionElement.child("Id");

  const int lowBound = checkedIntegerArgument(
      evaluate(requiredFunctionArgument(lowBoundElement, "RandomOrderInt", "Low"), _object), "RandomOrderInt", "Low");
  const int highBound = checkedIntegerArgument(
      evaluate(requiredFunctionArgument(highBoundElement, "RandomOrderInt", "High"), _object), "RandomOrderInt", "High");
  const string idExpression = XMLTranscode(idElement);
  if (!idElement || idExpression.find_first_not_of(" \t\r\n") == string::npos) {
    throw CmodError(CmodError::Kind::Project,
                    "RandomOrderInt is missing its Id.",
                    "Function: RandomOrderInt -> Id",
                    "Open this RandomOrderInt in the LASSIE function editor, accept it, and save the project to assign an independent Id.");
  }
  const int id = checkedIntegerArgument(
      evaluate(idExpression, _object), "RandomOrderInt", "Id");
  
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

  return to_string(Random::RandOrderInt(lowBound, highBound, id));
}

//---------------------------------------------------------------------------//

string Utilities::function_RandomDensity(pugi::xml_node _functionElement, void* _object) {
  pugi::xml_node envelopeNumberElement = GNES(GFEC(_functionElement));
  pugi::xml_node lowBoundElement = GNES(envelopeNumberElement);
  pugi::xml_node highBoundElement = GNES(lowBoundElement);

  const int envelopeNumber = checkedEnvelopeNumber(
      evaluate(XMLTranscode(envelopeNumberElement), _object), envelopeLibrary->size(), "RandomDensity");
  double lowBound = evaluate(XMLTranscode(lowBoundElement), _object);
  double highBound = evaluate(XMLTranscode(highBoundElement), _object);

  ProbabilityEnvelope env(*(envelopeLibrary->getEnvelope(envelopeNumber)));
  // initialize the count table
  env.generateCountTable(1000);

  // sample from the count table
  double rand = Random::Rand(0, 1);
  double resultNumber = env.sample(rand) * (highBound - lowBound) + lowBound;
  // cout << "lowbound: " << lowBound << ", highbound: " << highBound << ", result: " << resultNumber << endl;

  return to_string(resultNumber);
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
    ObjectReferenceGuard reference(resolvingObjectReferences, k, "sieve", fileName);
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
  throw CmodError(CmodError::Kind::Project,
                  "Cannot construct a sieve from function '" + XMLTranscode(functionNameElement) + "'.",
                  "Sieve expression: " + XMLTranscode(_SIVFunction),
                  "Use MakeSieve, ReadSIVFile, or a Select list containing sieve functions.");
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
    ObjectReferenceGuard reference(resolvingObjectReferences, k, "pattern", fileName);

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
    int mod = static_cast<int>(evaluate(XMLTranscode(elementIter), _object));

    elementIter = GNES(elementIter);
    int low = static_cast<int>(evaluate(XMLTranscode(elementIter), _object));

    elementIter = GNES(elementIter);
    int high = static_cast<int>(evaluate(XMLTranscode(elementIter), _object));

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
  throw CmodError(CmodError::Kind::Project,
                  "Cannot construct a pattern from function '" + XMLTranscode(functionNameElement) + "'.",
                  "Pattern expression: " + XMLTranscode(_PATFunction),
                  "Use MakePattern, ExpandPattern, ReadPATFile, or a Select list containing pattern functions.");
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
    ObjectReferenceGuard reference(resolvingObjectReferences, k, "spatialization", fileName);

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
    ObjectReferenceGuard reference(resolvingObjectReferences, k, "reverb", fileName);

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
    ObjectReferenceGuard reference(resolvingObjectReferences, k, "filter", fileName);

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

  // pugi::xml_node is a non-owning handle. Keep the parsed document alive on
  // the Event, just like the SPA/REV/FIL evaluators do for temporary XML.
  ((Event*)_object)->addTemporaryXMLDocument(std::move(_docPtr));
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
    throw CmodError(CmodError::Kind::Project,
                    "Cannot construct an envelope from function '" + functionName + "'.",
                    "Envelope expression: " + _input,
                    "Use EnvLib, MakeEnvelope, ReadENVFile, or a Select list containing envelope functions.");
  }
return returnEnvelope;
}

//----------------------------------------------------------------------------//

Envelope* Utilities::envLib(pugi::xml_node _functionElement, void* _object){
//  <Env>3</Env>
//  <Scale>1.0</Scale>
  const int envelopeNumber = checkedEnvelopeNumber(
      evaluate(XMLTranscode(_functionElement), _object), envelopeLibrary->size(), "EnvLib");
  Envelope* env = envelopeLibrary->getEnvelope(envelopeNumber);
  //cout <<"EnvLib: #"<<envelopeNumber<<endl;
  double scale = evaluate(XMLTranscode(GNES(_functionElement)), _object);
  env->scale(static_cast<m_value_type>(scale));
  return env;

}

//----------------------------------------------------------------------------//

Envelope* Utilities::readEnvFile(pugi::xml_node _functionElement, void* _object){
  //<File>object name</File>

  pugi::xml_node file = getEventElement(eventEnv, XMLTranscode(_functionElement));
  ObjectReferenceGuard reference(resolvingObjectReferences, file, "envelope", XMLTranscode(_functionElement));

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

  const auto childCount = [](pugi::xml_node node) {
    size_t count = 0;
    for (; node; node = node.next_sibling()) {
      if (node.type() == pugi::node_element) ++count;
    }
    return count;
  };
  const size_t xCount = childCount(x);
  const size_t yCount = childCount(y);
  const size_t typeCount = childCount(t);
  const size_t propertyCount = childCount(p);
  if (xCount < 2 || xCount != yCount) {
    throw CmodError(CmodError::Kind::Project,
                    "MakeEnvelope has " + to_string(xCount) + " X points and "
                        + to_string(yCount) + " Y points.",
                    "Function: MakeEnvelope -> Xs/Ys",
                    "Provide at least two matching X/Y point pairs in the envelope editor.");
  }
  if (typeCount != xCount - 1 || propertyCount != xCount - 1) {
    throw CmodError(CmodError::Kind::Project,
                    "MakeEnvelope has " + to_string(xCount) + " points, "
                        + to_string(typeCount) + " segment interpolation types, and "
                        + to_string(propertyCount) + " segment length properties.",
                    "Function: MakeEnvelope -> Types/Pros",
                    "Provide one interpolation type and one FIXED/FLEXIBLE property for each segment between adjacent points.");
  }

  // create the collection of points
  vector<xy_point> points;


  float prevYVal = 0;
  float prevXVal = 0;
  while (x!=NULL && y!=NULL) {
    xy_point xy;
    xy.x = static_cast<m_value_type>(evaluate(XMLTranscode(x), _object));
    xy.y = static_cast<m_value_type>(evaluate(XMLTranscode(y), _object));

    if (xy.x - prevXVal < 0) { // flag to keep previous xval
      xy.x = static_cast<m_value_type>(prevXVal * 1.01);
    }
    if (xy.y < 0) { // flag to keep previous yval
      xy.y = prevYVal;
    }
    prevXVal = xy.x;
    prevYVal = xy.y;

    points.push_back(xy);
    x = GNES(x);
    y = GNES(y);

  }

  // create the collection of segments
  vector<envelope_segment> segments;
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
      throw CmodError(CmodError::Kind::Project,
                      "MakeEnvelope interpolation type '" + XMLTranscode(t) + "' is not supported.",
                      "Function: MakeEnvelope -> segment " + to_string(segments.size() + 1),
                      "Choose LINEAR, SPLINE, or EXPONENTIAL for this segment.");
    }

    if (XMLTranscode(p).compare("FIXED")==0) {
      seg.lengthType = FIXED;
    }
    else if (XMLTranscode(p).compare("FLEXIBLE")==0) {
      seg.lengthType = FLEXIBLE;
    }
    else {
      throw CmodError(CmodError::Kind::Project,
                      "MakeEnvelope length property '" + XMLTranscode(p) + "' is not supported.",
                      "Function: MakeEnvelope -> segment " + to_string(segments.size() + 1),
                      "Choose FIXED or FLEXIBLE for this segment's length property.");
    }

    segments.push_back(seg);

    t = GNES(t);
    p = GNES(p);
  }

  // Create a new envelope given the points and segments defined
  Envelope* madeEnv = new Envelope(points, segments);
  madeEnv->scale(static_cast<m_value_type>(scale));

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
    env->scale(static_cast<m_value_type>(scale));
    return env;
}
