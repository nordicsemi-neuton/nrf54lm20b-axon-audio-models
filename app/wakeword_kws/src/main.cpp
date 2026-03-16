
#include <zephyr/kernel.h>

#include <nrf_edgeai/rt/nrf_edgeai_runtime.h>
#include <nrf_edgeai/rt/nrf_edgeai_runtime_aux.h>  // for utility functions nrf_edgeai_model_axon_init_persistent_vars

#include "models/wakeword/nrf_edgeai_generated/nrf_edgeai_user_model.h"
#include "models/kws/nrf_edgeai_generated/nrf_edgeai_user_model.h"

#include "dmic.h"

//////////////////////////////////////////////////////////////////////////////

#define MODEL_WAKEWORD_LABEL        "Okay Nordic"
#define KEYWORD_SPOTTING_TIMEOUT_MS 7000

//////////////////////////////////////////////////////////////////////////////

typedef enum application_state_e
{
    APP_WAITING_FOR_WAKEWORD,
    APP_WAITING_FOR_KEYWORDS,
} application_state_t;

typedef enum keyword_labels_e
{
    KEYWORD_DOWN    = 0,
    KEYWORD_GO      = 1,
    KEYWORD_LEFT    = 2,
    KEYWORD_NO      = 3,
    KEYWORD_OFF     = 4,
    KEYWORD_ON      = 5,
    KEYWORD_RIGHT   = 6,
    KEYWORD_SILENCE = 7,
    KEYWORD_STOP    = 8,
    KEYWORD_UNKNOWN = 9,
    KEYWORD_UP      = 10,
    KEYWORD_YES     = 11,

    KEYWORDS_cnt
} keyword_labels_t;

typedef struct
{
    const char* name;
    flt32_t     prob_threshold;
    size_t      num_in_row;
} keyword_detection_ctx_t;

//////////////////////////////////////////////////////////////////////////////

static bool is_wakeword_detected(flt32_t probability);
static bool is_keyword_detected(uint16_t     predicted_class,
                                flt32_t      probability,
                                const char** pp_keyword_name,
                                uint8_t*     p_average_probability);

//////////////////////////////////////////////////////////////////////////////

static const char* KEYWORDS_LABELS[] = { [KEYWORD_DOWN] = "Down",   [KEYWORD_GO] = "Go",
                                         [KEYWORD_LEFT] = "Left",   [KEYWORD_NO] = "No",
                                         [KEYWORD_OFF] = "Off",     [KEYWORD_ON] = "On",
                                         [KEYWORD_RIGHT] = "Right", [KEYWORD_SILENCE] = "Silence",
                                         [KEYWORD_STOP] = "Stop",   [KEYWORD_UNKNOWN] = "Unknown",
                                         [KEYWORD_UP] = "Up",       [KEYWORD_YES] = "Yes" };

static const keyword_detection_ctx_t KEYWORDS_CTX[] = {
    [KEYWORD_DOWN]    = { .name           = KEYWORDS_LABELS[KEYWORD_DOWN],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_GO]      = { .name           = KEYWORDS_LABELS[KEYWORD_GO],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_LEFT]    = { .name           = KEYWORDS_LABELS[KEYWORD_LEFT],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_NO]      = { .name           = KEYWORDS_LABELS[KEYWORD_NO],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_OFF]     = { .name           = KEYWORDS_LABELS[KEYWORD_OFF],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_ON]      = { .name           = KEYWORDS_LABELS[KEYWORD_ON],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_RIGHT]   = { .name           = KEYWORDS_LABELS[KEYWORD_RIGHT],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_SILENCE] = { .name           = KEYWORDS_LABELS[KEYWORD_SILENCE],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_STOP]    = { .name           = KEYWORDS_LABELS[KEYWORD_STOP],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_UNKNOWN] = { .name           = KEYWORDS_LABELS[KEYWORD_UNKNOWN],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_UP]      = { .name           = KEYWORDS_LABELS[KEYWORD_UP],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
    [KEYWORD_YES]     = { .name           = KEYWORDS_LABELS[KEYWORD_YES],
                         .prob_threshold = 0.9f,
                         .num_in_row     = 22 },
};

//////////////////////////////////////////////////////////////////////////////

int main()
{
    printk("Starting Wakeword gated KWS Application...\n");
    printk("\tWakeword: %s\n", MODEL_WAKEWORD_LABEL);
    printk("\tSupported keywords: ");
    for (size_t i = 0; i < KEYWORDS_cnt; i++)
    {
        printk("%s%s", KEYWORDS_LABELS[i], (i < KEYWORDS_cnt - 1) ? ", " : "\n");
    }

    nrf_edgeai_rt_version_t libver = nrf_edgeai_runtime_version();
    printk("Nordic Edge AI Library version: %d.%d.%d\n",
           libver.field.major,
           libver.field.minor,
           libver.field.patch);

    nrf_edgeai_t* p_wakeword_model = nrf_edgeai_user_model_wakeword();
    nrf_edgeai_t* p_kws_model      = nrf_edgeai_user_model_kws();

    // Initialize wakeword model
    nrf_edgeai_err_t res = nrf_edgeai_init(p_wakeword_model);
    __ASSERT(res == NRF_EDGEAI_ERR_SUCCESS,
             "Failed to initialize Wakeword Edge AI model, error code: %d\n",
             res);

    // Initialize keyword spotting model
    res = nrf_edgeai_init(p_kws_model);
    __ASSERT(res == NRF_EDGEAI_ERR_SUCCESS,
             "Failed to initialize KWS Edge AI model, error code: %d\n",
             res);

    // Initialize PDM microphone
    int err = dmic_init();
    __ASSERT(err == 0, "Failed to initialize DMIC, error code: %d\n", err);

    err = dmic_start();
    __ASSERT(err == 0, "Failed to start DMIC, error code: %d\n", err);

    void*  audio_buffer;
    size_t audio_buffer_size;

    const int32_t read_timeout   = 100;
    uint32_t      kws_start_time = 0;

    application_state_t app_state = APP_WAITING_FOR_WAKEWORD;
    printk("Waiting for wakeword...\n");

    while (true)
    {
        // Read audio data from DMIC,
        err = dmic_read_buffer(&audio_buffer, &audio_buffer_size, read_timeout);
        if (err < 0)
        {
            printk("Failed to read from DMIC (err %d)\n", err);
            continue;
        }

        size_t samples_num = audio_buffer_size / DMIC_SAMPLE_BYTES;

        switch (app_state)
        {
            case APP_WAITING_FOR_WAKEWORD:
            {
                res = nrf_edgeai_feed_inputs(p_wakeword_model, audio_buffer, samples_num);

                if (res != NRF_EDGEAI_ERR_SUCCESS) { break; }

                res = nrf_edgeai_run_inference(p_wakeword_model);

                if (res != NRF_EDGEAI_ERR_SUCCESS) { break; }

                uint16_t predicted_class = p_wakeword_model->decoded_output.classif.predicted_class;
                flt32_t  probability =
                    p_wakeword_model->decoded_output.classif.probabilities.p_f32[predicted_class];

                if (is_wakeword_detected(probability))
                {
                    printk("Wakeword detected! Starting keyword spotting...\n");
                    // Re-initialize KWS model to reset its state after wakeword detection
                    nrf_edgeai_model_axon_init_persistent_vars(p_kws_model);
                    kws_start_time = k_uptime_get_32();
                    app_state      = APP_WAITING_FOR_KEYWORDS;
                }
            }
            break;
            case APP_WAITING_FOR_KEYWORDS:
            {
                if (k_uptime_get_32() - kws_start_time > KEYWORD_SPOTTING_TIMEOUT_MS)
                {
                    printk("Keyword spotting timeout\nWaiting for wakeword...\n");
                    // Re-initialize WW model to reset its state after keyword detection
                    nrf_edgeai_model_axon_init_persistent_vars(p_wakeword_model);
                    app_state = APP_WAITING_FOR_WAKEWORD;
                    break;
                }

                res = nrf_edgeai_feed_inputs(p_kws_model, audio_buffer, samples_num);

                if (res != NRF_EDGEAI_ERR_SUCCESS) { break; }

                res = nrf_edgeai_run_inference(p_kws_model);

                if (res != NRF_EDGEAI_ERR_SUCCESS) { break; }

                uint16_t predicted_class = p_kws_model->decoded_output.classif.predicted_class;
                flt32_t  probability =
                    p_kws_model->decoded_output.classif.probabilities.p_f32[predicted_class];

                const char* keyword_name            = NULL;
                uint8_t     average_probability_pct = 0;

                if (is_keyword_detected(predicted_class,
                                        probability,
                                        &keyword_name,
                                        &average_probability_pct))
                {
                    printk("Keyword detected: %s, %d %%\n", keyword_name, average_probability_pct);
                    kws_start_time = k_uptime_get_32();  // reset timeout after keyword detection
                }
            }
            break;

            default:
                break;
        }

        dmic_free_buffer(audio_buffer);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static bool is_wakeword_detected(flt32_t probability)
{
#define THRESHOLD  0.95f
#define POS_IN_ROW 12
#define ROW_LEN    33
    static bool result_row[ROW_LEN];

    bool is_detected = false;

    /* move result by 1 position*/
    for (int i = 0; i < ROW_LEN - 1; i++)
    {
        result_row[i] = result_row[i + 1];
    }

    result_row[ROW_LEN - 1] = probability > THRESHOLD ? true : false;

    /* chuck mode */
    static int counter = 0;
    counter++;
    if (counter == ROW_LEN)
    {
        /* count */
        int pos_count = 0;
        for (int i = 0; i < ROW_LEN; i++)
        {
            if (result_row[i] == true) { pos_count++; }
        }
        /* test */
        if (pos_count >= POS_IN_ROW) { is_detected = true; }
        /* reset counter */
        counter = 0;
    }

    return is_detected;
}

//////////////////////////////////////////////////////////////////////////////

static bool is_keyword_detected(uint16_t     predicted_class,
                                flt32_t      probability,
                                const char** pp_keyword_name,
                                uint8_t*     p_average_probability)
{
    typedef struct
    {
        uint16_t predicted_class;
        size_t   count;
        flt32_t  average_probability;
    } keyword_runtime_ctx_t;

    bool is_detected = false;

    static const size_t num_keywords = sizeof(KEYWORDS_CTX) / sizeof(KEYWORDS_CTX[0]);

    if (predicted_class >= num_keywords || (predicted_class == KEYWORD_UNKNOWN) ||
        (predicted_class == KEYWORD_SILENCE))
    {
        *pp_keyword_name       = NULL;
        *p_average_probability = 0;
        return is_detected;
    }

    static keyword_runtime_ctx_t   runtime_ctx   = { 0 };
    const keyword_detection_ctx_t* p_keyword_ctx = &KEYWORDS_CTX[predicted_class];

    if (predicted_class != runtime_ctx.predicted_class)
    {
        // Reset runtime context if predicted class changes
        runtime_ctx.predicted_class     = predicted_class;
        runtime_ctx.count               = 0;
        runtime_ctx.average_probability = 0;
    }

    runtime_ctx.count++;
    runtime_ctx.average_probability +=
        (probability - runtime_ctx.average_probability) / runtime_ctx.count;

    if ((runtime_ctx.count >= p_keyword_ctx->num_in_row) &&
        (runtime_ctx.average_probability >= p_keyword_ctx->prob_threshold))
    {
        *pp_keyword_name       = p_keyword_ctx->name;
        *p_average_probability = (uint8_t)(runtime_ctx.average_probability * 100);
        // Reset runtime context after keyword is detected
        memset(&runtime_ctx, 0, sizeof(runtime_ctx));
        is_detected = true;
    }

    return is_detected;
}