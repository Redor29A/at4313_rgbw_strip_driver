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
/*    _|____0_____|__1__|__2__|__3__|__4__|__5__|__6__|__7__|           */
/*     |          |  FE | DOR | UPE |     |     |     |     |           */
/*    0| UART     | 255 | 255 | 255 |     |     |     |     |		    */
/*    _|__________|_____|_____|_____|_____|_____|_____|_____|           */
/*     |          | 100 | 255 | 255 | 255 | 255 |     |     |           */
/*    1| MANUAL   | BRG |  R  |  G  |  B  |  W  |     |     |           */
/*    _|__________|_____|_____|_____|_____|_____|_____|_____|           */
/*     |          | 255 |  2  | 100 | 255 |     |     |     |           */
/*    2| FADE     | C_t | MOD | BRG | SAT |     |  m  |     |           */
/*    _|__________|_____|_____|_____|_____|_____|_____|_____|           */
/*     |          |  1  | 255 | 255 | 255 | 255 | 255 | 255 |           */
/*    3| PULSE    | RND?| C_t | STR |  R  |  G  |  B  |  W  |           */
/*    _|__________|_____|_____|_____|_____|_____|_____|_____|           */
/*                                                                      */
/*                                                                      */
/************************************************************************/
#include "main.h"
#include "disp_class.h"
#include "rgbw_pwm.h"
#include "usart.h"
#define rand_mass_len 48

const uint8_t PROGMEM rand_mass[3][rand_mass_len] = {
{0,255,144,216,0,255,127,255,144,229,0,101,0,233,174,255,0,153,233,242,0,233,29,101,195,0,208,255,225,255,255,106,0,0,148,144,0,255,229,4,178,33,229,165,255,63,255,255},
{255,16,255,0,59,0,255,0,0,255,255,255,255,255,255,238,255,255,0,255,255,255,255,0,0,255,255,0,255,136,106,255,255,255,255,0,0,0,255,255,0,0,255,255,0,255,0,0},
{225,0,0,255,255,131,0,29,255,0,221,0,229,0,0,0,169,0,255,0,63,0,0,255,255,55,0,208,0,0,0,0,131,187,0,255,255,238,0,0,255,255,0,0,203,0,29,29}};
uint16_t eeporn_tim;
uint8_t rand_i;
uint8_t val, val_tmp;
int16_t disp_num;
int8_t val_tmp_enc;
#define menu_x_len 7
#define menu_y_len 3
uint8_t menu[menu_y_len+1][menu_x_len+1] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 0x00, 0x00, 0x00, 0x00, 0x00},
{0x00, 0x00, 0x00, 100, 255, 0x00, 0x00},
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
const uint8_t menu_max[menu_y_len+1][menu_x_len+1] = {
{0x00, 255, 255, 255, 0x00, 0x00, 0x00, 0x00},
{0x00, 100, 255, 255, 255, 255, 0x00, 0x00},
{0x00, 255, 2, 100, 255, 255, 255, 0x00},
{0x00, 1, 255, 255, 255, 255, 255, 255}};
uint8_t menu_len[menu_y_len+1] = {3,5,4,7}; 
uint8_t menu_x;
uint8_t menu_y;
const uint8_t menu_name[menu_y_len+1][4] = {
{0x00, 0x3E, 0x6D, 0x7C},
{0x76, 0x77, 0x54, 0x5E},
{0x71, 0x77, 0x5E, 0x79},
{0x73, 0x3E, 0x38, 0x6D}};
uint16_t phase_fade;
uint8_t phase_pulse;
bool pulse_dir;
bool menu_edit;
bool butn_lst;
bool butn;
uint16_t butn_tim;

uint8_t rgbw[4];
uint8_t num;
USART serial(14400);
RGBW strip;

void eeporn_save(){
	bool eeporn_save_flag;
	eeprom_busy_wait();
	if((eeprom_read_byte((uint8_t*)0) != menu_x)) eeporn_save_flag = true;
	eeprom_busy_wait();
	if((eeprom_read_byte((uint8_t*)1) != menu_y)) eeporn_save_flag = true;
	for(uint8_t y = 0; y <= menu_y_len; y++){
		for(uint8_t x = 1; x <= menu_x_len; x++){
			eeprom_busy_wait();
			if(eeprom_read_byte((uint8_t*)((x+(menu_x_len*y))+2)) != menu[y][x]) eeporn_save_flag = true;
		}
	}
	if(eeporn_save_flag){
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t*)0, menu_x);
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t*)1, menu_y);
		for(uint8_t y = 1; y <= menu_y_len; y++){
			for(uint8_t x = 1; x <= menu_x_len; x++){
				eeprom_busy_wait();
				eeprom_write_byte((uint8_t*)((x+(menu_x_len*y))+2), menu[y][x]);
			}
		}
	}
}
void eeporn_read(){
	if (eeprom_read_byte((uint8_t*)0) == 0xff)
	{
		eeporn_save();
	}
	else{
		eeprom_busy_wait();
		menu_x = eeprom_read_byte((uint8_t*)0);
		eeprom_busy_wait();
		menu_y = eeprom_read_byte((uint8_t*)1);
		for(uint8_t y = 1; y <= menu_y_len; y++){
			for(uint8_t x = 1; x <= menu_x_len; x++){
				eeprom_busy_wait();
				menu[y][x] = eeprom_read_byte((uint8_t*)((x+(menu_x_len*y))+2));
			}
		}
	}
}
void enc_setup(){
	MCUCR |= (1<<ISC10)|(1<<ISC00); //  Interrupt Sense Control The  Any logical change of INT1 AND INT0 generates an interrupt request.
	GIMSK |= (1<<INT1)|(1<<INT0); //enc_setup
	DDRD &= ~(1 << 2);
	DDRD &= ~(1 << 3);
	DDRD &= ~(1 << 4);
}
void enc(bool pm){
	if((menu_x == 0) & menu_edit){
		if(pm){ 
			if(menu_y==menu_y_len){
				menu_y = 0;
				}
			else{
				menu_y++;
			}
		}
		else{
			if(menu_y==0){
				menu_y = menu_y_len;
			}
			else{
				menu_y--;
			}
		}
		
	}
	else if(menu_edit){
		if(pm){
			if(menu[menu_y][menu_x] == menu_max[menu_y][menu_x]){
				menu[menu_y][menu_x] = 0;
			}
			else{
				menu[menu_y][menu_x]++;
			}
		}
		else {
			if(menu[menu_y][menu_x] == 0){
				menu[menu_y][menu_x] = menu_max[menu_y][menu_x];
			}
			else{
				menu[menu_y][menu_x]--;
			}
		}
	}
	else{
		if(pm){
			if (menu_x == menu_len[menu_y]){
				menu_x = 0;
			}
			else{
				menu_x++;
			}
		}
		else{
			if (menu_x == 0){
				menu_x = menu_len[menu_y];
			}
			else{
				menu_x--;
			}
		}
	}
}

void check_enc(){
	_delay_us(50);
	val_tmp = 0;
	if((PIND & (1<<PIND2))==0)
	val_tmp |= (1<<1);

	if((PIND & (1<<PIND3))==0)
	val_tmp |= (1<<0);

	if((val==3 && val_tmp==1) || (val==0 && val_tmp==2)){
		val_tmp_enc++;
	}
	else if((val==2 && val_tmp==0) || (val==1 && val_tmp==3)){
		val_tmp_enc--;
	}
	if(val_tmp_enc > 0){enc(true);val_tmp_enc = 0;}
    if(val_tmp_enc < 0){enc(false);val_tmp_enc = 0;}
	val = val_tmp;
}

ISR(INT0_vect){
	check_enc();
}
ISR(INT1_vect){
	check_enc();;
}
void menu_update(){
	switch(menu_y){
		case 0:
			/*if (serial.check_receive_bufer())
			{
				uint8_t stat[4];
				for(uint8_t i = 0; i < 4; i++){
					rgbw[i] = serial.wait_and_receive_byte();
					stat[i] = serial.check_last_status();
				}
				for(uint8_t i = 0; i < 4; i++){
					//serial.send_byte(rgbw[i]);
					if ((stat[i] & (1<<FE))>0) menu[menu_y][1]++;
					if ((stat[i] & (1<<DOR))>0) menu[menu_y][2]++;
					if ((stat[i] & (1<<UPE))>0) menu[menu_y][3]++;
				}
			}*/
		break;
		case 1:
			for(uint8_t i = 0; i < 4; i++){
				rgbw[i] = strip.rgb_fix((menu[menu_y][i+2]*menu[menu_y][1])/100);
			}
		break;
		case 2:
			if(menu[menu_y][0] >= menu[menu_y][1]/10){
				if(menu[menu_y][6] != menu[menu_y][2]){
					phase_fade = 0;
				}
				if (phase_fade < 255) {
					rgbw[0] = phase_fade;
					rgbw[2] = 255 - phase_fade;
					rgbw[1] = 0;
				}
				else if (phase_fade < 511) {
					rgbw[1] = phase_fade-255;
					rgbw[0] = 255 - (phase_fade - 255);
					rgbw[2] = 0;
				}
				else if (phase_fade < 767) {
					rgbw[2] = phase_fade - 511;
					rgbw[1] = 255 - (phase_fade - 511);
					rgbw[0] = 0;
				}
				rgbw[3] = strip.rgb_fix(255-menu[menu_y][4]);
				for(uint8_t i = 0; i < 3; i++){
					rgbw[i] = rgbw[i] * menu[menu_y][4]/255;
				}
				for(uint8_t i = 0; i < 4; i++){
					rgbw[i] = rgbw[i] * menu[menu_y][3]/100;
				}
				if (menu[menu_y][2] == 2){
					for(uint8_t i = 0; i < 3; i++){
						rgbw[i] = strip.rgb_fix(rgbw[i]);
					}
				}
					
				if ((menu[menu_y][2] == 0) | (menu[menu_y][2] == 0)){
					phase_fade++;
					if (phase_fade > 767) phase_fade = 0;
				}
				else if(menu[menu_y][2] == 1){
						
					if (phase_fade == 0) phase_fade = 768;
					phase_fade--;
				}
	
				menu[menu_y][6] = menu[menu_y][2];
 				menu[menu_y][0] = 0;
			}
			menu[menu_y][0]++;
		break;
		case 3:
			if(menu[menu_y][1] == 1){menu_len[menu_y] = 3;}
			else{menu_len[menu_y] = 7;}
			if(menu[menu_y][0] >= menu[menu_y][2]/10){
				for(uint8_t i = 0; i < 4; i++){
					rgbw[i] = menu[menu_y][i+4]*phase_pulse/255;
				}
				if(phase_pulse >= 255) pulse_dir=false;
				if(phase_pulse <= menu[menu_y][3]){
					if (menu[menu_y][1] == 1)
					{
						for(uint8_t i = 0; i < 3; i++){
							//menu[menu_y][i+4] = rand() % 255;
							menu[menu_y][i+4] = pgm_read_byte(&rand_mass[i][rand_i]);
						}
						rand_i++;
						if (rand_i >= rand_mass_len){rand_i = 0;}
						//menu[menu_y][7] = rand() % 40;
					}
					pulse_dir=true;
				}
				if(pulse_dir) phase_pulse++;
				else phase_pulse--;
				menu[menu_y][0] = 0;
			}
			menu[menu_y][0]++;
		break;
	}
}

int main(void)
{
	
	DDRD |= (1 << 6); // led
	eeporn_read();
	enc_setup();
	PORTD |= (1 << 0);
	//UCSRB |= (1 << RXCIE); // USART0_RX_vect enable
	
	TM1637 disp(2);
	
	
	
	uint8_t data[4];
	sei();
    while (1) 
    {
		if (eeporn_tim >= 65534){
			eeporn_save();
			eeporn_tim = 0;
		}
		eeporn_tim++;

		if(butn_tim > 50){
			butn = ((PIND & (1<<PIND4)) > 1);
			if(butn > butn_lst){
				menu_edit = !menu_edit;
			}
			if(menu_edit & (menu_y == 0) & (menu_x != 0)) menu_edit = false;
			butn_lst = butn;
			butn_tim = 0;
			if (menu_edit){
				PORTD |= (1<<6);
			}
			else{
				PORTD &= ~(1<<6);
			}
		}
		butn_tim++;
		
		
		if(menu_x == 0){
			for(int i = 0; i<4; i++){
				data[i] = menu_name[menu_y][i];
			}
		}
		else{
			disp_num = menu[menu_y][menu_x];
			for(int i = 0; i<4; i++){
				switch(i){
					case 0:
					//num = disp_num / 1000; break;
					num = menu_x; break;
					case 1:
					num = (disp_num / 100)%10; break;
					case 2:
					num = (disp_num % 100)/10; break;
					case 3:
					num = disp_num % 10; break;
					
				}
				data[i] = disp.toSeg(num);
			}
		}
		disp.setSegments(data);
		menu_update();
		strip.pwm_set(rgbw);
    }
}