#include "video_settings.h"
#include "camera_views.h"
#include "features.h"
#include "logging.h"
#include "support.h"

typedef struct {
    uint32_t magic, version;
    int values[VIDEO_SETTING_COUNT];
} SavedVideoSettings;

#define SETTINGS_MAGIC 0x32444846u
#define SETTINGS_VERSION 5u
#define DEFAULT_SETTING_VALUES                                                                     \
    {[DRIFT_MODE] = DRIFT_DRIVER_ONLY,                                                             \
     [DRIFT_CHASE_SENSITIVITY] = 35,                                                               \
     [DRIFT_CHASE_SPEED] = 70,                                                                     \
     [DRIFT_CHASE_RANGE] = 80,                                                                     \
     [DRIFT_DRIVER_SENSITIVITY] = 50,                                                              \
     [DRIFT_DRIVER_SPEED] = 100,                                                                   \
     [DRIFT_DRIVER_RANGE] = 20,                                                                    \
     [FOV_CHASE] = 97,                                                                             \
     [FOV_FAR_CHASE] = 116,                                                                        \
     [FOV_DRIVER] = 96,                                                                            \
     [FOV_HOOD] = 132,                                                                             \
     [FOV_BUMPER] = 96,                                                                            \
     [PHOTO_SAMPLES_PRESET] = 1,                                                                   \
     [MINIMAP_ORIENTATION] = 1,                                                                    \
     [AUTODRIVE_CINEMATIC] = 1,                                                                    \
     [SPAWN_LOCATION] = SPAWN_DEFAULT,                                                             \
     [CAR_CHANGE_POSITION] = CAR_POSITION_NEAREST_ROAD}

static const SavedVideoSettings defaults = {SETTINGS_MAGIC, SETTINGS_VERSION,
                                            DEFAULT_SETTING_VALUES};
static SavedVideoSettings settings = {SETTINGS_MAGIC, SETTINGS_VERSION, DEFAULT_SETTING_VALUES};
_Static_assert(sizeof(SavedVideoSettings) == 92, "Saved settings ABI");

static wchar_t settings_path[PATH_CAPACITY];
static int import_legacy, import_placement;
static const int photo_samples[] = {0, 256, 512, 1024, 2048, 4096};

static int retired_setting(unsigned index) {
    return index == RESERVED_MOTION_SLOT || index == RESERVED_FILTERING_SLOT;
}

static int valid_value(unsigned index, int value) {
    if (index >= VIDEO_SETTING_COUNT || value < 0) {
        return 0;
    }
    if (index == DRIFT_MODE) {
        return value <= DRIFT_OFF;
    }
    if (index < DRIFT_SETTING_COUNT) {
        return value <= 100;
    }
    if (index < CAMERA_SETTING_COUNT) {
        return value >= FOV_MIN_DEGREES * 2 && value <= FOV_MAX_DEGREES * 2;
    }
    if (retired_setting(index)) {
        return value <= 3;
    }
    if (index == PHOTO_SAMPLES_PRESET) {
        return value <= 5;
    }
    if (index == SPAWN_LOCATION) {
        return value <= SPAWN_FESTIVAL_SITE;
    }
    return value <= 1;
}

static void save_settings(void) {
    if (!settings_path[0]) {
        return;
    }
    HANDLE file = CreateFileW(settings_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written = 0;
    int saved = file != INVALID_HANDLE_VALUE &&
                WriteFile(file, &settings, sizeof(settings), &written, NULL) &&
                written == sizeof(settings);
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }
    if (!saved) {
        log_append("VIDEO: could not save Video menu choices; current choices "
                   "remain active.\r\n");
    }
}

void video_settings_load(const wchar_t *directory) {
    settings = defaults;
    import_legacy = import_placement = 1;
    settings_path[0] = 0;
    if (!directory || !path_join(settings_path, directory, L"FH2_ExtraOptions.dat")) {
        return;
    }
    HANDLE file = CreateFileW(settings_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    SavedVideoSettings saved = defaults;
    LARGE_INTEGER size;
    DWORD read = 0;

    static const struct {
        DWORD bytes;
        unsigned settings;
    } layouts[] = {
        {8 + DRIFT_SETTING_COUNT * sizeof(int), DRIFT_SETTING_COUNT},
        {8 + CAMERA_SETTING_COUNT * sizeof(int), CAMERA_SETTING_COUNT},
        {12 + CAMERA_SETTING_COUNT * sizeof(int), CAMERA_SETTING_COUNT},
        {8 + SPAWN_LOCATION * sizeof(int), SPAWN_LOCATION},
        {sizeof(SavedVideoSettings), VIDEO_SETTING_COUNT},
    };

    int valid = GetFileSizeEx(file, &size) && size.QuadPart >= 8 &&
                size.QuadPart <= (LONGLONG)sizeof(saved) &&
                ReadFile(file, &saved, (DWORD)size.QuadPart, &read, NULL) && read == size.QuadPart;
    CloseHandle(file);
    unsigned count = 0;
    if (valid && saved.magic == SETTINGS_MAGIC && saved.version >= 1 &&
        saved.version <= ARRAY_COUNT(layouts)) {
        count = layouts[saved.version - 1].settings;
        valid = size.QuadPart == layouts[saved.version - 1].bytes;
    } else {
        valid = 0;
    }
    for (unsigned index = 0; valid && index < count; ++index) {
        valid = valid_value(index, saved.values[index]);
    }
    if (valid) {
        import_legacy = saved.version < 4;
        import_placement = saved.version < 5;
        saved.version = defaults.version;
        if (import_legacy) {
            for (unsigned index = REALTIME_SHADOWS; index < SPAWN_LOCATION; ++index) {
                saved.values[index] = defaults.values[index];
            }
        }
        saved.values[RESERVED_FILTERING_SLOT] = saved.values[RESERVED_MOTION_SLOT] = 0;
        settings = saved;
    }
}

int video_setting_get(unsigned index) {
    return index < VIDEO_SETTING_COUNT ? settings.values[index] : 0;
}

int video_setting_set(unsigned index, int value) {
    if (retired_setting(index) || !valid_value(index, value) || settings.values[index] == value) {
        return 0;
    }
    settings.values[index] = value;
    if (index >= MINIMAP_ORIENTATION) {
        video_settings_refresh_hud();
    }
    save_settings();
    return 1;
}

void video_settings_reset(void) {
    settings = defaults;
    video_settings_refresh_hud();
    save_settings();
}

static int nearest_preset(const int *presets, unsigned count, int value) {
    int selected = 0, distance = 0x7fffffff;
    for (unsigned index = 0; index < count; ++index) {
        int error = presets[index] - value;
        if (error < 0) {
            error = -error;
        }
        if (error < distance) {
            selected = (int)index;
            distance = error;
        }
    }
    return selected;
}

void video_settings_seed(const ModOptions *legacy) {
    if (!import_legacy && !import_placement) {
        return;
    }
    if (import_legacy) {
        settings.values[REALTIME_SHADOWS] = legacy->smooth_sun_shadows != 0;
        settings.values[PHOTO_SAMPLES_PRESET] =
            nearest_preset(photo_samples, ARRAY_COUNT(photo_samples), legacy->photo_mode_samples);
        settings.values[MINIMAP_ORIENTATION] = legacy->minimap_follows_camera != 0;
        settings.values[AUTODRIVE_CINEMATIC] = legacy->autodrive_cinematic != 0;
        settings.values[SKIP_CHAMPIONSHIP_DIALOGUE] = legacy->skip_ineligible_car_dialogue != 0;
    }
    if (import_placement) {
        settings.values[SPAWN_LOCATION] = !legacy->skip_car_meet_on_startup ? SPAWN_DEFAULT
                                          : legacy->spawn_at_festival       ? SPAWN_FESTIVAL_SITE
                                                                      : SPAWN_CAR_MEET_ENTRANCE;
        settings.values[CAR_CHANGE_POSITION] =
            legacy->snap_car_to_road ? CAR_POSITION_NEAREST_ROAD : CAR_POSITION_SAME_LOCATION;
    }
    import_legacy = import_placement = 0;
    save_settings();
}

void video_settings_apply(ModOptions *options) {
    options->drift_camera = 1;
    options->smooth_sun_shadows = settings.values[REALTIME_SHADOWS];
    options->photo_mode_samples = photo_samples[settings.values[PHOTO_SAMPLES_PRESET]];
    options->minimap_follows_camera = settings.values[MINIMAP_ORIENTATION];
    options->autodrive_cinematic = settings.values[AUTODRIVE_CINEMATIC];
    options->skip_ineligible_car_dialogue = settings.values[SKIP_CHAMPIONSHIP_DIALOGUE];
    options->skip_car_meet_on_startup = settings.values[SPAWN_LOCATION] != SPAWN_DEFAULT;
    options->spawn_at_festival = settings.values[SPAWN_LOCATION] == SPAWN_FESTIVAL_SITE;
    options->snap_car_to_road = settings.values[CAR_CHANGE_POSITION] == CAR_POSITION_NEAREST_ROAD;
}

void video_settings_refresh_hud(void) {
    minimap_set_follow_camera(settings.values[MINIMAP_ORIENTATION]);
    autodrive_set_cinematic(settings.values[AUTODRIVE_CINEMATIC]);
    road_set_enabled(settings.values[CAR_CHANGE_POSITION] == CAR_POSITION_NEAREST_ROAD);
}

unsigned video_setting_page(unsigned index) {
    return index < CAMERA_SETTING_COUNT || index >= MINIMAP_ORIENTATION ? HUD_PAGE : VIDEO_PAGE;
}

void video_settings_reset_page(unsigned page) {
    for (unsigned index = 0; index < VIDEO_SETTING_COUNT; ++index) {
        if (video_setting_page(index) == page) {
            settings.values[index] = defaults.values[index];
        }
    }
    if (page == HUD_PAGE) {
        video_settings_refresh_hud();
    }
    save_settings();
}

static unsigned fov_setting(unsigned view) {
    return view == CAMERA_CHASE || view == CAMERA_CHASE_NEAR ? FOV_CHASE
           : view >= CAMERA_CHASE_FAR && view <= CAMERA_BUMPER
               ? FOV_FAR_CHASE + view - CAMERA_CHASE_FAR
               : CAMERA_SETTING_COUNT;
}

float camera_fov_degrees(unsigned view) {
    unsigned index = fov_setting(view);
    return index < CAMERA_SETTING_COUNT ? video_setting_get(index) * 0.5f : 0;
}

float camera_fov_default(unsigned view) {
    unsigned index = fov_setting(view);
    return index < CAMERA_SETTING_COUNT ? defaults.values[index] * 0.5f : 0;
}

int drift_view_enabled(unsigned view) {
    int mode = settings.values[DRIFT_MODE];
    return mode == DRIFT_ON || (mode == DRIFT_DRIVER_ONLY && view >= CAMERA_DRIVER);
}

float drift_look_speed(unsigned view) {
    int speed = settings.values[view >= CAMERA_DRIVER ? DRIFT_DRIVER_SPEED : DRIFT_CHASE_SPEED];
    return speed <= 50 ? 0.25f + speed * 0.015f : 1.0f + (speed - 50) * 0.06f;
}

float drift_angle(unsigned view, float angle) {
    unsigned family = view >= CAMERA_DRIVER ? DRIFT_DRIVER_SENSITIVITY : DRIFT_CHASE_SENSITIVITY;
    angle *= settings.values[family] / 50.0f;
    float range = settings.values[family + 2] * (2.09439510239f / 100.0f);
    return angle < -range ? -range : angle > range ? range : angle;
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) int Fh2TestDriftSettingGet(unsigned index) {
    return index < DRIFT_SETTING_COUNT ? video_setting_get(index) : 0;
}

__declspec(dllexport) int Fh2TestDriftSettingSet(unsigned index, int value) {
    return index < DRIFT_SETTING_COUNT && video_setting_set(index, value);
}

__declspec(dllexport) void Fh2TestDriftSettingsLoad(const wchar_t *directory) {
    video_settings_load(directory);
}

__declspec(dllexport) void Fh2TestDriftSettingsReset(void) {
    video_settings_reset();
}

__declspec(dllexport) int Fh2TestCameraSettingGet(unsigned index) {
    return video_setting_get(index);
}

__declspec(dllexport) int Fh2TestCameraSettingSet(unsigned index, int value) {
    return video_setting_set(index, value);
}

__declspec(dllexport) void Fh2TestSettingsSeed(ModOptions *options) {
    video_settings_seed(options);
    video_settings_apply(options);
}
#endif
