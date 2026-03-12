
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <nrf_edgeai/nrf_edgeai.h>

#include "nrf_edgeai_generated/nrf_edgeai_user_model.h"
#include "dmic.h"
#include "leds.h"

//////////////////////////////////////////////////////////////////////////////

#define CONFIDENCE_THRESHOLD      0.68f
#define PREDICTION_NUM_IN_ROW     20
#define MAX_PREDICTION_NUM_IN_ROW 31
#define PRINT_RAW_PROBABILITY     0

//////////////////////////////////////////////////////////////////////////////

static bool snoring_detection_postprocessing(nrf_edgeai_t* p_model);

//////////////////////////////////////////////////////////////////////////////

int main()
{
    printk("Starting Snoring Detection Application...\n");
    printk("Model postprocessig adjustable parameters:\n");
    printk("\tconfidence threshold: %f,\n"
           "\tnumber of predictions in row: %d\n",
           CONFIDENCE_THRESHOLD,
           PREDICTION_NUM_IN_ROW);

    __ASSERT(PREDICTION_NUM_IN_ROW < MAX_PREDICTION_NUM_IN_ROW,
             "Number of predictions in row should be less than %d to fit in the bitmask used for "
             "postprocessing",
             MAX_PREDICTION_NUM_IN_ROW);

    nrf_edgeai_rt_version_t libver = nrf_edgeai_runtime_version();
    printk("Nordic Edge AI Library version: %d.%d.%d\n",
           libver.field.major,
           libver.field.minor,
           libver.field.patch);

    // Get pointer to the Edge AI model instance
    nrf_edgeai_t* p_model = nrf_edgeai_user_model();

    // Initialize snoring model
    nrf_edgeai_err_t res = nrf_edgeai_init(p_model);
    __ASSERT(res == NRF_EDGEAI_ERR_SUCCESS,
             "Failed to initialize Edge AI model, error code: %d",
             res);
    // Initialize PDM microphone and LEDs
    int err = dmic_init();
    __ASSERT(err == 0, "Failed to initialize DMIC, error code: %d", err);

    err = leds_init();
    __ASSERT(err == 0, "Failed to initialize LEDs, error code: %d", err);

    printk("Initialization completed!\n");
    printk("Listening Audio Environment for snoring ...\n");

    err = dmic_start();
    __ASSERT(err == 0, "Failed to start DMIC, error code: %d", err);

    void*         audio_buffer;
    size_t        audio_buffer_size;
    const int32_t read_timeout = 100;

    while (true)
    {
        // Read audio data from DMIC,
        err = dmic_read_buffer(&audio_buffer, &audio_buffer_size, read_timeout);
        if (err < 0)
        {
            printk("Failed to read from DMIC (err %d)", err);
            continue;
        }

        size_t samples_num = audio_buffer_size / DMIC_SAMPLE_BYTES;

        // Feed audio data to the model dsp pipeline and
        // waiting for internal buffers to be filled with enough data for feature extraction
        res = nrf_edgeai_feed_inputs(p_model, audio_buffer, samples_num);

        if (res != NRF_EDGEAI_ERR_SUCCESS) { continue; }

        // Run feature extraction and model inference
        res = nrf_edgeai_run_inference(p_model);

        if (res != NRF_EDGEAI_ERR_SUCCESS) { continue; }

        // Run postprocessing on the model output to determine if snoring is detected based on the model inference results
        if (snoring_detection_postprocessing(p_model))
        {
            printk("Snoring detected! \n");
            leds_blink_led0();
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static bool snoring_detection_postprocessing(nrf_edgeai_t* p_model)
{
    // Rolling postprocessing state kept across calls.
    // - predicitons_history stores recent boolean detections as bits (newest at bit 0).
    // - prediction_count tracks how many `1` bits are currently in the window.
    static uint32_t prediction_count;
    static uint32_t predicitons_history;

    // Read model confidence for the currently predicted class.
    const uint16_t predicted_class = p_model->decoded_output.classif.predicted_class;
    const float probability = p_model->decoded_output.classif.probabilities.p_f32[predicted_class];

    // Convert probability to a binary detection for this frame.
    const bool detected = probability > CONFIDENCE_THRESHOLD;

    // Check the bit that will fall out of the window after the left shift.
    // MAX_PREDICTION_NUM_IN_ROW is the history bit-width limit used by this algorithm.
    const bool oldest_entry = (bool)(predicitons_history & BIT(MAX_PREDICTION_NUM_IN_ROW));

    // Update rolling count in O(1): add newest detection, remove oldest.
    prediction_count = prediction_count + detected - oldest_entry;
    // Shift history left and append current detection at LSB.
    predicitons_history = (predicitons_history << 1) | detected;

#if PRINT_RAW_PROBABILITY
    printk("Predictions count: %2u, probability: %0.3f", prediction_count, probability);
#endif

    if (prediction_count >= PREDICTION_NUM_IN_ROW)
    {
        // Enough positive frames accumulated: emit one detection event
        // and clear state to avoid repeated triggers from stale history.
        prediction_count    = 0;
        predicitons_history = 0;

        return true;
    }

    return false;
}