#include "SignalHandlers.h"

using namespace std;

void segfaultHandler(int signal) {
    void *buf[BACKTRACE_NUM + 2];
    size_t size = backtrace(buf, BACKTRACE_NUM + 2);        // Do a backtrace of the stack
    char **messages = backtrace_symbols(buf, size);

    std::cerr << "--------------------------------------------------------------------------------\n";
    std::cerr << "Segmentation Fault, printing stacktrace of " << BACKTRACE_NUM << " most recent function calls:\n\n";


    for (size_t i = 2; i < size; ++i) {                     //Skip the first two frames since they are this function and the signal generator
        size_t len=std::strlen(messages[i]);
        char* parser;

        //parse the backtrace message to find function name and its respective memory offset
        parser = std::find(messages[i], messages[i] + len, '(');
        char* function = parser==messages[i]+len ? NULL : parser;

        parser = std::find(messages[i], messages[i] + len, '+');
        char* start = parser==messages[i]+len ? NULL : parser;

        parser = std::find(messages[i], messages[i] + len, ')');
        char* end = parser==messages[i]+len ? NULL : parser;


        if (function && start && end && function < start) { //If the function name and mem offset are found
            *function++ = '\0';
            *start++ = '\0';
            *end = '\0';

            int status;
            char *real_name = abi::__cxa_demangle(function, NULL, NULL, &status);   //Demangle the function name

            if (status)                                     //If the demangling failed, print the mangled name                                   
                std::cerr << "--External to CMOD  " << function << " mem offset:" << start << "\n";
            else
                std::cerr << "--" << real_name << " mem offset:" << start << "\n";

            free(real_name);
        } 
        
        else
            std::cerr << "External to CMOD  " << messages[i] << "\n";
    }
    std::cerr << "\n--------------------------------------------------------------------------------\n";

    free(messages);

    std::signal(SIGSEGV, SIG_DFL);                          //Reset the signal handler to default and raise it again to properly exit
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