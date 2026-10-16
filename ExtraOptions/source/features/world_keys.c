// Ctrl+key world controls (time, weather, traffic, repair) for the KeyboardShortcut components that
// FH2 Mods adds to free roam. The game reads those keys from window key events, which Azimuth does not
// deliver, so a key press here marks the wanted script and the matching component fires itself from its
// own per-frame update: no component pointer is ever kept, so nothing can outlive its entity.
#include "world_keys.h"
#include "features.h"
#include "game_input.h"
#include "logging.h"
#include "support.h"

#define UPDATE_RVA 0x5a8070u       // shared component update(this, dt), slot 2 of both vtables below
#define FLOW_STRING_OFFSET 0x158u  // std::string StateFlow, read by the components' own fire functions
#define FIRE_SLOT 6
#define PENDING_MILLISECONDS 500u

typedef void (*ComponentUpdate)(void *, float);
typedef void (*ComponentFire)(void *);

static unsigned char *game_image;
static void shortcut_update(void *self, float dt);
static unsigned char original_callback[8], replacement_callback[8];
static const unsigned char *accepted_callbacks[] = {original_callback, replacement_callback};
PatchDefinition world_keys_hook_patches[2];

// Ctrl + key -> the script its component runs (names from FH2 Mods' apply.ps1 and the E3 demo)
static const struct {
    int key;
    const char *flow;
} bindings[] = {
    {'T', "e3_keyboard_shortcuts.goto_time_of_day_midnight"},
    {'Y', "e3_keyboard_shortcuts.fh2mods_time_sunrise"},
    {'U', "e3_keyboard_shortcuts.fh2mods_time_midday"},
    {'I', "e3_keyboard_shortcuts.goto_time_of_day_5_ish"},
    {'P', "e3_keyboard_shortcuts.fh2mods_time_stop"},
    {'W', "e3_keyboard_shortcuts.goto_weather_rain"},
    {'R', "e3_keyboard_shortcuts.fh2mods_weather_clear"},
    {'G', "e3_keyboard_shortcuts.fh2mods_traffic_off"},
    {'N', "e3_keyboard_shortcuts.fh2mods_repair"},
    {'B', "e3_keyboard_shortcuts.fh2mods_snap_to_road"},
};
static int keys_down[ARRAY_COUNT(bindings)];
static int sky_key_down;
static const char *volatile pending_flow;
static uint64_t pending_since;
static int update_seen;

void world_keys_prepare_hook(unsigned char *image) {
    game_image = image;
    uintptr_t original = (uintptr_t)(image + UPDATE_RVA);
    uintptr_t replacement = (uintptr_t)shortcut_update;
    memory_copy(original_callback, &original, 8);
    memory_copy(replacement_callback, &replacement, 8);
    static const uint32_t vtable_slots[2] = {0x2ca1f08, 0x2ca1f70};
    static const char *names[2] = {"keyboard_shortcut_update", "keyboard_shortcut_toggle_update"};
    for (int i = 0; i < 2; ++i) {
        world_keys_hook_patches[i] = (PatchDefinition){vtable_slots[i],
                                                       0,
                                                       8,
                                                       GROUP_VEHICLE_CONTROLS,
                                                       names[i],
                                                       original_callback,
                                                       replacement_callback,
                                                       replacement_callback,
                                                       accepted_callbacks,
                                                       ARRAY_COUNT(accepted_callbacks)};
    }
}

void world_keys_tick(int focused) {
    int ctrl = focused && game_input_key_down(VK_CONTROL);
    int sky_key = ctrl && game_input_key_down('F');
    if (sky_key && !sky_key_down) {
        if (sky_road_active()) {  // coming down: the snap-to-road script puts the car back on a road
            pending_since = GetTickCount64();
            pending_flow = "e3_keyboard_shortcuts.fh2mods_snap_to_road";
        }
        sky_road_toggle();
    }
    sky_key_down = sky_key;
    for (unsigned i = 0; i < ARRAY_COUNT(bindings); ++i) {
        int down = ctrl && game_input_key_down(bindings[i].key);
        if (down && !keys_down[i]) {
            pending_since = GetTickCount64();
            pending_flow = bindings[i].flow;
        }
        keys_down[i] = down;
    }
    if (pending_flow && GetTickCount64() - pending_since > PENDING_MILLISECONDS) {
        pending_flow = NULL;  // no matching component is loaded (not in free roam)
    }
}

static int flow_matches(void *component, const char *flow) {
    unsigned char *string = (unsigned char *)component + FLOW_STRING_OFFSET;
    size_t size, capacity;
    const char *text = (const char *)string;
    if (!memory_read(string + 0x10, &size, sizeof(size)) ||
        !memory_read(string + 0x18, &capacity, sizeof(capacity)) || size != text_length(flow) ||
        (capacity >= 16 && !memory_read_pointer(string, (void **)&text))) {
        return 0;
    }
    char buffer[96];
    if (size >= sizeof(buffer) || !memory_read(text, buffer, size)) {
        return 0;
    }
    buffer[size] = 0;
    return text_equal_ignore_case(buffer, flow);
}

static void shortcut_update(void *self, float dt) {
    ((ComponentUpdate)(game_image + UPDATE_RVA))(self, dt);
    if (!update_seen) {
        update_seen = 1;
        log_append("WORLD KEYS: keyboard shortcut components are live; Ctrl keys active.\r\n");
    }
    const char *flow = pending_flow;
    if (flow && flow_matches(self, flow)) {
        pending_flow = NULL;
        void **vtable;
        if (memory_read_pointer(self, (void **)&vtable)) {
            void *fire;
            if (memory_read_pointer(vtable + FIRE_SLOT, &fire)) {
                ((ComponentFire)fire)(self);
            }
        }
    }
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) void Fh2TestWorldKeysPress(int key) {
    for (unsigned i = 0; i < ARRAY_COUNT(bindings); ++i) {
        if (bindings[i].key == key) {
            pending_since = GetTickCount64();
            pending_flow = bindings[i].flow;
        }
    }
}

__declspec(dllexport) void Fh2TestWorldKeysUpdate(void *component) {
    shortcut_update(component, 0.016f);
}

__declspec(dllexport) int Fh2TestWorldKeysPending(void) {
    return pending_flow != NULL;
}
#endif
