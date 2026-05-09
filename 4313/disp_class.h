


#ifndef DISP_CLASS_H_
#define DISP_CLASS_H_


#define F_CPU 20000000

#include <avr/io.h>
#include <util/delay.h>

#define CLK 0 //port b
#define DIO 1 //port b
#define m_bitDelay  10
#define BRIGHTNESS 3
#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80

class TM1637{
	private:
	uint8_t m_brightness;
	void set_clk_state(bool state);
	void set_dio_state(bool state);
	void set_clk_mode(bool mode);
	void set_dio_mode(bool mode);
	bool read_dio();
	void bitDelay();
	void disp_start();
	void disp_stop();
	bool disp_writeByte(uint8_t b);
	public:
	TM1637(uint8_t brightness, bool on = 1);
	void setBrightness(uint8_t brightness, bool on = 1);
	void setSegments(const uint8_t segments[], uint8_t length  = 4, uint8_t pos = 0);
	void setNumbers(const uint8_t Numbers[]);
	uint8_t toSeg(uint8_t NUM);
};


#endif /* DISP_CLASS_H_ */