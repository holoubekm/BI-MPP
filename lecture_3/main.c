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

uint32_t pci_cfg_read(uint32_t addr) {
	uint32_t tmp = inl(CONFADD);
	addr = addr | (tmp & 0x01);
	addr = addr | (((tmp >> 24) & (0x7F)) << 24);
	addr = addr | (0x01 << 31);
	outl(addr, CONFADD);
	return inl(CONFDATA);
}

void pci_cfg_write(uint32_t addr, uint32_t val) {
	uint32_t tmp = inl(CONFADD);
	addr = addr | (tmp & 0x01);
	addr = addr | (((tmp >> 24) & (0x7F)) << 24);
	addr = addr | (0x01 << 31);
	outl(addr, CONFADD);
	outl(val, CONFDATA);
}

uint32_t read_dev_reg(uint8_t bus_num, uint8_t dev_num, uint8_t fun_num, uint8_t reg_num) {
	uint32_t addr = 0;
	addr = addr | ((bus_num & 0xFF) << 16);
	addr = addr | ((dev_num & 0x1F) << 11);
	addr = addr | ((fun_num & 0x07) << 8);
	addr = addr | ((reg_num & 0x3F) << 2);
	return pci_cfg_read(addr);
}

void write_dev_reg(uint8_t bus_num, uint8_t dev_num, uint8_t fun_num, uint8_t reg_num, uint32_t val) {
	uint32_t addr = 0;
	addr = addr | ((bus_num & 0xFF) << 16);
	addr = addr | ((dev_num & 0x1F) << 11);
	addr = addr | ((fun_num & 0x07) << 8);
	addr = addr | ((reg_num & 0x3F) << 2);
	pci_cfg_write(addr, val);
}

void print_bin(uint32_t val) { 
	for(int x = 0; x < 32; x++) {
		printf("%02d ", 31 - x);
	} printf("\n");

	for(int x = 0; x < 32; x++) {
		printf(" %d ", (val >> (31 - x)) & 0x01);
	} printf("\n");
}

void read_first_sector(uint32_t base) {

	// Do registru base+6 (Drive/Head) zapište hodnotu 0xE0 (disk 0, LBA), výběr disku.
	outb(0xE0, base + 0x6);
	// Přečtete registr base+7 (Status). Pokud je bit 7 nulový pokračujte.
	uint8_t out = inb(base + 0x7);
	if(((out >> 7) & 0x1)){
		return;
	}
	
	// Do registru base+6 (Drive/Head) zapište hodnotu 0xE0 (disk 0 master, LBA). Opětovný zápis je nutný, první zápis vybírá disk, druhý zápis aktualizuje nastavení.
	outb(0xE0, base + 0x6);
	// Do registru base+2 (Sektor Count) zapište hodnotu 1, protože budeme číst jen jeden sektor.
	outb(0x01, base + 0x2);
	// Do registru base+3 (LBA0-7) zapište 0.
	outb(0x00, base + 0x3);
	// Do registru base+4 (LBA8-15) zapište 0.
	outb(0x00, base + 0x4);
	// Do registru base+5 (LBA16-23) zapi3te 0.
	outb(0x00, base + 0x5);
	// Do registru base+6 zapište 0xE0 (LBA24-27, disk 0, LBA mód).
	outb(0xE0, base + 0x6);
	// Do registru base+7 zapište 0x20 (čtení disku, LBA0..27).
	outb(0x20, base + 0x7);
	// Čtěte registr base+7 (Status) a testujte bit 7. Pokud je 1 čekejte.
	while(inb(base + 0x7) >> 7);
	// Čtěte registr base+7 (Status) a ověřte, že bit 7 je nula a bit 3 (DRQ) je jedna.
	out = inb(base + 0x7);
	if(((out >> 7) & 0x1) != 0x0 || ((out >> 3) & 0x1) != 0x1) {
		return;
	}
	
	// V cyklu for proveďte 256 čtení registru base+0 šestnáctibitou V/V operací inw(addr). Vyčtete 512 hodnot. Hodnoty zobrazte na obrazovce v šestnáckové soustavě.
	for(uint16_t x = 0; x < 256; x++) {
		uint16_t val = inw(base + 0x0);
		printf("%04x ", val);
	} printf("\n");
}
int main(int argc, char* argv[]){
	iopl(0x03);
	ioperm(CONFADD, 1, 1);
	ioperm(CONFDATA, 1, 1);

	uint16_t sata_ven_id = 0x8086;
	uint16_t sata_dev_id = 0x1e08;
	uint8_t bus_id = 0x0;

	for(uint8_t dev_num = 0; dev_num <= 0x1F; dev_num++) {
		for(uint8_t fun_num = 0; fun_num <= 0xf7; fun_num++) {
			
			uint32_t out = read_dev_reg(bus_id, dev_num, fun_num, 0x0);
			uint16_t dev_id = (out >> 16);
			uint16_t ven_id = (out & 0xffff);

			if(dev_id == sata_dev_id && ven_id == sata_ven_id) {
				printf("dev id: %d\n", dev_id);
				printf("ven id: %d\n\n", ven_id);

				//BAR2
				// write_dev_reg(bus_id, dev_num, fun_num, 0x6, 5);
				uint32_t bar2 = read_dev_reg(bus_id, dev_num, fun_num, 0x6);
				printf("bar2: %x\n", bar2);
				uint32_t base = bar2 & ~0x3;
				printf("base: %x\n", base);

				//Test register existence
				outb(0x5A, base + 0x2);
				out = inb(base + 0x2);
				printf("out: %x\n\n", out);
				if(out == 0x5A) {
					printf("Register \"base + 0x2\" je namapovan.\n");
				} else  {
					printf("Register \"base + 0x2\" neni namapovan!\n");
				}

				//Checkout the allocated size
				write_dev_reg(bus_id, dev_num, fun_num, 0x6, 0xFFFFFFFF);
				bar2 = read_dev_reg(bus_id, dev_num, fun_num, 0x6);
				bar2 = bar2 & 0xFFFFFFFC;
				uint8_t cnt = 0;
				while((bar2 & 0x1) == 0) {
					cnt++;
					bar2 = bar2 >> 1;
				}
				printf("Pocet nulovanych bitu: %d\n", cnt);
				//Change mapped register address
				uint32_t newBase = 0xf110;
				write_dev_reg(bus_id, dev_num, fun_num, 0x6, newBase);
				bar2 = read_dev_reg(bus_id, dev_num, fun_num, 0x6);
				printf("bar2: %x\n", bar2);
				base = bar2 & ~0x3;


				outb(0x5A, base + 0x2);
				out = inb(base + 0x2);
				printf("out: %x\n", out);
				if(out == 0x5A) {
					printf("Register \"base + 0x2\" je namapovan.\n");
				}

				read_first_sector(base);
				break;
			}
		}
	}
	return 0;
}

