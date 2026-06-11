/*
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
//   modifier.h
//
//----------------------------------------------------------------------------//

#ifndef MODIFIER_H
#define MODIFIER_H

// CMOD includes
#include "Libraries.h"
#include "Define.h"

//----------------------------------------------------------------------------//

/**
 * @file Modifier.h
 * @brief Per-note synthesis modifier (tremolo, vibrato, bend, etc.).
 *
 * Modifiers are attached to Bottom events and reshape the rendered sound
 * after a Note has been chosen. Each Modifier carries a type tag, an
 * "apply how" flag (apply across the whole SOUND or to individual
 * PARTIALs), a probability envelope, and the parameter envelopes that
 * drive its behaviour (spread, direction, velocity, plus a queue of
 * per-step values). The user-facing list of modifier types is mirrored in
 * LASSIE's @ref ModifierType enum.
 */

/**
 * @brief One configured modifier instance attached to a Bottom event.
 *
 * Stateful — keeps a checkpoint for WAVE_TYPE modifiers and tracks which
 * partial index it last fired against when @c applyHow == PARTIAL.
 */
class Modifier {
  private:
    string type;      // TREMOLO, VIBRATO, BEND, etc
    string applyHow;  // SOUND or PARTIAL
    Envelope* probEnv;
    double spread;
    double direction;
    double velocity;
    deque<Envelope*> env_values; // values for the mod
    double checkPt;  // hold onto the checkpoint in case mod is WAVE_TYPE
    int partialNum;  // Partial num if applyHow==PARTIAL
    string partialResultString; // Partial result string for applying a modifier to multiple partials

  public:
   /**
    *  Default constructor for a Modifier.
    **/
    Modifier();

   /**
     *  Normal constructor for a Modifier.
     *  \param modType the type of modifier
     *  \param prob probability envelope
     *  \param modApplyHow apply to SOUND or PARTIAL
    **/
    Modifier(string modType, Envelope* prob, string modApplyHow, int modPartialNum=0);

   /**
     *  Copy constructor for a Modifier.
     *  \param orig a reference to the modifier to copy
    **/
    Modifier(const Modifier& orig);

   /**
     *  Assignment operator for a Modifier.
     *  \param rhs the modifier to copy
    **/
    Modifier& operator=(const Modifier& rhs);

   /**
     *  Destructor.
    **/
    ~Modifier();

   /**
     *  Add a value envelope to the Modifier.
     *  \param env pointer to the Envelope to add
    **/
    void addValueEnv( Envelope* env );
    
   /**
     *  Add detune spread to detune Modifier.
     *  \param detune spread to add
    **/
    void addSpread(double spread_);
  
   /**
     *  Add detune direction to detune Modifier.
     *  \param detune direction to add
    **/
    void addDirection(double dir_);

    /**
     *  Add detune velocity to detune Modifier.
     *  \param detune velocity to add
    **/
    void addVelocity(double vel);
  
   /**
     *  Get the probability (between 0 and 1) that this modifier will occur
     *  \param checkPoint the checkpoint (in time) of the probability
     *  \return a percentage value between 0 and 1
    **/
    float getProbability(double checkPoint);

    /**
     *  Get the name of the modifier
     *  \return the name/type of the modifier
     **/
    string getModName();

    /**
     *  Get the partial number of the modifier for applyHow == PARTIAL
     *  \return the name/type of the modifier
     **/
    int getPartialNum();

    string getPartialResultString();

   /**
     *  Checks whether a Modifier should occur, given a checkpoint
     *  \param checkPoint the checkpoint (in time) of the probability
     *  \return true if the Mod should occur, false otherwise
    **/
    bool willOccur(double checkPoint);

   /**
     *  Apply the modifier to a sound
     *  \param snd pointer to the sound to add this modifier to
     *  \param partNum the number of partials in the sound (only used if applyHow == "PARTIAL")
    **/
    void applyModifier(Sound* snd);

  private:
   /**
     *  Helper method to apply this mod to an entire sound
     *  \param snd pointer to the sound to add this modifier to
    **/
    void applyModSound(Sound* snd);

   /**
     *  Helper method to apply this mod to a single partial
     *  \param snd pointer to the sound to add this modifier to
     *  \param partNum the partial to apply the mod to
    **/
    void applyModPartial(Sound* snd);
};
  
#endif
  
