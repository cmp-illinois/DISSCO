// Tests for LASS envelopes: the shapes that drive loudness, frequency,
// spatialization and most other sound parameters over time. Each test checks
// what an envelope *produces* (its values at given times, sample by sample,
// after copying or saving), not how it computes them. Run through ctest
// (LASS.Envelopes). -- Tony Chai, 2026
#include "StandardHeaders.h"
#include "Envelope.h"
#include "EnvelopeLibrary.h"
#include "Types.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// Name of the test currently running, so every failure message says which
// test it came from.
std::string currentTest;
int currentTestFailures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED [" << currentTest << "]: " << message << '\n';
        ++currentTestFailures;
    }
}

// Envelope values are floats, and getValue() reads from a table with 100
// steps per segment, so most checks allow a small difference.
void expectNear(double actual, double expected, const std::string& message,
                double tolerance = 1.0e-4)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED [" << currentTest << "]: " << message
                  << ": expected " << expected << ", got " << actual << '\n';
        ++currentTestFailures;
    }
}

// Thrown to stop just the current test when the following checks can't run.
struct RequirementFailed {
    std::string message;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw RequirementFailed{message};
}

// Builds an envelope through the given (x, y) points. Every segment uses
// `interp`; `stretch` gives each segment's length type (all FLEXIBLE if empty).
Envelope makeEnvelope(const std::vector<xy_point>& points,
                      interpolation_type interp = LINEAR,
                      std::vector<stretch_type> stretch = {})
{
    std::vector<envelope_segment> segments;
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        envelope_segment segment{};
        segment.interType = interp;
        segment.lengthType = i < stretch.size() ? stretch[i] : FLEXIBLE;
        segments.push_back(segment);
    }
    return Envelope(points, segments);
}

// Plays an envelope the way a partial does (duration and sampling rate set,
// then read through valueIterator) and returns the first `count` samples.
std::vector<double> playSamples(Envelope& envelope, double duration, int rate, int count)
{
    envelope.setDuration(static_cast<m_time_type>(duration));
    envelope.setSamplingRate(rate);
    Iterator<m_value_type> iterator = envelope.valueIterator();
    std::vector<double> samples;
    while (static_cast<int>(samples.size()) < count && iterator.hasNext())
        samples.push_back(iterator.next());
    return samples;
}

std::string text(double value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

// A unique path in the system temp folder, removed when the test ends.
struct TempFile {
    std::filesystem::path path;
    explicit TempFile(const std::string& name)
        : path(std::filesystem::temp_directory_path()
               / ("lass-envelope-test-" + name + "-"
                  + std::to_string(reinterpret_cast<std::uintptr_t>(this))))
    {}
    ~TempFile() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    char* c_str() { pathString = path.string(); return pathString.data(); }
private:
    std::string pathString;
};

// ---------------------------------------------------------------------------
// Reading values

// A straight ramp reads back the value on the line at any time, and the times
// scale with the total length it's played over.
void testLinearRampFollowsItsPoints()
{
    Envelope ramp = makeEnvelope({{0, 0}, {1, 1}});
    expectNear(ramp.getValue(0, 1), 0.0, "ramp at its start");
    expectNear(ramp.getValue(0.25f, 1), 0.25, "ramp a quarter of the way");
    expectNear(ramp.getValue(0.5f, 1), 0.5, "ramp halfway");
    expectNear(ramp.getValue(1, 1), 1.0, "ramp at its end");
    expectNear(ramp.getValue(1, 4), 0.25, "ramp played over 4 s, at 1 s");
    expectNear(ramp.getValue(2, 4), 0.5, "ramp played over 4 s, at 2 s");
}

// An up-then-down shape reaches its peak exactly at the middle point.
void testMultiSegmentShapePeaksAtItsBreakpoint()
{
    Envelope triangle = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0}});
    expectNear(triangle.getValue(0.25f, 1), 0.5, "halfway up the first segment");
    expectNear(triangle.getValue(0.5f, 1), 1.0, "at the peak point");
    expectNear(triangle.getValue(0.75f, 1), 0.5, "halfway down the second segment");
    expectNear(triangle.getValue(1, 1), 0.0, "at the end point");
}

// FLEXIBLE segments keep their share of the total length: a point at x=0.25
// lands at 25% of whatever duration the envelope is played over.
void testFlexibleSegmentsStretchToTheDuration()
{
    Envelope envelope = makeEnvelope({{0, 0}, {0.25f, 1}, {1, 0}});
    expectNear(envelope.getValue(1, 4), 1.0, "peak at 1 s of a 4 s sound (25%)");
    expectNear(envelope.getValue(2.5f, 10), 1.0, "peak at 2.5 s of a 10 s sound (25%)");
    expectNear(envelope.getValue(7, 10), 0.4, "partway down the flexible release of a 10 s sound");
}

// A FIXED segment keeps its length in seconds whatever the duration, and the
// FLEXIBLE segments share what's left (e.g. a 0.2 s attack on any note).
void testFixedSegmentKeepsItsLengthInSeconds()
{
    Envelope envelope = makeEnvelope({{0, 0}, {0.2f, 1}, {1, 0}}, LINEAR, {FIXED, FLEXIBLE});
    expectNear(envelope.getValue(0.2f, 2), 1.0, "2 s sound: peak right after the 0.2 s attack");
    expectNear(envelope.getValue(1.1f, 2), 0.5, "2 s sound: halfway through the 1.8 s release");
    expectNear(envelope.getValue(0.2f, 4), 1.0, "4 s sound: peak still at 0.2 s");
    expectNear(envelope.getValue(2.1f, 4), 0.5, "4 s sound: halfway through the 3.8 s release");
}

// Playing an envelope over a duration shorter than its FIXED part must not
// change how it plays at other durations afterwards.
void testShortDurationDoesNotChangeTheEnvelope()
{
    Envelope envelope = makeEnvelope({{0, 0}, {0.2f, 1}, {1, 0}}, LINEAR, {FIXED, FLEXIBLE});
    expectNear(envelope.getValue(0.2f, 2), 1.0, "2 s sound before any short use: peak at 0.2 s");
    envelope.getValue(0.05f, 0.1f);  // a 0.1 s sound, shorter than the 0.2 s attack
    expectNear(envelope.getValue(0.2f, 2), 1.0, "2 s sound after a 0.1 s use: peak still at 0.2 s");
    expectNear(envelope.getValue(1.1f, 2), 0.5, "2 s sound after a 0.1 s use: release still 1.8 s long");
}

// When every segment is FIXED but they don't add up to the duration, the
// whole shape is scaled to fit, keeping the points in proportion.
void testAllFixedSegmentsScaleToFit()
{
    Envelope envelope = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0}}, LINEAR, {FIXED, FIXED});
    expectNear(envelope.getValue(1, 2), 1.0, "2 s sound: peak moves to 1 s");
    expectNear(envelope.getValue(0.5f, 2), 0.5, "2 s sound: halfway up at 0.5 s");
    expectNear(envelope.getValue(1.5f, 2), 0.5, "2 s sound: halfway down at 1.5 s");
}

// The exponential curve LASS draws from y1 to y2, at fraction i of the
// segment: y1 + (y2 - y1) * (1 - e^(a*i)) / (1 - e^a), with a = 3 for a rise
// and -3 for a fall.
double exponentialCurve(double y1, double y2, double i)
{
    const double a = y1 > y2 ? -3.0 : 3.0;
    return y1 + (y2 - y1) * (1 - std::exp(a * i)) / (1 - std::exp(a));
}

// Exponential segments curve: a rise stays low for most of its length and
// shoots up at the end; a fall drops quickly and then flattens out. Each
// value may be the curve at that point or one table step (1%) later, but
// not a different curve.
void testExponentialCurvesBendTheRightWay()
{
    auto expectOnCurve = [](Envelope& envelope, double y1, double y2, double i,
                            const std::string& message) {
        const double actual = envelope.getValue(static_cast<m_value_type>(i), 1);
        const double a = exponentialCurve(y1, y2, i);
        const double b = exponentialCurve(y1, y2, std::min(i + 0.01, 1.0));
        const double tolerance = 2.0e-3;  // y=0 is drawn as 0.0001, plus float rounding
        if (actual < std::min(a, b) - tolerance || actual > std::max(a, b) + tolerance)
            expect(false, message + ": expected between " + text(std::min(a, b)) + " and "
                              + text(std::max(a, b)) + ", got " + text(actual));
    };

    Envelope rise = makeEnvelope({{0, 0}, {1, 1}}, EXPONENTIAL);
    expectOnCurve(rise, 0, 1, 0, "exponential rise at its start");
    expectOnCurve(rise, 0, 1, 0.5, "exponential rise halfway (well below a straight line's 0.5)");
    expectOnCurve(rise, 0, 1, 0.9, "exponential rise at 90%");
    expectNear(rise.getValue(1, 1), 1.0, "exponential rise at its end");

    Envelope fall = makeEnvelope({{0, 1}, {1, 0}}, EXPONENTIAL);
    expectOnCurve(fall, 1, 0, 0, "exponential fall at its start");
    expectOnCurve(fall, 1, 0, 0.5, "exponential fall halfway (well below a straight line's 0.5)");
    expectOnCurve(fall, 1, 0, 0.9, "exponential fall at 90%");
    expectNear(fall.getValue(1, 1), 0.0, "exponential fall at its end", 1.0e-3);
}

// ---------------------------------------------------------------------------
// Playing sample by sample (what a partial actually hears)

// Played at 100 samples per second for 1 s, a ramp gives one value per
// sample along the line, starting from its first point.
void testValueIteratorPlaysTheShapeSampleBySample()
{
    Envelope ramp = makeEnvelope({{0, 0}, {1, 1}});
    std::vector<double> samples = playSamples(ramp, 1, 100, 100);
    require(samples.size() == 100,
            "the iterator should give at least 100 samples for 1 s at 100 Hz, got "
                + std::to_string(samples.size()));
    for (int k : {0, 1, 25, 50, 75, 99})
        expectNear(samples[k], k / 100.0, "sample " + std::to_string(k) + " of the ramp", 1.0e-3);
}

// Sample-by-sample playback and getValue() describe the same shape, including
// across a FIXED attack and over a duration that isn't 1 s. (Playback starts
// each segment on a whole sample, so the rate is high enough for that
// rounding not to matter here.)
void testValueIteratorMatchesGetValue()
{
    const double duration = 2;
    const int rate = 1000;
    Envelope played = makeEnvelope({{0, 0}, {0.25f, 1}, {0.5f, 0.6f}, {1, 0}}, LINEAR,
                                   {FIXED, FLEXIBLE, FLEXIBLE});
    Envelope looked = makeEnvelope({{0, 0}, {0.25f, 1}, {0.5f, 0.6f}, {1, 0}}, LINEAR,
                                   {FIXED, FLEXIBLE, FLEXIBLE});
    std::vector<double> samples = playSamples(played, duration, rate, rate * 2);
    require(samples.size() == static_cast<std::size_t>(rate * 2),
            "the iterator should give " + std::to_string(rate * 2) + " samples, got "
                + std::to_string(samples.size()));
    int mismatches = 0;
    for (int k = 0; k < rate * 2; k += 50) {
        const double t = static_cast<double>(k) / rate;
        const double expected = looked.getValue(static_cast<m_value_type>(t),
                                                static_cast<m_value_type>(duration));
        if (std::abs(samples[k] - expected) > 0.021 && mismatches++ < 5)
            expect(false, "at " + text(t) + " s the iterator gives " + text(samples[k])
                              + " but getValue gives " + text(expected));
    }
}

// A copy of a playback iterator continues from the same place as the
// original, and the two then play independently (and both can be destroyed).
void testCopiedIteratorContinuesIndependently()
{
    Envelope ramp = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0}});
    ramp.setDuration(1);
    ramp.setSamplingRate(100);
    Iterator<m_value_type> original = ramp.valueIterator();
    for (int k = 0; k < 30; ++k)
        original.next();
    {
        Iterator<m_value_type> copy(original);
        for (int k = 30; k < 70; ++k) {
            const double fromCopy = copy.next();
            if (std::abs(fromCopy - (k <= 50 ? k / 50.0 : (100 - k) / 50.0)) > 1.0e-3) {
                expect(false, "copy at sample " + std::to_string(k) + " gives " + text(fromCopy));
                break;
            }
        }
    }
    expectNear(original.next(), 0.6, "original at sample 30, after the copy played on and was destroyed", 1.0e-3);
}

// ---------------------------------------------------------------------------
// Changing and copying envelopes

// scale() multiplies every point, and getMaxValue() reports the new peak.
void testScaleMultipliesEveryPoint()
{
    Envelope triangle = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0.4f}});
    expectNear(triangle.getMaxValue(), 1.0, "peak before scaling");
    triangle.scale(2.5f);
    expectNear(triangle.getValue(0.5f, 1), 2.5, "peak value after scale(2.5)");
    expectNear(triangle.getValue(1, 1), 1.0, "end value after scale(2.5)");
    expectNear(triangle.getMaxValue(), 2.5, "getMaxValue after scale(2.5)");
}

// getMaxValue() reports the highest point, even when every point is negative.
void testMaxValueOfANegativeEnvelope()
{
    Envelope negative = makeEnvelope({{0, -3}, {0.5f, -1}, {1, -2}});
    expectNear(negative.getMaxValue(), -1.0, "highest point of an envelope from -3 to -1 to -2");
}

// getScaledValueNew(t, k) reads the value at t times k. Asking twice must give
// the same answer (the envelope itself must not keep getting scaled).
void testScaledValueDoesNotAccumulate()
{
    Envelope flat = makeEnvelope({{0, 1}, {1, 1}});
    expectNear(flat.getScaledValueNew(0.5, 2), 2.0, "first getScaledValueNew(0.5, 2)");
    expectNear(flat.getScaledValueNew(0.5, 2), 2.0, "second getScaledValueNew(0.5, 2)");
    expectNear(flat.getValue(0.5f, 1), 1.0, "the envelope itself after getScaledValueNew");
}

// clone() and the copy constructor give independent envelopes with the same
// shape; changing one leaves the other alone.
void testCopiesAreIndependent()
{
    Envelope original = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0}}, LINEAR, {FIXED, FLEXIBLE});
    std::unique_ptr<Envelope> cloned(original.clone());
    Envelope copied(original);

    expectNear(cloned->getValue(0.5f, 2), 1.0, "clone keeps the FIXED attack (peak at 0.5 s of 2 s)");
    expectNear(copied.getValue(0.5f, 2), 1.0, "copy keeps the FIXED attack (peak at 0.5 s of 2 s)");

    cloned->scale(3);
    copied.setPoint(1, xy_point(0.5f, 0.2f));
    expectNear(original.getValue(0.5f, 1), 1.0, "original peak after changing the clone and the copy");
    expectNear(cloned->getValue(0.5f, 1), 3.0, "clone peak after scale(3)");
    expectNear(copied.getValue(0.5f, 1), 0.2, "copy peak after setPoint");
}

// A copy of an envelope that has already been played plays the same shape
// when it's played at the same duration and rate.
void testCopyOfAPlayedEnvelopePlays()
{
    Envelope ramp = makeEnvelope({{0, 0}, {1, 1}});
    playSamples(ramp, 1, 100, 100);
    Envelope copy(ramp);
    const std::vector<double> samples = playSamples(copy, 1, 100, 100);
    require(samples.size() == 100, "the copy should give 100 samples, got "
                                       + std::to_string(samples.size()));
    expectNear(samples[50], 0.5, "the copy's sample 50", 1.0e-3);
    expectNear(samples[99], 0.99, "the copy's sample 99", 1.0e-3);
}

// addToShape() appends another envelope after the end of this one.
void testAddToShapeAppendsAfterTheEnd()
{
    Envelope attack = makeEnvelope({{0, 0}, {1, 1}});
    Envelope release = makeEnvelope({{0, 1}, {1, 0}});
    attack.addToShape(&release);
    expectNear(attack.getPoint(2).x, 2.0, "appended end point moves to x=2");
    expectNear(attack.getValue(1, 2), 1.0, "joined shape peaks where the two meet");
    expectNear(attack.getValue(1.5f, 2), 0.5, "halfway down the appended part");
    expectNear(attack.getValue(2, 2), 0.0, "joined shape ends at the appended end value");
}

// multiply() gives an envelope whose value at each point of either input is
// the product of the two inputs there.
void testMultiplyGivesTheProductAtEachPoint()
{
    Envelope triangle = makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0.5f}});
    Envelope ramp = makeEnvelope({{0, 0.5f}, {0.25f, 1}, {1, 2}});
    std::unique_ptr<Envelope> product(triangle.multiply(triangle, ramp));
    require(product != nullptr, "multiply returned nullptr");
    const std::vector<xy_point> points = product->getPoints();
    require(points.size() == 4,
            "the product should have a point at x = 0, 0.25, 0.5 and 1, got "
                + std::to_string(points.size()) + " points");
    const double expectedX[] = {0, 0.25, 0.5, 1};
    // triangle * ramp at each x: 0*0.5, 0.5*1, 1*(1+1/3), 0.5*2
    const double expectedY[] = {0, 0.5, 4.0 / 3.0, 1};
    for (int i = 0; i < 4; ++i) {
        expectNear(points.at(i).x, expectedX[i], "product point " + std::to_string(i) + " x", 1.0e-3);
        expectNear(points.at(i).y, expectedY[i], "product point " + std::to_string(i) + " y", 0.02);
    }
}

// ---------------------------------------------------------------------------
// The envelope library (how projects store their envelopes)

// Every envelope in the library text is loaded, whether or not the text ends
// with a line break (a hand-edited project may not have one).
void testLibraryLoadsEveryEnvelope()
{
    const std::string twoEnvelopes =
        "Envelope 1\n0 0 LINEAR FLEXIBLE\n1 1\n"
        "Envelope 2\n0 1 EXPONENTIAL FIXED\n1 0";
    for (const bool trailingNewline : {true, false}) {
        const std::string label = trailingNewline ? "with a final line break"
                                                  : "without a final line break";
        TempFile file("load");
        {
            std::ofstream out(file.path);
            out << twoEnvelopes << (trailingNewline ? "\n" : "");
        }
        EnvelopeLibrary library;
        const int loaded = library.loadLibraryNewFormat(file.c_str());
        expect(loaded == 2, label + ": expected 2 envelopes, got " + std::to_string(loaded));
        if (loaded != 2)
            continue;
        std::unique_ptr<Envelope> second(library.getEnvelope(2));
        expectNear(second->getValue(0, 1), 1.0, label + ": envelope 2 starts at 1", 0.035);
        expect(second->getSegmentInterpolationType(0) == EXPONENTIAL,
               label + ": envelope 2 keeps its EXPONENTIAL segment");
        expect(second->getSegmentLengthType(0) == FIXED,
               label + ": envelope 2 keeps its FIXED segment");
    }
}

// Saving a library and loading the file back gives the same envelopes.
void testLibrarySaveThenLoadKeepsEnvelopes()
{
    EnvelopeLibrary saved;
    saved.addEnvelope(new Envelope(makeEnvelope({{0, 0}, {0.3f, 1}, {1, 0.25f}}, LINEAR,
                                                {FIXED, FLEXIBLE})));
    saved.addEnvelope(new Envelope(makeEnvelope({{0, 1}, {1, 0}}, CUBIC_SPLINE)));

    TempFile file("save");
    require(saved.saveLibrary(file.c_str()), "saveLibrary reported failure");
    EnvelopeLibrary loaded;
    require(loaded.loadLibrary(file.c_str()) == 2, "the saved file should load 2 envelopes");

    for (int number = 1; number <= 2; ++number) {
        std::unique_ptr<Envelope> before(saved.getEnvelope(number));
        std::unique_ptr<Envelope> after(loaded.getEnvelope(number));
        const std::vector<xy_point> beforePoints = before->getPoints();
        const std::vector<xy_point> afterPoints = after->getPoints();
        const std::string label = "envelope " + std::to_string(number);
        if (beforePoints.size() != afterPoints.size()) {
            expect(false, label + ": point count changed from "
                              + std::to_string(beforePoints.size()) + " to "
                              + std::to_string(afterPoints.size()));
            continue;
        }
        for (std::size_t i = 0; i < beforePoints.size(); ++i) {
            const std::string point = label + " point " + std::to_string(i);
            expectNear(afterPoints.at(i).x, beforePoints.at(i).x, point + " x", 1.0e-3);
            expectNear(afterPoints.at(i).y, beforePoints.at(i).y, point + " y", 1.0e-3);
        }
        for (int s = 0; s + 1 < static_cast<int>(beforePoints.size()); ++s) {
            const std::string segment = label + " segment " + std::to_string(s);
            expect(after->getSegmentInterpolationType(s) == before->getSegmentInterpolationType(s),
                   segment + " interpolation type changed");
            expect(after->getSegmentLengthType(s) == before->getSegmentLengthType(s),
                   segment + " FIXED/FLEXIBLE changed");
        }
    }
}

// getEnvelope() hands out a copy, so changing it can't change the library;
// asking for a number past the end gives nullptr instead of crashing.
void testLibraryHandsOutIndependentCopies()
{
    EnvelopeLibrary library;
    library.addEnvelope(new Envelope(makeEnvelope({{0, 0}, {1, 1}})));
    {
        std::unique_ptr<Envelope> borrowed(library.getEnvelope(1));
        borrowed->scale(5);
    }
    std::unique_ptr<Envelope> again(library.getEnvelope(1));
    expectNear(again->getValue(1, 1), 1.0, "library envelope after a copy of it was scaled");
    expect(library.getEnvelope(2) == nullptr, "getEnvelope past the end should give nullptr");
}

// Assigning one library to another copies its envelopes, so the copy still
// works after the original is gone.
void testLibraryAssignmentCopiesEnvelopes()
{
    EnvelopeLibrary copy;
    {
        EnvelopeLibrary original;
        original.addEnvelope(new Envelope(makeEnvelope({{0, 0}, {0.5f, 1}, {1, 0}})));
        copy = original;
    }
    expect(copy.size() == 1, "the assigned library should hold 1 envelope, got "
                                 + std::to_string(copy.size()));
    if (copy.size() == 1) {
        std::unique_ptr<Envelope> envelope(copy.getEnvelope(1));
        expectNear(envelope->getValue(0.5f, 1), 1.0, "assigned envelope's peak after the original is gone");
    }
}

// Runs one test and reports it by name. An unexpected exception counts as a
// failure of that test only, so the remaining tests still run. Everything goes
// to std::cerr (unbuffered), so if a test crashes outright, the last
// "RUN" line in the log names the test that crashed.
void runTest(const char* name, void (*test)(), std::vector<std::string>& failedTests)
{
    currentTest = name;
    currentTestFailures = 0;
    std::cerr << "RUN  " << name << '\n';

    try {
        test();
    } catch (const RequirementFailed& e) {
        std::cerr << "FAILED [" << name << "]: " << e.message
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

    runTest("testLinearRampFollowsItsPoints", testLinearRampFollowsItsPoints, failedTests);
    runTest("testMultiSegmentShapePeaksAtItsBreakpoint", testMultiSegmentShapePeaksAtItsBreakpoint, failedTests);
    runTest("testFlexibleSegmentsStretchToTheDuration", testFlexibleSegmentsStretchToTheDuration, failedTests);
    runTest("testFixedSegmentKeepsItsLengthInSeconds", testFixedSegmentKeepsItsLengthInSeconds, failedTests);
    runTest("testShortDurationDoesNotChangeTheEnvelope", testShortDurationDoesNotChangeTheEnvelope, failedTests);
    runTest("testAllFixedSegmentsScaleToFit", testAllFixedSegmentsScaleToFit, failedTests);
    runTest("testExponentialCurvesBendTheRightWay", testExponentialCurvesBendTheRightWay, failedTests);
    runTest("testValueIteratorPlaysTheShapeSampleBySample", testValueIteratorPlaysTheShapeSampleBySample, failedTests);
    runTest("testValueIteratorMatchesGetValue", testValueIteratorMatchesGetValue, failedTests);
    runTest("testCopiedIteratorContinuesIndependently", testCopiedIteratorContinuesIndependently, failedTests);
    runTest("testScaleMultipliesEveryPoint", testScaleMultipliesEveryPoint, failedTests);
    runTest("testMaxValueOfANegativeEnvelope", testMaxValueOfANegativeEnvelope, failedTests);
    runTest("testScaledValueDoesNotAccumulate", testScaledValueDoesNotAccumulate, failedTests);
    runTest("testCopiesAreIndependent", testCopiesAreIndependent, failedTests);
    runTest("testCopyOfAPlayedEnvelopePlays", testCopyOfAPlayedEnvelopePlays, failedTests);
    runTest("testAddToShapeAppendsAfterTheEnd", testAddToShapeAppendsAfterTheEnd, failedTests);
    runTest("testMultiplyGivesTheProductAtEachPoint", testMultiplyGivesTheProductAtEachPoint, failedTests);
    runTest("testLibraryLoadsEveryEnvelope", testLibraryLoadsEveryEnvelope, failedTests);
    runTest("testLibrarySaveThenLoadKeepsEnvelopes", testLibrarySaveThenLoadKeepsEnvelopes, failedTests);
    runTest("testLibraryHandsOutIndependentCopies", testLibraryHandsOutIndependentCopies, failedTests);
    runTest("testLibraryAssignmentCopiesEnvelopes", testLibraryAssignmentCopiesEnvelopes, failedTests);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " LASS envelope test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }

    std::cerr << "\nAll LASS envelope tests passed\n";
    return 0;
}
