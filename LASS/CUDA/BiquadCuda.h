// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LASS_CUDA_BIQUAD_CUDA_H
#define LASS_CUDA_BIQUAD_CUDA_H

#include <cstddef>

namespace lass_cuda {

// Coefficients are normalized by a0, as in BiQuadFilter::do_filter.
struct BiquadCoefficients {
    float b0, b1, b2, a1, a2;
};

// Checks the runtime driver and initializes an accessible device context.
bool hasUsableGpu();

// Returns false if no usable CUDA device exists or any CUDA operation fails.
// The caller may then recompute the entire channel on the CPU. Input and
// output are distinct host buffers of count floats; no filter state is kept.
bool filterBiquad(const float* input, float* output, std::size_t count,
                  BiquadCoefficients coefficients);

} // namespace lass_cuda

#endif
