#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rtc_rx8900.h"
#include "display_control.h"
#include "light_sensor.h"
#include "pico/multicore.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"

// #include "light_sensor.h"

#define DEBUG

//---- macro difinition -----
#define SWA_PIN 2
#define SWB_PIN 1
#define SWC_PIN 0

#define cursor_timeadjust_MAX 11

// FLASH
#define FLASH_TARGET_OFFSET 0x1F0000

#define MAX_NUM_SETTING 5

// Config
#define ENABLE_AUTO_ONOFF 1
#define DISABLE_AUTO_ONOFF 0

//-------------------------------------------------------
//- Global Variable
//-------------------------------------------------------

const uint8_t version[9] = {
	0b00111111,		
	0b00111111,		
	0b00111111,		
	0b00111111,		
	0b00000110,
	0b00111111,		
	0b00000000,		
	0b00000000,	
	0b00000000
};

const uint16_t mode_disp_eff[3] = {
	0b01111001,		// E
	0b01110001,		// F
	0b01110001		// F
};

//---- operation mode difinition ----------------------
typedef enum {
	mode_poweron,
	mode_power_up_animation,
	mode_clock,
	mode_clock_calendar,
	mode_timeadjust,
	mode_settings,
	mode_random_idle,
	mode_random,
	mode_demo,
	mode_off_animation,
	mode_on_animation,
	mode_poweroff,
	mode_time_animation
} Operation_mode;

Operation_mode opmode, opmode_prev;
bool flg_display_off;

//---- hardware ------------------
Display_Control dcnt;
RtcRx8900 rtc;
LightSensor ls;
rtc_time now_datetime,prev_datetime,timeadjust_datetime, prev_timeadjust_datetime;
bool flg_timeupdate;
bool flg_random_in;

uint32_t light_count;
uint32_t light_val;
uint8_t guruguru_count;
bool time_tick_flg=false;

uint16_t num_to_segment[10] = {
	0b00111111,		
	0b00000110,		
	0b01011011,		
	0b01001111,		
	0b01100110,		
	0b01101101,		
	0b01111101,		
	0b00000111,		
	0b01111111,	
	0b01101111
};

//---- time adjust -------------
uint8_t cursor_timeadjust;
uint8_t cursor_setting;

//---- settings ----------------
typedef enum{
	effect,
	brightness_auto,
	brightness,
	auto_onoff,
	on_time,
	off_time
} Num_setting;
Num_setting num_setting;

typedef struct {
	Change_effect change_effect;
	uint8_t brightness_setting;
	uint8_t brightness_auto;
	uint8_t auto_onoff;
	rtc_time auto_on_time;
	rtc_time auto_off_time;
} ClockConfig;
ClockConfig conf;

//---- flash data --------------
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

typedef struct{
	Change_effect change_effect;
	uint8_t flash_initialized;
	uint8_t brightness;
	uint8_t brightness_auto;
	uint8_t auto_onoff;
	rtc_time auto_off_time;
	rtc_time auto_on_time;
} Nvol_Config;

typedef union Nvol_Data{
	uint8_t flash_byte[FLASH_PAGE_SIZE];
	Nvol_Config config;
} Nvol_data;

Nvol_data flash_data;
bool flg_flash_write;

//-------------------------------------------------------
//- Function Prototyping
//-------------------------------------------------------
void check_button(uint8_t SW_PIN, void (*short_func)(void), void (*long_func)(void));
void swa_short_push(void);
void swa_long_push(void);
void swb_short_push(void);
void swb_long_push(void);
void swc_short_push(void);
void swc_long_push(void);
void timeadjust_add(rtc_time *time_add);

void flash_read(uint8_t *read_data);
bool flash_write(uint8_t *write_data);

void parameter_backup(void);

void time_add_hm(rtc_time *time_rtc, uint8_t cursor_time);

//------------------------------------------------------
//-- IRQ : Timer function
//------------------------------------------------------
//---- timer_alarm1 :
static void timer_alarm1_irq(void){
    // Clear the irq
    hw_clear_bits(&timer_hw->intr, 1u << 1);
    uint64_t target = timer_hw->timerawl + 1000000; // interval 40ms
    timer_hw->alarm[1] = (uint32_t)target;
	rtc_time tmp_datetime;

	tmp_datetime.all = 0;

	// Real time clock read
	if((opmode==mode_power_up_animation)||(opmode==mode_clock) || (opmode==mode_clock_calendar)){
		rtc.get_time(&tmp_datetime);
		if(tmp_datetime.all != now_datetime.all){
			prev_datetime = now_datetime;
			now_datetime = tmp_datetime;

			if(flg_display_off == false){
				if(conf.auto_onoff == ENABLE_AUTO_ONOFF){
					// auto-off 
					if(
						(tmp_datetime.bcd.hour == conf.auto_off_time.bcd.hour) &&
						(tmp_datetime.bcd.min == conf.auto_off_time.bcd.min) &&
						(tmp_datetime.bcd.sec == conf.auto_off_time.bcd.sec)
					){
						opmode_prev = opmode;
						opmode = mode_off_animation;
					}else{
						flg_timeupdate = true;
					}
				}else{
					flg_timeupdate = true;
				}
			}else{
				// auto-on
				if(
					(tmp_datetime.bcd.hour == conf.auto_on_time.bcd.hour) &&
					(tmp_datetime.bcd.min == conf.auto_on_time.bcd.min) &&
					(tmp_datetime.bcd.sec == conf.auto_on_time.bcd.sec)
				){
					opmode_prev = opmode;
					opmode = mode_on_animation;
					flg_display_off = false;
					flg_timeupdate = true;
				}
			}
		}
	}
}

//---- timer_alarm2 : core0 周期実行タスク
static void timer_alarm2_irq(void){
	int i;

    // Clear the irq
    hw_clear_bits(&timer_hw->intr, 1u << 2);
    uint64_t target = timer_hw->timerawl + 1000; // interval 10ms
    timer_hw->alarm[2] = (uint32_t)target;

	if(opmode!=mode_power_up_animation){
		// 明るさに応じてDutyを変更
		// ちらつき対策のため複数回の平均を取る
		// 現状は暫定の式
		if(conf.brightness_auto==1){
			light_val += ls.get_light_value();	 //50-130くらいの間
			light_count++;
			if(light_count >= 32){
				light_val = light_val >> 5;

				if(light_val>110) light_val = 110;
				if(light_val<10) light_val = 10;

				if(opmode != mode_power_up_animation){
					for(i=0;i<9;i++){
						dcnt.set_duty(i, (uint8_t)light_val);
					}
				}
				light_count = 0;
				light_val = 0;
			}
		}else{
			light_count++;
			if(light_count > 100){
				for(i=0;i<NUM_DIGIT;i++){
					dcnt.set_duty(i, conf.brightness_setting*10+10);
				}
				light_count = 0;
			}
		}
	}
}

//------------------------------------------------------
//-- core1 entry
//------------------------------------------------------
// core1は表示処理のみ行う
void core1_entry(){
	uint64_t target;

	uint8_t i;
	for(i=0;i<NUM_DIGIT;i++){
		dcnt.set_duty(i, 1);
		dcnt.set_seg(i, 0);
	}

	ls = new_LightSensor();
	ls.init();

	hw_set_bits(&timer_hw->inte, 1u<<1);		// Alarm2
	irq_set_exclusive_handler(TIMER_IRQ_1, timer_alarm1_irq);
	irq_set_enabled(TIMER_IRQ_1, true);
	target = timer_hw -> timerawl + 100000;
	timer_hw -> alarm[1] = (uint32_t)target;

	// Timer settings
	hw_set_bits(&timer_hw->inte, 1u<<2);		// Alarm2
	irq_set_exclusive_handler(TIMER_IRQ_2, timer_alarm2_irq);
	irq_set_enabled(TIMER_IRQ_2, true);
	target = timer_hw -> timerawl + 10000;
	timer_hw -> alarm[2] = (uint32_t)target;

	rtc_time tmp_datetime;

    while(1)
	{
		//-----------------------------------------
		//---- Core1 main routine
		//-----------------------------------------
		// 入力に応じた動作モードの遷移を行う
        //---- SWA -----------------------
        check_button(SWA_PIN, swa_short_push, swa_long_push);

        //---- SWB -----------------------
        check_button(SWB_PIN, swb_short_push, swb_long_push);

        //---- SWC -----------------------
        check_button(SWC_PIN, swc_short_push, swc_long_push);
		
	}
}

//------------------------------------------------------
//-- main function 
//------------------------------------------------------
int main(void)
{
	uint16_t i;
	uint32_t animation_counter = 0;
	uint16_t temp;
	uint8_t num_powerup_animation;
	uint8_t flg_animation_digit[NUM_DIGIT];
	uint8_t flg_animation_count=0;
	rtc_time tmp_datetime;

	for(i=0;i<NUM_DIGIT;i++){
		flg_animation_digit[i] = 0;
	}

	light_count = 0;
	light_val = 0;
	num_setting = effect;
	opmode = mode_power_up_animation;
	flg_timeupdate = true;
	flg_flash_write = false;
	flg_display_off = false;
	flg_random_in = false;

	uint16_t sec1,sec2,min1,min2,hour1,hour2,weekday,day1,day2,month1,month2,year1,year2;
	
	sleep_ms(500);

     // clock settings
	set_sys_clock_khz (125000, true);
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    48 * MHZ,
                    48 * MHZ);

    // Re init uart now that clk_peri has changed
    stdio_init_all();

	//---- create instances --------------------
	dcnt = new_display_control();

	//---- Hardware initialization -------------
	dcnt.init();

	//---- non-volatile(Flash) memory read -----
	flash_read(flash_data.flash_byte);

	if(flash_data.config.flash_initialized != 0x5A){
		for(i=0;i<FLASH_PAGE_SIZE;i++){
			flash_data.flash_byte[i] = 0;
		}
		flash_data.config.auto_off_time.all = 0;
		flash_data.config.auto_off_time.bcd.hour = 0x22;
		flash_data.config.auto_off_time.bcd.min = 0x00;
		flash_data.config.auto_off_time.bcd.sec = 0x00;
		flash_data.config.auto_on_time.all = 0;
		flash_data.config.auto_on_time.bcd.hour = 0x06;
		flash_data.config.auto_on_time.bcd.min = 0x00;
		flash_data.config.auto_on_time.bcd.sec = 0x00;
		flash_data.config.auto_onoff = 0;
		flash_data.config.brightness = 7;
		flash_data.config.brightness_auto = 1;
		flash_data.config.change_effect = cm_crossfade;
		flash_data.config.flash_initialized = 0x5A;
		flash_write(flash_data.flash_byte);
	}

	conf.change_effect = flash_data.config.change_effect;
	dcnt.set_effect(conf.change_effect);
	conf.auto_onoff = flash_data.config.auto_onoff;
	conf.auto_on_time = flash_data.config.auto_on_time;
	conf.auto_off_time = flash_data.config.auto_off_time;
	conf.brightness_auto = flash_data.config.brightness_auto;
	conf.brightness_setting = flash_data.config.brightness;

	// Random seed set
	rtc = new_RtcRx8900();
	rtc.init();

	if(rtc.is_voltage_low()){
		rtc.hard_init();
	}

	rtc.get_time(&tmp_datetime);
	srand(tmp_datetime.all);

	num_powerup_animation = rand()%2;

	// SW initialized
	gpio_init(SWA_PIN);
	gpio_set_dir(SWA_PIN, GPIO_IN);
	gpio_pull_up(SWA_PIN);
	gpio_init(SWB_PIN);
	gpio_set_dir(SWB_PIN, GPIO_IN);
	gpio_pull_up(SWB_PIN);
	gpio_init(SWC_PIN);
	gpio_set_dir(SWC_PIN, GPIO_IN);
	gpio_pull_up(SWC_PIN);

    // Core1 Task 
    multicore_launch_core1(core1_entry);

    while(1)
	{
		//--------------------------------------
		//---- Core0 main routine
		//--------------------------------------
		dcnt.task();

		switch(opmode){
			case mode_power_up_animation:
				if(num_powerup_animation == 0){
					// 乱数表示しながらフェードイン
					if(animation_counter%20 == 0){
						for(i=0;i<NUM_DIGIT;i++){
							if(animation_counter < 7800-100*i){
								dcnt.set_seg(i, num_to_segment[rand()%10]);
							}else{
								temp = version[i];
								if((i==2)||(i==4)){
									temp = temp | SEG_DOT_MASK;
								}
								dcnt.set_seg(i, temp);
							}
						}
					}

					if(animation_counter < 4000){
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, animation_counter/40);
						}
					}

					if(animation_counter > 11000){
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, 0);
						}
					}else if(animation_counter > 10000){
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, (11001-animation_counter)/10);
						}
					}

					if(animation_counter > 13000){
						animation_counter = 0;
						opmode = mode_clock;

						if(conf.brightness_auto==0){
							for(i=0;i<NUM_DIGIT;i++){
								dcnt.set_duty(i, 0);
							}	
						}
					}

					animation_counter++;

				}else{
					// 垂直ロールで乱数を表示した後、バージョン表示を行う
					if(animation_counter==0){
						dcnt.set_effect(cm_vertical_roll);		// 一時的に変更する
					}

					if(animation_counter%100 == 0){
						if(animation_counter < 5001){
							if(flg_animation_count<NUM_DIGIT){
								do{
									temp = (uint8_t)rand()%NUM_DIGIT;
								}while(flg_animation_digit[temp] != 0);

								flg_animation_digit[temp] = 1;
								dcnt.set_seg(temp, num_to_segment[(uint8_t)rand()%10]);
								dcnt.set_change_flg(temp);
								flg_animation_count++;

							}else{
								temp = (uint8_t)rand()%NUM_DIGIT;
								dcnt.set_seg(temp, num_to_segment[(uint8_t)rand()%10]);
								dcnt.set_change_flg(temp);
							}
						}else if(animation_counter < 8000){
							if(flg_animation_count < NUM_DIGIT*2){
								do{
									temp = rand()%NUM_DIGIT;
								}while(flg_animation_digit[temp] != 1);

								flg_animation_digit[temp] = 0;
								flg_animation_count++;

								if((temp==2)||(temp==4)){
									dcnt.set_seg(temp, version[temp] | SEG_DOT_MASK);
									dcnt.set_change_flg(temp);
								}else{
									dcnt.set_seg(temp, version[temp]);
									dcnt.set_change_flg(temp);
								}
							}
						}			
					}

					if(animation_counter > 8000){
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, 0);
						}						
					}else if(animation_counter > 7000){
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, (8001-animation_counter)/10);
						}
					}else{
						for(i=0;i<NUM_DIGIT;i++){
							dcnt.set_duty(i, 100);
						}
					}

					if(animation_counter > 9000){
						if(conf.brightness_auto==0){
							for(i=0;i<NUM_DIGIT;i++){
								dcnt.set_duty(i, 0);
							}	
						}
						animation_counter = 0;
						opmode = mode_clock;
						dcnt.set_effect(conf.change_effect);
					}

					animation_counter++;
				}

				break;

			case mode_clock:
				flg_random_in = false;
				if(flg_display_off == true){
					// all segment off
					for(i=0;i<NUM_DIGIT;i++) dcnt.set_seg(i, 0x00);
				}else{
					if(flg_timeupdate == true){
						// 時計表示
						dcnt.set_seg(0, 0x00);
						dcnt.set_seg(1, num_to_segment[now_datetime.separate.sec1]);
						dcnt.set_seg(2, num_to_segment[now_datetime.separate.sec2]);
						dcnt.set_seg(3, num_to_segment[now_datetime.separate.min1]|SEG_DOT_MASK);
						dcnt.set_seg(4, num_to_segment[now_datetime.separate.min2]);
						dcnt.set_seg(5, num_to_segment[now_datetime.separate.hour1]|SEG_DOT_MASK);
						dcnt.set_seg(6, num_to_segment[now_datetime.separate.hour2]);
						dcnt.set_seg(7, 0x00);
						dcnt.set_seg(8, 0x00);

						if(now_datetime.separate.hour2 != prev_datetime.separate.hour2) dcnt.set_change_flg(6);
						if(now_datetime.separate.hour1 != prev_datetime.separate.hour1) dcnt.set_change_flg(5);
						if(now_datetime.separate.min2 != prev_datetime.separate.min2) dcnt.set_change_flg(4);
						if(now_datetime.separate.min1 != prev_datetime.separate.min1) dcnt.set_change_flg(3);
						if(now_datetime.separate.sec2 != prev_datetime.separate.sec2) dcnt.set_change_flg(2);
						if(now_datetime.separate.sec1 != prev_datetime.separate.sec1) dcnt.set_change_flg(1);

						flg_timeupdate = false;
					}
				}

				break;

			case mode_clock_calendar:
				flg_random_in = false;
				if(flg_display_off == true){
					// all segment off
					for(i=0;i<NUM_DIGIT;i++) dcnt.set_seg(i, 0x00);
						dcnt.set_seg(8, num_to_segment[now_datetime.separate.month2]);

				}else{
					if(flg_timeupdate == true){
						// 時計表示
						dcnt.set_seg(0, num_to_segment[now_datetime.separate.min1]);
						dcnt.set_seg(1, num_to_segment[now_datetime.separate.min2]);
						if(now_datetime.separate.sec1%2 == 0){
							dcnt.set_seg(2, num_to_segment[now_datetime.separate.hour1]|SEG_DOT_MASK);
						}else{
							dcnt.set_seg(2, num_to_segment[now_datetime.separate.hour1]);
						}
						dcnt.set_seg(3, num_to_segment[now_datetime.separate.hour2]);
						dcnt.set_seg(4, 0x00);
						dcnt.set_seg(5, num_to_segment[now_datetime.separate.day1]);
						dcnt.set_seg(6, num_to_segment[now_datetime.separate.day2]);
						dcnt.set_seg(7, num_to_segment[now_datetime.separate.month1]|SEG_CONMA_MASK|SEG_DOT_MASK);
						dcnt.set_seg(8, num_to_segment[now_datetime.separate.month2]);

						if(now_datetime.separate.month2 != prev_datetime.separate.month2) dcnt.set_change_flg(8);
						if(now_datetime.separate.month1 != prev_datetime.separate.month1) dcnt.set_change_flg(7);
						if(now_datetime.separate.day2 != prev_datetime.separate.day2) dcnt.set_change_flg(6);
						if(now_datetime.separate.day1 != prev_datetime.separate.day1) dcnt.set_change_flg(5);
						if(now_datetime.separate.hour2 != prev_datetime.separate.hour2) dcnt.set_change_flg(3);
						if(now_datetime.separate.hour1 != prev_datetime.separate.hour1) dcnt.set_change_flg(2);
						if(now_datetime.separate.min2 != prev_datetime.separate.min2) dcnt.set_change_flg(1);
						if(now_datetime.separate.min1 != prev_datetime.separate.min1) dcnt.set_change_flg(0);

						flg_timeupdate = false;
					}
				}

				break;

			case mode_timeadjust:
				// 時計合わせ表示
				sec1 = num_to_segment[timeadjust_datetime.separate.sec1&0x0F];
				sec2 = num_to_segment[timeadjust_datetime.separate.sec2&0x07];
				min1 = num_to_segment[timeadjust_datetime.separate.min1&0x0F];
				min2 = num_to_segment[timeadjust_datetime.separate.min2&0x07];
				hour1 = num_to_segment[timeadjust_datetime.separate.hour1&0x0F];
				hour2 = num_to_segment[timeadjust_datetime.separate.hour2&0x03];
				weekday = timeadjust_datetime.separate.weekday;
				day1 = num_to_segment[timeadjust_datetime.separate.day1];
				day2 = num_to_segment[timeadjust_datetime.separate.day2];
				month1 = num_to_segment[timeadjust_datetime.separate.month1];
				month2 = num_to_segment[timeadjust_datetime.separate.month2];
				year1 = num_to_segment[timeadjust_datetime.separate.year1];
				year2 = num_to_segment[timeadjust_datetime.separate.year2];

				switch(cursor_timeadjust){
					case 0:
						sec1 = sec1 | SEG_DASH_MASK;
						break;
					case 1:
						sec2 = sec2 | SEG_DASH_MASK;
						break;
					case 2:
						min1 = min1 | SEG_DASH_MASK;
						break;
					case 3:
						min2 = min2 | SEG_DASH_MASK;
						break;
					case 4:
						hour1 = hour1 | SEG_DASH_MASK;
						break;
					case 5:
						hour2 = hour2 | SEG_DASH_MASK;
						break;

					case 6:
						day1 = day1 | SEG_DASH_MASK;
						break;

					case 7:
						day2 = day2 | SEG_DASH_MASK;
						break;
					
					case 8:
						month1 = month1 | SEG_DASH_MASK;
						break;
					
					case 9:
						month2 = month2 | SEG_DASH_MASK;
						break;

					case 10:
						year1 = year1 | SEG_DASH_MASK;
						break;

					case 11:
						year2 = year2 | SEG_DASH_MASK;
						break;
				}

				if(cursor_timeadjust < 6){
					dcnt.set_seg(0, 0x00);
					dcnt.set_seg(1, sec1);
					dcnt.set_seg(2, sec2);
					dcnt.set_seg(3, min1);
					dcnt.set_seg(4, min2);
					dcnt.set_seg(5, hour1);
					dcnt.set_seg(6, hour2);
					dcnt.set_seg(7, 0x00);
					dcnt.set_seg(8, 0x00);
				}else{
					dcnt.set_seg(0, day1);
					dcnt.set_seg(1, day2);
					dcnt.set_seg(2, 0x00);
					dcnt.set_seg(3, month1);
					dcnt.set_seg(4, month2);
					dcnt.set_seg(5, 0x00);
					dcnt.set_seg(6, year1);
					dcnt.set_seg(7, year2);
					dcnt.set_seg(8, 0x00);
				}
				break;

			case mode_settings:

				switch(num_setting){
					// effect setting
					case effect:
						dcnt.set_seg(8, 0b01111001);		// E
						dcnt.set_seg(7, 0b01110001);		// F
						dcnt.set_seg(6, 0b01110001);		// F
						dcnt.set_seg(5, 0x00);
						switch(conf.change_effect){
							case cm_normal:
								dcnt.set_seg(4, 0x00);
								dcnt.set_seg(3, 0b01010100);	// n
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b01010000);	// r
								dcnt.set_seg(0, 0b01010101);	// m
								break;
							case cm_fade:
								dcnt.set_seg(4, 0x00);
								dcnt.set_seg(3, 0b01110001);	// F
								dcnt.set_seg(2, 0b01110111);	// A
								dcnt.set_seg(1, 0b01011110);	// d
								dcnt.set_seg(0, 0b01111001);	// E
								break;
							case cm_crossfade:
								dcnt.set_seg(4, 0b01011000);	// c
								dcnt.set_seg(3, 0b01010000);	// r
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b01101101);	// S
								dcnt.set_seg(0, 0b01101101);	// S
								break;
							case cm_horizontal_roll:
								dcnt.set_seg(4, 0b01110100);	// h
								dcnt.set_seg(3, 0b01010000);	// r
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b00111000);	// L
								dcnt.set_seg(0, 0b00111000);	// L
								break;
							case cm_vertical_roll:
								dcnt.set_seg(4, 0b01111110);	// v
								dcnt.set_seg(3, 0b01010000);	// r
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b00111000);	// L
								dcnt.set_seg(0, 0b00111000);	// L
								break;
							case cm_draw:
								dcnt.set_seg(4, 0x00);
								dcnt.set_seg(3, 0b01011110);	// d
								dcnt.set_seg(2, 0b01010000);	// r
								dcnt.set_seg(1, 0b01110111);	// A
								dcnt.set_seg(0, 0b00011101);	// w
								break;

						}
						break;

					case brightness_auto:
						// brightness auto on/off
						dcnt.set_seg(8, 0b01111100);		// b
						dcnt.set_seg(7, 0b01110111);		// A
						dcnt.set_seg(6, 0b00111110);		// U
						dcnt.set_seg(5, 0b00000111);		// T
						dcnt.set_seg(4, 0b01011100);		// o
						dcnt.set_seg(3, 0x00);
						switch(conf.brightness_auto){
							case 0:
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b01110001);	// F
								dcnt.set_seg(0, 0b01110001);	// F
								break;
							case 1:
								dcnt.set_seg(2, 0x00);
								dcnt.set_seg(1, 0b01011100);	// o
								dcnt.set_seg(0, 0b01010100);	// n
								break;
						}
						break;

					case brightness:
						// brightness setting
						dcnt.set_seg(8, 0b01111100);		// b
						dcnt.set_seg(7, 0b00111000);		// L
						dcnt.set_seg(6, 0b01111001);		// E
						dcnt.set_seg(5, 0b01111110);		// V
						dcnt.set_seg(4, 0b01111001);		// E
						dcnt.set_seg(3, 0b00111000);		// L
						dcnt.set_seg(2, 0x00);
						dcnt.set_seg(1, 0x00);
						dcnt.set_seg(0, num_to_segment[conf.brightness_setting]);
						break;
					
					case auto_onoff:
						// auto-on/off setting
						dcnt.set_seg(8, 0b01011100);		// o
						dcnt.set_seg(7, 0b01010100);		// n
						dcnt.set_seg(6, 0b01011100);		// o
						dcnt.set_seg(5, 0b01110001);		// F
						dcnt.set_seg(4, 0b01110001);		// F
						dcnt.set_seg(3, 0x00);
						dcnt.set_seg(2, 0x00);
						switch(conf.auto_onoff){
							case 0:
								dcnt.set_seg(2, 0b01011100);	// o
								dcnt.set_seg(1, 0b01110001);	// F
								dcnt.set_seg(0, 0b01110001);	// F
								break;
							case 1:
								dcnt.set_seg(2, 0x00);
								dcnt.set_seg(1, 0b01011100);	// o
								dcnt.set_seg(0, 0b01010100);	// n
								break;
						}
						break;
					
					case on_time:
						// auto-on time setting 
						dcnt.set_seg(8, 0b01011100);		// o
						dcnt.set_seg(7, 0b01010100);		// n
						dcnt.set_seg(6, 0x00);
						dcnt.set_seg(5, 0x00);
						dcnt.set_seg(4, 0x00);
						if(cursor_setting==3){
							dcnt.set_seg(3, num_to_segment[conf.auto_on_time.separate.hour2] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(3, num_to_segment[conf.auto_on_time.separate.hour2]);
						}

						if(cursor_setting==2){
							dcnt.set_seg(2, num_to_segment[conf.auto_on_time.separate.hour1] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(2, num_to_segment[conf.auto_on_time.separate.hour1]);
						}

						if(cursor_setting==1){
							dcnt.set_seg(1, num_to_segment[conf.auto_on_time.separate.min2] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(1, num_to_segment[conf.auto_on_time.separate.min2]);
						}

						if(cursor_setting==0){
							dcnt.set_seg(0, num_to_segment[conf.auto_on_time.separate.min1] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(0, num_to_segment[conf.auto_on_time.separate.min1]);
						}
						break;

					case off_time:
						// auto-off time setting
						dcnt.set_seg(8, 0b01011100);			// o
						dcnt.set_seg(7, 0b01110001);			// F
						dcnt.set_seg(6, 0b01110001);			// F
						dcnt.set_seg(5, 0x00);
						dcnt.set_seg(4, 0x00);
						if(cursor_setting==3){
							dcnt.set_seg(3, num_to_segment[conf.auto_off_time.separate.hour2] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(3, num_to_segment[conf.auto_off_time.separate.hour2]);
						}

						if(cursor_setting==2){
							dcnt.set_seg(2, num_to_segment[conf.auto_off_time.separate.hour1] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(2, num_to_segment[conf.auto_off_time.separate.hour1]);
						}

						if(cursor_setting==1){
							dcnt.set_seg(1, num_to_segment[conf.auto_off_time.separate.min2] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(1, num_to_segment[conf.auto_off_time.separate.min2]);
						}

						if(cursor_setting==0){
							dcnt.set_seg(0, num_to_segment[conf.auto_off_time.separate.min1] | SEG_DASH_MASK);
						}else{
							dcnt.set_seg(0, num_to_segment[conf.auto_off_time.separate.min1]);
						}
						break;

					case 6:
						// TBD

						break;
					
					default:

						break;
				}
				break;

			case mode_random_idle:
				if(flg_random_in==false){
					// all digit display 0.
					for(i=0;i<NUM_DIGIT;i++){
						dcnt.set_seg(i, num_to_segment[0]);
					}
					flg_random_in = true;
				}

				break;

			case mode_random:
			case mode_demo:
				// display random value
				if(animation_counter%20 == 0){
					for(i=0;i<NUM_DIGIT;i++){
						if(animation_counter < (4000 + 300*NUM_DIGIT - 300*i)){
							dcnt.set_seg(i, num_to_segment[rand()%10]);
						}else{
							// nop
						}
					}
				}

				if(opmode == mode_random){
					if(animation_counter > 8000){
						animation_counter = 0;
						opmode = mode_random_idle;
					}
				}else{
					if(animation_counter > 8000){
						animation_counter = 0;
					}					
				}

				animation_counter++;

				break;

			case mode_off_animation:
				// 乱数表示しながら消える
				if(animation_counter%20 == 0){
					for(i=0;i<NUM_DIGIT;i++){
						if(animation_counter > (600 + 80*i)){
							dcnt.set_seg(i, 0);
						}else if(animation_counter > 80*i){
							dcnt.set_seg(i, num_to_segment[rand()%10]);
						}else{

						}
					}
				}

				if(animation_counter > 1500){
					animation_counter = 0;
					opmode = opmode_prev;
					flg_display_off = true;
				}

				animation_counter++;

				break;

			case mode_on_animation:
				// 乱数表示しながら表示
				if(animation_counter%20 == 0){
					if(opmode_prev == mode_clock){
						for(i=0;i<NUM_DIGIT;i++){
							if(animation_counter > (600 + 80*i)){
								switch(i){
									case 1:
										dcnt.set_seg(1, num_to_segment[now_datetime.separate.sec1]);
										break;
									case 2:
										dcnt.set_seg(2, num_to_segment[now_datetime.separate.sec2]);
										break;
									case 3:
										dcnt.set_seg(3, num_to_segment[now_datetime.separate.min1]|SEG_DOT_MASK);
										break;
									case 4:
										dcnt.set_seg(4, num_to_segment[now_datetime.separate.min2]);
										break;
									case 5:
										dcnt.set_seg(5, num_to_segment[now_datetime.separate.hour1]|SEG_DOT_MASK);
										break;
									case 6:
										dcnt.set_seg(6, num_to_segment[now_datetime.separate.hour2]);
										break;
									case 0:
									case 7:
									case 8:
										dcnt.set_seg(i, 0x00);
										break;
								}
							}else if(animation_counter > 80*i){
								dcnt.set_seg(i, num_to_segment[rand()%10]);
							}
						}
					}else if(opmode_prev == mode_clock_calendar){
						for(i=0;i<NUM_DIGIT;i++){
							if(animation_counter > (600 + 80*i)){
								switch(i){
									case 0:
										dcnt.set_seg(0, num_to_segment[now_datetime.separate.min1]);
										break;
									case 1:
										dcnt.set_seg(1, num_to_segment[now_datetime.separate.min2]);
										break;
									case 2:
										if(now_datetime.separate.sec1%2 == 0){
											dcnt.set_seg(2, num_to_segment[now_datetime.separate.hour1]|SEG_DOT_MASK);
										}else{
											dcnt.set_seg(2, num_to_segment[now_datetime.separate.hour1]);
										}
										break;
									case 3:
										dcnt.set_seg(3, num_to_segment[now_datetime.separate.hour2]);
										break;
									case 4:
										dcnt.set_seg(4, 0x00);
										break;
									case 5:
										dcnt.set_seg(5, num_to_segment[now_datetime.separate.day1]);
										break;
									case 6:
										dcnt.set_seg(6, num_to_segment[now_datetime.separate.day2]);
										break;
									case 7:
										dcnt.set_seg(7, num_to_segment[now_datetime.separate.month1]|SEG_CONMA_MASK|SEG_DOT_MASK);
										break;
									case 8:
										dcnt.set_seg(8, num_to_segment[now_datetime.separate.month2]);
										break;
								}
							}else if(animation_counter > 80*i){
								dcnt.set_seg(i, num_to_segment[rand()%10]);
							}
						}
					}
					
				}

				if(animation_counter > 1500){
					animation_counter = 0;
					opmode = opmode_prev;
				}

				animation_counter++;

				break;
				

			default:
				break;
		}

		if(flg_flash_write == true){
			parameter_backup();
			flg_flash_write = false;
		}

	}
}

//---- switch check (only use main function) -------------
void check_button(uint8_t SW_PIN, void (*short_func)(void), void (*long_func)(void))
{
    uint16_t count_sw = 0;

    if(!gpio_get(SW_PIN)){
        sleep_ms(100);
        while((!gpio_get(SW_PIN)) && (count_sw<200)){
            count_sw++;
            sleep_ms(10);
        }

        if(count_sw==200){
            // Long-push
            long_func();
        }else{
            // Short-push
            short_func();
        }

        while(!gpio_get(SW_PIN));
    }
}

void swa_short_push(void)
{
	switch(opmode){
		case mode_clock:
			opmode = mode_clock_calendar;
			flg_timeupdate = true;
			break;
		case mode_clock_calendar:
			opmode = mode_clock;
			flg_timeupdate = true;
			break;

		case mode_settings:
			if(cursor_setting==0){
				cursor_setting =3;
			}else{
				cursor_setting--;
			}
			break;
		case mode_timeadjust:
			if(cursor_timeadjust==0){
				cursor_timeadjust=cursor_timeadjust_MAX;
			}else{
				cursor_timeadjust--;
			}
			break;
		case mode_random_idle:
			opmode = mode_random;
			break;

		case mode_random:
		case mode_demo:

			break;
	}
}

void swa_long_push(void)
{
	switch(opmode){
		case mode_clock:
		case mode_clock_calendar:
			opmode = mode_settings;
			cursor_setting = 0;
			break;
		case mode_settings:
			opmode = mode_clock;
			dcnt.set_effect(conf.change_effect);
			flash_data.config.change_effect = conf.change_effect;	
			flg_flash_write = true;
			break;
		case mode_timeadjust:
			opmode = mode_clock;
			if(timeadjust_datetime.all != prev_timeadjust_datetime.all){
				timeadjust_datetime.bcd.weekday = 0x01;
				rtc.set_time(timeadjust_datetime);
			}
			break;
		case mode_random_idle:
		case mode_random:
		case mode_demo:
			opmode = mode_clock;
			break;
	}
}

void swb_short_push(void)
{
	switch(opmode){
		case mode_clock:
		case mode_clock_calendar:
			// pass
			break;
		case mode_settings:
			switch(num_setting){
				case effect:
					switch(conf.change_effect){
						case cm_normal:
							conf.change_effect = cm_fade;
							break;
						case cm_fade:
							conf.change_effect = cm_crossfade;
							break;
						case cm_crossfade:
							conf.change_effect = cm_horizontal_roll;
							break;
						case cm_horizontal_roll:
							conf.change_effect = cm_vertical_roll;
							break;
						case cm_vertical_roll:
							conf.change_effect = cm_draw;
							break;
						case cm_draw:
							conf.change_effect = cm_normal;
							break;
					}
					break;
				
				case brightness:
					conf.brightness_setting++;
					if(conf.brightness_setting > 9) conf.brightness_setting=0;
					break;

				case brightness_auto:
					conf.brightness_auto++;
					if(conf.brightness_auto > 1) conf.brightness_auto=0;
					break;
				
				case auto_onoff:
					conf.auto_onoff++;
					if(conf.auto_onoff>1) conf.auto_onoff = 0;
					break;

				case on_time:
					time_add_hm(&conf.auto_on_time, cursor_setting);
					break;

				case off_time:
					time_add_hm(&conf.auto_off_time, cursor_setting);
					break;

				default:
					break;
			}
			break;
		case mode_timeadjust:
			timeadjust_add(&timeadjust_datetime);
			break;
		case mode_random_idle:
		case mode_random:
			break;
		case mode_demo:

			break;
	}
}

void swb_long_push(void)
{
	switch(opmode){
		case mode_clock:
		case mode_clock_calendar:
			opmode = mode_random_idle;
			break;
		case mode_settings:
			
			break;
		case mode_timeadjust:

			break;
		case mode_random_idle:
		case mode_random:
			opmode = mode_demo;
			break;
		case mode_demo:

			break;
	}
}

void swc_short_push(void)
{
	switch(opmode){
		case mode_clock:
		case mode_clock_calendar:
			
			break;
		case mode_settings:
			num_setting++;
			cursor_setting = 0;
			if(num_setting > MAX_NUM_SETTING){
				num_setting = 0;
			}
			break;
		case mode_timeadjust:
			if(cursor_timeadjust==cursor_timeadjust_MAX){
				cursor_timeadjust=0;
			}else{
				cursor_timeadjust++;
			}
			break;
		case mode_random_idle:
		case mode_random:
		case mode_demo:

			break;
	}
}

void swc_long_push(void)
{
	switch(opmode){
		case mode_clock:
		case mode_clock_calendar:
			opmode = mode_timeadjust;
			cursor_timeadjust = 0;
			timeadjust_datetime.all = now_datetime.all;		// now_datetimeは変えないようにする
			prev_timeadjust_datetime.all = timeadjust_datetime.all;
			break;
		case mode_settings:

			break;
		case mode_timeadjust:

			break;
		case mode_random_idle:
		case mode_random:
		case mode_demo:

			break;
	}
}

void timeadjust_add(rtc_time *time_add)
{
	uint16_t sec1,sec2,min1,min2,hour1,hour2,day1,day2,month1,month2,year1,year2;
	uint8_t month = time_add->separate.month1 + time_add->separate.month2*10;
	uint8_t day = time_add->separate.day1 + time_add->separate.day2*10;

	switch (cursor_timeadjust)
	{
		case 0:
			// sec 1
			time_add->separate.sec1++;
			if(time_add->separate.sec1>9) time_add->separate.sec1 = 0;
			break;
		
		case 1:
			// sec 2
			time_add->separate.sec2++;
			if(time_add->separate.sec2>5) time_add->separate.sec2 = 0;
			break;
		
		case 2:
			// min 1
			time_add->separate.min1++;
			if(time_add->separate.min1>9) time_add->separate.min1 = 0;
			break;
		
		case 3:
			// min 2
			time_add->separate.min2++;
			if(time_add->separate.min2>5) time_add->separate.min2 = 0;
			break;

		case 4:
			// hour 1
			time_add->separate.hour1++;
			if(time_add->separate.hour2 > 1){
				if(time_add->separate.hour1 > 3) time_add->separate.hour1 = 0;
			}else{
				if(time_add->separate.hour1 > 9) time_add->separate.hour1 = 0;
			}
			break;
		
		case 5:
			// hour 2
			time_add->separate.hour2++;
			if(time_add->separate.hour1 > 3){
				if(time_add->separate.hour2 > 1) time_add->separate.hour2 = 0;
			}else{
				if(time_add->separate.hour2 > 2) time_add->separate.hour2 = 0;
			}
			break;

		case 6:
			// day 1
			time_add->separate.day1++;
			switch (month){
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					if(time_add->separate.day2 > 2){
						if(time_add->separate.day1 > 1) time_add->separate.day1 = 0;
					}else{
						if(time_add->separate.day1 > 9) time_add->separate.day1 = 0;
					}
					break;
				case 4:
				case 6:
				case 9:
				case 11:
					if(time_add->separate.day2 > 2){
						time_add->separate.day1 = 0;
					}else{
						if(time_add->separate.day1 > 9) time_add->separate.day1 = 0;
					}
					break;
				case 2:
					// leap year check
					if((time_add->separate.year1 + time_add->separate.year2*10)%4 == 0){
						if(time_add->separate.day1 > 9) time_add->separate.day1 = 0;
					}else{
						if(time_add->separate.day2 > 1){
							if(time_add->separate.day1 > 8) time_add->separate.day1 = 0;
						}else{
							if(time_add->separate.day1 > 9) time_add->separate.day1 = 0;
						}
					}
			}
			break;

		case 7:
			// day 2
			time_add->separate.day2++;
			switch (month){
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					if(time_add->separate.day1 > 1){
						if(time_add->separate.day2 > 2) time_add->separate.day2 = 0;
					}else{
						if(time_add->separate.day2 > 3) time_add->separate.day2 = 0;
					}
					break;
				case 4:
				case 6:
				case 9:
				case 11:
					if(time_add->separate.day1 > 0){
						if(time_add->separate.day2 > 2) time_add->separate.day2 = 0;
					}else{
						if(time_add->separate.day2 > 3) time_add->separate.day2 = 0;
					}
					break;
				case 2:
					// leap year check
					if((time_add->separate.year1 + time_add->separate.year2*10)%4 == 0){
						if(time_add->separate.day2 > 2) time_add->separate.day2 = 0;
					}else{
						if(time_add->separate.day1 > 8){
							if(time_add->separate.day2 > 2) time_add->separate.day2 = 0;
						}else{
							if(time_add->separate.day2 > 3) time_add->separate.day2 = 0;
						}
					}
			}
			break;
		
		case 8:
			// month 1
			time_add->separate.month1++;

			if(time_add->separate.month2 > 0){
				if(time_add->separate.month1 > 2) time_add->separate.month1 = 0;
			}else{
				if(time_add->separate.month1 > 9) time_add->separate.month1 = 1;
			}

			month = time_add->separate.month1 + time_add->separate.month2 * 10;

			switch (month)
			{
				case 1:
				case 3:
				case 5:
				case 7:
				case 8:
				case 10:
				case 12:
					// pass
					break;
				case 4:
					if(day > 30) time_add->separate.month1 = 5;
					break;
				case 6:
					if(day > 30) time_add->separate.month1 = 7;
					break;
				case 9:
					if(day > 30) time_add->separate.month1 = 1;
					break;
				case 11:
					if(day > 30) time_add->separate.month1 = 0;
					break;
				case 2:
					// leap year check
					if((time_add->separate.year1 + time_add->separate.year2*10)%4 == 0){
						if(day > 28) time_add->separate.month1 = 3;
					}else{
						if(day > 27) time_add->separate.month1 = 3;
					}
					break;
			}

			break;

		case 9:
			// month 2
			time_add->separate.month2++;

			if(time_add->separate.month1 > 2){
				if(time_add->separate.month2 > 0) time_add->separate.month2 = 0;
			}else if(time_add->separate.month1 == 0){
				if(time_add->separate.month2 > 1) time_add->separate.month2 = 1;
			}else{
				if(time_add->separate.month2 > 1) time_add->separate.month2 = 0;
			}

			month = time_add->separate.month1 + time_add->separate.month2 * 10;

			switch (month)
			{
				case 10:
				case 12:
					// pass
					break;
				case 11:
					if(day > 30) time_add->separate.month2 = 0;
					break;
			}
			break;

		case 10:
			// year 1
			time_add->separate.year1++;
			if(time_add->separate.year1>9) time_add->separate.year1 = 0;

			if((time_add->separate.year1 + time_add->separate.year2 * 10)%4 != 0){
				if(time_add->bcd.month == 0x02){
					if(time_add->bcd.day == 0x29){
						// not leap-year, but Feb 29
						while((time_add->separate.year1 + time_add->separate.year2 * 10)%4 != 0){
							time_add->separate.year1++;
						}
					}
				}
			}
			break;

		case 11:
			// year 2
			time_add->separate.year2++;
			if(time_add->separate.year2>9) time_add->separate.year2 = 0;

			if((time_add->separate.year1 + time_add->separate.year2 * 10)%4 != 0){
				if(time_add->bcd.month == 0x02){
					if(time_add->bcd.day == 0x29){
						// not leap-year, but Feb 29
						while((time_add->separate.year1 + time_add->separate.year2 * 10)%4 != 0){
							time_add->separate.year2++;
						}
					}
				}
			}
			break;
	}

}


bool flash_write(uint8_t *write_data){

    // Note that a whole number of sectors must be erased at a time.
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, write_data, FLASH_PAGE_SIZE);

    bool mismatch = false;
    int i;
    for (i = 0; i < FLASH_PAGE_SIZE; i++) {
        if (write_data[i] != flash_target_contents[i]){
            mismatch = true;
        }
    }
    if (mismatch)
        return false;
    else
        return true;
}

void flash_read(uint8_t *read_data){
    int i;

    for(i=0;i<FLASH_PAGE_SIZE;i++){
        read_data[i] = flash_target_contents[i];
    }
}

void parameter_backup(void){
	flash_data.config.auto_off_time.all = conf.auto_off_time.all;
	flash_data.config.auto_on_time.all = conf.auto_on_time.all;
	flash_data.config.auto_onoff = conf.auto_onoff;
	flash_data.config.brightness = conf.brightness_setting;
	flash_data.config.brightness_auto = conf.brightness_auto;
	flash_data.config.change_effect = conf.change_effect;
	flash_data.config.flash_initialized = 0x5A;
	flash_write(flash_data.flash_byte);
}

//---- time add process(only hour/min)
void time_add_hm(rtc_time *time_rtc, uint8_t cursor_time){
	switch(cursor_time){
		case 0:
			time_rtc->separate.min1++;
			if(time_rtc->separate.min1>9) time_rtc->separate.min1 = 0;
			break;
		case 1:
			time_rtc->separate.min2++;
			if(time_rtc->separate.min2>5) time_rtc->separate.min2 = 0;
			break;
		case 2:
			time_rtc->separate.hour1++;
			if(time_rtc->separate.hour2 < 2){
				// max value is 9 (09/19)
				if(time_rtc->separate.hour1 > 9) time_rtc->separate.hour1 = 0;
			}else{
				// max value is 3 (23)
				if(time_rtc->separate.hour1 > 3) time_rtc->separate.hour1 = 0;
			}
			break;
		case 3:
			time_rtc->separate.hour2++;
			if(time_rtc->separate.hour1 < 4){
				// max value is 2 (03/13/23)
				if(time_rtc->separate.hour2 > 2) time_rtc->separate.hour2 = 0;
			}else{
				// max value is 1 (04/14)
				if(time_rtc->separate.hour2 > 1) time_rtc->separate.hour2 = 0;
			}
			break;
	}	
}