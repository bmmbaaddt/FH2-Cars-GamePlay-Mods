#include "vehicle_controls.h"
#include "autodrive_game.h"
#include "features.h"
#include "game_input.h"
#include "world_keys.h"
#include "roof_fade.h"
#include "support.h"

#define RIGHT_STICK_BUTTON 0x80
#define ROOF_HOLD_MILLISECONDS 2000u
#define ROOF_BRANCH_SETTLE_MILLISECONDS 350u
#define ROOF_FADE_MILLISECONDS 350u
#define ROOF_COVER_MILLISECONDS 750u
#define ROOF_COVER_SPEED 50.0f
#define ROOF_RENDER_TIMEOUT_MILLISECONDS 5000u

typedef void (*HeadlightSetter)(void *, unsigned char);
typedef void *(*VelocityGetter)(void *, float *);
typedef void (*AnimationEvent)(void *, const char *);

typedef struct {
    union {
        char text[16];
        const char *pointer;
    } storage;

    size_t length, capacity;
} AnimationName;

typedef void *(*FindAnimationEvent)(void *, void **, const AnimationName *);

static unsigned char *game_image;
typedef void (*CarUpdate)(void *, float, float, unsigned);
typedef void (*AnimationDetail)(void *, int);
typedef void (*AnimationSpeed)(void *, float);
static void vehicle_update(void *car, float dt, float frame_dt, unsigned frame);
static unsigned char original_callback[8], replacement_callback[8];
static const unsigned char *accepted_callbacks[] = {original_callback, replacement_callback};
PatchDefinition vehicle_controls_hook_patch;

typedef struct {
    void *controller, *car, *animation;
    uint64_t pressed_at;
    int blocked, pending, consumed, stationary, roof_blocked, freeroam;
} VehicleGesture;

static VehicleGesture gesture = {.blocked = 1};

typedef struct {
    void *car, *animation;
    int active, enabled;
} HeadlightState;

static HeadlightState headlights;

// Indicators use the setters the traffic AI calls; the game's renderer does the blinking.
typedef enum { SIGNALS_OFF, SIGNALS_LEFT, SIGNALS_RIGHT, SIGNALS_HAZARDS } SignalMode;

static struct {
    void *car, *animation;
    SignalMode mode;
} signals;

enum { KEY_HEADLIGHTS, KEY_LEFT, KEY_RIGHT, KEY_HAZARDS, KEY_COUNT };
static int keys[KEY_COUNT], keys_down[KEY_COUNT];

typedef struct {
    void *car, *animation;
    int active, open;
    uint64_t ready_at;
    const char *pending_event;
} RoofState;

static RoofState roof;

static struct {
    void *car, *animation;
    int active, enabled;
} popup_lights;

static void *car_meet_car, *car_meet_animation;

typedef enum {
    ROOF_FADE_IDLE,
    ROOF_FADE_OUT,
    ROOF_COVERED,
    ROOF_FADE_IN,
    ROOF_MEET_CLOSE
} RoofFadePhase;

static struct {
    RoofFadePhase phase;
    uint64_t started_at;
    void *car, *animator, *animation;
    float original_speed;
    int accelerated;
} roof_transition;

static struct {
    void *car, *animator;
    uint64_t last_tick;
    float remaining;
} roof_lock;

static void finish_roof_transition(void);

#ifdef FH2_TWEAKS_TEST
static int use_test_clock;
static uint64_t test_clock;
#endif

static uint64_t clock_milliseconds(void) {
#ifdef FH2_TWEAKS_TEST
    if (use_test_clock) {
        return test_clock;
    }
#endif
    return GetTickCount64();
}

void vehicle_controls_bind(unsigned char *image) {
    if (game_image != image) {
        finish_roof_transition();
        gesture = (VehicleGesture){.blocked = 1};
        headlights = (HeadlightState){0};
        roof = (RoofState){0};
        popup_lights.active = 0;
        car_meet_car = car_meet_animation = NULL;
        roof_lock.remaining = 0;
    }
    game_image = image;
    roof_fade_prepare_hook(image);
    world_keys_prepare_hook(image);
    uintptr_t original = (uintptr_t)(image + 0x2a9400);
    uintptr_t replacement = (uintptr_t)vehicle_update;
    memory_copy(original_callback, &original, 8);
    memory_copy(replacement_callback, &replacement, 8);
    vehicle_controls_hook_patch = (PatchDefinition){0x2c5cc00,
                                                    0,
                                                    8,
                                                    GROUP_VEHICLE_CONTROLS,
                                                    "local_car_vehicle_controls_update",
                                                    original_callback,
                                                    replacement_callback,
                                                    replacement_callback,
                                                    accepted_callbacks,
                                                    ARRAY_COUNT(accepted_callbacks)};
}

void vehicle_controls_set_keys(int headlights_key, int left, int right, int hazards) {
    keys[KEY_HEADLIGHTS] = headlights_key;
    keys[KEY_LEFT] = left;
    keys[KEY_RIGHT] = right;
    keys[KEY_HAZARDS] = hazards;
}

static void apply_signals(void *car, SignalMode mode) {
    ((HeadlightSetter)(game_image + 0xf755c0))(car, mode == SIGNALS_HAZARDS);
    ((HeadlightSetter)(game_image + 0xf76f10))(car, mode == SIGNALS_LEFT);
    ((HeadlightSetter)(game_image + 0xf774e0))(car, mode == SIGNALS_RIGHT);
}

static void cancel_gesture(void) {
    gesture.pending = 0;
    gesture.blocked = 1;
    gesture.car = gesture.animation = NULL;
    if (roof_transition.phase != ROOF_MEET_CLOSE) {
        roof.pending_event = NULL;
        finish_roof_transition();
    }
}

static void *car_animation(void *car);

static void toggle_headlights(void *car) {
    unsigned char manual, forced, automatic;
    unsigned char *bytes = car;
    if (!memory_read(bytes + 0x51a1, &manual, 1) || !memory_read(bytes + 0x51a2, &forced, 1) ||
        !memory_read(bytes + 0x51b4, &automatic, 1) || !memory_is_writable(bytes + 0x51a1, 2)) {
        return;
    }
    void *animation = car_animation(car);
    int owned = headlights.active && headlights.car == car && headlights.animation == animation;
    unsigned char enabled = !(owned ? headlights.enabled : manual ? forced : automatic);
    headlights.car = car;
    headlights.animation = animation;
    headlights.active = 1;
    headlights.enabled = enabled;
    ((HeadlightSetter)(game_image + 0xf74ef0))(car, enabled);
    ((HeadlightSetter)(game_image + 0xf74ee0))(car, 1);
}

static int car_is_stationary(void *car) {
    unsigned char *target = (unsigned char *)car + 0x20;
    void *body, *vtable, *target_vtable;
    uintptr_t getter;
    if (!memory_read_pointer(target, &target_vtable) || target_vtable != game_image + 0x2c5ce28 ||
        !memory_read((unsigned char *)target_vtable + 0x10, &getter, sizeof(getter)) ||
        getter != (uintptr_t)game_image + 0xf646b0 ||
        !memory_read_pointer(target + 0x4be8, &body) || !memory_read_pointer(body, &vtable) ||
        !memory_read((unsigned char *)vtable + 0x288, &getter, sizeof(getter)) ||
        getter < (uintptr_t)game_image + 0x1000 || getter >= (uintptr_t)game_image + 0x1dcc000) {
        return 0;
    }
    _Alignas(16) float velocity[4] = {0};
    ((VelocityGetter)(game_image + 0xf646b0))(target, velocity);
    float speed_squared = 0;
    for (unsigned i = 0; i < 3; ++i) {
        if (!float_is_finite(velocity[i])) {
            return 0;
        }
        speed_squared += velocity[i] * velocity[i];
    }
    return speed_squared <= 0.01f;
}

static void *car_animation(void *car) {
    void *animator, *animation;
    if (!memory_read_pointer((unsigned char *)car + 0x4d40, &animator) ||
        !memory_read_pointer((unsigned char *)animator + 0x1c8, &animation)) {
        return NULL;
    }
    return animation;
}

static void finish_roof_transition(void) {
    void *animator;
    if (roof_transition.accelerated &&
        memory_read_pointer((unsigned char *)roof_transition.car + 0x4d40, &animator) &&
        animator == roof_transition.animator &&
        memory_is_writable((unsigned char *)animator + 0x20, 4)) {
        ((AnimationSpeed)(game_image + 0xf74e00))(roof_transition.car,
                                                  roof_transition.original_speed);
    }
    roof_transition.phase = ROOF_FADE_IDLE;
    roof_transition.accelerated = 0;
    roof_fade_set_opacity(0.0f);
}

static float roof_animation_duration(void *car) {
    unsigned model_id;
    if (!memory_read((unsigned char *)car + 0x118, &model_id, sizeof(model_id))) {
        return 0;
    }
    return model_id == 1218 ? 13.0f : model_id == 1062 ? 21.0f : model_id == 2004 ? 12.0f : 0.0f;
}

static int roof_is_busy(void *car) {
    return roof.pending_event || roof_transition.phase != ROOF_FADE_IDLE ||
           (roof_lock.car == car && roof_lock.remaining > 0.0f);
}

static void update_roof_lock(void *car, uint64_t now) {
    if (roof_lock.remaining <= 0.0f) {
        return;
    }
    void *animator;
    if (car != roof_lock.car || !memory_read_pointer((unsigned char *)car + 0x4d40, &animator) ||
        animator != roof_lock.animator) {
        roof_lock.remaining = 0.0f;
        return;
    }
    uint64_t elapsed = now - roof_lock.last_tick;
    roof_lock.last_tick = now;
    if (elapsed > 250) {
        elapsed = 250;
    }
    float speed;
    // Long gaps and paused updates must not count as time spent animating.
    if (autodrive_game_window_focused() && autodrive_game_is_freeroam() &&
        memory_read((unsigned char *)animator + 0x20, &speed, sizeof(speed)) &&
        float_is_finite(speed) && speed > 0.0f) {
        roof_lock.remaining -= (float)elapsed * 0.001f * speed;
    }
}

static int has_animation_event(void *animation, const char *name) {
    uintptr_t map[7];
    if (!animation || !memory_read((unsigned char *)animation + 0x268, map, sizeof(map)) ||
        !map[0] || !map[2] || !map[6] || map[5] > 65535 || map[6] > map[5] + 1 || !map[1] ||
        map[1] > 65536) {
        return 0;
    }
    AnimationName key = {.length = text_length(name), .capacity = 15};
    if (key.length < sizeof(key.storage.text)) {
        memory_copy(key.storage.text, name, key.length);
    } else {
        key.storage.pointer = name;
        key.capacity = key.length;
    }
    void *entry = NULL;
    ((FindAnimationEvent)(game_image + 0x16fd4d0))((unsigned char *)animation + 0x268, &entry,
                                                   &key);
    return entry && entry != (void *)map[0];
}

static void update_popup_lights(void *car, void *animation, uint64_t now) {
    if (!roof.active || roof.car != car || roof.animation != animation || now < roof.ready_at) {
        return;
    }
    unsigned char manual, forced, automatic;
    unsigned char *bytes = car;
    if (!memory_read(bytes + 0x51a1, &manual, 1) || !memory_read(bytes + 0x51a2, &forced, 1) ||
        !memory_read(bytes + 0x51b4, &automatic, 1)) {
        return;
    }
    int enabled = manual ? forced != 0 : automatic != 0;
    if (popup_lights.active && popup_lights.car == car && popup_lights.animation == animation &&
        popup_lights.enabled == enabled) {
        return;
    }
    static const char *events[2][2] = {{"headlightL:close", "headlightR:close"},
                                       {"headlightL:open", "headlightR:open"}};
    for (unsigned side = 0; side < 2; ++side) {
        const char *event = events[enabled][side];
        if (has_animation_event(animation, event)) {
            ((AnimationEvent)(game_image + 0xf5d300))(car, event);
        }
    }
    popup_lights.car = car;
    popup_lights.animation = animation;
    popup_lights.enabled = enabled;
    popup_lights.active = 1;
}

static void request_roof_toggle(void *car, void *animation, uint64_t now) {
    if (roof_is_busy(car) || !has_animation_event(animation, "roof:open") ||
        !has_animation_event(animation, "roof:close")) {
        return;
    }
    if (roof.car != car || roof.animation != animation) {
        roof.car = car;
        roof.animation = animation;
        roof.open = 0;
        popup_lights.active = 0;
    }
    roof.active = 1;
    ((AnimationDetail)(game_image + 0x16fbe90))(animation, 0);
    roof.pending_event = has_animation_event(animation, "roof:toggle") ? "roof:toggle"
                         : roof.open                                   ? "roof:close"
                                                                       : "roof:open";
    roof.ready_at = now + ROOF_BRANCH_SETTLE_MILLISECONDS;
    if (!roof_animation_duration(car)) {
        void *animator;
        float speed;
        if (memory_read_pointer((unsigned char *)car + 0x4d40, &animator) &&
            memory_read((unsigned char *)animator + 0x20, &speed, sizeof(speed)) &&
            float_is_finite(speed) && memory_is_writable((unsigned char *)animator + 0x20, 4)) {
            roof_transition.phase = ROOF_FADE_OUT;
            roof_transition.started_at = now;
            roof_transition.car = car;
            roof_transition.animator = animator;
            roof_transition.animation = animation;
            roof_transition.original_speed = speed;
            roof_transition.accelerated = 0;
            roof_fade_set_opacity(0.0f);
        }
    }
}

static void close_roof_for_car_meet(void *car) {
    if (!autodrive_game_is_carmeet()) {
        car_meet_car = car_meet_animation = NULL;
        return;
    }
    void *animation = car_animation(car);
    if (car_meet_car == car && car_meet_animation == animation) {
        return;
    }
    size_t event_count;
    if (!animation ||
        !memory_read((unsigned char *)animation + 0x270, &event_count, sizeof(event_count)) ||
        !event_count) {
        return;
    }
    finish_roof_transition();
    cancel_gesture();
    roof_lock.remaining = 0.0f;
    if (!has_animation_event(animation, "roof:close")) {
        car_meet_car = car;
        car_meet_animation = animation;
        return;
    }
    void *animator;
    float speed;
    if (!memory_read_pointer((unsigned char *)car + 0x4d40, &animator) ||
        !memory_read((unsigned char *)animator + 0x20, &speed, sizeof(speed)) ||
        !float_is_finite(speed) || speed < 0.0f) {
        return;
    }
    car_meet_car = car;
    car_meet_animation = animation;
    uint64_t now = clock_milliseconds();
    if (roof.car != car || roof.animation != animation) {
        roof.open = 0;
    }
    roof.car = car;
    roof.animation = animation;
    roof.active = 1;
    ((AnimationDetail)(game_image + 0x16fbe90))(animation, 0);
    roof.pending_event = "roof:close";
    roof.ready_at = now + ROOF_BRANCH_SETTLE_MILLISECONDS;
    roof_transition.phase = ROOF_MEET_CLOSE;
    roof_transition.started_at = now;
    roof_transition.car = car;
    roof_transition.animator = animator;
    roof_transition.animation = roof.animation;
    roof_transition.original_speed = speed;
    roof_transition.accelerated = 0;
}

// Returns which shortcut keys went down since the last frame (one bit per KEY_*).
static int keyboard_presses(int focused) {
    int pressed = 0;
    for (int i = 0; i < KEY_COUNT; ++i) {
        int down = focused && game_input_key_down(keys[i]);
        if (down && !keys_down[i]) {
            pressed |= 1 << i;
        }
        keys_down[i] = down;
    }
    return pressed;
}

static void keyboard_tick(void *car, int focused) {
    int pressed = keyboard_presses(focused);
    void *animation = car_animation(car);
    if (signals.car != car || signals.animation != animation) {
        signals.car = car;  // a different car: its indicators start off
        signals.animation = animation;
        signals.mode = SIGNALS_OFF;
    }
    if (pressed & (1 << KEY_HEADLIGHTS)) {
        toggle_headlights(car);
    }
    static const SignalMode modes[KEY_COUNT] = {0, SIGNALS_LEFT, SIGNALS_RIGHT, SIGNALS_HAZARDS};
    for (int i = KEY_LEFT; i < KEY_COUNT; ++i) {
        if (pressed & (1 << i)) {
            signals.mode = signals.mode == modes[i] ? SIGNALS_OFF : modes[i];
            apply_signals(car, signals.mode);
        }
    }
}

static void vehicle_controls_tick(void *car, int focused) {
    void *controller;
    unsigned short buttons;
    int available = autodrive_game_buttons(&controller, &buttons) == 0;
    int down = available && (buttons & RIGHT_STICK_BUTTON);
    if (!focused || !available || controller != gesture.controller) {
        cancel_gesture();
        gesture.controller = controller;
        return;
    }
    if (!down && !gesture.pending) {
        gesture.blocked = 0;
        return;
    }
    if (gesture.blocked) {
        return;
    }
    int freeroam = autodrive_game_is_freeroam();
    if (!autodrive_game_has_gameplay_input(controller) ||
        (gesture.pending && gesture.freeroam != freeroam)) {
        cancel_gesture();
        return;
    }
    if (!car) {
        cancel_gesture();
        return;
    }
    void *animation = car_animation(car);
    if (!gesture.pending) {
        gesture.car = car;
        gesture.animation = animation;
        gesture.pending = 1;
        gesture.consumed = 0;
        gesture.freeroam = freeroam;
        gesture.pressed_at = clock_milliseconds();
        gesture.stationary = freeroam && car_is_stationary(car);
        gesture.roof_blocked = roof_is_busy(car);
    } else if (gesture.car != car || gesture.animation != animation) {
        cancel_gesture();
    } else {
        if (!gesture.consumed && gesture.stationary) {
            gesture.stationary = car_is_stationary(car);
        }
        uint64_t now = clock_milliseconds();
        if (!gesture.consumed && now - gesture.pressed_at >= ROOF_HOLD_MILLISECONDS) {
            gesture.consumed = 1;
            if (gesture.stationary && !gesture.roof_blocked) {
                request_roof_toggle(car, animation, now);
            }
        }
        if (!down) {
            if (!gesture.consumed) {
                toggle_headlights(car);
            }
            gesture.pending = 0;
        }
    }
}

static void update_roof_transition(void *car, void *animation, uint64_t now) {
    if (roof_transition.phase == ROOF_FADE_IDLE) {
        return;
    }
    int meet_close = roof_transition.phase == ROOF_MEET_CLOSE;
    if (car != roof_transition.car || animation != roof_transition.animation ||
        (meet_close ? !autodrive_game_is_carmeet()
                    : !autodrive_game_window_focused() || !autodrive_game_is_freeroam())) {
        roof.pending_event = NULL;
        finish_roof_transition();
        return;
    }
    uint64_t elapsed = now - roof_transition.started_at;
    if (meet_close) {
        if (!roof.pending_event && elapsed >= ROOF_COVER_MILLISECONDS) {
            finish_roof_transition();
            roof.active = 0;
        }
        return;
    }
    if (roof_transition.phase == ROOF_FADE_OUT) {
        float opacity =
            elapsed < ROOF_FADE_MILLISECONDS ? (float)elapsed / ROOF_FADE_MILLISECONDS : 1.0f;
        roof_fade_set_opacity(opacity);
        if (elapsed >= ROOF_RENDER_TIMEOUT_MILLISECONDS) {
            roof.pending_event = NULL;
            finish_roof_transition();
        }
    } else if (roof_transition.phase == ROOF_COVERED) {
        if (elapsed >= ROOF_COVER_MILLISECONDS) {
            ((AnimationSpeed)(game_image + 0xf74e00))(car, roof_transition.original_speed);
            roof_transition.accelerated = 0;
            roof_transition.phase = ROOF_FADE_IN;
            roof_transition.started_at = now;
        } else {
            ((AnimationSpeed)(game_image + 0xf74e00))(car, ROOF_COVER_SPEED);
        }
    } else {
        if (elapsed >= ROOF_FADE_MILLISECONDS) {
            finish_roof_transition();
        } else {
            roof_fade_set_opacity(1.0f - (float)elapsed / ROOF_FADE_MILLISECONDS);
        }
    }
}

static void maintain_controls(void *car) {
    if (signals.mode && signals.car == car) {
        apply_signals(car, signals.mode);
    }
    if (!headlights.active && !roof.active && !roof.pending_event &&
        roof_transition.phase == ROOF_FADE_IDLE && roof_lock.remaining <= 0.0f) {
        return;
    }
    void *animation = car_animation(car);
    uint64_t now = clock_milliseconds();
    update_roof_lock(car, now);
    update_roof_transition(car, animation, now);
    if (headlights.active && headlights.car == car && headlights.animation == animation) {
        ((HeadlightSetter)(game_image + 0xf74ef0))(car, (unsigned char)headlights.enabled);
        ((HeadlightSetter)(game_image + 0xf74ee0))(car, 1);
    }
    int detail;
    if (roof.active && roof.car == car && roof.animation == animation &&
        memory_read((unsigned char *)animation + 0x10, &detail, 4) && detail != 0) {
        ((AnimationDetail)(game_image + 0x16fbe90))(animation, 0);
    }
    update_popup_lights(car, animation, now);
    if (roof.pending_event && roof.car == car && roof.animation == animation &&
        now >= roof.ready_at &&
        (roof_transition.phase == ROOF_FADE_IDLE || roof_transition.phase == ROOF_MEET_CLOSE ||
         roof_fade_black_frame_presented())) {
        const char *event = roof.pending_event;
        roof.pending_event = NULL;
        int meet_close = roof_transition.phase == ROOF_MEET_CLOSE;
        if (meet_close || (autodrive_game_is_freeroam() && car_is_stationary(car))) {
            if (roof_transition.phase == ROOF_FADE_OUT || meet_close) {
                ((AnimationSpeed)(game_image + 0xf74e00))(car, ROOF_COVER_SPEED);
                roof_transition.accelerated = 1;
                if (!meet_close) {
                    roof_transition.phase = ROOF_COVERED;
                }
                roof_transition.started_at = now;
            }
            ((AnimationEvent)(game_image + 0xf5d300))(car, event);
            roof.open = meet_close ? 0 : !roof.open;
            void *animator;
            if (!meet_close && memory_read_pointer((unsigned char *)car + 0x4d40, &animator)) {
                float duration = roof_animation_duration(car);
                roof_lock.car = car;
                roof_lock.animator = animator;
                roof_lock.last_tick = now;
                roof_lock.remaining =
                    (duration > 0.0f ? duration
                                     : ROOF_COVER_MILLISECONDS * 0.001f * ROOF_COVER_SPEED) +
                    0.5f;
            }
        } else {
            finish_roof_transition();
        }
    }
}

static void vehicle_update(void *self, float dt, float frame_dt, unsigned frame) {
    void *local_car = autodrive_game_get_car();
    int owned = local_car == self;
    if (owned) {
        close_roof_for_car_meet(local_car);
        int focused = autodrive_game_window_focused();
        keyboard_tick(local_car, focused);
        world_keys_tick(focused);
        vehicle_controls_tick(local_car, focused);
        maintain_controls(self);
    }
    ((CarUpdate)(game_image + 0x2a9400))(self, dt, frame_dt, frame);
    if (owned) {
        maintain_controls(self);
    }
    autodrive_game_release_car(local_car);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) void *Fh2TestVehicleHookAddress(void) {
    return (void *)vehicle_update;
}

__declspec(dllexport) void Fh2TestVehicleReset(void) {
    finish_roof_transition();
    gesture = (VehicleGesture){.blocked = 1};
    headlights = (HeadlightState){0};
    roof = (RoofState){0};
    popup_lights.active = 0;
    car_meet_car = car_meet_animation = NULL;
    roof_lock.remaining = 0.0f;
}

__declspec(dllexport) void Fh2TestVehicleTime(uint64_t milliseconds) {
    use_test_clock = 1;
    test_clock = milliseconds;
}

__declspec(dllexport) int Fh2TestVehicleRoofBusy(void) {
    return roof_is_busy(roof_lock.car);
}
#endif
