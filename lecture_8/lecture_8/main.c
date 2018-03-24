#include "cpu.h"
#include "log.h"
#include "display.h"
#include "touchpad.h"
#include "usb_lib.h"

#define EP0_OUT_BUF_SIZE 64
#define EP0_IN_BUF_SIZE 64
#define EP1_IN_BUF_SIZE 8
#define EP2_OUT_BUF_SIZE 16

DECLARE_EP_BEGIN
    DECLARE_EP(ep0); 
    DECLARE_EP(ep1); // IN
    DECLARE_EP(ep2); // OUT
DECLARE_EP_END

DECLARE_BUFFER_BEGIN
    DECLARE_BUFFER(ep0_buf_out, EP0_OUT_BUF_SIZE);
    DECLARE_BUFFER(ep0_buf_in, EP0_IN_BUF_SIZE);
    DECLARE_BUFFER(ep1_buf_in, EP1_IN_BUF_SIZE);
    DECLARE_BUFFER(ep2_buf_out, EP2_OUT_BUF_SIZE);
DECLARE_BUFFER_END

enum DEV_STATE {
	DEV_DETACHED,
	DEV_ATTACHED,
	DEV_POWERED,
	DEV_DEFAULT,
	DEV_ADDRESSED,
	DEV_CONFIGURED
};

enum EP_STATE {
	EP_DEFAULT,
	EP_SETUP,
	EP_DATA1_IN,
	EP_ACK_IN,
	EP_ACK_OUT,
	EP_ACK_ADDR,

	EP_HEADER,
	EP_SEND,
	EP_RECV,
	EP_ACK_SEND,
	EP_ACK_RECV,
};

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

const device_descr_t devdsc __attribute__((space(auto_psv))) = {
    sizeof(device_descr_t), 1, 0x0200, 0, 0, 0, 64, 0x1111, 0x2222, 0x0100, 0, 0, 0, 1
};

const struct config {
    config_descriptor_t config_descr;
    interf_descriptor_t interf_descr;
    endpoint_descriptor_t endp_descr1;
    endpoint_descriptor_t endp_descr2;
} CONFIG __attribute__((space(auto_psv))) = {
    {
        sizeof(config_descriptor_t),
        2,               // bDescriptorType
        sizeof(CONFIG),
        1,               // bNumInterfaces
        0x01,            // bConfigurationValue
        0,               // iConfiguration
        0x80,            // bmAttributes
        50               // bMaxPower
    },
    {
        sizeof(interf_descriptor_t),
        4,               // bDescriptorType
        0,               // bInterfaceNumber
        0x0,             // bAlternateSetting
        2,               // bNumEndpoints
        //0x03,            // bInterfaceClass
        //0x01,            // bInterfaceSubClass
        //0x02,            // bInterfaceProtocol
        0xff,
        0xff,
        0xff,
        0                // iInterface
    },
    {
        sizeof(endpoint_descriptor_t),
        5,               // bDescriptorType
        0x81,             // bEndpointAddress
        0x03,               // bmAttributes
        8,              // wMaxPacketSize
        10                // bInterval
    }
    ,
    {
        sizeof(endpoint_descriptor_t),
        5,               // bDescriptorType
        0x02,             // bEndpointAddress
        0x02,               // bmAttributes
        16,              // wMaxPacketSize
        0                // bInterval
    }
};

int tp_state = 0;
int ep1_state = 0;
int ep2_state = 0;
int ep2_dir = 0;
static int status = 0;
enum EP_STATE ep0_state = EP_DEFAULT;
enum DEV_STATE dev_state = DEV_DEFAULT;

void process_control_transfer(int ep) {
	int cur_size;
	static byte usb_addr;

	if(is_setup((ep & 0x80), EP(ep0))) {
		ep0_state = EP_SETUP;
	}

	if(ep0_state == EP_SETUP) {
		log_str("EP_SETUP\n");
		//Strana 250
		usb_device_req_t req;
		copy_from_buffer(ep0_buf_out, &req, sizeof(usb_device_req_t));
		log_int(" 0x%02lx\n", req.bmRequestType);
		log_int(" 0x%02lx\n", req.bRequest);
		log_int(" 0x%04lx\n", req.wValue);
		log_int(" 0x%04lx\n", req.wIndex);
		log_int(" 0x%04lx\n", req.wLength);
		switch(req.bRequest) {
			case GET_DESCRIPTOR:
				log_str(" GET_DESCRIPTOR\n");
				switch((req.wValue >> 8) & 0xFF){
					case DSC_DEVICE:
						log_str("  DSC_DEVICE\n");
						cur_size = MIN(sizeof(devdsc), req.wLength);
						copy_to_buffer(ep0_buf_in, &devdsc, cur_size);
						usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, cur_size);
						ep0_state = EP_DATA1_IN;
					break;
					case DSC_CONFIGURATION:
						log_str("  DSC_CONFIGURATION\n");
						cur_size = MIN(sizeof(CONFIG), req.wLength);
						copy_to_buffer(ep0_buf_in, &CONFIG, cur_size);
						usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, cur_size);
						ep0_state = EP_DATA1_IN;
					break;
				}
			break;
			case SET_ADDRESS:
				log_str(" SET_ADDRESS\n");
				usb_addr = req.wValue & 0xFF;
				usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, 0);
				ep0_state = EP_ACK_ADDR;
			break;
			case GET_STATUS:
				log_str("  GET_STATUS\n");
				cur_size = MIN(sizeof(status), req.wLength);
				copy_to_buffer(ep0_buf_in, &status, cur_size);
				usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, cur_size);
				ep0_state = EP_DATA1_IN;
			break;
			case SET_CONFIGURATION:
				log_str("  SET_CONFIGURATION\n");
				usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_IN, ep0_buf_in, 0);
				ep0_state = EP_ACK_IN;
			break;

		}
	} else if(ep0_state == EP_DATA1_IN) {
		log_str("EP_DATA1_IN\n");
		usb_ep_transf_start(EP(ep0), USB_TRN_DATA1_OUT, ep0_buf_out, EP0_OUT_BUF_SIZE);
		ep0_state = EP_ACK_IN;
	} else if(ep0_state == EP_ACK_IN) {
		log_str("EP_ACK_IN\n");
		usb_ep_transf_start(EP(ep0), USB_TRN_SETUP, ep0_buf_out, EP0_OUT_BUF_SIZE);
		ep0_state = EP_SETUP;
	} else if (ep0_state == EP_ACK_ADDR) {
		log_str("EP_ACK_ADDR\n");
		usb_set_address(usb_addr);
		usb_ep_transf_start(EP(ep0), USB_TRN_SETUP, ep0_buf_out, EP0_OUT_BUF_SIZE);
		ep0_state = EP_SETUP;
	}
}

void process_ep_transfer(int ep) {
	// log_str("process_ep_transfer\n");
	if(ep == 0x81) {
		if(tp_state = 1) {
			log_str("DATA SENT OK\n");
			tp_state = 2;
		}
	} else if(ep == 0x02) {
		if(ep2_state == 1) {	
			// log_str("EP2 DONE\n");
			char buf[EP2_OUT_BUF_SIZE+1];
			copy_from_buffer(ep2_buf_out, &buf, EP2_OUT_BUF_SIZE);
			buf[EP2_OUT_BUF_SIZE] = '\0';
			log_str(buf);
			log_str("\n");
			ep2_state = 0;
		}
	}
}

int cnt = 0;
int main(int argc, char* argv[]) {
	cpu_init();
	disp_init();
	touchpad_init();
	log_init();

	while(1) {
		switch(dev_state) {
			case DEV_DETACHED:
				log_str("DEV_DETACHED\n");
				usb_init();
				while(!is_attached());
				usb_enable();
				dev_state = DEV_ATTACHED;
			break;
			case DEV_ATTACHED:
				log_str("DEV_ATTACHED\n");
				while(!is_powered());
				dev_state = DEV_POWERED;
			break;
			case DEV_POWERED:
			case DEV_DEFAULT:
			case DEV_ADDRESSED:
			case DEV_CONFIGURED:
				if(is_reset()) {
					log_str("DEV_RESET\n");
					usb_reset();
					usb_init_ep(0, EP_SETUP_INOUT, EP(ep0));
					usb_init_ep(1, EP_IN, EP(ep1));
					usb_init_ep(2, EP_OUT, EP(ep2));

					usb_ep_transf_start(EP(ep0), USB_TRN_SETUP, ep0_buf_out, EP0_OUT_BUF_SIZE);
					dev_state = DEV_DEFAULT;
					ep0_state = EP_DEFAULT;
					ep1_state = 0;
					ep2_state = 0;
					tp_state = 0;
					break;
				}

				if (is_idle()) {
					log_str("IDLE\n");
					continue;
				}

				if(is_sof()) {
					// log_str("SOF\n");
					continue;
				}

				int touchpad_status;
				if (cnt++ % 5000 == 0 && (touchpad_status = get_touchpad_status()) != 0) {
					if(tp_state == 0) {
						int trn = (ep1_state != 1) ? USB_TRN_DATA1_IN : USB_TRN_DATA0_IN;
						int cur_size = MIN(sizeof(touchpad_status), EP1_IN_BUF_SIZE);
						copy_to_buffer(ep1_buf_in, &touchpad_status, cur_size);
						usb_ep_transf_start(EP(ep1), trn, ep1_buf_in, EP1_IN_BUF_SIZE);
						ep1_state = ep1_state ? 0 : 1;
						tp_state = 1;
					}
				} else {
					if(tp_state == 2) {
						tp_state = 0;
					}
				}

				if(ep2_state == 0) {
					int trn = (ep2_dir != 1) ? USB_TRN_DATA1_OUT : USB_TRN_DATA0_OUT;
					usb_ep_transf_start(EP(ep2), trn, ep2_buf_out, EP2_OUT_BUF_SIZE);
					ep2_dir = ep2_dir ? 0 : 1;
					ep2_state = 1;
				}
			break;
		}

		if(is_transfer_done()) {
			log_str("TRANSFER DONE\n");
			int ep_num = get_trn_status();
			if (ep_num == 0 || ep_num == 0x80) {
				process_control_transfer(ep_num);
				continue;
			}
			process_ep_transfer(ep_num);
			continue;
		}

		if(is_attached() == FALSE) {
			dev_state = DEV_DETACHED;
		}

		log_main_loop();
	}
	return 0;
}
