/*
 * rgbw_pwm.h
 *
 * Created: 30.12.2024 0:14:41
 *  Author: Redor
 */ 


#ifndef RGBW_PWM_H_
#define RGBW_PWM_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>



class RGBW{
	public:
		RGBW(uint8_t speed = 1);
		void pwm_set(uint8_t* rgbw);
		uint8_t rgb_fix(uint8_t color);
	private:
		static const uint8_t PROGMEM logPwm[256];
		void timer1_setup();
		void timer0_setup();
	};

#endif /* RGBW_PWM_H_ */