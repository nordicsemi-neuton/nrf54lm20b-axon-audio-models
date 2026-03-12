/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef  _AXON_MFCC_H_
#define  _AXON_MFCC_H_

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "drivers/axon/nrf_axon_dsp_intrinsics.h"
/**
 * MFCCs
 */

#include "axon/nrf_axon_stringization.h"

#define AXON_MFCC_CONST_FILE_NAME_ROOT axon/mfcc/axon_mfcc_const_
#define AXON_MFCC_PROPERTIES_FILE_NAME_ROOT axon/mfcc/axon_mfcc_properties_
#define AXON_MFCC_CMD_BUF_FILE_NAME_ROOT axon/mfcc/axon_mfcc_cmd_buf_
#define AXON_MFCC_DOT_H _.h

#define AXON_MFCC_PROPERTIES_FILE_NAME STRINGIZE_3_CONCAT(AXON_MFCC_PROPERTIES_FILE_NAME_ROOT, NRF_AXON_MFCC_NAME, AXON_MFCC_DOT_H)
#define AXON_MFCC_CONST_FILE_NAME STRINGIZE_3_CONCAT(AXON_MFCC_CONST_FILE_NAME_ROOT, NRF_AXON_MFCC_NAME, AXON_MFCC_DOT_H)
#define AXON_MFCC_CMD_BUF_FILE_NAME STRINGIZE_3_CONCAT(AXON_MFCC_CMD_BUF_FILE_NAME_ROOT, NRF_AXON_MFCC_NAME, AXON_MFCC_DOT_H)

#include AXON_MFCC_PROPERTIES_FILE_NAME

#if AXON_MFCC_QUANT_ENABLE 
typedef int8_t AXON_MFCC_DATA_TYPE; /**< ...saturates and packs to this type */
#else
typedef int32_t AXON_MFCC_DATA_TYPE; /**< ...saturates and packs to this type */
#endif

/*
 * Adds additional elements so that the ln() operation can include the FFT energy.
 */
#define FILTER_BANK_EXTRA_COEFFS 2

/*
 * Axon mfcc initialization.
 */
int axon_mfcc_init();


/**
 * Called at least 1 time to pass the axon handle & quantization parameters
*/
int axon_mfcc_calculate_init(
  int32_t quant_inv_scale_factor,
  int8_t quant_inv_scale_factor_radix,
  int8_t quant_zero_point
);
/*
 * Call this at the beginning of a new audio stream to clear out the memory of
 * the last stream.
 */
void axon_mfcc_restart();

/*
 * Call this once per audio slice. Calculates the MFCCs
 * Audio samples can be submitted in 2 discontiguous buffers, raw_input_ping and raw_input_pong.
 * raw_input_ping contains the 1st ping_count samples, and raw_input_pong contains the remaining
 * samples. There must be an even number of samples in each buffer (0 is allowed).
 * Note the pong can be null if ping has the entire window (ping_count==MEL32_AUDIO_FRAME_LEN).
 *
 * @returns
 * - Otherwise, result is an nrf_axon_result_e.
 */
nrf_axon_result_e axon_mfcc_process_frame(
    const int16_t *raw_input_ping,
    uint32_t ping_count,
    const int16_t *raw_input_pong,
    bool last_frame, 
    uint8_t input_stride,
    void *output_buffer 
    );

/**
 * Retrieves the 32bit, unquantized results from the last invocation of axon_mfcc_process_frame().
 */
void axon_mfcc_get_unquantized_outputs(int32_t *output_buffer);
/*
 * Returns time in ticks of most recent mfcc calculation.
*/
uint32_t axon_mfcc_return_profiling_ticks();

#endif

#ifdef __cplusplus
}
#endif
