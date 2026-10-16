// Cockpit steering wheel lock-to-lock angle, visuals only. The game queues the drawn wheel's angle to the
// renderer through CCarPresentationDeferred::SetSteeringWheelAngle(this, angle, transform, value); the drawn
// wheel turns only as far as the road wheels (about 84 degrees lock to lock, 2 x the usual 42 degree lock).
// Both vtable entries of that call are pointed at a wrapper that scales the angle on its way to the renderer.
#include "features.h"
#include "support.h"

#define SET_WHEEL_ANGLE_RVA 0x134fb20u
#define ROAD_WHEEL_LOCK 42.0f  // SteerMaxAngle of nearly every car

typedef void (*SetWheelAngle)(void *, float, void *, float);

static unsigned char *game_image;
static float scale = 1.0f;
static void set_wheel_angle(void *self, float angle, void *transform, float value);
static unsigned char original_callback[8], replacement_callback[8];
static const unsigned char *accepted_callbacks[] = {original_callback, replacement_callback};
PatchDefinition steering_wheel_patches[2];

void steering_wheel_prepare_patch(unsigned char *image, const ModOptions *options) {
    game_image = image;
    uintptr_t original = (uintptr_t)(image + SET_WHEEL_ANGLE_RVA);
    // 0 = the game's own wheel: the entries stay pointed at the game's function
    uintptr_t replacement = options->steering_wheel_lock ? (uintptr_t)set_wheel_angle : original;
    scale = (float)options->steering_wheel_lock * 0.5f / ROAD_WHEEL_LOCK;
    memory_copy(original_callback, &original, 8);
    memory_copy(replacement_callback, &replacement, 8);
    static const uint32_t slots[2] = {0x2ebe9a8, 0x2ebf130};
    static const char *names[2] = {"cockpit_steering_wheel_angle", "cockpit_steering_wheel_angle_refcounted"};
    for (int i = 0; i < 2; ++i) {
        steering_wheel_patches[i] = (PatchDefinition){slots[i], 0, 8, GROUP_VEHICLE_CONTROLS, names[i],
                                                      original_callback, replacement_callback,
                                                      replacement_callback, accepted_callbacks,
                                                      ARRAY_COUNT(accepted_callbacks)};
    }
}

static void set_wheel_angle(void *self, float angle, void *transform, float value) {
    ((SetWheelAngle)(game_image + SET_WHEEL_ANGLE_RVA))(self, angle * scale, transform, value);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) float Fh2TestSteeringWheelScale(float angle) {
    return angle * scale;
}
#endif
