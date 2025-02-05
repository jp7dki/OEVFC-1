#include "dynamic_control.h"
#include "vfd_if.h"

#define SEG_NUM_0 0x003F;

//--------------------------------------
// private valuable
//--------------------------------------
static Vfd_Interface dc_vfd_interface;
static uint16_t dc_duty[NUM_DIGIT];
static uint16_t dc_seg1[NUM_DIGIT];
static uint16_t dc_seg2[NUM_DIGIT];
static uint16_t dc_seg2_ratio[NUM_DIGIT];

//--------------------------------------
// private function
//--------------------------------------


//--------------------------------------
// public function
//--------------------------------------
// function : dc_init
// argument:
//      None
// return:
//      None
// description:
//      Initialize GPIO, variables.
static void dc_init(void)
{
    uint8_t i;

    dc_vfd_interface = new_vfd_interface();
    dc_vfd_interface.init();

    for(i=0;i<NUM_DIGIT;i++){
        dc_duty[i] = 100;
        dc_seg1[i] = SEG_NUM_0;
        dc_seg2[i] = SEG_NUM_0;
        dc_seg2_ratio[i] = 0;
    }
}

// function : dc_task
// argument:
//      None
// return:
//      None
// description:
//      display all-digit num by dynamic control
static void dc_task(void)
{
    uint64_t blink_time, seg1_time, seg2_time;
    uint8_t i;

    for(i=0;i<NUM_DIGIT;i++){
        blink_time = (130-dc_duty[i]);
        seg2_time = ((uint64_t)dc_duty[i]*(uint64_t)dc_seg2_ratio[i])>>7;
        seg1_time = dc_duty[i]-seg2_time;

        if(seg2_time > 130) seg2_time=130;
        if(seg1_time > 130) seg1_time=130;

//        dc_vfd_interface.disp_off();
        sleep_us(blink_time);
        
        if(seg1_time != 0){
            dc_vfd_interface.disp(i, dc_seg1[i]);
            sleep_us(seg1_time);
        }

        if(seg2_time != 0){
            dc_vfd_interface.disp(i, dc_seg2[i]);
            sleep_us(seg2_time);
        }
        dc_vfd_interface.disp_off();

    }
}

// function : dc_set_duty
// argument:
//      uint8_t digit : selected digit number
//      uint16_t duty : set duty value
// return:
//      None
// description:
//      set duty value to the selected digit.
static void dc_set_duty(uint8_t digit, uint16_t duty)
{
    dc_duty[digit] = duty;
}

// function : dc_set_seg1
// argument:
//      uint8_t digit : selected digit number
//      uint16_t seg : segment1 value
// return:
//      None
// description:
//      set segment1 value to the selected digit.
static void dc_set_seg1(uint8_t digit, uint16_t seg)
{
    dc_seg1[digit] = seg;
}

// function : dc_set_seg2
// argument:
//      uint8_t digit : selected digit number
//      uint16_t seg : segment2 value
// return:
//      None
// description:
//      set segment2 value to the selected digit.
static void dc_set_seg2(uint8_t digit, uint16_t seg)
{
    dc_seg2[digit] = seg;
}

// function : dc_set_seg2_ratio
// argument:
//      uint8_t digit : selected digit number
//      uint8_t rate : segment2 ratio
// return:
//      None
// description:
//      set segment2 display ratio.
static void dc_set_seg2_ratio(uint8_t digit, uint16_t ratio)
{
    dc_seg2_ratio[digit] = ratio;
}

//--------------------------------------
// Constructor
//--------------------------------------
Dynamic_Control new_dynamic_control(void)
{
    return ((Dynamic_Control){
        .init = dc_init,
        .task = dc_task,
        .set_duty = dc_set_duty,
        .set_seg1 = dc_set_seg1,
        .set_seg2 = dc_set_seg2,
        .set_seg2_ratio = dc_set_seg2_ratio
    });
}