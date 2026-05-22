#ifndef LASSIE_HPP
#define LASSIE_HPP

enum class FunctionReturnType {
  functionReturnInt,
  functionReturnFloat,
  functionReturnSPA,
  functionReturnREV,
  functionReturnENV,
  functionReturnSIV,
  functionReturnPAT,
  funcitonReturnMEA,
  functionReturnFIL, // added for filter object
  functionReturnMGP, // ZIYUAN CHEN, July 2023 - added for "Modifier Group"
  functionReturnSPE, //added for generating spectrum from distance
  functionReturnIntList,
  functionReturnFloatList,
  functionReturnEnvelopeList,
  functionReturnString, //used to parse FileValue
  functionReturnList,
  functionReturnMakeListFun
};

enum class ModifierType {
  modifierTremolo = 0,
  modifierVibrato = 1,
  modifierGlissando = 2,
  modifierDetune = 3,
  modifierAmptrans = 4,
  modifierFreqtrans = 5,
  modifierWave_type = 6
};

enum class CMODFunction {
  functionRandom,
  functionRandomInt,
  functionRandomOrderInt,
  functionRandomSeed,
  functionRandomizer,
  functionRandomDensity,
  functionInverse,
  functionLN,
  functionFibonacci,// not in used??
  functionDecay,

  functionMarkov, // generate from markov

  functionStochos,
  functionEnvLib,
  functionSelect,
  functionValuePick,

  functionChooseL,
  functionGetPattern,
  functionExpandPattern,
  functionAdjustPattern, // not in used??
  functionMakeEnvelope,
  functionMakeSieve,
  functionMakePattern,
  functionMakeList,
  functionRawList,
  functionReadENVFile,
  functionReadSIVFile,
  functionReadPATFile,
  functionReadSPAFile,
  functionReadREVFile,
  functionReadFILFile,

  functionSPA,

  functionREV_Simple,
  functionREV_Medium,
  functionREV_Advanced,

  functionMakeFilter,

  function_staticCURRENT_TYPE,
  function_staticCURRENT_CHILD_NUM,
  function_staticCURRENT_PARTIAL_NUM,
  function_staticCURRENT_DENSITY,
  function_staticCURRENT_SEGMENT,
  function_staticAVAILABLE_EDU,
  function_staticCURRENT_LAYER,
  function_staticPREVIOUS_CHILD_DURATION,

  functionSpectrum_Gen,

  NOT_A_FUNCTION
};

#endif
