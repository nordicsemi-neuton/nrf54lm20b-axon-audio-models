/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */
#define static_assert _Static_assert
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "drivers/axon/nrf_axon_driver.h"
#include "axon/nrf_axon_platform.h"
#include "axon/nrf_axon_logging.h"
#include "drivers/axon/nrf_axon_dsp_intrinsics.h"
#include "axon/mfcc/axon_mfcc.h"
#include AXON_MFCC_CONST_FILE_NAME
#include <assert.h>

// algorithm develop mode. Run the intrinsics, don't try to compile them
#define AXON_FE_MODE_DEVELOP 1

#if NRF_AXON_SIMULATOR_BUILD
# include "axon/nrf_axon_platform_simulator.h"
// algorithm compiler mode. Intrinsics are executed and also compiled. Compiled command buffers written to a header file. Only supported on simulator builds
# define AXON_FE_MODE_COMPILER 2
#endif

// compiled mode. Intrinsics do not execute (but remain in place to serve as documentation). Compiled header file is included and command buffers are run.
#define AXON_FE_MODE_COMPILED 3


#if !NRF_AXON_MFCC_CALCULATE_ONLY
# define AXON_FE_MODE AXON_FE_MODE_COMPILER
#else
// choose this to develop the algorithm by running intrinsics
// # define AXON_FE_MODE AXON_FE_MODE_DEVELOP

// choose this to use the compiled command buffer
# define AXON_FE_MODE AXON_FE_MODE_COMPILED
#endif

#define AXON_NLOG_RADIX 12  // AXON log treats input as Q11.12

/**
 * Radix after fft power is twice the input radix (radix after window) minus fft power rounding.
 * This can be a negative number.
 */
#define MFCC_POST_FFT_POWER_RADIX  (2*(MFCC_WINDOW_RADIX-MFCC_WINDOW_ROUND)-MFCC_FFT_POWER_ROUND)

#if (AXON_MFCC_MFCC_FFT_MAGNITUDE) 
/**
 * if fft magnitude is in use, a square root occurs which halves the radix up to this point. Therefore 
 * the radix has be an even number going into the magnitude operation because only whole bit-level rounding is supported.
 */
# if (MFCC_POST_FFT_POWER_RADIX & 1)
    assert(0);
# else
#   define MFCC_RADIX_BEFORE_FILTERBANK (MFCC_POST_FFT_POWER_RADIX>>1)
# endif
#else 
// no magnitude so don't divide radix by 2 after FFT power
# define MFCC_RADIX_BEFORE_FILTERBANK (MFCC_POST_FFT_POWER_RADIX)
#endif

// add the radix contribution from the filterbank step.
#define MFCC_RADIX_AFTER_FILTERBANK  (MFCC_RADIX_BEFORE_FILTERBANK + MFCC_FILTER_BANK_RADIX - MFCC_FILTER_BANK_ROUND)

/**
 * calculate the net rounding that is performed before the natural log. This determines the offset that will be added
 * because axon's natural log interprets inputs (and outputs) as q11.12
 */
#define MFCC_NLOG_ADJUSTMENT_BIT_COUNT (AXON_NLOG_RADIX - MFCC_RADIX_AFTER_FILTERBANK)

/*
 * value gets added to the log of the filter banks to compensate for all the rounding
 * that has occurred and for the interpretation of the log input as a Q11.12.
 */
#define LN_2_TOTHE_1_Q12 2839.13085    /* LN(2^1) =  0.693147, but as a Q12*/
#define MFCC_NLOG_OFFSET_FLOAT (LN_2_TOTHE_1_Q12*MFCC_NLOG_ADJUSTMENT_BIT_COUNT)
#define MFCC_NLOG_OFFSET ((MFCC_NLOG_OFFSET_FLOAT >= 0.0) ? (int)(MFCC_NLOG_OFFSET_FLOAT + 0.5) : (int)(MFCC_NLOG_OFFSET_FLOAT - 0.5))

// radix of the unquantized MFCC output. This is 12 plus the net extra radix after the DCT
#define AXON_MFCC_UNQUANTIZED_RADIX (AXON_NLOG_RADIX+MFCC_DCT_RADIX-MFCC_DCT_ROUND)

static struct {
  uint32_t profiling_ticks;
} axon_mfcc_state_info;

static int32_t axon_mfcc_one_ptr = 1;
static int32_t axon_mfcc_fft_power_ln_offset_ptr = MFCC_NLOG_OFFSET;

static int32_t axon_mfcc_quant_zeropoint_ptr = 0;
static int32_t axon_mfcc_zero_ptr = 0;
static int32_t axon_mfcc_quantization_inv_scale_factor_ptr = 1; 

/*
* intermediate results buffer
*/
static union {
  int32_t fft[(AXON_MFCC_FFT_LEN+1)*2]; // sized for 512 complex numbers, but only the 1st half is used after FFT operation

  struct {
    int32_t fft_1st_half[AXON_MFCC_FRAME_LEN+1]; // holds the 1st 256 complex numbers
    int32_t after_filter_banks[AXON_MFCC_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS]; // filter bank and later results go here
    int32_t fft_energy[FILTER_BANK_EXTRA_COEFFS];
    AXON_MFCC_DATA_TYPE quant_output[AXON_MFCC_WIDTH];
  };
} axon_mfcc_buffers;

#if AXON_FE_MODE==AXON_FE_MODE_COMPILED

  // in compiled mode, need a cmd buffer info instance
static nrf_axon_cmd_buffer_info_s mfcc_cmd_buf_info;
  // include the compiled command buffer header file. It has references to symbols decared above.
# include AXON_MFCC_CMD_BUF_FILE_NAME

#elif AXON_FE_MODE==AXON_FE_MODE_COMPILER
  // need compiler apis
# include "nrf_axon_nn_compiler.h"
void axon_compiler_start_intrinsic_compiling(const char *cmd_buf_name,
      const nrf_axon_compiler_symbol_desc_s *symbol_list,
      int symbol_list_cnt);
int axon_compiler_stop_intrinsic_compiling();
#endif

int axon_mfcc_calculate_init(
  int32_t quant_inv_scale_factor,
  int8_t quant_inv_scale_factor_radix,
  int8_t quant_zero_point
) {

  axon_mfcc_quant_zeropoint_ptr = quant_zero_point;
  if (quant_inv_scale_factor_radix > MFCC_QUANT_INV_SCALE_FACTOR_RADIX) {
    axon_mfcc_quantization_inv_scale_factor_ptr = quant_inv_scale_factor >> (quant_inv_scale_factor_radix - MFCC_QUANT_INV_SCALE_FACTOR_RADIX);
  } else {
    axon_mfcc_quantization_inv_scale_factor_ptr = quant_inv_scale_factor << (MFCC_QUANT_INV_SCALE_FACTOR_RADIX-quant_inv_scale_factor_radix);
  }
  

  // nrf_axon_platform_printf("AXON_FE_MODE = %d\n", AXON_FE_MODE);

#if AXON_FE_MODE!=AXON_FE_MODE_COMPILED
  return 0;
#else
  // nrf_axon_platform_printf("AXON_MFCC_CMD_BUF_FILE_NAME = %s\n", AXON_MFCC_CMD_BUF_FILE_NAME);
  // compiled mode can be asynchronous
  nrf_axon_init_command_buffer_info(&mfcc_cmd_buf_info, cmd_buffer_axon_mfcc, sizeof(cmd_buffer_axon_mfcc)/sizeof(cmd_buffer_axon_mfcc[0]));
  // make sure pointers aren't null or 0 length
  return (mfcc_cmd_buf_info.cmd_buf_ptr==NULL) ||
        (mfcc_cmd_buf_info.length==0) ? - 1 : 0;
#endif


}

void axon_mfcc_restart() {
}

uint32_t axon_mfcc_return_profiling_ticks() {
  return axon_mfcc_state_info.profiling_ticks;
}

#if AXON_FE_MODE==AXON_FE_MODE_COMPILER
/**
 * declare all the symbols that will be referenced in the compiled command buffer.
 */
static const nrf_axon_compiler_symbol_desc_s mfcc_compiler_symbols[] = {
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_ARRAY_ELEMENT(mfcc_window), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_ARRAY_ELEMENT(axon_mfcc_filter_banks), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_ARRAY_ELEMENT(axon_mfcc_dct), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(axon_mfcc_one_ptr), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(axon_mfcc_fft_power_ln_offset_ptr), 
#if AXON_MFCC_QUANT_ENABLE
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(axon_mfcc_quant_zeropoint_ptr), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(axon_mfcc_zero_ptr), 
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(axon_mfcc_quantization_inv_scale_factor_ptr), 
#endif
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_ARRAY_ELEMENT(axon_mfcc_buffers.fft), 
#if MFCC_APPEND_ENERGY
  NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(&axon_mfcc_log_offset_add), 
#endif

};
#endif //  AXON_FE_MODE==AXON_FE_MODE_COMPILER

/*
 * API function
 */
nrf_axon_result_e axon_mfcc_process_frame(
    const int16_t *input_ping,
    uint32_t ping_count,
    const int16_t *input_pong,
    bool last_frame, 
    uint8_t input_stride,
    void *output_buffer) 
 { 
#define AXON_MFCC_WAIT_MODE NRF_AXON_SYNC_MODE_BLOCKING_POLLING
// #define AXON_MFCC_WAIT_MODE NRF_AXON_SYNC_MODE_BLOCKING_EVENT
  nrf_axon_result_e result;

#if !(CONFIG_AXONS_ML_PROFILING)
  nrf_axon_platform_set_profiling_gpio();
  axon_mfcc_state_info.profiling_ticks = nrf_axon_platform_get_ticks();
#endif 

  // copy data and multiply by hamming window the "ping" portion of the input, place into fft buffer.
  // this needs to be an intrinsic because before locations & sizes are not known at compile time.
  
  /*
   * Input are 16bit (q15) audio samples and 16bit (q14) coeficients with a maximum value of 1. Rounding of 14
   * prevents the output from exceeding q15 and overflowing the 24bit FFT input maximum.
  */
  if (0 > (result = axon_xty_16_16_32_output_stride(input_ping, mfcc_window, axon_mfcc_buffers.fft, 
                                          ping_count, MFCC_WINDOW_ROUND, 1, AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }

  if (AXON_MFCC_FRAME_LEN > ping_count) {
    // do the pong portion. fft has a factor of 2 offset because it is complex pairs.
    if (0 > (result = axon_xty_16_16_32_output_stride(input_pong, mfcc_window+ping_count, axon_mfcc_buffers.fft + (2 *ping_count), 
                                            AXON_MFCC_FRAME_LEN-ping_count, MFCC_WINDOW_ROUND, 1, AXON_MFCC_WAIT_MODE, true))) {
      return result;
    }
  }

#if AXON_FE_MODE!=AXON_FE_MODE_COMPILED
  // not in compiled mode, so use intrinsics

# if AXON_FE_MODE==AXON_FE_MODE_COMPILER
  // kickoff the background compiler (1st time through only)
  static bool already_compiled = false;
  if (!already_compiled) {
    axon_compiler_start_intrinsic_compiling("axon_mfcc", mfcc_compiler_symbols, sizeof(mfcc_compiler_symbols)/sizeof(*mfcc_compiler_symbols));
  }
# endif

  // windowed input has been copied to real coefficients in the FFT buffer. Need to 0-out the imaginary coefficients
  if (0 > (result=axon_memset_32_output_stride(0, axon_mfcc_buffers.fft + 1, AXON_MFCC_FRAME_LEN, 1, AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }
  // 0 stuff any required FFT input padding, real & imaginary coefficients
  if (AXON_MFCC_FFT_LEN > AXON_MFCC_FRAME_LEN) {
    if (0 > (result=axon_memset_32_output_stride(0, axon_mfcc_buffers.fft + (2*AXON_MFCC_FRAME_LEN), 2*(AXON_MFCC_FFT_LEN-AXON_MFCC_FRAME_LEN), 0, AXON_MFCC_WAIT_MODE, true))) {
      return result;
    }
  }

  // fft power. There is an implied rounding of AXON_MFCC_FFT_LEN_LOG2 built into the fft_power intrinsic, so only do the extra padding needed on top of that. 
  if (0 > (result=axon_fft_power_24(axon_mfcc_buffers.fft, axon_mfcc_buffers.fft, AXON_MFCC_FFT_LEN_LOG2, true, MFCC_FFT_POWER_ROUND,AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }
#if AXON_MFCC_MFCC_FFT_MAGNITUDE
if (0 > (result=axon_sqrt_24(axon_mfcc_buffers.fft, axon_mfcc_buffers.fft, AXON_MFCC_FFT_LEN, AXON_MFCC_WAIT_MODE, true))) {
  return result;
}
#endif

  // filter banks. 1 MAR per filter. lengths, inputs, and coefficients are different for each one.
  for (int bank_ndx=0; bank_ndx < AXON_MFCC_FILTERBANK_COUNT; bank_ndx++) {
    if (sizeof(axon_mfccc_coefs_t)==2) {
      result=axon_mar_16_24_24(filter_banks[bank_ndx].coefs, &axon_mfcc_buffers.fft[filter_banks[bank_ndx].first_tap], &axon_mfcc_buffers.after_filter_banks[bank_ndx], filter_banks[bank_ndx].tap_count, kAxonRoundingNone+MFCC_FILTER_BANK_ROUND, AXON_MFCC_WAIT_MODE, true);
    } else {
      result = -1;
    }
    if (0 > result) {
      return result;
    }
  }
#if AXON_MFCC_MFCC_ENERGY_APPEND
  if (0 > (result = MfccAxonOpFftPowerSum())) { return result; } 
#endif

  // natural log
  if (0 > (result=axon_logn_11p12(axon_mfcc_buffers.after_filter_banks, axon_mfcc_buffers.after_filter_banks, AXON_MFCC_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS, AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }

  // add the log offset to correct for mis-matched radix
# if AXON_MFCC_MFCC_ENERGY_APPEND
  if (0 > (result = MfccAxonOpAddLogOffsetVector())) { return result; } 
# else
  if (0 > (result=axon_axpb_24_24(axon_mfcc_buffers.after_filter_banks, &axon_mfcc_one_ptr, &axon_mfcc_fft_power_ln_offset_ptr, axon_mfcc_buffers.after_filter_banks, AXON_MFCC_FILTERBANK_COUNT, kAxonRoundingNone, AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }
# endif

  // dcts are a MAR followed by a series of MARXs (MAR where only the X is reloaded, in this case the dct coeficients)
  int32_t *output = axon_mfcc_buffers.after_filter_banks;

  // 1st row is a MAR because we will load the input into the "y_input"
  if (0 > (result=axon_mar_16_24_24((int16_t*)axon_mfcc_dct, axon_mfcc_buffers.after_filter_banks, output, AXON_MFCC_FILTERBANK_COUNT, kAxonRoundingNone + MFCC_DCT_ROUND, AXON_MFCC_WAIT_MODE, true))) {
    return result;
  }

  // subsequent rows are Marx; the y is not reloaded, it is retained from before.
  for (uint8_t output_ndx=1; output_ndx<AXON_MFCC_WIDTH; output_ndx++) {
    output++; // next output.
    if (0 > (result=axon_marx_16_24((int16_t*)(axon_mfcc_dct+output_ndx), output, AXON_MFCC_FILTERBANK_COUNT, kAxonRoundingNone + MFCC_DCT_ROUND, AXON_MFCC_WAIT_MODE, true))) {
      return result;
    }
  }

#if AXON_MFCC_QUANT_ENABLE
  // quantization. quantization results are stored in axon_mfcc_buffers.quant_output
  if (0 > (result=axon_axpb_24_24(axon_mfcc_buffers.after_filter_banks, &axon_mfcc_quantization_inv_scale_factor_ptr, &axon_mfcc_zero_ptr, (int32_t *)axon_mfcc_buffers.quant_output, AXON_MFCC_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS, kAxonRoundingNone + AXON_MFCC_UNQUANTIZED_RADIX + MFCC_QUANT_INV_SCALE_FACTOR_RADIX, AXON_MFCC_WAIT_MODE, true))) {
        return result;
  }

  if (0 > (result=axon_axpb_24_24((int32_t *)axon_mfcc_buffers.quant_output, &axon_mfcc_one_ptr, &axon_mfcc_quant_zeropoint_ptr, (int32_t *)axon_mfcc_buffers.quant_output, AXON_MFCC_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS, kAxonRoundingNone, AXON_MFCC_WAIT_MODE, false))) {
      return result;
  }
#endif

# if AXON_FE_MODE==AXON_FE_MODE_COMPILER
  // stop the background compiler (1st time through only)
  if (!already_compiled) {
    axon_compiler_stop_intrinsic_compiling();
    already_compiled = true;
  }
# endif

#else // compiled mode

  if (NRF_AXON_RESULT_SUCCESS>(result=nrf_axon_run_cmd_buf_sync(&mfcc_cmd_buf_info, AXON_MFCC_WAIT_MODE, false))) {
    return result;
  }
#endif

#if AXON_MFCC_QUANT_ENABLE
  // can saturate directly to output if the output is 32bit aligned and length is a multiple of 4
# if 0==(AXON_MFCC_WIDTH & 0x3)
  if (0==(output_buffer&3)) {
    // width is a multiple of 4 and output is 32bit aligned. Saturate directly to output
    result =axon_saturate_32_8(axon_mfcc_buffers.quant_output, (int8_t*)output_buffer, AXON_MFCC_WIDTH,AXON_MFCC_WAIT_MODE, false);
  else {
    // width is a multiple of 4 but output is not 32bit aligned. Saturate to an aligned buffer then copy to output
    result =axon_saturate_32_8(axon_mfcc_buffers.quant_output, (int8_t*)axon_mfcc_buffers.quant_output, AXON_MFCC_WIDTH,AXON_MFCC_WAIT_MODE, false);
    memcpy(output_buffer, axon_mfcc_buffers.quant_output, AXON_MFCC_WIDTH);
  }
# else 
  // output length is not a multiple of 4 so will saturate to an aligned buffer then copyt to output.
    result =axon_saturate_32_8((int32_t *)axon_mfcc_buffers.quant_output, (int8_t*)axon_mfcc_buffers.quant_output, AXON_MFCC_WIDTH,AXON_MFCC_WAIT_MODE, false);
    memcpy(output_buffer, axon_mfcc_buffers.quant_output, AXON_MFCC_WIDTH);
# endif
#else
  // unquantized output is in after_filter_banks
  memcpy(output_buffer, axon_mfcc_buffers.after_filter_banks, AXON_MFCC_WIDTH*sizeof(AXON_MFCC_DATA_TYPE));
#endif

#if AXON_MFCC_MFCC_ENERGY_APPEND
   axon_mfcc_buffers.after_filter_banks[0] = axon_mfcc_buffers.fft_energy[0];
#endif

#if !(CONFIG_AXONS_ML_PROFILING)
  axon_mfcc_state_info.profiling_ticks = nrf_axon_platform_get_ticks() - axon_mfcc_state_info.profiling_ticks;
  nrf_axon_platform_clear_profiling_gpio();
#endif

  return result;
}

void axon_mfcc_get_unquantized_outputs(int32_t *output_buffer) {
  memcpy(output_buffer, axon_mfcc_buffers.after_filter_banks, AXON_MFCC_WIDTH*sizeof(int32_t));
}
