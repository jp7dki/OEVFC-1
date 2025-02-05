#include "rtc_rx8900.h"

//--------------------------------------
// private variable
//--------------------------------------


//--------------------------------------
// private function
//--------------------------------------
int rx8900_read_reg(uint8_t addr, uint8_t *data){
    int i2c_result;
    i2c_result = i2c_write_timeout_us(RTC_I2C_NUM, RTC_ADDR, &addr, 1, true, 600);
    if((i2c_result == PICO_ERROR_TIMEOUT) || (i2c_result == PICO_ERROR_GENERIC)){
        return i2c_result;
    }
    i2c_result = i2c_read_timeout_us(RTC_I2C_NUM, RTC_ADDR, data, 1, false, 600);
    return i2c_result;
}

int rx8900_read_reg_n(uint8_t addr, uint8_t *data, uint8_t data_num){
    int i2c_result;
    i2c_result = i2c_write_timeout_us(RTC_I2C_NUM, RTC_ADDR, &addr, 1, true, 600);
    if((i2c_result == PICO_ERROR_TIMEOUT) || (i2c_result == PICO_ERROR_GENERIC)){
        return i2c_result;
    }
    i2c_result = i2c_read_timeout_us(RTC_I2C_NUM, RTC_ADDR, data, data_num, false, 600);
    return i2c_result;
}

int rx8900_write_reg(uint8_t addr, uint8_t data){
    
    uint8_t i2c_data[2];

    // Counter reset to avoid countup within setting datetime.
    i2c_data[0] = addr;         // Write Register address
    i2c_data[1] = data;         // Write data
    return i2c_write_timeout_us(RTC_I2C_NUM, RTC_ADDR, i2c_data, 2, false, 1000);
}

//--------------------------------------
// public function
//--------------------------------------
static void rx8900_init(void)
{
    i2c_init(RTC_I2C_NUM, RTC_BAUD);
    gpio_init(RTC_SDA_PIN);
    gpio_init(RTC_SCL_PIN);
    gpio_set_function(RTC_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(RTC_SCL_PIN, GPIO_FUNC_I2C);

/*    rx8900_write_reg(0x0D, 0x08);
    rx8900_write_reg(0x0E, 0x00);
    rx8900_write_reg(0x0F, 0xC0);*/
//    bi_decl(bi_2pins_with_func(RTC_SDA_PIN, RTC_SCL_PIN, GPIO_FUNC_I2C));
}

static void rx8900_hard_init(void)
{
    rx8900_write_reg(0x0D, 0x08);
    rx8900_write_reg(0x0E, 0x00);
    rx8900_write_reg(0x0F, 0xC0);
    rx8900_write_reg(RX8900_REG_YEAR, 0x25);
    rx8900_write_reg(RX8900_REG_MONTH, 1);
    rx8900_write_reg(RX8900_REG_DAY, 1);
    rx8900_write_reg(RX8900_REG_WEEK, 1);
    rx8900_write_reg(RX8900_REG_HOUR, 0);
    rx8900_write_reg(RX8900_REG_MIN, 0);
    rx8900_write_reg(RX8900_REG_SEC, 0);
    rx8900_write_reg(0x0F, 0xC1);

}

static int rx8900_get_time(rtc_time *time)
{
    int result;
    uint8_t data[7];
    result = rx8900_read_reg_n(RX8900_REG_SEC, data, 7);

    time->bcd.sec = data[0];
    time->bcd.min = data[1];
    time->bcd.hour = data[2];
    time->bcd.weekday = data[3];
    time->bcd.day = data[4];
    time->bcd.month = data[5];
    time->bcd.year = data[6];
    time->bcd.dummy = 0;
    
    /*
    result =  rx8900_read_reg(RX8900_REG_YEAR,  &(time->bcd.year)   );
    result |= rx8900_read_reg(RX8900_REG_MONTH, &(time->bcd.month)  );
    result |= rx8900_read_reg(RX8900_REG_DAY,   &(time->bcd.day)    );
    result |= rx8900_read_reg(RX8900_REG_WEEK,  &(time->bcd.weekday));
    result |= rx8900_read_reg(RX8900_REG_HOUR,  &(time->bcd.hour)   );
    result |= rx8900_read_reg(RX8900_REG_MIN,   &(time->bcd.min)    );
    result |= rx8900_read_reg(RX8900_REG_SEC,   &(time->bcd.sec)    );
    */
    return result;
}

static int rx8900_set_time(rtc_time time)
{
    int result;

    result =  rx8900_write_reg(RX8900_REG_SEC,   time.bcd.sec    );
    result |= rx8900_write_reg(RX8900_REG_MIN,   time.bcd.min    );
    result |= rx8900_write_reg(RX8900_REG_HOUR,  time.bcd.hour   );
    result |= rx8900_write_reg(RX8900_REG_WEEK,  time.bcd.weekday);
    result |= rx8900_write_reg(RX8900_REG_DAY,   time.bcd.day    );
    result |= rx8900_write_reg(RX8900_REG_MONTH, time.bcd.month  );
    result |= rx8900_write_reg(RX8900_REG_YEAR,  time.bcd.year   );
    result |= rx8900_write_reg(0x0F, 0xC1);

    return result;
}

static bool rx8900_is_voltage_low(void){
    uint8_t result;
    rx8900_read_reg(0x0E, &result);

    if((result&0x02)!=0){
        return true;
    }else{
        return false;
    }
}


//--------------------------------------
// Constructor
//--------------------------------------
RtcRx8900 new_RtcRx8900(void)
{
    return ((RtcRx8900){
        .init = rx8900_init,
        .hard_init = rx8900_hard_init,
        .get_time = rx8900_get_time,
        .set_time = rx8900_set_time,
        .is_voltage_low = rx8900_is_voltage_low
    });
}