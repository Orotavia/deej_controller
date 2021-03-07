#include <avr/io.h>
#include <stdint.h>


void rtc_start(uint16_t period, uint8_t prescaler){	
	while(RTC.STATUS > 0);
	/*
	RTC.INTCTRL &= ~RTC_OVF_bm;
	RTC.CTRLA |= RTC_RTCEN_bm;
	
	while(RTC.STATUS > 0);
	RTC.PER = period-1;
	RTC.CTRLA = RTC_RUNSTDBY_bm | (prescaler) | RTC_RTCEN_bm;
	
	RTC.INTCTRL = RTC_OVF_bm;
	RTC.CNT = 0;
	*/
	
	
	//RTC.INTCTRL &= ~RTC_OVF_bm;
	RTC.PER = period-1;
	RTC.CNT = 0;
	RTC.INTCTRL = RTC_OVF_bm;
	RTC.CTRLA = RTC_RUNSTDBY_bm | (prescaler) | RTC_RTCEN_bm;
}

void rtc_start_cmp(uint16_t period, uint16_t compare, uint8_t prescaler){	
	while(RTC.STATUS > 0);
	//RTC.INTCTRL &= ~RTC_OVF_bm;
	RTC.CMP = compare;
	RTC.PER = period-1;
	RTC.CNT = 0;
	RTC.INTCTRL = RTC_OVF_bm | RTC_CMP_bm;
	RTC.CTRLA = RTC_RUNSTDBY_bm | (prescaler) | RTC_RTCEN_bm;
}

void rtc_stop(){
	while(RTC.STATUS > 0);
	RTC.CTRLA &= ~(RTC_RTCEN_bm);
	RTC.INTCTRL &= ~(RTC_OVF_bm | RTC_CMP_bm);
}