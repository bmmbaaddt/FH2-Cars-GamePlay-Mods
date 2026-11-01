// Traffic cars for sale (FH2 Mods' "traffic and AI cars for sale" pack). The Autoshow and every car list read
// the Drivable_Data_Car view, which the game creates without the cars in TrafficCars. The pack used to empty
// TrafficCars, but free roam's traffic reloads from it (after fast travel, for one) and then waits forever
// for a slot it can fill. Now the pack keeps TrafficCars and puts the cars it sells in Data_Car_Buckets, and
// the view leaves out only the traffic cars no dealer bucket has. No stock traffic car has a bucket, so
// without the pack the view lists exactly the same cars.
#include "features.h"

#define VIEW_LENGTH_RVA 0x21cbf9u // mov r8d,0x70: the statement is copied into a std::string by its fixed length
#define VIEW_LOAD_RVA 0x21cbffu  // lea rdx,[rip+"CREATE VIEW Drivable_Data_Car ..."] before the statement runs
#define VIEW_TEXT_RVA 0x2c46f40u // .rdata's unused tail, zero up to .data at 0x2c47000

#define VIEW_SQL "CREATE VIEW Drivable_Data_Car AS SELECT*FROM Data_Car WHERE Id NOT IN" \
                 "(SELECT CarId FROM TrafficCars EXCEPT SELECT CarId FROM Data_Car_Buckets)"
static const unsigned char view_text[sizeof(VIEW_SQL)] = VIEW_SQL;
static const unsigned char unused_text[sizeof(VIEW_SQL)];
static const unsigned char original_load[7] = {0x48, 0x8d, 0x15, 0x1a, 0xf2, 0xbd, 0x01};
// the same lea, its target moved to VIEW_TEXT_RVA (0x2c46f40 - 0x21cc06 = 0x2a2a33a)
static const unsigned char new_load[7] = {0x48, 0x8d, 0x15, 0x3a, 0xa3, 0xa2, 0x02};
static const unsigned char original_length[6] = {0x41, 0xb8, 0x70, 0x00, 0x00, 0x00};
static const unsigned char new_length[6] = {0x41, 0xb8, sizeof(VIEW_SQL) - 1, 0x00, 0x00, 0x00};
_Static_assert(sizeof(VIEW_SQL) - 1 < 0x100, "the length is patched as one byte");
static const unsigned char *accepted_text[] = {unused_text, view_text};
static const unsigned char *accepted_length[] = {original_length, new_length};
static const unsigned char *accepted_load[] = {original_load, new_load};
PatchDefinition dealer_traffic_patches[3] = {
    {VIEW_TEXT_RVA, 0, sizeof(VIEW_SQL), GROUP_VEHICLE_CONTROLS, "dealer_traffic_view_text", unused_text,
     view_text, view_text, accepted_text, 2},
    {VIEW_LOAD_RVA, 0, 7, GROUP_VEHICLE_CONTROLS, "dealer_traffic_view_load", original_load, new_load, new_load,
     accepted_load, 2},
    {VIEW_LENGTH_RVA, 0, 6, GROUP_VEHICLE_CONTROLS, "dealer_traffic_view_length", original_length, new_length,
     new_length, accepted_length, 2},
};
