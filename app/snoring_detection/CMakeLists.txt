# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Include application-specific Kconfig before finding Zephyr
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(snoring_detection)

# Note: The nrf_edgeai library is linked directly in zephyr/CMakeLists.txt using zephyr_link_libraries(),
# and the axon SDK sources and libraries are added directly to Zephyr via zephyr_sources() in the same CMakeLists.txt.

target_sources(app PRIVATE
    src/main.cpp
    src/nrf_edgeai_generated/nrf_edgeai_user_model.c
)
