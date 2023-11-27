#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/delay.h>


void printarr(char *msg, uint8_t len){
	for(uint8_t i=0; i<len; i++){
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

volatile uint16_t tdcovf=0;

ISR(RTC_CNT_vect){
	tdcovf++;
	RTC.INTFLAGS = 0x1;
}

volatile uint16_t tdc, ignlow, ignhigh, rpmlow, rpmhigh;
volatile uint8_t ignlowt, ignhight, rpmlowt, rpmhight;

// rpm input int
ISR(PORTB_PORT_vect){
	if(VPORTB.IN & 0x1){
		rpmhigh = RTC.CNT;
		rpmhight = GPIOR0;
	}else{
		rpmlow = RTC.CNT;
		rpmlowt = GPIOR0;
	}
	VPORTB.INTFLAGS = 0x1;
}

// ign input int
ISR(PORTC_PORT_vect){
	if(VPORTC.IN & 0x1){
		ignhigh = RTC.CNT;
		ignhight = GPIOR0;
	}else{
		ignlow = RTC.CNT;
		ignlowt = GPIOR0;
	}
	VPORTC.INTFLAGS = 0x1;
}

ISR(TCA0_CMP0_vect){
	VPORTA.IN = PIN2_bm;	// toggle pa2

	uint16_t tval = GPIOR2<<8 | GPIOR3;
	uint8_t tooth = GPIOR0;
/*
	// bosch 60-2
	switch(tooth){
		case 115: // missing tooth
			tval = tval<<2;
			tooth = 0;
			break;
		case 29: // tdc stroke a
		case 89: // tdc stroke b
			VPORTA.IN = PIN4_bm; // toggle pa4
			tdc = RTC.CNT;
			GPIOR1 = 1;
		default:
			tooth++;
			break;
	}
*/
/*
	// bosch 60-2 blocked 2 windows 180deg
	switch(tooth){
		case 55: // missing tooth (added)
			tval = tval<<2;
			tooth++;
			break;
		case 111: // missing tooth
			tval = tval<<2;
			tooth = 0;
			break;

		case 29: // tdc stroke a
		case 85: // tdc strobe b
			VPORTA.IN = PIN4_bm; // toggle pa4
			tdc = RTC.CNT;
			GPIOR1 = 1;
		default:
			tooth++;
			break;
	}
*/

	// regina 44-2-2
	switch(tooth){
		case 39: // missing tooth
			tval = tval<<2;
			tooth++;
			break;
		case 79: // missing tooth
			tval = tval<<2;
			tooth = 0;
			break;
		case 21: // tdc stroke a
		case 61: // tdc stroke b
			VPORTA.IN = PIN4_bm; // toggle pa4
			tdc = RTC.CNT;
			GPIOR1 = 1;
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
	RTC.INTCTRL = 0x1;	// overflow int
	RTC.CTRLA = 0x1;	// enable div1

	// outputs
	PORTA.DIRSET = PIN2_bm; // pa2 out vr
	PORTA.DIRSET = PIN4_bm; // pa4 out tdc

	// tca0	vr output
	TCA0.SINGLE.CMP0 = 0xffff;
	TCA0.SINGLE.INTCTRL = 0x10;	// int cmp0
	TCA0.SINGLE.CTRLB = 0x01;	// freq out mode
	TCA0.SINGLE.CTRLA = 0x3;	// enable, div2

	// rpm pin input
	PORTB.PIN0CTRL = 0x11;	// pb0 interrupt both edges, pull up

	// ign pin input
	PORTC.PIN0CTRL = 0x11;	// pc0 interrupt both edges, pull up



	// state
	GPIOR0 = 0; // tooth count
	GPIOR1 = 0; // output flag

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
	uint16_t rpm, dwell, ignangle;
	float tmp, speed;

	while(1){
		count++;

		if(GPIOR1 & 0x01){
			// rpm
			speed = (((float)rpmlow-(float)rpmhigh)/32768.0)*(30.0/12.0);
			//tmp = 30.0/((((float)rpmlow-(float)rpmhigh)/32768.0)*(30.0/12.0));
			tmp = 30.0/speed;
			rpm = (uint16_t)tmp;

			// dwell
			tmp = ((float)ignhigh-(float)ignlow)/0.032768;
			dwell = (uint16_t)tmp;

			// ign angle
			//tmp = ((float)tdc-(float)ignhigh)/32768.0;
			//tmp = (*180.0)/((((float)tdc-(float)ignhigh)/32768.0)*(30.0/12.0));
			tmp = (((float)tdc-(float)ignhigh)*180.0)/speed;
			ignangle = (uint16_t)tmp;


			// process log output
			uint32_to_str(count, logmsg); // count
			uint16_to_str(tdcovf, &logmsg[11]); // 
			uint16_to_str(tdc, &logmsg[17]); // 

			uint16_to_str(rpm, &logmsg[23]); // 
			uint16_to_str(dwell, &logmsg[29]); // 
			uint16_to_str(ignangle, &logmsg[35]); // 
/*
			uint16_to_str(rpmlow, &logmsg[23]); // 
			uint16_to_str(rpmhigh, &logmsg[29]); // 
			uint16_to_str(rpmlowt, &logmsg[35]); // 
			uint16_to_str(rpmhight, &logmsg[41]); // 
			uint16_to_str(ignlow, &logmsg[47]); // 
			uint16_to_str(ignhigh, &logmsg[53]); // 
			uint16_to_str(ignlowt, &logmsg[59]); // 
			uint16_to_str(ignhight, &logmsg[65]); // 
			uint16_to_str(rpm, &logmsg[71]); // 
*/
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

			GPIOR1 &= ~(0x01);
		}
	}
}
