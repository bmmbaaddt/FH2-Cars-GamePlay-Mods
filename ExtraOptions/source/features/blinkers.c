#include "blinkers.h"
#include "autodrive_game.h"
#include "features.h"
#include "support.h"

enum {
    DPAD_UP = 1,
    DPAD_LEFT = 4,
    DPAD_RIGHT = 8,
    HAZARDS = 16,
    DPAD_SIGNALS = DPAD_UP | DPAD_LEFT | DPAD_RIGHT,
    INDICATOR_HOLD_MS = 1000,
    RADIO_RELEASE_MS = 200
};

typedef unsigned char (*RadioQuery)(void *, unsigned, unsigned, unsigned char);
static unsigned char *image;
static uint64_t (*milliseconds)(void);

typedef struct {
    void *controller, *car;
    uint64_t pressed_at, last_tick, radio_deadline;
    unsigned held, active, radio_release;
    int blocked, consumed, turned, photo_ready;
} BlinkerState;

static BlinkerState state = {.blocked = 1};

static unsigned char radio_query(void *controller, unsigned device, unsigned action,
                                 unsigned char consume);
static unsigned char thunk[12], photo_thunk[12], photo_ready_thunk[12];
static float photo_query(void *controller, unsigned device, unsigned action);
static unsigned char photo_hold_ready(void *owner);
static const unsigned char padding[12] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
                                          0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static const unsigned char *const accepted[] = {padding, thunk};
PatchDefinition blinkers_radio_hook_patch = {
    0x964db4, 0,     12,       GROUP_VEHICLE_CONTROLS, "blinker_radio_query_thunk", padding,
    thunk,    thunk, accepted, ARRAY_COUNT(accepted)};

static const unsigned char *const photo_accepted[] = {padding, photo_thunk};
PatchDefinition hazards_photo_hook_patch = {
    0x9630d4, 0,           12,          GROUP_VEHICLE_CONTROLS, "hazards_photo_input_thunk",
    padding,  photo_thunk, photo_thunk, photo_accepted,         ARRAY_COUNT(photo_accepted)};

static const unsigned char *const photo_ready_accepted[] = {padding, photo_ready_thunk};
PatchDefinition hazards_photo_ready_hook_patch = {0x965464,
                                                  0,
                                                  12,
                                                  GROUP_VEHICLE_CONTROLS,
                                                  "hazards_photo_ready_thunk",
                                                  padding,
                                                  photo_ready_thunk,
                                                  photo_ready_thunk,
                                                  photo_ready_accepted,
                                                  ARRAY_COUNT(photo_ready_accepted)};

void blinkers_reset(void) { state = (BlinkerState){.blocked = 1}; }

void blinkers_bind(unsigned char *game_image, uint64_t (*clock)(void)) {
    if (image != game_image) {
        blinkers_reset();
    }
    image = game_image;
    milliseconds = clock;
    static const unsigned char code[12] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0};
    uintptr_t callback = (uintptr_t)radio_query;
    memory_copy(thunk, code, sizeof(code));
    memory_copy(thunk + 2, &callback, sizeof(callback));
    callback = (uintptr_t)photo_query;
    memory_copy(photo_thunk, code, sizeof(code));
    memory_copy(photo_thunk + 2, &callback, sizeof(callback));
    callback = (uintptr_t)photo_hold_ready;
    memory_copy(photo_ready_thunk, code, sizeof(code));
    memory_copy(photo_ready_thunk + 2, &callback, sizeof(callback));
}

static void cancel_hold(void) {
    state.held = state.radio_release = 0;
    state.photo_ready = 0;
    state.blocked = 1;
}

static int accepts_signal_input(void *controller) {
    if (autodrive_game_has_gameplay_input(controller)) {
        switch (autodrive_game_presentation_mode()) {
        case 3:  // Race/free roam
        case 4:  // Race with collidables
        case 6:  // Race
        case 9:  // Splitscreen
        case 12: // Player LOD0
            return 1;
        }
    }
    cancel_hold();
    return 0;
}

static void update_lights(void *car) {
    if (!state.active || car != state.car) {
        return;
    }
    unsigned char *lights = (unsigned char *)car + 0x51bb;
    if (memory_is_writable(lights, 3)) {
        lights[0] = state.active == HAZARDS;
        lights[1] = state.active == DPAD_LEFT;
        lights[2] = state.active == DPAD_RIGHT;
    }
}

static void stop_lights(void *car) {
    if (state.active && car == state.car && memory_is_writable((unsigned char *)car + 0x51bb, 3)) {
        static const unsigned char off[3] = {0};
        memory_copy((unsigned char *)car + 0x51bb, off, sizeof(off));
    }
    state.active = 0;
    state.turned = 0;
}

void blinkers_tick(void *car, int focused, uint64_t now) {
    void *controller;
    unsigned short buttons;
    int available = autodrive_game_buttons(&controller, &buttons) == 0;
    unsigned down = buttons & DPAD_SIGNALS;
    if (car != state.car) {
        state.active = state.turned = 0;
        cancel_hold();
        state.car = car;
    }
    if (!car || !focused || !available || controller != state.controller ||
        !accepts_signal_input(controller) || (state.last_tick && now - state.last_tick > 250)) {
        cancel_hold();
        state.controller = controller;
        state.last_tick = now;
        update_lights(car);
        return;
    }
    state.last_tick = now;
    float steering = 0.0f;
    int valid_steering =
        memory_read((unsigned char *)controller + 0xa0, &steering, 4) && float_is_finite(steering);
    float magnitude = steering < 0.0f ? -steering : steering;
    if (state.active && state.active != HAZARDS && valid_steering) {
        if (magnitude >= 0.15f) {
            state.turned = 1;
        } else if (state.turned && magnitude <= 0.05f) {
            stop_lights(car);
        }
    }
    if (state.radio_release && now > state.radio_deadline) {
        state.radio_release = 0;
    }
    if (state.blocked) {
        if (!down) {
            state.blocked = 0;
        }
    } else if ((down & (down - 1)) != 0) {
        cancel_hold();
    } else if (!state.held) {
        if (down) {
            state.held = down;
            state.pressed_at = now;
            state.consumed = 0;
            state.radio_release = 0;
        }
    } else if (down && down != state.held) {
        cancel_hold();
    } else {
        if (!state.consumed && now - state.pressed_at >= INDICATOR_HOLD_MS) {
            state.consumed = 1;
            if (state.held == DPAD_UP) {
                state.photo_ready = 1;
            } else if (state.active == state.held) {
                stop_lights(car);
            } else {
                state.active = state.held;
                state.turned = valid_steering && magnitude >= 0.15f;
            }
        }
        if (!down) {
            if (!state.consumed) {
                if (state.held == DPAD_UP) {
                    if (state.active == HAZARDS) {
                        stop_lights(car);
                    } else {
                        state.active = HAZARDS;
                        state.turned = 0;
                    }
                } else {
                    state.radio_release = state.held;
                    state.radio_deadline = now + RADIO_RELEASE_MS;
                }
            }
            state.photo_ready = 0;
            state.held = 0;
        }
    }
    update_lights(car);
}

static unsigned char radio_query(void *controller, unsigned device, unsigned action,
                                 unsigned char consume) {
    unsigned side = action == 0x17 ? DPAD_LEFT : action == 0x18 ? DPAD_RIGHT : 0;
    void *selected;
    unsigned short buttons;
    if (!side || device || autodrive_game_buttons(&selected, &buttons) != 0 ||
        selected != controller || !accepts_signal_input(controller) ||
        !autodrive_game_window_focused()) {
        return ((RadioQuery)(image + 0x1072500))(controller, device, action, consume);
    }
    uint64_t now = milliseconds();
    if ((buttons & DPAD_SIGNALS || state.held) && state.last_tick != now) {
        void *car = autodrive_game_get_car();
        blinkers_tick(car, 1, now);
        autodrive_game_release_car(car);
    }
    if (buttons & (DPAD_LEFT | DPAD_RIGHT)) {
        return 0;
    }
    if (state.controller == controller && state.radio_release & side) {
        state.radio_release &= ~side;
        return milliseconds() <= state.radio_deadline;
    }
    return ((RadioQuery)(image + 0x1072500))(controller, device, action, consume);
}

static float photo_query(void *controller, unsigned device, unsigned action) {
    void *selected;
    unsigned short buttons;
    if (device || action != 0xf || autodrive_game_buttons(&selected, &buttons) != 0 ||
        selected != controller || !accepts_signal_input(controller) ||
        !autodrive_game_window_focused()) {
        return ((float (*)(void *, unsigned, unsigned))(image + 0x106f040))(controller, device,
                                                                            action);
    }
    int pending = (buttons & DPAD_UP) || state.held == DPAD_UP || state.photo_ready;
    uint64_t now = milliseconds();
    if ((buttons & DPAD_SIGNALS || state.held) && state.last_tick != now) {
        void *car = autodrive_game_get_car();
        blinkers_tick(car, 1, now);
        autodrive_game_release_car(car);
    }
    if (state.photo_ready && state.controller == controller) {
        return 1.0f;
    }
    if (pending) {
        return 0.0f;
    }
    return ((float (*)(void *, unsigned, unsigned))(image + 0x106f040))(controller, device, action);
}

static unsigned char photo_hold_ready(void *owner) {
    void *controller;
    unsigned short buttons;
    if (state.photo_ready && state.held == DPAD_UP && !state.blocked &&
        autodrive_game_buttons(&controller, &buttons) == 0 && controller == state.controller &&
        (buttons & DPAD_SIGNALS) == DPAD_UP && accepts_signal_input(controller) &&
        autodrive_game_window_focused()) {
        return 1;
    }
    return ((unsigned char (*)(void *))(image + 0x963480))(owner);
}

void blinkers_toggle(void *car, int which) {
    static const unsigned modes[4] = {0, DPAD_LEFT, DPAD_RIGHT, HAZARDS};
    if (!car || which < 1 || which > 3) {
        return;
    }
    if (car != state.car) {
        state.active = state.turned = 0;
        cancel_hold();
        state.car = car;
    }
    if (state.active == modes[which]) {
        stop_lights(car);
    } else {
        state.active = modes[which];
        state.turned = 0;
        update_lights(car);
    }
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) unsigned Fh2TestBlinkerSide(void) { return state.active; }
#endif
