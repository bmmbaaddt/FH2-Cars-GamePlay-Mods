#ifndef FH2_BLINKERS_H
#define FH2_BLINKERS_H
#include "extra_options.h"

void blinkers_bind(unsigned char *image, uint64_t (*clock)(void));
void blinkers_tick(void *car, int focused, uint64_t now);
void blinkers_reset(void);
// 1 = left, 2 = right, 3 = hazards: switches that one on, or off when it is already on
void blinkers_toggle(void *car, int which);
#endif
