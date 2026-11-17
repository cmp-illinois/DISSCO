#include "ModifierUsageQtAdapter.hpp"

#include "../dialogs/PartialModifierFormat.hpp"

#include <ModifierUsage.hpp>

#include <QObject>

#include <cmath>
#include <limits>
#include <utility>

namespace {

double parseProbability(const QString& text)
{
    bool valid = false;
    const double probability = text.trimmed().toDouble(&valid);
    return valid ? probability
                 : std::numeric_limits<double>::quiet_NaN();
}

std::string utf8(const QString& text)
{
    return text.toUtf8().toStdString();
}

bool hasConfiguredValue(const QString& value)
{
    const QString normalized = value.trimmed();
    return !normalized.isEmpty()
        && normalized.compare(QStringLiteral("N/A"), Qt::CaseInsensitive) != 0;
}

void validateEffectParameters(const Modifier& modifier,
                              int oneBasedPosition,
                              QStringList& diagnostics)
{
    const auto require = [&](const QString& value, const QString& field) {
        if (!hasConfiguredValue(value)) {
            diagnostics.append(
                QObject::tr("Modifier %1 is missing %2.")
                    .arg(oneBasedPosition)
                    .arg(field));
        }
    };
    const auto requireRange = [&](const QString& value,
                                  const QString& field,
                                  double minimum,
                                  double maximum) {
        if (!hasConfiguredValue(value)) {
            require(value, field);
            return;
        }
        bool valid = false;
        const double number = value.trimmed().toDouble(&valid);
        if (!valid || !std::isfinite(number)
            || number < minimum || number > maximum) {
            diagnostics.append(
                QObject::tr("Modifier %1: %2 must be between %3 and %4.")
                    .arg(oneBasedPosition)
                    .arg(field)
                    .arg(minimum)
                    .arg(maximum));
        }
    };
    const auto requireDirection = [&](const QString& value) {
        const QString field = QObject::tr("Detune Direction");
        if (!hasConfiguredValue(value)) {
            require(value, field);
            return;
        }
        bool valid = false;
        const double direction = value.trimmed().toDouble(&valid);
        if (!valid || !std::isfinite(direction) || direction == 0.0) {
            diagnostics.append(
                QObject::tr("Modifier %1: Detune Direction must be a "
                            "finite, non-zero number.")
                    .arg(oneBasedPosition));
        }
    };

    if (modifier.applyhow_flag) {
        const QString error = PartialModifierFormat::validationError(
            static_cast<int>(modifier.type),
            modifier.partialresult_string);
        if (!error.isEmpty()) {
            diagnostics.append(
                QObject::tr("Modifier %1: %2")
                    .arg(oneBasedPosition)
                    .arg(error));
        }
        return;
    }

    switch (modifier.type) {
    case 0: // Tremolo
    case 1: // Vibrato
    case 7: // Phase Modulation
    case 8: //Ring Modulation
        require(modifier.amplitude, QObject::tr("Magnitude"));
        require(modifier.rate, QObject::tr("Rate"));
        break;
    case 2: // Glissando
    case 6: // Wave Type
        require(modifier.amplitude, QObject::tr("Magnitude"));
        break;
    case 3: // Detune
        requireRange(modifier.detune_spread,
                     QObject::tr("Detune Spread"), 0.0, 1.0);
        requireDirection(modifier.detune_direction);
        requireRange(modifier.detune_velocity,
                     QObject::tr("Detune Velocity"), -1.0, 1.0);
        break;
    case 4: // Amplitude Transient
    case 5: // Frequency Transient
        require(modifier.amplitude, QObject::tr("Magnitude"));
        require(modifier.rate, QObject::tr("Rate"));
        require(modifier.width, QObject::tr("Width"));
        break;
    default:
        diagnostics.append(
            QObject::tr("Modifier %1 has an unknown type.")
                .arg(oneBasedPosition));
        break;
    }
}

} // namespace

ModifierUsageAnalysis analyzeModifierUsage(
    const QList<Modifier>& modifiers,
    ModifierSamplingScope scope)
{
    using namespace dissco::modifier_usage;

    ModifierUsageAnalysis analysis;
    Config config;
    config.scope = scope == ModifierSamplingScope::PerBottom
        ? SamplingScope::PerBottom
        : SamplingScope::PerSound;
    config.orderedModifiers.reserve(
        static_cast<std::size_t>(modifiers.size()));

    for (int modifierIndex = 0;
         modifierIndex < modifiers.size();
         ++modifierIndex) {
        const Modifier& modifier = modifiers[modifierIndex];
        validateEffectParameters(
            modifier, modifierIndex + 1, analysis.diagnostics);
        Entry entry;
        entry.id = utf8(modifier.instance_id);
        // An envelope's chance depends on time; validate the rest of the
        // configuration with a placeholder chance instead.
        const bool hasEnvelope =
            !modifier.default_on_envelope.trimmed().isEmpty();
        analysis.has_default_on_envelope =
            analysis.has_default_on_envelope || hasEnvelope;
        entry.defaultOnChance = hasEnvelope
            ? 1.0
            : parseProbability(modifier.default_on_chance);
        entry.rules.reserve(
            static_cast<std::size_t>(modifier.rules.size()));

        for (const ModifierChanceRule& sourceRule : modifier.rules) {
            Rule rule;
            rule.onChance = parseProbability(sourceRule.on_chance);
            rule.when.reserve(
                static_cast<std::size_t>(sourceRule.conditions.size()));
            for (const ModifierCondition& condition
                 : sourceRule.conditions) {
                rule.when.push_back(Predicate{
                    utf8(condition.modifier_id),
                    condition.required_on
                });
            }
            entry.rules.push_back(std::move(rule));
        }
        config.orderedModifiers.push_back(std::move(entry));
    }

    analysis.overall_usage_available =
        modifiers.size() <= modifierUsageExactPreviewLimit;
    CompileOptions options;
    options.overallUsageMode = analysis.overall_usage_available
        ? OverallUsageMode::Exact
        : OverallUsageMode::Skip;
    CompileResult compiled = compile(std::move(config), options);
    for (const Diagnostic& diagnostic : compiled.diagnostics)
        analysis.diagnostics.append(
            QString::fromStdString(diagnostic.message));

    if (!compiled.program)
        return analysis;

    const std::vector<OverallUsage>& overall =
        compiled.program->overallUsage();
    analysis.overall_on_chances.reserve(
        static_cast<qsizetype>(overall.size()));
    for (const OverallUsage& usage : overall)
        analysis.overall_on_chances.append(usage.chance);
    return analysis;
}
