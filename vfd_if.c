#include "vfd_if.h"

// list of digit gpio number
uint digit_gpio_list[] = {
    DIGIT1,
    DIGIT2,
    DIGIT3,
    DIGIT4,
    DIGIT5,
    DIGIT6,
    DIGIT7,
    DIGIT8,
    DIGIT9
};

// list of segment gpio number
uint segment_gpio_list[11] = {
    SEG_A,
    SEG_B,
    SEG_C,
    SEG_D,
    SEG_E,
    SEG_F,
    SEG_G,
    SEG_DOT,
    SEG_CONMA,
    SEG_DASH,
    SEG_HALF
};

//--------------------------------------
// private function
//--------------------------------------
// --- Initialize functions ------------
// gpio initialization : digit
void init_pin_digit(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, false);      // all digit off
}

void init_pin_digits(void)
{
    init_pin_digit(DIGIT1);
    init_pin_digit(DIGIT2);
    init_pin_digit(DIGIT3);
    init_pin_digit(DIGIT4);
    init_pin_digit(DIGIT5);
    init_pin_digit(DIGIT6);
    init_pin_digit(DIGIT7);
    init_pin_digit(DIGIT8);
    init_pin_digit(DIGIT9);
}

// gpio initialization : segment
void init_pin_segment(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, false);      // all segment off
}

void init_pin_segments(void)
{
    init_pin_segment(SEG_A);
    init_pin_segment(SEG_B);
    init_pin_segment(SEG_C);
    init_pin_segment(SEG_D);
    init_pin_segment(SEG_E);
    init_pin_segment(SEG_F);
    init_pin_segment(SEG_G);
    init_pin_segment(SEG_DOT);
    init_pin_segment(SEG_CONMA);
    init_pin_segment(SEG_DASH);
    init_pin_segment(SEG_HALF);
}

// gpio initialization : filament
void init_pin_filaments(void)
{
    // filamentの配線をミスしたため単純にONするのみの処理に変更する。
	gpio_init(FILAMENT1);
	gpio_set_function(FILAMENT1, GPIO_FUNC_SIO);
	gpio_set_dir(FILAMENT1, GPIO_OUT);
	gpio_put(FILAMENT1, true);
	gpio_init(FILAMENT2);
	gpio_set_function(FILAMENT2, GPIO_FUNC_SIO);
	gpio_set_dir(FILAMENT2, GPIO_OUT);
	gpio_put(FILAMENT2, false);

/*
    gpio_init(gpio_a);
    gpio_set_function(gpio_a, GPIO_FUNC_PWM);
    gpio_init(gpio_b);
    gpio_set_function(gpio_b, GPIO_FUNC_PWM);

    // PWM initialization
    uint slice_num = pwm_gpio_to_slice_num(gpio_a);
    pwm_set_wrap(slice_num, 0);
    
    // set  PWM perod
    pwm_set_wrap(slice_num, 1000);
    pwm_set_output_polarity(slice_num, true, false);
    pwm_set_chan_level(slice_num, 0, 400);
    pwm_set_chan_level(slice_num, 1, 600);
    pwm_set_enabled(slice_num, false);      // disable pwm output
*/
}

//---- Display functions --------------
// digit off
void digit_off(uint gpio)
{
    gpio_put(gpio, false);
}

// digit on
void digit_on(uint gpio)
{
    gpio_put(gpio, true);
}

// all digit off
void all_digit_off(void)
{
    digit_off(DIGIT1);
    digit_off(DIGIT2);
    digit_off(DIGIT3);
    digit_off(DIGIT4);
    digit_off(DIGIT5);
    digit_off(DIGIT6);
    digit_off(DIGIT7);
    digit_off(DIGIT8);
    digit_off(DIGIT9);
}

// segment off
void segment_off(uint gpio)
{
    gpio_put(gpio, false);
}

// segment on
void segment_on(uint gpio)
{
    gpio_put(gpio, true);
}

// all segment off
void all_seg_off(void)
{
    segment_off(SEG_A);
    segment_off(SEG_B);
    segment_off(SEG_C);
    segment_off(SEG_D);
    segment_off(SEG_E);
    segment_off(SEG_F);
    segment_off(SEG_G);
    segment_off(SEG_DOT);
    segment_off(SEG_CONMA);
    segment_off(SEG_DASH);
    segment_off(SEG_HALF);
}

// selected segment on
void seg_on(uint16_t segment)
{
    uint8_t i;

    for(i=0;i<11;i++){
        if(((segment>>i)&0x01) == 0){
            segment_off(segment_gpio_list[i]);
        }else{
            segment_on(segment_gpio_list[i]);
        }
    }

}

// display selected number
void display_seg_num(uint8_t num)
{
    uint8_t i;

    all_seg_off();
    for(i=0;i<8;i++){
    }
}

//--------------------------------------
// public function
//--------------------------------------
// function : vi_init(void)
// argument:
//      None
// return:
//      None
// description:
//      Initialize GPIO, variables.
static void vi_init(void)
{
    init_pin_digits();
    init_pin_segments();
    init_pin_filaments();
}

// function : vi_disp
// argument:
//      uint8_t digit   : select display digit
//      uint16_t seg    : select display segment
// return:
//      None
// description:
//      Display selected display/segment.
static void vi_disp(uint8_t digit, uint16_t seg)
{
    digit_on(digit_gpio_list[digit]);
    seg_on(seg);
}

// function : vi_disp_off
// argument:
//      None
// return:
//      None
// description:
//      Display all off
static void vi_disp_off(void)
{
    all_digit_off();
    all_seg_off();
}

//--------------------------------------
// Constructor
//--------------------------------------
Vfd_Interface new_vfd_interface(void)
{
    return ((Vfd_Interface){
        .init = vi_init,
        .disp = vi_disp,
        .disp_off = vi_disp_off
    });
}