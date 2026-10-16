#include "features.h"
#include "logging.h"
#include "support.h"
#include "video_options_text.h"
#include "video_options_xaml.h"
#include "video_settings.h"

#define RESOURCE_THUNK_RVA 0x13fe354u
#define RESET_SLOT_RVA (0x2e60370u + 0x590u)
#define XAML_LIMIT 65536u

enum {
    NATIVE_DELETE_OBJECT = 0x257730,
    NATIVE_COLLECTION_TYPE = 0x1476110,
    NATIVE_ALLOCATE_TYPED = 0x14424e0,
    NATIVE_COLLECTION_INIT = 0xaefd60,
    NATIVE_MAKE_STRING = 0x13fe8e0,
    NATIVE_INSERT_ITEM = 0xb52170,
    NATIVE_REGISTER_VIDEO_TYPE = 0xeca710,
    NATIVE_REGISTER_HUD_TYPE = 0xd453d0,
    HUD_TYPE_REGISTRY = 0x35235e0,
    NATIVE_RESET_HUD = 0xd33ae0,
    NATIVE_INT_PROPERTY = 0xed81f0,
    NATIVE_COLLECTION_PROPERTY = 0xed77d0,
    NATIVE_FLOAT_PROPERTY = 0xed6e90,
    NATIVE_REGISTER_PROPERTY = 0x155ee30,
    VIDEO_TYPE_REGISTRY = 0x35470d0,
    NATIVE_ALLOCATE = 0x3229c0,
    NATIVE_FREE = 0x31c7b0,
    NATIVE_GET_RESOURCE = 0xaabec0,
    NATIVE_RESET_VIDEO = 0xebf060,
    NATIVE_OBJECT_INIT = 0x143eb80,
    NATIVE_OBJECT_CLEANUP = 0x1442050,
    NATIVE_BOX_INT = 0x4702d0,
    NATIVE_BOX_FLOAT = 0x3f5990,
    NATIVE_PROPERTY_CHANGED = 0xcf9c60,
};

typedef struct {
    void **vtable;
    unsigned char header[0x20];
    void *getter, *setter, *unused;
    const char *name;
    void *type;
    uint64_t flags;
} NativeProperty;

typedef struct NativePropertyLink {
    NativeProperty *property;
    struct NativePropertyLink *next;
} NativePropertyLink;

typedef struct {
    void **vtable;
    unsigned char header[0x20];
    char *bytes;
    int length;
    int capacity;
} NativeResource;

_Static_assert(sizeof(NativeProperty) == 0x58, "Native property descriptor");
_Static_assert(offsetof(NativeResource, bytes) == 0x28, "Native resource buffer");

typedef void *(*Allocate)(size_t);
typedef void (*Free)(void *);
typedef void (*ObjectInitialize)(void *);
typedef void *(*PropertyFactory)(void);
typedef void *(*TypeFactory)(void *);
typedef void **(*BoxValue)(void **, const void *);
typedef void **(*GetResource)(void *, void **);
typedef void (*PropertyChanged)(void *, void *);
typedef void (*ResetOptions)(void *);
typedef void **(*MakeString)(void **, const wchar_t *);
typedef void (*InsertItem)(void *, int, void *);

enum {
    CHOICES_DRIFT,
    CHOICES_TOGGLE,
    CHOICES_PHOTO,
    CHOICES_MINIMAP,
    CHOICES_SPAWN,
    CHOICES_CAR_POSITION,
    CHOICE_COLLECTION_COUNT
};

static unsigned char *image;
static Allocate allocate;
static Free free_object;
static GetResource native_resource;
static ResetOptions native_reset, native_hud_reset;
static ObjectInitialize object_initialize, object_cleanup;
static BoxValue box_int, box_float;
static PropertyChanged property_changed;
static NativeProperty properties[VIDEO_SETTING_COUNT + CHOICE_COLLECTION_COUNT];
static NativePropertyLink links[2][VIDEO_SETTING_COUNT + CHOICE_COLLECTION_COUNT];
static void *choices[CHOICE_COLLECTION_COUNT];
static const unsigned choice_counts[] = {3, 2, 6, 2, 3, 2};
static void *registered_image[2], *initialized_image;
static void *resource_vtable[7];
static unsigned char resource_thunk[12], reset_original[8], reset_callback[8],
    hud_reset_original[8], hud_reset_callback[8];
static const unsigned char unused_thunk[12] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
                                               0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static const unsigned char *const accepted_thunk[] = {unused_thunk, resource_thunk};
static const unsigned char *const accepted_reset[] = {reset_original, reset_callback};
static const unsigned char *const accepted_hud_reset[] = {hud_reset_original, hud_reset_callback};

static const char *const property_names[] = {"DriftMode",
                                             "DriftChaseSensitivity",
                                             "DriftChaseSpeed",
                                             "DriftChaseRange",
                                             "DriftDriverSensitivity",
                                             "DriftDriverSpeed",
                                             "DriftDriverRange",
                                             "FovChase",
                                             "FovFarChase",
                                             "FovDriver",
                                             "FovHood",
                                             "FovBumper",
                                             "RealtimeShadows",
                                             NULL,
                                             "PhotoSamplesPreset",
                                             NULL,
                                             "MinimapOrientation",
                                             "AutodriveCinematic",
                                             "SkipChampionshipDialogue",
                                             "SpawnLocation",
                                             "CarChangePosition",
                                             "DriftModes",
                                             "ToggleChoices",
                                             "PhotoChoices",
                                             "MinimapChoices",
                                             "SpawnChoices",
                                             "CarPositionChoices"};

static void release_object(void *object) {
    if (object && InterlockedDecrement((volatile LONG *)((unsigned char *)object + 0x10)) == 0) {
        ((void (*)(void *))(image + NATIVE_DELETE_OBJECT))(object);
    }
}

static unsigned property_index(const NativeProperty *property) {
    uintptr_t address = (uintptr_t)property;
    uintptr_t first = (uintptr_t)properties;
    size_t offset = address - first;
    if (address < first || offset >= sizeof(properties) || offset % sizeof(*properties)) {
        return ARRAY_COUNT(properties);
    }
    return (unsigned)(offset / sizeof(*properties));
}

static void **get_value(void **output, NativeProperty *property, void *page) {
    (void)page;
    unsigned index = property_index(property);
    *output = NULL;
    if (index >= VIDEO_SETTING_COUNT && index < ARRAY_COUNT(properties)) {
        *output = choices[index - VIDEO_SETTING_COUNT];
        if (*output) {
            InterlockedIncrement((volatile LONG *)((unsigned char *)*output + 0x10));
        }
    } else if (index == DRIFT_MODE || (index >= REALTIME_SHADOWS && index < VIDEO_SETTING_COUNT)) {
        int value = video_setting_get(index);
        box_int(output, &value);
    } else if (index < VIDEO_SETTING_COUNT) {
        float value = (index < DRIFT_SETTING_COUNT) ? video_setting_get(index) / 100.0f
                                                    : video_setting_get(index) * 0.5f;
        box_float(output, &value);
    }
    return output;
}

static void set_value(NativeProperty *property, void *page, void *boxed) {
    unsigned index = property_index(property);
    if (!boxed || index >= VIDEO_SETTING_COUNT) {
        return;
    }
    int value;
    if (index == DRIFT_MODE || index >= REALTIME_SHADOWS) {
        if (!memory_read((unsigned char *)boxed + 0x28, &value, sizeof(value))) {
            return;
        }
    } else {
        float number;
        if (!memory_read((unsigned char *)boxed + 0x28, &number, sizeof(number)) ||
            !float_is_finite(number)) {
            return;
        }
        if (index < DRIFT_SETTING_COUNT) {
            if (!(number >= 0 && number <= 1)) {
                return;
            }
            value = (int)(number * 100 + 0.5f);
        } else {
            if (!(number >= FOV_MIN_DEGREES && number <= FOV_MAX_DEGREES)) {
                return;
            }
            value = (int)(number * 2 + 0.5f);
        }
    }
    if (video_setting_set(index, value)) {
        property_changed(page, property);
    }
}

static const wchar_t *choice_text(unsigned collection, unsigned index) {
    const VideoOptionsText *text = video_options_text(image);
    static const wchar_t *const photo_samples[] = {NULL, L"256", L"512", L"1024", L"2048", L"4096"};
    switch (collection) {
        case CHOICES_DRIFT:
            return text->choices[index];
        case CHOICES_TOGGLE:
            return text->choices[index ? 0 : 2];
        case CHOICES_PHOTO:
            return index ? photo_samples[index] : text->default_choice;
        case CHOICES_MINIMAP:
            return text->minimap_choices[index];
        case CHOICES_SPAWN:
            return index ? text->spawn_choices[index - 1] : text->default_choice;
        case CHOICES_CAR_POSITION:
            return text->car_position_choices[index];
    }
    return NULL;
}

static int make_choices(unsigned collection) {
    void *type = ((TypeFactory)(image + NATIVE_COLLECTION_TYPE))(NULL);
    void *items = ((void *(*)(size_t, void *))(image + NATIVE_ALLOCATE_TYPED))(0x58, type);
    if (!items) {
        return 0;
    }
    ((ObjectInitialize)(image + NATIVE_COLLECTION_INIT))(items);
    static const unsigned vtables[] = {0x2db73f8, 0x2db7470, 0x2db74d0, 0x2db74e8, 0x2db7508};
    static const unsigned offsets[] = {0, 0x30, 0x38, 0x40, 0x48};
    for (unsigned i = 0; i < ARRAY_COUNT(vtables); ++i) {
        *(void **)((unsigned char *)items + offsets[i]) = image + vtables[i];
    }
    for (unsigned index = 0; index < choice_counts[collection]; ++index) {
        void *label = NULL;
        ((MakeString)(image + NATIVE_MAKE_STRING))(&label, choice_text(collection, index));
        if (!label) {
            release_object(items);
            return 0;
        }
        ((InsertItem)(image + NATIVE_INSERT_ITEM))(items, (int)index, label);
        release_object(label);
    }
    choices[collection] = items;
    return 1;
}

static int register_properties(unsigned page) {
    if (registered_image[page] == image) {
        return 1;
    }
    ((void (*)(void))(
        image + (page == HUD_PAGE ? NATIVE_REGISTER_HUD_TYPE : NATIVE_REGISTER_VIDEO_TYPE)))();
    if (initialized_image != image) {
        for (unsigned i = 0; i < ARRAY_COUNT(choices); ++i) {
            if (!make_choices(i)) {
                for (unsigned j = 0; j < i; ++j) {
                    release_object(choices[j]);
                    choices[j] = NULL;
                }
                return 0;
            }
        }
    }
    for (unsigned index = 0; index < ARRAY_COUNT(properties); ++index) {
        if (!property_names[index]) {
            continue;
        }
        unsigned factory = index >= VIDEO_SETTING_COUNT ? NATIVE_COLLECTION_PROPERTY
                           : index == DRIFT_MODE || index >= REALTIME_SHADOWS
                               ? NATIVE_INT_PROPERTY
                               : NATIVE_FLOAT_PROPERTY;
        NativeProperty *property = &properties[index];
        if (initialized_image != image) {
            NativeProperty *original = ((PropertyFactory)(image + factory))();
            object_initialize(property);
            property->vtable = original->vtable;
            memory_copy((unsigned char *)property + 0x28, (unsigned char *)original + 0x28, 0x30);
            property->name = property_names[index];
            property->getter = (void *)get_value;
            property->setter = index >= VIDEO_SETTING_COUNT ? NULL : (void *)set_value;
        }
        links[page][index].property = property;
        ((void (*)(void *, NativePropertyLink *))(image + NATIVE_REGISTER_PROPERTY))(
            image + (page == HUD_PAGE ? HUD_TYPE_REGISTRY : VIDEO_TYPE_REGISTRY),
            &links[page][index]);
    }
    initialized_image = image;
    registered_image[page] = image;
    log_append(page == HUD_PAGE ? "HUD: drift, FOV and extra controls added to the native HUD "
                                  "menu.\r\n"
                                : "VIDEO: graphics controls added to the native Video menu.\r\n");
    return 1;
}

static void reset_page(void *page, unsigned kind) {
    (kind == HUD_PAGE ? native_hud_reset : native_reset)(page);
    video_settings_reset_page(kind);
    if (registered_image[kind] == image) {
        for (unsigned index = 0; index < VIDEO_SETTING_COUNT; ++index) {
            if (property_names[index] && video_setting_page(index) == kind) {
                property_changed(page, &properties[index]);
            }
        }
    }
}

static void reset_options(void *page) {
    reset_page(page, VIDEO_PAGE);
}

static void reset_hud_options(void *page) {
    reset_page(page, HUD_PAGE);
}

static void *delete_resource(NativeResource *resource, unsigned flags) {
    object_cleanup(resource);
    if (flags & 1) {
        free_object(resource);
    }
    return resource;
}

static void **get_resource(void *reader, void **output) {
    native_resource(reader, output);
    NativeResource *original = *output;
    if (!original || original->length <= 0 || original->length > (int)(XAML_LIMIT / 2) ||
        !original->bytes || !video_options_is_xaml(original->bytes, original->length)) {
        return output;
    }
    NativeResource *replacement = allocate(sizeof(NativeResource) + XAML_LIMIT);
    if (!replacement) {
        return output;
    }
    char *bytes = (char *)(replacement + 1);
    size_t length =
        video_options_extend_xaml(image, original->bytes, original->length, bytes, XAML_LIMIT);
    unsigned page = video_options_page(original->bytes, original->length);
    if (!length || !register_properties(page)) {
        free_object(replacement);
        log_append("SETTINGS: menu layout did not match; original menu retained.\r\n");
        return output;
    }
    // The parser's stream owns this buffer through the native AVUI reference
    // count.
    object_initialize(replacement);
    memory_copy(resource_vtable, original->vtable - 1, sizeof(resource_vtable));
    resource_vtable[1] = (void *)delete_resource;
    replacement->vtable = resource_vtable + 1;
    replacement->bytes = bytes;
    replacement->length = (int)length;
    replacement->capacity = XAML_LIMIT;
    *output = replacement;
    release_object(original);
    return output;
}

PatchDefinition video_options_hook_patches[3] = {
    {RESOURCE_THUNK_RVA, 0, 12, GROUP_VIDEO_OPTIONS, "drift_video_resource_thunk", unused_thunk,
     resource_thunk, resource_thunk, accepted_thunk, 2},
    {RESET_SLOT_RVA, 0, 8, GROUP_VIDEO_OPTIONS, "drift_video_reset_options", reset_original,
     reset_callback, reset_callback, accepted_reset, 2},
    {0x2df07c8, 0, 8, GROUP_VIDEO_OPTIONS, "hud_reset_options", hud_reset_original,
     hud_reset_callback, hud_reset_callback, accepted_hud_reset, 2}};

void video_options_prepare_hooks(unsigned char *game_image) {
    image = game_image;
    allocate = (Allocate)(image + NATIVE_ALLOCATE);
    free_object = (Free)(image + NATIVE_FREE);
    native_resource = (GetResource)(image + NATIVE_GET_RESOURCE);
    native_reset = (ResetOptions)(image + NATIVE_RESET_VIDEO);
    native_hud_reset = (ResetOptions)(image + NATIVE_RESET_HUD);
    object_initialize = (ObjectInitialize)(image + NATIVE_OBJECT_INIT);
    object_cleanup = (ObjectInitialize)(image + NATIVE_OBJECT_CLEANUP);
    box_int = (BoxValue)(image + NATIVE_BOX_INT);
    box_float = (BoxValue)(image + NATIVE_BOX_FLOAT);
    property_changed = (PropertyChanged)(image + NATIVE_PROPERTY_CHANGED);
    static const unsigned char code[12] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0};
    memory_copy(resource_thunk, code, sizeof(code));
    uintptr_t callback = (uintptr_t)get_resource;
    memory_copy(resource_thunk + 2, &callback, 8);
    callback = (uintptr_t)native_reset;
    memory_copy(reset_original, &callback, 8);
    callback = (uintptr_t)reset_options;
    memory_copy(reset_callback, &callback, 8);
    callback = (uintptr_t)native_hud_reset;
    memory_copy(hud_reset_original, &callback, 8);
    callback = (uintptr_t)reset_hud_options;
    memory_copy(hud_reset_callback, &callback, 8);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) const wchar_t *Fh2TestSettingChoice(unsigned collection, unsigned index) {
    return collection < ARRAY_COUNT(choice_counts) && index < choice_counts[collection]
               ? choice_text(collection, index)
               : NULL;
}

__declspec(dllexport) void *Fh2TestVideoResourceHookAddress(void) {
    return (void *)get_resource;
}

__declspec(dllexport) size_t Fh2TestVideoXaml(const char *text, size_t length, char *out,
                                              size_t capacity) {
    return video_options_extend_xaml(image, text, length, out, capacity);
}

__declspec(dllexport) void *Fh2TestVideoProperty(unsigned index) {
    return index < ARRAY_COUNT(properties) && property_names[index] ? &properties[index] : NULL;
}

__declspec(dllexport) const wchar_t *Fh2TestVideoChoiceText(unsigned index) {
    return index < 3 ? choice_text(0, index) : NULL;
}
#endif
