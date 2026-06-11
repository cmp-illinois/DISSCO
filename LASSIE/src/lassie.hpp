#ifndef LASSIE_HPP
#define LASSIE_HPP

/**
 * @file lassie.hpp
 * @brief Project-wide enum definitions for LASSIE.
 *
 * LASSIE is the graphical composition environment for DISSCO. This file holds
 * the enums shared across UI dialogs, the function registry, and the project
 * model: the CMOD "functions" the user can drop into an event tree, the return
 * type those functions produce, and the modifier types attached to bottom
 * events. Definitions here mirror the schema understood by CMOD when parsing
 * a `.dissco` project on the command line.
 */

/**
 * @brief Value categories produced by a CMOD function call.
 *
 * Used by FunctionRegistry and the dialog factories to know which editor /
 * field type to present when an event attribute is bound to a function.
 */
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

/**
 * @brief Per-note synthesis modifiers exposed by the Modifiers widget.
 *
 * Ordering must stay in sync with CMOD's Modifier parsing — the integer value
 * is what gets serialized into the project XML.
 */
enum class ModifierType {
  modifierTremolo = 0,
  modifierVibrato = 1,
  modifierGlissando = 2,
  modifierDetune = 3,
  modifierAmptrans = 4,
  modifierFreqtrans = 5,
  modifierWave_type = 6
};

/**
 * @brief Identifier for every CMOD-callable function the user can pick.
 *
 * The function registry (FunctionRegistry) maps each value to a
 * FunctionWidget implementation under dialogs/functions/impl, and to the
 * textual form CMOD expects to find in a project file. NOT_A_FUNCTION is the
 * sentinel returned when a lookup fails.
 */
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
