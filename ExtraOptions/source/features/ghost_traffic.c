// Drive through every other car. Each car's collidable component works out its car-to-car collision from
// the preset requests on it (collidablepresets.xml); the last request with a collision value wins, and the
// result goes to the physics as a "collision disabled" flag at component+0x54. Free-roam AI only collides
// because of its ai_ow request: Ctrl+G's fade-out drops it, which is why fading cars can be driven through.
// The flag computation is replaced with a call that keeps the game's answer for the local player's car
// (its base preset is default_local) and disables collision for every other car. Visibility is unchanged.
#include "features.h"
#include "support.h"

#define RESOLVE_FLAG_RVA 0xf7b9dau  // xor eax,eax; cmp esi,1; setne al; mov [rdi+0x54],eax
#define CALLBACK_SLOT_RVA 0xf7b0f8u // int3 padding after a function, holds the callback pointer

static int collision_flag(unsigned char *component, int collision);
static const unsigned char original_code[11] = {0x33, 0xc0, 0x83, 0xfe, 0x01, 0x0f, 0x95, 0xc0, 0x89, 0x47, 0x54};
// mov rcx,rdi; mov edx,esi; call [rip+slot]   (slot - next instruction = 0xf7b0f8 - 0xf7b9e5)
static const unsigned char hook_code[11] = {0x48, 0x89, 0xf9, 0x89, 0xf2, 0xff, 0x15, 0x13, 0xf7, 0xff, 0xff};
static const unsigned char unused_slot[8] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static unsigned char callback_slot[8];
static const unsigned char *code_wanted, *slot_wanted;
static const unsigned char *accepted_code[] = {original_code, hook_code};
static const unsigned char *accepted_slot[2] = {unused_slot, callback_slot};
PatchDefinition ghost_traffic_patches[2];

void ghost_traffic_prepare_patch(const ModOptions *options) {
    uintptr_t callback = (uintptr_t)collision_flag;
    memory_copy(callback_slot, &callback, 8);
    code_wanted = options->ghost_traffic ? hook_code : original_code;
    slot_wanted = options->ghost_traffic ? callback_slot : unused_slot;
    ghost_traffic_patches[0] = (PatchDefinition){CALLBACK_SLOT_RVA, 0, 8, GROUP_VEHICLE_CONTROLS,
                                                 "ghost_traffic_callback", unused_slot, slot_wanted,
                                                 slot_wanted, accepted_slot, 2};
    ghost_traffic_patches[1] = (PatchDefinition){RESOLVE_FLAG_RVA, 0, 11, GROUP_VEHICLE_CONTROLS,
                                                 "ghost_traffic_collision_flag", original_code, code_wanted,
                                                 code_wanted, accepted_code, 2};
}

static int is_local_player(unsigned char *component) {
    // component+0x18: base request -> +0x10 preset record -> +0x20 std::string name
    unsigned char *request, *preset;
    size_t size;
    char name[16];
    if (!memory_read_pointer(component + 0x18, (void **)&request) || !request ||
        !memory_read_pointer(request + 0x10, (void **)&preset) || !preset ||
        !memory_read(preset + 0x30, &size, sizeof(size)) || size != 13 ||
        !memory_read(preset + 0x20, name, 13)) {  // 13 chars fit the string's inline buffer
        return 0;
    }
    name[13] = 0;
    return text_equal_ignore_case(name, "default_local");
}

// collision: 1 = on, anything else = off. Returns and stores the game's "collision disabled" flag.
static int collision_flag(unsigned char *component, int collision) {
    int disabled = collision != 1 || !is_local_player(component);
    *(int *)(component + 0x54) = disabled;
    return disabled;
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) int Fh2TestGhostTrafficFlag(unsigned char *component, int collision) {
    return collision_flag(component, collision);
}
#endif
