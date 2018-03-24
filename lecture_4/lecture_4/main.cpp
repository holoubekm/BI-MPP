#include <cstdio>
#include <cstdlib>
#include "libusb.h"

#define	DILTZ(value) \
	if (value < 0) { \
		printf("ERROR at line %d: %s\n", __LINE__, libusb_strerror((libusb_error)value)); \
		return 1; \
	} \


#define SIZE 1024
uint16_t ven_id = 0x04B4;
uint16_t dev_id = 0x0033;


int main(int argc, char* argv[]) {
	const struct libusb_version* ver;
	libusb_context *ctx;
	libusb_init(&ctx);
	ver = libusb_get_version();

	printf("LIBUSB version %d.%d.%d.%d\n", ver->major, ver->minor, ver->micro, ver->nano);

	libusb_device **list;
	ssize_t count = libusb_get_device_list(NULL, &list);
	printf("Number of devices: %d\n", count);

	libusb_device* dev = NULL;
	struct libusb_device_descriptor devdscr;
	for(int x = 0; x < count; x++) {
		libusb_get_device_descriptor(list[x], &devdscr); 
		printf("%04X:%04X addr: (%d)\n", devdscr.idVendor, devdscr.idProduct, libusb_get_device_address(list[x]));
		if(devdscr.idVendor == ven_id && devdscr.idProduct == dev_id) {
			printf("\tDevice found!\n");
			dev = list[x];
		}
	}

	struct libusb_device_descriptor dev_dsc;
	struct libusb_config_descriptor* cfg_dsc;
	DILTZ(libusb_get_device_descriptor(dev, &dev_dsc));
	DILTZ(libusb_get_config_descriptor(dev, 0, &cfg_dsc));

	libusb_device_handle* dh = libusb_open_device_with_vid_pid(ctx, dev_dsc.idVendor, dev_dsc.idProduct);
	if (dh == NULL) {
		printf("ERROR at line %d: %s\n", __LINE__, "cannot open the device");
		return 1;
	}
	DILTZ(libusb_set_configuration(dh, cfg_dsc->bConfigurationValue));
	DILTZ(libusb_claim_interface(dh, 0));

	unsigned char buf[SIZE];
	int transferred = 0;

	int xAbs = 0, yAbs = 0, zAbs = 0;
	while(true) {
		DILTZ(libusb_interrupt_transfer(dh, 0x81, buf, SIZE, &transferred, 10000));
		
		bool left = buf[0] & 0x01;
		bool right = (buf[0] >> 1) & 0x01;
		bool middle = (buf[0] >> 2) & 0x01;
		bool xSign = (buf[0] >> 4) & 0x01;
		bool ySign = (buf[0] >> 5) & 0x01;

		//int x = xSign ? -buf[1] : buf[1];
		//int y = ySign ? -buf[2] : buf[2];

		int x = buf[1] > 128 ? buf[1] - 256 : buf[1];
		int y = buf[2] > 128 ? buf[2] - 256 : buf[2];
		int z = buf[3] > 128 ? buf[3] - 256 : buf[3];

		xAbs += x;
		yAbs += y;
		zAbs += z;

		// 		 	D7 		D6 		D5 		D4 		D3 		D2 	D1 	D0
		// 1. byte 	Yover 	Xover 	Ysign 	Xsign 	Tag 	M 	R 	L
		// 2. byte 	X7 		X6 		X5	 	X4	 	X3	 	X2 	X1 	X0
		// 3. byte 	Y7 		Y6 		Y5	 	Y4	 	Y3	 	Y2 	Y1 	Y0
		// 4. byte 	Z7 		Z6 		Z5	 	Z4	 	Z3	 	Z2 	Z1 	Z0

		system("clear");
		printf("%4d:%4d %3d:%3d L[%d] M[%d %d %d] R[%d]\n", xAbs, yAbs, x, y, left, middle, z, zAbs, right);
		if (right)
			break;
	}

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	system("pause");
	return 0;
}
