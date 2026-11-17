#ifndef MODIFIERUSAGEQTADAPTER_HPP
#define MODIFIERUSAGEQTADAPTER_HPP

#include "event_struct.hpp"

#include <QStringList>
#include <QVector>

inline constexpr int modifierUsageExactPreviewLimit = 12;

struct ModifierUsageAnalysis {
    QVector<double> overall_on_chances;
    QStringList diagnostics;
    bool overall_usage_available = true;
    // True when a Default ON uses an envelope, so its chance changes over
    // time and overall_on_chances are not meaningful.
    bool has_default_on_envelope = false;

    bool isValid() const { return diagnostics.isEmpty(); }
};

/**
 * Validates a LASSIE modifier list and, for small lists, calculates each exact
 * marginal ON rate. Exact preview is deliberately bounded because arbitrary
 * conditional rules require exponential enumeration; synthesis never does.
 *
 * Runtime sampling stays in the shared, Qt-free ModifierUsage module. This
 * adapter is deliberately narrow so the editor preview cannot acquire its own
 * subtly different probability semantics.
 */
ModifierUsageAnalysis analyzeModifierUsage(
    const QList<Modifier>& modifiers,
    ModifierSamplingScope scope);

#endif // MODIFIERUSAGEQTADAPTER_HPP
