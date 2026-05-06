/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "drivers/axon/nrf_axon_driver.h"
#include "nrf_axon_nn_compiler_types.h"
#include <stdio.h>

/**
 * Size of the version field in the intermediate respresentation .bin file
 */
#define NRF_AXON_INTERMEDIATE_REPRESENTATION_VERSION_SIZE (4)
#define NRF_AXON_INTERMEDIATE_REPRESENTATION_VERSION (0x00010200) //4 byte version of MAJOR.MINOR.PATCH format

/**
 * helper macro for determining the stride between rows (ie, number of bytes between the start of each row).
 * "width" is the number of elements in the row.
 * "bytewidth" is the size in bytes of each element (1, 2, or 4)
 * "input_is_packed" is true if the start of each row is not on a 4byte boundary, false if it is.
 */
#define NRF_AXON_NN_STRIDE_CEIL(width_in_bytes) ((4)*(((width_in_bytes)+(4)-1)/(4)))
#define NRF_AXON_NN_STRIDE_WIDTH_IN_BYTES(width, bytewidth, data_is_packed) ((uint16_t)(data_is_packed ? (width * bytewidth) : NRF_AXON_NN_STRIDE_CEIL((width)*(bytewidth))))

/**
 * @brief Axon NN compiler symbol descriptor structure
 * 
 * Any symbols that are used in the axon commands need to be presented in a list to the compiler
 * so that it can resolve symbol addresses to symbol names. These symbol names will appear in the 
 * compiled command buffers and will be resolved by the linker when the command buffers are linked into an application.
 */
typedef struct {
  const void *addr;         ///< @location of the symbol in the current build.
  const char *name;   ///< @name of the symbol. verbatim name that is passed to the axon function (including @, ., ->) without any additional symbols. For arrays, the base address is sufficient. Eg @my_array[5] can be passed as my_array. @my_array[my_index] is not allowed.
  int element_size;   ///< @size of the symbol in bytes. Used to calculate an offset from an array base.
  int length;         ///< @number of elements in the array ()
} nrf_axon_compiler_symbol_desc_s;

/**
 * @brief Declares a nrf_axon_compiler_symbol_desc_s instance for an array-type variable
 * 
 * Symbol is declared "naked". This name (with an offset) will appear in the compiled command buffer.
 */
#define NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_ARRAY_ELEMENT(symbol_name) \
{.addr = symbol_name, .name = STRINGIZE(symbol_name), .element_size = sizeof(*symbol_name), .length=sizeof(symbol_name)/sizeof(*symbol_name)}

/**
 * @brief Declares a nrf_axon_compiler_symbol_desc_s instance for a non-pointer variable
 * 
 * Symbol is declared with an ampersand. This name preceded with an ampersand will appear in the compiled command buffer.
 */
#define NRF_AXON_COMPILER_SYMBOL_INFO_DECLARE_VAR_ELEMENT(symbol_name) \
{.addr = &symbol_name, .name = "&" STRINGIZE(symbol_name), .element_size = sizeof(symbol_name), .length=1}

/**
 * @brief Various file names passed to the nn compiler.
 * 
 * This struct is used internally by the compiler. The file names are passed via a command line parameter list,
 * then placed into an instance of this struct.
 */
typedef struct {
  char* input_model_desc_file_name;             /**< Name of the input intermediate results bin file produced by the executor*/
  char* input_test_vectors_file_name;           /**< Name of the input test vectors file. */
  char* output_labels_file_name;                /**< Name of output labels; applicable for classification models. Contains just the highest scored label. */
  char* output_results_file_name;               /**< Name of the output results file. Contains the entire output layer contents. */
  char* output_model_full_header_file_name;     /**< Name of the compiled, full-model output header file. */
  char* output_model_partial_header_file_name;  /**< Name of the compiled, individual layer models output header file. */
  char* output_test_vectors_file_name;          /**< Name of the output test vectors header file. */
  char* output_sim_env_file_name;               /**< not used. */
} nrf_axon_nn_compiler_io_files_s;

/**
 * @brief top level function in pre-compiled library for compiling NN models for Axon.
 * 
 * @param[in] axonpro_compiler_io_files_ptr Listing of the files the compiler will access.
 * @param[out] compiler_return_struct Populated with the results of the compilation.
 * @retval result code, nrf_axon_compiler_result_e
 */
/**
 * top-level function to read in the axon-intermediate-representation file and compile the primary model header file.
 * Has no dependencies.
 */
int nrf_axon_nn_compile_full_model(nrf_axon_nn_compiler_io_files_s* axonpro_compiler_io_files_ptr, nrf_axon_nn_compiler_return_s* compiler_return_struct);
/**
 * top-level function to perform test inference on the test vectors for the model compiled by nrf_axon_nn_compile_full_model().
 * Generates layer model file as well.
 * Requires nrf_axon_nn_compile_full_model() be invoked prior to invoking it.
 */
int nrf_axon_nn_compiler_test_inference(nrf_axon_nn_compiler_io_files_s* axonpro_compiler_io_files_ptr, nrf_axon_nn_compiler_return_s* compiler_return_struct);

/**
 * top-level function the calls the other 2 in succession, "legacy mode."
 */
int nrf_axon_compile_and_infer(nrf_axon_nn_compiler_io_files_s* axonpro_compiler_io_files_ptr, nrf_axon_nn_compiler_return_s* compiler_return_struct);


#ifdef __cplusplus
} // extern "C" {
#endif
