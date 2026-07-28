#ifndef MODIFIERUIPOLICY_HPP
#define MODIFIERUIPOLICY_HPP

namespace ModifierUiPolicy {

inline constexpr int fieldCount = 8;

// Fields: probability, magnitude, rate, width, spread, direction, velocity,
// partial-result string.
inline bool fieldEnabled(int modifierType, int field, bool applyByPartial)
{
    static constexpr bool fields[8][7] = {
        /* TREMOLO   */ { true, true,  true,  false, false, false, false },
        /* VIBRATO   */ { true, true,  true,  false, false, false, false },
        /* GLISSANDO */ { true, true,  false, false, false, false, false },
        /* DETUNE    */ { true, false, false, false, true,  true,  true  },
        /* AMPTRANS  */ { true, true,  true,  true,  false, false, false },
        /* FREQTRANS */ { true, true,  true,  true,  false, false, false },
        /* WAVE_TYPE */ { true, true,  true,  false, false, false, false },
        /* PHASE_MOD */ { true, true,  true,  false, false, false, false },
    };
    if (modifierType < 0 || modifierType >= 8 || field < 0 || field >= fieldCount)
        return false;
    // In PARTIAL mode, CMOD reads modifier envelopes from PartialResultString.
    // Keep only that field editable so the top-level SOUND fields do not look active.
    if (applyByPartial) {
        return field == 7;
    }
    // In SOUND mode, PartialResultString is not used.
    return field == 7 ? false : fields[modifierType][field];
}

} // namespace ModifierUiPolicy

#endif // MODIFIERUIPOLICY_HPP
