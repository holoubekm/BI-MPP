#include <stdlib.h>
#include <stdio.h>
#include <sys/io.h>
#include <stdint.h>
 
#define CONFADD 		  0xCF8
#define CONFDATA 		  0xCFC

void print_bin(uint32_t val);

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

	for(uint16_t bus_num = 0; bus_num <= 0xFF; bus_num++) {
		for(uint8_t dev_num = 0; dev_num <= 0x1F; dev_num++) {
			for(uint8_t fun_num = 0; fun_num <= 0x07; fun_num++) {
				uint8_t reg_num = 0x0;
				uint32_t val = 0;
				val = val | ((bus_num & 0xFF) << 16);
				val = val | ((dev_num & 0x1F) << 11);
				val = val | ((fun_num & 0x07) << 8);
				val = val | ((reg_num & 0x3F) << 2);
				uint32_t out = pci_cfg_read(val);
				
				uint16_t dev_id = (out >> 16);
				uint16_t ven_id = (out & 0xffff);

				if(dev_id != 0xffff || ven_id != 0xffff) {
					printf("%04x:%04x\n", ven_id, dev_id);
				}
			}
		}
	}
	return 0;
}
