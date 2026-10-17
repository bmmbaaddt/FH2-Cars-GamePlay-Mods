#include "features.h"
#include "support.h"
#include <xmmintrin.h>

#define NATIVE_DENSITY 295u
#define SAMPLE_DENSITY_RVA 0x2e834e4u

static const uint32_t native_density = NATIVE_DENSITY;
static uint32_t default_density = NATIVE_DENSITY;
static unsigned char *prepared_image;
static uint32_t selected_density = NATIVE_DENSITY;
static uint32_t applied_density = NATIVE_DENSITY;
static unsigned selected_samples = 129;
#ifdef FH2_AZIMUTH
static const uint32_t launcher_densities[] = {587u, 1170u};
#endif

static const unsigned char *const accepted_densities[] = {
    (const unsigned char *)&native_density,        (const unsigned char *)&selected_density,
    (const unsigned char *)&applied_density,
#ifdef FH2_AZIMUTH
    (const unsigned char *)&launcher_densities[0], (const unsigned char *)&launcher_densities[1],
#endif
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
    (const unsigned char *)&default_density,
    (const unsigned char *)&selected_density,
    (const unsigned char *)&selected_density,
    accepted_densities,
    ARRAY_COUNT(accepted_densities),
};

void photo_sampling_prepare_patch(unsigned char *game_image, const ModOptions *options) {
    if (prepared_image != game_image) {
        default_density = NATIVE_DENSITY;
#ifdef FH2_AZIMUTH
        uint32_t loaded_density;
        memory_copy(&loaded_density, game_image + SAMPLE_DENSITY_RVA, sizeof(loaded_density));
        for (unsigned i = 0; i < ARRAY_COUNT(launcher_densities); ++i) {
            if (loaded_density == launcher_densities[i]) {
                default_density = loaded_density;
                break;
            }
        }
#endif
        applied_density = default_density;
        prepared_image = game_image;
    }
    selected_density = options->photo_mode_samples ? NATIVE_DENSITY : default_density;
    selected_samples = grid_sample_count(selected_density);
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
