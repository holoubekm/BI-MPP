#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "libusb.h"

#define	DILTZ(value) \
	if (value < 0) { \
		printf("ERROR at line %d: %s\n", __LINE__, libusb_strerror(value)); \
		return 1; \
	} \


#define BUF_IN 8
#define BUF_OUT 16

uint16_t ven_id = 0x1111;
uint16_t dev_id = 0x2222;

void flushSTDIN() {
    int dummy;
    while ((dummy = getchar()) != '\n' && dummy != EOF);
}

int main(int argc, char* argv[]) {
  int x, y;
  char dummy;
	libusb_context *ctx;
	libusb_init(&ctx);

	libusb_device **list;
	ssize_t count = libusb_get_device_list(NULL, &list);
	printf("Number of devices: %d\n", count);

	libusb_device* dev = NULL;
	struct libusb_device_descriptor devdscr;
	for(x = 0; x < count; x++) {
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

	int transferred = 0;
	int recv = 0;
	unsigned char bufOUT[BUF_OUT];
	unsigned char bufIN[BUF_IN];
        
	printf("Send or Receive data? s/r");
	while(scanf("%c", &dummy) != 1 || (dummy != 's' && dummy != 'r'));
	if(dummy == 's') {
            getc(stdin);
            for(x = 0; x < 10; x++) {
                printf("Write down message: ");
                fgets(bufOUT, sizeof(bufOUT), stdin);
                if(strlen(bufOUT) <= 1)
                    break;
                DILTZ(libusb_bulk_transfer(dh, 0x02, bufOUT, sizeof(bufOUT), &recv, 1000));
                printf("Data sent to device\n");
                Sleep(1000);
            }
	} else {
            for(x = 0; x < 10; x++) {
                //DILTZ(libusb_bulk_transfer(dh, 0x81, bufIN, sizeof(bufIN), &transferred, 10000));
                DILTZ(libusb_interrupt_transfer(dh, 0x81, bufIN, sizeof(bufIN), &transferred, 10000));
                int val = *((int*)bufIN);
                switch(val) {
                    case 1:
                        printf("UP\n");
                        break;
                    case 2:
                        printf("RIGHT\n");
                        break;
                    case 4:
                        printf("DOWN\n");
                        break;
                    case 8:
                        printf("LEFT\n");
                        break;
                    case 16:
                        printf("MIDDLE\n");
                        break;
                    default:
                        printf("TP_VAL: %d\n", val);
                        break;
                }
                Sleep(1000);
            }
	}

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	system("pause");
	return 0;
}
