.. _axon_intrinsics:

Axon Intrinsics
###############

Overview
********

This application demonstrates use of the axon intrinsic functions. Each intrinsic is execuated against a test vector and compare to an expected value.

Only the Axon Simulator and Nordic devices with the Axons ML accelerator can be targeted.

Building and Running
********************
#. To build the simulator application in VS Code, install CMake extension, and add the `<simulator>`_ folder to the workspace.
#. For a command line zephyr build, 
    #. run west --build in this folder.
    #. Flash the build to the device.
    #. Monitor the UART for messages.
#. For a VS Code simulator build
    #. Use CMake extension to build and run the application. 


Sample Output (simulator build)
*******************************
.. code-block:: console

    TEST:   AXON_INTRINSICS CASE COUNT      45
    xty_16_16_32_output_stride_tests: 5 cases

    TEST:   AXON_INTRINSICS START CASE NO   0
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 0       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   1
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 1       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   2
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 2       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   3

    TEST:   AXON_INTRINSICS CASE NO 3       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   4

    TEST:   AXON_INTRINSICS CASE NO 4       RESULT: PASS
    memset_32_output_stride tests: 3 cases

    TEST:   AXON_INTRINSICS START CASE NO   5
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 5       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   6
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 6       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   7

    TEST:   AXON_INTRINSICS CASE NO 7       RESULT: PASS
    saturate_32_24_tests: 3 cases

    TEST:   AXON_INTRINSICS START CASE NO   8
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 8       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   9
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 9       RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   10
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 10      RESULT: PASS
    xty_16_16_32_tests: 4 cases

    TEST:   AXON_INTRINSICS START CASE NO   11
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 11      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   12
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 12      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   13
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 13      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   14

    TEST:   AXON_INTRINSICS CASE NO 14      RESULT: PASS
    fft_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   15
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 15      RESULT: PASS
    xspys_24_24_24_input_stride2_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   16
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 16      RESULT: PASS
    xspys_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   17
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 17      RESULT: PASS
    xsmys_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   18
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 18      RESULT: PASS
    mar_16_24_32_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   19
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 19      RESULT: PASS
    mar_16_24_24_tests: 3 cases

    TEST:   AXON_INTRINSICS START CASE NO   20
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 20      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   21
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 21      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   22
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 22      RESULT: PASS
    sqrt_24_tests: 4 cases

    TEST:   AXON_INTRINSICS START CASE NO   23
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 23      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   24

    TEST:   AXON_INTRINSICS CASE NO 24      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   25

    TEST:   AXON_INTRINSICS CASE NO 25      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   26

    TEST:   AXON_INTRINSICS CASE NO 26      RESULT: PASS
    logn_11p12_tests: 4 cases

    TEST:   AXON_INTRINSICS START CASE NO   27
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 27      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   28

    TEST:   AXON_INTRINSICS CASE NO 28      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   29

    TEST:   AXON_INTRINSICS CASE NO 29      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   30

    TEST:   AXON_INTRINSICS CASE NO 30      RESULT: PASS
    exp_11p12_tests: 4 cases

    TEST:   AXON_INTRINSICS START CASE NO   31
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 31      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   32

    TEST:   AXON_INTRINSICS CASE NO 32      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   33

    TEST:   AXON_INTRINSICS CASE NO 33      RESULT: PASS

    TEST:   AXON_INTRINSICS START CASE NO   34

    TEST:   AXON_INTRINSICS CASE NO 34      RESULT: PASS
    xs_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   35
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 35      RESULT: PASS
    axpby_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   36
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 36      RESULT: PASS
    xty_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   37
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 37      RESULT: PASS
    xpy_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   38
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 38      RESULT: PASS
    xmy_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   39
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 39      RESULT: PASS
    acc_16_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   40
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 40      RESULT: PASS
    fir_24_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   41
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 41      RESULT: PASS
    fir_24_16_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   42
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 42      RESULT: PASS
    axpb_24_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   43
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 43      RESULT: PASS


    fft_power_24_tests: 1 cases

    TEST:   AXON_INTRINSICS START CASE NO   44
    Verify ...  PASSED!

    TEST:   AXON_INTRINSICS CASE NO 44      RESULT: PASS


    TEST:   AXON_INTRINSICS COMPLETE        PASS COUNT      45      FAIL COUNT      0

    axon_app_intrinsics complete!
    