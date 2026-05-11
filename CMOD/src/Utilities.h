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

/*
//
//  Utilities.h
//  Created by Ming-ching Chiu 2012-2013
//
//  The Utilities object is designed to evaluate the XML strings to their
//  proper format, whether an Event, a number, an Envelope or other objects such
//  as Pattern, Sieve, etc.
//
//  The Utilities is also the interface between the CMOD Event and LASS Score.
//  CMOD Events add the Sound objects they produce to LASS Score through
//  The Utilities.
//
//
*/


#ifndef UTILITIES_H
#define UTILITIES_H
#include "Libraries.h"
#include "Patter.h"
#define EQUAL_TEMP 0.5
#define FUNDAMENTAL 0.1
#define CONTINUUM 0.4
#define HZ 0.7
#define POW2 0.3

class Event;
class Piece;
class Patter;
class Sieve;
class Score;

using namespace xercesc; // for DOMElement

class Utilities{

public:

  /**
   * Constructor
   * \param _root
   * \param _workingPath
   * \param _soundSynthesis
   * \param _outputParticel
   * \param _numThreads The number of threads to use in multi-threaded rendering
   * \param _numChannels The number of channels to use
   * \param _samplingRate The sampling rate
   * \param _piece
   **/
  Utilities(DOMElement* _root,
            std::string _workingPath,
            bool _soundSynthesis,
            bool _outputParticel,
            int _numThreads,
            int _numChannels,
            int _samplingRate,
            Piece* _piece);

  /**
   * Destructor
   **/
  ~Utilities();


  //------------------------------ Evaluation --------------------------------//


  /**
   * Evaluate an XML std::string to a number.
   * An object (mostly an Event) must be provided to evaluate static functions
   * such as current_child_num (see evaluateFunction).
   * \param _input The XML std::string to evaluate
   * \param _object
   * \return The XML std::string evaluated as a number
   *
   * \\ TODO: Figure out what significance _object has
   */
  double evaluate(std::string _input, void* _object);

  Sieve* evaluateSieve(std::string _input, void* _object);

  /**
   * Returns whether the input DOMElement is a sieve function for the purposes
   * of aligning notes
   * \param input the element to consider
   * \return whether the element is a sieve function that can be aligned
   */
  static bool isSieveFunction(DOMElement* input);

  /**
   * Evaluate a function std::string to a CMOD object
   * The caller has to cast the returned pointer properly by itself.
   * \param _input The function std::string to evaluate
   * \param _object
   * \param _returnType The type of CMOD object that this function must return
   * \return The CMOD object this function returns
   *
   * \\ TODO: Figure out what significance _object has
   */
  void* evaluateObject(std::string _input, void* _object, EventType _returnType);

  /**
   * Evaluate a std::string to a DOMElement (which represents a CMOD Event)
   * \param _type The type of event this std::string represents
   * \param _eventName This event's name
   * \return The DOMElement the std::string represents
   */
  DOMElement* getEventElement(EventType _type, std::string _eventName);

  /**
   * Convert a DOMElement to a std::string
   * \param _thisFunctionElement The DOMElement to convert
   * \return A std::string representing the DOMElement
   */
  static std::string XMLTranscode(DOMElement* _thisFunctionElement);

  // --- Getters --- //

  /**
   * Gets the output particel
   * \return The output particel
   */
  bool getOutputParticel(){return outputParticel;}

  /**
   * Gets the number of channels
   * \return The number of channels
   */
  int getNumberOfChannels(){return numChannels;}

  /**
   * Gets the sampling rate
   * \return The sampling rate
   */
  int getSamplingRate(){return samplingRate;}

  /**
  * Gets the envelope shape in env_lib corresponding
  * to env_num and scale it
  * \return envelope pointer
  */
  Envelope* getEnvelopeshape(int env_num, double scale);

  //Name to Aesthetic value mapping. Experimental.

    std::map<std::string, double> eventValues;
    std::map<std::string, double> topEventValues;
    std::map<std::string, double> highEventValues;
    std::map<std::string, double> midEventValues;
    std::map<std::string, double> lowEventValues;
    std::map<std::string, double> bottomEventValues;

  int currChild;

  //Number to name mapping for modification. Experimental.

 std::map<int, std::string> topEventnames;
 std::map<int, std::string> highEventnames;
 std::map<int, std::string> midEventnames;
 std::map<int, std::string> lowEventnames;
 std::map<int, std::string> bottomEventnames;
 std::map<int, std::string> spectrumEventnames;
 std::map<int, std::string> envelopeEventnames;
 std::map<int, std::string> sieveEventnames;
 std::map<int, std::string> spatializationEventnames;
 std::map<int, std::string> patternEventnames;
 std::map<int, std::string> reverbEventnames;
 std::map<int, std::string> filterEventnames;
std::map<int, std::string> notesEventnames;


  //----------------------------- Other tasks --------------------------------//

  /**
   * Interface between CMOD Event and LASS Score
   * \param _sound
   *
   * This adds the _sound to the score iff _soundSynthesis == TRUE
   * Otherwise deletes _sound
   *
   * \\ TODO: Find difference between a Score with and without _soundSynthesis
   */
  void addSound(Sound* _sound);

  /**
   * Return the final product
   * \return The final rendered score. Returns NULL if there is no score.
   *
   * No score means _soundSynthesis == FALSE
   *
   * \\ TODO: Find difference between a Score with and without _soundSynthesis
   */
  MultiTrack* doneCMOD();

  /**
   * Removes the unnecessary space in a std::string
   * \param _originalString The std::string to clean upper
   * \return The _originalString without any spaces
   **/
  static std::string removeSpaces(std::string _originalString);

  /**
   * Helper function to convert a std::string, in the format of a list separated by
   * commas, to a std::vector containing each of those separate elements.
   * \param _listElement The list to convert
   * \return A std::vector containing the separate elements
   */
  static std::vector<std::string> listElementToStringVector(DOMElement* _listElement);


private:



  // Helper for getting Envelopes
  Envelope* getEnvelope(std::string _input, void* _object);
  Envelope* envLib(DOMElement* _functionElement, void* _object);
  Envelope* readEnvFile (DOMElement* _functionElement, void* _object);
  Envelope* makeEnvelope (DOMElement* _functionElement, void* _object);

  EnvelopeLibrary* envelopeLibrary;

  // --- Helper for getting Pattern --- //

  /**
   * Evaluates a function std::string to a Patter.
   * \param _functionString The function std::string to evaluate
   * \param _object
   * \return The Patter this _functionString represents
   *
   * \TODO: Figure out what _object is
   */
  Patter* getPattern(std::string _functionString, void* _object);

  /**
   * Converts a DOMElement to a Patter.
   * \param _object
   * \param _PATFunction The DOMElement representing a pattern function
   * \return The DOMElement as a Patter
   *
   * \TODO: Figure out what _object is
   * \TODO: Figure out if I'm right about how Patter's Expand() works
   */
  Patter* getPatternHelper(void* _object, DOMElement* _PATFunction);


  // --- Helper for getting Sieve --- //

  /**
   * Evaluates a function std::string to a Sieve.
   * \param _functionString The function std::string to evaluate
   * \param _object
   * \return The Sieve this _functionString represents
   *
   * \TODO: Figure out what _object is
   */
  Sieve* getSieve(std::string _functionString, void* _object);

  /**
   * Converts a DOMElement to a Sieve.
   * \param _object
   * \param _SIVFunction The DOMElement representing a sieve function
   * \return The DOMElement as a Sieve
   *
   * \TODO: Figure out what _object is
   */
  Sieve* getSieveHelper(void* _object, DOMElement* _SIVFunction);

  //Helper for getting SPA
  DOMElement* getSPAFunctionElement(void* _object);
  DOMElement* getSPAFunctionElementHelper(void* _object, DOMElement* _SPAFunction, bool _initialCall);

  // Helpers for getting REV
  DOMElement* getREVFunctionElement(void* _object);
  DOMElement* getREVFunctionElementHelper(void* _object, DOMElement* _REVFunction, bool _initialCall);

  // Helpers for getting FIL
  DOMElement* getFILFunctionElement(void* _object);
  DOMElement* getFILFunctionElementHelper(void* _object, DOMElement* _FILFunction, bool _initialCall);

  DOMElement* getSpectrum(std::string _functionString, void* _object);
  /**
   * Helper function for Utilities::evaluate() to evaluate CMOD functions
   **/
  std::string evaluateFunction(std::string _functionString, void* _object);

  Sieve* evaluateSieveFunction(std::string _functionString, void* _object);

  /**
   * Helper function for finding, in a std::string, the substring which represents
   * a CMOD function.
   * This function assume that at least 1 "<Fun>" exists in the std::string.
   * \param _input The std::string to search
   * \return The index of _input where the substring begins
   **/
  static size_t findTheEndOfFirstFunction(std::string _input);


  //--------------------------- CMOD Functions -------------------------------//

  /**
   *
   * \param _functionElement
   * \param _object
   * \return
   *
   * \TODO: Figure out what _functionElement is
   */
  std::string function_RandomInt(DOMElement* _functionElement, void* _object);
  std::string function_RandomOrderInt(DOMElement* _functionElement, void* _object);
  std::string function_Random(DOMElement* _functionElement, void* _object);
  std::string function_Select(DOMElement* _functionElement, void* _object);
  std::string function_SelectObject(DOMElement* _functionElement, void* _object);
  std::string function_GetPattern(DOMElement* _functionElement, void* _object);
  std::string function_Randomizer(DOMElement* _functionElement, void* _object);
  std::string function_Inverse(DOMElement* _functionElement, void* _object);
  std::string function_Markov(DOMElement* _functionElement, void* _object);
  std::string function_LN(DOMElement* _functionElement, void* _object);
  std::string function_RandomDensity(DOMElement* _functionElement, void* _object);

  /**
   * Not yet implemented.
   */
  std::string function_Fibonacci(DOMElement* _functionElement, void* _object);
  std::string function_MakeList(DOMElement* _functionElement, void* _object);
  
// I believe the following ARE implemented (sever)
  std::string function_Decay(DOMElement* _functionElement, void* _object);
  std::string function_Stochos(DOMElement* _functionElement, void* _object);
  std::string function_ValuePick(DOMElement* _functionElement, void* _object);
  std::string function_ChooseL(DOMElement* _functionElement, void* _object);
  Sieve* sieve_ValuePick(DOMElement* _functionElement, void* _object);
  Sieve* sieve_ChooseL(DOMElement* _functionElement, void* _object);
  //------------------------ CMOD Static Functions ---------------------------//

  /**
   * Returns an event's current child type.
   * \param _object The event to analyse
   * \return That event's current child type
   */
  std::string static_function_CURRENT_TYPE(void* _object);

  /**
   * Returns the event's current child number
   * \param _object The event to analyse
   * \return The number of children the event has
   */
  std::string static_function_CURRENT_CHILD_NUM(void* _object);

  /**
   * Returns the number of the current partial.
   * \param _object The event to analyse
   * \return The number of the event's current partial
   */
  std::string static_function_CURRENT_PARTIAL_NUM(void* _object);

  /**
   * Not yet implemented.
   */
  std::string static_function_CURRENT_DENSITY(void* _object);

  /**
   * Not yet implemented.
   */
  std::string static_function_CURRENT_SEGMENT(void* _object);

  /**
   * Returns an event's duration in EDU.
   * \param _object The event to analyse
   * \return The event's duration in EDU
   */
  std::string static_function_AVAILABLE_EDU(void* _object);

  /**
   * Returns the layer of an event's current child.
   * \param _object The event to analyse
   * \return The layer of an event's current child
   */
  std::string static_function_CURRENT_LAYER(void* _object);

  /**
   * Returns the previous child's duration
   * \param _object The event to analyse
   * \return The event's previous child's duration
   */
  std::string static_function_PREVIOUS_CHILD_DURATION(void* _object);




  //------------------------------- Fields -----------------------------------//

  //Storage of DOMElement representations of CMOD Events
  std::map<std::string,DOMElement*> topEventElements;
  std::map<std::string,DOMElement*> highEventElements;
  std::map<std::string,DOMElement*> midEventElements;
  std::map<std::string,DOMElement*> lowEventElements;
  std::map<std::string,DOMElement*> bottomEventElements;
  std::map<std::string,DOMElement*> spectrumElements;
  std::map<std::string,DOMElement*> envelopeElements;
  std::map<std::string,DOMElement*> sieveElements;
  std::map<std::string,DOMElement*> spatializationElements;
  std::map<std::string,DOMElement*> patternElements;
  std::map<std::string,DOMElement*> reverbElements;
  std::map<std::string,DOMElement*> filterElements;
  std::map<std::string,DOMElement*> notesElements;

  // Storage of LASS Parsed/generated Envelopes

  std::vector< MarkovModel<float> > markovModelLibrary;

  // Piece Configurations
  bool soundSynthesis = true;
  bool outputParticel = true;
  int numThreads;
  int numChannels;
  int samplingRate;
  Piece* piece;
  Score* score;

  // A flag to indicate that the CMOD computation is done.
  bool doneCreatingSoundObjects = false;

};

#endif
