/*
 * deej_controller.c
 *
 * Created: 9/27/2020 7:20:21 PM
 * Author : stomp
 */ 

// On Port C
#define EN0_A (1<<0)
#define EN0_B (1<<1)
#define EN0_SW (1<<2)
#define EN1_SW (1<<3)

// On Port B
#define EN2_SW (1<<4)
#define EN3_SW (1<<5)

// On Port A
#define EN1_A (1<<1)
#define EN1_B (1<<2)
#define EN3_A (1<<3)
#define EN3_B (1<<4)
#define EN2_A (1<<5)
#define EN2_B (1<<6)

#define SHIFT_CLK (1<<0)
#define LATCH_CLK (1<<1)
#define SR_DATA (1<<7)

#define mem_clickRate ((uint8_t *)5)
#define mem_saveDelay ((uint8_t *)6)
#define mem_updatePeriod ((uint16_t *)7)
// address 8 not available due to period being 16-bit word

//static const char msg_clickSave[] = "Volume will increment %d per click\n>";

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "usart_lib.h"
//#include "tiny_clk_setup.h"
#include "tiny_rtc.h"

volatile int8_t load_flag[] = {0, 0, 0, 0};
volatile int32_t encoder_accum[] = {0, 0, 0, 0};
volatile uint8_t mute[] = {0xFF, 0xFF, 0xFF, 0xFF};
	
volatile uint8_t chg_flag = 0;


uint8_t enc_inc = 2;
uint8_t save_delay = 10;
uint16_t update_period = 500;

volatile uint8_t rx_index;
volatile uint8_t rx_word;

volatile uint8_t save_flag = 0;

char rx_message[32];
char tx_message[32];
char * arg;
uint8_t argD = 0;

uint32_t parser(char *in_string, uint8_t argc){
	uint8_t parse_index = 0;
	uint8_t match_count = 0;
	char token_buf[6] = "";
	uint32_t token_index = 0;
	
	while(match_count < (argc+1)){
		while((in_string[parse_index++] != ' ') && parse_index < 32);
		if(parse_index < 32)
			match_count++;
	}
	
	while((in_string[parse_index] !=  ' ') && (in_string[parse_index] != '\n') && (in_string[parse_index] != 0)){
		token_buf[token_index++] = in_string[parse_index++];
	}
	token_index = strtol(token_buf, NULL, 10);
	return token_index;
	
}

void updateAllFromMemory(){
	enc_inc = eeprom_read_byte(mem_clickRate);
	save_delay = eeprom_read_byte(mem_saveDelay);
	update_period = eeprom_read_word(mem_updatePeriod);
	for(uint16_t i = 0; i < 4; i++){
		encoder_accum[i] = eeprom_read_byte((uint8_t*)i);
	}
}

void shiftWrite5(uint64_t data){
	for(uint8_t i=0; i<(8*5); i++){
		if(data & 0x01){
			PORTA.OUTSET = SR_DATA;
		} else{
			PORTA.OUTCLR = SR_DATA;
		}
		
		PORTB.OUTSET = SHIFT_CLK;
		_delay_us(10);
		PORTB.OUTCLR = SHIFT_CLK;
		
		data=data>>1;  //Now bring next bit at MSB position
	}
	PORTB.OUTSET = LATCH_CLK;
	_delay_us(10);
	PORTB.OUTCLR = LATCH_CLK;
}

uint64_t percentToLED(uint8_t in){
	switch((in+9)/10){
		case 0:
			return 0x000;
			break;
		case 1:
			return 0x001;
			break;
		case 2:
			return 0x003;
			break;
		case 3:
			return 0x007;
			break;
		case 4:
			return 0x00F;
		case 5:
			return 0x01F;
			break;
		case 6:
			return 0x03F;
			break;
		case 7:
			return 0x07F;
			break;
		case 8:
			return 0x0FF;
			break;
		case 9:
			return 0x1FF;
			break;
		case 10:
			return 0x3FF;
			break;
		default:
			return 0x000;
			break;
	}
		
}

void updateLEDs(){
	uint64_t led_val = (percentToLED(encoder_accum[0]&mute[0]) << 30) | (percentToLED(encoder_accum[1]&mute[1]) << 20) | (percentToLED(encoder_accum[2]&mute[2]) << 10) | (percentToLED(encoder_accum[3]&mute[3]) << 0);
	shiftWrite5( led_val );
}

void startTimerA(uint16_t top){
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.PER = (uint32_t)top*(F_CPU/1024)/1000;
	TCA0.SINGLE.CNT = 0;
	TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV1024_gc;
}

void stopTimerA(){
	TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;	//clear flag
}

int main(void){
	//_delay_ms(1000);
    usart_init(F_CPU, 9600);
	//usart_tx_string("begin\n>");		// REPLACE WITH reads from EEPROM
	
	updateAllFromMemory();

	PORTA.DIRSET = SR_DATA;
	PORTB.DIRSET = SHIFT_CLK | LATCH_CLK;

	PORTA.DIRCLR = EN1_A | EN1_B | EN2_A | EN2_B | EN3_A | EN3_B;
	PORTB.DIRCLR = EN2_SW | EN3_SW;
	PORTC.DIRCLR = EN0_A | EN0_B | EN0_SW | EN1_SW;
	
	PORTA.PIN1CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN1 A
	PORTA.PIN2CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN1 B
	PORTA.PIN3CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN3 A
	PORTA.PIN4CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN3 B
	PORTA.PIN5CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN2 A
	PORTA.PIN6CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN2 B
	
	PORTC.PIN0CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN0 A
	PORTC.PIN1CTRL |= PORT_ISC_BOTHEDGES_gc;	//EN0 B
	
	PORTC.PIN2CTRL |= PORT_ISC_FALLING_gc | PORT_PULLUPEN_bm;	//EN0 SW
	PORTC.PIN3CTRL |= PORT_ISC_FALLING_gc | PORT_PULLUPEN_bm;	//EN1 SW
	PORTB.PIN4CTRL |= PORT_ISC_FALLING_gc | PORT_PULLUPEN_bm;	//EN2 SW
	PORTB.PIN5CTRL |= PORT_ISC_FALLING_gc | PORT_PULLUPEN_bm;	//EN3 SW
	
	
	//for(uint16_t i = 0; i < updateRate; i++){
	for(uint8_t i = 0; i < 10; i++){
		shiftWrite5((((uint64_t)1<<i)<<0) | (((uint64_t)1<<i)<<10) | (((uint64_t)1<<i)<<20) | (((uint64_t)1<<i)<<30));
		_delay_ms(100);
	}
		//sprintf(tx_message, "%d|%d|%d|%d\r\n", (int)(1023*(encoder_accum[0]&mute[0])/100), (int)(1023*(encoder_accum[1]&mute[1])/100), (int)(1023*(encoder_accum[2]&mute[2])/100), (int)(1023*(encoder_accum[3]&mute[3])/100));
		//usart_tx_string(tx_message);
	//}
	updateLEDs();
	
	startTimerA(update_period);

	sei();
	set_sleep_mode(SLEEP_MODE_IDLE);
    while (1){
		
		
		sleep_mode();
		
		// **********************
		// Physical Interaction Response
		// **********************
		if(chg_flag){
			rtc_start(save_delay, RTC_PRESCALER_DIV32768_gc);		// after delay, save to EEPROM. 
			
			for(uint8_t i = 0; i < 4; i++){
				if(encoder_accum[i] > 100){
					encoder_accum[i] = 100;
				} else if(encoder_accum[i] < 0){
					encoder_accum[i] = 0;
				} 
			}
			
			updateLEDs();
			sprintf(tx_message, "%d|%d|%d|%d\r\n", (int)(1023*(encoder_accum[0]&mute[0])/100), (int)(1023*(encoder_accum[1]&mute[1])/100), (int)(1023*(encoder_accum[2]&mute[2])/100), (int)(1023*(encoder_accum[3]&mute[3])/100));
			usart_tx_string(tx_message);
			chg_flag = 0;
		}
		/*for(uint8_t i = 0; i < 4; i++){
			if(chg_flag[i]){
				chg_flag[i] = 0;
				rtc_start(save_delay, RTC_PRESCALER_DIV32768_gc);		// after delay, save to EEPROM. 
													// add 'EN0' bit to the flag, so it saves Encoder 0's value, eg
			
				if(encoder_accum[i] > 100){
					encoder_accum[i] = 100;
				} else if(encoder_accum[i] < 0){
					encoder_accum[i] = 0;
				} else{
					updateLEDs();
					sprintf(tx_message, "%d|%d|%d|%d\r\n", (int)(1023*(encoder_accum[0]&mute[0])/100), (int)(1023*(encoder_accum[1]&mute[1])/100), (int)(1023*(encoder_accum[2]&mute[2])/100), (int)(1023*(encoder_accum[3]&mute[3])/100));
					usart_tx_string(tx_message);
				}
			}
		}*/
		
		// **********************
		// Periodic UART Update
		// **********************
		if(TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm){
			TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;	//clear flag
			sprintf(tx_message, "%d|%d|%d|%d\r\n", (int)(1023*(encoder_accum[0]&mute[0])/100), (int)(1023*(encoder_accum[1]&mute[1])/100), (int)(1023*(encoder_accum[2]&mute[2])/100), (int)(1023*(encoder_accum[3]&mute[3])/100));
			usart_tx_string(tx_message);
		}
		
		// **********************
		// Post-Delay EEPROM Save
		// **********************
		if(save_flag & 0x80){
			if(save_flag & (1<<0))
				//usart_tx_string("Saved EN0!\n");	//EEPROM write 0				
				eeprom_write_byte((uint8_t *)0, encoder_accum[0]);
			if(save_flag & (1<<1))
				//usart_tx_string("Saved EN1!\n");	//EEPROM write 1
				eeprom_write_byte((uint8_t *)1, encoder_accum[1]);
			if(save_flag & (1<<2))
				//usart_tx_string("Saved EN2!\n");	//EEPROM write 2
				eeprom_write_byte((uint8_t *)2, encoder_accum[2]);
			if(save_flag & (1<<3))
				//usart_tx_string("Saved EN3!\n");	//EEPROM write 3
				eeprom_write_byte((uint8_t *)3, encoder_accum[3]);
			
			save_flag = 0;
		}
		
		// **********************
		// UART Command Interface
		// **********************
		if(rx_word){
			stopTimerA();
			if(!strcmp(rx_message, "help\n") || !strcmp(rx_message, "?\n") || !strcmp(rx_message, "commands\n")){
				usart_tx_string("setClickRate n\n\tSets the increment/decrement ");
				usart_tx_string("per encoder click, 1-100\n\n");
				
				usart_tx_string("setSaveDelay n\n\tSets the delay in seconds ");
				usart_tx_string("after which I save volumes to EEPROM, 1-255\n\n");
				
				usart_tx_string("setVolume n m\n\tSets the volume of encoder n ");
				usart_tx_string("to the value of m, 0-100\n\n");
				
				usart_tx_string("setUpdateRate n\n\tSets time between volume");
				usart_tx_string("updates in ms, 0-255\n\n");
				
				usart_tx_string("resume\n\tResumes periodic operation\n\n");
				
				sprintf(tx_message, "ClickRate = %d    SaveDelay = %d    Update Period = %d\n>", enc_inc, save_delay, update_period);
				usart_tx_string(tx_message);
				
			} else if(!strncmp(rx_message, "setClickRate ", 13) && rx_index >= 14){
				enc_inc = parser(rx_message, 0);
				eeprom_write_byte(mem_clickRate, enc_inc);
				sprintf(tx_message, "Volume will increment %d per click\n>", enc_inc);
				usart_tx_string(tx_message);
				
			} else if(!strncmp(rx_message, "setSaveDelay ", 13) && rx_index >= 14){
				save_delay = parser(rx_message, 0);
				eeprom_write_byte(mem_saveDelay, save_delay);
				sprintf(tx_message, "Volume will save after %d seconds\n>", save_delay);
				usart_tx_string(tx_message);
				
			} else if(!strncmp(rx_message, "setVolume ", 10) && rx_index >= 13){
				encoder_accum[parser(rx_message, 0)] = parser(rx_message, 1);
				sprintf(tx_message, "Encoder %d volume set to %d\n>", (int)parser(rx_message, 0), (int)parser(rx_message, 1));
				usart_tx_string(tx_message);
				
			} else if(!strncmp(rx_message, "setUpdatePeriod ", 16) && rx_index >= 17){
				update_period = parser(rx_message, 1);
				eeprom_write_word(mem_updatePeriod, update_period);
				sprintf(tx_message, "Volume will send every %d milliseconds\n>", update_period);
				usart_tx_string(tx_message);
			} else if(!strncmp(rx_message, "resume", 6)){
				startTimerA(update_period);
			}
			
			usart_reset_rx();
		}
		
    }
}

ISR(PORTA_PORT_vect){
	
	switch(PORTA.INTFLAGS){
		case(EN1_A):
			if(((PORTA.IN&EN1_A) && !(PORTA.IN&EN1_B)) || (!(PORTA.IN&EN1_A) && (PORTA.IN&EN1_B))){
				encoder_accum[1] += enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<1);
			}
			break;
		case(EN1_B):
			if(((PORTA.IN&EN1_A) && !(PORTA.IN&EN1_B)) || (!(PORTA.IN&EN1_A) && (PORTA.IN&EN1_B))){
				encoder_accum[1] -= enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<1);
			}
			break;
		
		case(EN2_A):
			if(((PORTA.IN&EN2_A) && !(PORTA.IN&EN2_B)) || (!(PORTA.IN&EN2_A) && (PORTA.IN&EN2_B))){
				encoder_accum[2] += enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<2);
			}
			break;
		case(EN2_B):
			if(((PORTA.IN&EN2_A) && !(PORTA.IN&EN2_B)) || (!(PORTA.IN&EN2_A) && (PORTA.IN&EN2_B))){
				encoder_accum[2] -= enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<2);
			}
		break;
		
		case(EN3_A):
			if(((PORTA.IN&EN3_A) && !(PORTA.IN&EN3_B)) || (!(PORTA.IN&EN3_A) && (PORTA.IN&EN3_B))){
				encoder_accum[3] += enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<3);
			}
			break;
		case(EN3_B):
			if(((PORTA.IN&EN3_A) && !(PORTA.IN&EN3_B)) || (!(PORTA.IN&EN3_A) && (PORTA.IN&EN3_B))){
				encoder_accum[3] -= enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<3);
			}
			break;
		
		default:
			break;
	}
	PORTA.INTFLAGS = 0xFF;
}



ISR(PORTB_PORT_vect){
	switch(PORTB.INTFLAGS){
		case(EN2_SW):
			mute[2] ^= 0xFF;
			chg_flag = 0xFF;
			save_flag |= (1<<2);
			_delay_ms(50);
			while(!(PORTB.IN&EN2_SW));
			_delay_ms(50);
			break;
		case(EN3_SW):
			mute[3] ^= 0xFF;
			chg_flag = 0xFF;
			save_flag |= (1<<3);
			_delay_ms(50);
			while(!(PORTB.IN&EN3_SW));
			_delay_ms(50);
			break;
		default:
			break;
	}
	PORTB.INTFLAGS = 0xFF;
}



ISR(PORTC_PORT_vect){
	
	switch(PORTC.INTFLAGS){
		case(EN0_A):
			if(((PORTC.IN&EN0_A) && !(PORTC.IN&EN0_B)) || (!(PORTC.IN&EN0_A) && (PORTC.IN&EN0_B))){
				encoder_accum[0] += enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<0);
			}
			break;
			
		case(EN0_B):
			if(((PORTC.IN&EN0_A) && !(PORTC.IN&EN0_B)) || (!(PORTC.IN&EN0_A) && (PORTC.IN&EN0_B))){
				encoder_accum[0] -= enc_inc;
				chg_flag = 0xFF;
				save_flag |= (1<<0);
			}
			break;
				
		case(EN0_SW):
			mute[0] ^= 0xFF;
			chg_flag = 0xFF;
			save_flag |= (1<<0);
			_delay_ms(50);
			while(!(PORTC.IN&EN0_SW));
			_delay_ms(50);
			break;
		case(EN1_SW):
			mute[1] ^= 0xFF;
			chg_flag = 0xFF;
			save_flag |= (1<<1);
			_delay_ms(50);
			while(!(PORTC.IN&EN1_SW));
			_delay_ms(50);
			break;
			
		default:
			break;
	}	
	PORTC.INTFLAGS = 0xFF;
}


ISR(USART0_RXC_vect){
	rx_message[rx_index] = usart_rx();
	if(rx_message[rx_index++] == '\n'){			//terminate word on newline
		rx_word = 0xFF;
	}
}

ISR(RTC_CNT_vect){
	save_flag |= 0x80;		// the 'activate' bit
	rtc_stop();
	RTC_INTFLAGS = (RTC_OVF_bm) | (RTC_CMP_bm);
}

ISR(TCA0_OVF_vect){
	
}

