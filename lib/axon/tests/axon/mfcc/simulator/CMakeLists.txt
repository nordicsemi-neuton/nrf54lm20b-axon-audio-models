#
# Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
#
# The information contained herein is confidential property of Nordic Semiconductor ASA.
# The use, copying, transfer or disclosure of such information is prohibited except by
# express written agreement with Nordic Semiconductor ASA.
#
cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

project(axon_fe_mfcc VERSION 0.1.0 LANGUAGES C)


# This is a simulator build.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(NOT_A_ZEPHYR_BUILD 1)
set(CONFIG_NRF_AXON 1)
set(CONFIG_NRF_AXON_MFCC_COMPILER 1)
set(CONFIG_NRF_AXON_MFCC 1)

set(AXON_ROOT "${CMAKE_SOURCE_DIR}/../../../../axon_simulator")
if (DEFINED NRF_AXON_MFCC_NAME)
	message(STATUS "NRF_AXON_MFCC_NAME DEFINED ${NRF_AXON_MFCC_NAME}")
else()
	set(NRF_AXON_MFCC_NAME 16kfps_30ms_20ms_hann_fftmag_40fb_20to4khz_10coef)  # inform mfcc library what mfcc configuration files to use.
	# set(NRF_AXON_MFCC_NAME 16kfps_32ms_16ms_ham_fftpwr_32fb_0to8khz_10coef)  # inform mfcc library what mfcc configuration files to use.
	message(STATUS "NRF_AXON_MFCC_NAME NOT DEFINED, building model ${NRF_AXON_MFCC_NAME}")
endif()

add_subdirectory(${AXON_ROOT} ..)

set(AXONS_FE_MFCC_BASE_LIBS nrf-axon-nn-compiler-internal nrf-axon-mfcc-interface nrf-axon-platform nrf-axon-simulator-internal nrf-axon-driver-internal nrf-axon-driver)
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	# linux requires explicit link to math lib; windows and zephyr don't.
	set(AXONS_FE_MFCC_PLATFORM_LIBS  m)
endif()


add_executable(axons_ml_app_mfcc_compiler 
		../src/axon_app_mfcc_compiler.c
		../src/axon_app_mfcc.c
)

target_link_libraries(axons_ml_app_mfcc_compiler 
	PRIVATE
		nrf-axon-mfcc-compiler
		${AXONS_FE_MFCC_BASE_LIBS}
		${AXONS_FE_MFCC_PLATFORM_LIBS}
)
target_compile_definitions(axons_ml_app_mfcc_compiler
	PRIVATE  
		NOT_A_ZEPHYR_BUILD=1
)

add_executable(axons_ml_app_mfcc
		../src/axon_app_mfcc_simulator.c
		../src/axon_app_mfcc.c
)

target_link_libraries(axons_ml_app_mfcc
	PRIVATE
		nrf-axon-mfcc
		${AXONS_FE_MFCC_BASE_LIBS}
		${AXONS_FE_MFCC_PLATFORM_LIBS}
)
target_compile_definitions(axons_ml_app_mfcc
	PRIVATE  
		NOT_A_ZEPHYR_BUILD=1
)

add_executable(axons_ml_app_mfcc_generate_vectors
		../src/axon_app_mfcc_generate_vectors.c
)

target_link_libraries(axons_ml_app_mfcc_generate_vectors
	PRIVATE
		nrf-axon-mfcc
		${AXONS_FE_MFCC_BASE_LIBS}
		${AXONS_FE_MFCC_PLATFORM_LIBS}
)
target_compile_definitions(axons_ml_app_mfcc_generate_vectors
	PRIVATE  
		NOT_A_ZEPHYR_BUILD=1
)

add_library(axon_ml_shared_lib_mfcc SHARED
	../src/axon_shared_lib_mfcc.c
	../src/axon_app_mfcc.c
)

target_link_libraries(axon_ml_shared_lib_mfcc
		PRIVATE
		nrf-axon-mfcc
		${AXONS_FE_MFCC_BASE_LIBS}
		${AXONS_FE_MFCC_PLATFORM_LIBS}
)
target_compile_definitions(axon_ml_shared_lib_mfcc
	PRIVATE  
		NOT_A_ZEPHYR_BUILD=1
)

