/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * top level file for axon_app_mfcc for zephyr builds.
 * Runs compiled-in vectors.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/printk.h>

#include <zephyr/logging/log.h>

// #define MY_LOG_LEVEL LOG_LEVEL_NONE
#define MY_LOG_LEVEL LOG_LEVEL_DBG

LOG_MODULE_REGISTER(kws, MY_LOG_LEVEL);

extern int main_mfcc_test_compiled_in_vectors();
int main(void)
{
  while (1) {
    printk("axon_fe_mfcc on %s\n", CONFIG_BOARD);

    // Start inference
    main_mfcc_test_compiled_in_vectors();

    k_sleep(K_FOREVER);
  }
  return 0;
}
