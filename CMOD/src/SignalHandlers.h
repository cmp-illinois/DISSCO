
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
#include <execinfo.h>
#include <cstdlib>
#include <unistd.h>
#include <cxxabi.h>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
  #include <io.h>

  #ifndef STDERR_FILENO
    #define STDERR_FILENO 2
  #endif

  inline int backtrace(void** buffer, int size) {
    return 0;
  }

  inline char** backtrace_symbols(void* const* buffer, int size) {
    return nullptr;
  }

  inline void backtrace_symbols_fd(void* const* buffer, int size, int fd) {
  }

  namespace abi {
    inline char* __cxa_demangle(const char* mangled_name,
                                char* output_buffer,
                                size_t* length,
                                int* status) {
      if (status) {
        *status = -1;
      }
      return nullptr;
    }
  }
#else
  #include <execinfo.h>
  #include <unistd.h>
  #include <cxxabi.h>
#endif

#define BACKTRACE_NUM 10    // Number of stack frames to print

// Rubin Du 2024
// Custom signal handler to print stack trace on segfault and then exit
void segfaultHandler(int signal);

// Unimplemented, can be used to detect ctrl+c while the output is generating and decide whether to abandon and clean up the output or not, and release resources
void interruptHandler(int signal);

// Unimplemented, could be the same as the above but for quitting unexpectedly
void terminateHandler();

//Unimplemented, could be useful in conjunction with assert to detect trash inputs or other unexpected behavior
void abortHandler();

#endif