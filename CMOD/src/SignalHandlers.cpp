#include "SignalHandlers.h"

using namespace std;

void segfaultHandler(int /*signal*/) {
    std::cerr << "--------------------------------------------------------------------------------\n"
              << "Segmentation fault. Re-raising with default handler for OS diagnostics.\n"
              << "(Run under a debugger — gdb / lldb / MSVC — for a stack trace.)\n"
              << "--------------------------------------------------------------------------------"
              << std::endl;

    // Reset to the default handler and re-raise so the OS produces its usual
    // crash artifacts (core dump on POSIX, Windows Error Reporting on Win32).
    std::signal(SIGSEGV, SIG_DFL);
    std::raise(SIGSEGV);
}

// Unimplemented
void interruptHandler(int signal) {
    exit(1);
}

// Unimplemented
void terminateHandler() {
    exit(1);
}

// Unimplemented
void abortHandler() {
    exit(1);
}
