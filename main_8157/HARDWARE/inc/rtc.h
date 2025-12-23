#ifndef __RTC_H
#define __RTC_H
#include "sys.h"

typedef struct
{
	uint16_t year;
	uint8_t  month;
	uint8_t  data;
	uint8_t  week;
	uint8_t  hour;
	uint8_t  min;
	uint8_t  sec;
} rtc_time_t;

uint8_t RTC_Init(void);
void RTC_Get_Time(rtc_time_t *rtc);
void RTC_set_Time(rtc_time_t rtc);
void TimeBySecond(u32 second);
void time_to_second_function(uint32_t *time, uint32_t *second);

void RTC_Get_Time_Test(void);

#endif

















