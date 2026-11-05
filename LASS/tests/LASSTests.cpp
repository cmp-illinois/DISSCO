// Basic unit tests for LASS: each test checks one common or crash-prone
// behavior (bad input, ownership/copies, NaN-producing math). Run with
// `ctest --test-dir build`. -- Tony Chai, 2026
#include "StandardHeaders.h"
#include "Constant.h"
#include "Envelope.h"
#include "MarkovModel.h"
#include "MultiPan.h"
#include "MultiTrack.h"
#include "Pan.h"
#include "Reverb.h"
#include "Sound.h"
#include "SoundSample.h"
#include "Spatializer.h"
#include "Track.h"
#include "Types.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
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

void expectNear(double actual, double expected, const std::string& message)
{
    // m_value_type (the storage behind getParam/setParam) is float, so a
    // round trip through it loses more precision than a double-only
    // tolerance would allow.
    constexpr double tolerance = 1.0e-6;
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED [" << currentTest << "]: " << message
                  << ": expected " << expected << ", got " << actual << '\n';
        ++currentTestFailures;
    }
}

// Expects two multi-channel results to hold the same samples.
void expectSameChannels(MultiTrack& actual, MultiTrack& expected, const std::string& what)
{
    if (actual.size() != expected.size()) {
        expect(false, what + ": " + std::to_string(actual.size()) + " channels instead of "
                          + std::to_string(expected.size()));
        return;
    }
    for (int c = 0; c < expected.size(); c++) {
        SoundSample& a = actual.get(c)->getWave();
        SoundSample& e = expected.get(c)->getWave();
        if (a.getSampleCount() != e.getSampleCount()) {
            expect(false, what + ": channel " + std::to_string(c) + " has "
                              + std::to_string(a.getSampleCount()) + " samples instead of "
                              + std::to_string(e.getSampleCount()));
            continue;
        }
        for (m_sample_count_type i = 0; i < e.getSampleCount(); i++)
            expectNear(a[i], e[i], what + ": channel " + std::to_string(c) + " sample "
                                       + std::to_string(i));
    }
}

// MultiPan::clone() must deep-copy: once the original is deleted, the clone
// still spreads a track over the channels exactly as the original did.
// (This used to only check that nothing crashed.) -- Tony Chai, 2026
void testMultiPanCloneIsIndependent()
{
    constexpr int channels = 4;
    Track track(8, 44100, true);
    for (m_sample_count_type i = 0; i < 8; i++)
        track.getWave()[i] = static_cast<float>(i + 1);

    MultiPan* original = new MultiPan(channels);
    original->addEntryLocation(0.0f, 0.0f, 1.0f);
    original->addEntryLocation(1.0f, static_cast<float>(M_PI / 2.0), 0.5f);
    original->doneAddEntryLocation();
    MultiPan* clone = original->clone();

    std::unique_ptr<MultiTrack> expected(original->spatialize_Track(track, channels));
    delete original;
    std::unique_ptr<MultiTrack> actual(clone->spatialize_Track(track, channels));
    delete clone;

    double loudest = 0.0;
    for (int c = 0; c < expected->size(); c++)
        for (m_sample_count_type i = 0; i < 8; i++)
            loudest = std::max(loudest, std::abs(static_cast<double>(expected->get(c)->getWave()[i])));
    expect(loudest > 0.0, "the original MultiPan produced silence, so the comparison proves nothing");
    expectSameChannels(*actual, *expected, "clone used after the original was deleted");
}

// Sound::setDetune must reject invalid values and canonicalize direction to +/-1.
void testSoundSetDetuneValidation()
{
    {
        Sound sound;
        sound.setDetune(/*direction=*/0.5, /*spread=*/0.3, /*velocity=*/-0.2);
        expectNear(sound.getParam(DETUNE_DIRECTION), 1.0,
                   "positive direction should canonicalize to +1.0");
        expectNear(sound.getParam(DETUNE_SPREAD), 0.3,
                   "valid spread should be stored as-is");
        expectNear(sound.getParam(DETUNE_VELOCITY), -0.2,
                   "valid velocity should be stored as-is");
    }
    {
        Sound sound;
        sound.setDetune(/*direction=*/-0.7, /*spread=*/0.1, /*velocity=*/0.1);
        expectNear(sound.getParam(DETUNE_DIRECTION), -1.0,
                   "negative direction should canonicalize to -1.0");
    }
    // Rejection cases compare against the value just before the call, since
    // Sound's constructor pre-seeds non-zero DETUNE defaults (not 0.0).
    {
        Sound sound;
        const double before = sound.getParam(DETUNE_SPREAD);
        sound.setDetune(/*direction=*/0.5, /*spread=*/1.5, /*velocity=*/0.2);
        expectNear(sound.getParam(DETUNE_SPREAD), before,
                   "out-of-range spread must be rejected, leaving the param untouched");
    }
    {
        Sound sound;
        const double before = sound.getParam(DETUNE_DIRECTION);
        sound.setDetune(/*direction=*/0.0, /*spread=*/0.5, /*velocity=*/0.2);
        expectNear(sound.getParam(DETUNE_DIRECTION), before,
                   "zero direction is not a valid sign and must be rejected");
    }
    {
        Sound sound;
        const double before = sound.getParam(DETUNE_DIRECTION);
        sound.setDetune(/*direction=*/std::numeric_limits<double>::quiet_NaN(),
                         /*spread=*/0.5, /*velocity=*/0.2);
        expectNear(sound.getParam(DETUNE_DIRECTION), before,
                   "non-finite direction must be rejected, leaving the param untouched");
    }
    {
        Sound sound;
        const double before = sound.getParam(DETUNE_VELOCITY);
        sound.setDetune(/*direction=*/0.5, /*spread=*/0.2, /*velocity=*/-2.0);
        expectNear(sound.getParam(DETUNE_VELOCITY), before,
                   "out-of-range velocity must be rejected, leaving the param untouched");
    }
    {
        // All-zero is a silent no-op sentinel.
        Sound sound;
        const double before = sound.getParam(DETUNE_SPREAD);
        sound.setDetune(0.0, 0.0, 0.0);
        expectNear(sound.getParam(DETUNE_SPREAD), before,
                   "all-zero sentinel should not error, crash, or change existing params");
    }
}

// spatialize_MultiTrack: output channels should equal the average of all input tracks.
void testSpatializerMultiTrackComposite()
{
    constexpr m_sample_count_type sampleCount = 4;
    constexpr m_rate_type samplingRate = 44100;
    constexpr int numOutputTracks = 2;
    const float samplesA[sampleCount] = {1.0f, 2.0f, -1.0f, 0.5f};
    const float samplesB[sampleCount] = {0.0f, -2.0f, 3.0f, 1.0f};

    MultiTrack input;
    Track* trackA = new Track(sampleCount, samplingRate, true);
    Track* trackB = new Track(sampleCount, samplingRate, true);
    for (m_sample_count_type i = 0; i < sampleCount; i++) {
        trackA->getWave()[i] = samplesA[i];
        trackB->getWave()[i] = samplesB[i];
    }
    input.add(trackA);
    input.add(trackB);

    Spatializer spatializer;
    MultiTrack* result = spatializer.spatialize_MultiTrack(
        input, numOutputTracks, sampleCount, samplingRate);

    expect(result->size() == numOutputTracks,
           "spatialize_MultiTrack output channel count should match numTracks");
    for (int c = 0; c < numOutputTracks && c < result->size(); c++) {
        SoundSample& outWave = result->get(c)->getWave();
        for (m_sample_count_type i = 0; i < sampleCount; i++) {
            const double expected =
                (samplesA[i] + samplesB[i]) / numOutputTracks;
            expectNear(outWave[i], expected,
                       "channel " + std::to_string(c) + " sample " + std::to_string(i)
                           + " should be the average of the input tracks");
        }
    }
    delete result;
}

// The basic spatializer spreads one track evenly over every channel, and each
// channel is its own copy (changing one leaves the others alone).
void testSpatializerSpreadsTrackOverIndependentChannels()
{
    constexpr m_sample_count_type sampleCount = 4;
    const float samples[sampleCount] = {1.0f, -2.0f, 3.0f, 0.5f};
    Track input(sampleCount, 44100, true);
    for (m_sample_count_type i = 0; i < sampleCount; i++)
        input.getWave()[i] = samples[i];

    for (int channels : {1, 2, 3}) {
        Spatializer spatializer;
        std::unique_ptr<MultiTrack> result(spatializer.spatialize_Track(input, channels));
        const std::string label = std::to_string(channels) + " channel(s)";
        expect(result->size() == channels, label + ": wrong channel count");
        if (result->size() != channels)
            continue;
        for (int c = 0; c < channels; c++)
            for (m_sample_count_type i = 0; i < sampleCount; i++)
                expectNear(result->get(c)->getWave()[i], samples[i] / channels,
                           label + ": channel " + std::to_string(c) + " sample "
                               + std::to_string(i));
        result->get(channels - 1)->getWave()[0] = 99.0f;
        for (int c = 0; c + 1 < channels; c++)
            expectNear(result->get(c)->getWave()[0], samples[0] / channels,
                       label + ": channel " + std::to_string(c)
                           + " changed when the last channel was changed");
    }
    expectNear(input.getWave()[0], samples[0], "the input track must not change");
}

// Reverb keeps its own copy of the reverb-amount envelope: deleting the
// caller's envelope right after making the Reverb must not change what it
// produces. It's compared with the same Reverb whose envelope stays alive.
// (This used to only check that nothing crashed.) -- Tony Chai, 2026
void testReverbOwnsItsOwnEnvelopeCopy()
{
    auto makeAmount = [] {
        vector<xy_point> points = {xy_point(0.0f, 0.5f), xy_point(1.0f, 0.5f)};
        envelope_segment seg{};
        seg.interType = LINEAR;
        seg.lengthType = FLEXIBLE;
        seg.timeValue = 1.0f;
        vector<envelope_segment> segs = {seg};
        return new Envelope(points, segs);
    };
    // A click followed by silence, so the reverb tail is easy to see.
    Track click(4410, 44100, true);
    click.getWave()[0] = 1.0f;

    Envelope* keptAmount = makeAmount();
    Reverb reference(keptAmount, 0.5f, 0.7f, 0.03f, 44100);
    Envelope* deletedAmount = makeAmount();
    Reverb reverb(deletedAmount, 0.5f, 0.7f, 0.03f, 44100);
    delete deletedAmount;

    std::unique_ptr<Track> expected(&reference.do_reverb_Track(click));
    std::unique_ptr<Track> actual(&reverb.do_reverb_Track(click));
    delete keptAmount;

    SoundSample& e = expected->getWave();
    SoundSample& a = actual->getWave();
    double tail = 0.0;
    for (m_sample_count_type i = 1; i < e.getSampleCount(); i++)
        tail = std::max(tail, std::abs(static_cast<double>(e[i])));
    expect(tail > 1e-4, "the reverb added no tail after the click");
    if (a.getSampleCount() != e.getSampleCount()) {
        expect(false, "output has " + std::to_string(a.getSampleCount())
                          + " samples instead of " + std::to_string(e.getSampleCount()));
        return;
    }
    for (m_sample_count_type i = 0; i < e.getSampleCount(); i++)
        expectNear(a[i], e[i], "sample " + std::to_string(i)
                                   + " changed when the caller's envelope was deleted");
}

// Sound(numPartials, freq) should handle numPartials <= 0 safely, not crash.
void testSoundInvalidPartialCountIsSafe()
{
    Sound zero(0, 440.0);
    expect(zero.size() == 0, "Sound(0, freq) should safely produce zero partials, not crash");

    Sound negative(-3, 440.0);
    expect(negative.size() == 0, "Sound(negative, freq) should safely produce zero partials, not crash");

    Sound valid(4, 440.0);
    expect(valid.size() == 4, "Sound(4, freq) should produce exactly 4 partials");
}

// A flat two-point envelope should read back the same constant value everywhere.
void testEnvelopeConstantValueInterpolation()
{
    vector<xy_point> points = {xy_point(0.0f, 0.3f), xy_point(1.0f, 0.3f)};
    envelope_segment seg{};
    seg.interType = LINEAR;
    seg.lengthType = FLEXIBLE;
    seg.timeValue = 1.0f;
    vector<envelope_segment> segs = {seg};

    Envelope envelope(points, segs);

    const double totalLength = 1.0;
    expectNear(envelope.getValue(0.0, totalLength), 0.3,
               "flat envelope should read its constant value at the start");
    expectNear(envelope.getValue(0.5, totalLength), 0.3,
               "flat envelope should read its constant value at the midpoint");
    expectNear(envelope.getValue(1.0, totalLength), 0.3,
               "flat envelope should read its constant value at the end (the exact-endpoint fast path)");
}

// normalize() should turn rows into proper distributions; nextSample() should be stateful.
void testMarkovModelNormalizeAndSample()
{
    // Distinct int payloads so a returned value identifies the sampled state.
    MarkovModel<int> model;
    model.from_str(
        "3 "
        "10 20 30 "  // stateValues
        "2 2 0 "     // initialDistribution (unnormalized, sums to 4)
        "1 1 0 "     // row 0 (unnormalized, sums to 2)
        "0 0 0 "     // row 1 (all zero -> must become uniform)
        "0 0 1 "     // row 2 (already normalized)
    );
    model.normalize();

    expect(model.getStateSize() == 3, "state size should be preserved by from_str");

    expectNear(model.getInitialProbability(0), 0.5, "initial[0] should normalize to 0.5");
    expectNear(model.getInitialProbability(1), 0.5, "initial[1] should normalize to 0.5");
    expectNear(model.getInitialProbability(2), 0.0, "initial[2] should stay 0.0");

    expectNear(model.getTransitionProbability(0, 0), 0.5, "row 0 should normalize to sum 1");
    expectNear(model.getTransitionProbability(0, 1), 0.5, "row 0 should normalize to sum 1");
    expectNear(model.getTransitionProbability(1, 0), 1.0 / 3.0, "all-zero row should become uniform");
    expectNear(model.getTransitionProbability(1, 1), 1.0 / 3.0, "all-zero row should become uniform");
    expectNear(model.getTransitionProbability(1, 2), 1.0 / 3.0, "all-zero row should become uniform");
    expectNear(model.getTransitionProbability(2, 2), 1.0, "already-normalized row should be unchanged");

    expect(model.nextSample(0.1) == 10, "first sample should draw from the initial distribution");
    expect(model.nextSample(0.9) == 20, "second sample should draw from the transition row instead");
}

// Pan should split a signal across channels by each channel's distance from
// the (constant) pan position. Tony Chai, 2026
void testPanCenteredAcrossThreeChannels()
{
    Constant panValue(0.5f);
    Pan pan(panValue);

    Track track(4, 44100, true);
    for (m_sample_count_type i = 0; i < 4; i++) track.getWave()[i] = 2.0f;

    MultiTrack* result = pan.spatialize_Track(track, 3);
    expect(result->size() == 3, "Pan should produce exactly numTracks channels");

    const double expectedScale[3] = {0.5, 1.0, 0.5};
    for (int c = 0; c < 3; c++) {
        SoundSample& wave = result->get(c)->getWave();
        for (m_sample_count_type i = 0; i < 4; i++) {
            expectNear(wave[i], expectedScale[c] * 2.0,
                       "channel " + std::to_string(c) + " sample " + std::to_string(i)
                           + ": centered pan should scale each channel by its distance from center");
        }
    }
    delete result;
}

// Regression test: panning to a single output channel divided by
// numTracks-1 (== 0), silently filling the channel with NaN. Tony Chai, 2026
void testPanSingleChannelIsUnscaled()
{
    Constant panValue(0.5f);
    Pan pan(panValue);

    Track track(4, 44100, true);
    for (m_sample_count_type i = 0; i < 4; i++) track.getWave()[i] = 2.0f;

    MultiTrack* result = pan.spatialize_Track(track, 1);
    expect(result->size() == 1, "Pan with numTracks=1 should still produce one channel");
    SoundSample& wave = result->get(0)->getWave();
    for (m_sample_count_type i = 0; i < 4; i++) {
        expect(!std::isnan(wave[i]), "sample " + std::to_string(i)
                                         + ": single-channel pan must not produce NaN");
        expectNear(wave[i], 2.0, "sample " + std::to_string(i)
                                     + ": single-channel pan should pass the signal through unscaled");
    }
    delete result;
}

// An out-of-range state should throw a catchable exception, not abort the process.
void testMarkovModelInvalidStateThrows()
{
    MarkovModel<int> model(2);
    model.normalize();

    bool threw = false;
    try {
        model.getTransitionProbabilities(5); // out of range for a 2-state model
    } catch (const std::out_of_range&) {
        threw = true;
    }
    expect(threw,
           "querying an out-of-range state must throw a catchable exception, not abort the process");
}

// Renders a one-partial 440 Hz sound lasting 1 second into 2 channels and
// checks the audio itself: each channel holds exactly 1 second of samples,
// none of them NaN or infinite, the sound isn't silent, and its pitch is
// 440 Hz (counted as upward zero crossings). -- Tony Chai, 2026
void testSoundRendersItsPitchAndLength()
{
    constexpr m_rate_type rate = 44100;
    constexpr double frequency = 440.0;
    Sound sound(1, frequency);
    sound.setParam(DURATION, 1.0);
    std::unique_ptr<MultiTrack> rendered(sound.render(2, rate));
    expect(rendered->size() == 2, "expected 2 channels, got "
                                      + std::to_string(rendered->size()));
    for (int c = 0; c < rendered->size(); c++) {
        const std::string channel = "channel " + std::to_string(c);
        SoundSample& wave = rendered->get(c)->getWave();
        expect(wave.getSampleCount() == rate,
               channel + ": expected " + std::to_string(rate) + " samples (1 second), got "
                   + std::to_string(wave.getSampleCount()));
        long long badSamples = 0;
        int upwardCrossings = 0;
        double peak = 0.0;
        for (m_sample_count_type i = 0; i < wave.getSampleCount(); i++) {
            const double value = wave[i];
            if (!std::isfinite(value)) {
                ++badSamples;
                continue;
            }
            peak = std::max(peak, std::abs(value));
            if (i > 0 && wave[i - 1] < 0 && value >= 0)
                ++upwardCrossings;
        }
        expect(badSamples == 0, channel + ": " + std::to_string(badSamples)
                                    + " NaN/infinite samples");
        expect(peak > 1e-4, channel + ": the sound is silent (peak "
                                + std::to_string(peak) + ")");
        // A 440 Hz wave crosses zero upward 440 times a second; allow one
        // either way for where the wave starts and ends.
        expect(std::abs(upwardCrossings - 440) <= 1,
               channel + ": pitch should be 440 Hz, but the wave crossed zero upward "
                   + std::to_string(upwardCrossings) + " times in 1 second");
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

    runTest("testMultiPanCloneIsIndependent", testMultiPanCloneIsIndependent, failedTests);
    runTest("testSoundSetDetuneValidation", testSoundSetDetuneValidation, failedTests);
    runTest("testSpatializerMultiTrackComposite", testSpatializerMultiTrackComposite, failedTests);
    runTest("testSpatializerSpreadsTrackOverIndependentChannels", testSpatializerSpreadsTrackOverIndependentChannels, failedTests);
    runTest("testReverbOwnsItsOwnEnvelopeCopy", testReverbOwnsItsOwnEnvelopeCopy, failedTests);
    runTest("testSoundInvalidPartialCountIsSafe", testSoundInvalidPartialCountIsSafe, failedTests);
    runTest("testEnvelopeConstantValueInterpolation", testEnvelopeConstantValueInterpolation, failedTests);
    runTest("testMarkovModelNormalizeAndSample", testMarkovModelNormalizeAndSample, failedTests);
    runTest("testMarkovModelInvalidStateThrows", testMarkovModelInvalidStateThrows, failedTests);
    runTest("testPanCenteredAcrossThreeChannels", testPanCenteredAcrossThreeChannels, failedTests);
    runTest("testPanSingleChannelIsUnscaled", testPanSingleChannelIsUnscaled, failedTests);
    runTest("testSoundRendersItsPitchAndLength", testSoundRendersItsPitchAndLength, failedTests);

    if (!failedTests.empty()) {
        std::cerr << '\n' << failedTests.size() << " LASS test(s) failed:\n";
        for (const std::string& name : failedTests)
            std::cerr << "  " << name << '\n';
        return 1;
    }

    std::cerr << "\nAll LASS tests passed\n";
    return 0;
}
