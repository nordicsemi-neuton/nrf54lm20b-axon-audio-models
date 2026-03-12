Axon MFCC Algorithm
###################

.. contents::
   :local:
   :depth: 2

Axon MFCC provides an axon-accelerated implementation of MFCCs. It also acts as a template for developing algorithms to run on axon.
This readme will describe how to use MFCCs as-is, and separately, how algorithm development can occur using MFCC as an example.

Usage
*****
There are 4 phases to using this implementation of MFCC; gen-constants, develop, compile, and deploy.

gen-constants (to be supported in a later release)
=============
gen-constants is a script that takes the MFCC parameters and generates the constants used to calculate the mfccs.
The user provides the following parameters via an input .yaml file: 

* a name to identify the mfccs (often times it is the name of the model being fed with the mfccs)
* audio sample rate
* window size and slide
* window function type (ie, hamming or hann)
* low and high limits of mel frequency
* Whether to feed FFT power or FFT magnitude into the filter banks.
* number of filter banks
* number of final coefficents

gen-constants uses these parameters to generate the window, filterbank, and DCT coefficents.
It will also determine the values for certain internal settings used by the algorithm (rounding values, 
log offset values, etc).

The output of gen-constants is 2 header files:
- axon_mfcc_properties_<name>.h -> public properties to be shared with the rest of the system. It is included in axon_mfcc.h
- axon_mfcc_const_<name>.h -> private constants to be consumed by mfcc only.

These files are fed to the compile and calculate components.

develop
=======
In develop phase, the intrinsic functions are used to calculate the output (compiling the intrinsics into a command buffer is a performance optimization that occurs in a later phase).
Users can step through code, examine intermediate results, and adjust rounding levels as needed. (Note that any rounding level changes should be back-fed into the config.yaml file of the gen-constants script.)
In this phase, the macro AXON_FE_MODE in `<axon_mfcc_calculate.c>`_ should be set to AXON_FE_MODE_DEVELOP.
ie,

.. code-block:: c

    #if !AXON_MFCC_CALCULATE_ONLY
    # define AXON_FE_MODE AXON_FE_MODE_COMPILER
    #else
    // choose this to develop the algorithm by running intrinsics
    # define AXON_FE_MODE AXON_FE_MODE_DEVELOP

    // choose this to use the compiled command buffer
    // # define AXON_FE_MODE AXON_FE_MODE_COMPILED
    #endif

The application axons_ml_app_mfcc in `tests/axon/mfcc <../../../test/axon/mfcc>`_ provides a framework for developing the algorithm.
Note that this application can be targeted to zephyr builds to profile performance.

The MFCC algorithm can be further evaluated by using its outputs to train and/or test the neural net model that will be consuming the MFCCs.

The application axons_ml_app_mfcc_generate_vectors in `samples/axon_fe_mfcc <../../samples/axon_fe_mfcc>`_ can be used to calculate the MFCCs on test vectors supplied in .csv files.


compile
=======
The compile process translates the intrinsic function calls into axon machine code. This optimizes performance on-target because the number of cpu/axon interactions is reduced.
The application axons_ml_app_mfcc_compiler in `samples/axon_fe_mfcc <../../samples/axon_fe_mfcc>`_ performs the mfcc compilation.

The output of the compile process is 1 header file:

.. code-block:: console

    axon_mfcc_cmd_buf_<mfcc_name>

where <mfcc_name> is specified by build variable AXON_MFCC_NAME.

This file is produced in the execution folder of the application, and must be copied to `<include>`_ in order be used in a compiled-mode build.

To run the compiled version, change the macro AXON_FE_MODE in `<src/axon_mfcc_calculte.c>`_ to AXON_FE_MODE_COMPILED, ie,

.. code-block:: c

    #if !AXON_MFCC_CALCULATE_ONLY
    # define AXON_FE_MODE AXON_FE_MODE_COMPILER
    #else
    // choose this to develop the algorithm by running intrinsics
    // # define AXON_FE_MODE AXON_FE_MODE_DEVELOP

    // choose this to use the compiled command buffer
    # define AXON_FE_MODE AXON_FE_MODE_COMPILED
    #endif

Re-compile axons_ml_app_mfcc, and run.
Note that this version can be run on-target and the performance can be compared to the non-compiled version's performance.

Deploying the MFCC algorithm
=======================
The sample `samples/kws_inference <../../samples/kws_inference>`_ shows mfccs integrated into a keyword spotting neural net model.
It can be targeted for the simulator and zephyr.

