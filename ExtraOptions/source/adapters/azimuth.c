#include "azimuth_proxy.h"
#include "azimuth_host.h"
#include "logging.h"
#include "runtime.h"
#include "settings.h"
#include "sha256.h"
#include "support.h"

#define LOAD_LIBRARY_IAT 0x2853c0u
#define TLS_LOADER_RETURN 0xcbb0cu

typedef HMODULE(WINAPI *LoadLibraryFunction)(LPCWSTR, HANDLE, DWORD);
typedef int(WINAPI *MessageBoxFunction)(HWND, LPCWSTR, LPCWSTR, UINT);

static HINSTANCE mod_instance;
static unsigned char *host_image;
static LoadLibraryFunction original_load_library;
static void *volatile *load_library_slot;
static volatile LONG initialized;
static wchar_t mod_directory[PATH_CAPACITY], game_path[PATH_CAPACITY];
static wchar_t launcher_directory[PATH_CAPACITY], path_buffer[PATH_CAPACITY];
static wchar_t game_folder[PATH_CAPACITY];
static wchar_t filename[PATH_CAPACITY];
static unsigned char hash_buffer[16384];
static int notifications;

static const unsigned char supported_host_hash[32] = {
    0x78, 0xa5, 0xf6, 0x35, 0xb3, 0x24, 0x19, 0x18, 0x70, 0x7b, 0x5a, 0xcd, 0xde, 0xaa, 0x96, 0xbd,
    0x59, 0xcc, 0xb3, 0x81, 0x94, 0x90, 0xe9, 0x21, 0x8e, 0xe1, 0x52, 0x49, 0x7c, 0xa7, 0x36, 0x39};

static void notify_refusal(void) {
    ++notifications;
#ifndef FH2_TWEAKS_TEST
    HMODULE user32 = LoadLibraryExW(L"user32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    MessageBoxFunction show =
        user32 ? (MessageBoxFunction)(uintptr_t)GetProcAddress(user32, "MessageBoxW") : NULL;
    if (show) {
        wchar_t message[1024];
        int length =
            MultiByteToWideChar(CP_UTF8, 0, status_get(), -1, message, ARRAY_COUNT(message));
        if (length) {
            show(NULL, message, L"Forza Horizon 2 Extra Options", MB_OK | MB_ICONWARNING);
        }
    }
    if (user32) {
        FreeLibrary(user32);
    }
#endif
}

static int refuse(const char *reason) {
    status_set(reason);
    ModOptions options;
    settings_defaults(&options);
    log_start(mod_directory, &options);
    notify_refusal();
    return 0;
}

static int verify_host_file(const wchar_t *path) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    LARGE_INTEGER size;
    Sha256Context context;
    unsigned char digest[32];
    DWORD count;
    uint64_t total = 0;
    int valid = GetFileSizeEx(file, &size) && size.QuadPart == 3593728;
    sha256_initialize(&context);
    while (valid && total < 3593728) {
        valid = ReadFile(file, hash_buffer, sizeof(hash_buffer), &count, NULL) && count;
        if (valid) {
            sha256_update(&context, hash_buffer, count);
            total += count;
        }
    }
    CloseHandle(file);
    sha256_finalize(&context, digest);
    return valid && total == 3593728 && memory_equal(digest, supported_host_hash, 32);
}

// Another Extra Options build behind Azimuth's TLS helper (ForzaInfinity, an older Extra Options, either
// directly or as azimuth-tls.dll.original behind a loader) would patch the game a second time.
static int tls_helper_has_extra_options(const wchar_t *name) {
    static const char marker[] = "FH2_ExtraOptions";
    enum { MARKER = sizeof(marker) - 1 };
    if (!path_join(path_buffer, launcher_directory, name)) {
        return 0;
    }
    HANDLE file = CreateFileW(path_buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD kept = 0, count;
    int found = 0;
    while (!found && ReadFile(file, hash_buffer + kept, sizeof(hash_buffer) - kept, &count, NULL) &&
           count) {
        DWORD size = kept + count;
        for (DWORD at = 0; !found && at + MARKER <= size; ++at) {
            found = memory_equal(hash_buffer + at, marker, MARKER);
        }
        kept = size < MARKER ? size : MARKER - 1;  // a marker split across two reads
        memory_copy(hash_buffer, hash_buffer + size - kept, kept);
    }
    CloseHandle(file);
    return found;
}

static int directory_from_module(HMODULE module, wchar_t *directory) {
    DWORD count = GetModuleFileNameW(module, directory, PATH_CAPACITY);
    if (!count || count >= PATH_CAPACITY) {
        return 0;
    }
    while (count && directory[count - 1] != L'\\' && directory[count - 1] != L'/') {
        --count;
    }
    directory[count] = 0;
    return count != 0;
}

static int resolve_game_path(void) {
    if (!path_join(path_buffer, launcher_directory, L"data\\azimuth.ini")) {
        return 0;
    }
    wchar_t selected[32];
    GetPrivateProfileStringW(L"launcher", L"game", L"1a96545b", selected, ARRAY_COUNT(selected),
                             path_buffer);
    if (lstrcmpiW(selected, L"1a96545b")) {
        status_set("INACTIVE: the selected Azimuth title is not Forza Horizon 2.");
        return -1;
    }
    DWORD count = GetPrivateProfileStringW(L"title-1a96545b", L"folder", L"", game_folder,
                                           PATH_CAPACITY, path_buffer);
    if (!count) {
        count = GetPrivateProfileStringW(L"launcher", L"game_folder", L"", game_folder,
                                         PATH_CAPACITY, path_buffer);
    }
    if (!count || count >= PATH_CAPACITY - 1) {
        return 0;
    }
    const wchar_t *folder = game_folder;
    if (!((game_folder[0] && game_folder[1] == L':') || game_folder[0] == L'\\' ||
          game_folder[0] == L'/')) {
        if (!path_join(path_buffer, launcher_directory, game_folder)) {
            return 0;
        }
        folder = path_buffer;
    }
    count = GetFullPathNameW(folder, PATH_CAPACITY, game_path, NULL);
    return count && count < PATH_CAPACITY && path_join(game_folder, game_path, L"Mount") &&
           path_join(game_path, game_folder, GAME_EXE_NAME);
}

static int restore_loader_import(void) {
    DWORD protection;
    if (!VirtualProtect((void *)load_library_slot, sizeof(void *), PAGE_READWRITE, &protection)) {
        return 0;
    }
    void *previous = InterlockedCompareExchangePointer(load_library_slot,
                                                       (void *)(uintptr_t)original_load_library,
                                                       (void *)(uintptr_t)azimuth_load_library);
    DWORD ignored;
    int restored = VirtualProtect((void *)load_library_slot, sizeof(void *), protection, &ignored);
    return previous == (void *)(uintptr_t)azimuth_load_library && restored;
}

static int game_image_is_readable(unsigned char *base) {
    uintptr_t cursor = (uintptr_t)base, end = cursor + GAME_IMAGE_SIZE;
    if (end < cursor) {
        return 0;
    }
    while (cursor < end) {
        MEMORY_BASIC_INFORMATION region;
        if (VirtualQuery((void *)cursor, &region, sizeof(region)) != sizeof(region) ||
            region.State != MEM_COMMIT || region.AllocationBase != base ||
            (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            return 0;
        }
        DWORD protection = region.Protect & 0xff;
        if (protection != PAGE_READONLY && protection != PAGE_READWRITE &&
            protection != PAGE_WRITECOPY && protection != PAGE_EXECUTE_READ &&
            protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY) {
            return 0;
        }
        uintptr_t next = (uintptr_t)region.BaseAddress + region.RegionSize;
        if (next <= cursor) {
            return 0;
        }
        cursor = next;
    }
    return 1;
}

HMODULE WINAPI azimuth_on_load_library(LPCWSTR path, HANDLE file, DWORD flags, void *context) {
    void *caller = __builtin_return_address(0);
    HMODULE module = original_load_library(path, file, flags);
    DWORD error = GetLastError();
    if (caller == host_image + TLS_LOADER_RETURN) {
        if (!restore_loader_import()) {
            refuse("REFUSED: could not restore Azimuth's loader import. No game patches applied.");
        } else if (module) {
            unsigned char *game = NULL;
            IMAGE_DOS_HEADER dos;
            IMAGE_NT_HEADERS64 nt;
            if (!memory_read_pointer(context, (void **)&game) ||
                !memory_read(game, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
                dos.e_lfanew < 64 || dos.e_lfanew > 4096 ||
                !memory_read(game + dos.e_lfanew, &nt, sizeof(nt)) ||
                nt.Signature != IMAGE_NT_SIGNATURE ||
                nt.FileHeader.TimeDateStamp != GAME_TIMESTAMP ||
                nt.OptionalHeader.SizeOfImage != GAME_IMAGE_SIZE || !game_image_is_readable(game)) {
                refuse(
                    "REFUSED: Azimuth's mapped game does not match FH2. No game patches applied.");
            } else if (runtime_initialize(game, game_path, mod_directory)) {
                log_append("AZIMUTH: winmm proxy initialized; original TLS helper retained.\r\n");
            } else {
                notify_refusal();
            }
        }
    }
    SetLastError(error);
    return module;
}

static int arm_loader(unsigned char *host, const wchar_t *host_path) {
    if (!verify_host_file(host_path) || azimuth_host_build(host) != AZIMUTH_0910) {
        return refuse("REFUSED: unsupported Azimuth build. This mod requires the exact Azimuth "
                      "0.9.10 executable (SHA-256 "
                      "78a5f635b3241918707b5acddeaa96bd59ccb3819490e9218ee152497ca73639). "
                      "Extra Options is disabled; no game or compatibility patches were applied. "
                      "Install the matching mod release for your Azimuth version.");
    }
    static const unsigned char loader[] = {0x40, 0x57, 0x48, 0x83, 0xec, 0x40, 0x48, 0x8b, 0xf9};
    static const unsigned char call[] = {0xff, 0x15, 0xb4, 0x98, 0x1b, 0x00};
    if (!memory_equal(host + 0xcba60, loader, sizeof(loader)) ||
        !memory_equal(host + TLS_LOADER_RETURN - sizeof(call), call, sizeof(call))) {
        return refuse("REFUSED: Azimuth's TLS loading code has changed. No patches applied.");
    }
    host_image = host;
    load_library_slot = (void *volatile *)(host + LOAD_LIBRARY_IAT);
    original_load_library = LoadLibraryExW;
    if (*load_library_slot != (void *)(uintptr_t)original_load_library) {
        return refuse(
            "REFUSED: another mod owns Azimuth's LoadLibraryExW import. No patches applied.");
    }
    DWORD protection;
    if (!VirtualProtect((void *)load_library_slot, sizeof(void *), PAGE_READWRITE, &protection)) {
        return refuse("REFUSED: Azimuth's loader import is inaccessible. No patches applied.");
    }
    void *previous = InterlockedCompareExchangePointer(load_library_slot,
                                                       (void *)(uintptr_t)azimuth_load_library,
                                                       (void *)(uintptr_t)original_load_library);
    DWORD ignored;
    int restored = VirtualProtect((void *)load_library_slot, sizeof(void *), protection, &ignored);
    if (previous != (void *)(uintptr_t)original_load_library || !restored) {
        if (previous == (void *)(uintptr_t)original_load_library) {
            DWORD writable;
            if (VirtualProtect((void *)load_library_slot, sizeof(void *), PAGE_READWRITE,
                               &writable)) {
                InterlockedCompareExchangePointer(load_library_slot,
                                                  (void *)(uintptr_t)original_load_library,
                                                  (void *)(uintptr_t)azimuth_load_library);
                VirtualProtect((void *)load_library_slot, sizeof(void *), protection, &ignored);
            }
        }
        return refuse(
            "REFUSED: Azimuth's loader import could not be installed. No game patches applied.");
    }
    status_set("Waiting for Azimuth to load its original TLS helper.");
    return 1;
}

void azimuth_proxy_start(void *caller) {
    if (InterlockedCompareExchange(&initialized, 1, 0)) {
        return;
    }
    DWORD length = GetModuleFileNameW(NULL, filename, PATH_CAPACITY);
    if (!length || length >= PATH_CAPACITY) {
        return;
    }
    const wchar_t *name = filename;
    for (const wchar_t *p = filename; *p; ++p) {
        if (*p == L'\\' || *p == L'/') {
            name = p + 1;
        }
    }
    if (lstrcmpiW(name, L"azimuth.exe")) {
        return;
    }
    unsigned char *host = (unsigned char *)GetModuleHandleW(NULL);
    if (!directory_from_module(mod_instance, mod_directory) ||
        !directory_from_module(NULL, launcher_directory)) {
        refuse("REFUSED: cannot resolve the mod and launcher paths. No patches applied.");
        return;
    }
    if (tls_helper_has_extra_options(L"data\\bin\\azimuth-tls.dll") ||
        tls_helper_has_extra_options(L"data\\bin\\azimuth-tls.dll.original")) {
        status_set("INACTIVE: another Extra Options build (ForzaInfinity or an older Extra Options) "
                   "is installed in data\\bin, so this one stays off. No patches applied.");
        ModOptions options;
        settings_defaults(&options);
        log_start(mod_directory, &options);
        return;
    }
    if (!verify_host_file(filename)) {
        arm_loader(host, filename);
        return;
    }
    if (caller != host + 0x78e90) {
        refuse("REFUSED: unexpected Azimuth initialization order. No patches applied.");
        return;
    }
    int resolved = resolve_game_path();
    if (resolved < 0) {
        return;
    }
    if (!resolved) {
        refuse("REFUSED: cannot resolve the FH2 game folder from data/azimuth.ini. No patches "
               "applied.");
        return;
    }
    arm_loader(host, filename);
}

void adapter_process_attach(HINSTANCE instance) {
    mod_instance = instance;
    DisableThreadLibraryCalls(instance);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) int Fh2TestArmAzimuth(void *host, const wchar_t *host_path,
                                            const wchar_t *directory, const wchar_t *game) {
    lstrcpyW(mod_directory, directory);
    lstrcpyW(game_path, game);
    return arm_loader(host, host_path);
}

__declspec(dllexport) int Fh2TestAzimuthNotifications(void) {
    return notifications;
}

__declspec(dllexport) int Fh2TestAzimuthGamePath(const wchar_t *launcher, wchar_t *output) {
    lstrcpyW(launcher_directory, launcher);
    int result = resolve_game_path();
    if (result == 1) {
        lstrcpyW(output, game_path);
    }
    return result;
}
#endif
