/*
 * tiny_rtc.h
 *
 * Created: 8/28/2020 8:03:09 PM
 *  Author: stomp
 */ 


#ifndef TINY_RTC_H_
#define TINY_RTC_H_

void rtc_start(uint16_t period, uint8_t prescaler);
void rtc_start_cmp(uint16_t period, uint16_t compare, uint8_t prescaler);
void rtc_stop();



#endif /* TINY_RTC_H_ */