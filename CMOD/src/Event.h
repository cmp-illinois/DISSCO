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
//  Event.h
//
//    This class defines a generic event.  It is used to manage all the
//    common things between types of events (top, high, low, bottom, etc.).
//
//----------------------------------------------------------------------------//

#ifndef EVENT_H
#define EVENT_H

#include "Libraries.h"
#include "Define.h"
#include "Matrix.h"
#include "Note.h"
#include "Rational.h"
#include "Tempo.h"
#include "TimeSpan.h"
#include "Utilities.h"
#include "Patter.h"


/**
 * @file Event.h
 * @brief Core Event class — the common base for every layer in a DISSCO
 *        composition tree (top, high, mid, low, bottom).
 *
 * Each Event holds the metadata that is shared regardless of layer:
 * timespan, tempo, pattern cache, the XML nodes for the sieve / spa /
 * reverb / filter / modifier attribute blocks, and the bookkeeping needed
 * to merge ancestor modifiers when expanding children. Layer-specific
 * behaviour lives in subclasses (e.g. @ref Bottom).
 */

/**
 * @brief A (key, pattern) cache entry kept on an Event.
 *
 * Patterns are looked up by the literal XML text that defined them; this
 * lets the Event reuse the same Patter instance when the same pattern is
 * referenced multiple times within a single event expansion.
 */
class PatternPair{

protected:
  string key;
  Patter* pattern;
public:
  PatternPair(string _key, Patter* _pat):key(_key),pattern(_pat){}
  ~PatternPair();
  string getKey(){return key;}
  Patter* getPattern (){return pattern;}

};


/**
 * @brief Per-child snapshot used while a Bottom event is building its notes.
 *
 * Bundles the XML element describing the child (either a spectrum entry, if
 * the bottom is a Sound, or a note entry), the resolved timespan, the
 * resolved tempo, and the child's display name / numeric type so the
 * Bottom's expansion routines can iterate without going back to the parser.
 */
class SoundAndNoteWrapper{
public:
  // this is the element of the spectrum if the bottom event is a sound
  // or of the note
  pugi::xml_node element;
  TimeSpan ts;
  string name;
  int type;
  Tempo tempo;

  SoundAndNoteWrapper(pugi::xml_node _element, TimeSpan _ts, string _name, int _type, Tempo _tempo ):
    element(_element),
    ts(_ts),
    name(_name),
    type(_type),
    tempo(_tempo){}
};


/**
 * @brief Generic event in a DISSCO composition tree.
 *
 * Event is the shared base for every hierarchical layer in a piece:
 * top, high, mid, low, and bottom. It carries the data common to all
 * layers (name, integer type, timespan, tempo, pattern cache) along with
 * the unresolved XML element handles for the attribute sub-blocks (attack
 * sieve, duration sieve, spatialization, reverberation, filter, modifiers).
 *
 * Modifier inheritance is handled here too: the
 * `modifiersIncludingAncestorsElement` is computed when an event is
 * expanded and is then passed to each child, so a Bottom event sees the
 * full chain of modifiers applied above it. The underlying XML document
 * that backs the merged tree is owned by `modifiersDoc` for the Event's
 * lifetime.
 *
 * Subclasses extend Event with layer-specific behaviour — most notably
 * @ref Bottom, which produces the actual Notes and Sounds.
 */
class Event {
public:
    //---------------------------- Information -------------------------------//

    //Name of the event
    string name;

    //Type of the event
    int type;

    //Timespan for the event
    TimeSpan ts;

    //Tempo of the event
    Tempo tempo;

    //stores the pattern needed for the Event and its XML representation
    vector<PatternPair*> patternStorage;

    //Utilities needs these
    pugi::xml_node AttackSieveElement;
    pugi::xml_node DurationSieveElement;

    pugi::xml_node spatializationElement;
    pugi::xml_node reverberationElement;
    pugi::xml_node filterElement;
    pugi::xml_node modifiersElement;

    //This Element is created by the Event, not the parser. It is passed to the
    // children and needs to be deleted once the event is done.
    pugi::xml_node modifiersIncludingAncestorsElement;
    // pugi handles are non-owning; the merged <Modifiers> tree lives in this
    // document, which the Event owns for its lifetime.
    std::unique_ptr<pugi::xml_document> modifiersDoc;

    // Storage for temporary parsers. For evaluating objects, XML parsers are
    // sometimes created by the utilities object. The event has the ownership
    // of these temporary parsers and is responsible to clean up.
    vector<std::unique_ptr<pugi::xml_document>> temporaryXMLDocuments;

protected:
    //------------------------------ Children --------------------------------//

    //File value to the event definitions of children


    //Children of the event
    vector<Event*> childEvents;

    //this is only used for bottom to stored its children
    vector<SoundAndNoteWrapper*> childSoundsAndNotes;
    //------------------------------ Building --------------------------------//

    //Child Event Def
    pugi::xml_node childEventDefElement;

    //Maximum allowed duration for a child
    float maxChildDur;

    //Percentage of child start time divided by total parent duration
    double checkPoint;

   //Previous start time for Sweep             //added by sever
    double previousChildEndTime;

    //Previous start time (only used by sweep generation method)
    TimeSpan tsPrevious;

    //Previous child duration		NOT USED
    //double previousChildDuration;

    // Whether to align note durations to a sieve. True for Bottoms 
    //  that contain Notes.
    bool sieveAligned;
    //-------------------------- Layers and Types ----------------------------//

    //Names of the layers
    vector< vector<string> > layerVect;
    vector<pugi::xml_node> layerElements;


    //Names of the children by type.
    vector<string> typeVect;
    vector<pugi::xml_node> childTypeElements; //discretepackage
    //Number of children to create (all layers)
    int numChildren;

    //Number of children to create for each layer
    vector<int> layerNumChildren;

    //Number of children yet to create (initially equal to layerNumChildren)
    vector<int> layerRemainingChildren;

    //Density of each layer.
    vector<float> layerDensity;

    //--------------------------- Restart Control ----------------------------//

    /*Putative child events are stored here. The children are not added
    immediately because the build process can fail and need to be restarted in
    the case of buildDiscrete.*/
    vector<Event*> temporaryChildEvents;

    //Number of restarts remaining.
    int restartsRemaining;
    static const int restartsNormallyAllowed = 6;
    static const int restartsAllowedWithFewerChildren = 10;

    ///Restarts the build process if necessary (for buildDiscrete).
    void tryToRestart(void);

    //---------------------------- Static Values -----------------------------//

    //Stores value for the getter to the static function CURRENT_CHILD_NUM
    int currChildNum;

    //Stores value for the getter to the static function CURRENT_CHILD_TYPE
    int childType;

    //--------------------------- Discrete Matrix ----------------------------//

    Matrix* matrix;

    //-------------------------- Used by build... ----------------------------//

    /*This stores the intermediate child timespan before it has been implemented
    an actual Event.*/
    TimeSpan tsChild;

    Utilities* utilities;
    pugi::xml_node childStartTimeElement ;
    pugi::xml_node childTypeElement;
    pugi::xml_node childDurationElement;
    pugi::xml_node methodFlagElement;
    pugi::xml_node childStartTypeFlag;
    pugi::xml_node childDurationTypeFlag;

    // This thing sorta works, but killing a thread waiting for cin causes
    // memory leak..   -- Ming-ching May 06, 2013
    std::thread discreteWaitForInputIfFailedThread;
    string discreteFailedResponse;


  public:


    /**
    * This is the constructor for new CMOD model
    *
    **/

    Event(pugi::xml_node _element,
          TimeSpan _timeSpan,
          int _type,
          Tempo _tempo,
          Utilities* _utilities,
          pugi::xml_node _ancestorSpa,
          pugi::xml_node _ancestorRev,
          pugi::xml_node _ancestorFil,
          pugi::xml_node _ancestorModifiers);


	/**
     * NEW FUNCTION 2_23_16 NEEDS TESTING
	 * Finds and prints (right now) all leaf events
	 * TO BE USED ONLY AFTER THE BUILD HAS COMPLETED.
	 */
    void findLeafChildren(vector<Event*> &leafChildren);

    /**
    *   buildChildren. Builds sub-events from parsed information and
    *   information already set for this event.
    *   Contains a loop whthin which objects are created one at a time.
    *   Each object has at least three parameters: start time, duration,
    *   and type.  They are selected using one of the following methods:
    *     CONTINUUM for continuous probability, non-sequential order
    *     DISCRETE  for discrete values, using a Matrix object, non-sequential
    *     SWEEP for reading values from a file provided by the user,
    *   sequential order
    *
    *  This function is overloaded in Bottom
    **/

    virtual void buildChildren();

    /**
    * Added by Rishabh. Experimental.
    **/

    virtual void modifyChildren();

    /**
    *	Event destructor.
    **/
    virtual ~Event();


    //------------- Used by FileValue static functions  ------------//
    /**
     *	Returns the events name
     **/
    string getEventName() {return name;};

    /**
    *	Returns the number of the child this event is currently building
    **/
    int getCurrentChild() {return currChildNum;};

    /**
     * Returns the number of children under this event
     */
    int getNumberOfChildren() { return numChildren; };

    /**
     *	Returns layer of the child this event is currently building
     **/
    int getCurrentLayer();

    /**
    *   Returns the current child type
    **/
    int getCurrentChildType() {return childType;};

    /**
    *   Returns the previous child duration
    *		NOT USED
    **/
    double getPreviousChildEndTime(){ return previousChildEndTime;}

    /**
    *  Returns the number of current partial -- will call to bottom in most cases
    **/
    virtual int getCurrPartialNum() {return 0;};

    /**
    *  Return this events duration in EDU
    **/
    int getAvailableEDU();

    /**
    *  Returns check point of the event
    *  \return check point of the event as a double
    **/
    double getCheckPoint() {return checkPoint;};


    /**
    * Checks the event to see if it was built successfully.
    **/
    void checkEvent(bool buildResult);

    /**
    *   Outputs information about the current subEvent.
    **/
    virtual void outputProperties();

    /**
    * Adds pointers to any notes in this Event (or any children) to a vector
    *
    * \param noteVect a reference to a vector of notes
    **/
    virtual list<Note> getNotes();

    /**
    * gives the ownership of the temporary XML parser to the event.
    **/
    void addTemporaryXMLDocument(std::unique_ptr<pugi::xml_document> _doc);

    /**
    * gives the ownership of the pattern to the event.
    **/
    void addPattern(std::string _string, Patter* _pat);

    /**
    * todo: incomplete function
    **/
    void setDiscreteFailedResponse(string _input)
      { discreteFailedResponse = _input;}

  //------------- Private helper functions  ------------//
  protected:

    /**
    *  Method for determining the EDU-wise exactness of the duration. Returns
    *  "Yes", "No", or "Almost". "Almost" occurs when the EDU duration is
    *  within one-percent of the actual duration. This occurs when floating-
    *  point math approximates the result, but does not exactly produce it.
    *  "Yes" can only occur when the result is exactly divisible. "No" occurs
    *  for all other case (i.e., when there are 3.2 EDU for the total duration).
    **/
    string getEDUDurationExactness(void);

    /**
    *  Method for assigning float values for stimeSec and duration using
    *  continuos (stochastic) distributions and int value for type -
    *  actually, a discrete value..
    *
    *   Follows aq slightly different procedure than Sweep and Discrete.  Why?
    *  \param iter FileValues to pass in for new objects
    **/
    bool buildContinuum();

    /**
     *  Method for assigning stimeSec and durSec values in sequential order -
     *  "sweeping" from left to right or beginning to end of the event.
     *  For stime and duration two different methods are used, one for integer
     *  values the other for float values.  Type being a discrete value, the
     *  integer values method is used for it.
     *  \param iter FileValues to pass in for new objects
     **/
    bool buildSweep();

    /**
    *   Wrapper for assigning values for stimeMatrix, type and durMatrix
    *   using a matrix.  Calls ObjCoordinates, Adjustments, and TimeConvert.
    *   \param iter FileValues to pass in for new objects
    **/
    virtual bool buildDiscrete();

    /**
    *  Converts "SECONDS" to "sec.", "PERCENTAGE" to "%", etc.
    **/
    string unitTypeToUnits(string type);

    /**
    *  helper functions
    **/
    string getTempoStringFromDOMElement(pugi::xml_node _element);

    string getTimeSignatureStringFromDOMElement(pugi::xml_node _element);

    void buildMatrix(bool discrete);
    
    /**
     * Checks if time is valid, if it is not, change it to closest
     *  valid value
     *  \param: int endTime time to be verified
     *  similar to a function in Note class
     **/
    int verify_valid(int endTime); 
};
#endif
