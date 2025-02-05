#ifndef DYNAMIC_CONTROL_H
#define DYNAMIC_CONTROL_H

//-------------------------------------------------------
//- Header files include
//-------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include "vfd_if.h"
#include "pico/multicore.h"

//---- dynamic control structure --------------------
typedef struct dynamic_Control Dynamic_Control;

struct dynamic_Control
{
    void (*init)(void);
    void (*task)(void);
    void (*set_duty)(uint8_t digit, uint16_t duty);
    void (*set_seg1)(uint8_t digit, uint16_t seg);
    void (*set_seg2)(uint8_t digit, uint16_t seg);
    void (*set_seg2_ratio)(uint8_t digit, uint16_t ratio);
};

//---- Constructor ----------------------
Dynamic_Control new_dynamic_control(void);

#endif