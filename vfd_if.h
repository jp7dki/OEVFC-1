#ifndef VFD_IF_H
#define VFD_IF_H

//-------------------------------------------------------
//- Header files include
//-------------------------------------------------------
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

//-------------------------------------------------------
//- Macro difinition
//-------------------------------------------------------
// GPIO number definition
#define DIGIT1 3
#define DIGIT2 14
#define DIGIT3 11
#define DIGIT4 9
#define DIGIT5 8
#define DIGIT6 7
#define DIGIT7 17
#define DIGIT8 20
#define DIGIT9 24

#define SEG_A 4
#define SEG_B 5
#define SEG_C 6
#define SEG_D 18
#define SEG_E 19
#define SEG_F 22
#define SEG_G 21
#define SEG_DOT 10
#define SEG_CONMA 12
#define SEG_DASH 23
#define SEG_HALF 13

#define FILAMENT1 25
#define FILAMENT2 26

// Display parameters
#define NUM_DIGIT 9

// segment mask
#define SEG_A_MASK (1 << 0)
#define SEG_B_MASK (1 << 1)
#define SEG_C_MASK (1 << 2)
#define SEG_D_MASK (1 << 3)
#define SEG_E_MASK (1 << 4)
#define SEG_F_MASK (1 << 5)
#define SEG_G_MASK (1 << 6)
#define SEG_CONMA_MASK (1 << 7)
#define SEG_DOT_MASK (1 << 8)
#define SEG_DASH_MASK (1 << 9)
#define SEG_HALF_MASK (1 << 10)

//---- VFD IF structure --------------------
typedef struct  vfd_Interface Vfd_Interface;

struct vfd_Interface
{   
    void (*init)(void);
    void (*disp)(uint8_t digit, uint16_t seg);
    void (*disp_off)(void);
};

//---- Constructor ----------------------
Vfd_Interface new_vfd_interface(void);

#endif