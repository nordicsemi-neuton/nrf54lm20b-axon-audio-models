/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "drivers/axon/nrf_axon_driver.h"
#include "drivers/axon/nrf_axon_nn_infer.h"
#include "nrf_axon_nn_compiler.h"
#include "axon/nrf_axon_stringization.h"
#include "axon/nrf_axon_platform.h"

/*
* Create the model include header file name and structure from 
* the model name.
*/
#define AXON_MODEL_FILE_NAMES_ROOT nrf_axon_model_
#define AXON_MODEL_TEST_VECTORS_OUTPUT_FILE_NAME_TAIL _test_vectors_.h
#define AXON_MODEL_BIN_FILE_NAME_TAIL_DOT_BIN _bin_.bin
#define AXON_MODEL_CSV_TEST_VECTORS_FILE_NAME_TAIL_DOT_CSV _test_vectors_.csv
#define AXON_MODEL_TEST_INFERENCE_LABELS_FILE_NAME_TAIL _test_inference_labels_.csv
#define AXON_MODEL_TEST_INFERENCE_RESULTS_FILE_NAME_TAIL_DOT_CSV _test_inference_results_.csv
#define AXON_MODEL_LAYERS_FILE_NAME_TAIL _layers
#define AXON_MODEL_HDR_FILE_NAME_TAIL 

#define AXON_MODEL_HDR_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_HDR_FILE_NAME_TAIL)
#define AXON_MODEL_LAYERS_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_LAYERS_FILE_NAME_TAIL)
#define AXON_MODEL_TEST_INFERENCE_LABELS_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_TEST_INFERENCE_LABELS_FILE_NAME_TAIL)
#define AXON_MODEL_TEST_INFERENCE_RESULTS_CSV_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_TEST_INFERENCE_RESULTS_FILE_NAME_TAIL_DOT_CSV)
#define AXON_MODEL_DESC_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_BIN_FILE_NAME_TAIL_DOT_BIN)
#define AXON_MODEL_TEST_VECTORS_CSV_FILE_NAME STRINGIZE_2_CONCAT(AXON_MODEL_NAME, AXON_MODEL_CSV_TEST_VECTORS_FILE_NAME_TAIL_DOT_CSV)
#define AXON_MODEL_TEST_VECTORS_OUTPUT_FILE_NAME STRINGIZE_3_CONCAT(AXON_MODEL_FILE_NAMES_ROOT, AXON_MODEL_NAME, AXON_MODEL_TEST_VECTORS_OUTPUT_FILE_NAME_TAIL)

#if defined(_MSC_FULL_VER) && (_MSC_FULL_VER < 195035721)
// Old versions of MSVC places executable in build/<variant> folder
# define MODEL_DESC_BINS_RELATIVE_PATH "../../../model_bins/"
#else
// gcc and newer versions of MSVC places executable in build folder
# define MODEL_DESC_BINS_RELATIVE_PATH "../../model_bins/"
#endif
static void print_usage() {

  
  printf("axon nn compiler\n");
#ifdef _MSC_FULL_VER 
  /**
   * Visual Studio Build tools 2022 (17.8.3):193833133
   * visual studio build tools 2026, (18.1.1): 195035721
   */
  printf("built with MSVC ver %d\n", _MSC_FULL_VER);
#endif
}
/**
 * The initial file names are used when the compiler is compiled as a stand-alone application.
 */
static nrf_axon_nn_compiler_io_files_s compiler_io_files = {
  .input_model_desc_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_DESC_FILE_NAME,
  .input_test_vectors_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_TEST_VECTORS_CSV_FILE_NAME,
  .output_model_full_header_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_HDR_FILE_NAME,
  .output_model_partial_header_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_LAYERS_FILE_NAME,  
  .output_test_vectors_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_TEST_VECTORS_OUTPUT_FILE_NAME,
  .output_sim_env_file_name = MODEL_DESC_BINS_RELATIVE_PATH "sim_env",
  .output_labels_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_TEST_INFERENCE_LABELS_FILE_NAME,
  // .output_results_file_name = MODEL_DESC_BINS_RELATIVE_PATH AXON_MODEL_TEST_INFERENCE_RESULTS_CSV_FILE_NAME,
};

static void parse_parameters(int argc, char* argv[]) {
  if (argc <= 1) {
    return;
  } 
    
  memset(&compiler_io_files,0, sizeof(compiler_io_files));
  // advance past the 1st arg (exe file name)
  argv++;
  while (--argc) {
    char* this_string = *argv++;
    if (0 == memcmp(this_string, "-b", 2 * sizeof(char))) {
      compiler_io_files.input_model_desc_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-v", 2 * sizeof(char))) {
      compiler_io_files.input_test_vectors_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-t", 2 * sizeof(char))) {
      compiler_io_files.output_test_vectors_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-l", 2 * sizeof(char))) {
      compiler_io_files.output_labels_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-r", 2 * sizeof(char))) {
      compiler_io_files.output_results_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-f", 2 * sizeof(char))) {
      compiler_io_files.output_model_full_header_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-p", 2 * sizeof(char))) {
      compiler_io_files.output_model_partial_header_file_name = this_string + 2;
    } else if (0 == memcmp(this_string, "-s", 2 * sizeof(char))) {
      compiler_io_files.output_sim_env_file_name = this_string + 2;
    } else {
      printf("unused parameter <%s>\n", this_string);
    }
  }

}
static int compile_only(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct)
{
  parse_parameters(argc, argv);
/**
 * top-level function to read in the axon-intermediate-representation file and compile the primary model header file.
 * Has no dependencies.
 */
  return nrf_axon_nn_compile_full_model(&compiler_io_files, compiler_return_struct);
}

static int test_infer_after_compile(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct)
{
  parse_parameters(argc, argv);
  /**
   * top-level function to perform test inference on the test vectors for the model compiled by nrf_axon_nn_compile_full_model().
   * Generates layer model file as well.
   * Requires nrf_axon_nn_compile_full_model() be invoked prior to invoking it.
   */
  return nrf_axon_nn_compiler_test_inference(&compiler_io_files, compiler_return_struct);
}

static int compile_and_test_infer(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct)
{
  parse_parameters(argc, argv);

/**
 * top-level function the calls the other 2 in succession, "legacy mode."
 */
  return nrf_axon_compile_and_infer(&compiler_io_files, compiler_return_struct);
}

EXPORT_API int CompilerLibMain(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct){
  return compile_and_test_infer(argc,argv,compiler_return_struct);
}

EXPORT_API int nrf_axon_compile_model(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct){
  return compile_only(argc,argv,compiler_return_struct);
}

EXPORT_API int nrf_axon_infer_test_vectors(int argc, char* argv[], nrf_axon_nn_compiler_return_s* compiler_return_struct){
  return test_infer_after_compile(argc,argv,compiler_return_struct);
}

int main(int argc, char* argv[]){
  return compile_and_test_infer(argc,argv,NULL);
}