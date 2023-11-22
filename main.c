#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/delay.h>


void printarr(char *msg, int len){
	for(int i=0; i<len; i++){
		USART0.TXDATAL = msg[i];
		while(!(USART0.STATUS & 0x20)){}
	}
}

char *uint32_to_str(uint32_t n, char *dest) {
    dest += 10;
    *dest-- = 0;
    while (n) {
        *dest-- = (n % 10) + '0';
        n /= 10;
    }
    return dest + 1;
}

char *uint16_to_str(uint16_t n, char *dest) {
    dest += 5;
    *dest-- = 0;
    while (n) {
        *dest-- = (n % 10) + '0';
        n /= 10;
    }
    return dest + 1;
}

ISR(TCA0_CMP0_vect){
	VPORTA.IN = PIN2_bm;

	uint16_t tval = GPIOR2<<8 | GPIOR3;
	uint8_t tooth = GPIOR0;
/*
	// bosch 60-2
	switch(tooth){
		case 115:
			tval = tval<<2;
			tooth = 0;
			break;
		default:
			tooth++;
			break;
	}
*/
/*
	// bosch 60-2 blocked 2 windows 180deg
	switch(tooth){
		case 55:
			tval = tval<<2;
			tooth++;
			break;
		case 111:
			tval = tval<<2;
			tooth = 0;
			break;
		default:
			tooth++;
			break;
	}
*/

	// regina 44-2-2
	switch(tooth){
		case 39:
			tval = tval<<2;
			tooth++;
			break;
		case 79:
			tval = tval<<2;
			tooth = 0;
			break;
		default:
			tooth++;
			break;
	}

	GPIOR0 = tooth;
	TCA0.SINGLE.CMP0 = tval;
	TCA0.SINGLE.INTFLAGS = 0x10;
}

int main(){
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = 0;	// no clock prescale

	// usart pa1
	PORTMUX.CTRLB = 0x1;	// alt pins for usart0 pa1
	PORTA.DIRSET = PIN1_bm;	// tx pin
	USART0.BAUD = 80;	// 20.000mhz 1000000 baud
	USART0.CTRLC = 0x3;	// async mode, parity disable, 1 stop bit, 8 bit
	USART0.CTRLB = 0x40; 	// txmit enable

	// rtc
	while(RTC.STATUS & 0x01){} // wait for ctrla to not be busy
	RTC.CTRLA = 0x1;	// enable div1

	// vr output pin
	PORTA.DIRSET = PIN2_bm; // pa2 out

	// tca0	vr output
	TCA0.SINGLE.CMP0 = 0xffff;
	TCA0.SINGLE.INTCTRL = 0x10;	// int cmp0
	TCA0.SINGLE.CTRLB = 0x01;	// freq out mode
	TCA0.SINGLE.CTRLA = 0x3;	// enable, div2

	// tcb0 pa5 input rpm strobe
	TCB0.CTRLB = 0x05;	// input cap freq and pw
	TCB0.CTRLA = 0x03;	// enable, div2

	// tcb1 pa3 input ignition
	TCB0.CTRLB = 0x05;	// input cap freq and pw
	TCB0.CTRLA = 0x03;	// enable, div2


	// state
	GPIOR0 = 0; // tooth count
	GPIOR1 = 0; // 

	// 420 rpm
	// 60-2
	//GPIOR2 = 0x2e;
	//GPIOR3 = 0x81;
	// 44-2-2
	//GPIOR2 = 0x3f;
	//GPIOR3 = 0x69;

	// 800 rpm
	// 60-2
	//GPIOR2 = 0x18;
	//GPIOR3 = 0x6a;
	// 44-2-2
	GPIOR2 = 0x21;
	GPIOR3 = 0x4d;

	// 2000 rpm
	// 60-2
	//GPIOR2 = 0x9; // cmp valh
	//GPIOR3 = 0xc4; // cmp vall
	// 44-2-2
	//GPIOR2 = 0x0d; // cmp valh
	//GPIOR3 = 0x51; // cmp vall

	// 5000 rpm
	// 60-2
	//GPIOR2 = 0x03;
	//GPIOR3 = 0xe8;
	// 44-2-2
	//GPIOR2 = 0x05;
	//GPIOR3 = 0x24;

	sei();

	uint32_t count=0;
	char logmsg[80];
	for(int i=0; i<80; i++) logmsg[i] = '0';

	while(1){
			count++;

			continue;

			// TODO how about fill the back of the buffer to the front so easier to trim unneeded zeros
			uint32_to_str(count, logmsg); // count
			uint16_to_str(0, &logmsg[11]); // 
			uint16_to_str(0, &logmsg[17]); // 
			uint16_to_str(0, &logmsg[23]); // 
			uint16_to_str(0, &logmsg[29]); // 
			uint16_to_str(0, &logmsg[35]); // 
			uint16_to_str(0, &logmsg[41]); // 
			uint16_to_str(0, &logmsg[47]); // 
			uint16_to_str(0, &logmsg[53]); // 
			uint16_to_str(0, &logmsg[59]); // 
			uint16_to_str(0, &logmsg[65]); // 
			uint16_to_str(0, &logmsg[71]); // 

			logmsg[10] = ',';
			logmsg[16] = ',';
			logmsg[22] = ',';
			logmsg[28] = ',';
			logmsg[34] = ',';
			logmsg[40] = ',';
			logmsg[46] = ',';
			logmsg[52] = ',';
			logmsg[58] = ',';
			logmsg[64] = ',';
			logmsg[70] = ',';

			logmsg[76] = '\r';
			logmsg[77] = '\n';
			printarr(logmsg, 78);

			for(int i=0; i<80; i++) logmsg[i] = '0';

		_delay_ms(100);
	}
}
