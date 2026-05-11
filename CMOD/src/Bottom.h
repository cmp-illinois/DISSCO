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
//   Bottom.h  This class is used to store and manage the details of the
//   bottom level objects.  Unlike the other events, bottom does not create
//   subevents.  In stead it creates sounds for synthesis and notes for
//   traditional notated scores are created.  Visual events might be added
//   later.
//
//----------------------------------------------------------------------------//
// all codes with the comment "multistaffs" are added by xiaoyi han
#ifndef BOTTOM_H
#define BOTTOM_H

// CMOD includes
#include "Libraries.h"
#include "Define.h"
#include "Modifier.h"
#include "Event.h"
#include "Note.h"
#include "TimeSpan.h"
#include "Tempo.h"
#include "Utilities.h"
#include "Random.h"
#include "Piece.h"
#include "Patter.h"
#include "ProbabilityEnvelope.h" // consider moving this into LASS.h
#include <string>
#include <stdlib.h>

//----------------------------------------------------------------------------//
class Bottom : public Event {

  private:

    /*DOMElements are held here because these variables need to be recomputed for
    every sound and note since there could be some randomness built in. */

    DOMElement* frequencyElement;
    DOMElement* loudnessElement;
    DOMElement* modifiersElement;
    /* ZIYUAN CHEN, July 2023 - The "Modifier Group" is only present
       in Bottom events, so this element doesn't appear in Event.h */
    DOMElement* modifierGroupElement;
    DOMElement* ancestorModifiersElement;

    //Current partial during the processing of the event
    int currPartialNum;


    /*Maps each mod name to an indicator if it's being used:
     -1 = initVal, 0 = notUsed, 1 = used.*/
    std::map<std::string, double> mod_used;

    //Pitch number for a well-tempered frequency (used to create notes)

    int wellTempPitch;		//(sever will change it to float) ???
//  float wellTempPitch;
    int absPitchNum;

    std::vector<Sound*> childSounds;
    std::vector<Note*> childNotes;

//----------------------------------------------------------------------------//

  public:

    /**
    * This is the constructor for new CMOD model
    *
    */

    Bottom(DOMElement* _element, TimeSpan _timeSpan, int _type, Tempo _tempo, Utilities* _utilities, DOMElement* _ancestorSpa, DOMElement* _ancestorRev,
          DOMElement* _ancestorFil,DOMElement* _ancestorModifiers);



    /**
    *   Destructor.
    **/
    ~Bottom();


    //--------------------- Build Methods  -----------------------//
    /**
     *  Starting point for building children. Overrides the usual
     *  method in Event, allowing the creation of Sounds, Notes,
     *  and Visuals instead of child Events.
     **/
    void constructChild(SoundAndNoteWrapper* _SoundNoteWrapper);

    /**
     *  The build method
     *
     **/
    void buildChildren();

    /**
    * Added by Rishabh. Experimental.
    **/

    void modifyChildren();


    /**
     *  Returns the number of current partial -- overrides Event
     **/
    int getCurrPartialNum() {return currPartialNum;};

    /**
     * Creates a sound and adds the sound to the Score.
     **/
    void buildSound(SoundAndNoteWrapper* _soundNoteWrapper);

    /**
     *  Creates a note (traditional notation) with all its attributes.
     **/
    void buildNote(SoundAndNoteWrapper* _soundNoteWrapper);
//  void buildNote(TimeSpan tsChild, int type, std::string name);

    /**
     *  Overloaded to prevent Event from printing.
     **/
    void print() {}

    /**
     *  Overloaded to prevent Event from printing.
     **/
    void printParticel() {}

    /**
     * Prints a note
     * \param n the note to print
     * \param type type of the note
     * \param name filename of the note
     **/
    void printNote(Note& n, int type, std::string name);

    /**
     * Prints a note
     * \param n the note to print
     * \param type type of the note
     * \param name filename of the note
     **/
    void printNoteParticel(Note& n, int type, std::string name);

    /**
     * Adds pointers to any notes in this Bottom event to a std::vector (LIST ?)
     * \param noteVect a reference to a std::vector of notes
    **/
    std::list<Note> getNotes();

    /**
     * building the notes / sounds. However in the new version building notes
     * is not implemented yet.
     **/
    void buildChildEvents();

    /**
     * construct each child.
     **/
    void constructChild(TimeSpan ts, int type, std::string name, Tempo tempo);

    //getters
    DOMElement* getSPAElement(){return spatializationElement;}
    DOMElement* getREVElement(){return reverberationElement;}
    DOMElement* getFILElement(){return filterElement;}



    //--------------------- Private helper methods  -----------------------//
  private:
    /**
     *  Computes a base frequency for the bottom event
     *    Continuum-Hertz takes a float value
     * 	  Continuum-power takes a float, the exponent of 2, rejects values
     *      outside the range MINFREQ-CEILING when C0 corresponds to 2**0
     *    Equal temperament takes an integer, the nth step from C0 and 
     *      computes the frequency in Hertz, a float value
     *    Fundamental takes 2 values: a fundamental frequency in Hertz and an
     *      integer, a partial number. NOTE: (higher) partials may have a 
     *      frequency diferent from that of corresponding Equal Temperament
     *      pitch.
     **/
    float computeBaseFreq();

    /**
     *  Computes a loudness value for the bottom event
     **/
    float computeLoudness();

    /**
     *  Computes the number of partials to create
     **/
    int computeNumPartials(float baseFreq, DOMElement* _spectrum);

    /**
     *  Computes a deviation value for the bottom event
     **/
    float computeDeviation( DOMElement* _spectrum);

    /**
     *   Assigns a frequency to a partial according to baseFrequency and the
     *   deviation (which is randomly selected as positive or negative).
     *   Frequencies are checked against the MINFREQ and CEILING
     *   (see define.h) and set (applied to the sound object) with setParam
     *   \param part reference to a partial
     *   \param deviation the deviation
     *   \param baseFrequency the fundamental frequency
     *   \param partNum the number of the partial
     **/
    float setPartialFreq(Partial& part, float deviation, float baseFrequency, int partNum);

    /**
     *  Chooses an envelope number for each partial then a scaling factor and
     *  then assigns the scaled envelope.  There are different methods for
     *  choosing the envelope and its scaling factor.
     * \param part reference to a partial
     * \param partNum the number of the partial
     **/
    void setPartialSpectrum(Partial& part, int partNum, DOMElement* _element);

    /**
     *  Given frequency, loudness, distance and envelope, add partials to the
     *  given sound
     * \param newsound pointer to a sound
     * \param frequency frequency of the sounds
     * \param loudness loudness at the destination of the sounds
     * \param envNum envelope's number in envelope library
     **/
    void generatePartials(Sound* newsound, float frequency, float loudness, float distance, Envelope* waveShape);

    /**
     *  Helper function for interpolating partials between spetrums of different
     *  frequencies where the curve is defined by 2^n. Retruns the amplitude at
     *  x given two data points.
     * \param x1 lower frequency
     * \param y2 amplitude of the partial at x1
     * \param x2 higher frequency
     * \param y2 amplitude of the partial at y2
     * \param x the frequency whose amplitude we are looking for
     **/
     double calculateFreqPartial(double x1, double y1, double x2, double y2, double x);

    /**
     *  Assigns values to the array of ampscales based on a randomly selected rule
     *  ==== not yet implemented ====
     * \param numPartials The number of partials
     * \param ampScale An array of floats for determining the amplitude scale
     **/
    void rules(int numPartials, float ampScale[]);

    /** ZIYUAN CHEN, July 2023
     *  Computes a Pan for applying spatializationStereo to a sound.
     *  \param envstr transcoded envelope std::string to be applied
     *  \return a Pan object parameterized by the envelope
     **/
    Pan computeSpatializationStereo(std::string envstr);

    /** ZIYUAN CHEN, July 2023
     *  Computes a MultiPan for applying spatializationMultiPan to a sound.
     *  \param mult a list of envelopes to be applied
     *  \return a MultiPan object associated with the designated partial
     **/
    MultiPan computeSpatializationMultiPan(std::vector<Envelope*> mult);

    /** ZIYUAN CHEN, July 2023
     *  Computes a MultiPan for applying spatializationPolar to a sound.
     *  \param thetaEnvStr transcoded "theta" envelope std::string to be applied
     *  \param radiusEnvStr transcoded "radius" envelope std::string to be applied
     *  \return a MultiPan object with 100 entry locations
     *     parameterized by theta and radius
     **/
    MultiPan computeSpatializationPolar(std::string thetaEnvStr, std::string radiusEnvStr);

    /** ZIYUAN CHEN, July 2023
     *  Computes a Reverb for applying reverberationSimple to a sound.
     *  \param sizeElement pointer to a DOMElement containing "Room Size" (float)
     *  \param iPartial index of the partial currently being processed
     *  \return pointer to a parameterized Reverb object
     **/
    Reverb* computeReverberationSimple(DOMElement* sizeElement, int iPartial);

    /** ZIYUAN CHEN, July 2023
     *  Computes a Reverb for applying ReverberationMedium to a sound.
     *  \param percentElement pointer to a DOMElement containing "Reverb Percentage" (envelope)
     *  \param spreadElement pointer to a DOMElement containing "Hilow Spread" (float)
     *  \param allPassElement pointer to a DOMElement containing "Gain All Pass" (float)
     *  \param delayElement pointer to a DOMElement containing "Delay" (float)
     *  \param iPartial index of the partial currently being processed
     *  \return pointer to a parameterized Reverb object
     **/
    Reverb* computeReverberationMedium(DOMElement* percentElement,
      DOMElement* spreadElement, DOMElement* allPassElement,
      DOMElement* delayElement, int iPartial);

    /** ZIYUAN CHEN, July 2023
     *  Computes a Reverb for applying ReverberationAdvanced to a sound.
     *  \param percentElement pointer to a DOMElement containing "Reverb Percentage" (envelope)
     *  \param combGainListElement pointer to a DOMElement containing "Comb Gain List" (6 floats)
     *  \param lpGainListElement pointer to a DOMElement containing "LP Gain List" (6 floats)
     *  \param allPassElement pointer to a DOMElement containing "Gain All Pass" (float)
     *  \param delayElement pointer to a DOMElement containing "Delay" (float)
     *  \param iPartial index of the partial currently being processed
     *  \return pointer to a parameterized Reverb object
     **/
    Reverb* computeReverberationAdvanced(DOMElement* percentElement,
      DOMElement* combGainListElement, DOMElement* lpGainListElement,
      DOMElement* allPassElement, DOMElement* delayElement, int iPartial);

//----------------------------------------------------------------------------//

    /**
     *  Applies filter to a sound
     *  \param s a pointer to the sound being created
     **/
    void applyFilter(Sound* s);

    /**
     *  Applies spatialization to a sound
     *  \param s a pointer to the sound being created
     *  \param numPartials the number of partials in the sound
     **/

    void applySpatialization(Sound* s, int numPartials);
    /**
     *  Sets the spatialization of a sound according to a stereo environment
     *  \param s pointer to a sound
     *  \param envList List of FileValues (envelopes)
     *  \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     *  \param numParts the number of partials in this sound
     **/
    void spatializationStereo(Sound *s,
                              DOMElement* _channels,
                              std::string applyHow,
                              int numParts);

    /**
     *  Sets the spatialization of a sound by assigning each speaker in an
     *     array its own envelope
     *  \param s pointer to a sound
     *  \param outerList List of Lists of FileValues (envelopes)
     *  \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     *  \param numParts the number of partials in this sound
     **/
    void spatializationMultiPan(Sound *s,
                                DOMElement* _channels,
                                std::string applyHow,
                                int numParts);

    /**
     *  Sets the spatialization of a sound assuming a speaker array
     *     forming a circle and uses polar coordinates to spread
     *     the sound between them. (see LASS).
     *  \param s pointer to a sound
     *  \param channels are actually theta and radius

     *  \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     *  \param numParts the number of partials in this sound
     **/
    void spatializationPolar(Sound *s,
                             DOMElement* _channels,
                             std::string applyHow,
                             int numParts);

    /**
     *  Applies reverberation to a sound
     *  \param s a pointer to the sound being created
     *  \param numPartials the number of partials contained in the sound
     **/
    void applyReverberation(Sound* s, int numPartials);

    /**
     *  Sets the reverberation of a sound according to a simple
     *     "room size" parameter
     *  \param s a pointer to the sound being created
     *  \param paramsElement pointer to the <Sizes> element
     *  \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     *  \param numPartials the number of partials contained in the sound
     **/
    void reverberationSimple(Sound *s,
                             DOMElement* paramsElement,
                             std::string applyHow,
                             int numPartials);

    /**
     *  Sets the reverberation of a sound according to an envelope,
     *     "hi/low spread", "gain all pass", and "delay" parameters
     *     ==== Applying by partial not implemented in LASSIE ====
     * \param s a pointer to the sound being created
     * \param paramsElement pointer to the <Percent> (envelope) element
     * \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     * \param numPartials the number of partials contained in the sound
     **/
    void reverberationMedium(Sound *s,
                             DOMElement* paramsElement,
                             std::string applyHow,
                             int numPartials);

    /**
     * Sets the reverberation of a sound according to an envelope,
     *     6 comb gain filters, 6 lp gain filters,
     *     "gain all pass", and "delay" parameters
     *     ==== Applying by partial not implemented in LASSIE ====
     * \param s a pointer to the sound being created
     * \param paramsElement pointer to the <Percent> (envelope) element
     * \param applyHow std::string containing info if it applies to sound or
     *     individual partials
     * \param numPartials the number of partials contained in the sound
     **/
    void reverberationAdvanced(Sound *s,
                               DOMElement* paramsElement,
                               std::string applyHow,
                               int numPartials);

    /**
    *  Use of modifiers: tremolo, vibrato, transients. Makes 3 lists/maps -
    *  one for modifiers with no dependencies, one for modifiers grouped
    *  together, and one for modifiers with direct dependencies on other
    *  modifiers. It goes through each list (in the order mentioned) to
    *  find which modifiers to use and their respective values and applies
    *  each of them.
    **/
    void applyModifiers(Sound *s, int numPartials);

    /**
     *  Apply modifiers for a note.
     **/
    //  std::vector<std::string> applyNoteModifiers();
    std::vector<std::string> applyNoteModifiers(DOMElement* _playingMethods);
    std::vector<std::string> applyNoteModifiersOld();
    // multistaffs
    /**
     *  Apply staff for a note.
     **/
    // int applyNoteStaffs(DOMElement* _playingMethods);

};
#endif
