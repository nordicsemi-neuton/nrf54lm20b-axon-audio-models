"""
Copyright (c) 2026 Nordic Semiconductor

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import os
import sys
import yaml
import time
# import glob
import numpy as np
# import pandas as pd
# import datetime as dt
import tensorflow as tf
import tensorflow_datasets as tfds
import python_speech_features as psf
import matplotlib.pyplot as plt

# from ctypes import *
from tensorflow import keras
from tensorflow.python.platform import gfile
# from tensorflow.keras import layers
from tensorflow.keras.models import Model
from tensorflow.keras.layers import Input, Dense, Activation, Flatten, BatchNormalization, Dropout  # , Reshape
# , GlobalAveragePooling2D
from tensorflow.keras.layers import Conv2D, DepthwiseConv2D, AveragePooling2D
from tensorflow.keras.regularizers import l2

from utility import model_data_helper_script as mdhs

label_count = 12
sampling_rate = 16000
sample_rate_khz = sampling_rate/1000
window_size_ms = 32
window_stride_ms = 20
audio_duration_ms = 1000
dct_coefficient_count = 10
desired_samples = int(sampling_rate * audio_duration_ms / 1000)
window_size_samples = int(sampling_rate * window_size_ms / 1000)
window_stride_samples = int(sampling_rate * window_stride_ms / 1000)
length_minus_window = (desired_samples - window_size_samples)
spectrogram_length = 1 + int(length_minus_window / window_stride_samples)

default_get_data_args = {
    'data_directory': r"data\kws_g12\training_data",
    'save_raw_data_csv': True,
    'train_data_fraction': 0.01,
    'batch_file_size_limit_mb': 100,
    'save_raw_data_npy': True,
    'background_noise_dir': None
}

default_train_model_args = {
    'model_directory': None,
    'train_feature_data': None,
    'train_feature_label': None,
    'test_feature_data': None,
    'test_feature_label': None,
    'val_feature_data': None,  # r"data\kws_g12\training_data",
    'val_feature_label': None,
    'axon_fe_dll_path': None,
    'sampling_rate': sampling_rate,
    'audio_duration_ms': audio_duration_ms,
    'window_size_ms': window_size_ms,
    'window_stride_ms': window_stride_ms,
    'dct_coefficient_count': dct_coefficient_count,
    'labels_count': label_count,
    'learning_rate': 0.001,
    'mfcc_shift': 12,
    'model_training_epochs': 50
}


def get_default_get_data_arguments():
    return default_get_data_args


def get_default_train_model_arguments():
    return default_train_model_args


def get_kws_model_dataset_args(args):
    data_args = args['get_data_config']
    if data_args is None:
        return default_get_data_args
    # write code here to populate any empty fields or commands in the arguments dictionary
    for key in data_args:
        if data_args[key] is None or data_args[key] == "":
            data_args[key] = default_get_data_args[key]
    return data_args


def get_kws_model_training_args(args):
    train_model_args = args['train_model_config']
    if train_model_args is None:
        return default_train_model_args
    for key in train_model_args:
        if train_model_args[key] is None or train_model_args[key] == "":
            train_model_args[key] = default_train_model_args[key]
    return train_model_args


def create_kws_model(train_model_config):
    model_settings = train_model_config
    desired_samples = int(
        model_settings['sampling_rate'] * model_settings['audio_duration_ms'] / 1000)
    window_size_samples = int(
        model_settings['sampling_rate'] * model_settings['window_size_ms'] / 1000)
    window_stride_samples = int(
        model_settings['sampling_rate'] * model_settings['window_stride_ms'] / 1000)
    length_minus_window = desired_samples - window_size_samples
    spectrogram_length = 1 + int(length_minus_window / window_stride_samples)
    input_shape = [spectrogram_length,
                   model_settings['dct_coefficient_count'], 1]
    filters = 64
    weight_decay = 1e-4
    regularizer = l2(weight_decay)
    final_pool_size = (int(input_shape[0]/2), int(input_shape[1]/2))

    # Model layers
    # Input pure conv2d
    inputs = Input(shape=input_shape)
    x = Conv2D(filters, (10, 4), strides=(2, 2), padding='same',
               kernel_regularizer=regularizer)(inputs)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)
    x = Dropout(rate=0.2)(x)

    # First layer of separable depthwise conv2d
    # Separable consists of depthwise conv2d followed by conv2d with 1x1 kernels
    x = DepthwiseConv2D(depth_multiplier=1, kernel_size=(
        3, 3), padding='same', kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)
    x = Conv2D(filters, (1, 1), padding='same',
               kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    # Second layer of separable depthwise conv2d
    x = DepthwiseConv2D(depth_multiplier=1, kernel_size=(
        3, 3), padding='same', kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)
    x = Conv2D(filters, (1, 1), padding='same',
               kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    # Third layer of separable depthwise conv2d
    x = DepthwiseConv2D(depth_multiplier=1, kernel_size=(
        3, 3), padding='same', kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)
    x = Conv2D(filters, (1, 1), padding='same',
               kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    # Fourth layer of separable depthwise conv2d
    x = DepthwiseConv2D(depth_multiplier=1, kernel_size=(
        3, 3), padding='same', kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)
    x = Conv2D(filters, (1, 1), padding='same',
               kernel_regularizer=regularizer)(x)
    x = BatchNormalization()(x)
    x = Activation('relu')(x)

    # Reduce size and apply final softmax
    x = Dropout(rate=0.4)(x)

    x = AveragePooling2D(pool_size=final_pool_size)(x)
    x = Flatten()(x)
    outputs = Dense(model_settings['labels_count'], activation='softmax')(x)

    # Instantiate model.
    model = Model(inputs=inputs, outputs=outputs)

    # model.compile(
    #     #optimizer=keras.optimizers.RMSprop(learning_rate=args.learning_rate),  # Optimizer
    #     optimizer=keras.optimizers.Adam(learning_rate=0.02),  # Optimizer
    #     # Loss function to minimize
    #     loss=keras.losses.SparseCategoricalCrossentropy(),
    #     # List of metrics to monitor
    #     metrics=[keras.metrics.SparseCategoricalAccuracy()],
    # )
    return model


def get_mfcc(audio):
    mfcc = psf.base.mfcc(audio, winlen=window_size_ms/1000, winstep=window_stride_ms/1000, numcep=10,
                         nfilt=32, nfft=512, lowfreq=0, preemph=0, ceplifter=0, appendEnergy=False, winfunc=np.hamming)
    return mfcc[0:spectrogram_length]


def calculate_save_mfcc_npy(data, full_numpy_file_name, save_numpy=True, max_count=-1, save_as_int=False):
    start_time = time.time()
    mfcc_val = []
    labels_val = []
    count = 0
    for elements in data:
        count += 1
        mfcc = get_mfcc(elements['audio'])
        mfcc_val.append(mfcc)
        labels_val.append(elements['label'])
        if (count > max_count) and max_count != -1:
            break

    mfcc_dtype = np.float32
    if save_as_int:
        mfcc_dtype = np.int16
    mfcc = np.array(mfcc_val, dtype=mfcc_dtype)
    label = np.array(labels_val, dtype=mfcc_dtype)
    if (save_numpy):
        np.save(full_numpy_file_name+"_mfcc_"+".npy", mfcc)
        np.save(full_numpy_file_name+"_label_"+".npy", label)
    print(
        f"calculating and saving mfcc took {time.time() - start_time} seconds")
    return mfcc, label


def get_kws_model_data(args, get_raw_data_for_out_of_band_mfccs=False):
    data_args = get_kws_model_dataset_args(args)
    SAVE_RAW_DATA_CSV = data_args['save_raw_data_csv']
    DATA_DIR = data_args['data_directory']
    SAVE_RAW_DATA_NPY = data_args['save_raw_data_npy']
    SAMPLE_SIZE_MB = data_args['batch_file_size_limit']
    GET_MFCC_AS_NPY = False
    ds_train, ds_test, ds_val, data_info = get_kws_raw_data(DATA_DIR)
    # move some data out of train data to validation dataset to have balance training
    full_ds = ds_train.concatenate(ds_val)
    full_ds = full_ds.shuffle(len(full_ds))

    train_data_size = int(len(full_ds)*0.8)
    ds_val = full_ds.skip(train_data_size)
    ds_train = full_ds.take(train_data_size)

    # taking only a fraction of the data for getting the numbers
    DATA_FRACTION = data_args['train_data_fraction']  # 1 #/ 1000
    ds_train = ds_train.take(int(len(ds_train)*DATA_FRACTION))
    # ds_test = ds_test.take(int(len(ds_test)*DATA_FRACTION)) #taking the full test data set for running tests
    ds_val = ds_val.take(int(len(ds_val)*DATA_FRACTION))
    if get_raw_data_for_out_of_band_mfccs:
        # print(data_info)
        train = ds_train.map(mdhs.cast_and_pad_audio)
        test = ds_test.map(mdhs.cast_and_pad_audio)
        val = ds_val.map(mdhs.cast_and_pad_audio)

        if SAVE_RAW_DATA_CSV:
            train_x, train_y = mdhs.convert_to_numpy_in_batches(
                train, data_args['train_data_fraction'])
            test_x, test_y = mdhs.convert_to_numpy_in_batches(test)
            val_x, val_y = mdhs.convert_to_numpy_in_batches(val)
            print(f"train data shape {train_x.shape, train_y.shape}")
            print(f"test data shape {test_x.shape, test_y.shape}")
            print(f"val data shape {val_x.shape, val_y.shape}")
            mdhs.save_to_csv_in_batches(
                train_x, train_y, file_name=DATA_DIR+"/train", sample_size_in_mb=SAMPLE_SIZE_MB)
            mdhs.save_to_csv_in_batches(
                test_x, test_y, file_name=DATA_DIR+"/test", sample_size_in_mb=SAMPLE_SIZE_MB)
            mdhs.save_to_csv_in_batches(
                val_x, val_y, file_name=DATA_DIR+"/val", sample_size_in_mb=SAMPLE_SIZE_MB)
            if SAVE_RAW_DATA_NPY:
                time_pre = time.time()
                np.save(DATA_DIR+"/train_x.npy", train_x)
                np.save(DATA_DIR+"/train_y.npy", train_y)
                time_post = time.time()
                print(
                    f"saving npy train data took {time_post - time_pre} seconds")
                np.save(DATA_DIR+"/test_x.npy", test_x)
                np.save(DATA_DIR+"/test_y.npy", test_y)
                time_pre = time.time()
                print(
                    f"saving npy test data took {time_pre - time_post} seconds")
                np.save(DATA_DIR+"/val_x.npy", val_x)
                np.save(DATA_DIR+"/val_y.npy", val_y)
                time_post = time.time()
                print(
                    f"saving npy val data took {time_post - time_pre} seconds")
        if GET_MFCC_AS_NPY:
            test_mfcc = calculate_save_mfcc_npy(test, DATA_DIR+"/test_mfcc_")
            print(f"test mfcc shapes {test_mfcc[0].shape, test_mfcc[1].shape}")
            val_mfcc = calculate_save_mfcc_npy(val, DATA_DIR+"/val_mfcc_")
            print(f"val mfcc shapes {val_mfcc[0].shape, val_mfcc[1].shape}")
            train_mfcc = calculate_save_mfcc_npy(
                train, DATA_DIR+"/train_mfcc_")
            print(
                f"train mfcc shapes {train_mfcc[0].shape, train_mfcc[1].shape}")
    else:
        print("get data using augmentation")
        train_args = get_kws_model_training_args(args)
        # see if background data is present
        if data_args['background_noise_dir'] is not None:
            BACKGROUND_NOISE_DIR_NAME = '_background_noise_'
            background_data = ml_commons_prepare_background_data(
                data_args['background_noise_dir'], BACKGROUND_NOISE_DIR_NAME)
        else:
            print("background data is None!")
            background_data = []
        # extract spectral features and add background noise
        ds_train = ds_train.map(ml_commons_get_preprocess_audio_func(train_args, is_training=True,
                                                                     background_data=background_data),
                                num_parallel_calls=tf.data.experimental.AUTOTUNE)
        ds_test = ds_test.map(ml_commons_get_preprocess_audio_func(train_args, is_training=False,
                                                                   background_data=background_data),
                              num_parallel_calls=tf.data.experimental.AUTOTUNE)
        ds_val = ds_val.map(ml_commons_get_preprocess_audio_func(train_args, is_training=False,
                                                                 background_data=background_data),
                            num_parallel_calls=tf.data.experimental.AUTOTUNE)

        # get the axon mfccs
        if train_args['feature_type'] == 'axon_mfcc':
            axon_fe_lib = mdhs.AxonMfccLibClass(train_args['axon_fe_dll_path'])
            desired_samples = int(
                train_args['sampling_rate'] * train_args['audio_duration_ms'] / 1000)
            window_size_samples = int(
                train_args['sampling_rate'] * train_args['window_size_ms'] / 1000)
            window_stride_samples = int(
                train_args['sampling_rate'] * train_args['window_stride_ms'] / 1000)
            length_minus_window = (desired_samples - window_size_samples)
            spectrogram_length = 1 + \
                int(length_minus_window / window_stride_samples)
            length_minus_window = (desired_samples - window_size_samples)

            train_x, train_y = mdhs.convert_to_numpy_in_batches(ds_train)
            test_x, test_y = mdhs.convert_to_numpy_in_batches(ds_test)
            val_x, val_y = mdhs.convert_to_numpy_in_batches(ds_val)

            # call the mfcc dll
            train_mfcc = mdhs.get_axon_fe_mfcc(
                axon_fe_lib, train_x, spectrogram_length, train_args['dct_coefficient_count'], train_args['mfcc_shift'])
            test_mfcc = mdhs.get_axon_fe_mfcc(
                axon_fe_lib, test_x, spectrogram_length, train_args['dct_coefficient_count'], train_args['mfcc_shift'])
            val_mfcc = mdhs.get_axon_fe_mfcc(
                axon_fe_lib, val_x, spectrogram_length, train_args['dct_coefficient_count'], train_args['mfcc_shift'])

            np.save(DATA_DIR+"/axon_dll_train_mfcc.npy", train_mfcc)
            np.save(DATA_DIR+"/axon_dll_train_y.npy", test_y)
            np.save(DATA_DIR+"/axon_dll_test_mfcc.npy", test_mfcc)
            np.save(DATA_DIR+"/axon_dll_test_y.npy", test_y)
            np.save(DATA_DIR+"/axon_dll_val_mfcc.npy", val_mfcc)
            np.save(DATA_DIR+"/axon_dll_val_y.npy", test_y)

            # convert back to tensorflow data set
            ds_train = mdhs.convert_to_tensorflow_data_set(train_mfcc, train_y)
            ds_test = mdhs.convert_to_tensorflow_data_set(test_mfcc, test_y)
            ds_val = mdhs.convert_to_tensorflow_data_set(val_mfcc, val_y)
        else:
            # may need to save the test data as npy for running multiple tests after training
            SAVE_TEST_DATA_AS_NPY = True
            if SAVE_TEST_DATA_AS_NPY:
                test_x, test_y = mdhs.convert_to_numpy_in_batches(
                    ds_test, data_key='audio', label_key='label')
                np.save(DATA_DIR+"/tf_test_x.npy", test_x)
                np.save(DATA_DIR+"/tf_test_y.npy", test_y)
                print("saved test data as npy")

        # change output from a dictionary to a feature,label tuple
        ds_train = ds_train.map(mdhs.convert_dataset)
        ds_test = ds_test.map(mdhs.convert_dataset)
        ds_val = ds_val.map(mdhs.convert_dataset)

        ds_train = ds_train.batch(train_args['batch_size'])
        ds_test = ds_test.batch(train_args['batch_size'])
        ds_val = ds_val.batch(train_args['batch_size'])
    return ds_train, ds_test, ds_val


def get_kws_raw_data(DATA_DIR):
    time_pre = time.time()
    splits = ['train', 'test', 'validation']
    (ds_train, ds_test, ds_val), ds_info = tfds.load('speech_commands', split=splits, data_dir=DATA_DIR,
                                                     with_info=True)
    print(f"getting kws raw data took {time.time() - time_pre} seconds")
    return ds_train, ds_test, ds_val, ds_info


def plot_training(plot_dir, history, model_variant=""):
    if not os.path.exists(plot_dir):
        os.makedirs(plot_dir)
    plt.subplot(2, 1, 1)
    plt.plot(history.history['sparse_categorical_accuracy'],
             label='Training Accuracy')
    plt.plot(
        history.history['val_sparse_categorical_accuracy'], label='Val Accuracy')
    plt.title('Accuracy vs Epoch')
    plt.ylabel('Accuracy')
    plt.grid(True)
    plt.subplot(2, 1, 2)
    plt.plot(history.history['loss'], label='Loss')
    plt.plot(history.history['val_loss'], label='Val Loss')
    plt.ylabel('Loss')
    plt.xlabel('Epoch')
    plt.grid(True)
    plt.legend(loc="upper left")
    model_variant = "/"+model_variant + ".png"
    plt.savefig(plot_dir+model_variant)


def load_model(train_args):  # TODO can be moved to model data helper script
    # create the model here if there is no model available from the path
    if train_args['model_directory'] is not None:
        assert train_args['model_directory'].endswith(".h5") or os.path.isdir(
            train_args['model_directory']), "Keras model is not valid."
        model = tf.keras.models.load_model(train_args['model_directory'])
        checkpoint_dir = os.path.dirname(train_args['model_directory'])
        print(f"loaded model from path {train_args['model_directory']}")
    else:
        # print("Model path is none! Please provide a full path to a model in keras format!")
        # return
        model = create_kws_model(train_args)
        checkpoint_dir = args['get_data_config']['data_directory']
        model.save(checkpoint_dir+"/starter_init_model.h5")

    return model, checkpoint_dir


# TODO can be moved to model data helper script
def train_model(ds_train, ds_val, ds_test, train_args, model, checkpoint_dir):
    checkpoint_path = checkpoint_dir+f"/kws_mfcc_testing_cp/kws_{train_args['model_name']}_lr_{train_args['learning_rate']}/" + \
        "_{epoch:04d}_{sparse_categorical_accuracy:.5f}_{val_sparse_categorical_accuracy:.5f}.h5"
    # create a callback that saves the model
    early_stopping = tf.keras.callbacks.EarlyStopping(
        monitor="val_sparse_categorical_accuracy", patience=10)  # FIXME make this a yaml input?
    callback = tf.keras.callbacks.ModelCheckpoint(
        checkpoint_path, save_best_only=True, monitor='val_sparse_categorical_accuracy', mode='max', verbose=1)
    callback_list = [callback, early_stopping]
    model.compile(optimizer=keras.optimizers.Adam(learning_rate=train_args['learning_rate']), loss=keras.losses.SparseCategoricalCrossentropy(
    ), metrics=[keras.metrics.SparseCategoricalAccuracy()])
    # start fitting the data set

    train_hist = model.fit(ds_train, validation_data=ds_val,
                           batch_size=train_args['batch_size'], epochs=train_args['model_training_epochs'], callbacks=callback_list)
    model.summary()
    # evaluate the model using the test data set
    train_scores = model.evaluate(ds_train)
    print("Train loss:", train_scores[0])
    print("Train accuracy:", train_scores[1])

    val_scores = model.evaluate(ds_val)
    print("Val loss:", val_scores[0])
    print("Val accuracy:", val_scores[1])

    test_scores = model.evaluate(ds_test)
    print("Test loss:", test_scores[0])
    print("Test accuracy:", test_scores[1])

    # plot the history
    plot_training(checkpoint_dir+"/plots", train_hist,
                  train_args['model_name']+f"lr_{train_args['learning_rate']}_{test_scores[1]:.4f}")
    # save the last model
    model.save(checkpoint_dir+f"/kws_mfcc_testing_cp/kws_{train_args['model_name']}_lr_{train_args['learning_rate']}/" +
               f"/kws_test_{train_args['model_name']}_{test_scores[1]:.4f}.h5")


def train_kws_model_from_features(train_args):

    model, checkpoint_dir = load_model(train_args)
    # get the features
    ds_train = mdhs.load_and_preprocess_training_data(
        train_args['train_feature_data'], train_args['train_feature_label'], shift_value=train_args['mfcc_shift'], input_shape=model.input_shape)
    ds_test = mdhs.load_and_preprocess_training_data(
        train_args['test_feature_data'], train_args['test_feature_label'], shift_value=train_args['mfcc_shift'], input_shape=model.input_shape)
    ds_val = mdhs.load_and_preprocess_training_data(
        train_args['val_feature_data'], train_args['val_feature_label'], shift_value=train_args['mfcc_shift'], input_shape=model.input_shape)
    # run training cycles
    train_model(ds_train, ds_val, ds_test, train_args, model, checkpoint_dir)


def train_kws_model_from_raw(args):
    print("getting dataset....")
    start_time = time.time()
    ds_train, ds_test, ds_val = get_kws_model_data(args)
    print(f"Done getting data, took {time.time() - start_time} seconds")

    # this is taken from the dataset web page.
    # there should be a better way than hard-coding this
    train_shuffle_buffer_size = len(ds_train)  # 85511
    val_shuffle_buffer_size = len(ds_val)  # 10102
    test_shuffle_buffer_size = len(ds_test)  # 4890

    ds_train = ds_train.shuffle(train_shuffle_buffer_size)
    ds_val = ds_val.shuffle(val_shuffle_buffer_size)
    ds_test = ds_test.shuffle(test_shuffle_buffer_size)

    train_args = get_kws_model_training_args(args)

    model, checkpoint_dir = load_model(train_args)
    # run training cycles
    train_model(ds_train, ds_val, ds_test, train_args, model, checkpoint_dir)


def train_kws_model(args):
    train_args = get_kws_model_training_args(args)
    if train_args['use_raw_data']:
        train_kws_model_from_raw(args)
    else:
        train_kws_model_from_features(train_args)
        # for i in range(0,5):
        #     print(f" training for {i+1} time")
        #     t=time.time()
        #     train_kws_model_from_features(train_args)
        #     print(f" training for {i+1} time took {time.time() - t} seconds")


def test_kws_model(args):
    # SAVE_INFERENCE_LABELS_AS_NPY=True
    test_args = args['test_model_config']
    model = tf.keras.models.load_model(test_args['model_directory'])
    ds_test = mdhs.load_and_preprocess_training_data(
        test_args['test_feature_data'], test_args['test_feature_label'], input_shape=model.input_shape)
    test_scores = model.evaluate(ds_test)
    print("Test loss:", test_scores[0])
    print("Test accuracy:", test_scores[1])


def get_min_max_saturate_samples(arg):
    raw_samples = mdhs.get_min_max()
    # get mfccs
    true_mfccs = []
    for samples in raw_samples:
        true_mfccs.append(get_mfcc(samples))
    true_mfccs = np.array(true_mfccs)

    true_mfccs = true_mfccs.reshape(
        true_mfccs.shape[0], spectrogram_length*dct_coefficient_count)
    np.savetxt(r"_raw_samples_test.csv", np.array(
        raw_samples), delimiter=',', fmt='%d')
    np.savetxt(r"_raw_samples_mfcc_true.csv",
               true_mfccs, delimiter=',', fmt='%d')


def ml_commons_prepare_background_data(bg_path, BACKGROUND_NOISE_DIR_NAME):
    return prepare_background_data(bg_path, BACKGROUND_NOISE_DIR_NAME)


def prepare_background_data(bg_path, BACKGROUND_NOISE_DIR_NAME):
    """Searches a folder for background noise audio, and loads it into memory.
    It's expected that the background audio samples will be in a subdirectory
    named '_background_noise_' inside the 'data_dir' folder, as .wavs that match
    the sample rate of the training data, but can be much longer in duration.
    If the '_background_noise_' folder doesn't exist at all, this isn't an
    error, it's just taken to mean that no background noise augmentation should
    be used. If the folder does exist, but it's empty, that's treated as an
    error.
    Returns:
        List of raw PCM-encoded audio samples of background noise.
    Raises:
        Exception: If files aren't found in the folder.
    """
    background_data = []
    background_dir = os.path.join(bg_path, BACKGROUND_NOISE_DIR_NAME)
    if not os.path.exists(background_dir):
        return background_data
    # with tf.Session(graph=tf.Graph()) as sess:
    #    wav_filename_placeholder = tf.placeholder(tf.string, [])
    #    wav_loader = io_ops.read_file(wav_filename_placeholder)
    #    wav_decoder = contrib_audio.decode_wav(wav_loader, desired_channels=1)
    search_path = os.path.join(bg_path, BACKGROUND_NOISE_DIR_NAME, '*.wav')
    # for wav_path in gfile.Glob(search_path):
    #    wav_data = sess.run(wav_decoder, feed_dict={wav_filename_placeholder: wav_path}).audio.flatten()
    #    self.background_data.append(wav_data)
    for wav_path in gfile.Glob(search_path):
        # audio = tfio.audio.AudioIOTensor(wav_path)
        raw_audio = tf.io.read_file(wav_path)
        audio = tf.audio.decode_wav(raw_audio)
        background_data.append(audio[0])
    if not background_data:
        raise Exception('No background wav files were found in ' + search_path)
    return background_data


def ml_commons_get_preprocess_audio_func(train_args, is_training=False, background_data=[], axon_fe_mfcc_function=None):
    def prepare_processing_graph(next_element):
        desired_samples = int(
            train_args['sampling_rate'] * train_args['audio_duration_ms'] / 1000)
        window_size_samples = int(
            train_args['sampling_rate'] * train_args['window_size_ms'] / 1000)
        window_stride_samples = int(
            train_args['sampling_rate'] * train_args['window_stride_ms'] / 1000)
        length_minus_window = (desired_samples - window_size_samples)
        spectrogram_length = 1 + \
            int(length_minus_window / window_stride_samples)
        length_minus_window = (desired_samples - window_size_samples)
        background_frequency = 0.8  # train_args['background_frequency']
        background_volume_range_ = 0.1  # train_args['background_volume_range']
        wav_decoder = tf.cast(next_element['audio'], tf.float32)
        # normalizing the audio sample here with the max value present in the audio sample
        wav_decoder = wav_decoder/tf.reduce_max(wav_decoder)
        wav_decoder = tf.pad(
            wav_decoder, [[0, desired_samples-tf.shape(wav_decoder)[-1]]])
        # Allow the audio sample's volume to be adjusted.
        foreground_volume_placeholder_ = tf.constant(1, dtype=tf.float32)

        scaled_foreground = tf.multiply(wav_decoder,
                                        foreground_volume_placeholder_)
        time_shift_padding_placeholder_ = tf.constant([[2, 2]], tf.int32)
        time_shift_offset_placeholder_ = tf.constant([2], tf.int32)
        scaled_foreground.shape
        padded_foreground = tf.pad(
            scaled_foreground, time_shift_padding_placeholder_, mode='CONSTANT')
        sliced_foreground = tf.slice(
            padded_foreground, time_shift_offset_placeholder_, [desired_samples])
        if is_training and background_data != []:
            # background_volume_range = tf.constant(background_volume_range_,dtype=tf.float32)
            background_index = np.random.randint(len(background_data))
            background_samples = background_data[background_index]
            background_offset = np.random.randint(
                0, len(background_samples) - desired_samples)
            background_clipped = background_samples[background_offset:(
                background_offset + desired_samples)]
            background_clipped = tf.squeeze(background_clipped)
            background_reshaped = tf.pad(
                background_clipped, [[0, desired_samples-tf.shape(wav_decoder)[-1]]])
            background_reshaped = tf.cast(background_reshaped, tf.float32)
            if np.random.uniform(0, 1) < background_frequency:
                background_volume = np.random.uniform(
                    0, background_volume_range_)
            else:
                background_volume = 0
            background_volume_placeholder_ = tf.constant(
                background_volume, dtype=tf.float32)
            background_data_placeholder_ = background_reshaped
            background_mul = tf.multiply(background_data_placeholder_,
                                         background_volume_placeholder_)
            background_add = tf.add(background_mul, sliced_foreground)
            sliced_foreground = tf.clip_by_value(background_add, -1.0, 1.0)
        if train_args['feature_type'] == 'mfcc':
            stfts = tf.signal.stft(sliced_foreground, frame_length=window_size_samples,
                                   frame_step=window_stride_samples, fft_length=None,
                                   window_fn=tf.signal.hann_window
                                   )
            spectrograms = tf.abs(stfts)
            num_spectrogram_bins = stfts.shape[-1]
            # default values used by contrib_audio.mfcc as shown here
            # https://kite.com/python/docs/tensorflow.contrib.slim.rev_block_lib.contrib_framework_ops.audio_ops.mfcc
            lower_edge_hertz, upper_edge_hertz, num_mel_bins = 20.0, 4000.0, 40
            linear_to_mel_weight_matrix = tf.signal.linear_to_mel_weight_matrix(num_mel_bins, num_spectrogram_bins,
                                                                                train_args['sampling_rate'],
                                                                                lower_edge_hertz, upper_edge_hertz)
            mel_spectrograms = tf.tensordot(
                spectrograms, linear_to_mel_weight_matrix, 1)
            mel_spectrograms.set_shape(
                spectrograms.shape[:-1].concatenate(linear_to_mel_weight_matrix.shape[-1:]))
            # Compute a stabilized log to get log-magnitude mel-scale spectrograms.
            log_mel_spectrograms = tf.math.log(mel_spectrograms + 1e-6)
            # Compute MFCCs from log_mel_spectrograms and take the first 13.
            mfccs = tf.signal.mfccs_from_log_mel_spectrograms(
                log_mel_spectrograms)[..., :train_args['dct_coefficient_count']]
            mfccs = tf.reshape(
                mfccs, [spectrogram_length, train_args['dct_coefficient_count'], 1])
            next_element['audio'] = mfccs
            # next_element['label'] = tf.one_hot(next_element['label'],12)
        elif train_args['feature_type'] == 'axon_mfcc':
            """
            Loading the axon mfcc in the tensorflow graph does not work, 
            another approach yet to be tested to perform the axon mfcc calculation using the dll is by converting the calculate function as a python generator function
            and passing that to the model.fit to perform the calculation every epoch.
            the following link might be the explaination to get to a solution
            https://pyimagesearch.com/2018/12/24/how-to-use-keras-fit-and-fit_generator-a-hands-on-tutorial/
            """
            # if no feature type is mentioned, currently only support getting augmented audio for calculating using axons
            # tf.config.run_functions_eagerly(True)
            # input_array = sliced_foreground.numpy()
            # output_array = np.zeros(shape=[490], dtype=np.int32)
            # _ip_memory = c_int16*(16000)
            # _op_memory = c_int32*(490)
            # _op_mfcc_op_width_memory = c_uint32*(1)
            # _mfcc_op_width_array = np.array([0], dtype=np.int32)
            # _ip_ptr = input_array.ctypes.data_as(POINTER(_ip_memory))
            # _op_ptr = output_array.ctypes.data_as(POINTER(_op_memory))
            # _mfcc_op_width_array_ptr = _mfcc_op_width_array.ctypes.data_as(POINTER(_op_mfcc_op_width_memory))
            # axon_fe_mfcc_function(_ip_ptr,16000, _op_ptr, _mfcc_op_width_array_ptr)
            # next_element['audio'] = output_array #sliced_foreground
            # tf.config.run_functions_eagerly(False)
            sliced_foreground = sliced_foreground * \
                tf.constant(2**15, dtype=tf.float32)
            sliced_foreground = tf.cast(sliced_foreground, tf.int16)
            next_element['audio'] = sliced_foreground
        return next_element
    return prepare_processing_graph


if __name__ == "__main__":
    args = None
    if len(sys.argv) == 2:
        with open(sys.argv[1], 'r') as f:
            args = yaml.safe_load(f)
    if args['script_config']['run_mode'] == "train":
        start_time = time.time()
        train_kws_model(args)
        print(f"training took {time.time() - start_time} seconds ")
    elif args['script_config']['run_mode'] == "test":
        test_kws_model(args)
    elif args['script_config']['run_mode'] == "get_data":
        get_kws_model_data(args, True)
    print("Done!")
