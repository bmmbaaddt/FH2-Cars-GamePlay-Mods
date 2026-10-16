#include "features.h"
#include "logging.h"
#include "support.h"

typedef struct {
    const char *data;
    uintptr_t reserved;
    size_t length, capacity;
} BorrowedGameString;

typedef unsigned char (*ResolveFile)(void *, void **, void *, const void *, unsigned char,
                                     unsigned char, unsigned char, void **, unsigned char);
typedef void *(*Allocate)(size_t);
typedef void *(*ConstructVolume)(void *, const void *, unsigned char);
typedef void (*Reference)(void *);
typedef unsigned char (*MountVolume)(void *, const void *, void **, unsigned, unsigned char,
                                     unsigned char, const void *);
typedef void (*SortVolumes)(void *);
typedef unsigned char (*FileExists)(void *, const BorrowedGameString *);

static unsigned char *image;
static ResolveFile original_resolve;
static Allocate allocate;
static ConstructVolume construct_volume;
static FileExists original_exists;
static void *folder_vtable[1 + 0x1f0 / sizeof(void *)];
static volatile LONG mount_state;
static unsigned char original_pointer[8], callback_pointer[8];
static const unsigned char *const accepted[] = {original_pointer, callback_pointer};

static BorrowedGameString borrowed_string(const char *text) {
    return (BorrowedGameString){text, 0, text_length(text), 31};
}

static unsigned char folder_file_exists(void *volume, const BorrowedGameString *path) {
    const char *text = path->capacity < 16 ? (const char *)path : path->data;
    // Archive selectors such as <*>0 are virtual paths, not disk filenames.
    for (size_t index = 0; index < path->length; ++index) {
        unsigned char character = (unsigned char)text[index];
        if (character < 32 || character == '<' || character == '>' || character == '*' ||
            character == '?' || character == '"' || character == '|') {
            return 0;
        }
    }
    return original_exists(volume, path);
}

static int mount_folder(void *filesystem, const char *folder, unsigned priority) {
    void *storage = allocate(0x50);
    if (!storage) {
        return 0;
    }
    BorrowedGameString source = borrowed_string(folder);
    BorrowedGameString destination = borrowed_string("game:\\media\\");
    void *volume = construct_volume(storage, &source, 1);
    *(void ***)volume = folder_vtable + 1;
    void **methods = *(void ***)volume;
    ((Reference)methods[0x10 / 8])(volume);
    void **filesystem_methods = *(void ***)filesystem;
    // Native mounts retain the volume. Do not enable hash-manifest filtering.
    int mounted = ((MountVolume)filesystem_methods[0x1e8 / 8])(filesystem, &destination, &volume,
                                                               priority, 0, 0, 0);
    ((Reference)methods[0x18 / 8])(volume);
    return mounted;
}

static void ensure_folders_mounted(void *filesystem) {
    void *root;
    if (!memory_read((unsigned char *)filesystem + 0x68, &root, sizeof(root)) || !root ||
        InterlockedCompareExchange(&mount_state, 1, 0) != 0) {
        return;
    }
    int override = mount_folder(filesystem, "game:\\mediaoverride\\", 0x7ffffffe);
    int pc = mount_folder(filesystem, "game:\\mediapc\\", 0x7ffffffd);
    void **methods = *(void ***)filesystem;
    ((SortVolumes)methods[0x1f8 / 8])(filesystem);
    InterlockedExchange(&mount_state, 2);
    log_append(override && pc ? "FILES: mediaoverride > mediapc > original "
                                "assets; native folder mounts ready.\r\n"
                              : "FILES: an override mount failed; original asset "
                                "lookup remains available.\r\n");
}

static unsigned char resolve_file(void *filesystem, void **volume, void *relative_path,
                                  const void *path, unsigned char flag1, unsigned char flag2,
                                  unsigned char flag3, void **mount, unsigned char flag4) {
    ensure_folders_mounted(filesystem);
    return original_resolve(filesystem, volume, relative_path, path, flag1, flag2, flag3, mount,
                            flag4);
}

PatchDefinition media_overrides_hook_patch = {0x2e6ef28,
                                              0,
                                              8,
                                              GROUP_MEDIA_OVERRIDES,
                                              "media_override_native_resolver",
                                              original_pointer,
                                              callback_pointer,
                                              callback_pointer,
                                              accepted,
                                              2};

void media_overrides_prepare_hook(unsigned char *game_image) {
    if (image != game_image) {
        InterlockedExchange(&mount_state, 0);
    }
    image = game_image;
    original_resolve = (ResolveFile)(image + 0x11f49b0);
    allocate = (Allocate)(image + 0x3229c0);
    construct_volume = (ConstructVolume)(image + 0x11e05a0);
    // Preserve the native RTTI locator preceding the method table.
    memory_copy(folder_vtable, image + 0x2eab910, sizeof(folder_vtable));
    original_exists = (FileExists)folder_vtable[1 + 0xe8 / 8];
    folder_vtable[1 + 0xe8 / 8] = (void *)folder_file_exists;
    uintptr_t original = (uintptr_t)original_resolve, callback = (uintptr_t)resolve_file;
    memory_copy(original_pointer, &original, sizeof(original));
    memory_copy(callback_pointer, &callback, sizeof(callback));
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) void *Fh2TestMediaOverridesHookAddress(void) {
    return (void *)resolve_file;
}

__declspec(dllexport) void Fh2TestMediaOverridesReset(void) {
    InterlockedExchange(&mount_state, 0);
}
#endif
