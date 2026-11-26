#pragma once
#include "masterlabpro_module_abi.h"

#define MLP_INPUT_COMPRESSOR_HI UINT64_C(0x4d4c50434f4d5031)
#define MLP_INPUT_COMPRESSOR_LO UINT64_C(0x2495a158935cd387)

/* Optional v1 telemetry export. Returns nonnegative dB reduction for LOW, MID,
   HIGH, SUM (indices 0..3). Safe concurrently with process, while instance lives. */
typedef float (MLP_CALL *MlpReadGainReductionV1)(const MlpInstance*, uint32_t);
