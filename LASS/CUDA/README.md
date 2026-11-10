# CUDA biquad filtering

DISSCO combines partials into a multichannel sound, applies the sound's
`BiQuadFilter`, then applies reverb and spatialization. The CUDA backend handles
that biquad step. It uses the same normalized five coefficients and two sample
input/output history as the CPU implementation. Each channel begins with zero
history, including when the same filter object is reused for another channel.
The former CPU biquad reused output history and did not reset both histories;
the corrected recurrence can change audio from older builds.

With a CUDA compiler and toolkit installed, the normal CMake build includes
this backend by default. `-DLASS_ENABLE_CUDA=OFF` builds the CPU version.
`-DCMAKE_CUDA_ARCHITECTURES=<target>` can select the target GPU architecture
when cross compiling. The CUDA runtime is linked statically, so the executable
can reach its CPU fallback on machines without a CUDA toolkit installation.
At runtime, sounds with at least 32,768 total samples try CUDA if a usable GPU
is present. Missing GPUs, insufficient device memory, kernel errors, and shorter
sounds use the CPU implementation automatically. A CUDA build also runs on
machines without an NVIDIA GPU.

The CUDA implementation splits each channel into 256 sample chunks. One kernel
computes an affine transfer summary for each chunk, a small scan computes the
correct starting output history for every chunk, and a final kernel filters
the chunks in parallel. It copies a channel back only after all CUDA work
succeeds. CPU and GPU floating point results can differ slightly at chunk
boundaries because their operation order differs.

On a CUDA platform, configure and build normally, then run the focused test:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target lass-cuda-biquad-test
ctest --test-dir build -R '^lass-cuda-biquad$' --output-on-failure
```

The test compares the CUDA kernel with a CPU recurrence and checks two
independent channels through LASS's public `Filter` path. It reports a skip
when no usable GPU exists. For performance measurements, compare full project
renders on the target hardware with `LASS_ENABLE_CUDA` on and off; the threshold
is a transfer cost heuristic, not a measured crossover for every GPU.
