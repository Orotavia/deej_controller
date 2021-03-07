#ifndef USART_LIB_H
#define USART_LIB_H

void usart_init(uint32_t fclk, uint32_t baud);

void usart_tx(char a);
void usart_tx_string(char str[]);
void usart_tx_val(int32_t input, uint8_t base);

char usart_rx();

void usart_reset_rx();

#endif
