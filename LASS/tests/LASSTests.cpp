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

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

int failureCount = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failureCount;
    }
}

void expectNear(double actual, double expected, const std::string& message)
{
    // m_value_type (the storage behind getParam/setParam) is float, so a
    // round trip through it loses more precision than a double-only
    // tolerance would allow.
    constexpr double tolerance = 1.0e-6;
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << message << ": expected " << expected
                   << ", got " << actual << '\n';
        ++failureCount;
    }
}

// MultiPan::clone() must deep-copy, not share pointers with the original.
void testMultiPanCloneIsIndependent()
{
    MultiPan* original = new MultiPan(4);
    original->addEntryLocation(0.0f, 0.0f, 1.0f);
    original->addEntryLocation(1.0f, static_cast<float>(M_PI / 2.0), 0.5f);
    original->doneAddEntryLocation();

    MultiPan* clone = original->clone();
    delete original;

    clone->print();
    delete clone;

    expect(true, "MultiPan clone/delete-original/use-clone completed without crashing");
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
                       "composited multi-track output sample mismatch");
        }
    }
    delete result;
}

// Reverb must own a private copy of its Envelope, not a dangling pointer to the caller's.
void testReverbOwnsItsOwnEnvelopeCopy()
{
    Envelope* callerOwnedInput = new Envelope();
    Reverb* reverb = new Reverb(callerOwnedInput, /*hilow_spread=*/0.5f,
                                 /*gainAllPass=*/0.7f, /*delay=*/0.03f,
                                 /*samplingRate=*/44100);
    delete reverb;

    expect(true,
           "Reverb construct/destroy with a caller-owned Envelope completed without crashing");
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
                       "centered pan should scale each channel by its distance from center");
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
        expect(!std::isnan(wave[i]), "single-channel pan must not produce NaN");
        expectNear(wave[i], 2.0, "single-channel pan should pass the signal through unscaled");
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

} // namespace

int main()
{
    testMultiPanCloneIsIndependent();
    testSoundSetDetuneValidation();
    testSpatializerMultiTrackComposite();
    testReverbOwnsItsOwnEnvelopeCopy();
    testSoundInvalidPartialCountIsSafe();
    testEnvelopeConstantValueInterpolation();
    testMarkovModelNormalizeAndSample();
    testMarkovModelInvalidStateThrows();
    testPanCenteredAcrossThreeChannels();
    testPanSingleChannelIsUnscaled();

    if (failureCount > 0) {
        std::cerr << failureCount << " LASS test(s) failed\n";
        return 1;
    }

    std::cout << "LASSTests passed\n";
    return 0;
}
