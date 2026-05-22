// Central list of every FunctionWidget subclass registered with the
// FunctionRegistry at startup. Adding a new ported function should be a
// one-line change in this file (plus the new subclass itself).

#include "FunctionRegistry.hpp"
#include "ChooseLFunction.hpp"
#include "FibonacciFunction.hpp"
#include "InverseFunction.hpp"
#include "LNFunction.hpp"
#include "MarkovFunction.hpp"
#include "RandomFunction.hpp"
#include "RandomIntFunction.hpp"
#include "RandomOrderIntFunction.hpp"

void registerAllFunctions() {
    auto& r = FunctionRegistry::instance();
    r.registerType<InverseFunction>();
    r.registerType<LNFunction>();
    r.registerType<FibonacciFunction>();
    r.registerType<ChooseLFunction>();
    r.registerType<MarkovFunction>();
    r.registerType<RandomFunction>();
    r.registerType<RandomIntFunction>();
    r.registerType<RandomOrderIntFunction>();
}
