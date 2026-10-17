#ifndef FH2_VIDEO_OPTIONS_TEXT_H
#define FH2_VIDEO_OPTIONS_TEXT_H
#include <stddef.h>

typedef struct {
    const char *camera, *driver;
    const wchar_t *garage_label, *garage_question;
    const char *parameters[3];
    const char *descriptions[4];
    const wchar_t *choices[3];
    const char *fov_views[5];
    const char *fov_description;
    const char *settings_names[12];
    const char *settings_descriptions[12];
    const wchar_t *default_choice;
    const wchar_t *minimap_choices[2];
    const wchar_t *spawn_choices[2];
    const wchar_t *car_position_choices[2];
} VideoOptionsText;

const VideoOptionsText *video_options_text(unsigned char *game_image);
#endif
