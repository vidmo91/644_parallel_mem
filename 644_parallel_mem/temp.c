
/*
* 644_EEPROM.c
*
* Created: 4/2/2019 12:30:39 PM
* Author : mw
*/




/*
EEPROM/FLASH/SRAM interface
62256 SRAM
28C256 EEPROM
39FS010 FLASH
ATmega644P


PD7 - !WE
PD6 - !OE
PD5 - !CE

PORTA - Data
PORTB - Address L
PORTC - Address H
39SF010 A15 and A16 tied LOW
*/



/*



#define RANGE 0x2ff
#define OFFSETA 0x00
#define OFFSETV 0x0


#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include "uart.h"




//////////////////////////////// CONTROL BUS
#define DDRCTRL DDRD
#define PORTCTRL PORTD
#define PINCTRL PIND
#define WE PD7
#define OE PD6
#define CE PD5

#define J0 PD4
#define J1 PD3
#define J2 PD2
#define RXD0 PD1
#define TXD0 PD0


//////////////////////////////// DATA BUS
#define DDRDAT DDRA
#define PORTDAT PORTA
#define PINDAT PINA
//////////////////////////////// LOW BYTE ADDRESS BUS
#define DDRADRL DDRB
#define PORTADRL PORTB
#define PINADRL PINB
//////////////////////////////// HIGH BYTE ADDRESS BUS
#define DDRADRH DDRC
#define PORTADRH PORTC
#define PINADRH PINC

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readMEMinit(){
	PORTCTRL|=(1<<OE);
	PORTCTRL|=(1<<CE);
	PORTCTRL|=(1<<WE);
	PORTDAT=0;
	DDRDAT=0;
	_delay_loop_1(1);
}
uint8_t readMEM(uint16_t address){
	PORTADRL=address;
	PORTADRH=address>>8;
	_delay_loop_1(1); //187ns delay.
	PORTCTRL &= ~(1<<CE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL &= ~(1<<OE);
	_delay_loop_1(1); //187ns delay.
	return PINDAT;
}
void readMEMend(){
	PORTCTRL|=(1<<OE)|(1<<CE);
}
uint8_t readMEMbyte(uint16_t address){
	readMEMinit();
	uint8_t temp=readMEM(address);
	readMEMend();
	return temp;
}
uint8_t readMEMblock(uint16_t address,uint8_t* data, uint16_t size){
	if (size>0){
		readMEMinit();
		for(uint16_t i=address;i<address+size;i++){
			*(data+i)=readMEM(address+i);
		}
		readMEMend();
		return 1;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void writeMEMinit(){
	//PORTCTRL &= ~(1<<CE);
	PORTCTRL |= (1<<CE);
	PORTCTRL |= (1<<OE);
	PORTCTRL |= (1<<WE);
	PORTDAT=0;
	DDRDAT=0xFF;
	_delay_loop_1(1); //187ns delay.

}
void writeMEM(uint16_t address,uint8_t data){
	PORTADRL=address;
	PORTADRH=address>>8;
	_delay_loop_1(1); //187ns delay.
	PORTCTRL&=~(1<<WE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL&=~(1<<CE);
	PORTDAT=data;
	_delay_loop_1(1);
	PORTCTRL|=(1<<CE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL|=(1<<WE);

}

void writeMEMend(){
	PORTCTRL|=(1<<OE);
	PORTCTRL|=(1<<CE);
	PORTCTRL|=(1<<WE);
	DDRDAT=0;
	PORTDAT=0;
	_delay_loop_1(1); //187ns delay.
}
void writeMEMbyte(uint16_t address,uint8_t data){
	writeMEMinit();
	writeMEM(address,data);
	writeMEMend();
	//_delay_us(25);

}
uint8_t writeMEMblock(uint16_t address,uint8_t* data, uint16_t size){
	if (size>0){
		writeMEMinit();
		for(uint16_t i=address;i<address+size;i++){
			writeMEM(address+i, *(data+i));
		}
		writeMEMend();
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void disableEEPROMprotection(){
	//for AT28C256 only
	writeMEMinit();
	writeMEM(0x5555,0xAA);
	writeMEM(0x2AAA,0x55);
	writeMEM(0x5555,0x80);
	writeMEM(0x5555,0xAA);
	writeMEM(0x2AAA,0x55);
	writeMEM(0x5555,0x20);
	writeMEMend();
}

uint16_t readID(){
	writeMEMinit();
	writeMEM(0x5555,0xAA);
	writeMEM(0x2AAA,0x55);
	writeMEM(0x5555,0x90);
	_delay_us(1);
	writeMEMend();
	uint16_t temp;
	temp=readMEMbyte(0)<<8;
	temp|=readMEMbyte(1);
	return temp;
}

int main(void)
{
	DDRCTRL |= (1<<WE)|(1<<CE)|(1<<OE);//outputs
	DDRCTRL&=~((1<<J0)|(1<<J1)|(1<<J2));//jumpers in

	PORTCTRL |= (1<<WE)|(1<<CE)|(1<<OE)|(1<<J0)|(1<<J1)|(1<<J2);
	DDRDAT = 0x00; // DATA input
	DDRADRL = 0xFF; // Address L out
	DDRADRH = 0xFF; // Address H out
	
	uart_init();
	stdout = &uart_output;
	stdin = &uart_input;
	
	PORTCTRL &= (1<<CE);

	printf("\nSRAM or EEPROM or FLASH interface\n\n");

	//uncomment for AT28C256 only
	//disableEEPROMprotection();
	if (PINCTRL&(1<<J0))
	{
		printf("\nwriting test pattern...\n\n");
		for (uint16_t i=0+OFFSETA;i<=RANGE+OFFSETA;i++)
		{
			writeMEMbyte(i,i+OFFSETV);
		}
	}
	
	while (1)
	{
		uint8_t temp;
		// 	uncomment for 39SF010 ID reading
		// 	printf("/////////////////////////////////_ID_ = 0x%08x",readID());
		
		printf("address\t\t    0    1    2    3    4    5    6    7\t    8    9    a    b    c    d    e    F\t error\n");
		for (uint16_t i=0+OFFSETA;i<=RANGE+OFFSETA; i+=0x10)
		{
			uint8_t error=0;
			printf("0x%04x\t\t",i);
			for (uint8_t j=0;j<0x10; j++){
				if (j==8){
					printf("\t");
				}
				temp=readMEMbyte(i+j);
				if (temp!=(uint8_t)(i+j+OFFSETV))
				error++;
				
				printf(" 0x%02x",temp);

			}
			if(error)
			printf("\t    %02d\n",error);
			else
			printf("\n");
		}
		printf("\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n\n");
		_delay_ms(3000);
	}
}
 */ 
