#include "display_control.h"

//--------------------------------------
// private variable
//--------------------------------------
static Dynamic_Control dcnt_dynamic_control;
static Change_effect dcnt_change_effect;
static uint16_t dcnt_buf_seg_previous[NUM_DIGIT];
static uint16_t dcnt_buf_seg_next[NUM_DIGIT];
static uint16_t dcnt_duty[NUM_DIGIT];
static bool dcnt_seg_change_flg[NUM_DIGIT];
static uint16_t dcnt_switch_counter[NUM_DIGIT];
static bool dcnt_change_flg[NUM_DIGIT];

static bool dcnt_duty_set_flg;


const uint16_t table_seg_vroll_seg1[11][5] = 
    {
        {       // 0
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_E_MASK|SEG_D_MASK|SEG_F_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK,
            SEG_D_MASK
        },{     // 1
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_C_MASK,
            SEG_C_MASK,
            0
        },{     // 2
            SEG_A_MASK|SEG_B_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_D_MASK|SEG_E_MASK,
            SEG_D_MASK
        },{     // 3
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_D_MASK||SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK,
            SEG_D_MASK
        },{     // 4
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_G_MASK,
            SEG_C_MASK,
            0
        },{     // 5
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK,
            SEG_D_MASK
        },{     // 6
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK,
            SEG_D_MASK
        },{     // 7
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_C_MASK,
            SEG_C_MASK,
            0
        },{     // 8
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_E_MASK,
            SEG_D_MASK
        },{     // 9
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_C_MASK|SEG_D_MASK,
            SEG_D_MASK
        },{     // 例外処理
            0,0,0,0,0
        }
    };

const uint16_t table_seg_vroll_seg2[11][5] = 
    {
        {       // 0
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK
        },{     // 1
            0,
            SEG_B_MASK,
            SEG_B_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 2
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK
        },{     // 3
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK
        },{     // 4
            0,
            SEG_B_MASK|SEG_F_MASK,
            SEG_B_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 5
            SEG_A_MASK,
            SEG_A_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 6
            SEG_A_MASK,
            SEG_A_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 7
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK
        },{     // 8
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 9
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 例外処理
            0,0,0,0,0
        }
    };

const uint16_t table_seg_hroll_seg1[10][3] = 
    {
        {       // 0
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 1
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 2
            SEG_A_MASK|SEG_B_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_B_MASK
        },{     // 3
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 4
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 5
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_C_MASK
        },{     // 6
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_C_MASK
        },{     // 7            
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 8
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 9
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK
        }
    };

const uint16_t table_seg_hroll_seg2[10][3] = 
    {
        {       // 0
            SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK
        },{     // 1
            0,
            0,
            SEG_B_MASK|SEG_C_MASK
        },{     // 2
            SEG_E_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_D_MASK|SEG_E_MASK|SEG_G_MASK
        },{     // 3
            0,
            SEG_A_MASK|SEG_D_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_G_MASK
        },{     // 4
            SEG_F_MASK,
            SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_C_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 5
            SEG_F_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 6
            SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 7            
            0,
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK
        },{     // 8
            SEG_E_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_F_MASK|SEG_G_MASK
        },{     // 9
            SEG_F_MASK,
            SEG_A_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK|SEG_D_MASK|SEG_F_MASK|SEG_G_MASK
        }
    };

const uint16_t table_seg_draw[11][7] = 
    {
        {       // 0
            SEG_A_MASK, 
            SEG_A_MASK|SEG_F_MASK, 
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK, 
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK, 
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK, 
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK|SEG_C_MASK|SEG_B_MASK
        },{     // 1
            SEG_B_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_C_MASK
        },{     // 2
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_E_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_E_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_E_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_E_MASK|SEG_D_MASK
        },{     // 3
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK
        },{     // 4
            SEG_F_MASK,
            SEG_F_MASK|SEG_G_MASK,
            SEG_F_MASK|SEG_G_MASK|SEG_B_MASK,
            SEG_F_MASK|SEG_G_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_F_MASK|SEG_G_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_F_MASK|SEG_G_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_F_MASK|SEG_G_MASK|SEG_B_MASK|SEG_C_MASK
        },{     // 5
            SEG_A_MASK,
            SEG_A_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK
        },{     // 6
            SEG_A_MASK,
            SEG_A_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK|SEG_C_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_E_MASK|SEG_D_MASK|SEG_C_MASK|SEG_G_MASK
        },{     // 7
            SEG_A_MASK,
            SEG_A_MASK|SEG_B_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_B_MASK|SEG_C_MASK
        },{     // 8
            SEG_A_MASK,
            SEG_A_MASK|SEG_F_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK,
            SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK|SEG_E_MASK|SEG_B_MASK
        },{     // 9
            SEG_B_MASK,
            SEG_B_MASK|SEG_A_MASK,
            SEG_B_MASK|SEG_A_MASK|SEG_F_MASK,
            SEG_B_MASK|SEG_A_MASK|SEG_F_MASK|SEG_G_MASK,
            SEG_B_MASK|SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK,
            SEG_B_MASK|SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK,
            SEG_B_MASK|SEG_A_MASK|SEG_F_MASK|SEG_G_MASK|SEG_C_MASK|SEG_D_MASK
        },{     // 例外処理
            0,0,0,0,0,0,0
        }
    };


//--------------------------------------
// private function
//--------------------------------------
uint8_t seg_to_num(uint16_t seg){
    switch (seg&0x7F)
    {
    case 0b00111111:
        return 0;
    case 0b00000110:
        return 1;
    case 0b01011011:
        return 2;
    case 0b01001111:
        return 3;
    case 0b01100110:
        return 4;
    case 0b01101101:
        return 5;	
    case 0b01111101:
        return 6;		
	case 0b00000111:
        return 7;
	case 0b01111111:
        return 8;	
	case 0b01101111:
        return 9;
    default:
        return 10;
    }
}

//--------------------------------------
// public function
//--------------------------------------
// function : dcnt_init(void)
// argument:
//      None
// return:
//      None
// description:
//      Initialize variables.
static void dcnt_init(void)
{
    uint8_t i;

    dcnt_duty_set_flg = false;

    dcnt_dynamic_control = new_dynamic_control();
    dcnt_dynamic_control.init();
    
    dcnt_change_effect = cm_normal;

    for(i=0;i<NUM_DIGIT;i++){
        dcnt_buf_seg_next[i] = 0;
        dcnt_buf_seg_previous[i] = 0;
        dcnt_seg_change_flg[i] = false;
        dcnt_change_flg[i] = false;
        dcnt_switch_counter[i] = 0;
    }
    
}

// function : dcnt_task
// argument:
//      None
// return:
//      None
// description:
//      Display task
static void dcnt_task(void)
{
    uint8_t i;

    dcnt_dynamic_control.task();

    switch(dcnt_change_effect){
        // 数字更新のエフェクト：なし
        case cm_normal:
            for(i=0;i<NUM_DIGIT;i++){
                dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);
            }
            break;

        // 数字更新のエフェクト：フェード
        case cm_fade:
            for(i=0;i<NUM_DIGIT;i++){
                if(dcnt_change_flg[i] == true){
                    dcnt_switch_counter[i]++;

                    if(dcnt_switch_counter[i] < 129){
                        dcnt_dynamic_control.set_duty(i, (uint16_t)((uint64_t)dcnt_duty[i]*(128-(uint64_t)dcnt_switch_counter[i])>>7));

                    }else if(dcnt_switch_counter[i] < 257){
                        dcnt_dynamic_control.set_duty(i, (uint16_t)((uint64_t)dcnt_duty[i]*((uint64_t)dcnt_switch_counter[i]-128)>>7));
                        dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                    }else{
                        dcnt_switch_counter[i] = 0;
                        dcnt_change_flg[i] = false;
                    }
                }else{
                    dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                    dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);
                }

            }
            break;

        // 数字更新のエフェクト：クロスフェード
        case cm_crossfade:
            for(i=0;i<NUM_DIGIT;i++){
                dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);
                if(dcnt_change_flg[i] == true){
                    dcnt_dynamic_control.set_seg2_ratio(i, dcnt_switch_counter[i]);
                    dcnt_dynamic_control.set_seg2(i, dcnt_buf_seg_next[i]);
                }else{
                    dcnt_dynamic_control.set_seg2_ratio(i, 0);
                    dcnt_dynamic_control.set_seg2(i, 0x00);

                    dcnt_buf_seg_previous[i] = dcnt_buf_seg_next[i];
                    dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_previous[i]);
                }

                if(dcnt_change_flg[i] == true){
                    if(dcnt_switch_counter[i]>=127){
                        dcnt_switch_counter[i]=0;
                        dcnt_change_flg[i]=false;
                    }else{
                        dcnt_switch_counter[i]++;
                    }
                }

            }
            break;

        case cm_draw:
            for(i=0;i<NUM_DIGIT;i++){
                dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);

                if(dcnt_change_flg[i] == true){
                    dcnt_switch_counter[i]++;

                    if(dcnt_switch_counter[i]<100){
                        dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]&0xFF80);
                    }else if(dcnt_switch_counter[i]<140){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][0] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<180){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][1] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<220){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][2] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<260){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][3] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<300){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][4] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<340){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][5] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<380){
                        dcnt_dynamic_control.set_seg1(i, table_seg_draw[seg_to_num(dcnt_buf_seg_next[i])][6] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else{
                        dcnt_switch_counter[i] = 0;
                        dcnt_change_flg[i] = false;
                    }
                }else{
                    dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                }
            }

            break;

        case cm_horizontal_roll:
            for(i=0;i<NUM_DIGIT;i++){
                dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);

                if(dcnt_change_flg[i] == true){
                    dcnt_switch_counter[i]++;
                    
                    if(dcnt_switch_counter[i]<70){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][0] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<140){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][1] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<210){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][2] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<280){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][0] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<350){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][1] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<420){
                        dcnt_dynamic_control.set_seg1(i, table_seg_hroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][2] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else{
                        dcnt_switch_counter[i] = 0;
                        dcnt_change_flg[i] = false;
                    }
                }else{
                    dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                    dcnt_buf_seg_previous[i] = dcnt_buf_seg_next[i];
                }
            }
            break;

        

        // 数字更新のエフェクト：垂直ロール
        case cm_vertical_roll:
            for(i=0;i<NUM_DIGIT;i++){
                dcnt_dynamic_control.set_duty(i, dcnt_duty[i]);

                if(dcnt_change_flg[i] == true){
                    dcnt_switch_counter[i]++;
                    
                    if(dcnt_switch_counter[i]<30){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][0] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<60){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][1] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<90){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][2] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<120){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][3] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<150){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg1[seg_to_num(dcnt_buf_seg_previous[i])][4] | (dcnt_buf_seg_previous[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<180){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][0] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<210){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][1] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<240){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][2] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<270){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][3] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else if(dcnt_switch_counter[i]<300){
                        dcnt_dynamic_control.set_seg1(i, table_seg_vroll_seg2[seg_to_num(dcnt_buf_seg_next[i])][4] | (dcnt_buf_seg_next[i]&0xFF80));
                    }else{
                        dcnt_switch_counter[i] = 0;
                        dcnt_change_flg[i] = false;
                    }
                }else{
                    dcnt_dynamic_control.set_seg1(i, dcnt_buf_seg_next[i]);
                    dcnt_buf_seg_previous[i] = dcnt_buf_seg_next[i];
                }
            }
            break;
    }
}



// function : dcnt_set_effect
// argument:
//      effect : display change effect
// return:
//      None
// description:
//      Set the display change effect
static void dcnt_set_effect(Change_effect effect)
{
    dcnt_change_effect = effect;
}

// function : dcnt_get_effect
// argument:
//      None
// return:
//      effect : display change effect
// description:
//      Get the display change effect
static Change_effect dcnt_get_effect(void)
{
    return dcnt_change_effect;
}

// function : dcnt_set_seg
// argument:
//      digit : selected digit
//      seg : display segment value
// return:
//      None
// description:
//      Set the display segment
static void dcnt_set_seg(uint8_t digit, uint16_t seg)
{
    dcnt_buf_seg_next[digit] = seg;
}

// function : dcnt_set_duty
// argument:
//      digit : selected digit
//      duty : duty value
// return:
//      None
// description:
//      Set the display duty
static void dcnt_set_duty(uint8_t digit, uint16_t duty)
{
    dcnt_duty[digit] = duty;
}

// function : dcnt_set_change_flg
// argment:
//      digit : selected digit
// return:
//      None
// description:
//      Set the change flag
static void dcnt_set_change_flg(uint8_t digit)
{
    dcnt_change_flg[digit] = true;
}

//--------------------------------------
// Constructor
//--------------------------------------
Display_Control new_display_control(void)
{
    return ((Display_Control){
        .init = dcnt_init,
        .task = dcnt_task,
        .set_effect = dcnt_set_effect,
        .get_effect = dcnt_get_effect,
        .set_seg = dcnt_set_seg,
        .set_duty = dcnt_set_duty,
        .set_change_flg = dcnt_set_change_flg
    });
}