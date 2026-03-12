#
# Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
#
# The information contained herein is confidential property of Nordic Semiconductor ASA.
# The use, copying, transfer or disclosure of such information is prohibited except by
# express written agreement with Nordic Semiconductor ASA.
#
cmake_minimum_required(VERSION 3.5.0)

# Caller must provide the NRF_AXON_MFCC_NAME in order for this 
# library to include the proper header files. 

message(STATUS ${CMAKE_CURRENT_SOURCE_DIR} " not zephyr? " ${NOT_A_ZEPHYR_BUILD})
message(STATUS "NRF_AXON_MFCC_NAME = " ${NRF_AXON_MFCC_NAME})



if (${NOT_A_ZEPHYR_BUILD})
	set (axons_mfcc_compiler_compile_defs
		NRF_AXON_MFCC_NAME=${NRF_AXON_MFCC_NAME}
	)
	set (axons_mfcc_compile_defs 
		${axons_mfcc_compiler_compile_defs}
		NRF_AXON_MFCC_CALCULATE_ONLY=1
	)

	add_library(nrf-axon-mfcc-interface INTERFACE)
	target_include_directories(nrf-axon-mfcc-interface
		INTERFACE
			include
	)
	
	add_library(nrf-axon-mfcc STATIC)
	target_sources(nrf-axon-mfcc 
		PRIVATE # compiler and calculator both get these source files
			axon_mfcc_calculate.c
	)
	# calculator gets pre-compiled cmd buffer header file
	target_compile_definitions(nrf-axon-mfcc
		PUBLIC
		NRF_AXON_SIMULATOR_BUILD=1
		${axons_mfcc_compile_defs}
	)
	target_link_libraries(nrf-axon-mfcc 
		PUBLIC 
			nrf-axon-mfcc-interface
		PRIVATE
			nrf-axon-driver-interface
	)
	
	if(DEFINED CONFIG_NRF_AXON_MFCC_COMPILER)
		# compiler gets compile source file...
		add_library(nrf-axon-mfcc-compiler STATIC)
		target_compile_definitions(nrf-axon-mfcc-compiler PUBLIC 
			${axons_mfcc_compiler_compile_defs}
			NRF_AXON_SIMULATOR_BUILD=1
		)
		target_sources(nrf-axon-mfcc-compiler
			PRIVATE # compiler and calculator both get these source files
				axon_mfcc_calculate.c
		)
		target_include_directories(nrf-axon-mfcc-compiler
			PRIVATE
				src
		)
		target_link_libraries(nrf-axon-mfcc-compiler
			PUBLIC 
				nrf-axon-mfcc-interface
				nrf-axon-compiler-interface
			PRIVATE
				nrf-axon-driver-interface
		)
	endif() #non-zephyr, compiler lib
else()
	zephyr_sources(
		axon_mfcc_calculate.c
	)

	# NRF_AXON_MFCC_NAME was defined in main app from the .prj file, just need to specify this is a calculate-only build.
	zephyr_compile_definitions(
		NRF_AXON_MFCC_CALCULATE_ONLY=1
	)

endif()

