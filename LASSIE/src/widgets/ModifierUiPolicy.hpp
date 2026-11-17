#ifndef MODIFIERUIPOLICY_HPP
#define MODIFIERUIPOLICY_HPP

#include "../core/event_struct.hpp"

#include <QString>
#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ModifierUiPolicy {

inline constexpr int fieldCount = 7;
inline constexpr int generatedSpectrumPartialCount = 20;

struct PartialRowConstraint {
    int suggestedRows = 1;
    // Zero means that the spectrum size is evaluated only at CMOD runtime.
    int maximumRows = 0;
    QString explanation;
};

struct SpectrumPartialCount {
    int count = 1;
    bool exact = false;
    bool generated = false;
};

inline bool staticIntegerValue(const QString& source, int* result = nullptr)
{
    const QString value = source.trimmed();
    bool validInteger = false;
    const int integer = value.toInt(&validInteger);
    if (validInteger) {
        if (result)
            *result = integer;
        return true;
    }

    bool validNumber = false;
    const double number = value.toDouble(&validNumber);
    if (!validNumber
        || !std::isfinite(number)
        || std::floor(number) != number
        || number < std::numeric_limits<int>::min()
        || number > std::numeric_limits<int>::max()) {
        return false;
    }
    if (result)
        *result = static_cast<int>(number);
    return true;
}

inline QString partialCountAfterExplicitListChange(
    const QString& configuredCount, int explicitRowCount)
{
    return staticIntegerValue(configuredCount)
        ? QString::number(explicitRowCount)
        : configuredCount;
}

inline bool usageSummaryVisible(Eventtype eventType)
{
    switch (eventType) {
    case top:
    case high:
    case mid:
    case low:
    case bottom:
        return true;
    default:
        return false;
    }
}

// Sampling scope only matters once a Bottom has at least one modifier.
inline bool samplingScopeVisible(Eventtype eventType, int modifierCount)
{
    return eventType == bottom && modifierCount > 0;
}

inline QString displayName(int modifierType)
{
    switch (modifierType) {
    case 0: return QStringLiteral("Tremolo");
    case 1: return QStringLiteral("Vibrato");
    case 2: return QStringLiteral("Glissando");
    case 3: return QStringLiteral("Detune");
    case 4: return QStringLiteral("Amplitude Transient");
    case 5: return QStringLiteral("Frequency Transient");
    case 6: return QStringLiteral("Wave Type");
    case 7: return QStringLiteral("Phase Modulation");
    case 8: return QStringLiteral("Ring Modulation");
    default: return QStringLiteral("Unknown Modifier");
    }
}

inline SpectrumPartialCount spectrumPartialCount(const SpectrumEvent& spectrum)
{
    QXmlStreamReader generatedReader(
        QStringLiteral("<root>") + spectrum.generate_spectrum
        + QStringLiteral("</root>"));
    bool hasGeneratedElement = false;
    bool sawWrapperElement = false;
    while (!generatedReader.atEnd()) {
        generatedReader.readNext();
        if (!generatedReader.isStartElement())
            continue;
        if (!sawWrapperElement) {
            sawWrapperElement = true;
            continue;
        }
        hasGeneratedElement = true;
    }
    if (!generatedReader.hasError() && hasGeneratedElement)
        return {generatedSpectrumPartialCount, true, true};

    const int listedCount = std::max(
        1, static_cast<int>(spectrum.spectrum.partials.size()));

    int declaredCount = 0;
    if (staticIntegerValue(spectrum.num_partials, &declaredCount)
        && declaredCount > 0) {
        return {declaredCount, true, false};
    }

    // A function-valued NumberOfPartials is evaluated by CMOD.  The explicit
    // list is still the best editor suggestion, but it is not a safe limit.
    return {listedCount, false, false};
}

inline int editorRowMaximum(const PartialRowConstraint& constraint,
                            int savedRows)
{
    const int preservedRows = std::max(1, savedRows);
    if (constraint.maximumRows > 0)
        return std::max(constraint.maximumRows, preservedRows);
    // Runtime-valued NumberOfPartials has no truthful finite UI cap. The row
    // count control is read-only and grows one row per Add action, so using
    // QSpinBox's full integer range does not trigger a bulk allocation.
    return std::numeric_limits<int>::max();
}

inline bool rowCountAllowed(const PartialRowConstraint& constraint,
                            int rows,
                            int grandfatheredRows = 0)
{
    const int effectiveMaximum = std::max(
        constraint.maximumRows, grandfatheredRows);
    return rows >= 1
        && (constraint.maximumRows <= 0 || rows <= effectiveMaximum);
}

// Fields: magnitude, rate, width, spread, direction, velocity,
// partial-result string.
inline bool soundFieldApplies(int modifierType, int field)
{
    static constexpr bool fields[9][7] = {
        /* TREMOLO   */ { true,  true,  false, false, false, false, false },
        /* VIBRATO   */ { true,  true,  false, false, false, false, false },
        /* GLISSANDO */ { true,  false, false, false, false, false, false },
        /* DETUNE    */ { false, false, false, true,  true,  true,  false },
        /* AMPTRANS  */ { true,  true,  true,  false, false, false, false },
        /* FREQTRANS */ { true,  true,  true,  false, false, false, false },
        /* WAVE_TYPE */ { true,  false, false, false, false, false, false },
        /* PHASE_MOD */ { true,  true,  false, false, false, false, false },
        /* RING_MOD  */ { true,  true,  false, false, false, false, false }
    };
    if (modifierType < 0 || modifierType >= 9 || field < 0 || field >= fieldCount)
        return false;
    return field < 6 && fields[modifierType][field];
}

inline bool fieldVisible(int modifierType, int field, bool applyByPartial)
{
    if (modifierType < 0 || modifierType >= 9 || field < 0 || field >= fieldCount)
        return false;
    if (field == 6)
        return applyByPartial;
    return soundFieldApplies(modifierType, field);
}

inline bool fieldEnabled(int modifierType, int field, bool applyByPartial)
{
    if (!fieldVisible(modifierType, field, applyByPartial))
        return false;
    // In PARTIAL mode CMOD reads effect parameters exclusively from
    // PartialResultString.  SOUND fields remain visible for orientation but
    // are disabled so they cannot appear to affect the per-partial result.
    return applyByPartial ? field == 6 : field != 6;
}

} // namespace ModifierUiPolicy

#endif // MODIFIERUIPOLICY_HPP
