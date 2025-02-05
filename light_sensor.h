#ifndef LIGHT_SENSOR
#define LIGHT_SENSOR

//-------------------------------------------------------
//- Header files include
//-------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"

//-------------------------------------------------------
//- Macro definition
//-------------------------------------------------------
#define LSENSOR_PIN 27

//---- Light sensor display structure --------------------
typedef struct lightSensor LightSensor;

struct lightSensor
{
    void (*init)(void);
    int (*get_light_value)(void);
};

//---- Constructor ----------------------
LightSensor new_LightSensor(void);

#endif