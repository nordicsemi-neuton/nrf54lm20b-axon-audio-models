/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

#include <stddef.h>
#include <stdio.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "axon/nrf_axon_platform.h"
#include "axon/mfcc/axon_mfcc.h"

extern int axon_mfcc_app_init();

EXPORT_API int axon_mfcc_lib_calc_mfcc(const int16_t *audio_in, unsigned audio_in_len, int32_t *mfccs_out, unsigned mfccs_out_len, unsigned *mfccs_out_width){
  static bool inited = false;
  if (!inited) {
    // parameter checking and initialization only occurs on 1st call.
    if (NULL==audio_in) {
      nrf_axon_platform_printf("ERROR! calc_mfcc audio_in is NULL\n");
      return -1;
    }
    if (NULL==mfccs_out) {
      nrf_axon_platform_printf("ERROR! calc_mfcc mfccs_out is NULL\n");
      return -2;
    }
    if (NULL==mfccs_out_width) {
      nrf_axon_platform_printf("ERROR! calc_mfcc out_width is NULL\n");
      return -3;
    }
    if (4 != sizeof(AXON_MFCC_DATA_TYPE)) {
      nrf_axon_platform_printf("ERROR! calc_mfcc mfccs compiled with quantization enabled\n");
      return -4;
    }
    nrf_axon_platform_printf("\nStart axons_shared_lib_mfcc!\n");

    nrf_axon_result_e result = nrf_axon_platform_init();

    if (result != NRF_AXON_RESULT_SUCCESS) {
      nrf_axon_platform_printf("\n\axon_platform_init failed!\n");
      return result;
    }
    axon_mfcc_app_init();
    inited = true;
  }
  *mfccs_out_width = AXON_MFCC_WIDTH;
  int slice_cnt = 0; 
  int32_t unused_output[AXON_MFCC_WIDTH]; // buffer to hold out that might be quantized.
  
  //loop until reach end of audio_in or mfccs_Out
  while ( (audio_in_len >= AXON_MFCC_FRAME_LEN) && 
           (mfccs_out_len >= AXON_MFCC_WIDTH)) {
    nrf_axon_result_e result=axon_mfcc_process_frame(audio_in, AXON_MFCC_FRAME_LEN, NULL, false, 1, unused_output);

    if (result < 0) {
      nrf_axon_platform_printf("ERROR! calc_mfccs failure %d on slice %d\n", result, slice_cnt);
      return result;
    }
    // now get the unquantized output
    axon_mfcc_get_unquantized_outputs(mfccs_out);

    // decrement counts...
    audio_in_len -= AXON_MFCC_FRAME_SHIFT;
    mfccs_out_len -= AXON_MFCC_WIDTH;
    // ...advance pointers
    audio_in += AXON_MFCC_FRAME_SHIFT;
    mfccs_out += AXON_MFCC_WIDTH;
    slice_cnt++;
  }
  return slice_cnt;

}

EXPORT_API void axon_mfcc_lib_close(){
  /**
   * @FIXME!!! PRINT OVERFLOW TOTALS HERE!
   */
  nrf_axon_platform_close();
}