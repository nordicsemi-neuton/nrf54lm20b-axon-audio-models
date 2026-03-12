/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

/**
 * Top level file for generating test vectors.
 * For simulator builds only.
 * Reads in a series of raw audio vectors from an csv files, and writes the corresponding mfccs to an output csv file.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "axon/nrf_axon_platform.h"
#include "axon/mfcc/axon_mfcc.h"
#if (NOT_A_ZEPHYR_BUILD)
#include "axon/nrf_axon_platform_simulator.h"
#endif

const int32_t quant_inverse_scaling_factor = 1;
const int8_t quant_inverse_scaling_factor_radix = 0;
const int8_t quant_zero_point = 0;

static int write_results_to_file(FILE* output_file, int32_t *mfccs, uint32_t mfcc_slice_cnt) {
  uint32_t i;
  for (i=0; i<AXON_MFCC_WIDTH*mfcc_slice_cnt-1; i++) {
    fprintf(output_file, "%d,", *(mfccs+i));
  }
  fprintf(output_file, "%d\n", *(mfccs+i));

  return 0;
}

/*
 * Processes a single audio vector, and optionally compares the results to the passed expected_output.
 * 
 */
static int axon_mfcc_process_input_vector(const int16_t *audio_vector, 
              uint32_t audio_vector_len, 
              uint8_t input_stride, 
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
  }

  return result;
}

static int process_mfcc_input_file(char* input_file_name, char* output_file_name, int8_t* buffer, uint32_t buffer_size) {
  FILE* input_vectors_file = NULL;
  FILE* output_vectors_file = NULL;
  int16_t *input_audio_vector = (int16_t *)buffer;
  uint32_t input_buffer_length = buffer_size/sizeof(*input_audio_vector);
  uint32_t input_vector_length;
  #define MFCC_BUFFER_LENGTH (AXON_MFCC_SLICE_CNT * AXON_MFCC_WIDTH * 2)
  static int32_t unquantized_mfccs[MFCC_BUFFER_LENGTH];
  

  if (0 != fopen_s(&input_vectors_file, input_file_name, "r")) {
    nrf_axon_platform_printf("ERROR: unable to read file: %s\n", input_file_name);
    return -2;
  }

  if (0 != fopen_s(&output_vectors_file, output_file_name, "w")) {
    nrf_axon_platform_printf("ERROR: unable to create file: %s\n", output_file_name);
    return -3;
  }

  while (!feof(input_vectors_file)) {
    input_vector_length = read_in_test_vector_int16(input_vectors_file, input_audio_vector, input_buffer_length);
    if (0 >= input_vector_length) {
      break;
    }

  unsigned slices_in_audio_vector = ((input_vector_length-AXON_MFCC_FRAME_LEN)/AXON_MFCC_FRAME_SHIFT)+1;
  // Only process the ones we can handle in the mfcc buffer
  if (slices_in_audio_vector > (AXON_MFCC_SLICE_CNT*2)) {
    slices_in_audio_vector = (AXON_MFCC_SLICE_CNT*2);
  }

  axon_mfcc_process_input_vector(input_audio_vector, // samples
        input_vector_length,  // # of samples
        1, // Stride between samples. For mono this is 1, for stereo it's 2
        NULL, // don't wan't quantized results
        unquantized_mfccs // get the quantized mfccs
        );   // expected output to compare to (can be null)

    write_results_to_file(output_vectors_file, unquantized_mfccs, slices_in_audio_vector);
  }

  fclose(input_vectors_file);
  fclose(output_vectors_file);

  return 0;
}

static int mfcc_generate_vectors(char* input_path, char* output_path)
{
  /**
  * Size the input buffer to 2x the expected size needed. This is the number of samples * size_of_each_sample_as_text
  * Each audio sample is 16bits which is a maximum of 6characters (including minus sign) + 1 for each comma.
  */
#define INPUT_BUFFER_SIZE ((AXON_MFCC_SLICE_CNT * AXON_MFCC_FRAME_SHIFT + AXON_MFCC_FRAME_LEN) * 7 * 2)

  nrf_axon_platform_printf("\n\n\rStart axon_app_mfcc_generate_vectors!\n\r");

  extern bool simulator_in_threadless_mode;
  simulator_in_threadless_mode = true;
  nrf_axon_result_e result = nrf_axon_platform_init();

  if (result != NRF_AXON_RESULT_SUCCESS) {
    nrf_axon_platform_printf("\n\axon_platform_init failed!\n");
    return result;
  }

  axon_mfcc_calculate_init(
    quant_inverse_scaling_factor,
    quant_inverse_scaling_factor_radix,
    quant_zero_point
  );

  nrf_axon_platform_printf("\n\nAxon MFCC get input files (*.csv) from folder %s ...\n", input_path);
  if(0 == nrf_axon_simulator_run_test_files(input_path, output_path, ".csv", "mfcc_", INPUT_BUFFER_SIZE, process_mfcc_input_file)) {
    nrf_axon_platform_printf("Axon MFCC output result files to folder %s\n", output_path);
  }

  nrf_axon_platform_printf("\r\n axon_app_mfcc complete!\r\n");
  nrf_axon_platform_close();

  
  return 0;
}


int main(int argc, char *argv[]) {
  char *mfcc_in=NULL;
  char *mfcc_out=NULL;
    if(argc >= 3) {
      mfcc_in = argv[1];
      mfcc_out = argv[2];
    } else {
      nrf_axon_platform_printf("\r\n Warning: paths for input files and output files not provided! Cmd format: exe <input_folder> <output_folder>\nUsing ../../mfcc_in ../../mfcc_out\n");
      mfcc_in = "../../mfcc_in";
      mfcc_out = "../../mfcc_out";

      /*
      nrf_axon_platform_printf("\r\n Error: please give pathes for input files and output files respectively! Cmd format: exe <input_folder> <output_folder>\n\n");
      return -1;
      */
    }
  return mfcc_generate_vectors(mfcc_in, mfcc_out);
}
