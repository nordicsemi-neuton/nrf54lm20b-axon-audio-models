
.. _axons_ml_release:

Axon NPU Release Notes
######################

.. contents::
   :local:
   :depth: 2

Release 1.0.1  05 Mar 2026
******************************

* Compiler release 1.0.1
* Manifest (west.yml) pulls v3.3.0-preview2 tag of sdk-nrf (SHA-1 ede152ec21).
* Tested and built with SDK toolchain version v3.2.0. Users are recommended to use this version as well.

Bug fixes
------------
* Properly handles fully-connected followed by softmax when weights quantization of the fully connected layer is per channel (TensorFlow v2.19)

Release 1.0.0  03 Mar 2026
******************************

* Compiler release 1.0.0
* First public release.
* Manifest (west.yml) pulls v3.3.0-preview2 tag of sdk-nrf (SHA-1 ede152ec21).
* Tested and built with SDK toolchain version v3.2.0. Users are recommended to use this version as well.

New features
------------
* Licenses updated to public Nordic license SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
* Arbitrary reshape operation supported.

Bug fixes
------------
* Softmax no longer specifies unpacked output. Affects a small number of models.

Release 0.7.0  17 Feb 2026
******************************

* Compiler release 0.2.0
* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 860c808db9. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.2.0. Users are recommended to use this version as well.

New features
------------
* Source tree modified to match sdk-edge-ai.
* Performance improvement in compiler/simulator when performing inference.

Known Issues
------------
* Not supported: ADD operation with broadcast on both height and width axes, and both inputs are dynamic (they are outputs from previous layers).
* Not supported: Per layer testing in unit tests as it has compilation errors due to incompatible types.

Bug fixes
------------
* Compiler checks for dilation on depthwise convolution and returns and error.

Release 0.6.0  06 Feb 2026
******************************

* Compiler release 0.1.4
* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 05c48a21fd. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.2.0. Users are recommended to use this version as well.

New features
------------
* Cleaned up comments in header files.
* Removed unused fields in single layer models.
* Add operator supports constant input and broadcasting.
* Multiply operator supports constant input (already supported broadcasting).

Bug fixes
------------
* 


Release 0.5.1  22 Dec 2025
******************************

* Compiler release 0.1.2
* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 2c5734bfce. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.2.0. Users are recommended to use this version as well.

New features
------------
* Improvement to 1x1 pointwise convolution output compiler optimizations. No longer limited by filter height.
* Average pool operation where the output on an axis is size 1, but the filter size is smaller than the input size on that axis uses "mean" operator and no longer has filter size limitation of 32.

Bug fixes
------------
* Compiler handles input to fully connected where channels dimension is explicitly stated.
* Fixed issue with SplitV operation when compiled on Linux.
* Performance estimates were not printed properly when compiling a model.

Release 0.5.0  12 Dec 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 c44088316d. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.1.0. Users are recommended to use this version as well.

New features
------------
* Renaming of files and APIs to conform with Nordic standards.
* base_inference test application renamed to test_nn_inference.
* Version numbering added to Axon NN Compiler: 0.1.0.
* Handling of input to and output from model inference simplified and made thread safe.
* Compiled models now declare a dedicated buffer for the packed output.
* Power management of Axon is automatic.
* Multiply and Broadcast Multiply operator support.
* Mean/Global Average pool dimension limits increased to 1024x1024 from 32x32.


Release 0.4.0  30 Oct 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 5040028849. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.1.0. Users are recommended to use this version as well.

New features
------------
* New API function 

.. code-block:: shell

  int16_t axon_nn_get_classification(const axon_nn_model_inference_wrapper_struct* model_wrapper, const int8_t *packed_output, const char** label, int32_t* score, uint32_t* profiling_ticks);

* Updated API function adds output_buffer parameter to safely copy the output from the interlayer buffer before the next async inference begins.

.. code-block:: shell

  AxonResultEnum axon_nn_model_infer_async(void *axon_handle, 
    axon_nn_model_inference_wrapper_struct* model_wrapper,
    const int8_t* input_vector, 
    uint32_t input_vector_length,
    int8_t *output_buffer,
    void (*inference_callback)(AxonResultEnum result, void* callback_context),
    void* callback_context);

* Separate model scan script `<compiler/scripts/axons_tflite_model_scan.py>`_
* base_inference() test application alternates between synchronous and asynchronous inference. (Used to be only synchronous.)

Bug fixes
------------
* Inference performance estimates were not properly displayed in nn compiler.

Patch Release 0.3.1  26 Sept 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 c8d23380c0. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.1.0. Users are recommended to use this version as well.

Fixes
------------
* Supports models where the last layer is not the output layer.
* Transposed option verified on stream style models.
* Fully connected layers with widths > 1024 and whose input is reshaped from hx1 to 1xw.

Release 0.3.0  19 Sept 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 101efdd10216. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.1.0. Users are recommended to use this version as well.

New features
------------
* NN Compiler support for sigmoid, Tanh, and SplitV 
* Maximum filter width for 1D convolutions increased from 16 to 32.
* Simulator performance model is now accurate to ~ +/- 20%
* Various bug fixes.
* Build support for board nRF54LM20PDK is deprecated. New build command:

.. code-block:: shell

  west build --build-dir build_zephyr . --board nrf54lm20dk/nrf54lm20a/cpuapp --no-sysbuild -DNCS_TOOLCHAIN_VERSION=NONE -DBOARD_ROOT=.

Patch Release 0.2.1  11 July 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Fixes NN compiler on Docker on MacOS on M4. 

  * Release includes a compiler library targeted for arm64/aarch64
  * Docker runs native code rather than emulation.

Release 0.2.0  30 June 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Manifest (west.yml) pulls main branch of sdk-nrf as it is a work in progress. Tested with SHA-1 72aaada80055d5d0f8ed1de76bd243b204122fba. Users can switch to this commit of sdk-ncs if necessary.
* Tested and built with SDK toolchain version v3.0.2. Users are recommended to use this version as well.

New features
------------
* MacOS support added.
* FPU enabled pre-compiled libraries.
* Pre-compiled libraries can be linked into C++ applications on zephyr. Simulator builds still have issues to be resolved.
* NN Compiler supports
  
  * strided-slice operator
  * concatenate operator
  * stateful models (that use VarHandle, ReadVariable, AssignVariable to manage the state variables)

* Build support for board nRF54LM20DK. Suggested build command:

.. code-block:: shell

  west build --build-dir build_zephyr . --board nrf54lm20pdk@0.2.0.csp/nrf54lm20a/cpuapp --no-sysbuild -DNCS_TOOLCHAIN_VERSION=NONE -DBOARD_ROOT=.

Known Issues
------------
* Neural-net compiler script does not work in docker on M4 MacOS. (resolved in version 0.2.1)

Release 0.1.0  05 March 2025
******************************

* Private release for internal usage and select partners/customers.
* Future public source code included.
* Linux and Windows platforms only (MacOS and Zephyr support to follow).

New features
------------
* Algorithm development in Axon simulator (MFCCs provided as an example).
* NN Compiler operator extensions (Softmax provided as an example).
* User build NN Base Inference simulator applications.
* Combining algorithms for feature extraction with NN inference (kws_inference provided as an example).

Release 0.0.1  24 October 2024
******************************
* Private release for internal usage and select partners/customers.
* Linux and Windows platforms only (MacOS and Zephyr support to follow).
* Supports prebuilt NN compiler only on Linux and Windows platforms.
* No support for algorithm development using the axon simulator and User NN compiler operation extensions.

