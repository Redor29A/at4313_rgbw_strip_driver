#include "usart.h"

uint8_t status = 0;
USART::USART( uint32_t baud )
{
	
	uint16_t ubrr = (F_CPU/(16*baud))-1;
	/* Set baud rate */
	UBRRH = ubrr>>8;
	UBRRL = ubrr;
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 2stop bit */
	UCSRC = (1<<UCSZ0)|(1<<UCSZ1);
}
void USART::send_byte(uint8_t byte){
	/* Wait for empty transmit buffer */
	while ( !( UCSRA & (1<<UDRE)) );
	/* Put data into buffer, sends the data */
	UDR = byte;
}
uint8_t USART::wait_and_receive_byte(){
	/* Wait for data to be received */
	uint16_t i = 0;
	while (!(UCSRA & (1<<RXC))){i ++; if(i >= 30000){return 0;}};
	/* Get status then data */
	/* from buffer */
	status = UCSRA;
	/* If error, return -1 */
	
	return UDR;
}
uint8_t USART::receive_byte(){
	/* from buffer */
	status = UCSRA;
	if(!USART::check_receive_bufer()) return 0;
	
	return UDR;
}
uint8_t USART::check_last_status(){
	return status;
}
bool USART::check_receive_bufer(){
	return (UCSRA & (1<<RXC));
}