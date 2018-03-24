#include "stdafx.h"

#define	DILTZ(value) \
	if ((value) < 0) { \
		printf("ERROR at line %d: %s\n", __LINE__, libusb_strerror((libusb_error)(value))); \
		system("pause"); \
		return 1; \
	} \

#define be_to_int16(buf) (((buf)[0]<<8)|(buf)[1])
#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

#define le_to_int16(buf) (((buf)[1]<<8)|(buf)[0])
#define le_to_int32(buf) (((buf)[3]<<24)|((buf)[2]<<16)|((buf)[1]<<8)|(buf)[0])

static uint8_t cdb_length[256] = {
	//	 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  0
	06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,06,  //  1
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  2
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  3
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  4
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,  //  5
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  6
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  7
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  8
	16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,  //  9
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  A
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,  //  B
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  C
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  D
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  E
	00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,  //  F
};

struct CWB {
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	uint8_t CBWCB[16];

};

struct CSW {
	uint8_t dCSWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;

};

#define TIME 10000
#define SIZE 1024000
uint16_t ven_id = 0x13FE;
uint16_t dev_id = 0x5200;

void fillCommandHeader(CWB& cwb, CSW& csb) {
	cwb.dCBWSignature[0] = 0x55;
	cwb.dCBWSignature[1] = 0x53;
	cwb.dCBWSignature[2] = 0x42;
	cwb.dCBWSignature[3] = 0x43;

	csb.dCSWSignature[0] = 0x55;
	csb.dCSWSignature[1] = 0x53;
	csb.dCSWSignature[2] = 0x42;
	csb.dCSWSignature[3] = 0x53;
}

void fillINQUIRY(CWB& cwb, CSW& csw) {
	memset(&cwb, 0x0, sizeof(CWB));
	memset(&csw, 0x0, sizeof(CSW));
	fillCommandHeader(cwb, csw);

	cwb.dCBWTag = 0x1;
	cwb.dCBWDataTransferLength = 0x24;
	cwb.bmCBWFlags = 0x80;
	cwb.bCBWLUN = 0x00;
	cwb.bCBWCBLength = cdb_length[0x12];

	cwb.CBWCB[0] = 0x12;
	cwb.CBWCB[1] = 0x00;
	cwb.CBWCB[2] = 0x00;
	cwb.CBWCB[3] = 0x00;
	cwb.CBWCB[4] = 0x24;
	cwb.CBWCB[5] = 0x00;

	csw.dCSWTag = 0x1;
}

void fillREQUEST() {
}

void fillSENSE(CWB& cwb, CSW& csw) {
	memset(&cwb, 0x0, sizeof(CWB));
	memset(&csw, 0x0, sizeof(CSW));
	fillCommandHeader(cwb, csw);

	cwb.dCBWTag = 0x1;
	cwb.dCBWDataTransferLength = 0x24;
	cwb.bmCBWFlags = 0x80;
	cwb.bCBWLUN = 0x00;
	cwb.bCBWCBLength = 0x12;

	cwb.CBWCB[0] = cdb_length[0x03];
	cwb.CBWCB[4] = 0x24;

	csw.dCSWTag = 0x1;
}

void fillCAPACITY(CWB& cwb, CSW& csw) {
	memset(&cwb, 0x0, sizeof(CWB));
	memset(&csw, 0x0, sizeof(CSW));
	fillCommandHeader(cwb, csw);

	cwb.dCBWTag = 0x1;
	cwb.dCBWDataTransferLength = 0x08;
	cwb.bmCBWFlags = 0x80;
	cwb.bCBWLUN = 0x00;
	cwb.bCBWCBLength = cdb_length[0x25];

	cwb.CBWCB[0] = 0x25;

	csw.dCSWTag = 0x1;
}


void fillREAD(CWB& cwb, CSW& csw, uint32_t block_size, uint32_t block_start, uint16_t block_count) {
	memset(&cwb, 0x0, sizeof(CWB));
	memset(&csw, 0x0, sizeof(CSW));
	fillCommandHeader(cwb, csw);

	cwb.dCBWTag = 0x1;
	cwb.dCBWDataTransferLength = block_size * block_count;
	cwb.bmCBWFlags = 0x80;
	cwb.bCBWLUN = 0x00;
	cwb.bCBWCBLength = cdb_length[0x28];

	cwb.CBWCB[0] = 0x28;
	cwb.CBWCB[2] = (block_start >> 24) & 0xFF;
	cwb.CBWCB[3] = (block_start >> 16) & 0xFF;
	cwb.CBWCB[4] = (block_start >> 8) & 0xFF;
	cwb.CBWCB[5] = (block_start >> 0) & 0xFF;

	cwb.CBWCB[7] = (block_count >> 8) & 0xFF;
	cwb.CBWCB[8] = block_count & 0xFF;

	csw.dCSWTag = 0x1;
}

void fillWRITE() {


}


void display_buffer_hex(unsigned char *buffer, unsigned size)
{
	unsigned i, j, k;

	for (i = 0; i<size; i += 16) {
		printf("\n  %08x  ", i);
		for (j = 0, k = 0; k<16; j++, k++) {
			if (i + j < size) {
				printf("%02x", buffer[i + j]);
			}
			else {
				printf("  ");
			}
			printf(" ");
		}
		printf(" ");
		for (j = 0, k = 0; k<16; j++, k++) {
			if (i + j < size) {
				if ((buffer[i + j] < 32) || (buffer[i + j] > 126)) {
					printf(".");
				}
				else {
					printf("%c", buffer[i + j]);
				}
			}
		}
	}
	printf("\n");
}

void test() {
	uint8_t fat32[512];
	std::ifstream input;
	input.open("C:\\Users\\Martin\\Desktop\\BI-MPP\\lecture_5\\Win32\\Debug\\examples\\dump.bin", std::fstream::binary);
	input.read((char*)fat32, 512);
	input.close();

	uint32_t offset = 2048;
	printf("Found partition %d, offset: %d blocks", 0, offset);

	uint8_t* ptr = fat32;
	uint16_t byter_per_sec = le_to_int16(&ptr[0x0B]);//	16 Bits	Always 512 Bytes
	uint8_t sec_per_clus = ptr[0x0D];//	8 Bits	1, 2, 4, 8, 16, 32, 64, 128
	uint16_t reserv_sec_cnt = le_to_int16(&ptr[0x0E]);//	16 Bits	Usually 0x20
	uint8_t num_fats = ptr[0x10];//	8 Bits	Always 2
	uint32_t sec_per_fat = le_to_int32(&ptr[0x24]); //32 Bits	Depends on disk size
	uint32_t fst_clus_root = le_to_int32(&ptr[0x2C]);//	32 Bits	Usually 0x00000002
	if (le_to_int16(&ptr[0x1FE]) != 0xAA55) {
		printf("Ending 0xAA55 doesn't match\n");
	}

	uint32_t fat_begin_lba = offset + reserv_sec_cnt;
	uint32_t cluster_begin_lba = offset + reserv_sec_cnt + (num_fats * sec_per_fat);
	uint8_t sectors_per_cluster = sec_per_clus;
	uint32_t root_dir_first_cluster = fst_clus_root;

	//lba_addr = cluster_begin_lba + (cluster_number - 2) * sectors_per_cluster;
}

int main(int argc, char* argv[]) {
	test();
	system("Pause");
	return 0;

	const struct libusb_version* ver;
	libusb_context *ctx;
	libusb_init(&ctx);
	libusb_set_debug(ctx, 3);
	ver = libusb_get_version();

	printf("LIBUSB version %d.%d.%d.%d\n", ver->major, ver->minor, ver->micro, ver->nano);

	libusb_device **list;
	ssize_t count = libusb_get_device_list(NULL, &list);
	printf("Number of devices: %d\n", count);

	libusb_device* dev = NULL;
	struct libusb_device_descriptor devdscr;
	for (int x = 0; x < count; x++) {
		libusb_get_device_descriptor(list[x], &devdscr);
		printf("%04X:%04X addr: (%d)\n", devdscr.idVendor, devdscr.idProduct, libusb_get_device_address(list[x]));
		if (devdscr.idVendor == ven_id && devdscr.idProduct == dev_id) {
			printf("\tDevice found!\n");
			dev = list[x];
		}
	}
	printf("\n");

	if (dev == nullptr) {
		printf("Device not found!\n");
		system("pause");
		return 1;
	}

	struct libusb_device_descriptor dev_dsc;
	struct libusb_config_descriptor* cfg_dsc;
	DILTZ(libusb_get_device_descriptor(dev, &dev_dsc));
	DILTZ(libusb_get_config_descriptor(dev, 0, &cfg_dsc));


	const libusb_interface *inter;
	const libusb_interface_descriptor *interdesc;
	const libusb_endpoint_descriptor *epdesc;
	for (int i = 0; i < (int)cfg_dsc->bNumInterfaces; i++) {
		inter = &cfg_dsc->interface[i];
		printf("Number of alternate settings: %d\n", inter->num_altsetting);
		for (int j = 0; j < inter->num_altsetting; j++) {
			interdesc = &inter->altsetting[j];
			printf("\tInterface Number: %d\n", (int)interdesc->bInterfaceNumber);
			printf("\tNumber of endpoints: %d\n", (int)interdesc->bNumEndpoints);
			for (int k = 0; k < (int)interdesc->bNumEndpoints; k++) {
				epdesc = &interdesc->endpoint[k];
				printf("\t\tDescriptor Type: %d\n", (int)epdesc->bDescriptorType);
				printf("\t\tEP Address: %d\n", (int)epdesc->bEndpointAddress);
				printf("\t\tEP Direction: %s\n", (epdesc->bEndpointAddress & LIBUSB_ENDPOINT_IN) ? "IN" : "OUT");
			}
		}
	}
	libusb_free_device_list(list, 1);


	libusb_device_handle* dh = libusb_open_device_with_vid_pid(ctx, dev_dsc.idVendor, dev_dsc.idProduct);
	if (dh == NULL) {
		printf("ERROR at line %d: %s\n", __LINE__, "cannot open the device");
		return 1;
	}
	int interface = 0;
	DILTZ(libusb_set_configuration(dh, cfg_dsc->bConfigurationValue));
	libusb_free_config_descriptor(cfg_dsc);
	DILTZ(libusb_claim_interface(dh, interface));

	int val = 0x0;
	uint8_t lun = 0;
	printf("Reading Max LUN:\n");
	DILTZ(val = libusb_control_transfer(dh, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0xFE, 0, 0, &lun, 1, TIME));
	if (val == 0)
		lun = 0;
	printf("Max LUN = %d\n", lun);

	uint8_t endpointIN = 129;
	uint8_t endpointOUT = 2;

	int recv = 0;
	unsigned char buffer[SIZE];
	CWB cwb;
	CSW csw;


	//INQUIRY
	fillINQUIRY(cwb, csw);
	memset(buffer, 0, SIZE);
	DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, buffer, 0x24, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
	if (csw.bCSWStatus != 0x0) {
		printf("Error while receiving data.\n");
		system("pause");
		return 0;
	}


	//CAPACITY
	fillCAPACITY(cwb, csw);
	memset(buffer, 0, SIZE);
	DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, buffer, 0x08, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
	if (csw.bCSWStatus == 0x1) {
		//SENSE
		printf("Error returned, reading SENSE data\n");
		fillSENSE(cwb, csw);
		memset(buffer, 0, SIZE);
		DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
		DILTZ(libusb_bulk_transfer(dh, endpointIN, buffer, 0x12, &recv, TIME));
		DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
		if (csw.bCSWStatus != 0x0) {
			printf("Error while receiving data.\n");
			system("pause");
			return 0;
		}
	}
	else if (csw.bCSWStatus != 0x0) {
		printf("Error while receiving data.\n");
		system("pause");
		return 0;
	}

	uint32_t max_lba = be_to_int32(&buffer[0]);
	uint32_t block_size = be_to_int32(&buffer[4]);
	double device_size = ((double)(max_lba + 1))*block_size / (1024 * 1024 * 1024);
	printf("max_lba: %d, block_size: %d, device_size: %fgb\n", max_lba, block_size, device_size);

	/*uint8_t block_cnt = 10;
	uint8_t block_start = 2047;*/
	//READ
	/*printf("Attempting to read %d bytes:\n", block_size);
	fillREAD(cwb, csw, block_size, block_start, block_cnt);
	memset(buffer, 0, SIZE);
	DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, buffer, block_cnt * block_size, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
	if (csw.bCSWStatus != 0x0) {
		printf("Error while receiving data.\n");
		system("pause");
		return 0;
	}*/
	//display_buffer_hex(buffer, block_size * block_cnt);

	uint8_t block_cnt = 1;
	uint8_t block_start = 0;
	uint8_t mbr[512];
	fillREAD(cwb, csw, block_size, block_start, block_cnt);
	memset(mbr, 0, 512);
	DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, mbr, 512, &recv, TIME));
	DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
	if (csw.bCSWStatus != 0x0) {
		printf("Error while receiving data.\n");
		system("pause");
		return 0;
	}

	display_buffer_hex(mbr, 512);
	uint8_t* ptr = mbr + 446;
	for (int x = 0; x < 4; x++) {
		if (ptr[4] == 0x0B || ptr[4] == 0x0C) {
			uint32_t offset = be_to_int32(&ptr[8]);
			printf("Found partition %d, offset: %d blocks", x, offset);

			uint8_t fat32[512];
			block_cnt = 1;
			fillREAD(cwb, csw, block_size, offset, block_cnt);
			memset(fat32, 0, 512);
			DILTZ(libusb_bulk_transfer(dh, endpointOUT, (unsigned char*)(&cwb), 31, &recv, TIME));
			DILTZ(libusb_bulk_transfer(dh, endpointIN, fat32, block_cnt * block_size, &recv, TIME));
			DILTZ(libusb_bulk_transfer(dh, endpointIN, (unsigned char*)(&csw), sizeof(csw), &recv, TIME));
			if (csw.bCSWStatus != 0x0) {
				printf("Error while receiving data.\n");
				system("pause");
				return 0;
			}

			ptr = fat32;
			uint16_t byter_per_sec = be_to_int16(&ptr[0x0B]);//	16 Bits	Always 512 Bytes
			uint8_t sec_per_clus = ptr[0x0D];//	8 Bits	1, 2, 4, 8, 16, 32, 64, 128
			uint16_t reserv_sec_cnt = be_to_int16(&ptr[0x0E]);//	16 Bits	Usually 0x20
			uint8_t num_fats = ptr[0x10];//	8 Bits	Always 2
			uint32_t sec_per_fat = be_to_int32(&ptr[0x24]); //32 Bits	Depends on disk size
			uint32_t fst_clus_root = be_to_int32(&ptr[0x2C]);//	32 Bits	Usually 0x00000002
			if (be_to_int16(&ptr[0x1FE]) != 0xAA55) {
				printf("Ending 0xAA55 doesn't match\n");
			}

			uint32_t fat_begin_lba = offset + reserv_sec_cnt;
			uint32_t cluster_begin_lba = offset + reserv_sec_cnt + (num_fats * sec_per_fat);
			uint8_t sectors_per_cluster = sec_per_clus;
			uint32_t root_dir_first_cluster = fst_clus_root;

			//lba_addr = cluster_begin_lba + (cluster_number - 2) * sectors_per_cluster;
		}
	}


	libusb_release_interface(dh, interface);
	libusb_close(dh);
	libusb_exit(ctx);

	system("pause");
	return 0;
}
