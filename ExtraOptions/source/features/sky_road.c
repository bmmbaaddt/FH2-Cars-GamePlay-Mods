// Sky road: Ctrl+F lifts the player's car onto an endless flat asphalt floor high above the map, Ctrl+F again
// drops it back to the nearest road. The floor is the engine's own CCollWorldInfiniteFlatPlane (normally only
// used when no track is loaded), raised to SKY_HEIGHT: the open world's top collision world
// (StreamedPhysics::CompoundCollisionWorld) is wrapped, and every query box that reaches the sky height gets the
// plane's two triangles, spanning the box, appended to the answer. Car physics: car+0x4c08 is the CCarDynamics
// rigid body seen through its secondary vtable (position get/set slots 78/80, velocity 81/82, spin 87/88).
#include "features.h"
#include "logging.h"
#include "support.h"

#define SKY_HEIGHT 3000.0f            // well above the highest ground of every map
#define COMPOUND_QUERY_SLOT 0x2eb4688u // StreamedPhysics::CompoundCollisionWorld vtable 0x2eb4680, slot 1
#define COMPOUND_QUERY_RVA 0x12ccc10u
#define COMPOUND_MISSING_SLOT 0x2eb4690u // slot 2: is collision for this box still missing (not streamed)?
#define COMPOUND_MISSING_RVA 0x12d3e00u
#define STREAMED_MISSING_SLOT 0x2eb46b0u // StreamedPhysics::CollisionWorld vtable 0x2eb46a0, slot 2
#define STREAMED_MISSING_RVA 0x12d3d30u
#define HOOK_COUNT 3
#define PLANE_CONSTRUCT_RVA 0x12a9d90u
#define PLANE_SIZE 0x90
#define PLANE_MARGIN 20.0f            // the stock plane pads the query box by the same
#define PLANE_COUNT 64                // per-query copies, so concurrent queries never share vertices
#define CAR_BODY_OFFSET 0x4c08u

typedef int (*PolyQuery)(void *, const float *, unsigned, void **, void **, unsigned char);
typedef unsigned char (*MissingQuery)(void *, const float *);
typedef void (*PlaneConstruct)(void *);
typedef void *(*VectorGet)(void *, float *);
typedef void (*VectorSet)(void *, const float *);

static unsigned char *game_image;
static int sky_query(void *self, const float *box, unsigned capacity, void **polys, void **vertices,
                     unsigned char flag);
static unsigned char sky_missing_compound(void *self, const float *box);
static unsigned char sky_missing_streamed(void *self, const float *box);
static unsigned char original_callbacks[HOOK_COUNT][8], replacement_callbacks[HOOK_COUNT][8];
static const unsigned char *accepted_callbacks[HOOK_COUNT][2] = {
    {original_callbacks[0], replacement_callbacks[0]},
    {original_callbacks[1], replacement_callbacks[1]},
    {original_callbacks[2], replacement_callbacks[2]}};
PatchDefinition sky_road_hook_patches[HOOK_COUNT];

static _Alignas(16) unsigned char planes[PLANE_COUNT][PLANE_SIZE];
static int planes_ready;
static unsigned next_plane;
static volatile int sky_active;
static volatile int toggle_requested;
static volatile unsigned queries_seen, floors_added, missing_asked, missing_overridden;
static uint64_t next_report;

void sky_road_prepare_hook(unsigned char *image) {
    game_image = image;
    static const uint32_t slots[HOOK_COUNT] = {COMPOUND_QUERY_SLOT, COMPOUND_MISSING_SLOT, STREAMED_MISSING_SLOT};
    static const char *names[HOOK_COUNT] = {"sky_road_world_query", "sky_road_world_missing",
                                            "sky_road_streamed_missing"};
    const uintptr_t originals[HOOK_COUNT] = {(uintptr_t)(image + COMPOUND_QUERY_RVA),
                                             (uintptr_t)(image + COMPOUND_MISSING_RVA),
                                             (uintptr_t)(image + STREAMED_MISSING_RVA)};
    const uintptr_t replacements[HOOK_COUNT] = {(uintptr_t)sky_query, (uintptr_t)sky_missing_compound,
                                                (uintptr_t)sky_missing_streamed};
    for (int i = 0; i < HOOK_COUNT; ++i) {
        memory_copy(original_callbacks[i], &originals[i], 8);
        memory_copy(replacement_callbacks[i], &replacements[i], 8);
        sky_road_hook_patches[i] = (PatchDefinition){slots[i], 0, 8, GROUP_VEHICLE_CONTROLS, names[i],
                                                     original_callbacks[i], replacement_callbacks[i],
                                                     replacement_callbacks[i], accepted_callbacks[i], 2};
    }
}

// Over mountains, borders and outside the map there is no streamed collision at all, so the game reports it as
// still missing and holds the car until it arrives, which it never does. At sky height the floor is the ground.
// Both the top world and the streamed terrain world are asked directly, so both answer "nothing missing" up there.
static unsigned char sky_missing(uint32_t original, void *self, const float *box) {
    unsigned char missing = ((MissingQuery)(game_image + original))(self, box);
    if (sky_active && box[5] >= SKY_HEIGHT - 50.0f) {
        __atomic_add_fetch(&missing_asked, 1, __ATOMIC_RELAXED);
        if (missing) {
            __atomic_add_fetch(&missing_overridden, 1, __ATOMIC_RELAXED);
        }
        return 0;
    }
    return missing;
}

static unsigned char sky_missing_compound(void *self, const float *box) {
    return sky_missing(COMPOUND_MISSING_RVA, self, box);
}

static unsigned char sky_missing_streamed(void *self, const float *box) {
    return sky_missing(STREAMED_MISSING_RVA, self, box);
}

static int sky_query(void *self, const float *box, unsigned capacity, void **polys, void **vertices,
                     unsigned char flag) {
    int count = ((PolyQuery)(game_image + COMPOUND_QUERY_RVA))(self, box, capacity, polys, vertices, flag);
    if (!sky_active) {
        return count;
    }
    __atomic_add_fetch(&queries_seen, 1, __ATOMIC_RELAXED);
    // box: min x, y, z at +0..+8, max x, y, z at +0x10..+0x18
    if (count < 0 || (unsigned)count + 2 > capacity || box[1] > SKY_HEIGHT + 2.0f ||
        box[5] < SKY_HEIGHT - 2.0f) {
        return count;
    }
    unsigned char *plane = planes[__atomic_fetch_add(&next_plane, 1, __ATOMIC_RELAXED) % PLANE_COUNT];
    // the stock plane's corners, in its own order: (min x, max z), (max x, max z), (max x, min z), (min x, min z)
    const float x0 = box[0] - PLANE_MARGIN, x1 = box[4] + PLANE_MARGIN;
    const float z0 = box[2] - PLANE_MARGIN, z1 = box[6] + PLANE_MARGIN;
    const float corners[4][3] = {{x0, SKY_HEIGHT, z1}, {x1, SKY_HEIGHT, z1}, {x1, SKY_HEIGHT, z0}, {x0, SKY_HEIGHT, z0}};
    for (int i = 0; i < 4; ++i) {
        memory_copy(plane + 0x10 + 0x10 * i, corners[i], 12);  // +0xc of each corner stays as constructed
    }
    polys[count] = plane + 0x50;
    polys[count + 1] = plane + 0x70;
    vertices[count] = vertices[count + 1] = plane + 0x10;
    __atomic_add_fetch(&floors_added, 1, __ATOMIC_RELAXED);
    return count + 2;
}

void sky_road_toggle(void) {
    toggle_requested = 1;
}

int sky_road_active(void) {
    return sky_active;
}

static int car_body(void *car, void ***vtable_out, void **body_out) {
    // the body is a class derived from CCarDynamics (its own vtable), so the slots used here are checked
    static const struct {
        int slot;
        uint32_t rva;
    } expected[] = {{78, 0x12cd150}, {80, 0x12e27d0}, {81, 0x12cdcf0}, {82, 0x12e3cf0}, {88, 0x12dd910}};
    void *body, **vtable;
    if (!memory_read_pointer((unsigned char *)car + CAR_BODY_OFFSET, &body) || !body ||
        !memory_read_pointer(body, (void **)&vtable)) {
        return 0;
    }
    for (unsigned i = 0; i < ARRAY_COUNT(expected); ++i) {
        void *method;
        if (!memory_read_pointer(vtable + expected[i].slot, &method) ||
            (unsigned char *)method != game_image + expected[i].rva) {
            return 0;
        }
    }
    *vtable_out = vtable;
    *body_out = body;
    return 1;
}

// Game thread, before the player's car update.
static void report(void *car) {
    void **vtable, *body;
    _Alignas(16) float position[4], velocity[4];
    if (!car_body(car, &vtable, &body)) {
        return;
    }
    ((VectorGet)vtable[78])(body, position);
    ((VectorGet)vtable[81])(body, velocity);
    log_append_value("SKY ROAD: x (m) ", (int)position[0]);
    log_append_value("SKY ROAD: z (m) ", (int)position[2]);
    log_append_value("SKY ROAD: height (cm) ", (int)(position[1] * 100));
    log_append_value("SKY ROAD: speed along x (km/h) ", (int)(3.6f * velocity[0]));
    log_append_value("SKY ROAD: speed along z (km/h) ", (int)(3.6f * velocity[2]));
    log_append_value("SKY ROAD: floor answers ", (int)floors_added);
    log_append_value("SKY ROAD: missing-collision checks up here ", (int)missing_asked);
    log_append_value("SKY ROAD: of those overridden ", (int)missing_overridden);
}

void sky_road_car_tick(void *car) {
    if (sky_active && GetTickCount64() >= next_report) {
        next_report = GetTickCount64() + 2000;
        report(car);
    }
    if (!toggle_requested) {
        return;
    }
    toggle_requested = 0;
    if (sky_active) {
        sky_active = 0;
        log_append_value("SKY ROAD: off. World queries while on: ", (int)queries_seen);
        log_append_value("SKY ROAD: floor answers while on: ", (int)floors_added);
        return;
    }
    void **vtable, *body;
    if (!car_body(car, &vtable, &body)) {
        log_append("SKY ROAD: refused, the car's physics body was not recognised.\r\n");
        return;
    }
    if (!planes_ready) {
        for (int i = 0; i < PLANE_COUNT; ++i) {
            ((PlaneConstruct)(game_image + PLANE_CONSTRUCT_RVA))(planes[i]);
        }
        planes_ready = 1;
    }
    _Alignas(16) float position[4], velocity[4];
    _Alignas(16) const float still[4] = {0};
    ((VectorGet)vtable[78])(body, position);
    ((VectorGet)vtable[81])(body, velocity);
    log_append_value("SKY ROAD: car x (cm) ", (int)(position[0] * 100));
    log_append_value("SKY ROAD: car y (cm) ", (int)(position[1] * 100));
    log_append_value("SKY ROAD: car z (cm) ", (int)(position[2] * 100));
    log_append_value("SKY ROAD: car vertical speed (cm/s) ", (int)(velocity[1] * 100));
    queries_seen = floors_added = missing_asked = missing_overridden = 0;
    next_report = GetTickCount64() + 2000;
    sky_active = 1;
    position[1] = SKY_HEIGHT + 0.8f;  // set down just above the floor
    velocity[1] = 0.0f;               // keep the horizontal speed and heading
    ((VectorSet)vtable[80])(body, position);
    ((VectorSet)vtable[82])(body, velocity);
    ((VectorSet)vtable[88])(body, still);
    log_append_value("SKY ROAD: on, floor height (m) ", (int)SKY_HEIGHT);
}

#ifdef FH2_TWEAKS_TEST
__declspec(dllexport) void Fh2TestSkyRoadActivate(int active) {
    if (!planes_ready) {
        for (int i = 0; i < PLANE_COUNT; ++i) {
            ((PlaneConstruct)(game_image + PLANE_CONSTRUCT_RVA))(planes[i]);
        }
        planes_ready = 1;
    }
    sky_active = active;
}
#endif
