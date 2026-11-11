// SPDX-License-Identifier: GPL-2.0-or-later
// Terminal benchmark for the LASS biquad path. The same source builds against
// CPU-only and CUDA-enabled LASS libraries so the workloads stay identical.

#include "BiQuadFilter.h"
#include "MultiTrack.h"
#include "Partial.h"
#include "Sound.h"
#include "Track.h"
#ifdef LASS_BENCH_WITH_CUDA
#include "BiquadCuda.h"
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <vector>

namespace {

constexpr m_rate_type kRate = 48000;
constexpr double kPi = 3.14159265358979323846;

struct BenchmarkCase {
    const char* name;
    const char* suite;
    int seconds;
    int channels;
    int partials;
};

constexpr BenchmarkCase kCases[] = {
    {"filter_1s_2ch", "filter", 1, 2, 0},
    {"filter_8s_2ch", "filter", 8, 2, 0},
    {"filter_32s_2ch", "filter", 32, 2, 0},
    {"filter_32s_8ch", "filter", 32, 8, 0},
    {"render_1s_2ch_8p", "render", 1, 2, 8},
    {"render_1s_2ch_32p", "render", 1, 2, 32},
    {"render_4s_2ch_24p", "render", 4, 2, 24},
    {"render_8s_4ch_8p", "render", 8, 4, 8},
    {"render_8s_4ch_32p", "render", 8, 4, 32},
};

class DiscardBuffer : public std::streambuf {
protected:
    int overflow(int character) override { return traits_type::not_eof(character); }
};

class QuietLassOutput {
public:
    QuietLassOutput() : previous_(std::cout.rdbuf(&discard_)) {}
    ~QuietLassOutput() { std::cout.rdbuf(previous_); }
    QuietLassOutput(const QuietLassOutput&) = delete;
    QuietLassOutput& operator=(const QuietLassOutput&) = delete;

private:
    DiscardBuffer discard_;
    std::streambuf* previous_;
};

std::uint64_t completedGpuCalls() {
#ifdef LASS_BENCH_WITH_CUDA
    return lass_cuda::successfulFilterCalls();
#else
    return 0;
#endif
}

void verifyBackend(std::uint64_t before, int channels, const char* name) {
#ifdef LASS_BENCH_WITH_CUDA
    const auto completed = completedGpuCalls() - before;
    if (completed != static_cast<std::uint64_t>(channels)) {
        throw std::runtime_error(std::string(name) + ": expected " +
                                 std::to_string(channels) + " successful GPU calls, got " +
                                 std::to_string(completed) + "; CUDA fell back to CPU");
    }
#else
    (void)before;
    (void)channels;
    (void)name;
#endif
}

double sampledMagnitude(MultiTrack& output, const BenchmarkCase& test) {
    if (output.size() != test.channels)
        throw std::runtime_error(std::string(test.name) + ": unexpected channel count");
    double sum = 0.0;
    for (int channel = 0; channel < test.channels; ++channel) {
        SoundSample& wave = output.get(channel)->getWave();
        if (wave.getSampleCount() != static_cast<m_sample_count_type>(test.seconds) * kRate)
            throw std::runtime_error(std::string(test.name) + ": unexpected sample count");
        const auto step = std::max<m_sample_count_type>(1, wave.getSampleCount() / 256);
        for (m_sample_count_type sample = 0; sample < wave.getSampleCount(); sample += step)
            sum += std::abs(static_cast<double>(wave[sample]));
    }
    if (!std::isfinite(sum) || sum == 0.0)
        throw std::runtime_error(std::string(test.name) + ": invalid or silent output");
    return sum;
}

template <typename Render>
std::pair<double, double> measure(const BenchmarkCase& test, int repetitions,
                                  Render&& render) {
    std::vector<double> timings;
    timings.reserve(repetitions);
    double magnitude = 0.0;
    for (int iteration = -1; iteration < repetitions; ++iteration) {
        const auto gpuCallsBefore = completedGpuCalls();
        const auto start = std::chrono::steady_clock::now();
        std::unique_ptr<MultiTrack> output(render());
        const auto stop = std::chrono::steady_clock::now();
        verifyBackend(gpuCallsBefore, test.channels, test.name);
        magnitude = sampledMagnitude(*output, test);
        if (iteration >= 0) {
            const auto elapsed = std::chrono::duration<double, std::milli>(stop - start);
            timings.push_back(elapsed.count());
        }
    }
    std::sort(timings.begin(), timings.end());
    return {timings[timings.size() / 2], magnitude};
}

std::pair<double, double> measureFilter(const BenchmarkCase& test, int repetitions) {
    const auto samples = static_cast<m_sample_count_type>(test.seconds) * kRate;
    MultiTrack input(test.channels, samples, kRate);
    for (m_sample_count_type sample = 0; sample < samples; ++sample) {
        const double t = static_cast<double>(sample) / kRate;
        const double value = 0.25 * std::sin(2.0 * kPi * 110.0 * t) +
                             0.15 * std::sin(2.0 * kPi * 440.0 * t) +
                             0.1 * std::sin(2.0 * kPi * 1760.0 * t);
        for (int channel = 0; channel < test.channels; ++channel)
            input.get(channel)->getWave()[sample] =
                static_cast<m_sample_type>(value * (1.0 - 0.04 * channel));
    }
    BiQuadFilter filter(0, 0.0f, 1000.0f, static_cast<float>(kRate), 1.0f);
    return measure(test, repetitions, [&] { return &filter.do_filter_MultiTrack(input); });
}

std::pair<double, double> measureRender(const BenchmarkCase& test, int repetitions) {
    Sound sound;
    sound.setParam(DURATION, static_cast<m_value_type>(test.seconds));
    sound.setParam(LOUDNESS, -1.0f);
    sound.setParam(DETUNE_FUNDAMENTAL, 0.0f);
    for (int index = 0; index < test.partials; ++index) {
        Partial partial;
        partial.setParam(PARTIAL_NUM, static_cast<m_value_type>(index));
        partial.setParam(FREQUENCY, static_cast<m_value_type>(110.0 * (index + 1)));
        partial.setParam(LOUDNESS_SCALAR,
                         static_cast<m_value_type>(0.5 / (index + 1)));
        partial.setParam(TREMOLO_AMP, 0.1f);
        partial.setParam(TREMOLO_RATE, static_cast<m_value_type>(2.0 + index % 3));
        partial.setParam(VIBRATO_AMP, 0.002f);
        partial.setParam(VIBRATO_RATE, static_cast<m_value_type>(4.0 + index % 4));
        partial.setParam(PHASE_AMP_ENV, 0.01f);
        partial.setParam(PHASE_RATE_ENV, static_cast<m_value_type>(3.0 + index % 5));
        sound.add(partial);
    }
    sound.use_filter(new BiQuadFilter(0, 0.0f, 1000.0f,
                                      static_cast<float>(kRate), 1.0f));
    return measure(test, repetitions,
                   [&] { return sound.render(test.channels, kRate); });
}

int parseRepetitions(const std::string& value) {
    const std::string prefix = "--repetitions=";
    if (value.rfind(prefix, 0) != 0) return -1;
    const std::string count = value.substr(prefix.size());
    if (count.empty() || count.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("--repetitions must be a positive integer");
    const long result = std::stol(count);
    if (result < 1 || result > 100)
        throw std::invalid_argument("--repetitions must be between 1 and 100");
    return static_cast<int>(result);
}

} // namespace

int main(int argc, char** argv) {
    try {
        int repetitions = 3;
        std::string suite = "all";
        bool quick = false;
        for (int arg = 1; arg < argc; ++arg) {
            const std::string option(argv[arg]);
            if (option == "--quick") quick = true;
            else if (option == "--suite=filter") suite = "filter";
            else if (option == "--suite=render") suite = "render";
            else if (option == "--suite=all") suite = "all";
            else if (option.rfind("--repetitions=", 0) == 0)
                repetitions = parseRepetitions(option);
            else
                throw std::invalid_argument("unknown argument: " + option);
        }

#ifdef LASS_BENCH_WITH_CUDA
        if (!lass_cuda::hasUsableGpu())
            throw std::runtime_error("CUDA build has no usable GPU; run on a GPU compute node");
#endif
        Track::setAmplitudeTracking(false);
        std::cout << "case,suite,seconds,channels,partials,samples_per_channel,"
                     "backend,repetitions,median_ms,sampled_magnitude\n";
        for (const auto& test : kCases) {
            if (suite != "all" && suite != test.suite) continue;
            if (quick && std::string(test.name) != "filter_1s_2ch" &&
                std::string(test.name) != "render_1s_2ch_8p") continue;
            std::pair<double, double> result;
            {
                QuietLassOutput quiet;
                result = std::string(test.suite) == "filter"
                             ? measureFilter(test, repetitions)
                             : measureRender(test, repetitions);
            }
            std::cout << test.name << ',' << test.suite << ',' << test.seconds << ','
                      << test.channels << ',' << test.partials << ','
                      << static_cast<m_sample_count_type>(test.seconds) * kRate << ','
#ifdef LASS_BENCH_WITH_CUDA
                      << "cuda"
#else
                      << "cpu"
#endif
                      << ',' << repetitions << ',' << std::fixed << std::setprecision(3)
                      << result.first << ',' << std::setprecision(6) << result.second
                      << '\n' << std::flush;
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
