
#include <zephyr/kernel.h>

#include <nrf_edgeai/rt/nrf_edgeai_runtime.h>
#include <nrf_edgeai/rt/nrf_edgeai_runtime_aux.h>  // for utility functions nrf_edgeai_model_axon_init_persistent_vars

#include "models/wakeword/nrf_edgeai_generated/nrf_edgeai_user_model.h"
#include "models/kws/nrf_edgeai_generated/nrf_edgeai_user_model.h"

#include "dmic.h"
#include "math.h"

//////////////////////////////////////////////////////////////////////////////

#define MODEL_WAKEWORD_LABEL        "Okay Nordic"
#define KEYWORD_SPOTTING_TIMEOUT_MS 7000
#define PRINT_RAW_PROBABILITY       0

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

typedef struct model_observability_meta_s
{
    size_t       num_classes;
    size_t       num_inferences_for_psi;
    size_t       num_bins;
    const float* entropy_bin_edges;
    const float* baseline_entropy;
    const float  probability_threshold;
} model_observability_meta_t;

typedef struct model_observability_ctx_s
{
    size_t* bin_entropy_counts;
    float*  bin_entropy_dist;
    size_t  inference_count;
} model_observability_ctx_t;

typedef struct model_observability_s
{
    const model_observability_meta_t meta;
    model_observability_ctx_t        ctx;
} model_observability_t;

//////////////////////////////////////////////////////////////////////////////

#define NUM_BINS 2

static const float  PROBABILITY_THRESHOLD  = 0.3f;
static const size_t NUM_INFERENCES_FOR_PSI = 5000;

static const float BASELINE_ENTROPY_DIST[] = { 0.908861453063231, 0.09113854693676901 };
static const float ENTROPY_BIN_EDGES[]     = { 2.8630409869967563e-11, 0.1897623273968213, 0.7 };

static size_t CURRENT_ENTROPY_BIN_COUNTS[NUM_BINS] = { 0 };
static float  CURRENT_ENTROPY_DIST[NUM_BINS]       = { 0.0f };

static model_observability_t model_observability = {
    .meta = {
        .num_classes = 1,
        .num_inferences_for_psi = NUM_INFERENCES_FOR_PSI,
        .num_bins = NUM_BINS,
        .entropy_bin_edges = ENTROPY_BIN_EDGES,
        .baseline_entropy = BASELINE_ENTROPY_DIST,
        .probability_threshold = PROBABILITY_THRESHOLD,
    },
    .ctx = {
        .bin_entropy_counts = CURRENT_ENTROPY_BIN_COUNTS,
        .bin_entropy_dist = CURRENT_ENTROPY_DIST,
        .inference_count = 0,
    },
};

//////////////////////////////////////////////////////////////////////////////

static float compute_psi(const float bin_entropy_current[],
                         const float bin_entropy_baseline[],
                         size_t      num_bins);
static float compute_entropy(const float probabilities[], size_t num_classes);
static void print_model_observability(model_observability_t* m_obsv, const float psi);
static void process_prediction_probability(model_observability_t* m_obsv,
                                           const float            probability[],
                                           size_t                 num_classes);

//////////////////////////////////////////////////////////////////////////////

int main()
{
    printk("Starting Wakeword gated KWS Application...\n");
    printk("\tWakeword: %s\n", MODEL_WAKEWORD_LABEL);

    nrf_edgeai_rt_version_t libver = nrf_edgeai_runtime_version();
    printk("Nordic Edge AI Library version: %d.%d.%d\n",
           libver.field.major,
           libver.field.minor,
           libver.field.patch);

    nrf_edgeai_t* p_wakeword_model = nrf_edgeai_user_model_wakeword();

    // Initialize wakeword model
    nrf_edgeai_err_t res = nrf_edgeai_init(p_wakeword_model);
    __ASSERT(res == NRF_EDGEAI_ERR_SUCCESS,
             "Failed to initialize Wakeword Edge AI model, error code: %d\n",
             res);

    // Initialize PDM microphone
    int err = dmic_init();
    __ASSERT(err == 0, "Failed to initialize DMIC, error code: %d\n", err);

    err = dmic_start();
    __ASSERT(err == 0, "Failed to start DMIC, error code: %d\n", err);

    void*  audio_buffer;
    size_t audio_buffer_size;

    const int32_t read_timeout   = 100;

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

                process_prediction_probability(&model_observability, &probability, 1);
            }
            break;
            case APP_WAITING_FOR_KEYWORDS:
                break;

            default:
                break;
        }

        dmic_free_buffer(audio_buffer);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static float compute_psi(const float bin_entropy_current[],
                         const float bin_entropy_baseline[],
                         size_t      num_bins)
{
    float sum = 0.0f;
    for (size_t i = 0; i < num_bins; i++)
    {
        sum += (bin_entropy_current[i] - bin_entropy_baseline[i]) *
               logf(bin_entropy_current[i] / bin_entropy_baseline[i]);
    }

    return sum;
}

//////////////////////////////////////////////////////////////////////////////

static float compute_entropy(const float probabilities[], size_t num_classes)
{
    float entropy = 0.0f;
    float epsilon = 1e-10f;  // Small constant to avoid log(0)

    for (size_t i = 0; i < num_classes; i++)
    {
        float p = probabilities[i];

        // Clip probabilities to avoid log(0) and log(>1)
        p = p < epsilon ? epsilon : (p > 1.0f - epsilon) ? 1.0f - epsilon : p;

        entropy += -(p * logf(p) + (1.0f - p) * logf(1.0f - p));
    }

    return entropy;
}

//////////////////////////////////////////////////////////////////////////////

static void print_model_observability(model_observability_t* m_obsv, const float psi)
{
    static size_t print_count = 0;
    print_count++;

    printk("model_obsv[%zu]: {\t", print_count);
    printk("num_classes = %u,\n", m_obsv->meta.num_classes);
    printk("num_inferences_for_psi = %u,\n", m_obsv->meta.num_inferences_for_psi);
    printk("num_bins = %u,\n", m_obsv->meta.num_bins);
    printk("probability_threshold = %f,\n", m_obsv->meta.probability_threshold);

    printk("entropy_bin_edges = [");
    for (size_t i = 0; i < m_obsv->meta.num_bins + 1; i++)
    {
        printk("%f,\n", m_obsv->meta.entropy_bin_edges[i]);
    }
    printk("], \n");

    printk("baseline_entropy = [");
    for (size_t i = 0; i < m_obsv->meta.num_bins; i++)
    {
        printk("%f,\n", m_obsv->meta.baseline_entropy[i]);
    }
    printk("], \n");

    printk("current_inference_count = %u,\n", m_obsv->ctx.inference_count);

    printk("current_bin_entropy_counts = [");
    for (size_t i = 0; i < m_obsv->meta.num_bins; i++)
    {
        printk("%u,", m_obsv->ctx.bin_entropy_counts[i]);
    }
    printk("], \n");

    printk("current_bin_entropy_dist = [");
    for (size_t i = 0; i < m_obsv->meta.num_bins; i++)
    {
        printk("%f,", m_obsv->ctx.bin_entropy_dist[i]);
    }
    printk("], \n");

    printk("psi_entropy = %f\n", psi);
    printk("}\n");
}

//////////////////////////////////////////////////////////////////////////////

static void process_prediction_probability(model_observability_t* m_obsv,
                                           const float            probability[],
                                           size_t                 num_classes)
{

    if (probability[0] < m_obsv->meta.probability_threshold) { return; }

    m_obsv->ctx.inference_count++;

    float e = compute_entropy(probability, num_classes);

    for (size_t i = 1; i < m_obsv->meta.num_bins; i++)
    {
        if (e <= m_obsv->meta.entropy_bin_edges[i])
        {
            m_obsv->ctx.bin_entropy_counts[i - 1]++;
            break;
        }
    }

    if (m_obsv->ctx.inference_count >= m_obsv->meta.num_inferences_for_psi)
    {
        for (size_t i = 0; i < m_obsv->meta.num_bins; i++)
        {
            m_obsv->ctx.bin_entropy_dist[i] =
                (float)m_obsv->ctx.bin_entropy_counts[i] / (float)m_obsv->ctx.inference_count;
        }

        float psi = compute_psi(m_obsv->ctx.bin_entropy_dist,
                                m_obsv->meta.baseline_entropy,
                                m_obsv->meta.num_bins);

        print_model_observability(m_obsv, psi);

        // Reset counts and distribution for the next round of PSI calculation
        for (size_t i = 0; i < m_obsv->meta.num_bins; i++)
        {
            m_obsv->ctx.bin_entropy_counts[i] = 0;
            m_obsv->ctx.bin_entropy_dist[i]   = 0.0f;
        }
        m_obsv->ctx.inference_count = 0;
    }
}