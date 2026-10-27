// "Random car" on the garage car list (My Cars, from the main menu or the pause menu): a right stick
// click or R on the keyboard moves the list to a random car of the garage, from the page's own
// per-frame update, so the list is only touched on the game's UI thread. A then gets in it as usual.
// Jump to Manufacturer already hooks this page's scene-loaded and update slots, so its hooks call
// random_car_garage_loaded and random_car_garage_update after the game's own.
#include "features.h"
#include "autodrive_game.h"
#include "game_input.h"
#include "support.h"

#define LIST_OFFSET 0xa08u          // the ItemsSelector named "List" (A_CarSelect.xaml)
#define ITEMS_OFFSET 0x1f8u         // ItemsControl items; slot 0xb8 / 8 of its vtable is the count
#define RIGHT_STICK_BUTTON 0x80
#define HELP_RIGHT_STICK 20         // ControllerButtonPanel RThumbDescription

typedef void (*PageAction)(void *);
typedef void **(*MakeString)(void **, const wchar_t *);

static unsigned char *game_image;
static int was_down;
static uint64_t seed;

static void release_object(void *object) {
    if (object && InterlockedDecrement((volatile LONG *)((unsigned char *)object + 0x10)) == 0) {
        ((PageAction)(game_image + 0x257730))(object);
    }
}

void random_car_garage_loaded(void *page) {
    was_down = 1;  // a button held while the page opens does not count
    void *panel = ((void *(*)(void *))(game_image + 0xced910))(page), *label = NULL;
    ((MakeString)(game_image + 0x13fe8e0))(&label, L"Random car");
    if (panel && label) {
        ((void (*)(void *, int, void **))(game_image + 0xb00010))(panel, HELP_RIGHT_STICK, &label);
        label = NULL;  // the panel took it
    }
    release_object(label);
}

static void pick_random_car(void *page) {
    void *list, *items, **vtable, *count_function;
    if (!memory_read_pointer((unsigned char *)page + LIST_OFFSET, &list) || !list ||
        !memory_read_pointer((unsigned char *)list + ITEMS_OFFSET, &items) || !items ||
        !memory_read_pointer(items, (void **)&vtable) ||
        !memory_read_pointer(vtable + 0xb8 / 8, &count_function)) {
        return;
    }
    int count = ((int (*)(void *))count_function)(items);
    if (count < 2) {
        return;
    }
    if (!seed) {
        seed = GetTickCount64() * 0x9e3779b97f4a7c15ull | 1;
    }
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    ((void (*)(void *, int))(game_image + 0xbafe20))(list, (int)(seed % (uint64_t)count));
}

void random_car_garage_update(void *page) {
    void *controller;
    unsigned short buttons;
    int down = (autodrive_game_buttons(&controller, &buttons) == 0 && (buttons & RIGHT_STICK_BUTTON)) ||
               (autodrive_game_window_focused() && game_key_down('R'));
    if (down && !was_down) {
        pick_random_car(page);
    }
    was_down = down;
}

void random_car_bind(unsigned char *image) {
    game_image = image;
}
