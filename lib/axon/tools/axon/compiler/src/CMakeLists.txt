#
# Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
#
# The information contained herein is confidential property of Nordic Semiconductor ASA.
# The use, copying, transfer or disclosure of such information is prohibited except by
# express written agreement with Nordic Semiconductor ASA.
#

cmake_minimum_required(VERSION 3.10.0)
project(nrf_axon_nn_compiler VERSION 0.1.0 LANGUAGES C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CONFIG_NRF_AXON 1)
set(NOT_A_ZEPHYR_BUILD 1)

# /*
# * These are sized to handle worst-case scenarios for the compiler, not
# * for on-target. The generated model file will static assert if the needed size
# * for a buffer is not avaialbe.
# */

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}")
message("CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_COMPILER_ID=${CMAKE_C_COMPILER_ID}")

set(COMPILER_CPU_ARCH "")
if((CMAKE_C_COMPILER_ID STREQUAL "GNU") OR (CMAKE_C_COMPILER_ID STREQUAL "clang") OR (CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang$") )
 	execute_process(
    	COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    	OUTPUT_VARIABLE COMPILER_TRIPLET
    	OUTPUT_STRIP_TRAILING_WHITESPACE
	)	
	string(REPLACE "-" ";" COMPILER_PARTS ${COMPILER_TRIPLET})
	list(GET COMPILER_PARTS 0 COMPILER_CPU_ARCH)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # Run cl.exe and extract "for x64", "for ARM64", etc.
    execute_process(
        COMMAND ${CMAKE_C_COMPILER}
        ERROR_VARIABLE CL_OUTPUT
        ERROR_STRIP_TRAILING_WHITESPACE
    )
	string(REGEX MATCH "for ([^ \r\n]+)" _match "${CL_OUTPUT}")
	if (CMAKE_MATCH_1)
		set(COMPILER_CPU_ARCH ${CMAKE_MATCH_1})
	endif()	
	string(TOLOWER "${COMPILER_CPU_ARCH}" COMPILER_CPU_ARCH)
endif()	
message("Compiler CPU Architecture: ${COMPILER_CPU_ARCH}")	
if((COMPILER_CPU_ARCH STREQUAL "x86_64") OR (COMPILER_CPU_ARCH STREQUAL "amd64") OR (COMPILER_CPU_ARCH STREQUAL "x64"))
	set(COMPILER_CPU_ARCH "amd64")
elseif((COMPILER_CPU_ARCH STREQUAL "aarch64") OR (COMPILER_CPU_ARCH STREQUAL "arm64"))
	set(COMPILER_CPU_ARCH "arm64")
else()# elseif((COMPILER_CPU_ARCH STREQUAL "arm") OR (COMPILER_CPU_ARCH STREQUAL "^i[3-9]86$") OR (COMPILER_CPU_ARCH STREQUAL "x86"))
	message(FATAL_ERROR "${COMPILER_CPU_ARCH} is not supported!")
endif()
if (NOT TARGET_CPU_ARCH)
	set(TARGET_CPU_ARCH ${COMPILER_CPU_ARCH})
endif()

if(TARGET_CPU_ARCH)
	# message("TARGET_CPU_ARCH=${TARGET_CPU_ARCH}")
	set(target_cpu_arch_label -${TARGET_CPU_ARCH})
endif()

set(AXONS_ML_ROOT "${CMAKE_SOURCE_DIR}/../../../../axon_simulator")

add_subdirectory(${AXONS_ML_ROOT} ..)

add_executable(nrf-axon-app-nn-compiler${target_cpu_arch_label}
	nrf_axon_app_nn_compiler.c
	nrf_axon_nn_extensions_compiler.c
)
target_link_libraries(nrf-axon-app-nn-compiler${target_cpu_arch_label}
		nrf-axon-nn-compiler-internal		
		nrf-axon-platform		
		nrf-axon-simulator-internal
		nrf-axon-driver
		nrf-axon-driver-internal
		nrf-axon-driver-interface
)

target_include_directories(nrf-axon-app-nn-compiler${target_cpu_arch_label}
	PRIVATE
		include
)
target_compile_definitions(nrf-axon-app-nn-compiler${target_cpu_arch_label} 
	PRIVATE
		# AXON_MODEL_NAME=tinyml_ad
		# AXON_MODEL_NAME=tinyml_kws
		AXON_MODEL_NAME=tinyml_kws_transposed
		# AXON_MODEL_NAME=tinyml_ic
)

add_library(nrf-axon-nn-compiler-lib${target_cpu_arch_label} SHARED 
	nrf_axon_app_nn_compiler.c
	../src/nrf_axon_nn_extensions_compiler.c
)
target_link_libraries(nrf-axon-nn-compiler-lib${target_cpu_arch_label}
	PRIVATE
	nrf-axon-nn-compiler-internal		
	nrf-axon-platform			
	nrf-axon-simulator-internal
	nrf-axon-driver
	nrf-axon-driver-interface
	nrf-axon-driver-internal
)
target_include_directories(nrf-axon-nn-compiler-lib${target_cpu_arch_label}
	PRIVATE
		include
)