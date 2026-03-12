/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "axon/mfcc/axon_mfcc.h"
#include "axon/nrf_axon_platform.h"
#if (NOT_A_ZEPHYR_BUILD)
#include "axon/nrf_axon_platform_simulator.h"
#endif

static FILE* compiler_output_file = NULL;

extern int axon_mfcc_app_init();
extern int axon_mfcc_run_compiled_in_vectors(char*);
void axon_compiler_set_intrinsic_compiler_file(FILE*);

int main()
{
  nrf_axon_platform_printf("\n\n\rStart axon_app_mfcc_compiler!\r\n");

  // open the compiled command buffer file.
  // note that this flie name has a relative path in it for when reading the file. Need to strip that off before writing it.
  const char *name_only = AXON_MFCC_CMD_BUF_FILE_NAME;
  unsigned name_len = strlen(name_only);
  name_only += name_len;
  for(unsigned ndx=0; ndx < name_len; ndx++,name_only--) {
    if (*name_only == '/') {
      name_only++;
      break;
    }
  }
  if (0 != fopen_s(&compiler_output_file, name_only, "w+")) {
    printf("ERROR: unable to create file: %s\n", name_only);
    return -3;
  }

  // pass this file to the compiler
  axon_compiler_set_intrinsic_compiler_file(compiler_output_file);

  extern bool simulator_in_threadless_mode;
  simulator_in_threadless_mode = true;
  nrf_axon_result_e result = nrf_axon_platform_init();

  if (result != NRF_AXON_RESULT_SUCCESS) {
    nrf_axon_platform_printf("\nInitInstance failed!\n");
  }

  axon_mfcc_app_init();

  // iterate through the compiled-in 
  axon_mfcc_run_compiled_in_vectors("axon_fe_mfcc_compiler_");

  /**
   * @FIXME!!! NEED TO READ-IN TEST VECTORS FROM FILE!!
  */
  fclose(compiler_output_file);

  nrf_axon_platform_printf("\r\n axon_app_mfcc_compiler complete!\r\n");
  nrf_axon_platform_close();

  
  return 0;
}
