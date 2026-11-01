// Tests for the functions composers type into LASSIE fields (RandomInt,
// Fibonacci, Select, Stochos, ...), as CMOD evaluates them. -- Tony Chai, 2026
//
// Each test hands CMOD the same <Fun>...</Fun> text that LASSIE's function
// editors write into a project, and checks the *result* -- the number or
// envelope that comes back, or that bad input is reported as a project
// error instead of crashing or producing garbage. It doesn't check how CMOD
// computes the result, so rewriting a function the same-results way won't
// break these tests.
//
// Random functions can't be compared to fixed numbers (each platform's rand()
// gives different sequences), so they're checked by properties that hold
// for any sequence: every draw is in range, every allowed value eventually
// shows up, averages land where they should, and the same seed repeats the
// same draws.
//
// Not covered here yet: the functions that read the current event
// (CURRENT_CHILD_NUM, GetPattern's per-event memory, ...), which need a
// running piece.

#include "../src/CmodError.h"
#include "../src/Libraries.h"
#include "../src/Patter.h"
#include "../src/Random.h"
#include "../src/Sieve.h"
#include "../src/Utilities.h"

#include <pugixml.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string currentTest;
int currentTestFailures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED [" << currentTest << "]: " << message << '\n';
        ++currentTestFailures;
    }
}

void expectNear(double actual, double expected, double tolerance, const std::string& what)
{
    expect(std::abs(actual - expected) <= tolerance,
           what + ": expected " + std::to_string(expected) + ", got " + std::to_string(actual));
}

// Thrown by require() to stop just the current test when the following
// checks depend on this one.
struct RequirementFailed : std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw RequirementFailed(message);
}

// Checks that `action` reports a CMOD project error (the kind LASSIE shows
// the user) rather than crashing, hanging, or quietly returning a number.
void expectProjectError(const std::function<void()>& action, const std::string& what)
{
    try {
        action();
        expect(false, what + ": expected a project error, but it succeeded");
    } catch (const CmodError& error) {
        // Exit code 2 means CMOD treats it as its own internal bug.
        expect(error.exitCode() == 1,
               what + ": expected a project error, got an internal CMOD error: "
                   + error.what());
    } catch (const std::exception& error) {
        expect(false, what + ": expected a project error, got another exception: "
                          + error.what());
    }
}

// Builds a function the way LASSIE's editors write it, e.g.
// fun("Fibonacci", {{"Entry", "10"}}) ->
// "<Fun><Name>Fibonacci</Name><Entry>10</Entry></Fun>".
std::string fun(const std::string& name,
                const std::vector<std::pair<std::string, std::string>>& fields)
{
    std::string xml = "<Fun><Name>" + name + "</Name>";
    for (const auto& [tag, value] : fields)
        xml += "<" + tag + ">" + value + "</" + tag + ">";
    return xml + "</Fun>";
}

// MakeEnvelope that stays at `value` from start to end.
std::string flatEnvelope(const std::string& value)
{
    return fun("MakeEnvelope", {{"Xs", "<X>0</X><X>1</X>"},
                                {"Ys", "<Y>" + value + "</Y><Y>" + value + "</Y>"},
                                {"Types", "<T>LINEAR</T>"},
                                {"Pros", "<P>FLEXIBLE</P>"},
                                {"Scale", "1"}});
}

// The library every test sees, in the format LASSIE saves projects in:
//   envelope 1 rises in a straight line from 0 to 1,
//   envelope 2 stays at 1,
//   Markov model 0 moves between the values 60 and 72.
const char* const kTestProject = R"(<ProjectRoot>
<EnvelopeLibrary>
2
Envelope 1
2
0.000     0.000     LINEAR              FLEXIBLE    1.000
1.000     1.000
Envelope 2
2
0.000     1.000     LINEAR              FLEXIBLE    1.000
1.000     1.000
</EnvelopeLibrary>
<MarkovModelLibrary>
1
2
60 72
0.25 0.75
0.1 0.9 0.6 0.4
</MarkovModelLibrary>
<Events></Events>
</ProjectRoot>)";

Utilities* cmod = nullptr;

// Evaluates a field the way CMOD does for project-level settings (no event).
double eval(const std::string& expression)
{
    return cmod->evaluate(expression, nullptr);
}

std::unique_ptr<Envelope> envelope(const std::string& expression)
{
    return std::unique_ptr<Envelope>(
        static_cast<Envelope*>(cmod->evaluateObject(expression, nullptr, eventEnv)));
}

std::unique_ptr<Patter> pattern(const std::string& expression)
{
    return std::unique_ptr<Patter>(
        static_cast<Patter*>(cmod->evaluateObject(expression, nullptr, eventPat)));
}

// MakeSieve as LASSIE writes it. Low/High are numbers here; the helper wraps
// them in flat envelopes the way LASSIE's "constant envelope" entries do.
std::string makeSieve(const std::string& low, const std::string& high,
                      const std::string& method, const std::string& elements,
                      const std::string& weightMethod, const std::string& weights,
                      const std::string& offset)
{
    return fun("MakeSieve", {{"Low", flatEnvelope(low)}, {"High", flatEnvelope(high)},
                             {"Method", method}, {"Elements", elements},
                             {"WeightMethod", weightMethod}, {"Weight", weights},
                             {"Offset", offset}});
}

// What a sieve contains: its elements in order, and the weight of each.
struct SieveContents {
    std::vector<int> elements;
    std::vector<double> weights;
};

SieveContents sieveContents(const std::string& expression)
{
    std::unique_ptr<Sieve> sieve(
        static_cast<Sieve*>(cmod->evaluateObject(expression, nullptr, eventSiv)));
    require(sieve != nullptr, "the sieve function didn't return a sieve");
    SieveContents contents;
    sieve->FillInVectors(contents.elements, contents.weights);
    return contents;
}

std::string listText(const std::vector<int>& values)
{
    std::string text;
    for (int value : values)
        text += (text.empty() ? "" : ", ") + std::to_string(value);
    return "{" + text + "}";
}

std::string listText(const std::vector<double>& values)
{
    std::string text;
    for (double value : values) {
        std::string number = std::to_string(value);
        number.erase(number.find_last_not_of('0') + 1);
        if (number.back() == '.')
            number.pop_back();
        text += (text.empty() ? "" : ", ") + number;
    }
    return "{" + text + "}";
}

void expectElements(const SieveContents& sieve, const std::vector<int>& expected,
                    const std::string& what)
{
    expect(sieve.elements == expected, what + ": expected elements " + listText(expected)
                                           + ", got " + listText(sieve.elements));
}

void expectWeights(const SieveContents& sieve, const std::vector<double>& expected,
                   const std::string& what)
{
    expect(sieve.weights == expected, what + ": expected weights " + listText(expected)
                                          + ", got " + listText(sieve.weights));
}

// Draws a numeric function many times and counts how often each value shows up.
std::map<int, int> countDraws(const std::string& expression, int draws)
{
    std::map<int, int> counts;
    for (int i = 0; i < draws; ++i)
        ++counts[static_cast<int>(eval(expression))];
    return counts;
}

std::string countsText(const std::map<int, int>& counts)
{
    std::string text;
    for (const auto& [value, count] : counts)
        text += (text.empty() ? "" : ", ") + std::to_string(value) + " x" + std::to_string(count);
    return "{" + text + "}";
}

// ---------------------------------------------------------------- tests ---

void testPlainArithmetic()
{
    expectNear(eval("2+3*4"), 14, 1e-9, "2+3*4");
    expectNear(eval("(1+1)^3"), 8, 1e-9, "(1+1)^3");
    expectNear(eval("7/2"), 3.5, 1e-9, "7/2");
    expectNear(eval(""), 0, 1e-9, "an empty field");
    expectProjectError([] { eval("2+*3"); }, "a malformed expression");
    expectProjectError([] { eval("1/0"); }, "division by zero");
}

void testFunctionsInsideExpressions()
{
    const std::string fib6 = fun("Fibonacci", {{"Entry", "6"}});
    expectNear(eval("10 + " + fib6 + " * 2"), 26, 1e-9, "10 + Fibonacci(6) * 2");
    expectNear(eval(fib6 + " + " + fib6), 16, 1e-9, "two functions in one field");
    expectNear(eval(fun("Inverse", {{"Entry", fun("Fibonacci", {{"Entry", "4"}})}})),
               1.0 / 3, 1e-6, "Inverse(Fibonacci(4)), one function inside another");
    expectProjectError([] { eval(fun("NoSuchFunction", {})); }, "an unknown function name");
}

void testInverse()
{
    expectNear(eval(fun("Inverse", {{"Entry", "4"}})), 0.25, 1e-6, "Inverse(4)");
    expectNear(eval(fun("Inverse", {{"Entry", "-2"}})), -0.5, 1e-6, "Inverse(-2)");
    expectProjectError([] { eval(fun("Inverse", {{"Entry", "0"}})); }, "Inverse(0)");
}

// Despite its name, LN gives e^(-x) (1 / 2.71828^x), not the natural log.
// These lock in what it does today; if that's ever corrected on purpose,
// update the expected values here.
void testLN()
{
    expectNear(eval(fun("LN", {{"Entry", "0"}})), 1.0, 1e-6, "LN(0)");
    expectNear(eval(fun("LN", {{"Entry", "1"}})), 0.367879, 1e-5, "LN(1)");
    expectNear(eval(fun("LN", {{"Entry", "2"}})), 0.135335, 1e-5, "LN(2)");
}

void testFibonacci()
{
    const std::map<int, double> expected = {
        {0, 1}, {1, 1}, {2, 1}, {3, 2}, {4, 3}, {10, 55}, {20, 6765}, {46, 1836311903}};
    for (const auto& [entry, value] : expected)
        expectNear(eval(fun("Fibonacci", {{"Entry", std::to_string(entry)}})), value, 1e-9,
                   "Fibonacci(" + std::to_string(entry) + ")");
    expectProjectError([] { eval(fun("Fibonacci", {{"Entry", "47"}})); },
                       "Fibonacci(47), past the largest supported entry");
}

void testDecay()
{
    const auto decay = [](const std::string& base, const std::string& type,
                          const std::string& rate, const std::string& index) {
        return eval(fun("Decay", {{"Base", base}, {"Type", type}, {"Rate", rate}, {"Index", index}}));
    };
    expectNear(decay("10", "EXPONENTIAL", "0.5", "2"), 2.5, 1e-6, "exponential decay 10*0.5^2");
    expectNear(decay("10", "EXPONENTIAL", "0.5", "0"), 10, 1e-6, "exponential decay at index 0");
    expectNear(decay("10", "LINEAR", "2", "3"), 4, 1e-6, "linear decay 10-2*3");
    expectNear(decay("10", "LINEAR", "2", "0"), 10, 1e-6, "linear decay at index 0");
    expectProjectError([&] { decay("10", "SIDEWAYS", "2", "3"); }, "an unknown decay type");
}

void testSelect()
{
    const auto select = [](const std::string& list, const std::string& index) {
        return eval(fun("Select", {{"List", list}, {"Index", index}}));
    };
    expectNear(select("10, 20, 30", "0"), 10, 1e-6, "Select index 0 (first entry)");
    expectNear(select("10, 20, 30", "2"), 30, 1e-6, "Select index 2 (last entry)");
    expectNear(select("10, 20, 30", "1+1"), 30, 1e-6, "Select with a calculated index");
    expectNear(select("5, " + fun("Fibonacci", {{"Entry", "7"}}), "1"), 13, 1e-6,
               "Select picking a function from the list");
    // Commas inside a nested function belong to that function, not to the list.
    expectNear(select(fun("Select", {{"List", "1, 2"}, {"Index", "1"}}) + ", 7", "0"), 2, 1e-6,
               "Select whose entry is another Select with its own commas");
    expectNear(select(fun("Select", {{"List", "1, 2"}, {"Index", "1"}}) + ", 7", "1"), 7, 1e-6,
               "the entry after a nested Select");
    expectProjectError([&] { select("10, 20, 30", "3"); }, "Select index past the end");
    expectProjectError([&] { select("10, 20, 30", "-1"); }, "Select with a negative index");
    expectProjectError([&] { select("", "0"); }, "Select with an empty list");
}

void testRandomInt()
{
    Random::Seed(1234);
    std::set<int> seen;
    bool allInRange = true;
    bool allWhole = true;
    for (int i = 0; i < 2000; ++i) {
        const double value = eval(fun("RandomInt", {{"Low", "3"}, {"High", "7"}}));
        allInRange = allInRange && value >= 3 && value <= 7;
        allWhole = allWhole && value == std::floor(value);
        seen.insert(static_cast<int>(value));
    }
    expect(allInRange, "RandomInt(3, 7) gave a value outside 3..7");
    expect(allWhole, "RandomInt(3, 7) gave a value that isn't a whole number");
    expect(seen.size() == 5, "RandomInt(3, 7) produced only " + std::to_string(seen.size())
                                 + " of the 5 possible values in 2000 draws");

    expectNear(eval(fun("RandomInt", {{"Low", "5"}, {"High", "5"}})), 5, 1e-9,
               "RandomInt with equal bounds");
    // Older project files call the fields LowBound/HighBound.
    const double old = eval(fun("RandomInt", {{"LowBound", "8"}, {"HighBound", "9"}}));
    expect(old == 8 || old == 9, "RandomInt with the older LowBound/HighBound field names gave "
                                     + std::to_string(old));
    expectProjectError([] { eval(fun("RandomInt", {{"Low", "7"}, {"High", "3"}})); },
                       "RandomInt with Low above High");
    expectProjectError([] { eval(fun("RandomInt", {{"Low", "1"}})); },
                       "RandomInt with no High field");
}

void testRandom()
{
    Random::Seed(1234);
    double sum = 0;
    double lowest = 1e9, highest = -1e9;
    const int draws = 5000;
    for (int i = 0; i < draws; ++i) {
        const double value = eval(fun("Random", {{"Low", "2"}, {"High", "4"}}));
        sum += value;
        lowest = std::min(lowest, value);
        highest = std::max(highest, value);
    }
    expect(lowest >= 2 && highest <= 4,
           "Random(2, 4) gave values from " + std::to_string(lowest) + " to "
               + std::to_string(highest) + ", outside 2..4");
    expectNear(sum / draws, 3.0, 0.1, "average of 5000 Random(2, 4) draws");
    expect(highest - lowest > 1.9, "Random(2, 4) didn't spread across the range (from "
                                       + std::to_string(lowest) + " to "
                                       + std::to_string(highest) + ")");
}

void testRandomizer()
{
    Random::Seed(1234);
    bool inRange = true;
    bool someBelow = false, someAbove = false;
    for (int i = 0; i < 1000; ++i) {
        const double value = eval(fun("Randomizer", {{"Base", "100"}, {"Deviation", "0.1"}}));
        inRange = inRange && value >= 90 && value <= 110;
        someBelow = someBelow || value < 100;
        someAbove = someAbove || value > 100;
    }
    expect(inRange, "Randomizer(100, 10%) gave a value outside 90..110");
    expect(someBelow && someAbove,
           "Randomizer(100, 10%) didn't vary to both sides of 100");
    expectNear(eval(fun("Randomizer", {{"Base", "100"}, {"Deviation", "0"}})), 100, 1e-6,
               "Randomizer with 0 deviation");
}

// RandomOrderInt deals out every number in the range once, in random order,
// before any repeats; each Id keeps its own deck.
void testRandomOrderInt()
{
    Random::Seed(1234);
    const std::set<int> full = {1, 2, 3, 4, 5, 6};
    const auto draw = [](const std::string& id) {
        return static_cast<int>(eval(fun("RandomOrderInt", {{"Low", "1"}, {"High", "6"}, {"Id", id}})));
    };
    for (int round = 1; round <= 3; ++round) {
        std::set<int> dealt;
        for (int i = 0; i < 6; ++i)
            dealt.insert(draw("101"));
        expect(dealt == full, "round " + std::to_string(round)
                                  + " of RandomOrderInt(1, 6) repeated a number before using all 6");
    }
    // Two Ids drawn alternately must not share or disturb each other's deck.
    std::set<int> a, b;
    for (int i = 0; i < 6; ++i) {
        a.insert(draw("201"));
        b.insert(draw("202"));
    }
    expect(a == full && b == full,
           "two RandomOrderInt Ids drawn alternately didn't each deal all 6 numbers");
    expectProjectError([] { eval(fun("RandomOrderInt", {{"Low", "1"}, {"High", "6"}})); },
                       "RandomOrderInt with no Id");
}

// Same project + same seed must give the same music.
void testSameSeedRepeats()
{
    const auto sequence = [](unsigned seed) {
        Random::Seed(seed);
        std::vector<double> values;
        for (int i = 0; i < 20; ++i) {
            values.push_back(eval(fun("RandomInt", {{"Low", "0"}, {"High", "1000"}})));
            values.push_back(eval(fun("Random", {{"Low", "0"}, {"High", "1"}})));
        }
        return values;
    };
    expect(sequence(42) == sequence(42), "the same seed gave different random values");
    expect(sequence(42) != sequence(43), "different seeds gave identical random values");
}

void testMarkov()
{
    Random::Seed(1234);
    std::set<double> seen;
    bool onlyStates = true;
    for (int i = 0; i < 200; ++i) {
        const double value = eval(fun("Markov", {{"Entry", "0"}}));
        onlyStates = onlyStates && (value == 60 || value == 72);
        seen.insert(value);
    }
    expect(onlyStates, "Markov model 0 gave a value that isn't one of its states (60, 72)");
    expect(seen.size() == 2, "Markov model 0 never moved between its two states in 200 draws");
    expectProjectError([] { eval(fun("Markov", {{"Entry", "1"}})); },
                       "Markov with a model number past the library");
    expectProjectError([] { eval(fun("Markov", {{"Entry", "-1"}})); },
                       "Markov with a negative model number");
}

void testRandomDensity()
{
    Random::Seed(1234);
    bool inRange = true;
    for (int i = 0; i < 1000; ++i) {
        const double value = eval(fun("RandomDensity", {{"Envelope", "1"}, {"Low", "10"}, {"High", "20"}}));
        inRange = inRange && value >= 10 && value <= 20;
    }
    expect(inRange, "RandomDensity(envelope 1, 10, 20) gave a value outside 10..20");
    expectProjectError([] { eval(fun("RandomDensity", {{"Envelope", "3"}, {"Low", "0"}, {"High", "1"}})); },
                       "RandomDensity with an envelope number past the library");
    expectProjectError([] { eval(fun("RandomDensity", {{"Envelope", "0"}, {"Low", "0"}, {"High", "1"}})); },
                       "RandomDensity with envelope number 0 (numbers start at 1)");
}

void testMakeEnvelope()
{
    const std::string ramp = fun("MakeEnvelope", {{"Xs", "<X>0</X><X>1</X>"},
                                                  {"Ys", "<Y>0.5</Y><Y>1</Y>"},
                                                  {"Types", "<T>LINEAR</T>"},
                                                  {"Pros", "<P>FLEXIBLE</P>"},
                                                  {"Scale", "2"}});
    // As a whole envelope (e.g. for Stochos), scaled by 2.
    const auto env = envelope(ramp);
    require(env != nullptr, "MakeEnvelope didn't return an envelope");
    expectNear(env->getValue(0, 1), 1.0, 1e-6, "MakeEnvelope start value (0.5 x scale 2)");
    expectNear(env->getValue(0.5, 1), 1.5, 1e-6, "MakeEnvelope halfway value (0.75 x scale 2)");
    expectNear(env->getValue(1, 1), 2.0, 1e-6, "MakeEnvelope end value (1 x scale 2)");
    // In a number field with no event, it gives the value at the start.
    expectNear(eval(ramp), 1.0, 1e-6, "MakeEnvelope typed into a number field");

    expectProjectError([] {
        envelope(fun("MakeEnvelope", {{"Xs", "<X>0</X><X>1</X>"}, {"Ys", "<Y>0</Y>"},
                                      {"Types", "<T>LINEAR</T>"}, {"Pros", "<P>FLEXIBLE</P>"},
                                      {"Scale", "1"}}));
    }, "MakeEnvelope with more X points than Y points");
    expectProjectError([] {
        envelope(fun("MakeEnvelope", {{"Xs", "<X>0</X><X>1</X>"}, {"Ys", "<Y>0</Y><Y>1</Y>"},
                                      {"Types", "<T>WIGGLY</T>"}, {"Pros", "<P>FLEXIBLE</P>"},
                                      {"Scale", "1"}}));
    }, "MakeEnvelope with an unknown segment type");
}

void testEnvLib()
{
    const auto doubled = envelope(fun("EnvLib", {{"Env", "1"}, {"Scale", "2"}}));
    require(doubled != nullptr, "EnvLib didn't return an envelope");
    expectNear(doubled->getValue(0.5, 1), 1.0, 1e-6, "library envelope 1 halfway, scale 2");
    expectNear(doubled->getValue(1, 1), 2.0, 1e-6, "library envelope 1 at the end, scale 2");
    // Scaling one use must not change the library's copy for the next use.
    const auto plain = envelope(fun("EnvLib", {{"Env", "1"}, {"Scale", "1"}}));
    expectNear(plain->getValue(1, 1), 1.0, 1e-6,
               "library envelope 1 after another field used it at scale 2");
    expectProjectError([] { envelope(fun("EnvLib", {{"Env", "3"}, {"Scale", "1"}})); },
                       "EnvLib with an envelope number past the library");
    expectProjectError([] { envelope(fun("EnvLib", {{"Env", "0"}, {"Scale", "1"}})); },
                       "EnvLib with envelope number 0 (numbers start at 1)");
}

void testStochosFunctions()
{
    const auto stochos = [](const std::vector<std::string>& envelopes) {
        std::string list;
        for (const std::string& env : envelopes)
            list += "<Envelope>" + env + "</Envelope>";
        return eval(fun("Stochos", {{"Method", "FUNCTIONS"}, {"Envelopes", list}, {"Offset", "0"}}));
    };
    // One envelope: just its value.
    expectNear(stochos({flatEnvelope("0.7")}), 0.7, 1e-6, "Stochos FUNCTIONS with one envelope");

    // Several envelopes split 0..1 into probability areas and return the
    // area number: here area 0 covers 30% and area 1 the other 70%.
    Random::Seed(1234);
    int zeros = 0, others = 0;
    for (int i = 0; i < 2000; ++i) {
        const double area = stochos({flatEnvelope("0.3"), flatEnvelope("1")});
        if (area == 0) ++zeros;
        else if (area != 1) ++others;
    }
    expect(others == 0, "Stochos FUNCTIONS with 2 areas returned something other than 0 or 1 ("
                            + std::to_string(others) + " times)");
    expectNear(zeros / 2000.0, 0.3, 0.05, "share of draws in the 30% area");
    expectProjectError([] {
        eval(fun("Stochos", {{"Method", "FUNCTIONS"}, {"Envelopes", ""}, {"Offset", "0"}}));
    }, "Stochos with no envelopes");
    expectProjectError([] {
        eval(fun("Stochos", {{"Method", "SIDEWAYS"},
                             {"Envelopes", "<Envelope>" + flatEnvelope("1") + "</Envelope>"},
                             {"Offset", "0"}}));
    }, "Stochos with an unknown method");
}

void testStochosRange()
{
    // Groups of 3 envelopes: lowest value, highest value, and how values are
    // spread between them. Offset picks the group.
    const std::string group1 = "<Envelope>" + flatEnvelope("0.2") + "</Envelope>"
                             + "<Envelope>" + flatEnvelope("0.8") + "</Envelope>"
                             + "<Envelope>" + fun("EnvLib", {{"Env", "1"}, {"Scale", "1"}}) + "</Envelope>";
    const std::string group2 = "<Envelope>" + flatEnvelope("5") + "</Envelope>"
                             + "<Envelope>" + flatEnvelope("6") + "</Envelope>"
                             + "<Envelope>" + fun("EnvLib", {{"Env", "1"}, {"Scale", "1"}}) + "</Envelope>";
    const auto stochos = [&](const std::string& offset) {
        return eval(fun("Stochos", {{"Method", "RANGE_DISTRIB"}, {"Envelopes", group1 + group2},
                                    {"Offset", offset}}));
    };
    Random::Seed(1234);
    bool firstInRange = true, secondInRange = true;
    for (int i = 0; i < 500; ++i) {
        const double a = stochos("0");
        const double b = stochos("1");
        firstInRange = firstInRange && a >= 0.2 && a <= 0.8;
        secondInRange = secondInRange && b >= 5 && b <= 6;
    }
    expect(firstInRange, "Stochos RANGE_DISTRIB group 0 gave a value outside 0.2..0.8");
    expect(secondInRange, "Stochos RANGE_DISTRIB group 1 gave a value outside 5..6");
    expectProjectError([&] { stochos("2"); }, "Stochos RANGE_DISTRIB offset past the last group");
}

// ---------------------------------------------------- sieves & patterns ---
//
// A sieve is a set of allowed whole numbers (e.g. allowed start times or
// pitches) with a weight for each, controlling how likely it is to be picked.

void testSieveElements()
{
    expectElements(sieveContents(makeSieve("0", "10", "MEANINGFUL", "2, 5, 8, 12", "PERIODIC", "1", "0")),
                   {2, 5, 8}, "MEANINGFUL keeps only the listed values within Low..High");
    expectElements(sieveContents(makeSieve("0", "10", "MEANINGFUL", "8, 2, 5", "PERIODIC", "1", "0")),
                   {2, 5, 8}, "MEANINGFUL sorts the listed values");
    expectElements(sieveContents(makeSieve("0", "10", "MEANINGFUL", "2, 5, 8", "PERIODIC", "1", "1")),
                   {3, 6, 9}, "MEANINGFUL with one offset shifts every value");
    expectElements(sieveContents(makeSieve("0", "10", "MEANINGFUL", "2, 5, 8", "PERIODIC", "1", "0, 1, 3")),
                   {2, 6}, "MEANINGFUL with one offset per value (8+3 falls outside 0..10)");
    expectElements(sieveContents(makeSieve("3", "6", "FAKE", "", "PERIODIC", "1", "0")),
                   {3, 4, 5, 6}, "FAKE keeps every whole number in Low..High");
    expectElements(sieveContents(makeSieve("2.6", "5.4", "FAKE", "", "PERIODIC", "1", "0")),
                   {3, 4, 5}, "Low/High round to the nearest whole number");
    expectElements(sieveContents(makeSieve("0", "12", "MODS", "3", "PERIODIC", "1", "0")),
                   {0, 3, 6, 9, 12}, "MODS 3 keeps the multiples of 3");
    expectElements(sieveContents(makeSieve("0", "12", "MODS", "3", "PERIODIC", "1", "1")),
                   {1, 4, 7, 10}, "MODS 3 with offset 1");
    expectElements(sieveContents(makeSieve("0", "10", "MODS", "2U3", "PERIODIC", "1", "0, 0")),
                   {0, 2, 3, 4, 6, 8, 9, 10}, "MODS 2U3 (multiples of 2 or of 3)");
    expectElements(sieveContents(makeSieve("0", "12", "MODS", "2I3", "PERIODIC", "1", "0, 0")),
                   {0, 6, 12}, "MODS 2I3 (multiples of both 2 and 3)");
}

void testSieveWeights()
{
    // The manual: PERIODIC "assigns weights ... by cycling through the given
    // ordered list of weights", whatever its length.
    expectWeights(sieveContents(makeSieve("1", "4", "FAKE", "", "PERIODIC", "1, 2", "0")),
                  {1, 2, 1, 2}, "PERIODIC repeats its weights across the elements");
    expectWeights(sieveContents(makeSieve("1", "4", "FAKE", "", "PERIODIC", "1", "0")),
                  {1, 1, 1, 1}, "PERIODIC with a single weight gives every element that weight");
    expectWeights(sieveContents(makeSieve("1", "6", "FAKE", "", "PERIODIC", "1, 2, 3", "0")),
                  {1, 2, 3, 1, 2, 3}, "PERIODIC with three weights cycles through all three");
    expectWeights(sieveContents(makeSieve("1", "3", "FAKE", "", "INCLUDE", "5, 0, 1", "0")),
                  {5, 0, 1}, "INCLUDE gives each element its own weight");
    // Multiples of 2 get weight 1 and multiples of 3 get 10; 0 and 6 are both.
    expectWeights(sieveContents(makeSieve("0", "6", "MODS", "2U3", "HIERARCHIC", "1, 10", "0, 0")),
                  {11, 1, 10, 1, 11}, "HIERARCHIC adds the weights of every modulus an element belongs to");
}

void testSieveErrors()
{
    expectProjectError([] { sieveContents(makeSieve("0", "10", "FIBONACCI", "1", "PERIODIC", "1", "0")); },
                       "sieve element method FIBONACCI (documented as unsupported)");
    expectProjectError([] { sieveContents(makeSieve("0", "10", "FAKE", "", "SIDEWAYS", "1", "0")); },
                       "an unknown sieve weight method");
    expectProjectError([] { sieveContents(makeSieve("0", "10", "MEANINGFUL", "20, 30", "PERIODIC", "1", "0")); },
                       "a sieve with no elements inside Low..High");
    expectProjectError([] { sieveContents(makeSieve("1", "4", "FAKE", "", "INCLUDE", "1, 1", "0")); },
                       "INCLUDE with fewer weights than elements");
    expectProjectError([] { sieveContents(makeSieve("0", "10", "MODS", "2U", "PERIODIC", "1", "0")); },
                       "a malformed MODS expression");
}

// ChooseL picks one element of a sieve at random, following its weights.
void testChooseL()
{
    Random::Seed(1234);
    const auto even = countDraws(
        fun("ChooseL", {{"Entry", makeSieve("1", "4", "FAKE", "", "INCLUDE", "1, 1, 1, 1", "0")}}), 2000);
    expect(even.size() == 4, "ChooseL from {1, 2, 3, 4} with equal weights didn't pick every element: "
                                 + countsText(even));
    for (const auto& [value, count] : even)
        expect(value >= 1 && value <= 4, "ChooseL picked " + std::to_string(value)
                                             + ", which isn't in the sieve {1, 2, 3, 4}");

    const auto weighted = countDraws(
        fun("ChooseL", {{"Entry", makeSieve("1", "3", "FAKE", "", "INCLUDE", "1, 0, 3", "0")}}), 2000);
    expect(weighted.count(2) == 0, "ChooseL picked an element whose weight is 0: " + countsText(weighted));
    expectNear(weighted.count(3) ? weighted.at(3) / 2000.0 : 0, 0.75, 0.05,
               "share of ChooseL picks for the element weighted 3 of 4");
}

// ValuePick builds a sieve whose range follows envelopes (Low/High times
// Range) and whose weights are blended with a distribution envelope.
void testValuePick()
{
    const auto valuePick = [](const std::string& method, const std::string& elements,
                              const std::string& weightMethod, const std::string& weights,
                              const std::string& dist, const std::string& offsets) {
        return fun("ValuePick", {{"Range", "10"}, {"Low", flatEnvelope("0.2")},
                                 {"High", flatEnvelope("0.8")}, {"Dist", flatEnvelope(dist)},
                                 {"Method", method}, {"Elements", elements},
                                 {"WeightMethod", weightMethod}, {"Weight", weights},
                                 {"Type", "CONSTANT"}, {"Offsets", offsets}});
    };
    Random::Seed(1234);
    // Range 10 x Low 0.2 / High 0.8 -> every whole number from 2 to 8.
    const auto all = countDraws(valuePick("FAKE", "", "PERIODIC", "1", "1", "0"), 3000);
    expect(!all.empty() && all.begin()->first == 2 && all.rbegin()->first == 8 && all.size() == 7,
           "ValuePick FAKE over Range 10 x 0.2..0.8 should pick every number 2..8, got "
               + countsText(all));

    // A weight of 0 with a flat-0 distribution envelope is never picked.
    const auto weighted = countDraws(valuePick("MEANINGFUL", "2, 4, 6, 20", "INCLUDE", "1, 0, 1", "0", "0, 0, 0, 0"), 2000);
    expect(weighted.count(4) == 0, "ValuePick picked 4, whose weight is 0: " + countsText(weighted));
    expect(weighted.count(20) == 0, "ValuePick picked 20, outside 2..8: " + countsText(weighted));
    expect(weighted.count(2) && weighted.count(6),
           "ValuePick didn't pick both 2 and 6: " + countsText(weighted));
}

// A pattern is a starting value plus a list of steps: start 60 with steps
// 2, 2, 1 gives 60, 62, 64, 65, then repeats.
void testMakePattern()
{
    const auto made = pattern(fun("MakePattern", {{"List", "2, 2, 1"}}));
    require(made != nullptr, "MakePattern didn't return a pattern");
    std::vector<int> values;
    for (int i = 0; i < 6; ++i)
        values.push_back(made->GetNextValue("IN_ORDER", 60));
    expect(values == std::vector<int>{60, 62, 64, 65, 60, 62},
           "MakePattern 2, 2, 1 from 60 in order: expected {60, 62, 64, 65, 60, 62}, got "
               + listText(values));

    const auto falling = pattern(fun("MakePattern", {{"List", "-3, 1+1"}}));
    std::vector<int> fallingValues;
    for (int i = 0; i < 3; ++i)
        fallingValues.push_back(falling->GetNextValue("IN_ORDER", 10));
    expect(fallingValues == std::vector<int>{10, 7, 9},
           "MakePattern -3, 1+1 from 10: expected {10, 7, 9}, got " + listText(fallingValues));

    expectProjectError([] { pattern(fun("MakePattern", {{"List", "2"}}))->GetNextValue("RANDOM", 60); },
                       "GetPattern with a method other than IN_ORDER");
}

// ExpandPattern EQUIVALENCE lets each value jump by whole multiples of
// Modulo (e.g. octaves of 12) while staying inside Low..High.
void testExpandPattern()
{
    Random::Seed(1234);
    const int originals[] = {60, 62, 64, 65};
    bool sameClass = true, inRange = true, everMoved = false;
    for (int trial = 0; trial < 200; ++trial) {
        const auto expanded = pattern(fun("ExpandPattern",
            {{"Method", "EQUIVALENCE"}, {"Modulo", "12"}, {"Low", "48"}, {"High", "84"},
             {"Pattern", fun("MakePattern", {{"List", "2, 2, 1"}})}}));
        for (int original : originals) {
            const int value = expanded->GetNextValue("IN_ORDER", 60);
            sameClass = sameClass && (value - original) % 12 == 0;
            inRange = inRange && value >= 48 && value <= 84;
            everMoved = everMoved || value != original;
        }
    }
    expect(sameClass, "ExpandPattern moved a value by something other than a multiple of 12");
    expect(inRange, "ExpandPattern produced a value outside 48..84");
    expect(everMoved, "ExpandPattern never moved any value in 200 tries");

    expectProjectError([] {
        pattern(fun("ExpandPattern", {{"Method", "SYMMETRIES"}, {"Modulo", "12"}, {"Low", "48"},
                                      {"High", "84"}, {"Pattern", fun("MakePattern", {{"List", "2"}})}}));
    }, "ExpandPattern SYMMETRIES (offered by LASSIE, not implemented in CMOD)");
    expectProjectError([] {
        pattern(fun("ExpandPattern", {{"Method", "EQUIVALENCE"}, {"Modulo", "12"}, {"Low", "0"},
                                      {"High", "5"}, {"Pattern", fun("MakePattern", {{"List", "2"}})}}))
            ->GetNextValue("IN_ORDER", 60);
    }, "ExpandPattern whose range can't hold any equivalent value");
}

void testEventOnlyFunctionsNeedAnEvent()
{
    for (const char* name : {"CURRENT_CHILD_NUM", "CURRENT_TYPE", "CURRENT_LAYER", "AVAILABLE_EDU"})
        expectProjectError([&] { eval(fun(name, {})); },
                           std::string(name) + " used outside an event");
}

void testRemoveSpaces()
{
    expect(Utilities::removeSpaces("a b") == "ab", "removeSpaces(\"a b\") gave \""
                                                       + Utilities::removeSpaces("a b") + "\"");
    const std::string doubled = Utilities::removeSpaces("a  b   c");
    expect(doubled == "abc", "removeSpaces(\"a  b   c\") gave \"" + doubled + "\"");
}

// Runs one test and reports it by name; a failure stops only that test.
void runTest(const std::string& name, void (*test)(), std::vector<std::string>& failedTests)
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
    }
    if (currentTestFailures > 0)
        failedTests.push_back(name);
    else
        std::cerr << "OK   " << name << '\n';
}

} // namespace

int main()
{
    // CMOD writes a scratch file (lib.temp) into the current folder while
    // loading the envelope library, so work in a fresh temporary folder.
    const fs::path previous = fs::current_path();
    const fs::path folder = fs::temp_directory_path()
        / ("dissco-cmod-function-tests-"
           + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(folder);
    fs::current_path(folder);

    pugi::xml_document project;
    project.load_string(kTestProject);
    std::vector<std::string> failedTests;
    try {
        Utilities utilities(project.document_element(), "", false, false, false, 1, 2, 44100, nullptr);
        cmod = &utilities;

        runTest("plain arithmetic", testPlainArithmetic, failedTests);
        runTest("functions inside expressions", testFunctionsInsideExpressions, failedTests);
        runTest("Inverse", testInverse, failedTests);
        runTest("LN", testLN, failedTests);
        runTest("Fibonacci", testFibonacci, failedTests);
        runTest("Decay", testDecay, failedTests);
        runTest("Select", testSelect, failedTests);
        runTest("RandomInt", testRandomInt, failedTests);
        runTest("Random", testRandom, failedTests);
        runTest("Randomizer", testRandomizer, failedTests);
        runTest("RandomOrderInt", testRandomOrderInt, failedTests);
        runTest("same seed repeats", testSameSeedRepeats, failedTests);
        runTest("Markov", testMarkov, failedTests);
        runTest("RandomDensity", testRandomDensity, failedTests);
        runTest("MakeEnvelope", testMakeEnvelope, failedTests);
        runTest("EnvLib", testEnvLib, failedTests);
        runTest("Stochos FUNCTIONS", testStochosFunctions, failedTests);
        runTest("Stochos RANGE_DISTRIB", testStochosRange, failedTests);
        runTest("sieve elements", testSieveElements, failedTests);
        runTest("sieve weights", testSieveWeights, failedTests);
        runTest("sieve errors", testSieveErrors, failedTests);
        runTest("ChooseL", testChooseL, failedTests);
        runTest("ValuePick", testValuePick, failedTests);
        runTest("MakePattern", testMakePattern, failedTests);
        runTest("ExpandPattern", testExpandPattern, failedTests);
        runTest("event-only functions need an event", testEventOnlyFunctionsNeedAnEvent, failedTests);
        runTest("removeSpaces", testRemoveSpaces, failedTests);
        cmod = nullptr;
    } catch (const std::exception& e) {
        std::cerr << "FAILED: could not load the test project: " << e.what() << '\n';
        failedTests.push_back("load test project");
    }

    fs::current_path(previous);
    fs::remove_all(folder);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " CMOD function test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }
    std::cerr << "\nAll CMOD function tests passed\n";
    return 0;
}
