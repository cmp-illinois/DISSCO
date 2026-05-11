
//----------------------------------------------------------------------------//
//
//   SignalHandlers.h
//
//   This file contains the declarations for signal handlers that can
//   be installed for custom behaviors when a signal is raised.
//
//----------------------------------------------------------------------------//

#ifndef SIGNALHANDLERS_H
#define SIGNALHANDLERS_H

#include <csignal>
#include <cstdlib>
#include <iostream>

// Originally authored by Rubin Du (2024) with a glibc/Itanium-ABI backtrace
// printer; the backtrace was removed in 2026 when DISSCO gained Windows
// support — std::stacktrace (C++23) is the eventual portable replacement,
// but isn't broadly available yet on the toolchains we target. Until then,
// attach a debugger (gdb / lldb / MSVC) for stack-trace detail.
//
// Custom signal handler that announces a segfault and re-raises with the
// default handler so the OS produces its usual diagnostics / core dump.
void segfaultHandler(int signal);

// Unimplemented, can be used to detect ctrl+c while the output is generating and decide whether to abandon and clean up the output or not, and release resources
void interruptHandler(int signal);

// Unimplemented, could be the same as the above but for quitting unexpectedly
void terminateHandler();

//Unimplemented, could be useful in conjunction with assert to detect trash inputs or other unexpected behavior
void abortHandler();

#endif
