/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
 */
/**
 * top level file for axon_app_mfcc for simultor builds to run compiled-in vectors.
 */
#include <stddef.h>
#include <stdio.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "axon/nrf_axon_platform.h"

extern int main_mfcc_test_compiled_in_vectors();

int main(int argc, char *argv[]) {
    // Start inference
  return main_mfcc_test_compiled_in_vectors();
}
