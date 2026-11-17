#ifndef EVENTSTRUCT_HPP
#define EVENTSTRUCT_HPP

/* 
   event_struct.hpp defines the composite data types and enums corresponding to
   the Event attributes in the DISSCO Xml spec. To extend the DISSCO Xml spec as
   reflected in LASSIE: add to the corresponding struct, add to the parser, add
   to the UI files in the corresponding window, and add to the save function. 
*/

// When initializing defaults for unsigneds (and flags generally) preference should be given to the value that will correspond to the leftmost option (e.g., in a Radio of three elements from L-R, ABC, prefer A).

#include <QList>
#include <QString>
#include <QUuid>

#include <cmath>

typedef enum {
    none,
    dotted,
    doubledotted,
    triple,
    quintuple,
    sextuple,
    septuple
} Tempoprefix;

typedef enum {
    whole,
    half,
    quarter,
    eighth,
    sixteenth,
    thirtysecond
} Temponotevalue;

typedef enum {
    top,
    high,
    mid,
    low,
    bottom,
    sound, /* i.e., Spectrum */
    env,
    sieve,
    spa,
    pattern,
    reverb,
    folder,
    note,
    filter,
    mea,
    spec /* NOT ACTUALLY USED */
} Eventtype;

typedef struct TimeSignature TimeSignature;
struct TimeSignature {
    QString bar_value = "4";
    QString note_value = "4";
    bool operator==(const TimeSignature&) const = default;
};

typedef struct Tempo Tempo;
struct Tempo {
    unsigned method_flag = 0; // true = as fraction, false = as note value
    QString prefix; /* both */
    QString note_value = "2"; /* both */
    QString frentry_1; /* as fraction */
    QString frentry_2; /* as fraction */
    QString valentry = "60"; /* as note */
    bool operator==(const Tempo&) const = default;
};

/*
typedef enum {
    fixed,
    density,
    bylayer
} Numchildrenflag;
*/

typedef struct NumChildren NumChildren;
struct NumChildren {
    unsigned method_flag = 0; /* from Numchildrenflag */
    QString entry_1;
    QString entry_2;
    QString entry_3;
    bool operator==(const NumChildren&) const = default;
};

/*
typedef enum {
    continuumdef,
    sweep,
    discrete
} Childdefnflag;

typedef enum {
    percentage,
    units,
    seconds
} Childdeftimeflag;
*/

typedef struct ChildDef ChildDef;
struct ChildDef {
    QString entry_1; /* start time */
    QString entry_2; /* type */
    QString entry_3; /* duration */
    QString attack_sieve;
    QString duration_sieve;
    unsigned definition_flag = 0; /* from Childdefnflag */
    unsigned starttype_flag = 2; /* from Childdeftimeflag */
    unsigned durationtype_flag = 2; /* from Childdeftimeflag */
    bool operator==(const ChildDef&) const = default;
};

/// Convert an Eventtype enum value to a human-readable display string.
inline QString eventtypeToDisplayString(int type) {
    switch (type) {
        case top:     return "Top";
        case high:    return "High";
        case mid:     return "Mid";
        case low:     return "Low";
        case bottom:  return "Bottom";
        case sound:   return "Spectrum";
        case env:     return "Envelope";
        case sieve:   return "Sieve";
        case spa:     return "Spatialization";
        case pattern: return "Pattern";
        case reverb:  return "Reverb";
        case folder:  return "Folder";
        case note:    return "Note";
        case filter:  return "Filter";
        default:      return QString::number(type);
    }
}

/// Convert a display string (e.g. "Top", "High") to the integer enum value as a string.
inline QString displayStringToEventtypeString(const QString& s) {
    if (s == "Top")             return "0";
    if (s == "High")            return "1";
    if (s == "Mid")             return "2";
    if (s == "Low")             return "3";
    if (s == "Bottom")          return "4";
    if (s == "Spectrum")        return "5";
    if (s == "Envelope")        return "6";
    if (s == "Sieve")           return "7";
    if (s == "Spatialization")  return "8";
    if (s == "Pattern")         return "9";
    if (s == "Reverb")          return "10";
    if (s == "Folder")          return "11";
    if (s == "Note")            return "12";
    if (s == "Filter")          return "13";
    return s; // already an integer string or unknown — pass through
}

typedef struct Package Package;
struct Package {
    QString event_name;
    QString event_type;
    QString weight;
    QString attack_envelope;
    QString attackenvelope_scale;
    QString duration_envelope;
    QString durationenvelope_scale;
    bool operator==(const Package&) const = default;
};

/* oddly, there's a ByLayer tag, which to me, appears to never contain anything
 * -Jacob, 06/28/25 */
typedef struct Layer Layer;
struct Layer {
    QString by_layer;
    QList<Package> discrete_packages;
    bool operator==(const Layer&) const = default;
};

/*
typedef enum {
    welltempered,
    fundamental,
    continuum
} Freqinfofreqflag;

typedef enum {
    hertz,
    power_of_two
} Freqinfocontflag;
*/

typedef struct FreqInfo FreqInfo;
struct FreqInfo {
    unsigned freq_flag = 0; /* from Freqinfofreqflag */
    unsigned continuum_flag = 0; /* from Freqinfocontflag */
    QString entry_1;
    QString entry_2;
    bool operator==(const FreqInfo&) const = default;
};

enum class ModifierSamplingScope {
    PerSound,
    PerBottom
};

namespace ModifierUsageImportPolicy {

inline bool hasCompleteMetadata(const QString& stableId,
                                bool hasDefaultOnAttribute,
                                const QString& defaultOn,
                                bool hasDefaultOnEnvelope = false)
{
    if (hasDefaultOnEnvelope)
        return !stableId.trimmed().isEmpty();
    bool isNumber = false;
    const double chance = defaultOn.trimmed().toDouble(&isNumber);
    return !stableId.trimmed().isEmpty()
        && hasDefaultOnAttribute
        && isNumber
        && std::isfinite(chance)
        && chance >= 0.0
        && chance <= 1.0;
}

} // namespace ModifierUsageImportPolicy

struct ModifierCondition {
    QString modifier_id;
    bool required_on = true;
    bool operator==(const ModifierCondition&) const = default;
};

struct ModifierChanceRule {
    QList<ModifierCondition> conditions;
    QString on_chance;
    bool operator==(const ModifierChanceRule&) const = default;
};

typedef struct Modifier Modifier;
struct Modifier {
    // Stable identity for conditional rules. Copies intentionally retain it;
    // newly constructed modifiers receive a new identity.
    QString instance_id =
        QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString default_on_chance = QStringLiteral("1");
    // Optional envelope for Default ON. When set, it replaces
    // default_on_chance: CMOD reads it at each sound's (per sound) or the
    // Bottom's (per bottom) position in the event that contains the Bottom.
    QString default_on_envelope;
    QList<ModifierChanceRule> rules;
    // True only while importing a Modifier whose <Usage> metadata was absent
    // or incomplete.
    // The writer always emits Usage and deliberately never serializes this.
    bool usage_metadata_needs_review = false;

    unsigned type = 0;
    bool applyhow_flag = false; // false == SOUND, true == PARTIAL
    QString amplitude;
    QString rate;
    QString width;
    QString detune_spread;
    QString detune_direction;
    QString detune_velocity;
    QString partialresult_string;
    bool operator==(const Modifier&) const = default;
};

typedef struct ExtraInfo ExtraInfo;
struct ExtraInfo {
    unsigned childtype_flag = 0;
    FreqInfo freq_info;
    QString loudness;
    QString phase = "0"; // normalized carrier-phase cycle: 0.25 == 90 degrees
    QString spa;
    QString reverb;
    QString filter;
    QList<Modifier> modifiers;
    ModifierSamplingScope modifier_sampling_scope =
        ModifierSamplingScope::PerSound;
    // Transient import warning. It is true only while an older file without a
    // <ModifierUsage> marker is open and is deliberately never serialized.
    bool modifier_usage_needs_review = false;
    bool operator==(const ExtraInfo&) const = default;
};

/* HEvents are Top, High, Mid, or Low events */
typedef struct HEvent HEvent;
struct HEvent {
    QString orderinpalette;
    Eventtype type = Eventtype::top;
    QString name;
    QString max_child_duration;
    QString edu_perbeat = "60";
    TimeSignature timesig;
    Tempo tempo;
    NumChildren numchildren;
    ChildDef child_event_def;
    QList<Layer> event_layers; //
    QString spa;
    QString reverb;
    QString filter;
    QList<Modifier> modifiers;
    bool operator==(const HEvent&) const = default;
};

typedef struct BottomEvent BottomEvent;
struct BottomEvent {
    HEvent event;
    ExtraInfo extra_info;
    bool operator==(const BottomEvent&) const = default;
};

typedef struct Spectrum Spectrum;
struct Spectrum {
    QList<QString> partials = {""};
    bool operator==(const Spectrum&) const = default;
};

typedef struct SpectrumEvent SpectrumEvent;
struct SpectrumEvent {
    /* implicitly, if we did have an Eventtype variable, it would == 5. */
    QString orderinpalette;
    QString name;
    QString num_partials = "1";
    QString deviation;
    QString generate_spectrum;
    Spectrum spectrum;
    bool operator==(const SpectrumEvent&) const = default;
};

typedef struct NoteInfo NoteInfo;
struct NoteInfo {
    QString staffs;
    QList<QString> modifiers; /* not to be confused with struct Modifier! */
    bool operator==(const NoteInfo&) const = default;
};

typedef struct NoteEvent NoteEvent;
struct NoteEvent {
    QString orderinpalette;
    QString name;
    NoteInfo note_info;
    bool operator==(const NoteEvent&) const = default;
};

typedef struct EnvelopeEvent EnvelopeEvent;
struct EnvelopeEvent {
    /* eventtype == 6 */
    QString orderinpalette;
    QString name;
    QString envelope_builder;
    bool operator==(const EnvelopeEvent&) const = default;
};

typedef struct SieveEvent SieveEvent;
struct SieveEvent {
    QString orderinpalette;
    QString name;
    QString sieve_builder;
    bool operator==(const SieveEvent&) const = default;
};

typedef struct SpaEvent SpaEvent;
struct SpaEvent {
    QString orderinpalette;
    QString name;
    QString spatialization;
    bool operator==(const SpaEvent&) const = default;
};

typedef struct PatternEvent PatternEvent;
struct PatternEvent {
    QString orderinpalette;
    QString name;
    QString pattern_builder;
    bool operator==(const PatternEvent&) const = default;
};

typedef struct ReverbEvent ReverbEvent;
struct ReverbEvent {
    QString orderinpalette;
    QString name;
    QString reverberation;
    bool operator==(const ReverbEvent&) const = default;
};

typedef struct FilterEvent FilterEvent;
struct FilterEvent {
    QString orderinpalette;
    QString name;
    QString filter_builder;
    bool operator==(const FilterEvent&) const = default;
};

#endif
