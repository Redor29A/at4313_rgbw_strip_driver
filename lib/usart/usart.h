/*
 * usart.h
 *
 * Created: 01.01.2025 0:44:48
 *  Author: Redor
 */ 


#ifndef USART_H_
#define USART_H_

#ifndef F_CPU
#define F_CPU 20000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

class USART{
	public:
	USART( uint32_t baud );
	void send_byte(uint8_t byte);
	uint8_t wait_and_receive_byte();
	uint8_t receive_byte();
	bool check_receive_bufer();
	uint8_t check_last_status();
	private:
	uint8_t status;
	};


#endif /* USART_H_ */
