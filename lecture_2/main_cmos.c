#include <stdlib.h>
#include <stdio.h>
#include <sys/io.h>
#include <stdint.h>
 

#define CMOS_PORT_ADDRESS 0x74
#define CMOS_PORT_DATA    0x75
#define CONFADD 		  0xCF8
#define CONFDATA 		  0xCFC

void print_bin(uint32_t val);

uint8_t read_cmos_reg(uint8_t val) {
	outb(val, CMOS_PORT_ADDRESS);
	return inb(CMOS_PORT_DATA);
}

uint8_t to_dec(uint8_t val) {
	return (val >> 4) * 10 + (val & 0x0F);
}

uint32_t pci_cfg_read(uint32_t val) {
	uint32_t tmp = inl(CONFADD);
	val = val | (tmp & 0x01);
	val = val | (((tmp >> 24) & (0x7F)) << 24);
	val = val | (0x01 << 31);
	outl(val, CONFADD);
	return inl(CONFDATA);
}

void pci_cfg_write(uint32_t addr, uint32_t val) {
	uint32_t tmp = inl(CONFADD);
	val = val | (tmp & 0x01);
	val = val | (((tmp >> 24) & (0x7F)) << 24);
	val = val | (0x01 << 31);
	outl(val, CONFADD);
	outl(val, CONFDATA);
}

void print_bin(uint32_t val) { 
	for(int x = 0; x < 32; x++) {
		printf("%02d ", 31 - x);
	} printf("\n");

	for(int x = 0; x < 32; x++) {
		printf(" %d ", (val >> (31 - x)) & 0x01);
	} printf("\n");
}

int main(int argc, char* argv[]){
	iopl(0x03);
	ioperm(CONFADD, 1, 1);
	ioperm(CONFDATA, 1, 1);

	while(((read_cmos_reg(0x0a) >> 6) & 0x01));

	unsigned char sec = to_dec(read_cmos_reg(0x00));
	unsigned char min = to_dec(read_cmos_reg(0x02));
	unsigned char hrs = to_dec(read_cmos_reg(0x04));

	unsigned char wkd = to_dec(read_cmos_reg(0x06));
	
	char* diw;
	switch(wkd) {
		case 0: diw = "nedele"; break;
		case 1: diw = "pondeli"; break;
		case 2: diw = "utery"; break;
		case 3: diw = "streda"; break;
		case 4: diw = "ctvrtek"; break;
		case 5: diw = "patek"; break;
		case 6: diw = "sobota"; break;
	}
	unsigned char yrs = to_dec(read_cmos_reg(0x09));
	unsigned char mth = to_dec(read_cmos_reg(0x08));
	unsigned char day = to_dec(read_cmos_reg(0x07));

	printf("%02d:%02d:%02d %s %d.%d.20%d\n", hrs, min, sec, diw, day, mth, yrs);
	return 0;
}
