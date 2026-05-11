
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

#include <iostream>
#include <csignal>
#include <execinfo.h>
#include <cstdlib>
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