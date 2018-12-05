#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include "usart.h"

int main(void)
{
	uint16_t wData = 0;
	uint16_t cnt = 0;
	
	DDRC = 0b00000000;
	PORTC = 0b11111110;
	DDRD = 0b11000000;

	// シリアル通信の初期化
    sio_init(9600, 8);    // SIO設定
    sei(); // 全割り込み許可

	while(1){
		wData = 0;
		for(uint8_t i = 0; i < 16; i++){
			ADCSRA = _BV(ADEN) | _BV(ADIF) | 0b011;
			ADMUX = _BV(REFS0) | 0;
			ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) | 0b011;
			loop_until_bit_is_set(ADCSRA, ADIF);
			wData += ADCW;
			_delay_ms(1);
		}
		wData = (wData + 8) >> 4;

		if(wData <= 75){
			//人がいない
			PORTD = 0b10000000; //PD7出力
			cnt++;

			if(cnt >= 40){
				//0x1 を送信
				sendChar(0x1);
				cnt = 0;
			}
		}
		else{
			//人がいる
			PORTD = 0b01000000; //PD6出力

			cnt = 0;
		}

		_delay_ms(250);
	}

	return 0;
}
