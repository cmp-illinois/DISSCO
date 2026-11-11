# LASS CUDA biquad filtering

LASS is DISSCO's additive sound synthesis library. A `Sound` mixes its
partials into a multichannel signal before applying a sound-level
`BiQuadFilter`. The optional CUDA backend accelerates that filter step; it
does not synthesize the partials on the GPU. Sound-level reverb and
spatialization can follow the filter. The focused test and benchmark generate
audio in memory, so they do not need an SND file, a GUI, or an audio output
device.

At build time, `LASS_ENABLE_CUDA=ON` selects the CUDA backend if CMake finds a
CUDA compiler and toolkit. `LASS_ENABLE_CUDA=OFF` builds the CPU version. At
runtime, the CUDA build tries the GPU for at least 32,768 samples across all
channels. Shorter sounds, unavailable GPUs, and CUDA errors use the CPU path.
The sample threshold is a heuristic, not a measured performance crossover.

## Prerequisites

The commands below are for a Linux shell at the **DISSCO repository root**.
Use a checkout containing the CUDA test, benchmark, and comparison script.
The [Delta example](#example-uiuc-ncsa-delta) uses its module system and Slurm;
on another system, provide the same tools and run the executables on an NVIDIA
GPU machine without the Slurm step.

| Requirement | Used for | Check |
| --- | --- | --- |
| CMake 3.25 or newer | Configure standalone `LASS` | `cmake --version` |
| C and C++ compilers, plus GNU Make | Build LASS and libsndfile with CMake's default Unix Makefiles generator | `cc --version`, `c++ --version`, `make --version` |
| libsndfile headers and library | Required by `LASS/CMakeLists.txt` even when no file is written | Check `sndfile.h` and `libsndfile.so` below |
| CUDA toolkit with `nvcc` | Compile the CUDA backend | `nvcc --version` |
| Accessible NVIDIA GPU and compatible driver | Run the CUDA test and benchmark | `nvidia-smi -L` on a GPU node |
| Python 3.9 or newer | Compare benchmark CSV files; build libsndfile from source | `python3 --version` |
| Git and network access | Only for the source-install example below | `git --version` |

Start in the repository root and check the tools available before configuring:

```sh
pwd
test -f LASS/CMakeLists.txt
test -f cmake-modules/FindSndFile.cmake
test -f LASS/CUDA/BiquadCudaTest.cpp
test -f LASS/CUDA/BiquadBenchmark.cpp
test -f LASS/CUDA/compare_benchmark.py
cmake --version                 # first line must report 3.25 or newer
cc --version
c++ --version
make --version
python3 -c 'import sys; assert sys.version_info >= (3, 9), sys.version; print(sys.version)'
```

Each `test -f` command must exit successfully. If a prerequisite is missing,
install or load it in the user environment before continuing. The standalone
LASS build requires libsndfile but does not require the Qt-based DISSCO UI.

### Provide libsndfile without administrator access

If libsndfile is already installed, set `LASS_SNDFILE_PREFIX` to its install
directory and skip the source build. The prefix must contain
`include/sndfile.h` and `lib/libsndfile.so`. A Conda environment containing
libsndfile can also serve as the prefix; the rest of this guide does not
require Conda. On Delta, `module spider libsndfile` checks whether a module is
available; an absent module does not prevent a user-owned source installation.
For an existing installation, set the prefix with
`export LASS_SNDFILE_PREFIX=/path/to/libsndfile-prefix`.

When no usable installation exists, build [libsndfile 1.2.2 from its upstream
source](https://github.com/libsndfile/libsndfile/releases/tag/1.2.2) in your
home directory. This builds the shared library with optional file codecs,
utilities, examples, and its own tests disabled. Those codecs are unnecessary
for the generated test audio. No `sudo` or system package manager is needed.

```sh
git --version
git clone --depth 1 --branch 1.2.2 \
  https://github.com/libsndfile/libsndfile.git "$HOME/libsndfile-1.2.2-src"
export LASS_SNDFILE_PREFIX="$HOME/.local/libsndfile-1.2.2"
cmake -S "$HOME/libsndfile-1.2.2-src" -B "$HOME/libsndfile-1.2.2-build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$LASS_SNDFILE_PREFIX" \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF \
  -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF \
  -DENABLE_EXTERNAL_LIBS=OFF -DENABLE_MPEG=OFF
cmake --build "$HOME/libsndfile-1.2.2-build" --parallel 4
cmake --install "$HOME/libsndfile-1.2.2-build"
```

If the source was downloaded on another machine, copy it to the cluster and
start at the `cmake -S` command. Verify either installation before configuring
LASS:

```sh
test -r "$LASS_SNDFILE_PREFIX/include/sndfile.h"
test -r "$LASS_SNDFILE_PREFIX/lib/libsndfile.so"
ls -l "$LASS_SNDFILE_PREFIX/include/sndfile.h" \
  "$LASS_SNDFILE_PREFIX/lib/libsndfile.so"
export LD_LIBRARY_PATH="$LASS_SNDFILE_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

Both `test -r` commands must succeed. Keep `LASS_SNDFILE_PREFIX` and
`LD_LIBRARY_PATH` set when building and running the test binaries. If the
library is installed under `lib64`, use that directory in the checks, CMake
option, and library path.

## Build CPU and CUDA versions

Use separate build directories and the same compiler, source revision, and
Release settings for a useful comparison. `BUILD_TESTING=ON` creates the
benchmark target. A `cmake --build` command only builds a directory that was
already configured: run the matching `cmake -S ... -B ...` command first for
each directory. On Delta, load the CUDA module before configuring both builds
so they use the same host compiler; `LASS_ENABLE_CUDA=OFF` still keeps CUDA out
of the CPU build. Run these commands from the DISSCO repository root:

```sh
cmake -S LASS -B build-lass-cpu \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_MODULE_PATH="$PWD/cmake-modules" -DBUILD_TESTING=ON \
  -DLASS_ENABLE_CUDA=OFF \
  -DSNDFILE_INCLUDE_DIR="$LASS_SNDFILE_PREFIX/include" \
  -DSNDFILE_LIBRARY="$LASS_SNDFILE_PREFIX/lib/libsndfile.so"
cmake --build build-lass-cpu --target lass-biquad-benchmark --parallel 4

cmake -S LASS -B build-lass-cuda \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 \
  -DCMAKE_MODULE_PATH="$PWD/cmake-modules" -DBUILD_TESTING=ON \
  -DLASS_ENABLE_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=80 \
  -DSNDFILE_INCLUDE_DIR="$LASS_SNDFILE_PREFIX/include" \
  -DSNDFILE_LIBRARY="$LASS_SNDFILE_PREFIX/lib/libsndfile.so"
cmake --build build-lass-cuda \
  --target lass-cuda-biquad-test lass-biquad-benchmark --parallel 4
```

`80` targets NVIDIA A100 GPUs; select the architecture of the GPU used on a
different machine. The CUDA configuration must print **`CUDA biquad filtering
enabled`**. If it instead says the compiler or toolkit is unavailable, the
CUDA build did not enable GPU filtering. In that case, verify `command -v nvcc`
and `nvcc --version`, then reconfigure. Successfully building
`lass-cuda-biquad-test` is an additional check that CMake created the CUDA
target. A missing target indicates that CUDA was not enabled. If `nvcc` rejects
the host compiler, load a compiler supported by that CUDA toolkit and
configure a fresh CUDA build directory.

After pulling a revision that adds or changes CMake targets, rerun the
`cmake -S LASS -B ...` command for each existing build directory before using
`cmake --build`. If `build-lass-cpu` does not exist, the CPU configure command
above creates it. If Make reports a missing `lass-biquad-benchmark` target,
the build directory has not been regenerated with this target; rerun its
configure command, then build. Keep the same libsndfile
paths and CUDA module when reconfiguring. If the libsndfile prefix is no longer
set in the shell, inspect the paths used by an earlier CUDA build:

```sh
grep -E '^SNDFILE_(INCLUDE_DIR|LIBRARY):' build-lass-cuda/CMakeCache.txt
```

## Example: UIUC NCSA Delta

Delta uses [Lmod modules](https://docs.ncsa.illinois.edu/systems/delta/en/latest/user_guide/software.html)
for software and [Slurm jobs](https://docs.ncsa.illinois.edu/systems/delta/en/latest/user_guide/running_jobs.html)
for GPU access. The example uses a CUDA 13.2 module and an A100 GPU. Module
versions, partitions, and accounts can change; inspect the current system
before using the example. A CUDA module supplies the toolkit, but `nvidia-smi`
must be checked inside a GPU allocation rather than on a login node.

A prior correctness run on Delta used an A100-SXM4-40GB. Its `nvidia-smi`
output reported driver `595.71.05` and `CUDA Version: 13.2`. That CUDA version
is the driver's reported capability; check `nvcc --version` separately for the
toolkit selected for a new build.

On a login node, check the module and available GPU account. Load the module
before configuring both build directories with the commands above:

```sh
module spider cuda
module spider cuda/13.2
module load cuda/13.2
module list
command -v nvcc
nvcc --version
accounts
sinfo -p gpuA100x4-interactive
```

`command -v nvcc` must print a path. `accounts` lists usable projects; select
one that permits GPU jobs. `sinfo` checks that the example partition exists.
If `cuda/13.2` is unavailable, choose an available CUDA module and verify that
its compiler supports the loaded C++ compiler. Use the CUDA build commands in
the previous section after loading the module.

Set the account to a value reported by `accounts`, then request one GPU. Use
the same allocation for the CPU and CUDA runs so they use the same host:

```sh
export DELTA_GPU_ACCOUNT=your_gpu_account
srun --account="$DELTA_GPU_ACCOUNT" --partition=gpuA100x4-interactive \
  --time=00:30:00 --nodes=1 --ntasks=1 --cpus-per-task=4 \
  --mem=16g --gpus-per-node=1 --pty bash
```

Inside the allocated shell, return to the DISSCO repository root if needed,
set the same libsndfile paths used for the build, and verify the runtime
environment. Change the prefix below if an existing installation was used:

```sh
export LASS_SNDFILE_PREFIX="$HOME/.local/libsndfile-1.2.2"
export LD_LIBRARY_PATH="$LASS_SNDFILE_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
printenv SLURM_JOB_ID
hostname
module list
nvidia-smi -L
nvidia-smi
ls -l "$LASS_SNDFILE_PREFIX/lib/libsndfile.so"
ldd ./build-lass-cuda/lass-cuda-biquad-test
```

`SLURM_JOB_ID` must be set, `nvidia-smi -L` must list an NVIDIA GPU, and
`ldd` must not report `not found` for libsndfile or another shared library.
The driver version shown by `nvidia-smi` and the toolkit version shown by
`nvcc --version` are different checks.

If `lass-cuda-biquad-test` prints `No usable CUDA device; GPU test skipped`,
the CUDA result has **not** been tested. Check `hostname` and `nvidia-smi -L`
before retrying. Delta's [login nodes have no GPUs](https://docs.ncsa.illinois.edu/systems/delta/en/latest/user_guide/architecture.html);
return to a login shell and start the `srun --pty bash` GPU allocation above
if the current host is a login node. A CUDA module provides `nvcc` for builds,
but loading it does not allocate a GPU. An `salloc` allocation alone can leave
the shell on the login node; launch a command with `srun` or use the interactive
`srun --pty bash` example above.

If the prompt is on a GPU compute node within a GPU allocation and
`nvidia-smi -L` still cannot communicate with the driver, the node's GPU is
not usable for this test. Record `hostname`, `SLURM_JOB_ID`, and the
`nvidia-smi` error, then try a new GPU allocation or report the issue to Delta
support. Installing a driver in a user environment will not resolve a missing
node driver.

## Run the correctness test and benchmark

Run these commands **inside the GPU allocation**, after the environment checks:

```sh
./build-lass-cuda/lass-cuda-biquad-test
./build-lass-cpu/lass-biquad-benchmark > cpu.csv
./build-lass-cuda/lass-biquad-benchmark > cuda.csv
python3 LASS/CUDA/compare_benchmark.py cpu.csv cuda.csv
```

The correctness test compares the CUDA kernel with a CPU recurrence and checks
two independent channels through the public filter path. Exit status `0` is a
pass; `1` is a failure; `77` means it skipped because no usable GPU was found.
Its near-unit-pole case permits `0.001` absolute error; other cases use a
`0.0002` absolute limit plus a small relative term. Check this test before
interpreting benchmark results.

The benchmark prints CSV. It warms up once, then reports the median of three
timed runs for each case. `filter` cases time only multichannel biquad filtering
on generated three-tone audio, scaling from 1 second and 2 channels to 32
seconds and 8 channels. `render` cases time a complete LASS `Sound::render`
call with 8 to 32 partials, tremolo, vibrato, phase modulation, mixing, and
the biquad filter. At fixed duration and channel count, more partials increase
synthesis work but not the filter's input size. The CUDA benchmark checks that
every timed channel actually completed on the GPU and exits with an error if
the CUDA path fell back to CPU.

The comparison script reports **CPU time / CUDA time** for each matching case.
For example, `2.0x` means CUDA took half as long; `0.5x` means CUDA took twice
as long. A larger synthesis does not guarantee a larger GPU benefit because
only the filter step is accelerated. The CSV files include a sampled output
magnitude for a basic output sanity check; the correctness test is the numeric
comparison.

For a quick environment check, append `--quick --repetitions=1` to both
benchmark commands. To run only one group, append `--suite=filter` or
`--suite=render` to both commands. Exit the allocated shell when finished to
release the GPU job.

## Implementation notes

The CPU and CUDA versions use the same five normalized biquad coefficients.
The filter keeps two previous input samples and two previous output samples;
each channel starts with zero history. The CUDA implementation divides each
channel into 256-sample chunks, summarizes each chunk's effect on filter state,
scans those summaries to find the starting state for each chunk, and then
filters the chunks in parallel. It copies a channel back only after CUDA work
completes successfully. Floating-point operation order differs from the CPU
recurrence, so small output differences are expected. The corrected CPU filter
also resets both histories between channels; audio can differ from older
builds that reused output history.
