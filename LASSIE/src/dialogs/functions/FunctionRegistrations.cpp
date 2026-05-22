// Central list of every FunctionWidget subclass registered with the
// FunctionRegistry at startup. Adding a new ported function should be a
// one-line change in this file (plus the new subclass itself).

#include "FunctionRegistry.hpp"

#include "ChooseLFunction.hpp"
#include "EnvLibFunction.hpp"
#include "FibonacciFunction.hpp"
#include "InverseFunction.hpp"
#include "LNFunction.hpp"
#include "MakeListFunction.hpp"
#include "MarkovFunction.hpp"
#include "RandomDensityFunction.hpp"
#include "RandomFunction.hpp"
#include "RandomIntFunction.hpp"
#include "RandomOrderIntFunction.hpp"
#include "RandomizerFunction.hpp"
#include "SpectrumGenFunction.hpp"

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
    r.registerType<RandomizerFunction>();
    r.registerType<RandomDensityFunction>();
    r.registerType<EnvLibFunction>();
    r.registerType<MakeListFunction>();
    r.registerType<SpectrumGenFunction>();
}
