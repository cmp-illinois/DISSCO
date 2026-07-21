/**
 * @file SignalHandlers.h
 * @brief Custom POSIX signal handlers for CMOD (Rubin Du, 2024).
 *
 * Right now only @ref segfaultHandler is wired up — it prints a symbolized
 * stack trace before exiting, so a crash mid-render produces something
 * actionable in the terminal log. The interrupt / terminate / abort hooks
 * are declared but intentionally left empty; they exist as anchor points
 * for cleanup logic once a graceful-shutdown story is needed.
 *
 * @ref BACKTRACE_NUM bounds the number of frames captured.
 */

#ifndef SIGNALHANDLERS_H
#define SIGNALHANDLERS_H

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <cstddef>

#ifdef _WIN32

#include <io.h>

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

// Windows does not provide POSIX backtrace functions.
// These stubs preserve compilation while disabling stack traces.
inline int backtrace(void**, int) {
  return 0;
}

inline char** backtrace_symbols(void* const*, int) {
  return nullptr;
}

inline void backtrace_symbols_fd(void* const*, int, int) {
}

namespace abi {

inline char* __cxa_demangle(const char*,
                           char*,
                           std::size_t*,
                           int* status) {
  if (status != nullptr) {
    *status = -1;
  }

  return nullptr;
}

}  // namespace abi

#else

#include <execinfo.h>
#include <unistd.h>
#include <cxxabi.h>

#endif

#define BACKTRACE_NUM 10

// Rubin Du 2024
// Custom signal handler to print stack trace on segfault and then exit.
void segfaultHandler(int signal);

// Can be used to detect Ctrl+C during output generation.
void interruptHandler(int signal);

// Can be used for unexpected termination handling.
void terminateHandler();

// Can be used with assert or invalid-input handling.
void abortHandler();

#endif