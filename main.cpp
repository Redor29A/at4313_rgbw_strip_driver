/************************************************************************/
/*                           AT4313                                     */
/*                                                                      */
/*               RST-PA2-1         20-VCC                               */
/*               RXD-PD0-2         19-PB7-SCK                           */
/*               TXD-PD1-3         18-PB6-MISO                          */
/*              XTAL-PA1-4         17-PB5-MOSI                          */
/*              XTAL-PA0-5         16-PB4-OC1B                          */
/*              ENC0-PD2-6         15-PB3-OC1A                          */
/*              ENC1-PD3-7         14-PB2-OC0A                          */
/*              BUTN-PD4-8         13-PB1-DIO_disp                      */
/*               RGB-PD5-9         12-PB0-CLK_disp                      */
/*                   GND-10        11-PD6-LED                           */
/*                                                                      */
/************************************************************************/
/*                         MENU                                         */
/*                                                                      */
/*    _|____0________|__1__|__2__|__3__|__4__|__5__|__6__|__7__|        */
/*     |             |  FE | DOR | UPE |     |     |     |     |        */
/*    0| UART|USb    | 255 | 255 | 255 |     |     |     |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/*     |             | 100 | 255 | 255 | 255 | 255 |     |     |        */
/*    1| MANUAL1|MAn1| BRG |  R  |  G  |  B  |  W  |     |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/*     |             | 100 | 255 | 255 | 255 | 255 |     |     |        */
/*    2| MANUAL2|MAn2| BRG |  R  |  G  |  B  |  W  |     |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/*     |             | 255 |  1  | 100 | 255 |     |     |     |        */
/*    3| RAINBOW|rAIn| SPD | DIR | BRG | SAT |     |     |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/*     |             | 255 | 100 | 255 | 255 | 255 | 255 |     |        */
/*    4| PULSE|PULS  | SPD | BRG |  R  |  G  |  B  |  W  |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/*     |             | 100 | 255 |     |     |     |     |     |        */
/*    5| WHITE|tEnP  | BRG | TMP |     |     |     |     |     |        */
/*    _|_____________|_____|_____|_____|_____|_____|_____|_____|        */
/************************************************************************/
/*                                                                      */
/*          P1                               P2                         */
/*     |   |   |   |             |    |    |    |    |    |             */
/*     |   |   |   |             |    |    |    |    |    |             */
/*    VCC GND RXD TXD           VCC  GND  RST  SCK  MISO MOSI           */
/*                                                                      */
/************************************************************************/
#include "main.h"
#include "disp_class.h"
#include "rgbw_pwm.h"
#include "usart.h"
#include <util/crc16.h>

#define menu_x_len 7
#define menu_y_len 5
#define UART_BAUD 14400UL
#define UART_HEADER 0xA5
#define UART_CMD_RGBW 0x01
#define UART_CMD_RELEASE 0x02

enum MenuPage : uint8_t {
	MENU_UART = 0,
	MENU_MANUAL1,
	MENU_MANUAL2,
	MENU_RAINBOW,
	MENU_PULSE,
	MENU_WHITE_TEMP,
};

enum MenuColumn : uint8_t {
	COL_TITLE = 0,
	COL_BRG = 1,
	COL_R = 2,
	COL_G = 3,
	COL_B = 4,
	COL_W = 5,
	COL_EXTRA = 6,
};

struct EepromData {
	uint8_t menu_x;
	uint8_t menu_y;
	uint8_t menu[menu_y_len+1][menu_x_len+1];
	uint8_t menu_max[menu_y_len+1][menu_x_len+1];
	uint8_t menu_len[menu_y_len+1];
	uint8_t menu_name[menu_y_len+1][4];
	uint8_t crc;
};

EepromData ee EEMEM = {
0,
0,
{
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 0x00, 0x00, 100, 255, 0x00, 0x00, 0x00},
{0x00, 4, 100, 255, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 127, 0x00, 0x00, 0x00, 0x00, 0x00}},
{
{0x00, 255, 255, 255, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 255, 255, 255, 255, 0x00, 0x00},
{0x00, 100, 255, 255, 255, 255, 0x00, 0x00},
{0x00, 255, 1, 100, 255, 0x00, 0x00, 0x00},
{0x00, 255, 100, 255, 255, 255, 255, 0x00},
{0x00, 100, 255, 0x00, 0x00, 0x00, 0x00, 0x00}},
{3,5,5,4,6,2},
{
{0x00, 0x3E, 0x6D, 0x7C},
{0b00010101, 0b01110111, 0b00110111, 0b00000110},
{0b00010101, 0b01110111, 0b00110111, 0b01011011},
{0x50, 0x77, 0x06, 0x54},
{0x73, 0x3E, 0x38, 0x6D},
{0x78, 0x79, 0x54, 0x73}},
0x1c};
uint8_t menu_x;
uint8_t menu_y;
bool eeprom_crc_ok;
uint8_t encoder_state;
uint8_t encoder_state_new;
volatile int8_t encoder_delta;
uint16_t phase_fade;
uint8_t phase_pulse;
bool uart_control;
bool menu_edit;
bool butn_lst;
bool butn;
uint16_t butn_tim;

uint8_t rgbw[4];
RGBW strip;
USART serial(UART_BAUD);

uint8_t eeprom_crc_calc(){
	uint8_t crc = 0;
	uint8_t *addr = (uint8_t*)&ee;
	for(uint8_t i = 0; i < sizeof(EepromData) - 1; i++){
		eeprom_busy_wait();
		crc = _crc8_ccitt_update(crc, eeprom_read_byte(addr + i));
	}
	return crc;
}

void eeprom_crc_update(){
	eeprom_busy_wait();
	eeprom_update_byte(&ee.crc, eeprom_crc_calc());
}

void startup_power_delay(){
	for(uint8_t i = 0; i < 30; i++){
		_delay_ms(10);
	}
}

bool eeprom_crc_check(){
	for(uint8_t i = 0; i < 5; i++){
		eeprom_busy_wait();
		uint8_t stored_crc = eeprom_read_byte(&ee.crc);
		if(eeprom_crc_calc() == stored_crc){
			return true;
		}
		_delay_ms(20);
	}
	return false;
}

uint8_t menu_get(uint8_t y, uint8_t x){
	eeprom_busy_wait();
	return eeprom_read_byte(&ee.menu[y][x]);
}

void menu_set(uint8_t y, uint8_t x, uint8_t value){
	eeprom_busy_wait();
	eeprom_update_byte(&ee.menu[y][x], value);
	eeprom_crc_update();
}

uint8_t menu_max_get(uint8_t y, uint8_t x){
	eeprom_busy_wait();
	return eeprom_read_byte(&ee.menu_max[y][x]);
}

uint8_t menu_len_get(uint8_t y){
	eeprom_busy_wait();
	return eeprom_read_byte(&ee.menu_len[y]);
}

uint8_t menu_name_get(uint8_t y, uint8_t x){
	eeprom_busy_wait();
	return eeprom_read_byte(&ee.menu_name[y][x]);
}

void menu_x_set(uint8_t value){
	menu_x = value;
	eeprom_busy_wait();
	eeprom_update_byte(&ee.menu_x, value);
	eeprom_crc_update();
}

void menu_y_set(uint8_t value){
	if(value != menu_y){
		phase_fade = 0;
		phase_pulse = 0;
	}
	menu_y = value;
	eeprom_busy_wait();
	eeprom_update_byte(&ee.menu_y, value);
	eeprom_crc_update();
}

void menu_value_step(bool pm){
	uint8_t value = menu_get(menu_y, menu_x);
	uint8_t max = menu_max_get(menu_y, menu_x);
	if(pm){
		value = (value == max) ? 0 : value + 1;
	}
	else {
		value = (value == 0) ? max : value - 1;
	}
	menu_set(menu_y, menu_x, value);
}

void eeprom_read_menu(){
	eeprom_crc_ok = eeprom_crc_check();
	eeprom_busy_wait();
	menu_y = eeprom_read_byte(&ee.menu_y);
	if(menu_y > menu_y_len){
		if(eeprom_crc_ok) menu_y_set(0);
		else menu_y = 0;
	}
	eeprom_busy_wait();
	menu_x = eeprom_read_byte(&ee.menu_x);
	if(menu_x > menu_len_get(menu_y)){
		if(eeprom_crc_ok) menu_x_set(0);
		else menu_x = 0;
	}
}
void enc_setup(){
	MCUCR |= (1<<ISC10)|(1<<ISC00); //  Interrupt Sense Control The  Any logical change of INT1 AND INT0 generates an interrupt request.
	GIMSK |= (1<<INT1)|(1<<INT0); //enc_setup
	DDRD &= ~(1 << 2);
	DDRD &= ~(1 << 3);
	DDRD &= ~(1 << 4);
}
void encoder_apply(bool increment){
	if((menu_x == COL_TITLE) && menu_edit){
		if(increment){ 
			if(menu_y==menu_y_len){
				menu_y_set(0);
				}
			else{
				menu_y_set(menu_y + 1);
			}
		}
		else{
			if(menu_y==0){
				menu_y_set(menu_y_len);
			}
			else{
				menu_y_set(menu_y - 1);
			}
		}
		
	}
	else if(menu_edit){
		menu_value_step(increment);
	}
	else{
		if(increment){
			if (menu_x == menu_len_get(menu_y)){
				menu_x_set(0);
			}
			else{
				menu_x_set(menu_x + 1);
			}
		}
		else{
			if (menu_x == 0){
				menu_x_set(menu_len_get(menu_y));
			}
			else{
				menu_x_set(menu_x - 1);
			}
		}
	}
}

void check_enc(){
	_delay_us(50);
	encoder_state_new = 0;
	if((PIND & (1<<PIND2))==0)
		encoder_state_new |= (1<<1);

	if((PIND & (1<<PIND3))==0)
		encoder_state_new |= (1<<0);

	if((encoder_state==3 && encoder_state_new==1) || (encoder_state==0 && encoder_state_new==2)){
		encoder_delta++;
	}
	else if((encoder_state==2 && encoder_state_new==0) || (encoder_state==1 && encoder_state_new==3)){
		encoder_delta--;
	}
	encoder_state = encoder_state_new;
}

ISR(INT0_vect){
	check_enc();
}
ISR(INT1_vect){
	check_enc();
}

void clear_rgbw(){
	for(uint8_t i = 0; i < 4; i++){
		rgbw[i] = 0;
	}
}

void update_encoder(){
	int8_t move;
	cli();
	move = encoder_delta;
	encoder_delta = 0;
	sei();

	while(move > 0){
		encoder_apply(true);
		move--;
	}
	while(move < 0){
		encoder_apply(false);
		move++;
	}
}

void update_button(){
	if(butn_tim <= 50){
		butn_tim++;
		return;
	}

	butn = ((PIND & (1<<PIND4)) > 1);
	if(butn > butn_lst){
		menu_edit = !menu_edit;
	}
	if(menu_edit && (menu_y == MENU_UART) && (menu_x != COL_TITLE)){
		menu_edit = false;
	}
	butn_lst = butn;
	butn_tim = 0;

	if(menu_edit){
		PORTD |= (1<<6);
	}
	else{
		PORTD &= ~(1<<6);
	}
}

void set_rgb_scaled(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness);

void handle_uart(){
	static uint8_t packet[8];
	static uint8_t index;

	while(serial.check_receive_bufer()){
		uint8_t byte = serial.receive_byte();
		if(index == 0 && byte != UART_HEADER){
			continue;
		}

		packet[index++] = byte;
		if(index < sizeof(packet)){
			continue;
		}
		index = 0;

		uint8_t checksum = 0;
		for(uint8_t i = 0; i < sizeof(packet) - 1; i++){
			checksum ^= packet[i];
		}
		if(checksum != packet[7]){
			continue;
		}

		if(packet[1] == UART_CMD_RGBW){
			uart_control = true;
			menu_y = MENU_UART;
			menu_x = COL_TITLE;
			set_rgb_scaled(packet[2], packet[3], packet[4], packet[5], packet[6]);
		}
		else if(packet[1] == UART_CMD_RELEASE){
			uart_control = false;
		}
	}
}

uint8_t scale8(uint8_t value, uint8_t scale){
	return ((uint16_t)value * scale) / 255;
}

uint8_t blend8(uint8_t a, uint8_t b, uint8_t amount){
	return a + (((int16_t)b - a) * amount) / 255;
}

uint8_t ease_up(uint8_t value){
	uint8_t inv = 255 - value;
	return 255 - scale8(inv, inv);
}

uint8_t effect_fix(uint8_t value){
	if(value == 255){
		value = 254;
	}
	return strip.rgb_fix(value);
}

void set_rgb_scaled(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness){
	rgbw[0] = effect_fix(((uint16_t)r * brightness) / 100);
	rgbw[1] = effect_fix(((uint16_t)g * brightness) / 100);
	rgbw[2] = effect_fix(((uint16_t)b * brightness) / 100);
	rgbw[3] = effect_fix(((uint16_t)w * brightness) / 100);
}

void set_rainbow_scaled(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness){
	uint8_t target = effect_fix(((uint16_t)254 * brightness) / 100);
	uint8_t white = effect_fix(((uint16_t)w * brightness) / 100);

	rgbw[0] = effect_fix(((uint16_t)r * brightness) / 100);
	rgbw[1] = effect_fix(((uint16_t)g * brightness) / 100);
	rgbw[2] = effect_fix(((uint16_t)b * brightness) / 100);
	rgbw[3] = white;

	uint16_t sum = rgbw[0] + rgbw[1] + rgbw[2];
	if(sum != 0){
		rgbw[0] = ((uint16_t)rgbw[0] * target) / sum;
		rgbw[1] = ((uint16_t)rgbw[1] * target) / sum;
		rgbw[2] = ((uint16_t)rgbw[2] * target) / sum;
	}
}

void rainbow_color(uint8_t hue, uint8_t saturation, uint8_t brightness){
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	if(hue < 85){
		r = 255 - hue * 3;
		g = hue * 3;
	}
	else if(hue < 170){
		hue -= 85;
		g = 255 - hue * 3;
		b = hue * 3;
	}
	else{
		hue -= 170;
		b = 255 - hue * 3;
		r = hue * 3;
	}

	r = blend8(255, r, saturation);
	g = blend8(255, g, saturation);
	b = blend8(255, b, saturation);
	set_rainbow_scaled(r, g, b, 255 - saturation, brightness);
}

void update_rainbow(){
	uint8_t speed = menu_get(menu_y, COL_BRG);
	uint8_t direction = menu_get(menu_y, COL_R);
	uint8_t brightness = menu_get(menu_y, COL_G);
	uint8_t saturation = menu_get(menu_y, COL_B);

	if(phase_pulse >= speed / 10){
		if(direction == 0){
			phase_fade++;
			if(phase_fade > 255) phase_fade = 0;
		}
		else {
			if(phase_fade == 0) phase_fade = 256;
			phase_fade--;
		}
		phase_pulse = 0;
	}
	phase_pulse++;
	rainbow_color(phase_fade, saturation, brightness);
}

void update_pulse(){
	uint8_t speed = menu_get(menu_y, COL_BRG);
	uint8_t brightness = menu_get(menu_y, COL_R);
	uint8_t pulse = (phase_fade < 255) ? phase_fade : 511 - phase_fade;
	pulse = ease_up(pulse);

	set_rgb_scaled(
		scale8(menu_get(menu_y, COL_G), pulse),
		scale8(menu_get(menu_y, COL_B), pulse),
		scale8(menu_get(menu_y, COL_W), pulse),
		scale8(menu_get(menu_y, COL_EXTRA), pulse),
		brightness
	);

	if(phase_pulse >= speed / 10){
		phase_fade++;
		if(phase_fade > 511) phase_fade = 0;
		phase_pulse = 0;
	}
	phase_pulse++;
}

void update_white_temp(){
	uint8_t brightness = menu_get(menu_y, COL_BRG);
	uint8_t temp = menu_get(menu_y, COL_R);
	uint8_t warm = 255 - temp;
	uint8_t r = blend8(255, 120, temp);
	uint8_t g = blend8(80, 210, temp);
	uint8_t b = blend8(0, 255, temp);
	uint8_t w = 160 + scale8(warm, 95);

	set_rgb_scaled(r, g, b, w, brightness);
}

void menu_update(){
	if(uart_control){
		return;
	}

	switch(menu_y){
		case MENU_UART:
			clear_rgbw();
		break;
		case MENU_MANUAL1:
		case MENU_MANUAL2:
		{
			uint8_t brightness = menu_get(menu_y, COL_BRG);
			for(uint8_t i = 0; i < 4; i++){
				rgbw[i] = strip.rgb_fix((menu_get(menu_y, i + COL_R) * brightness) / 100);
			}
		}
		break;
		case MENU_RAINBOW:
			update_rainbow();
		break;
		case MENU_PULSE:
			update_pulse();
		break;
		case MENU_WHITE_TEMP:
			update_white_temp();
		break;
	}
}

void fill_display_data(TM1637 &disp, uint8_t data[4]){
	if(menu_x == COL_TITLE){
		for(uint8_t i = 0; i < 4; i++){
			data[i] = menu_name_get(menu_y, i);
		}
		return;
	}

	int16_t disp_num = menu_get(menu_y, menu_x);
	data[0] = disp.toSeg(menu_x);
	data[1] = disp.toSeg((disp_num / 100) % 10);
	data[2] = disp.toSeg((disp_num % 100) / 10);
	data[3] = disp.toSeg(disp_num % 10);
}

void show_eeprom_error(TM1637 &disp){
	uint8_t data[4] = {0x79, 0x50, 0x50, 0x00};
	disp.setSegments(data);
	while(1){}
}

int main(void)
{
	
	DDRD |= (1 << 6); // led
	startup_power_delay();
	eeprom_read_menu();
	enc_setup();
	PORTD |= (1 << 0);
	
	TM1637 disp(2);
	
	uint8_t data[4];
	if(!eeprom_crc_ok){
		show_eeprom_error(disp);
	}

	sei();
    while (1) 
    {
		handle_uart();
		update_encoder();
		update_button();
		fill_display_data(disp, data);
		disp.setSegments(data);
		menu_update();
		strip.pwm_set(rgbw);
    }
}
