
.. _nrf_axon_root:

Axon NPU: Hardware Accelerated Machine Learning
###############################################

.. contents::
   :local:
   :depth: 2

Overview
********

The Axon NPU can accelerate neural networks as well as some feature extraction algorithms.
This repository contains the driver, sample, simulator, and tool chain files to support Axon NPU.
Applications can be created to target the SoC (running zephyr) or a software simulator running on the host machine.

The work flows for neural net models and algorithms are different, even though they share many of the same components.

Neural Net Workflow
*******************

The neural net compiler for Axon NPU is provided in `<tools/axon/compiler/scripts>`_
It has two components; an executor script and a compiler shared object/dll.
The compiler can work with a .tflite file (bare minimum) or the complete Keras model. The complete model with a test data set is needed if the user wants to measure quantization loss.
The user is responsible for populating a configuration .yml file with various parameters. 
The compiler will produce 3 header files: 
1. nrf_axon_model_<model_name>_.h compiled model header file.
2. nrf_axon_model_<model_name>_test_vectors_.h optional compiled-in test vectors.
3. nrf_axon_model_<model_name>_layers_.h optional header file with each layer as a separate model. 
(<model_name> is the short named assigned to the model in the configuration .yaml file.)

The compiler will also report the model's memory requirements and provide a performance estimate (if test data was provided).

The quickest way to verify the model runs properly and to get precise performance numbers is to use the test application `<tests/axon/inference>`_.

See `<tools/axon/compiler/scripts/README.rst>`_ for more details.

Algorithm Development Workflow
******************************

Axon supports hardware acceleration of various operations like FFT, exponent, natural log, dot product...
This functionality is exposed through intrinsics (listed here `<include/drivers/axon/nrf_axon_dsp_intrinsics.h>`_).

Users should develop their algorithms using the simulator where the full suite of debugging and logging tools reside.
Once the algorithm reaches a sufficient stage of maturity, the code can be instrumented to compile the discrete intrinsics into a unified command buffer.
The compiled algorithm will be faster and more efficient than the discrete intrinsics due to less driver involvement during execution.

Running the application on the simulator in compiler mode will create a header file of the compiled algorithm, similar to the neural net compiler output.

The application can then be instrumented further to run in compiled mode. This by-passes the intrinsics entirely and executes the compiled algorithm instead.
An example of this flow can be found in  `<tests/axon/mfcc>`_, which references the MFCC implementation in `<lib/axon/mfcc>`_.

Building Applications
*********************
All applications under `<tests/axon>`_ can target zephyr or the simulator. 
The simulator is simply a static library that is compiled into a console application. It is not cycle exact with hardware, but is bit exact and provides performance estimates.

Zephyr Applications
-------------------

Kconfig dependencies
====================

::

  CONFIG_NRF_AXON=y
  CONFIG_NRF_AXON_INTERLAYER_BUFFER_SIZE=140000 # set this to the maximum value of all Axon models included in the build.

To build the zephyr application, execute "west build..." in the application's root.


Simulator Applications
----------------------

To build a simulator application, use the CMakeLists.txt file under <application>/simulator. The simulator application links with various pre-compiled libraries, so it is required to install the same toolchain the pre-compiled libraries were compiled with.



* For Windows, install and use the MSVC tool chain with CMake extensions in VS Code.
* For Linux, install and use the GCC tool chain.
  
  * Create a folder under simulator build_cli_linux and move to this directory.
  * Generate the Cmake files: cmake -DCMAKE_C_COMPILER:FILEPATH=gcc -DCMAKE_CXX_COMPILER:FILEPATH=g++ -DCMAKE_C_FLAGS:STRING=-fPIC -S.. -B. -G Ninja
  * Build the image: 

::

   cmake --build .

* For MacOS, install and use the AppleClang tool chain.
  
  * Create a folder under simulator build_cli_macos and move to this directory.
  * Generate the Cmake files: cmake -DCMAKE_C_COMPILER:FILEPATH=clang -DCMAKE_CXX_COMPILER:FILEPATH=clang++ -DCMAKE_C_FLAGS:STRING=-fPIC -S.. -B. -G Ninja
  * Build the image: 

:: 

   cmake --build .

Each application's CMakeList.txt file adds the repos root directory. 
At each directory level, CMakeList.txt files add local subdirectories, conditioned on kconfig variables.

For zephyr apps, the kconfig values are declared in the .prj file. For simulator apps, the kconfig values are declared in the <application>/simulator/CMakeLists.txt file.

The applications' README.rst files provide more detail on each of the applications.

Repository Overview
*****************************


Various support code for axon.

`<tools/axon/compiler/scripts>`_
-------------------

Neural net compiler scripts and samples.

`<drivers/axon>`_
-------------------

Public source files for axon driver functionality.

`<lib/axon/platform>`_
-------------------

Implements target platform abstraction. Supported platforms are Zephyr and Simulator (linux, win64, and MacOS).
It is not advised for users to modify the platform code.

`<tests/axon>`_
-------------------
Test applications for various axon functionality.

`<include/axon>`_
-------------------

Header files for axon support code.

`<include/drivers/axon>`_
-------------------

Header files for axon driver.

