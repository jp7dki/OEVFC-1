#include "light_sensor.h"

//--------------------------------------
// private variable
//--------------------------------------


//--------------------------------------
// private function
//--------------------------------------

//--------------------------------------
// public function
//--------------------------------------
void ls_init(void){
    //---- ADC(Light sensor) ---------
    adc_init();
    adc_gpio_init(LSENSOR_PIN);
    gpio_set_dir(LSENSOR_PIN, GPIO_IN);
    adc_select_input(1);
}

int ls_get_light_value(void){
    int result = adc_read();

    return result;
}

//--------------------------------------
// Constructor
//--------------------------------------
LightSensor new_LightSensor(void)
{
    return ((LightSensor){
        .init = ls_init,
        .get_light_value = ls_get_light_value
    });
}