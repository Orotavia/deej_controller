#include <avr/io.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

extern uint8_t rx_index = 0x00;
extern uint8_t rx_word = 0x00;
extern volatile char rx_message[50];

void usart_init(uint32_t fclk, uint32_t baud){
	USART0.BAUD = 64UL*fclk / (16UL*baud);		
	PORTB.DIRSET = (1 << 2);	//PB2 as output for UART TX		
	PORTB.DIRCLR = (1 << 3);	//PB1 as input for UART RX		
	USART0.CTRLA = (USART_RXCIE_bm) | (USART_RXSIE_bm);						//interrupts on recieve character and start frame			
	USART0.CTRLB = (USART_RXEN_bm) | (USART_TXEN_bm) | (USART_SFDEN_bm);	//RX, TX, and start frame detect
	//USART0.DBGCTRL = 0x01;
}

void usart_tx(char a){
	while(!(USART0.STATUS & USART_DREIF_bm));	//while data register NOT empty
	USART0.TXDATAL = a;							//write data
	while(!(USART0.STATUS & USART_TXCIF_bm));	//wait for transmit complete
	USART0.STATUS |= (USART_TXCIF_bm);			//clear tx_complete flag
}

void usart_tx_string(char str[]){
	uint8_t i = 0;
	while(str[i] != 0){
		usart_tx(str[i++]);
	}
}

void usart_tx_val(int32_t input, uint8_t base){
	char message[32];
	itoa(input, message, base);
	usart_tx_string(message);
}

char usart_rx(){
	while(!(USART0.STATUS & USART_RXCIF_bm));	//wait for RX flag to go high
	return USART0.RXDATAL;
}

void usart_reset_rx(){
	rx_word = 0;
	rx_index = 0;
	memset(rx_message, 0x00, 32);
}

/*
ISR(USART0_RXC_vect){
	rx_message[rx_index] = usart_rx();
	if(rx_message[rx_index++] == '\n'){			//terminate word on newline
		rx_word = 0xFF;
	}
}*/