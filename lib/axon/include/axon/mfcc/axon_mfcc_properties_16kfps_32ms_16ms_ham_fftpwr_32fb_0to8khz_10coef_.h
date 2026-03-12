
/*
 * Copyright (c) 2024, Nordic Semiconductor ASA. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor ASA.
 * The use, copying, transfer or disclosure of such information is prohibited except by
 * express written agreement with Nordic Semiconductor ASA.
*/

#pragma once
#include <assert.h>

/**
 * Audio Feature Properties
 * This file specifies the public properties of the audio features. 
 * This file is auto-generated.
 */

#define AXON_MFCC_FFT_LEN_LOG2 9      // size of the fft log(2).
#define AXON_MFCC_FFT_LEN (1<<AXON_MFCC_FFT_LEN_LOG2)      // size of the fft
#define AXON_MFCC_FRAME_LEN 512       // Number of audio samples per frame, maximum value is 512 (limited by longest FFT supported)
#define AXON_MFCC_FRAME_SHIFT 256     // number of samples to shift between frames.
#define AXON_MFCC_SAMPLE_RATE 16000
#define AXON_MFCC_FILTERBANK_COUNT 32

/**
 * Used by framework code to size the circular buffer to store audio features in. Needs to be sized
 * at least as big as the input requirements of the model.
 */
#define AXON_MFCC_SLICE_CNT 61  

/*
 * MFCC parameters
 */
#define AXON_MFCC_MFCC_ENERGY_APPEND 0  // set to 1 if MFCC should be replaced with the log of the FFT energy
#define AXON_MFCC_MFCC_FFT_MAGNITUDE 0  // set to 1 if square-root should be taken of FFT power prior to filter banks.

#define AXON_MFCC_WIDTH 10  // number of MFCCs to calculate per audio window

#define AXON_MFCC_QUANT_ENABLE 1
