/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "axon/nrf_axon_platform.h"
#include "axon/mfcc/axon_mfcc.h"
#include "axon/nrf_axon_logging.h"
#if (NOT_A_ZEPHYR_BUILD)
#include "axon/nrf_axon_platform_simulator.h"
#endif

/*
 * AUDIO TEST VECTORS
 */
#include "test_audio/rawSampleon.h"

static const struct {
  const char *sample_label;
  int32_t sample_count;
  const int16_t *wave_data;
  const int8_t *pexpected_output_quantizedected_output;
  const int32_t *expected_output_unquantized;
  const int8_t *expected_output_quantized;
}  audio_sample_files[] ={
  {
      .sample_label = AUDIO_NAME_ON,
      .sample_count = sizeof(wave_data_on)/sizeof(wave_data_on[0]),
      .wave_data = wave_data_on,
      .expected_output_unquantized = (const int32_t *)PPCAT3(expected_unquantized_mfcc_output_on, _, NRF_AXON_MFCC_NAME),
      .expected_output_quantized = (const int8_t *)PPCAT3(expected_quantized_mfcc_output_on, _, NRF_AXON_MFCC_NAME),
  }
};

/**
 * use macro chicanery to generate symbol names assocatiated with the mfcc target name.
 */
const int32_t quant_inverse_scaling_factor =  PPCAT3(MFCC_QUANT_INV_SCALE_FACTOR, _, NRF_AXON_MFCC_NAME);
const int8_t quant_inverse_scaling_factor_radix =  PPCAT3(MFCC_QUANT_INV_SCALE_FACTOR_RADIX, _, NRF_AXON_MFCC_NAME);
const int8_t quant_zero_point =  PPCAT3(MFCC_QUANT_ZERO_POINT, _, NRF_AXON_MFCC_NAME);

static struct {
  char test_name[50];
  uint32_t case_no;
  uint32_t case_count_total;
  uint32_t case_count_pass;
} axon_test_info = {0};

/*
 * Processes a single audio vector, and optionally compares the results to the passed expected_output.
 * 
 */
int axon_mfcc_process_vector(const int16_t *audio_vector, 
              uint32_t audio_vector_len, 
              uint8_t input_stride, 
              const AXON_MFCC_DATA_TYPE* expected_output,
              AXON_MFCC_DATA_TYPE* output_vector,
              int32_t* output_vector_unquantized) {
  
  nrf_axon_result_e result = NRF_AXON_RESULT_SUCCESS;
  uint32_t frame_idx;
  AXON_MFCC_DATA_TYPE output[AXON_MFCC_WIDTH];
  axon_mfcc_restart();

  /*
   * loop through the vector, one frame at a time.
   */
  for (frame_idx=0;
      frame_idx<=((audio_vector_len-AXON_MFCC_FRAME_LEN)/AXON_MFCC_FRAME_SHIFT);
      frame_idx++,audio_vector += AXON_MFCC_FRAME_SHIFT*input_stride) {

    if(NULL != expected_output) {
      nrf_axon_platform_printf("\n\rTEST:\t%s\tSTART CASE NO\t%d\n", axon_test_info.test_name, axon_test_info.case_no);
    }
    result=axon_mfcc_process_frame(audio_vector, AXON_MFCC_FRAME_LEN, NULL, false, input_stride, output);
    if (result < 0) {
      nrf_axon_platform_printf("axon_mfcc_process_frame error %d\n", result);
      // error!
      break;
    }
    if (result < 0) {
      // error!
      break;
    }
    // save the output to the output vector
    if (NULL != output_vector) {
      memcpy(output_vector+AXON_MFCC_WIDTH*frame_idx, output, sizeof(AXON_MFCC_DATA_TYPE)*AXON_MFCC_WIDTH);
    }
    // same for unquantized output
    if (NULL != output_vector_unquantized) {
      axon_mfcc_get_unquantized_outputs(output_vector_unquantized+AXON_MFCC_WIDTH*frame_idx);
    }

    // compare to exected value
    if (NULL != expected_output) {
      if (sizeof(AXON_MFCC_DATA_TYPE)==1) {
        char name[20];
        snprintf(name, sizeof(name), "frame_%02d", frame_idx);
        nrf_axon_print_int8_vector(name, output, AXON_MFCC_WIDTH);
        if(0 == nrf_axon_verify_vectors_8(name, output, expected_output+frame_idx*AXON_MFCC_WIDTH, AXON_MFCC_WIDTH, 1)){
          nrf_axon_platform_printf("\n\rTEST:\t%s\tCASE NO\t%d\tRESULT:\t%s\n", axon_test_info.test_name, axon_test_info.case_no, "PASS");
          axon_test_info.case_count_pass++;
        } else {
          nrf_axon_platform_printf("\n\rTEST:\t%s\tCASE NO\t%d\tRESULT:\t%s\n", axon_test_info.test_name, axon_test_info.case_no, "FAIL");
        }
        axon_test_info.case_no++;
        nrf_axon_platform_printf("mfcc calc time:%u\n", axon_mfcc_return_profiling_ticks());
      } else if (sizeof(AXON_MFCC_DATA_TYPE)==4) {
        char name[20];
        snprintf(name, sizeof(name), "frame_%02d", frame_idx);
        nrf_axon_print_int32_vector(name, (int32_t *)output, AXON_MFCC_WIDTH, 1);
        if(0 == nrf_axon_verify_vectors(name, (int32_t *)output, (int32_t *)expected_output+frame_idx*AXON_MFCC_WIDTH, AXON_MFCC_WIDTH, 1)){
          nrf_axon_platform_printf("\n\rTEST:\t%s\tCASE NO\t%d\tRESULT:\t%s\n", axon_test_info.test_name, axon_test_info.case_no, "PASS");
          axon_test_info.case_count_pass++;
        } else {
          nrf_axon_platform_printf("\n\rTEST:\t%s\tCASE NO\t%d\tRESULT:\t%s\n", axon_test_info.test_name, axon_test_info.case_no, "FAIL");
        }
        axon_test_info.case_no++;
        nrf_axon_platform_printf("mfcc calc time:%u\n", axon_mfcc_return_profiling_ticks());
      }
    }
  }

  return result;
}

/*
 *
 */
int axon_mfcc_app_init() {
  return axon_mfcc_calculate_init(
    quant_inverse_scaling_factor,
    quant_inverse_scaling_factor_radix,
    quant_zero_point
  );
}

int axon_mfcc_run_compiled_in_vectors(char* test_name_root) {
  uint32_t audio_sample_ndx;
  for(audio_sample_ndx=0;
      audio_sample_ndx < sizeof(audio_sample_files)/sizeof(audio_sample_files[0]);
      audio_sample_ndx++) {
    nrf_axon_platform_printf("\r\n\r\n");
    nrf_axon_platform_printf("Audio Name: %s\r\n", audio_sample_files[audio_sample_ndx].sample_label);

    strcpy(axon_test_info.test_name, test_name_root);
    strcat(axon_test_info.test_name, audio_sample_files[audio_sample_ndx].sample_label);
    axon_test_info.case_count_total = ((audio_sample_files[audio_sample_ndx].sample_count-AXON_MFCC_FRAME_LEN)/AXON_MFCC_FRAME_SHIFT)+1;
    axon_test_info.case_count_pass = 0;
    axon_test_info.case_no = 0;
    nrf_axon_platform_printf("\n\rTEST:\t%s\tCASE COUNT\t%d\n", axon_test_info.test_name, axon_test_info.case_count_total);
    
    axon_mfcc_process_vector(audio_sample_files[audio_sample_ndx].wave_data, // samples
        audio_sample_files[audio_sample_ndx].sample_count,  // # of samples
        1, // Stride between samples. For mono this is 1, for stereo it's 2
#if AXON_MFCC_QUANT_ENABLE        
        audio_sample_files[audio_sample_ndx].expected_output_quantized,
#else
        audio_sample_files[audio_sample_ndx].expected_output_unquantized,
#endif
        NULL, // don't want the output vector
        NULL  // nor do we want the unquantized output vector
        );   // expected output to compare to (can be null)

    nrf_axon_platform_printf("\n\rTEST:\t%s\tCOMPLETE\tPASS COUNT\t%d\tFAIL COUNT\t%d\n", axon_test_info.test_name, axon_test_info.case_count_pass, axon_test_info.case_count_total-axon_test_info.case_count_pass);
  }

  return 0;
}

/**
 * top level function for testing compiled in vectors.
 * Runs on simulator and in Zephyr
 */
int main_mfcc_test_compiled_in_vectors()
{
  nrf_axon_platform_printf("\n\n\rStart axon_app_mfcc!\n\r");

  nrf_axon_result_e result = nrf_axon_platform_init();

  if (result != NRF_AXON_RESULT_SUCCESS) {
    nrf_axon_platform_printf("\n\axon_platform_init failed!\n");
    return result;
  }

  axon_mfcc_app_init();

  // iterate through the compiled-in test vectors
  axon_mfcc_run_compiled_in_vectors("axon_fe_mfcc_");

  nrf_axon_platform_printf("\r\n axon_app_mfcc complete!\r\n");
  nrf_axon_platform_close();

  return 0;
}
