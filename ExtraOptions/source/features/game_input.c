#include "game_input.h"
#include "autodrive_game.h"
#include "logging.h"

typedef SHORT(WINAPI *KeyState)(int);
typedef HWND(WINAPI *ForegroundWindow)(void);
typedef DWORD(WINAPI *WindowProcess)(HWND, LPDWORD);

static KeyState key_state;
static ForegroundWindow foreground_window;
static WindowProcess window_process;
static int input_initialized;

#ifdef FH2_TWEAKS_TEST
static int use_test_input;
static KeyboardInput test_input;
#endif

static int initialize_input(void) {
    if (!input_initialized) {
        input_initialized = 1;
        HMODULE module = LoadLibraryExW(L"user32.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (module) {
            key_state = (KeyState)(uintptr_t)GetProcAddress(module, "GetAsyncKeyState");
            foreground_window =
                (ForegroundWindow)(uintptr_t)GetProcAddress(module, "GetForegroundWindow");
            window_process =
                (WindowProcess)(uintptr_t)GetProcAddress(module, "GetWindowThreadProcessId");
        }
        if (!key_state || !foreground_window || !window_process) {
            log_append("AUTODRIVE: keyboard API unavailable; F2 is inactive.\r\n");
        }
    }
    return key_state && foreground_window && window_process;
}

int autodrive_game_window_focused(void) {
#ifdef FH2_TWEAKS_TEST
    if (use_test_input) {
        return test_input.focused;
    }
#endif
    if (!initialize_input()) {
        return 0;
    }
    HWND window = foreground_window();
    DWORD process_id = 0;
    if (window) {
        window_process(window, &process_id);
    }
    return process_id == GetCurrentProcessId();
}

KeyboardInput game_input_read(void) {
#ifdef FH2_TWEAKS_TEST
    if (use_test_input) {
        return test_input;
    }
#endif
    KeyboardInput input = {0};
    if (initialize_input()) {
        input.focused = autodrive_game_window_focused();
        input.toggle = (key_state(VK_F2) & 0x8000) != 0;
        input.cancel = (key_state(VK_ESCAPE) & 0x8000) != 0;
    }
    return input;
}

int game_input_key_down(int key) {
    return key && initialize_input() && (key_state(key) & 0x8000) != 0;
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) void Fh2TestAutodriveInput(int f2, int escape, int focus) {
    use_test_input = 1;
    test_input = (KeyboardInput){f2, escape, focus};
}
#endif
