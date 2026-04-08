#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(dmic.c)
zephyr_library_sources_ifdef(CONFIG_BSP_LEDS leds.c)

zephyr_include_directories(.)
