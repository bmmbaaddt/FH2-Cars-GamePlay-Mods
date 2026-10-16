#include "features.h"
#include "support.h"
#include <xmmintrin.h>

#define NATIVE_DENSITY 295u
#define SAMPLE_DENSITY_RVA 0x2e834e4u

static const uint32_t native_density = NATIVE_DENSITY;
static uint32_t selected_density = NATIVE_DENSITY;
static uint32_t applied_density = NATIVE_DENSITY;
static unsigned selected_samples = 129;
static const unsigned char *const accepted_densities[] = {
    (const unsigned char *)&native_density,
    (const unsigned char *)&selected_density,
    (const unsigned char *)&applied_density,
};

static unsigned grid_sample_count(unsigned density) {
    float step = 2.0f / _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((float)density)));
    unsigned count = 0;
    for (float y = -1.0f; y < 1.0f; y += step) {
        float y_squared = y * y;
        for (float x = -1.0f; x < 1.0f; x += step) {
            float radius = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x * x + y_squared)));
            if (radius < 0.75f) {
                ++count;
            }
        }
    }
    return count;
}

PatchDefinition photo_sampling_patch = {
    SAMPLE_DENSITY_RVA,
    0,
    sizeof(native_density),
    GROUP_PHOTO_SAMPLING,
    "photo_mode_sample_density",
    (const unsigned char *)&native_density,
    (const unsigned char *)&selected_density,
    (const unsigned char *)&selected_density,
    accepted_densities,
    ARRAY_COUNT(accepted_densities),
};

void photo_sampling_prepare_patch(const ModOptions *options) {
    selected_density = NATIVE_DENSITY;
    selected_samples = 129;
    unsigned target = (unsigned)options->photo_mode_samples;
    if (!target || target == 129) {
        return;
    }
    unsigned center = (unsigned)((float)target * 2.263537f + 0.5f);
    unsigned smallest_error = ~0u;
    unsigned first = center > 16 ? center - 16 : 1;
    for (unsigned density = first; density <= center + 16; ++density) {
        unsigned samples = grid_sample_count(density);
        unsigned error = samples > target ? samples - target : target - samples;
        if (error < smallest_error) {
            selected_density = density;
            selected_samples = samples;
            smallest_error = error;
        }
        if (!error) {
            break;
        }
    }
}

void photo_sampling_commit_options(void) {
    applied_density = selected_density;
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) unsigned Fh2TestPhotoSampleCount(void) {
    return selected_samples;
}
#endif
