// Tests for the modifier usage rules (ModifierUsage.hpp): which modifiers
// (vibrato, tremolo, ...) each sound gets, and how often. Bottom uses these
// rules for every sound CMOD makes.
//
// Written by Diyun (DiyunZ) with the modifier usage code ("Implement
// conditional modifier usage", 2026-07-31). It was dropped when that code
// moved into CMOD (3be7e4b1) and is restored here. Tony Chai added the
// harness (each failure is named and doesn't stop the others) and the last
// three tests: negative predicates, OverallUsageMode::Skip, and rule order.
// -- Tony Chai, 2026

#include "ModifierUsage.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace dissco::modifier_usage;

namespace {

// Test harness: every failure names the test it came from, and a failed
// check doesn't stop the other checks or tests -- only require() stops its
// own test, for checks the rest of that test depends on (e.g. "the config
// compiled"). -- Tony Chai, 2026
std::string currentTest;
int currentTestFailures = 0;

// Thrown by require() to end just the current test.
struct RequirementFailed : std::runtime_error {
    using std::runtime_error::runtime_error;
};

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED [" << currentTest << "]: " << message << '\n';
        ++currentTestFailures;
    }
}

// Like expect(), but stops the current test if it fails, because the
// following code would be invalid (e.g. using a program that didn't compile).
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw RequirementFailed(message);
}

void expectNear(double actual, double expected, const std::string& message)
{
    constexpr double tolerance = 1.0e-12;
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED [" << currentTest << "]: " << message
                  << ": expected " << expected << ", got " << actual << '\n';
        ++currentTestFailures;
    }
}

bool hasDiagnostic(const CompileResult& result, DiagnosticCode code)
{
    for (const Diagnostic& diagnostic : result.diagnostics) {
        if (diagnostic.code == code)
            return true;
    }
    return false;
}

void expectInvalid(Config config,
                   DiagnosticCode expectedCode,
                   const std::string& message)
{
    CompileResult result = compile(std::move(config));
    expect(!result.program.has_value(), message + " unexpectedly compiled");
    expect(hasDiagnostic(result, expectedCode),
           message + " did not report the expected diagnostic");
}

// Shared fixture: a worked example with one unconditional and two
// conditional modifiers, reused by testProfessorExample() below.
Config professorExample(SamplingScope scope = SamplingScope::PerSound)
{
    Config config;
    config.scope = scope;
    config.orderedModifiers = {
        Entry{"V", 0.60, {}},
        Entry{"T", 0.45, {
            Rule{{Predicate{"V", true}}, 0.30}
        }},
        Entry{"P", 0.15, {
            Rule{{Predicate{"V", true}, Predicate{"T", true}}, 0.25},
            Rule{{Predicate{"V", true}, Predicate{"T", false}}, 0.20},
            Rule{{Predicate{"V", false}, Predicate{"T", true}}, 0.65}
        }}
    };
    return config;
}

// Checks the worked example's overall-usage math and that a single
// selection draws exactly one random value.
void testProfessorExample()
{
    CompileResult result = compile(professorExample());
    require(result.program.has_value(), "Professor example failed to compile");
    expect(result.diagnostics.empty(), "Professor example produced diagnostics");

    Program program = std::move(*result.program);
    const std::vector<OverallUsage>& overall = program.overallUsage();
    require(overall.size() == 3, "Professor example usage count is wrong");
    expect(overall[0].id == "V", "V usage is out of order");
    expect(overall[1].id == "T", "T usage is out of order");
    expect(overall[2].id == "P", "P usage is out of order");
    expectNear(overall[0].chance, 0.600, "V overall usage is wrong");
    expectNear(overall[1].chance, 0.360, "T overall usage is wrong");
    expectNear(overall[2].chance, 0.279, "P overall usage is wrong");

    int randomCalls = 0;
    const Selection selected = program.select([&randomCalls] {
        ++randomCalls;
        return 0.50;
    });
    expect(randomCalls == 1, "Direct sampling did not use exactly one random value");
    expect(selected.orderedOnIds == std::vector<ModifierId>{"V"},
           "r=0.50 should select only Vibrato");
}

// p=0.0 and p=1.0 should be hard always-off/always-on, regardless of the
// random draw, and should report as exactly 0.0/1.0 overall usage.
void testZeroAndOneProbabilities()
{
    Config config;
    config.orderedModifiers = {
        Entry{"never", 0.0, {}},
        Entry{"always", 1.0, {}}
    };

    CompileResult result = compile(std::move(config));
    require(result.program.has_value(), "0/1 probability config failed to compile");
    Program program = std::move(*result.program);
    const Selection selected = program.select([] { return 0.999999; });
    expect(selected.orderedOnIds == std::vector<ModifierId>{"always"},
           "p=0/p=1 decisions are wrong");
    expectNear(program.overallUsage()[0].chance, 0.0, "p=0 overall usage is wrong");
    expectNear(program.overallUsage()[1].chance, 1.0, "p=1 overall usage is wrong");
}

// PerSound should draw a fresh random value on every selection; PerBottom
// should draw once and reuse that cached decision on later selections.
void testSamplingScopeRandomCalls()
{
    Config perSound;
    perSound.scope = SamplingScope::PerSound;
    perSound.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult soundResult = compile(std::move(perSound));
    require(soundResult.program.has_value(), "PerSound config failed to compile");
    Program soundProgram = std::move(*soundResult.program);

    int soundCalls = 0;
    const auto soundRandom = [&soundCalls] {
        ++soundCalls;
        return soundCalls == 1 ? 0.25 : 0.75;
    };
    const Selection firstSound = soundProgram.select(soundRandom);
    const Selection secondSound = soundProgram.select(soundRandom);
    expect(soundCalls == 2, "PerSound did not draw once per selection");
    expect(firstSound.orderedOnIds == std::vector<ModifierId>{"A"},
           "First PerSound selection is wrong");
    expect(secondSound.orderedOnIds.empty(),
           "Second PerSound selection is wrong");

    Config perBottom;
    perBottom.scope = SamplingScope::PerBottom;
    perBottom.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult bottomResult = compile(std::move(perBottom));
    require(bottomResult.program.has_value(), "PerBottom config failed to compile");
    Program bottomProgram = std::move(*bottomResult.program);

    int bottomCalls = 0;
    const auto bottomRandom = [&bottomCalls] {
        ++bottomCalls;
        return bottomCalls == 1 ? 0.25 : 0.75;
    };
    const Selection firstBottom = bottomProgram.select(bottomRandom);
    const Selection secondBottom = bottomProgram.select(bottomRandom);
    expect(bottomCalls == 1, "PerBottom drew more than once");
    expect(firstBottom == secondBottom, "PerBottom did not return its cached selection");
}

// A config with no modifiers should never touch the random source and
// should always select nothing, in both sampling scopes.
void testEmptyConfigDoesNotDraw()
{
    for (SamplingScope scope :
         {SamplingScope::PerSound, SamplingScope::PerBottom}) {
        Config config;
        config.scope = scope;
        CompileResult result = compile(std::move(config));
        require(result.program.has_value(), "Empty config failed to compile");

        int calls = 0;
        Program program = std::move(*result.program);
        const Selection first = program.select([&calls] {
            ++calls;
            return 0.5;
        });
        const Selection second = program.select([&calls] {
            ++calls;
            return 0.5;
        });
        expect(first.orderedOnIds.empty() && second.orderedOnIds.empty(),
               "Empty config selected a modifier");
        expect(calls == 0, "Empty config consumed a random value");
    }
}

// When multiple rules could match the same history, the one with more
// predicates (the more specific one) should win.
void testMostSpecificRuleWins()
{
    Config config;
    config.orderedModifiers = {
        Entry{"A", 0.5, {}},
        Entry{"B", 0.5, {}},
        Entry{"C", 0.1, {
            Rule{{Predicate{"A", true}}, 0.2},
            Rule{{Predicate{"A", true}, Predicate{"B", true}}, 0.8}
        }}
    };

    CompileResult result = compile(std::move(config));
    require(result.program.has_value(), "Specificity config failed to compile");
    Program program = std::move(*result.program);
    const Selection selected = program.select([] { return 0.10; });
    expect(selected.orderedOnIds == std::vector<ModifierId>({"A", "B", "C"}),
           "Most-specific matching rule did not win");
}

// One malformed config per diagnostic code compile() can report -- each
// should fail to compile with exactly that code, not compile or throw.
void testValidation()
{
    expectInvalid(
        Config{SamplingScope::PerSound, {Entry{"", 0.5, {}}}},
        DiagnosticCode::EmptyId,
        "Empty ID");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}}, Entry{"A", 0.5, {}}}},
        DiagnosticCode::DuplicateId,
        "Duplicate ID");

    expectInvalid(
        Config{SamplingScope::PerSound, {Entry{"A", 1.1, {}}}},
        DiagnosticCode::InvalidProbability,
        "Invalid default probability");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {Rule{{Predicate{"A", true}}, -0.1}}}}},
        DiagnosticCode::InvalidProbability,
        "Invalid rule probability");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {Rule{{}, 0.2}}}}},
        DiagnosticCode::EmptyRule,
        "Empty rule");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5,
                      {Rule{{Predicate{"A", true}, Predicate{"A", true}}, 0.2}}}}},
        DiagnosticCode::DuplicatePredicate,
        "Duplicate predicate");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5,
                      {Rule{{Predicate{"A", true}, Predicate{"A", false}}, 0.2}}}}},
        DiagnosticCode::ConflictingPredicate,
        "Conflicting predicate");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"missing", true}}, 0.2}}}}},
        DiagnosticCode::MissingReference,
        "Missing reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"A", true}}, 0.2}}}}},
        DiagnosticCode::SelfReference,
        "Self reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {Rule{{Predicate{"B", true}}, 0.2}}},
                Entry{"B", 0.5, {}}}},
        DiagnosticCode::ForwardReference,
        "Forward reference");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", 0.5, {}},
                Entry{"B", 0.5, {}},
                Entry{"C", 0.5,
                      {Rule{{Predicate{"A", true}}, 0.2},
                       Rule{{Predicate{"B", true}}, 0.3}}}}},
        DiagnosticCode::AmbiguousRules,
        "Equal-specificity overlapping rules");

    expectInvalid(
        Config{static_cast<SamplingScope>(99), {}},
        DiagnosticCode::InvalidSamplingScope,
        "Invalid sampling scope");

    expectInvalid(
        Config{SamplingScope::PerSound,
               {Entry{"A", std::numeric_limits<double>::quiet_NaN(), {}}}},
        DiagnosticCode::InvalidProbability,
        "NaN probability");
}

// A requiredOn=false predicate should match when the referenced modifier is
// OFF, and gate its rule's chance the same way an "on" predicate would.
void testNegativePredicateRuleApplies()
{
    Config config;
    config.orderedModifiers = {
        Entry{"A", 0.5, {}},
        Entry{"B", 0.2, {
            Rule{{Predicate{"A", false}}, 0.9}
        }}
    };

    CompileResult result = compile(std::move(config));
    require(result.program.has_value(), "Negative-predicate config failed to compile");
    Program program = std::move(*result.program);

    // r=0.6: A is off (0.6 > 0.5), so B's "A is off" rule applies (0.9 -> on).
    const Selection whenAOff = program.select([] { return 0.6; });
    expect(whenAOff.orderedOnIds == std::vector<ModifierId>{"B"},
           "B should turn on via its 'A is off' rule once A is off");

    // r=0.3: A is on (0.3 <= 0.5), so the rule no longer matches and B falls
    // back to its own default chance (0.2 -> off, since 0.3 > 0.2).
    const Selection whenAOn = program.select([] { return 0.3; });
    expect(whenAOn.orderedOnIds == std::vector<ModifierId>{"A"},
           "B should fall back to its default chance once A is on");
}

// CompileOptions::overallUsageMode = Skip should leave overallUsage() empty
// instead of running its worst-case-exponential computation.
void testOverallUsageModeSkip()
{
    CompileResult result =
        compile(professorExample(), CompileOptions{OverallUsageMode::Skip});
    require(result.program.has_value(), "Skip-mode config failed to compile");
    expect(result.program->overallUsage().empty(),
           "OverallUsageMode::Skip should leave overallUsage() empty");
}

// Rule order must not affect which rule wins: the same most-specific rule
// should be selected whether it's listed first or last in the config.
void testRuleOrderDoesNotAffectSelection()
{
    auto makeConfig = [](bool reversed) {
        std::vector<Rule> rules = {
            Rule{{Predicate{"A", true}}, 0.2},
            Rule{{Predicate{"A", true}, Predicate{"B", true}}, 0.8}
        };
        if (reversed)
            std::reverse(rules.begin(), rules.end());

        Config config;
        config.orderedModifiers = {
            Entry{"A", 0.5, {}},
            Entry{"B", 0.5, {}},
            Entry{"C", 0.1, rules}
        };
        return config;
    };

    CompileResult forward = compile(makeConfig(false));
    CompileResult reversed = compile(makeConfig(true));
    require(forward.program.has_value() && reversed.program.has_value(),
           "Both rule orderings should compile");

    Program forwardProgram = std::move(*forward.program);
    Program reversedProgram = std::move(*reversed.program);
    const Selection forwardSelection = forwardProgram.select([] { return 0.10; });
    const Selection reversedSelection = reversedProgram.select([] { return 0.10; });

    expect(forwardSelection == reversedSelection,
           "Reordering non-ambiguous rules must not change the selection");
    expect(forwardSelection.orderedOnIds == std::vector<ModifierId>({"A", "B", "C"}),
           "The more specific rule (both A and B on) should still win");
}

// A random source returning a value outside [0, 1) must be rejected, not
// silently used.
void testInvalidRandomValue()
{
    Config config;
    config.orderedModifiers = {Entry{"A", 0.5, {}}};
    CompileResult result = compile(std::move(config));
    require(result.program.has_value(), "Random validation config failed to compile");
    Program program = std::move(*result.program);

    bool threw = false;
    try {
        program.select([] { return 1.0; });
    } catch (const std::domain_error&) {
        threw = true;
    }
    expect(threw, "Out-of-range random value was accepted");
}

// Runs one test and reports it by name. A require() failure or unexpected
// exception fails only that test; the remaining tests still run. Output goes
// to std::cerr (unbuffered), so if a test crashes outright, the last "RUN"
// line in the log names it.
void runTest(const char* name, void (*test)(), std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    std::cerr << "RUN  " << name << '\n';

    try {
        test();
    } catch (const RequirementFailed& e) {
        std::cerr << "FAILED [" << name << "]: " << e.what()
                  << " (stopped this test; later checks depend on it)\n";
        ++currentTestFailures;
    } catch (const std::exception& e) {
        std::cerr << "FAILED [" << name << "]: unexpected exception: " << e.what() << '\n';
        ++currentTestFailures;
    } catch (...) {
        std::cerr << "FAILED [" << name << "]: unexpected non-standard exception\n";
        ++currentTestFailures;
    }

    if (currentTestFailures > 0) {
        failedTests.push_back(name);
    } else {
        std::cerr << "OK   " << name << '\n';
    }
}

} // namespace

int main()
{
    std::vector<std::string> failedTests;

    runTest("testProfessorExample", testProfessorExample, failedTests);
    runTest("testZeroAndOneProbabilities", testZeroAndOneProbabilities, failedTests);
    runTest("testSamplingScopeRandomCalls", testSamplingScopeRandomCalls, failedTests);
    runTest("testEmptyConfigDoesNotDraw", testEmptyConfigDoesNotDraw, failedTests);
    runTest("testMostSpecificRuleWins", testMostSpecificRuleWins, failedTests);
    runTest("testValidation", testValidation, failedTests);
    runTest("testNegativePredicateRuleApplies", testNegativePredicateRuleApplies, failedTests);
    runTest("testOverallUsageModeSkip", testOverallUsageModeSkip, failedTests);
    runTest("testRuleOrderDoesNotAffectSelection", testRuleOrderDoesNotAffectSelection, failedTests);
    runTest("testInvalidRandomValue", testInvalidRandomValue, failedTests);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " ModifierUsage test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }

    std::cerr << "\nAll ModifierUsage tests passed\n";
    return 0;
}
