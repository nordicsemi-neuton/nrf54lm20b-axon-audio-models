/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "nrf_axon_nn_compiler_types.h"
#include "nrf_axon_nn_compiler.h"
#include "drivers/axon/nrf_axon_nn_infer.h"
#include "nrf_axon_nn_compiler_op_extensions.h"
#include "drivers/axon/nrf_axon_nn_op_extensions.h"
#include "axon/nrf_axon_platform.h"

/**
 * Softmax compiler functions
 */

static void softmax_mem_usage(
  const nrf_axon_nn_model_layer_desc_s *layer,
  unsigned *output_ptr_offset,
  unsigned *output_stride,
  unsigned *scratch_mem_needed_size,
  bool *output_can_overwrite_input
)
{
  *scratch_mem_needed_size = 0;
  *output_ptr_offset = 0; /* output will be on 32bit boundary*/
  // softmax output is packed.
  *output_stride = (uint16_t)NRF_AXON_NN_STRIDE_WIDTH_IN_BYTES(layer->output_dimensions.width, layer->output_dimensions.byte_width, 1);
  // safe if there is only 1 dimension or input size matches output size.
  *output_can_overwrite_input = (layer->output_dimensions.channel_cnt==1) || 
        ((layer->output_dimensions.height==1) && (layer->output_dimensions.width==1)) ||
        (layer->input_dimensions[0].byte_width==layer->output_dimensions.byte_width);
}

static nrf_axon_compiler_result_e softmax_layer_compile(
  const nrf_axon_nn_model_layer_desc_s *layer, /*< information on the layer (dimensions, data widths, etc)*/
  const nrf_axon_nn_model_layer_suppl_s *layer_supplement,
  const int8_t *input1_ptr,
  const int8_t *input2_ptr,
  const int8_t *scratch_mem_ptr,
  const int8_t *output_ptr,
  bool input1_is_fully_connected
) 
{
  // require that input be 32bit (q11.12)
  if (layer->input_dimensions[0].byte_width != 4) {
    printf("ERROR! SOFT MAX EXPECTS 4BYTE WIDTH INPUT, GOT %d INSTEAD\n", layer->input_dimensions[0].byte_width);
    return NRF_AXON_COMPILER_RESULT_INVALID_BYTE_WIDTH;
  }
  nrf_axon_nn_op_extension_base1_args_s args = {0};
  args.ptr_args.input = (int8_t*)input1_ptr;
  args.ptr_args.output = (int8_t*)output_ptr;
  args.remaining_args.output_multiplier = layer->output_multipliers.ptr == NULL ? 1 : *layer->output_multipliers.ptr;
  args.remaining_args.output_rounding = layer->scale_shifts.ptr == NULL ? 0 : *layer->scale_shifts.ptr;
  args.remaining_args.output_bytewidth = layer->output_dimensions.byte_width;
  args.remaining_args.output_zeropoint = layer->output_zero_point;
  /**
   * the output of fully connected needs the width set to 1 and channel count set to width.
   */
  args.remaining_args.height = layer->output_dimensions.height;
  args.remaining_args.width = input1_is_fully_connected ? 1 : layer->output_dimensions.width;
  args.remaining_args.channel_cnt = input1_is_fully_connected ? layer->output_dimensions.width : layer->output_dimensions.channel_cnt;

  // softmax is implemented by the cpu in function nrf_axon_nn_op_extension_softmax
  return nrf_axon_nn_cmd_buff_add_software_op(
                        nrf_axon_nn_op_extension_softmax, /**< address of function to invoke */
                        "nrf_axon_nn_op_extension_softmax", /** < name of function to invoke. This is the symbol name that will be placed in the compiled output. */
                        sizeof (args.ptr_args)/sizeof(void *), /**< number of pointer arguments in ptr_argv */
                        sizeof(args.remaining_args), /**< size in bytes of remaining_args */
                        (NRF_AXON_PLATFORM_BITWIDTH_SIGNED_TYPE *)&args.ptr_args, /**< actual arguments to pass to the software function */
                        &args.remaining_args);        /**< opaque ptr to all other args to the function. */
}
/**
 * end softmax compiler functions
 */

/**
 * sigmoid compiler functions
 */

static void sigmoid_mem_usage(
  const nrf_axon_nn_model_layer_desc_s *layer,
  unsigned *output_ptr_offset,
  unsigned *output_stride,
  unsigned *scratch_mem_needed_size,
  bool *output_can_overwrite_input
)
{
  *output_ptr_offset = 0; /* output will be on 32bit boundary*/
  *output_stride = (uint16_t)NRF_AXON_NN_STRIDE_WIDTH_IN_BYTES(layer->output_dimensions.width, layer->output_dimensions.byte_width, 0);
  // no temp memory needed
  *scratch_mem_needed_size = 0;
  /**
   * 1:1 correlation between inputs. 
   * safe to overwrite input if input is unpacked and input size <= output size.
   */

  *output_can_overwrite_input = (layer->input_ids[0] >= 0) && // negative input id is externally generated and packed. All internal input is unpacked.
        (layer->input_dimensions[0].byte_width<=layer->output_dimensions.byte_width);
}

static nrf_axon_compiler_result_e sigmoid_layer_compile(
  const nrf_axon_nn_model_layer_desc_s *layer, /*< information on the layer (dimensions, data widths, etc)*/
  const nrf_axon_nn_model_layer_suppl_s *layer_supplement,
  const int8_t *input1_ptr,
  const int8_t *input2_ptr,
  const int8_t *scratch_mem_ptr,
  const int8_t *output_ptr,
  bool input1_is_fully_connected
) 
{
  // require that input be 16bit (q3.12)
  if (layer->input_dimensions[0].byte_width != 2) {
    printf("ERROR! SIGMOID EXPECTS 2BYTE WIDTH INPUT, GOT %d INSTEAD\n", layer->input_dimensions[0].byte_width);
    return NRF_AXON_COMPILER_RESULT_INVALID_BYTE_WIDTH;
  }
  nrf_axon_nn_op_extension_base1_args_s args = {0};
  args.ptr_args.input = (int8_t*)input1_ptr;
  args.ptr_args.output = (int8_t*)output_ptr;
  args.remaining_args.output_multiplier = layer->output_multipliers.ptr == NULL ? 1 : *layer->output_multipliers.ptr;
  args.remaining_args.output_rounding = layer->scale_shifts.ptr == NULL ? 0 : *layer->scale_shifts.ptr;
  args.remaining_args.output_bytewidth = layer->output_dimensions.byte_width;
  args.remaining_args.output_zeropoint = layer->output_zero_point;
  
  args.remaining_args.height = layer->output_dimensions.height;
  args.remaining_args.width = layer->output_dimensions.width;
  args.remaining_args.channel_cnt = layer->output_dimensions.channel_cnt;
  args.remaining_args.input_is_packed = (layer_supplement->inputs[0].stride & 4) != 0;

  // sigmoid is implemented by the cpu in function nrf_axon_nn_op_extension_sigmoid
  return nrf_axon_nn_cmd_buff_add_software_op(
                        nrf_axon_nn_op_extension_sigmoid, /**< address of function to invoke */
                        "nrf_axon_nn_op_extension_sigmoid", /** < name of function to invoke. This is the symbol name that will be placed in the compiled output. */
                        sizeof (args.ptr_args)/sizeof(void *), /**< number of pointer arguments in ptr_argv */
                        sizeof(args.remaining_args), /**< size in bytes of remaining_args */
                        (NRF_AXON_PLATFORM_BITWIDTH_SIGNED_TYPE *)&args.ptr_args, /**< actual arguments to pass to the software function */
                        &args.remaining_args);        /**< opaque ptr to all other args to the function. */
}

static void tanh_mem_usage(
  const nrf_axon_nn_model_layer_desc_s *layer,
  unsigned *output_ptr_offset,
  unsigned *output_stride,
  unsigned *scratch_mem_needed_size,
  bool *output_can_overwrite_input
)
{
  *output_ptr_offset = 0; /* output will be on 32bit boundary*/
  *output_stride = (uint16_t)NRF_AXON_NN_STRIDE_WIDTH_IN_BYTES(layer->output_dimensions.width, layer->output_dimensions.byte_width, 0);
  /**
   * 1:1 correlation between inputs. 
   * safe to overwrite input if input is unpacked and input size <= output size.
   */

  *output_can_overwrite_input = (layer->input_ids[0] >= 0) && // negative input id is externally generated and packed. All internal input is unpacked.
        (layer->input_dimensions[0].byte_width<=layer->output_dimensions.byte_width);
  
  *scratch_mem_needed_size = 0; 
}

static nrf_axon_compiler_result_e tanh_layer_compile(
  const nrf_axon_nn_model_layer_desc_s *layer, /*< information on the layer (dimensions, data widths, etc)*/
  const nrf_axon_nn_model_layer_suppl_s *layer_supplement,
  const int8_t *input1_ptr,
  const int8_t *input2_ptr,
  const int8_t *scratch_mem_ptr,
  const int8_t *output_ptr,
  bool input1_is_fully_connected
) 
{
  // require that input be 16bit (q3.12)
  if (layer->input_dimensions[0].byte_width != 2) {
    printf("ERROR! TANH EXPECTS 2BYTE WIDTH INPUT, GOT %d INSTEAD\n", layer->input_dimensions[0].byte_width);
    return NRF_AXON_COMPILER_RESULT_INVALID_BYTE_WIDTH;
  }
  nrf_axon_nn_op_extension_base1_args_s args = {0};
  args.ptr_args.input = (int8_t*)input1_ptr;
  args.ptr_args.output = (int8_t*)output_ptr;
  args.remaining_args.output_multiplier = layer->output_multipliers.ptr == NULL ? 1 : *layer->output_multipliers.ptr;
  args.remaining_args.output_rounding = layer->scale_shifts.ptr == NULL ? 0 : *layer->scale_shifts.ptr;
  args.remaining_args.output_bytewidth = layer->output_dimensions.byte_width;
  args.remaining_args.output_zeropoint = layer->output_zero_point;
  
  args.remaining_args.height = layer->output_dimensions.height;
  args.remaining_args.width = layer->output_dimensions.width;
  args.remaining_args.channel_cnt = layer->output_dimensions.channel_cnt;
  args.remaining_args.input_is_packed = (layer_supplement->inputs[0].stride & 4) != 0;

  // tanh is implemented by the cpu in function nrf_axon_nn_op_extension_tanh
  return nrf_axon_nn_cmd_buff_add_software_op(
                        nrf_axon_nn_op_extension_tanh, /**< address of function to invoke */
                        "nrf_axon_nn_op_extension_tanh", /** < name of function to invoke. This is the symbol name that will be placed in the compiled output. */
                        sizeof (args.ptr_args)/sizeof(void *), /**< number of pointer arguments in ptr_argv */
                        sizeof(args.remaining_args), /**< size in bytes of remaining_args */
                        (NRF_AXON_PLATFORM_BITWIDTH_SIGNED_TYPE *)&args.ptr_args, /**< actual arguments to pass to the software function */
                        &args.remaining_args);        /**< opaque ptr to all other args to the function. */
}
/**
 * end tanh compiler functions
 */

/**
 * reshape compiler functions
 */

static void reshape_mem_usage(
  const nrf_axon_nn_model_layer_desc_s *layer,
  unsigned *output_ptr_offset,
  unsigned *output_stride,
  unsigned *scratch_mem_needed_size,
  bool *output_can_overwrite_input
)
{
  *output_ptr_offset = 0; /* output will be on 32bit boundary*/
  *output_stride = layer->output_dimensions.width * layer->output_dimensions.byte_width; // output will be packed
  *output_can_overwrite_input = false;  
  *scratch_mem_needed_size = 0; 
}

static nrf_axon_compiler_result_e reshape_layer_compile(
  const nrf_axon_nn_model_layer_desc_s *layer, /*< information on the layer (dimensions, data widths, etc)*/
  const nrf_axon_nn_model_layer_suppl_s *layer_supplement,
  const int8_t *input1_ptr,
  const int8_t *input2_ptr,
  const int8_t *scratch_mem_ptr,
  const int8_t *output_ptr,
  bool input1_is_fully_connected
) 
{
  nrf_axon_nn_op_extension_base2_args_s args = {0};

  args.ptr_args.input = (int8_t*)input1_ptr;
  args.ptr_args.output = (int8_t*)output_ptr;

  args.remaining_args.input_height = layer->input_dimensions[0].height;
  args.remaining_args.input_width = layer->input_dimensions[0].width;
  args.remaining_args.input_channel_cnt = layer->input_dimensions[0].channel_cnt;
  
  args.remaining_args.output_height = layer->output_dimensions.height;
  args.remaining_args.output_width = layer->output_dimensions.width;
  args.remaining_args.output_channel_cnt = layer->output_dimensions.channel_cnt;
  // only 8bit output is supported
  if (layer->output_dimensions.byte_width != 1) {
    nrf_axon_platform_printf("ERROR: INVALID RESHAPE OUTPUT BYTEWIDTH %d; ONLY BYTEWIDTH 1 IS SUPPORTED!\n", layer->output_dimensions.byte_width);
    return NRF_AXON_COMPILER_RESULT_INVALID_OUTPUT_SIZE;
  }

  args.remaining_args.input_stride = layer_supplement->inputs[0].stride;
    
  // tanh is implemented by the cpu in function nrf_axon_nn_op_extension_tanh
  return nrf_axon_nn_cmd_buff_add_software_op(
                        nrf_axon_nn_op_extension_reshape, /**< address of function to invoke */
                        "nrf_axon_nn_op_extension_reshape", /** < name of function to invoke. This is the symbol name that will be placed in the compiled output. */
                        sizeof (args.ptr_args)/sizeof(void *), /**< number of pointer arguments in ptr_argv */
                        sizeof(args.remaining_args), /**< size in bytes of remaining_args */
                        (NRF_AXON_PLATFORM_BITWIDTH_SIGNED_TYPE *)&args.ptr_args, /**< actual arguments to pass to the software function */
                        &args.remaining_args); 
}
/**
 * end reshape compiler functions
 */

/*
 * The list of op exension compiler functions. 
 */
static nrf_axon_nn_op_extension_compiler_s axon_nn_op_extension_compilers[] = {
  NRF_AXON_NN_OP_EXTENSIONS_DECLARE_OP_COMPILER_SYMBOL(NRF_AXON_NN_OP_SOFTMAX, "softmax", softmax_mem_usage, softmax_layer_compile),
  NRF_AXON_NN_OP_EXTENSIONS_DECLARE_OP_COMPILER_SYMBOL(NRF_AXON_NN_OP_SIGMOID, "sigmoid", sigmoid_mem_usage, sigmoid_layer_compile),
  NRF_AXON_NN_OP_EXTENSIONS_DECLARE_OP_COMPILER_SYMBOL(NRF_AXON_NN_OP_TANH, "tanh", tanh_mem_usage, tanh_layer_compile),
  NRF_AXON_NN_OP_EXTENSIONS_DECLARE_OP_COMPILER_SYMBOL(NRF_AXON_NN_OP_RESHAPE, "reshape", reshape_mem_usage, reshape_layer_compile),
};

/*
 * Returns the list of op exension compiler functions.
 */
void nrf_axon_nn_compiler_extensions_get_op_compilers(nrf_axon_nn_op_extension_compiler_s **op_compiler_info, int *count) 
{
  *op_compiler_info = axon_nn_op_extension_compilers;
  *count = sizeof(axon_nn_op_extension_compilers) / sizeof(*axon_nn_op_extension_compilers);
}
