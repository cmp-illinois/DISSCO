// SPDX-License-Identifier: GPL-2.0-or-later
#include "BiquadCuda.h"

#include <cuda_runtime.h>

#include <cstddef>
#include <limits>

namespace lass_cuda {
namespace {

constexpr std::size_t kSamplesPerChunk = 256;
constexpr unsigned int kThreadsPerBlock = 256;

// At a chunk boundary, state is [y[n-1], y[n-2]]. The chunk maps its
// incoming state to M * state + offset. This makes whole chunks independent.
struct ChunkSummary {
    double m00, m01, m10, m11;
    float offset0, offset1;
};

struct OutputState {
    float previous, previous2;
};

struct Matrix2 {
    double m00, m01, m10, m11;
};

__device__ Matrix2 multiply(Matrix2 left, Matrix2 right) {
    return {
        left.m00 * right.m00 + left.m01 * right.m10,
        left.m00 * right.m01 + left.m01 * right.m11,
        left.m10 * right.m00 + left.m11 * right.m10,
        left.m10 * right.m01 + left.m11 * right.m11
    };
}

template <typename T>
class DeviceBuffer {
public:
    DeviceBuffer() = default;
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    ~DeviceBuffer() { if (data_) cudaFree(data_); }

    bool allocate(std::size_t count) {
        return cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(T))
            == cudaSuccess;
    }
    T* get() const { return data_; }

private:
    T* data_ = nullptr;
};

class CudaStream {
public:
    CudaStream() = default;
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
    ~CudaStream() {
        if (stream_) {
            cudaStreamSynchronize(stream_);
            cudaStreamDestroy(stream_);
        }
    }

    bool create() {
        return cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking)
            == cudaSuccess;
    }
    cudaStream_t get() const { return stream_; }

private:
    cudaStream_t stream_ = nullptr;
};

__device__ float feedForward(const float* input, std::size_t index,
                             BiquadCoefficients c) {
    const float x1 = index ? input[index - 1] : 0.0f;
    const float x2 = index > 1 ? input[index - 2] : 0.0f;
    return c.b0 * input[index] + c.b1 * x1 + c.b2 * x2;
}

__global__ void summarizeChunks(const float* input, std::size_t count,
                                BiquadCoefficients c, ChunkSummary* summaries) {
    const std::size_t chunk = static_cast<std::size_t>(blockIdx.x) * blockDim.x
                              + threadIdx.x;
    const std::size_t start = chunk * kSamplesPerChunk;
    if (start >= count) return;
    const std::size_t end = start + kSamplesPerChunk < count
                            ? start + kSamplesPerChunk : count;

    float y1 = 0.0f, y2 = 0.0f;
    for (std::size_t i = start; i < end; ++i) {
        const float y = feedForward(input, i, c) - c.a1 * y1 - c.a2 * y2;
        y2 = y1;
        y1 = y;
    }

    // The homogeneous state transition is constant. Repeated squaring is
    // cheaper and more accurate than updating the matrix for every sample.
    Matrix2 power{1.0, 0.0, 0.0, 1.0};
    Matrix2 base{-static_cast<double>(c.a1), -static_cast<double>(c.a2),
                 1.0, 0.0};
    std::size_t length = end - start;
    while (length) {
        if (length & 1) power = multiply(base, power);
        base = multiply(base, base);
        length >>= 1;
    }
    summaries[chunk] = {power.m00, power.m01, power.m10, power.m11, y1, y2};
}

// Only the compact chunk summaries are serial. All sample arithmetic is
// performed by the two parallel kernels around this scan.
__global__ void scanChunkStates(const ChunkSummary* summaries,
                                OutputState* starts, std::size_t chunks) {
    if (blockIdx.x || threadIdx.x) return;
    double y1 = 0.0, y2 = 0.0;
    for (std::size_t chunk = 0; chunk < chunks; ++chunk) {
        starts[chunk] = {static_cast<float>(y1), static_cast<float>(y2)};
        const ChunkSummary s = summaries[chunk];
        const double next1 = s.m00 * y1 + s.m01 * y2 + s.offset0;
        const double next2 = s.m10 * y1 + s.m11 * y2 + s.offset1;
        y1 = next1;
        y2 = next2;
    }
}

__global__ void filterChunks(const float* input, float* output,
                             std::size_t count, BiquadCoefficients c,
                             const OutputState* starts) {
    const std::size_t chunk = static_cast<std::size_t>(blockIdx.x) * blockDim.x
                              + threadIdx.x;
    const std::size_t start = chunk * kSamplesPerChunk;
    if (start >= count) return;
    const std::size_t end = start + kSamplesPerChunk < count
                            ? start + kSamplesPerChunk : count;
    float y1 = starts[chunk].previous;
    float y2 = starts[chunk].previous2;
    for (std::size_t i = start; i < end; ++i) {
        const float y = feedForward(input, i, c) - c.a1 * y1 - c.a2 * y2;
        output[i] = y;
        y2 = y1;
        y1 = y;
    }
}

bool selectUsableDevice() {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess) return false;
    for (int device = 0; device < deviceCount; ++device) {
        cudaDeviceProp properties{};
        if (cudaGetDeviceProperties(&properties, device) != cudaSuccess ||
            properties.computeMode == cudaComputeModeProhibited) continue;
        if (cudaSetDevice(device) == cudaSuccess && cudaFree(nullptr) == cudaSuccess)
            return true;
    }
    return false;
}

} // namespace

bool hasUsableGpu() {
    return selectUsableDevice();
}

bool filterBiquad(const float* input, float* output, std::size_t count,
                  BiquadCoefficients coefficients) {
    if (count == 0) return true;
    if (!input || !output ||
        count > std::numeric_limits<std::size_t>::max() / sizeof(float) ||
        !hasUsableGpu()) return false;

    const std::size_t chunks = 1 + (count - 1) / kSamplesPerChunk;
    if (chunks > std::numeric_limits<std::size_t>::max() / sizeof(ChunkSummary) ||
        chunks > std::numeric_limits<std::size_t>::max() / sizeof(OutputState))
        return false;

    int currentDevice = 0;
    cudaDeviceProp properties{};
    if (cudaGetDevice(&currentDevice) != cudaSuccess ||
        cudaGetDeviceProperties(&properties, currentDevice) != cudaSuccess)
        return false;
    const std::size_t blocks = 1 + (chunks - 1) / kThreadsPerBlock;
    if (blocks > static_cast<std::size_t>(properties.maxGridSize[0])) return false;

    DeviceBuffer<float> deviceInput, deviceOutput;
    DeviceBuffer<ChunkSummary> summaries;
    DeviceBuffer<OutputState> starts;
    CudaStream stream;
    if (!stream.create() || !deviceInput.allocate(count) || !deviceOutput.allocate(count) ||
        !summaries.allocate(chunks) || !starts.allocate(chunks)) return false;

    const std::size_t sampleBytes = count * sizeof(float);
    if (cudaMemcpyAsync(deviceInput.get(), input, sampleBytes,
                        cudaMemcpyHostToDevice, stream.get()) != cudaSuccess)
        return false;

    summarizeChunks<<<static_cast<unsigned int>(blocks), kThreadsPerBlock, 0,
                      stream.get()>>>(
        deviceInput.get(), count, coefficients, summaries.get());
    if (cudaGetLastError() != cudaSuccess) return false;
    scanChunkStates<<<1, 1, 0, stream.get()>>>(summaries.get(), starts.get(), chunks);
    if (cudaGetLastError() != cudaSuccess) return false;
    filterChunks<<<static_cast<unsigned int>(blocks), kThreadsPerBlock, 0,
                   stream.get()>>>(
        deviceInput.get(), deviceOutput.get(), count, coefficients, starts.get());
    if (cudaGetLastError() != cudaSuccess ||
        cudaMemcpyAsync(output, deviceOutput.get(), sampleBytes,
                        cudaMemcpyDeviceToHost, stream.get()) != cudaSuccess ||
        cudaStreamSynchronize(stream.get()) != cudaSuccess)
        return false;
    return true;
}

} // namespace lass_cuda
