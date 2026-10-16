#ifndef FH2_FEATURES_H
#define FH2_FEATURES_H
#include "extra_options.h"
extern PatchDefinition garage_hook_patch;
extern PatchDefinition road_hook_patch;
extern PatchDefinition gamerpic_hook_patch;
extern PatchDefinition treasure_map_hook_patch;
extern PatchDefinition minimap_hook_patch;
extern PatchDefinition photo_sampling_patch;
extern PatchDefinition world_keys_hook_patches[2];
extern PatchDefinition steering_wheel_patches[2];
extern PatchDefinition ghost_traffic_patches[2];
extern PatchDefinition camera_fov_hook_patches[6];
extern PatchDefinition rims_hook_patch;
extern PatchDefinition stock_wheels_hook_patches[3];
extern PatchDefinition drift_camera_hook_patch;
extern PatchDefinition video_options_hook_patches[3];
extern PatchDefinition drift_camera_view_hook_patches[4];
extern PatchDefinition media_overrides_hook_patch;
extern PatchDefinition vehicle_controls_hook_patch;
extern PatchDefinition autodrive_hook_patches[12];
void garage_prepare_hook(unsigned char *game_image);
void garage_commit_options(const ModOptions *options);
void *garage_callback_address(void);
void road_prepare_hook(unsigned char *game_image);
void road_set_enabled(int enabled);
void gamerpic_prepare_hook(unsigned char *game_image);
void treasure_map_prepare_hook(unsigned char *game_image);
void minimap_prepare_hook(unsigned char *game_image);
void minimap_set_follow_camera(int enabled);
void autodrive_set_cinematic(int enabled);
void photo_sampling_prepare_patch(const ModOptions *options);
void photo_sampling_commit_options(void);
void steering_wheel_prepare_patch(unsigned char *image, const ModOptions *options);
void ghost_traffic_prepare_patch(const ModOptions *options);
void camera_fov_prepare_hooks(unsigned char *game_image);
void rims_prepare_hook(unsigned char *game_image);
void stock_wheels_prepare_hooks(unsigned char *game_image);
void video_options_prepare_hooks(unsigned char *game_image);
void drift_camera_prepare_hook(unsigned char *game_image);
void drift_camera_commit_options(const ModOptions *options);
void media_overrides_prepare_hook(unsigned char *game_image);
void autodrive_prepare_hooks(unsigned char *game_image, const ModOptions *options);
void gamerpic_set_directory(const wchar_t *directory);
#endif
