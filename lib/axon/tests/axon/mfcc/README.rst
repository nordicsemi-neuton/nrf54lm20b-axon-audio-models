.. _axons_ml_ap_mfcc:

Axon Feature Extractor MFCC applications
########################################

.. contents::
   :local:
   :depth: 2

Overview
********

These applications demonstrate axon algorithm development and deployment for MFCCs (Mel Frequency Cepstrum Coefficients). 
The MFCC algorithm implementation is found under `axon_utilities/mfcc <../../axon_utilities/mfcc>`_.
The applications in this sample use the core MFCC algorithm to demonstrate the following:
* Creating an initial application for developing the algorithm and deploying it to the target device.
* Creating a compilation application to optimize the on-target performance.
* Creating a vector generation application to generate vectors for neural net model training or other evaluation.
* Creating a vector generation shared library to integrate directly into neural net model training scripts.

axons_ml_app_mfcc
-----------------
This build target is for algorithm development off target, and verification on-target. 
It performs inference on compiled-in test vectors and compares it results to expected value vectors. 
It can be built for targets zephyr and simulator.
The top level source files for simulator and zephyr builds, respectively are `<src/axon_app_mfcc_simulator.c>`_ and `<src/axon_app_mfcc_zephyr.c>`_.

axons_ml_app_mfcc_compiler
--------------------------
This variation runs on the simulator only. It is similar to axons_ml_app_mfcc, except it supports background axon intrinsic compiling. 
The core algorithm code is instrumented to support compilation. 
The application is responsible for creating the file for the compiled results to be written, and using the algorithm in compiler mode. 
Running this application will create a compiled command buffer header file that is then included by the core algorithm code.
The user must copy the header file from the execution directory to the algorithm include folder.
The top level source file is `<src/axon_app_mfcc_compiler.c>`_.

axons_ml_app_mfcc_generate_vectors
--------------------------
This variation runs on the simulator only. It accepts two command line parameters, input_directory and output_directory.
All .csv files in input_directory are used as input vector files (one vector per line). A corresponding output csv is created in output_directory.
The top level source file is `<src/axon_app_mfcc_generate_vectors.c>`_.

axons_ml_shared_lib_mfcc
------------------------
This variation can be called from external scripts or applications. Its only shared function will calculate the output for the passed input vector.
It is used in the tinyml_kws training script.
The top level source file is `<src/axon_shared_lib_mfcc.c>`_.

Building and Running
********************

#. To build the simulator application in VS Code, install CMake extension, and add the `<simulator>`_ folder to the workspace.
#. Select one of the sample mfccs in `axon_utilities/mfcc <../../axon_untilities/mfcc/include>`_. Note: generator scripts for producing custom MFCCs will be provided in a later release.
#. Edit `<prj.conf>`_ (for zephyr) or `<simulator/CMakeList.txt>`_ (for simulator) with the mfcc name.
    #. Specify the mfcc name in CONFIG_AXON_MFCC_NAME (zephyr prj.conf) or AXON_MFCC_NAME (simulator CMakeLists.txt). This is used to include the header files and specify the model symbol names.
#. For a command line zephyr build, 
    #. run west --build in this folder.
    #. Flash the build to the device.
    #. Monitor the UART for messages.
#. For a VS Code simulator build
    #. Use CMake extension to build and run the application. 

Algorithm Development Flow
**************************
Algorithm development flow is described in `axon_utilities/mfcc <../../axon_utilities/mfcc>`_.