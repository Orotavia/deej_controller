#include "tiny_clk_setup.h"

void clock_setup(uint8_t div){
	uint8_t clockreg = 0x01;
	switch(div){
		case 2:
		clockreg |= (0x00 << 1);
		break;
		case 4:
		clockreg |= (0x01 << 1);
		break;
		case 8:
		clockreg |= (0x02 << 1);
		break;
		case 16:
		clockreg |= (0x03 << 1);
		break;
		case 32:
		clockreg |= (0x04 << 1);
		break;
		case 64:
		clockreg |= (0x05 << 1);
		break;
		case 6:
		clockreg |= (0x08 << 1);
		break;
		case 10:
		clockreg |= (0x09 << 1);
		break;
		case 12:
		clockreg |= (0x0A << 1);
		break;
		case 24:
		clockreg |= (0x0B << 1);
		break;
		case 48:
		clockreg |= (0x0C << 1);
		break;
		default:
		clockreg = 0x00;		// no divison
		break;
	}
	
	//if(!clockreg)
	//fclk /= div;
	
	CCP = 0xD8;																	// unlock protected registers for the next 4 cycles
	CLKCTRL.MCLKCTRLB = clockreg;													// prescaler enable (divide by 2)
	// adjust fclk to account for new prescaler
	while(! (CLKCTRL.MCLKSTATUS & (CLKCTRL_OSC20MS_bm|CLKCTRL_OSC32KS_bm)) );	// wait for clocks stable
}