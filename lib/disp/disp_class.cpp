/*
 *   __A__
 *  |	  |
 *  F     B
 *  |__G__|
 *  |     |
 *  E     C
 *  |__D__|
 */
#include "disp_class.h"

const uint8_t ToSeg[37] = {
	// XGFEDCBA
	0b00111111,    // 0
	0b00000110,    // 1
	0b01011011,    // 2
	0b01001111,    // 3
	0b01100110,    // 4
	0b01101101,    // 5
	0b01111101,    // 6
	0b00000111,    // 7
	0b01111111,    // 8
	0b01101111,    // 9
	0b01110111,    // A
	0b01111100,    // b
	0b00111001,    // C
	0b01011110,    // d
	0b01111001,    // E
	0b01110001,    // F
	0b00111100,    // G
	0b01111100,    // h
	0b00110000,    // I
	0b00011110,    // J
	0b01110101,    // K
	0b00111000,    // L
	0b00010101,    // M
	0b00110111,    // n
	0b01011100,    // O
	0b01110011,    // P
	0b01100111,    // q
	0b00110011,    // r
	0b01101101,    // S
	0b01111000,    // t
	0b00111110,    // U
	0b00101110,    // V
	0b00101010,    // W
	0b01110110,    // X
	0b01101110,    // y
	0b01001011     // Z
};


TM1637::TM1637(uint8_t brightness, bool on){
	m_brightness = (brightness & 0x7) | (on? 0x08 : 0x00);
	DDRB |= (1 << DIO); // disp dio
	DDRB |= (1 << CLK); // disp clk
}
void TM1637::set_clk_state(bool state){
	if (state) PORTB |= (1 << CLK);
	else PORTB &= ~(1 << CLK);
		
}
void TM1637::set_dio_state(bool state){
	if (state){
		PORTB |= (1 << DIO);
	}
	else{
		PORTB &= ~(1 << DIO);
	}
}
void TM1637::set_clk_mode(bool mode){
	if (mode){
		DDRB |= (1 << CLK);
	}
	else{
		DDRB &= ~(1 << CLK);
	}
}
void TM1637::set_dio_mode(bool mode){
	if (mode){
		DDRB |= (1 << DIO);
	}
	else{
		DDRB &= ~(1 << DIO);
	}
}
bool TM1637::read_dio(){
	return (PINB & (1 << DIO)) > 0;
}
void TM1637::bitDelay()
{
	_delay_us(m_bitDelay);
}
void TM1637::disp_start()
{
	set_dio_mode(1);
	bitDelay();
}
void TM1637::disp_stop()
{
	set_dio_mode(1);
	bitDelay();
	set_clk_mode(0);
	bitDelay();
	set_dio_mode(0);
	bitDelay();
}
bool TM1637::disp_writeByte(uint8_t b)
{
	uint8_t data = b;

	// 8 Data Bits
	for(uint8_t i = 0; i < 8; i++) {
		// CLK low
		set_clk_mode(1);
		bitDelay();

		// Set data bit
		if (data & 0x01)
		set_dio_mode(0);
		else
		set_dio_mode(1);

		bitDelay();

		// CLK high
		set_clk_mode(0);
		bitDelay();
		data = data >> 1;
	}

	// Wait for acknowledge
	// CLK to zero
	set_clk_mode(1);
	set_dio_mode(0);
	bitDelay();

	// CLK to high
	set_clk_mode(0);
	bitDelay();
	uint8_t ack = read_dio();
	if (ack == 0)
	set_dio_mode(1);


	bitDelay();
	set_clk_mode(1);
	bitDelay();

	return ack;
}
void TM1637::setBrightness(uint8_t brightness, bool on)
{
	m_brightness = (brightness & 0x7) | (on? 0x08 : 0x00);
}
void TM1637::setSegments(const uint8_t segments[], uint8_t length, uint8_t pos)
{
	// Write COMM1
	disp_start();
	disp_writeByte(TM1637_I2C_COMM1);
	disp_stop();

	// Write COMM2 + first digit address
	disp_start();
	disp_writeByte(TM1637_I2C_COMM2 + (pos & 0x03));

	// Write the data bytes
	for (uint8_t k=0; k < length; k++)
	disp_writeByte(segments[k]);

	disp_stop();

	// Write COMM3 + brightness
	disp_start();
	disp_writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
	disp_stop();
}
void TM1637::setNumbers(const uint8_t Numbers[4]){
	
	uint8_t tmp[4] = {ToSeg[Numbers[0]], ToSeg[Numbers[1]], ToSeg[Numbers[2]], ToSeg[Numbers[3]]};
	setSegments(tmp);
}
uint8_t TM1637::toSeg(uint8_t NUM){
	return ToSeg[NUM];
}