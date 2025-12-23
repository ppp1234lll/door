#ifndef __ADC_H
#define __ADC_H
#include "bsp.h"

//ADC转换通道和对应的引脚
typedef enum
{
    ACN_POWER_ADAPTER01 = 1, // 适配器1 - 220V
    ACN_POWER_ADAPTER02 = 3, // 适配器2 - 220V
    ACN_POWER_ADAPTER03 = 2, // 适配器3 - 220V
	ACN_VIN_220V		= 8,
    ACN_TOTAL_CURRENT 	= 9,
    ACN_FILL_LIGHT  	= 10,
	
    ACN_PM25        	= 4, 
    ACN_HUMIDITY    	= 5,
    ACN_TEMP        	= 6,
    ACN_WIND        	= 7,
} ADC_CHANNEL_NUM;


typedef struct {
    uint16_t adc_power_adapter01;
    uint16_t adc_power_adapter02;
    uint16_t adc_power_adapter03;
    uint16_t adc_pm25;
    uint16_t adc_humidity;
    uint16_t adc_temperature;
    uint16_t adc_wind_force;
    uint16_t adc_vin_220v;
    uint16_t adc_total_current;
    uint16_t adc_fill_light;             
} adc_value_t;


void my_adc_init(void);
void Rheostat_ADC_Mode_Config(void);
void adc_deal_data_function(void);
adc_value_t adc_get_collection_data_function(void);
void adc_get_timer_function(void);
void adc_cacke_data_timer(void);

#endif 
