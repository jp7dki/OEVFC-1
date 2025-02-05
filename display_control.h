#ifndef DISPLAY_CONTROL_H
#define DISPLAY_CONTROL_H

//-------------------------------------------------------
//- Header files include
//-------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include "dynamic_control.h"

//---- display mode difinition ----------------------
typedef enum {
    cm_normal,
    cm_fade,
    cm_crossfade,
    cm_horizontal_roll,
    cm_vertical_roll,
    cm_draw
} Change_effect;

//---- display control structure --------------------
typedef struct display_Control Display_Control;

struct display_Control
{
    void (*init)(void);
    void (*task)(void);
    void (*set_effect)(Change_effect effect);
    Change_effect (*get_effect)(void);
    void (*set_seg)(uint8_t digit, uint16_t seg);
    void (*set_duty)(uint8_t digit, uint16_t duty);
    void (*set_change_flg)(uint8_t digit);
};

//---- Constructor ----------------------
Display_Control new_display_control(void);

#endif