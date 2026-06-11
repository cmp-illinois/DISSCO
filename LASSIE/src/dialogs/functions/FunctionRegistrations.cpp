// Central list of every FunctionWidget subclass registered with the
// FunctionRegistry at startup. Adding a new ported function should be a
// one-line change in this file (plus the new subclass itself).

#include "FunctionRegistry.hpp"

#include "impl/ChooseLFunction.hpp"
#include "impl/DecayFunction.hpp"
#include "impl/EnvLibFunction.hpp"
#include "impl/ExpandPatternFunction.hpp"
#include "impl/FibonacciFunction.hpp"
#include "impl/GetPatternFunction.hpp"
#include "impl/InverseFunction.hpp"
#include "impl/LNFunction.hpp"
#include "impl/MakeEnvelopeFunction.hpp"
#include "impl/MakeFilterFunction.hpp"
#include "impl/MakeListFunction.hpp"
#include "impl/MakePatternFunction.hpp"
#include "impl/MakeSieveFunction.hpp"
#include "impl/MarkovFunction.hpp"
#include "impl/RandomDensityFunction.hpp"
#include "impl/RandomFunction.hpp"
#include "impl/RandomIntFunction.hpp"
#include "impl/RandomOrderIntFunction.hpp"
#include "impl/RandomizerFunction.hpp"
#include "impl/ReadFileFunctions.hpp"
#include "impl/RevFunctions.hpp"
#include "impl/SelectFunction.hpp"
#include "impl/SPAFunction.hpp"
#include "impl/StochosFunction.hpp"
#include "impl/SpectrumGenFunction.hpp"
#include "impl/StaticConstantFunctions.hpp"
#include "impl/ValuePickFunction.hpp"

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
    r.registerType<ReadENVFileFunction>();
    r.registerType<ReadSIVFileFunction>();
    r.registerType<ReadPATFileFunction>();
    r.registerType<ReadSPAFileFunction>();
    r.registerType<ReadREVFileFunction>();
    r.registerType<ReadFILFileFunction>();
    r.registerType<CurrentTypeFunction>();
    r.registerType<CurrentChildNumFunction>();
    r.registerType<CurrentPartialNumFunction>();
    r.registerType<CurrentSegmentFunction>();
    r.registerType<AvailableEduFunction>();
    r.registerType<PreviousChildDurationFunction>();
    r.registerType<CurrentLayerFunction>();
    r.registerType<DecayFunction>();
    r.registerType<GetPatternFunction>();
    r.registerType<ExpandPatternFunction>();
    r.registerType<MakeFilterFunction>();
    r.registerType<MakePatternFunction>();
    r.registerType<MakeEnvelopeFunction>();
    r.registerType<MakeSieveFunction>();
    r.registerType<ValuePickFunction>();
    r.registerType<SelectFunction>();
    r.registerType<StochosFunction>();
    r.registerType<SPAFunction>();
    r.registerType<RevSimpleFunction>();
    r.registerType<RevMediumFunction>();
    r.registerType<RevAdvancedFunction>();
}
