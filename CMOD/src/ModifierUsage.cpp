#include "ModifierUsage.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace dissco::modifier_usage {
namespace {

struct CompiledPredicate {
    std::size_t modifierIndex = 0;
    bool requiredOn = false;
};

struct CompiledRule {
    std::vector<CompiledPredicate> when;
    double onChance = 0.0;
};

struct CompiledEntry {
    ModifierId id;
    double defaultOnChance = 1.0;
    std::vector<CompiledRule> rules;
};

bool validProbability(double probability)
{
    return std::isfinite(probability)
        && probability >= 0.0
        && probability <= 1.0;
}

void addDiagnostic(std::vector<Diagnostic>& diagnostics,
                   DiagnosticCode code,
                   const ModifierId& targetId,
                   std::string message)
{
    diagnostics.push_back(Diagnostic{code, targetId, std::move(message)});
}

// defaultOnChances, when given, replaces each entry's compiled Default ON.
double resolvedProbability(const std::vector<CompiledEntry>& entries,
                           std::size_t entryIndex,
                           const std::vector<bool>& decisions,
                           const std::vector<double>* defaultOnChances = nullptr)
{
    const CompiledEntry& entry = entries[entryIndex];
    for (const CompiledRule& rule : entry.rules) {
        const bool matches = std::all_of(
            rule.when.begin(), rule.when.end(),
            [&decisions](const CompiledPredicate& predicate) {
                return decisions[predicate.modifierIndex] == predicate.requiredOn;
            });
        if (matches)
            return rule.onChance;
    }
    return defaultOnChances ? (*defaultOnChances)[entryIndex]
                            : entry.defaultOnChance;
}

bool rulesCanOverlap(const CompiledRule& lhs, const CompiledRule& rhs)
{
    for (const CompiledPredicate& leftPredicate : lhs.when) {
        const auto right = std::find_if(
            rhs.when.begin(), rhs.when.end(),
            [&leftPredicate](const CompiledPredicate& candidate) {
                return candidate.modifierIndex == leftPredicate.modifierIndex;
            });
        if (right != rhs.when.end()
            && right->requiredOn != leftPredicate.requiredOn) {
            return false;
        }
    }
    return true;
}

void accumulateOverallUsage(const std::vector<CompiledEntry>& entries,
                            std::size_t entryIndex,
                            long double historyProbability,
                            std::vector<bool>& decisions,
                            std::vector<long double>& totals)
{
    if (entryIndex == entries.size() || historyProbability == 0.0L)
        return;

    const long double onChance = static_cast<long double>(
        resolvedProbability(entries, entryIndex, decisions));
    const long double onHistoryProbability = historyProbability * onChance;
    const long double offHistoryProbability =
        historyProbability * (1.0L - onChance);

    totals[entryIndex] += onHistoryProbability;

    if (onHistoryProbability != 0.0L) {
        decisions[entryIndex] = true;
        accumulateOverallUsage(entries, entryIndex + 1,
                               onHistoryProbability, decisions, totals);
    }
    if (offHistoryProbability != 0.0L) {
        decisions[entryIndex] = false;
        accumulateOverallUsage(entries, entryIndex + 1,
                               offHistoryProbability, decisions, totals);
    }
}

std::vector<OverallUsage> calculateOverallUsage(
    const std::vector<CompiledEntry>& entries)
{
    std::vector<bool> decisions(entries.size(), false);
    std::vector<long double> totals(entries.size(), 0.0L);
    accumulateOverallUsage(entries, 0, 1.0L, decisions, totals);

    std::vector<OverallUsage> result;
    result.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        result.push_back(
            OverallUsage{entries[i].id, static_cast<double>(totals[i])});
    }
    return result;
}

} // namespace

struct Program::Impl {
    SamplingScope scope = SamplingScope::PerSound;
    std::vector<CompiledEntry> entries;
    std::vector<OverallUsage> overall;
    std::optional<Selection> perBottomSelection;
};

Program::Program(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{
}

Program::Program(Program&&) noexcept = default;
Program& Program::operator=(Program&&) noexcept = default;
Program::~Program() = default;

Selection Program::select(const UnitRandom& nextUnit)
{
    return selectWith(nextUnit, nullptr);
}

Selection Program::select(const UnitRandom& nextUnit,
                          const std::vector<double>& defaultOnChances)
{
    if (m_impl && defaultOnChances.size() != m_impl->entries.size()) {
        throw std::invalid_argument(
            "ModifierUsage needs one Default ON chance per modifier.");
    }
    for (const double chance : defaultOnChances) {
        if (!validProbability(chance)) {
            throw std::domain_error(
                "ModifierUsage Default ON chances must be finite and between 0 and 1.");
        }
    }
    return selectWith(nextUnit, &defaultOnChances);
}

Selection Program::selectWith(const UnitRandom& nextUnit,
                              const std::vector<double>* defaultOnChances)
{
    if (!m_impl)
        throw std::logic_error("Cannot select with a moved-from ModifierUsage Program.");

    if (m_impl->scope == SamplingScope::PerBottom
        && m_impl->perBottomSelection.has_value()) {
        return *m_impl->perBottomSelection;
    }

    if (m_impl->entries.empty()) {
        Selection selection;
        if (m_impl->scope == SamplingScope::PerBottom)
            m_impl->perBottomSelection = selection;
        return selection;
    }

    if (!nextUnit)
        throw std::invalid_argument("ModifierUsage requires a random callback.");

    const double randomValue = nextUnit();
    if (!std::isfinite(randomValue)
        || randomValue < 0.0
        || randomValue >= 1.0) {
        throw std::domain_error(
            "ModifierUsage random callback must return a finite value in [0, 1).");
    }

    long double remainder = static_cast<long double>(randomValue);
    std::vector<bool> decisions(m_impl->entries.size(), false);
    Selection selection;
    selection.orderedOnIds.reserve(m_impl->entries.size());

    for (std::size_t i = 0; i < m_impl->entries.size(); ++i) {
        const long double probability = static_cast<long double>(
            resolvedProbability(m_impl->entries, i, decisions,
                                defaultOnChances));

        if (remainder < probability) {
            decisions[i] = true;
            selection.orderedOnIds.push_back(m_impl->entries[i].id);
            if (probability != 0.0L)
                remainder /= probability;
        } else {
            decisions[i] = false;
            if (probability != 1.0L)
                remainder = (remainder - probability) / (1.0L - probability);
        }

        // Exact interval arithmetic keeps the remainder in [0, 1). Floating
        // point division can round a value at the upper edge to exactly one.
        if (remainder >= 1.0L)
            remainder = std::nextafter(1.0L, 0.0L);
        else if (remainder < 0.0L)
            remainder = 0.0L;
    }

    if (m_impl->scope == SamplingScope::PerBottom)
        m_impl->perBottomSelection = selection;

    return selection;
}

const std::vector<OverallUsage>& Program::overallUsage() const noexcept
{
    static const std::vector<OverallUsage> empty;
    return m_impl ? m_impl->overall : empty;
}

CompileResult compile(Config config, CompileOptions options)
{
    CompileResult result;

    if (config.scope != SamplingScope::PerSound
        && config.scope != SamplingScope::PerBottom) {
        addDiagnostic(result.diagnostics,
                      DiagnosticCode::InvalidSamplingScope,
                      {},
                      "Sampling scope is not supported.");
    }

    std::unordered_map<ModifierId, std::size_t> indexById;
    indexById.reserve(config.orderedModifiers.size());
    for (std::size_t i = 0; i < config.orderedModifiers.size(); ++i) {
        const ModifierId& id = config.orderedModifiers[i].id;
        if (id.empty()) {
            addDiagnostic(result.diagnostics,
                          DiagnosticCode::EmptyId,
                          {},
                          "Modifier IDs must not be empty.");
            continue;
        }

        const auto [unused, inserted] = indexById.emplace(id, i);
        if (!inserted) {
            addDiagnostic(result.diagnostics,
                          DiagnosticCode::DuplicateId,
                          id,
                          "Modifier ID '" + id + "' is duplicated.");
        }
    }

    std::vector<CompiledEntry> compiledEntries;
    compiledEntries.reserve(config.orderedModifiers.size());

    for (std::size_t entryIndex = 0;
         entryIndex < config.orderedModifiers.size();
         ++entryIndex) {
        const Entry& sourceEntry = config.orderedModifiers[entryIndex];
        CompiledEntry compiledEntry;
        compiledEntry.id = sourceEntry.id;
        compiledEntry.defaultOnChance = sourceEntry.defaultOnChance;

        if (!validProbability(sourceEntry.defaultOnChance)) {
            addDiagnostic(
                result.diagnostics,
                DiagnosticCode::InvalidProbability,
                sourceEntry.id,
                "Default ON chance for modifier '" + sourceEntry.id
                    + "' must be finite and between 0 and 1.");
        }

        for (std::size_t ruleIndex = 0;
             ruleIndex < sourceEntry.rules.size();
             ++ruleIndex) {
            const Rule& sourceRule = sourceEntry.rules[ruleIndex];
            CompiledRule compiledRule;
            compiledRule.onChance = sourceRule.onChance;
            bool ruleIsValid = true;

            if (sourceRule.when.empty()) {
                addDiagnostic(
                    result.diagnostics,
                    DiagnosticCode::EmptyRule,
                    sourceEntry.id,
                    "An exception for modifier '" + sourceEntry.id
                        + "' must depend on at least one earlier modifier.");
                ruleIsValid = false;
            }

            if (!validProbability(sourceRule.onChance)) {
                addDiagnostic(
                    result.diagnostics,
                    DiagnosticCode::InvalidProbability,
                    sourceEntry.id,
                    "Exception ON chance for modifier '" + sourceEntry.id
                        + "' must be finite and between 0 and 1.");
                ruleIsValid = false;
            }

            std::unordered_map<std::size_t, bool> predicateByIndex;
            predicateByIndex.reserve(sourceRule.when.size());

            for (const Predicate& predicate : sourceRule.when) {
                const auto referenced = indexById.find(predicate.modifierId);
                if (referenced == indexById.end()) {
                    addDiagnostic(
                        result.diagnostics,
                        DiagnosticCode::MissingReference,
                        sourceEntry.id,
                        "Modifier '" + sourceEntry.id + "' references missing modifier '"
                            + predicate.modifierId + "'.");
                    ruleIsValid = false;
                    continue;
                }

                const std::size_t referencedIndex = referenced->second;
                if (referencedIndex == entryIndex) {
                    addDiagnostic(
                        result.diagnostics,
                        DiagnosticCode::SelfReference,
                        sourceEntry.id,
                        "Modifier '" + sourceEntry.id + "' cannot depend on itself.");
                    ruleIsValid = false;
                } else if (referencedIndex > entryIndex) {
                    addDiagnostic(
                        result.diagnostics,
                        DiagnosticCode::ForwardReference,
                        sourceEntry.id,
                        "Modifier '" + sourceEntry.id
                            + "' can reference only earlier modifiers.");
                    ruleIsValid = false;
                }

                const auto [existing, inserted] =
                    predicateByIndex.emplace(referencedIndex, predicate.requiredOn);
                if (!inserted) {
                    if (existing->second == predicate.requiredOn) {
                        addDiagnostic(
                            result.diagnostics,
                            DiagnosticCode::DuplicatePredicate,
                            sourceEntry.id,
                            "An exception for modifier '" + sourceEntry.id
                                + "' repeats a condition.");
                    } else {
                        addDiagnostic(
                            result.diagnostics,
                            DiagnosticCode::ConflictingPredicate,
                            sourceEntry.id,
                            "An exception for modifier '" + sourceEntry.id
                                + "' requires the same modifier to be both ON and OFF.");
                    }
                    ruleIsValid = false;
                }
            }

            if (ruleIsValid) {
                compiledRule.when.reserve(predicateByIndex.size());
                for (const auto& [modifierIndex, requiredOn] : predicateByIndex) {
                    compiledRule.when.push_back(
                        CompiledPredicate{modifierIndex, requiredOn});
                }
                std::sort(
                    compiledRule.when.begin(), compiledRule.when.end(),
                    [](const CompiledPredicate& lhs,
                       const CompiledPredicate& rhs) {
                        return lhs.modifierIndex < rhs.modifierIndex;
                    });
                compiledEntry.rules.push_back(std::move(compiledRule));
            }
        }

        for (std::size_t left = 0; left < compiledEntry.rules.size(); ++left) {
            for (std::size_t right = left + 1;
                 right < compiledEntry.rules.size();
                 ++right) {
                const CompiledRule& lhs = compiledEntry.rules[left];
                const CompiledRule& rhs = compiledEntry.rules[right];
                if (lhs.when.size() == rhs.when.size()
                    && rulesCanOverlap(lhs, rhs)) {
                    addDiagnostic(
                        result.diagnostics,
                        DiagnosticCode::AmbiguousRules,
                        sourceEntry.id,
                        "Modifier '" + sourceEntry.id
                            + "' has equal-specificity exceptions that can both match.");
                }
            }
        }

        // Rule declaration order is deliberately non-semantic.
        std::stable_sort(
            compiledEntry.rules.begin(), compiledEntry.rules.end(),
            [](const CompiledRule& lhs, const CompiledRule& rhs) {
                return lhs.when.size() > rhs.when.size();
            });

        compiledEntries.push_back(std::move(compiledEntry));
    }

    if (!result.diagnostics.empty())
        return result;

    auto impl = std::make_unique<Program::Impl>();
    impl->scope = config.scope;
    impl->entries = std::move(compiledEntries);
    if (options.overallUsageMode == OverallUsageMode::Exact)
        impl->overall = calculateOverallUsage(impl->entries);
    result.program.emplace(Program(std::move(impl)));
    return result;
}

} // namespace dissco::modifier_usage
