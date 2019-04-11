// Copyright 2019 M.Widomski
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// EEPROM/FLASH/SRAM interface
// implemented: simple read/write functions and Flash ID read and erasing
// main function is testing memory
// Tested with:
// 62256 SRAM
// FM1808 FRAM
// 39FS010 FLASH with A15 tied LOW
// ATmega644P




#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include "uart.h"			// baud rate defined in uart.c

//////////////////////////////// memory testing parameters
#define RANGE 0x2ff			  // max tested address
#define OFFSETA 0x00	  	  // starting address
#define OFFSETV 0x0			  // value for first memory cell (will be incremented)


//////////////////////////////// CONTROL BUS
#define DDRCTRL DDRD
#define PORTCTRL PORTD
#define PINCTRL PIND
#define WE PD7
#define OE PD6
#define CE PD5

#define J0 PD4				// enable writing pattern
#define J1 PD3				// enable chip/sector erase
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// READING

// read initialize - set ports and control signals
void readInit(){
	PORTCTRL|=(1<<OE);
	PORTCTRL|=(1<<CE);
	PORTCTRL|=(1<<WE);
	PORTDAT=0;
	DDRDAT=0;
	_delay_loop_1(1);
}

// "naked" one byte reading
uint8_t readMem(uint16_t address){
	PORTADRL=address;
	PORTADRH=address>>8;
	_delay_loop_1(1); //187ns delay.
	PORTCTRL &= ~(1<<CE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL &= ~(1<<OE);
	_delay_loop_1(1); //187ns delay.
	return PINDAT;
}

// read finalize - clearing signals, clearing address
void readEnd(){
	PORTCTRL|=(1<<OE)|(1<<CE);
	PORTADRL=0;
	PORTADRH=0;
}

// full byte read solution - initializing, reading and finalizing
uint8_t readByte(uint16_t address){
	readInit();
	uint8_t temp=readMem(address);
	readEnd();
	return temp;
}

// block read solution
uint8_t readBlock(uint16_t address, uint8_t* data, uint16_t size){
	if (size>0){
		for(uint16_t i=address;i<address+size;i++){
			*(data+i)=readByte(address+i);
		}
		return 1;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// WRITING

//write initialize  - set ports and control signals
void writeInit(){
	//PORTCTRL &= ~(1<<CE);
	PORTCTRL |= (1<<CE);
	PORTCTRL |= (1<<OE);
	PORTCTRL |= (1<<WE);
	PORTDAT=0;
	DDRDAT=0xFF;
	_delay_loop_1(1); //187ns delay.

}

// "naked" one byte writing
void writeMem(uint16_t address, uint8_t data){
	PORTADRL=address;
	PORTADRH=address>>8;
	_delay_loop_1(1); //187ns delay.
	PORTCTRL&=~(1<<WE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL&=~(1<<CE);
	PORTDAT=data;
	_delay_loop_1(1); //187ns delay.
	PORTCTRL|=(1<<CE);
	_delay_loop_1(1); //187ns delay.
	PORTCTRL|=(1<<WE);

}

// write finalize - clearing signals, clearing address
void writeEnd(){
	PORTCTRL|=(1<<OE);
	PORTCTRL|=(1<<CE);
	PORTCTRL|=(1<<WE);
	DDRDAT=0;
	PORTDAT=0;
	PORTADRL=0;
	PORTADRH=0;
	_delay_loop_1(1); //187ns delay.
}

// full byte write solution (not for flash) - initializing, reading and finalizing
void writeByte(uint16_t address, uint8_t data){
	writeInit();
	writeMem(address,data);
	writeEnd();
	//_delay_us(25);
}

// flash full byte write solution - initializing, reading and finalizing
void flashWritebyte(uint16_t address, uint8_t data){
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0xA0);
	writeMem(address,data);
	writeEnd();
	
	while(readByte(address)!=data){			//data polling - waiting for end of program operation
		_delay_loop_1(1); //187ns delay.
	}
	
	//_delay_us(25);
}

// block write solution - not for flash
uint8_t writeBlock(uint16_t address, uint8_t* data, uint16_t size){
	if (size>0){
		writeInit();
		for(uint16_t i=address;i<address+size;i++){
			writeMem(address+i, *(data+i));
		}
		writeEnd();
		return 1;
	}
	return 0;
}

// flash block write solution
uint8_t flashWriteBlock(uint16_t address, uint8_t* data, uint16_t size){
	if (size>0){
		for(uint16_t i=address;i<address+size;i++){
			flashWritebyte(address+i, *(data+i));
		}
		return 1;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// SPECIAL

void disableEEPROMprotection(){
	//for AT28C256 only
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x80);
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x20);
	writeEnd();
}
void flashShipErase(){
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x80);
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x10);
	writeEnd();
	while(readByte(0)!=0xff){			//data polling - waiting for end of erase operation
		_delay_loop_1(1); //187ns delay.
	}
	
}
void flashSectorErase(uint8_t addressH){
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x80);
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(addressH<<8,0x30);
	writeEnd();
	while(readByte(addressH<<8)!=0xff){			//data polling - waiting for end of erase operation
		_delay_loop_1(1); //187ns delay.
	}
	
}

// reading Flash ID H-family, L-device
uint16_t flashReadID(){
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0x90);
	writeEnd();
	uint16_t temp;
	temp=readByte(0)<<8;
	temp|=readByte(1);
	writeInit();
	writeMem(0x5555,0xAA);
	writeMem(0x2AAA,0x55);
	writeMem(0x5555,0xF0);
	writeEnd();
	_delay_us(50);
	return temp;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN

int main(void)
{
	DDRCTRL |= (1<<WE)|(1<<CE)|(1<<OE);		//set outputs as outputs
	DDRCTRL&=~((1<<J0)|(1<<J1)|(1<<J2));	//set jumpers as inputs
	PORTCTRL |= (1<<WE)|(1<<CE)|(1<<OE)|(1<<J0)|(1<<J1)|(1<<J2); // set control signals high and turn on pull-ups on jumpers
	DDRDAT = 0x00;	// DATA as input
	DDRADRL = 0xFF; // Address L as output
	DDRADRH = 0xFF; // Address H as output
	
	uart_init();			//initialize UART
	stdout = &uart_output;	//send stdout to UART
	stdin = &uart_input;	//send UART to stdin
	
	printf("\nSRAM or EEPROM or FLASH interface\n\n");
	
	uint16_t id=flashReadID();
	printf("\ndevice ID: 0x%04x\n\n",id);
	
	if (PINCTRL&(1<<J0))
	{
		printf("\nwriting test pattern...\n\n");
		
		
		if (PINCTRL&(1<<J1)){
			//flashChipErase();
			flashSectorErase(0);
		}
		for (uint16_t i=0+OFFSETA;i<=RANGE+OFFSETA;i++)
		{
			flashWritebyte(i,i+OFFSETV);
		}
	}
	
	while (1)
	{
		uint8_t temp;

		
		printf("address\t\t    0    1    2    3    4    5    6    7\t    8    9    a    b    c    d    e    F\t error\n");
		for (uint16_t i=0+OFFSETA;i<=RANGE+OFFSETA; i+=0x10)
		{
			uint8_t error=0;
			printf("0x%04x\t\t",i);
			for (uint8_t j=0;j<0x10; j++){
				if (j==8){
					printf("\t");
				}
				temp=readByte(i+j);
				if (temp!=(uint8_t)(i+j+OFFSETV)){
					error++;
				}
				printf(" 0x%02x",temp);
			}
			if(error){
				printf("\t    %02d\n",error);
			}
			else{
				printf("\n");
			}
		}
		printf("\n///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n\n");
		_delay_ms(3000);
	}
}