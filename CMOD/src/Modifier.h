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

class Modifier {
  private:
    std::string type;      // TREMOLO, VIBRATO, BEND, etc
    std::string applyHow;  // SOUND or PARTIAL
    Envelope* probEnv;
    double spread;
    double direction;
    double velocity;
    deque<Envelope*> env_values; // values for the mod
    double checkPt;  // hold onto the checkpoint in case mod is WAVE_TYPE
    int partialNum;  // Partial num if applyHow==PARTIAL
    std::string partialResultString; // Partial result std::string for applying a modifier to multiple partials

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
    Modifier(std::string modType, Envelope* prob, std::string modApplyHow, int modPartialNum=0);

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
    std::string getModName();

    /**
     *  Get the partial number of the modifier for applyHow == PARTIAL
     *  \return the name/type of the modifier
     **/
    int getPartialNum();

    std::string getPartialResultString();

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
  
