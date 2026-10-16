#ifndef FH2_GAME_INPUT_H
#define FH2_GAME_INPUT_H

typedef struct {
    int toggle, cancel, focused;
} KeyboardInput;

KeyboardInput game_input_read(void);
int game_input_key_down(int key);
#endif
