#include "features.h"
#include "stock_wheel_assets.h"
#include "support.h"

typedef struct {
    const char *data;
    uintptr_t reserved;
    size_t length, capacity;
} GameString;

typedef GameString *(*AssignText)(GameString *, const char *, size_t);
typedef GameString *(*ConcatStrings)(GameString *, const GameString *, const GameString *);
typedef GameString *(*ConcatText)(GameString *, const char *, const GameString *);
typedef GameString *(*AssignString)(GameString *, const GameString *, size_t, size_t);
typedef int (*CompareStrings)(const GameString *, const GameString *);
typedef unsigned char (*LoadWheels)(void *, const GameString *);
typedef unsigned char (*ReplaceModels)(void *, const GameString *, const GameString *);
typedef unsigned char (*FileExists)(void *, const GameString *);
typedef unsigned char (*ModelsReady)(void *);

static unsigned char *game_image;
static AssignText assign_text;
static ConcatStrings concat_strings;
static ConcatText concat_text;
static AssignString assign_string;
static CompareStrings compare_strings;
static LoadWheels original_load;
static ReplaceModels replace_models;
static unsigned char thunks[3][12];
static const unsigned char unused[12] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
                                         0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static const unsigned char *accepted[3][2];
PatchDefinition stock_wheels_hook_patches[3];

static const char *string_data(const GameString *string) {
    return string->capacity < 16 ? (const char *)string : string->data;
}

static GameString borrowed_string(const char *text) {
    size_t length = text_length(text);
    return (GameString){text, 0, length, length < 16 ? 16 : length};
}

static const StockWheelAssets *find_wheel(const GameString *name) {
    const char *text = string_data(name);
    for (unsigned index = 0; index < ARRAY_COUNT(stock_wheels); ++index) {
        if (text_equal_ignore_case(text, stock_wheels[index].name)) {
            return &stock_wheels[index];
        }
    }
    return 0;
}

static GameString *create_archive_path(GameString *output, const StockWheelAssets *wheel) {
    *output = (GameString){.capacity = 15};
    return assign_text(output, wheel->archive, text_length(wheel->archive));
}

static GameString *stock_archive_text(GameString *output, const char *prefix,
                                      const GameString *name) {
    const StockWheelAssets *wheel = find_wheel(name);
    return wheel ? create_archive_path(output, wheel) : concat_text(output, prefix, name);
}

static GameString *stock_archive_string(GameString *output, const GameString *prefix,
                                        const GameString *name) {
    const StockWheelAssets *wheel = find_wheel(name);
    return wheel ? create_archive_path(output, wheel) : concat_strings(output, prefix, name);
}

static int file_exists(void *filesystem, const GameString *path) {
    return path->length && ((FileExists)(*(void ***)filesystem)[0xe8 / 8])(filesystem, path);
}

static unsigned char models_ready(void *part) {
    return ((ModelsReady)(*(void ***)part)[0x10 / 8])(part);
}

static unsigned char load_stock_wheels(void *part, const GameString *name) {
    const StockWheelAssets *wheel = find_wheel(name);
    GameString *current = (GameString *)((unsigned char *)part + 0x198);
    if (!wheel || !compare_strings(current, name)) {
        return original_load(part, name);
    }
    void *filesystem = *(void **)(game_image + 0x36614f0);
    if (!filesystem) {
        return models_ready(part);
    }
    GameString models[4], textures[4];
    int textures_available = 1;
    for (unsigned index = 0; index < ARRAY_COUNT(models); ++index) {
        models[index] = borrowed_string(wheel->models[index]);
        if (!file_exists(filesystem, &models[index])) {
            return models_ready(part);
        }
        textures[index] = borrowed_string(wheel->textures[index]);
        if (!file_exists(filesystem, &textures[index])) {
            textures_available = 0;
        }
    }
    assign_string(current, name, 0, (size_t)-1);
    *((unsigned char *)part + 0x190) = !textures_available;
    // The native replacement copies paths and owns the loaded model resources.
    return replace_models(part, models, textures);
}

void stock_wheels_prepare_hooks(unsigned char *image) {
    game_image = image;
    assign_text = (AssignText)(image + 0x22c300);
    concat_strings = (ConcatStrings)(image + 0x211e10);
    concat_text = (ConcatText)(image + 0x2b3750);
    assign_string = (AssignString)(image + 0x22c1d0);
    compare_strings = (CompareStrings)(image + 0x22c7e0);
    original_load = (LoadWheels)(image + 0x11b0fb0);
    replace_models = (ReplaceModels)(image + 0x11b9da0);
    const uint32_t locations[] = {0xecd6a4, 0xecde31, 0xecfc22};
    const char *names[] = {"stock_wheel_archive_text_thunk", "stock_wheel_archive_string_thunk",
                           "stock_wheel_models_thunk"};
    const uintptr_t callbacks[] = {(uintptr_t)stock_archive_text, (uintptr_t)stock_archive_string,
                                   (uintptr_t)load_stock_wheels};
    const unsigned char jump[] = {0x48, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xe0};
    for (unsigned index = 0; index < ARRAY_COUNT(thunks); ++index) {
        memory_copy(thunks[index], jump, sizeof(jump));
        memory_copy(thunks[index] + 2, &callbacks[index], sizeof(callbacks[index]));
        accepted[index][0] = unused;
        accepted[index][1] = thunks[index];
        stock_wheels_hook_patches[index] = (PatchDefinition){
            locations[index], 0, 12, GROUP_RIMS, names[index], unused, thunks[index], thunks[index],
            accepted[index],  2};
    }
}
