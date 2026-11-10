// SPDX-License-Identifier: GPL-2.0-or-later
#include "BiquadCuda.h"
#include "BiQuadFilter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

namespace {

bool near(float actual, float expected, float absoluteTolerance = 0.0002f) {
    return std::isfinite(actual) &&
           std::fabs(actual - expected) <=
               absoluteTolerance + 0.001f * std::fabs(expected);
}

std::vector<float> makeInput(std::size_t count, float phase) {
    std::vector<float> samples(count);
    for (std::size_t i = 0; i < count; ++i)
        samples[i] = 0.3f * std::sin(static_cast<float>(i) * 0.019f + phase)
                   + (i % 311 == 0 ? 0.8f : 0.0f);
    return samples;
}

bool checkBackend(const std::vector<float>& input,
                  lass_cuda::BiquadCoefficients c,
                  float absoluteTolerance = 0.0002f) {
    std::vector<float> gpu(input.size());
    if (!lass_cuda::filterBiquad(input.data(), gpu.data(), input.size(), c)) {
        std::cerr << "CUDA filtering failed on a detected device\n";
        return false;
    }
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
    double maxAbsoluteError = 0.0;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const float expected = c.b0 * input[i] + c.b1 * x1 + c.b2 * x2
                             - c.a1 * y1 - c.a2 * y2;
        if (!near(gpu[i], expected, absoluteTolerance)) {
            std::cerr << std::setprecision(9)
                      << "Backend mismatch at sample " << i
                      << ", coefficients (b0,b1,b2,a1,a2) = ("
                      << c.b0 << ',' << c.b1 << ',' << c.b2 << ','
                      << c.a1 << ',' << c.a2 << ")"
                      << ", actual " << gpu[i]
                      << ", expected " << expected
                      << ", absolute error "
                      << std::fabs(static_cast<double>(gpu[i]) - expected)
                      << ", tolerance "
                      << absoluteTolerance + 0.001f * std::fabs(expected)
                      << '\n';
            return false;
        }
        maxAbsoluteError = std::max(maxAbsoluteError,
            std::fabs(static_cast<double>(gpu[i]) - expected));
        x2 = x1; x1 = input[i];
        y2 = y1; y1 = expected;
    }
    std::cout << std::setprecision(9) << "Backend max absolute error (a1="
              << c.a1 << ", a2=" << c.a2 << "): " << maxAbsoluteError
              << '\n';
    return true;
}

bool checkMultitrack(const std::vector<float>& first,
                     const std::vector<float>& second) {
    MultiTrack input;
    for (const auto* samples : {&first, &second}) {
        auto wave = std::make_unique<SoundSample>(samples->size(), 48000);
        for (std::size_t i = 0; i < samples->size(); ++i)
            (*wave)[i] = (*samples)[i];
        input.add(new Track(wave.release()));
    }

    BiQuadFilter filter(0, 0.0f, 2000.0f, 48000.0f, 1.0f);
    std::unique_ptr<MultiTrack> actual(&filter.do_filter_MultiTrack(input));
    if (actual->size() != 2) return false;
    for (int channel = 0; channel < 2; ++channel) {
        BiQuadFilter reference(0, 0.0f, 2000.0f, 48000.0f, 1.0f);
        SoundSample& wave = input.get(channel)->getWave();
        SoundSample& result = actual->get(channel)->getWave();
        if (result.getSampleCount() != wave.getSampleCount() ||
            result.getSamplingRate() != wave.getSamplingRate()) return false;
        for (long i = 0; i < wave.getSampleCount(); ++i) {
            if (!near(result[i], reference.do_filter(wave[i]))) {
                std::cerr << "Multitrack mismatch at channel " << channel
                          << ", sample " << i << '\n';
                return false;
            }
        }
    }
    return true;
}

} // namespace

int main() {
    if (!lass_cuda::hasUsableGpu()) {
        std::cout << "No usable CUDA device; GPU test skipped\n";
        return 77;
    }
    const auto first = makeInput(65539, 0.0f); // includes a short final chunk
    const auto second = makeInput(65539, 0.7f);
    const lass_cuda::BiquadCoefficients probe{0.2f, 0.1f, 0.05f, -0.4f, 0.12f};
    // This near-unit-pole case amplifies float rounding across chunks.
    const lass_cuda::BiquadCoefficients nearPole{
        0.001f, 0.002f, 0.001f, -1.98f, 0.9801f};
    if (!checkBackend(first, probe) ||
        !checkBackend(second, {0.1f, 0.05f, 0.02f, -1.8f, 0.81f}) ||
        !checkBackend(first, nearPole, 0.001f) ||
        !checkMultitrack(first, second)) return 1;
    return 0;
}
