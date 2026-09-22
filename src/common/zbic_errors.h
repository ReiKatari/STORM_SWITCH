/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef ZBIC_ERRORS_H_398273423
#define ZBIC_ERRORS_H_398273423

#if defined (__cplusplus)
extern "C" {
#endif

/* =====   ZBICERRORLIB_API : control library symbols visibility   ===== */
#ifndef ZBICERRORLIB_VISIBLE
   /* Backwards compatibility with old macro name */
#  ifdef ZBICERRORLIB_VISIBILITY
#    define ZBICERRORLIB_VISIBLE ZBICERRORLIB_VISIBILITY
#  elif defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZBICERRORLIB_VISIBLE __attribute__ ((visibility ("default")))
#  else
#    define ZBICERRORLIB_VISIBLE
#  endif
#endif

#ifndef ZBICERRORLIB_HIDDEN
#  if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__MINGW32__)
#    define ZBICERRORLIB_HIDDEN __attribute__ ((visibility ("hidden")))
#  else
#    define ZBICERRORLIB_HIDDEN
#  endif
#endif

#if defined(ZBIC_DLL_EXPORT) && (ZBIC_DLL_EXPORT==1)
#  define ZBICERRORLIB_API __declspec(dllexport) ZBICERRORLIB_VISIBLE
#elif defined(ZBIC_DLL_IMPORT) && (ZBIC_DLL_IMPORT==1)
#  define ZBICERRORLIB_API __declspec(dllimport) ZBICERRORLIB_VISIBLE /* It isn't required but allows to generate better code, saving a function pointer load from the IAT and an indirect jump.*/
#else
#  define ZBICERRORLIB_API ZBICERRORLIB_VISIBLE
#endif

/*-*********************************************
 *  Error codes list
 *-*********************************************
 *  Error codes _values_ are pinned down since v1.3.1 only.
 *  Therefore, don't rely on values if you may link to any version < v1.3.1.
 *
 *  Only values < 100 are considered stable.
 *
 *  note 1 : this API shall be used with static linking only.
 *           dynamic linking is not yet officially supported.
 *  note 2 : Prefer relying on the enum than on its value whenever possible
 *           This is the only supported way to use the error list < v1.3.1
 *  note 3 : ZBIC_isError() is always correct, whatever the library version.
 **********************************************/
typedef enum {
  ZBIC_error_no_error = 0,
  ZBIC_error_GENERIC  = 1,
  ZBIC_error_prefix_unknown                = 10,
  ZBIC_error_version_unsupported           = 12,
  ZBIC_error_frameParameter_unsupported    = 14,
  ZBIC_error_frameParameter_windowTooLarge = 16,
  ZBIC_error_corruption_detected = 20,
  ZBIC_error_checksum_wrong      = 22,
  ZBIC_error_literals_headerWrong = 24,
  ZBIC_error_dictionary_corrupted      = 30,
  ZBIC_error_dictionary_wrong          = 32,
  ZBIC_error_dictionaryCreation_failed = 34,
  ZBIC_error_parameter_unsupported   = 40,
  ZBIC_error_parameter_combination_unsupported = 41,
  ZBIC_error_parameter_outOfBound    = 42,
  ZBIC_error_tableLog_tooLarge       = 44,
  ZBIC_error_maxSymbolValue_tooLarge = 46,
  ZBIC_error_maxSymbolValue_tooSmall = 48,
  ZBIC_error_cannotProduce_uncompressedBlock = 49,
  ZBIC_error_stabilityCondition_notRespected = 50,
  ZBIC_error_stage_wrong       = 60,
  ZBIC_error_init_missing      = 62,
  ZBIC_error_memory_allocation = 64,
  ZBIC_error_workSpace_tooSmall= 66,
  ZBIC_error_dstSize_tooSmall = 70,
  ZBIC_error_srcSize_wrong    = 72,
  ZBIC_error_dstBuffer_null   = 74,
  ZBIC_error_noForwardProgress_destFull = 80,
  ZBIC_error_noForwardProgress_inputEmpty = 82,
  /* following error codes are __NOT STABLE__, they can be removed or changed in future versions */
  ZBIC_error_frameIndex_tooLarge = 100,
  ZBIC_error_seekableIO          = 102,
  ZBIC_error_dstBuffer_wrong     = 104,
  ZBIC_error_srcBuffer_wrong     = 105,
  ZBIC_error_sequenceProducer_failed = 106,
  ZBIC_error_externalSequences_invalid = 107,
  ZBIC_error_maxCode = 120  /* never EVER use this value directly, it can change in future versions! Use ZBIC_isError() instead */
} ZBIC_ErrorCode;

ZBICERRORLIB_API const char* ZBIC_getErrorString(ZBIC_ErrorCode code);   /**< Same as ZBIC_getErrorName, but using a `ZBIC_ErrorCode` enum argument */


#if defined (__cplusplus)
}
#endif

#endif /* ZBIC_ERRORS_H_398273423 */