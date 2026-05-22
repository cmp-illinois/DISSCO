// Central list of every FunctionWidget subclass registered with the
// FunctionRegistry at startup. Adding a new ported function should be a
// one-line change in this file (plus the new subclass itself).

#include "FunctionRegistry.hpp"
#include "InverseFunction.hpp"

void registerAllFunctions() {
    auto& r = FunctionRegistry::instance();
    r.registerType<InverseFunction>();
}
