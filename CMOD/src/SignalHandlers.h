
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