#
# Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
#
# The information contained herein is confidential property of Nordic Semiconductor ASA.
# The use, copying, transfer or disclosure of such information is prohibited except by
# express written agreement with Nordic Semiconductor ASA.
#
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(axon_fe_mfcc VERSION 0.1.0 LANGUAGES C)

target_sources(app PRIVATE
	src/axon_app_mfcc_zephyr.c
	src/axon_app_mfcc.c
)

# 
# mfcc name is specified through kConfig. However for automated builds the
# environment variable AXON_MFCC_NAME can override kConfig.
#
if (DEFINED ENV{NRF_AXON_MFCC_NAME})
    message(STATUS "NRF_AXON_MFCC_NAME DEFINED $ENV{NRF_AXON_MFCC_NAME}, OVERRIDES CONFIG_NRF_AXON_MFCC_NAME ${CONFIG_NRF_AXON_MFCC_NAME}")
    zephyr_compile_definitions(
        NRF_AXON_MFCC_NAME=$ENV{NRF_AXON_MFCC_NAME}
    )
else()
    message(STATUS "NRF_AXON_MFCC_NAME NOT DEFINED, USING CONFIG_NRF_AXON_MFCC_NAME ${CONFIG_NRF_AXON_MFCC_NAME}")
    zephyr_compile_definitions(
        NRF_AXON_MFCC_NAME=${CONFIG_NRF_AXON_MFCC_NAME}
    )
endif()
