/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "nrf_axon_nn_compiler.h"
#include "drivers/axon/nrf_axon_nn_op_extensions.h"

/**
 * @brief supplemental layer operation parameters
 * 
 * This structure that contains caclulated layer information in parallel with the information read in from the bin file
 * in the format nrf_axon_nn_model_layer_desc_s.
 */
typedef struct {
  unsigned output_ptr_offset; /**< offset from the output where the output data starts. Typically 0, but some operations (like concat on the width axis) can cause leading 0s to be inserted in front of each row. */
  unsigned output_stride;     /**< output stride (distance between the start of rows) in bytes. There can be up to 2 padding words. */
  unsigned layer_id;          /**< unique node ID of the layer. */
  struct {
    unsigned ptr_offset;      /**< offset from the input where the input data starts. Typically 0, but some operations (like concat on the width axis) can cause leading 0s to be inserted in front of each row. */
    unsigned stride;          /**< stride (distance between the start of rows) in bytes. There can be up to 2 padding words. */
    int zero_point;           /**< quantization zero point. */
  } inputs[NRF_AXON_NN_MAX_LAYER_INPUTS]; /**< Array of input information. */
  union {
    unsigned fully_connected_partial_sum_offset; /**< only used by fully connected. offset from partial sum pointer that the layer uses to store partial sums between overly wide inputs. If this is not 0, partial_sum[0] pts to packing convolution output. */
  };
} nrf_axon_nn_model_layer_suppl_s;

/**
 * @brief Helper function to determine if the output of a layer is packed (ie, stride=width*byte_width)
 * @param[in] output_dimensions structure with output dimensions, including width and byte_width
 * @param[in] output_stride distance in bytes between the start of each row.
 * @retval True if the output is packed, false if not
 */
inline bool nrf_axon_model_layer_output_is_packed(const nrf_axon_nn_compiler_model_layer_dimensions_s *output_dimensions, unsigned output_stride) {
  return output_dimensions->byte_width * output_dimensions->width == output_stride;
}
/**
 * @brief Function ptr type for reporting certain attributes of the operation.
 * 
 * Each compiler op extension implements this function.
 */
typedef void (*nrf_axon_nn_operator_mem_usage_func)(
  const nrf_axon_nn_model_layer_desc_s *layer,  /**< Describes the layer. */
  unsigned *output_ptr_offset,                  /**< populated with the offset from the output where the real data resides. Typically 0, but concat on the width axis can cause leading 0s to be inserted in front of each row. */
  unsigned *output_stride,                      /**< populated with output stride in bytes. Typically is the output width rounded up to the next multiple of 4. */
  unsigned *scratch_mem_needed_size,            /**< populated with size in bytes of any additional memory required by the layer computation. */
  bool *output_can_overwrite_input              /**< populated with true if the input_ptr and output_ptr can be the same. */
);

/**
 * @brief Function ptr type for compiling the operation into a command buffer.
 * 
 * Each compiler op extension implements this function.
 */
typedef nrf_axon_compiler_result_e (*nrf_axon_nn_layer_compile_func)(
  const nrf_axon_nn_model_layer_desc_s *layer,        /**< information on the layer (dimensions, data widths, etc)*/
  const nrf_axon_nn_model_layer_suppl_s *layer_suppl, /**< supplemental information to the layer. */
  const int8_t *input1_ptr,                           /**< address of the 1st input to the layer. */
  const int8_t *input2_ptr,                           /**< address of the 2nd input to the layer. Null if less than 2 inputs.*/
  const int8_t *scratch_ptr,                          /**< address of scratch memory to the layer (requested via scratchMem_needed_size returned by nrf_axon_nn_operator_mem_usage_func. */
  const int8_t *output_ptr,                           /**< adddress of the output of the layer. */
  bool input1_is_fully_connected                      /**< used for special handling in certain operations. */
);

/**
 * @brief Combines elements of a op extension compiler into one structure.
 */
typedef struct  {
  nrf_axon_nn_op_e the_op;                            /**< Enum uniquely defining this op. */
  const char *name;                                   /**< Name for the op. Printed in log messages but not used in compiled artifacts. */
  nrf_axon_nn_operator_mem_usage_func mem_usage_func; /**< Function that returns special memory attributes for the op. */
  nrf_axon_nn_layer_compile_func layer_compile_func;  /**< Function that compiles the op. */
} nrf_axon_nn_op_extension_compiler_s;

/**
 * @brief Macro to facilitate declaring op compilers.
 */
#define NRF_AXON_NN_OP_EXTENSIONS_DECLARE_OP_COMPILER_SYMBOL(THE_OP, THE_OP_NAME, MEM_USAGE_FUNC, LAYER_COMPILE_FUNC) \
{.the_op = THE_OP, .name=THE_OP_NAME, .mem_usage_func = MEM_USAGE_FUNC, .layer_compile_func=LAYER_COMPILE_FUNC}

/**
 * @brief Returns the list of op extension compilers.
 * @param[out] op_compiler_info populated by the function with a pointer to the list of op extension compilers.
 * @param[out] count populated by the function with the number of elements in op_compiler_info.
 */
void nrf_axon_nn_compiler_extensions_get_op_compilers(nrf_axon_nn_op_extension_compiler_s **op_compiler_info, int *count);

/**
 * @brief Compiles a software function into the command buffer.
 * 
 * @param[in] function_ptr address of function to compile into the command buffer. 
 * @param[in] function_name, name of function pointed to by function_ptr. This is the symbol name that will be placed in the compiled output.
 * @param[in] ptr_argc number of pointer arguments in ptr_argv. These must all at the top of the argument list.
 * @param[in] remaining_arg_size size in bytes of remaining_args
 * @param[in] ptr_args  actual ptr value arguments to pass to the software function
 * @param[in] remaining_args  opaque ptr to all other args to the function.
 */
nrf_axon_compiler_result_e nrf_axon_nn_cmd_buff_add_software_op(
  axon_op_extension_func function_ptr,
  char *function_name,
  uint16_t ptr_argc, 
  uint16_t remaining_arg_size,
  NRF_AXON_PLATFORM_BITWIDTH_SIGNED_TYPE* ptr_args,
  void *remaining_args
);

#ifdef __cplusplus
} // extern "C" {
#endif

